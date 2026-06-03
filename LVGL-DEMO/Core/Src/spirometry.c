/*
 * spirometry.c
 *
 * Spirometry engine: acquisition → computation → GUI update.
 * Draw callbacks inspired by the full main.c reference implementation.
 */

#include "spirometry.h"
#include "ui/screens.h"
#include "spiro_classify.h"

#include "stm32f4xx_hal.h"
#include "adc.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

/* ── Patient parameters (set from patient screen before maneuver) ────────── */
int   patient_sex       = 0;     /* 0 = Male, 1 = Female                    */
float patient_age       = 25.0f; /* years                                    */
float patient_height_cm = 170.0f;/* cm                                       */

/* ── Forward declarations ───────────────────────────────────────────────── */
static void     do_compute(void);
static void     gui_update_metrics(void);
static void     gui_update_fvl_graph(void);
static void     gui_update_vt_graph(void);
static void     gui_reset_display(void);
static float    compute_fef_at_volume_fraction(float fraction, float fvc);
static uint16_t poll_adc_sample(void);

/* ── Sample buffers ─────────────────────────────────────────────────────── */
static uint16_t s_raw[SPIRO_BUF_MAX_SAMPLES];
static float    s_vol[SPIRO_BUF_MAX_SAMPLES];

static inline float raw_to_lps(uint16_t raw)
{
    int32_t delta = (int32_t)raw - (int32_t)SPIRO_ZERO_COUNTS;
    if (delta < 0) delta = 0;
    return (float)delta / SPIRO_COUNTS_PER_LPS;
}

static uint32_t      s_n          = 0;
static spiro_state_t s_state      = SPIRO_STATE_IDLE;
static spiro_result_t s_result;
static bool          s_has_result = false;

static uint32_t s_start_tick  = 0;
static uint32_t s_quiet_since = 0;
static bool     s_in_quiet    = false;
static bool     s_saturated   = false;

#define DT_S  (1.0f / (float)SPIRO_ADC_FS_HZ)

/* ── Axis scale state (set by gui_update_*_graph, read by draw callbacks) ─ */
/* FVL: volume axis 0..fvl_x_max_ml mL, flow axis 0..fvl_y_max_mlps mL/s   */
static int32_t fvl_x_max_ml   = 1000;   /* default 1 L   */
static int32_t fvl_y_max_mlps = 2000;   /* default 2 L/s */

/* VT: time axis 0..vt_t_max_ms ms, volume axis 0..vt_v_max_ml mL           */
static int32_t vt_t_max_ms = 6000;
static int32_t vt_v_max_ml = 1000;

/* ── FVL draw-post callback ─────────────────────────────────────────────── */
/*
 * Draws directly into the LVGL strip buffer — no intermediate layer.
 * Inspired by the reference main.c fvl_draw_cb:
 *   - light grid lines
 *   - green flow-volume curve
 *   - yellow dot at PEF
 */
static void fvl_draw_cb(lv_event_t *e)
{
    lv_obj_t   *obj   = lv_event_get_target(e);
    lv_layer_t *layer = lv_event_get_layer(e);

    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    int32_t ox = coords.x1;
    int32_t oy = coords.y1;
    int32_t W  = coords.x2 - coords.x1;
    int32_t H  = coords.y2 - coords.y1;

    /* ── Grid lines ── */
    lv_draw_line_dsc_t gdsc;
    lv_draw_line_dsc_init(&gdsc);
    gdsc.color       = lv_color_hex(0x1e2a40);
    gdsc.width       = 1;
    gdsc.round_start = 0;
    gdsc.round_end   = 0;
    for (int g = 1; g <= 3; g++) {
        gdsc.p1.x = ox;     gdsc.p1.y = oy + (H * g) / 4;
        gdsc.p2.x = ox + W; gdsc.p2.y = oy + (H * g) / 4;
        lv_draw_line(layer, &gdsc);
        gdsc.p1.x = ox + (W * g) / 4; gdsc.p1.y = oy;
        gdsc.p2.x = ox + (W * g) / 4; gdsc.p2.y = oy + H;
        lv_draw_line(layer, &gdsc);
    }

    if (!s_has_result || s_result.n_samples < 2) return;

    /* ── Flow-Volume curve with EMA smoothing ──
     *
     * Two-pass approach:
     *  1. EMA (alpha=0.15) forward pass on flow values removes high-freq noise
     *     while preserving the overall curve shape and PEF peak.
     *  2. Plot every STEP-th smoothed sample — limits segments to ~canvas width.
     *
     * EMA: fl_smooth = alpha*fl_raw + (1-alpha)*fl_smooth_prev
     * alpha=0.15 => strong smoothing; the FS1015 has an 8ms response so this
     * is well-matched (at 200Hz, 8ms = 1.6 samples — EMA tau >> sensor tau).
     */
    #define FVL_EMA_ALPHA  0.15f
    #define FVL_STEP       4

    lv_draw_line_dsc_t ldsc;
    lv_draw_line_dsc_init(&ldsc);
    ldsc.color       = lv_color_hex(0x00E5A0);
    ldsc.width       = 2;
    ldsc.round_start = 1;
    ldsc.round_end   = 1;

    bool    first  = true;
    int32_t px0 = 0, py0 = 0;
    float   fl_ema = 0.0f;           /* EMA state */
    uint32_t peak_i = 0;
    float    peak_f_ema = 0.0f;      /* EMA'd peak for PEF dot */

    uint32_t n = s_result.n_samples;
    for (uint32_t i = 0; i < n; i++) {
        float fl_raw = raw_to_lps(s_result.raw_buf[i]);
        if (fl_raw < 0.0f) fl_raw = 0.0f;

        /* EMA forward pass */
        if (i == 0) fl_ema = fl_raw;
        else        fl_ema = FVL_EMA_ALPHA * fl_raw + (1.0f - FVL_EMA_ALPHA) * fl_ema;

        /* Track EMA peak for PEF dot position */
        if (fl_ema > peak_f_ema) { peak_f_ema = fl_ema; peak_i = i; }

        /* Only draw every STEP-th point to limit line segment count */
        if (i % FVL_STEP != 0 && i != n-1) continue;

        float vl = s_result.vol_buf[i];
        if (vl < 0.0f) vl = 0.0f;

        int32_t px1 = ox + (int32_t)((vl    * 1000.0f * W) / fvl_x_max_ml);
        int32_t py1 = oy + H - (int32_t)((fl_ema * 1000.0f * H) / fvl_y_max_mlps);
        if (px1 < ox) px1 = ox; else if (px1 > ox+W) px1 = ox+W;
        if (py1 < oy) py1 = oy; else if (py1 > oy+H) py1 = oy+H;

        if (!first) {
            ldsc.p1.x = px0; ldsc.p1.y = py0;
            ldsc.p2.x = px1; ldsc.p2.y = py1;
            lv_draw_line(layer, &ldsc);
        }
        px0 = px1; py0 = py1;
        first = false;
    }
    #undef FVL_EMA_ALPHA
    #undef FVL_STEP

    /* ── PEF dot at the EMA-smoothed peak ── */
    if (peak_f_ema > 0.0f) {
        float pvl = s_result.vol_buf[peak_i];
        if (pvl < 0.0f) pvl = 0.0f;
        int32_t px = ox + (int32_t)((pvl        * 1000.0f * W) / fvl_x_max_ml);
        int32_t py = oy + H - (int32_t)((peak_f_ema * 1000.0f * H) / fvl_y_max_mlps);
        if (px < ox) px = ox; else if (px > ox+W) px = ox+W;
        if (py < oy) py = oy; else if (py > oy+H) py = oy+H;

        lv_draw_rect_dsc_t rdsc;
        lv_draw_rect_dsc_init(&rdsc);
        rdsc.bg_color = lv_color_hex(0xFFB020);
        rdsc.bg_opa   = LV_OPA_COVER;
        rdsc.radius   = LV_RADIUS_CIRCLE;
        lv_area_t dot = { px-3, py-3, px+3, py+3 };
        lv_draw_rect(layer, &rdsc, &dot);
    }
}


/* ── VT draw-post callback ──────────────────────────────────────────────── */
/*
 * Inspired by the reference main.c vt_draw_cb:
 *   - light grid lines
 *   - cyan volume-time curve
 */
static void vt_draw_cb(lv_event_t *e)
{
    lv_obj_t   *obj   = lv_event_get_target(e);
    lv_layer_t *layer = lv_event_get_layer(e);

    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    int32_t ox = coords.x1;
    int32_t oy = coords.y1;
    int32_t W  = coords.x2 - coords.x1;
    int32_t H  = coords.y2 - coords.y1;

    /* ── Grid lines ── */
    lv_draw_line_dsc_t gdsc;
    lv_draw_line_dsc_init(&gdsc);
    gdsc.color       = lv_color_hex(0x1e2a40);
    gdsc.width       = 1;
    gdsc.round_start = 0;
    gdsc.round_end   = 0;
    for (int g = 1; g <= 3; g++) {
        gdsc.p1.x = ox;     gdsc.p1.y = oy + (H * g) / 4;
        gdsc.p2.x = ox + W; gdsc.p2.y = oy + (H * g) / 4;
        lv_draw_line(layer, &gdsc);
        gdsc.p1.x = ox + (W * g) / 4; gdsc.p1.y = oy;
        gdsc.p2.x = ox + (W * g) / 4; gdsc.p2.y = oy + H;
        lv_draw_line(layer, &gdsc);
    }

    if (!s_has_result || s_result.n_samples < 2) return;

    /* ── Volume-Time curve with EMA smoothing ──
     * Volume is already integrated so it's inherently smoother than flow,
     * but we still apply a light EMA (alpha=0.3) and plot every 4th point. */
    lv_draw_line_dsc_t ldsc;
    lv_draw_line_dsc_init(&ldsc);
    ldsc.color       = lv_color_hex(0x00D4FF);
    ldsc.width       = 2;
    ldsc.round_start = 1;
    ldsc.round_end   = 1;

    #define VT_EMA_ALPHA  0.3f
    #define VT_STEP       4
    bool    first  = true;
    int32_t px0 = 0, py0 = 0;
    float   vl_ema = 0.0f;
    uint32_t n = s_result.n_samples;

    for (uint32_t i = 0; i < n; i++) {
        float vl_raw = s_result.vol_buf[i];
        if (vl_raw < 0.0f) vl_raw = 0.0f;

        if (i == 0) vl_ema = vl_raw;
        else        vl_ema = VT_EMA_ALPHA * vl_raw + (1.0f - VT_EMA_ALPHA) * vl_ema;

        if (i % VT_STEP != 0 && i != n-1) continue;

        int32_t t_ms = (int32_t)(i * (uint32_t)(DT_S * 1000.0f));
        int32_t px1 = ox + (t_ms * W) / vt_t_max_ms;
        int32_t py1 = oy + H - (int32_t)((vl_ema * 1000.0f * H) / vt_v_max_ml);
        if (px1 < ox) px1 = ox; else if (px1 > ox+W) px1 = ox+W;
        if (py1 < oy) py1 = oy; else if (py1 > oy+H) py1 = oy+H;

        if (!first) {
            ldsc.p1.x = px0; ldsc.p1.y = py0;
            ldsc.p2.x = px1; ldsc.p2.y = py1;
            lv_draw_line(layer, &ldsc);
        }
        px0 = px1; py0 = py1;
        first = false;
    }
    #undef VT_EMA_ALPHA
    #undef VT_STEP
}

/* ── spiro_init ─────────────────────────────────────────────────────────── */
void spiro_init(void)
{
    memset(&s_result, 0, sizeof(s_result));
    s_state      = SPIRO_STATE_IDLE;
    s_has_result = false;
    s_n          = 0;

    if (objects.fvl_chart)
        lv_obj_add_event_cb(objects.fvl_chart, fvl_draw_cb, LV_EVENT_DRAW_POST, NULL);
    if (objects.vt_chart)
        lv_obj_add_event_cb(objects.vt_chart, vt_draw_cb, LV_EVENT_DRAW_POST, NULL);

    gui_reset_display();
}

/* ── spiro_reset ────────────────────────────────────────────────────────── */
void spiro_reset(void)
{
    s_state      = SPIRO_STATE_IDLE;
    s_n          = 0;
    s_has_result = false;
    s_saturated  = false;
    s_in_quiet   = false;
    gui_reset_display();
}

const spiro_result_t *spiro_get_result(void)
{
    return s_has_result ? &s_result : NULL;
}
spiro_state_t spiro_get_state(void) { return s_state; }

/* ── ADC polling ────────────────────────────────────────────────────────── */
extern ADC_HandleTypeDef hadc1;

static uint16_t poll_adc_sample(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 2);
    uint16_t raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return raw;
}

bool spiro_push_sample(uint16_t adc_raw)
{
    if (s_n >= SPIRO_BUF_MAX_SAMPLES) return false;
    if (adc_raw >= SPIRO_ADC_MAX) s_saturated = true;
    float flow = raw_to_lps(adc_raw);
    float vol  = (s_n > 0) ? s_vol[s_n-1] + flow * DT_S : 0.0f;
    s_raw[s_n] = adc_raw;
    s_vol[s_n] = vol;
    s_n++;
    return true;
}

/* ── spiro_process ──────────────────────────────────────────────────────── */
void spiro_process(void)
{
    uint32_t now = HAL_GetTick();

    switch (s_state) {

    case SPIRO_STATE_IDLE: {
        uint16_t raw  = poll_adc_sample();
        float    flow = raw_to_lps(raw);
        if (flow >= SPIRO_BLOW_THRESH_LPS) {
            s_n=0; s_saturated=false; s_in_quiet=false; s_has_result=false;
            s_start_tick = now;
            s_state = SPIRO_STATE_ACQUIRING;
            if (objects.fvl_wait_lbl) lv_obj_add_flag(objects.fvl_wait_lbl, LV_OBJ_FLAG_HIDDEN);
            if (objects.vt_wait_lbl)  lv_obj_add_flag(objects.vt_wait_lbl,  LV_OBJ_FLAG_HIDDEN);
            spiro_push_sample(raw);
        }
        break;
    }

    case SPIRO_STATE_ACQUIRING: {
        uint16_t raw  = poll_adc_sample();
        spiro_push_sample(raw);
        float    flow    = raw_to_lps(raw);
        uint32_t elapsed = now - s_start_tick;

        if (elapsed >= SPIRO_MAX_DURATION_MS || s_n >= SPIRO_BUF_MAX_SAMPLES) {
            s_state = SPIRO_STATE_COMPUTING;
            break;
        }
        if (flow < SPIRO_END_THRESH_LPS) {
            if (!s_in_quiet) { s_in_quiet=true; s_quiet_since=now; }
            else if ((now - s_quiet_since) >= SPIRO_END_QUIET_MS)
                s_state = SPIRO_STATE_COMPUTING;
        } else {
            s_in_quiet = false;
        }
        break;
    }

    case SPIRO_STATE_COMPUTING:
        do_compute();
        gui_update_metrics();
        gui_update_fvl_graph();
        gui_update_vt_graph();
        s_state = SPIRO_STATE_DISPLAYING;
        break;

    case SPIRO_STATE_DISPLAYING: {
        uint16_t raw  = poll_adc_sample();
        float    flow = raw_to_lps(raw);
        if (flow >= SPIRO_BLOW_THRESH_LPS) {
            s_n=0; s_saturated=false; s_in_quiet=false; s_has_result=false;
            s_start_tick=now; s_state=SPIRO_STATE_ACQUIRING;
            spiro_push_sample(raw);
        }
        break;
    }

    default: s_state = SPIRO_STATE_IDLE; break;
    }
}

/* ── do_compute ─────────────────────────────────────────────────────────── */
static void do_compute(void)
{
    if (s_n == 0) return;

    uint32_t duration_ms = (uint32_t)(s_n * 1000u / SPIRO_ADC_FS_HZ);

    float fvc = 0.0f;
    for (uint32_t i = 0; i < s_n; i++) {
        float fl = raw_to_lps(s_raw[i]);
        if (fl > 0.0f) fvc += fl * DT_S;
    }

    float pef = 0.0f;
    uint32_t pef_idx = 0;
    for (uint32_t i = 0; i < s_n; i++) {
        float fl = raw_to_lps(s_raw[i]);
        if (fl > pef) { pef = fl; pef_idx = i; }
    }

    uint32_t fev1_samples = SPIRO_ADC_FS_HZ;
    if (fev1_samples > s_n) fev1_samples = s_n;
    float fev1 = 0.0f;
    for (uint32_t i = 0; i < fev1_samples; i++) {
        float fl = raw_to_lps(s_raw[i]);
        if (fl > 0.0f) fev1 += fl * DT_S;
    }

    float ratio = (fvc > 0.01f) ? (fev1 / fvc * 100.0f) : 0.0f;
    float tpef  = (float)pef_idx * DT_S;

    uint32_t te_idx = 0;
    for (uint32_t i = 0; i < s_n; i++)
        if (raw_to_lps(s_raw[i]) > SPIRO_END_THRESH_LPS) te_idx = i;
    float te = (float)(te_idx + 1) * DT_S;

    float fef2575 = 0.0f;
    {
        float v25=0.25f*fvc, v75=0.75f*fvc;
        uint32_t i25=0, i75=0;
        for (uint32_t i=0;i<s_n;i++){
            if (s_vol[i]<=v25) i25=i;
            if (s_vol[i]<=v75) i75=i;
        }
        if (i75>i25){
            float sum=0.0f;
            for(uint32_t i=i25;i<=i75;i++) sum+=raw_to_lps(s_raw[i]);
            fef2575=sum/(float)(i75-i25+1);
        }
    }
    float fef50 = compute_fef_at_volume_fraction(0.50f, fvc);

    bool valid = (duration_ms >= SPIRO_MIN_DURATION_MS) && (fvc >= 0.05f);

    s_result.fev1        = fev1;
    s_result.fvc         = fvc;
    s_result.ratio       = ratio;
    s_result.pef         = pef;
    s_result.te          = te;
    s_result.tpef        = tpef;
    s_result.fef2575     = fef2575;
    s_result.fef50       = fef50;
    s_result.saturated   = s_saturated;
    s_result.valid       = valid;
    s_result.n_samples   = s_n;
    s_result.duration_ms = duration_ms;
    s_result.raw_buf     = s_raw;
    s_result.vol_buf     = s_vol;
    s_has_result = true;
}

static float compute_fef_at_volume_fraction(float fraction, float fvc)
{
    float target = fraction * fvc;
    for (uint32_t i=1;i<s_n;i++){
        if (s_vol[i]>=target){
            float t=(target-s_vol[i-1])/(s_vol[i]-s_vol[i-1]+1e-9f);
            return raw_to_lps(s_raw[i-1]) + t*(raw_to_lps(s_raw[i])-raw_to_lps(s_raw[i-1]));
        }
    }
    return 0.0f;
}

/* ── gui_update_metrics ─────────────────────────────────────────────────── */
static void gui_update_metrics(void)
{
    char buf[16];

    /* FEV1 — integer-scaled to avoid printf_float */
    if (objects.fev1_val) {
        int v=(int)(s_result.fev1*100.0f+0.5f); if(v<0)v=0; if(v>800)v=800;
        snprintf(buf,sizeof(buf),"%d.%02d",v/100,v%100);
        lv_label_set_text(objects.fev1_val,buf);
    }
    if (objects.obj2) {
        int32_t pct=(int32_t)(s_result.fev1/1.5f*100.0f);
        if(pct>100)pct=100;
        lv_bar_set_value(objects.obj2,pct,LV_ANIM_ON);
    }
    if (objects.obj3) {
        int v=(int)(s_result.ratio+0.5f); if(v<0)v=0; if(v>100)v=100;
        snprintf(buf,sizeof(buf),"%d%%",v);
        lv_label_set_text(objects.obj3,buf);
    }

    /* FVC */
    if (objects.obj4) {
        int v=(int)(s_result.fvc*100.0f+0.5f); if(v<0)v=0; if(v>800)v=800;
        snprintf(buf,sizeof(buf),"%d.%02d",v/100,v%100);
        lv_label_set_text(objects.obj4,buf);
    }
    if (objects.obj5) {
        int32_t pct=(int32_t)(s_result.fvc/1.667f*100.0f);
        if(pct>100)pct=100;
        lv_bar_set_value(objects.obj5,pct,LV_ANIM_ON);
    }

    /* FEV1/FVC ratio */
    if (objects.obj6) {
        int v=(int)(s_result.ratio*10.0f+0.5f); if(v<0)v=0; if(v>1000)v=1000;
        snprintf(buf,sizeof(buf),"%d.%01d",v/10,v%10);
        lv_label_set_text(objects.obj6,buf);
    }
    if (objects.obj7) {
        int32_t pct=(int32_t)s_result.ratio; if(pct>100)pct=100;
        lv_bar_set_value(objects.obj7,pct,LV_ANIM_ON);
    }

    /* PEF */
    if (objects.obj8) {
        int v=(int)(s_result.pef*100.0f+0.5f); if(v<0)v=0; if(v>1500)v=1500;
        snprintf(buf,sizeof(buf),"%d.%02d",v/100,v%100);
        lv_label_set_text(objects.obj8,buf);
    }
    if (objects.obj9) {
        int32_t pct=(int32_t)(s_result.pef/1.667f*100.0f);
        if(pct>100)pct=100;
        lv_bar_set_value(objects.obj9,pct,LV_ANIM_ON);
    }

    /* Extended strip */
    if (objects.te_val) {
        int v=(int)(s_result.te*10.0f+0.5f); if(v<0)v=0; if(v>150)v=150;
        snprintf(buf,sizeof(buf),"%d.%01ds",v/10,v%10);
        lv_label_set_text(objects.te_val,buf);
    }
    if (objects.tpef_val) {
        int v=(int)(s_result.tpef*100.0f+0.5f); if(v<0)v=0; if(v>1500)v=1500;
        snprintf(buf,sizeof(buf),"%d.%02ds",v/100,v%100);
        lv_label_set_text(objects.tpef_val,buf);
    }
    if (objects.fef2575_val) {
        int v=(int)(s_result.fef2575*100.0f+0.5f); if(v<0)v=0; if(v>1500)v=1500;
        snprintf(buf,sizeof(buf),"%d.%02d",v/100,v%100);
        lv_label_set_text(objects.fef2575_val,buf);
    }
    if (objects.fef50_val) {
        int v=(int)(s_result.fef50*100.0f+0.5f); if(v<0)v=0; if(v>1500)v=1500;
        snprintf(buf,sizeof(buf),"%d.%02d",v/100,v%100);
        lv_label_set_text(objects.fef50_val,buf);
    }
    if (objects.sat_label)
        lv_label_set_text(objects.sat_label, s_result.saturated ? "SAT!" : "");
    if (objects.validity_label) {
        if (!s_result.valid) {
            lv_label_set_text(objects.validity_label, "SHORT");
        } else {
            SpiroResult cls = spiro_classify(patient_sex, patient_age,
                                             patient_height_cm,
                                             s_result.fev1, s_result.fvc);
            lv_label_set_text(objects.validity_label, cls.label);
        }
    }
}

/* ── gui_update_fvl_graph ───────────────────────────────────────────────── */
/*
 * Auto-scales axes using the 2:1 L/s:L aspect ratio (ATS/ERS / ISO 26782).
 * Updates axis labels, stores scale in fvl_x_max_ml / fvl_y_max_mlps,
 * then invalidates the chart so fvl_draw_cb fires.
 */
static void gui_update_fvl_graph(void)
{
    if (!objects.fvl_chart || s_result.n_samples == 0) return;

    /* Step 1: x_max from FVC, min 1 L, round up to 0.5 L */
    float x_max = s_result.fvc;
    if (x_max < 1.0f) x_max = 1.0f;
    x_max = ceilf(x_max / 0.5f) * 0.5f;

    /* Step 2: y_max for 2:1 aspect on the 210x100 canvas */
    float y_max = 2.0f * x_max * (100.0f / 210.0f);

    /* Step 3: ensure PEF fits */
    if (s_result.pef > y_max) {
        y_max = ceilf(s_result.pef / 0.5f) * 0.5f;
        x_max = y_max * (210.0f / (2.0f * 100.0f));
        x_max = ceilf(x_max / 0.5f) * 0.5f;
        y_max = 2.0f * x_max * (100.0f / 210.0f);
    }

    fvl_x_max_ml   = (int32_t)(x_max * 1000.0f);
    fvl_y_max_mlps = (int32_t)(y_max * 1000.0f);

    /* Update axis labels */
    char buf[16];
    float yv[4] = { y_max, y_max*2.0f/3.0f, y_max/3.0f, 0.0f };
    for (int i = 0; i < 4; i++) {
        int v10 = (int)(yv[i]*10.0f+0.5f);
        if (v10 < 0) v10 = 0;
        if (v10 > 999) v10 = 999;
        if (v10 % 10 == 0) snprintf(buf,sizeof(buf),"%d",   v10/10);
        else                snprintf(buf,sizeof(buf),"%d.%d",v10/10, v10%10);
        lv_label_set_text(objects.fvl_ylabel[i], buf);
    }
    float xv[4] = { 0.0f, x_max/3.0f, x_max*2.0f/3.0f, x_max };
    for (int i = 0; i < 4; i++) {
        int v10 = (int)(xv[i]*10.0f+0.5f);
        if (v10 < 0) v10 = 0;
        if (v10 > 999) v10 = 999;
        if (v10 == 0)       snprintf(buf,sizeof(buf),"0");
        else if (v10%10==0) snprintf(buf,sizeof(buf),"%d",   v10/10);
        else                snprintf(buf,sizeof(buf),"%d.%d",v10/10, v10%10);
        lv_label_set_text(objects.fvl_xlabel[i], buf);
    }

    if (objects.fvl_wait_lbl) lv_obj_add_flag(objects.fvl_wait_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(objects.fvl_chart);
}

/* ── gui_update_vt_graph ────────────────────────────────────────────────── */
static void gui_update_vt_graph(void)
{
    if (!objects.vt_chart || s_result.n_samples == 0) return;

    /* Time range: te_s rounded up to nearest second, min 6 s */
    float t_max = s_result.te;
    if (t_max < 6.0f)  t_max = 6.0f;
    if (t_max > 15.0f) t_max = 15.0f;
    t_max = ceilf(t_max);

    /* Volume range: fvc rounded up to 0.5 L, min 1 L */
    float v_max = s_result.fvc;
    if (v_max < 1.0f) v_max = 1.0f;
    v_max = ceilf(v_max / 0.5f) * 0.5f;

    /* 1L:1s aspect ratio on 214x148 canvas */
    float ratio_req = 148.0f / 214.0f;
    if ((v_max / t_max) < ratio_req) {
        v_max = t_max * ratio_req;
        v_max = ceilf(v_max / 0.5f) * 0.5f;
    } else {
        t_max = v_max / ratio_req;
        t_max = ceilf(t_max);
        if (t_max > 15.0f) t_max = 15.0f;
    }

    vt_t_max_ms = (int32_t)(t_max * 1000.0f);
    vt_v_max_ml = (int32_t)(v_max * 1000.0f);

    /* Update axis labels */
    char buf[16];
    float yv[4] = { v_max, v_max*2.0f/3.0f, v_max/3.0f, 0.0f };
    for (int i = 0; i < 4; i++) {
        int v10 = (int)(yv[i]*10.0f+0.5f);
        if (v10 < 0) v10 = 0;
        if (v10 > 999) v10 = 999;
        if (v10 % 10 == 0) snprintf(buf,sizeof(buf),"%d",   v10/10);
        else                snprintf(buf,sizeof(buf),"%d.%d",v10/10, v10%10);
        lv_label_set_text(objects.vt_ylabel[i], buf);
    }
    float xv[4] = { 0.0f, t_max/3.0f, t_max*2.0f/3.0f, t_max };
    for (int i = 0; i < 4; i++) {
        int v10 = (int)(xv[i]*10.0f+0.5f);
        if (v10 < 0) v10 = 0;
        if (v10 > 999) v10 = 999;
        if (v10 == 0)       snprintf(buf,sizeof(buf),"0");
        else if (v10%10==0) snprintf(buf,sizeof(buf),"%d",   v10/10);
        else                snprintf(buf,sizeof(buf),"%d.%d",v10/10, v10%10);
        lv_label_set_text(objects.vt_xlabel[i], buf);
    }

    if (objects.vt_wait_lbl) lv_obj_add_flag(objects.vt_wait_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(objects.vt_chart);
}

/* ── gui_reset_display ──────────────────────────────────────────────────── */
static void gui_reset_display(void)
{
    if (objects.fev1_val)  lv_label_set_text(objects.fev1_val, "0.00");
    if (objects.obj2)      lv_bar_set_value(objects.obj2, 0, LV_ANIM_OFF);
    if (objects.obj3)      lv_label_set_text(objects.obj3, "0%");
    if (objects.obj4)      lv_label_set_text(objects.obj4, "0.00");
    if (objects.obj5)      lv_bar_set_value(objects.obj5, 0, LV_ANIM_OFF);
    if (objects.obj6)      lv_label_set_text(objects.obj6, "0.0");
    if (objects.obj7)      lv_bar_set_value(objects.obj7, 0, LV_ANIM_OFF);
    if (objects.obj8)      lv_label_set_text(objects.obj8, "0.00");
    if (objects.obj9)      lv_bar_set_value(objects.obj9, 0, LV_ANIM_OFF);

    if (objects.te_val)        lv_label_set_text(objects.te_val,      "--");
    if (objects.tpef_val)      lv_label_set_text(objects.tpef_val,    "--");
    if (objects.fef2575_val)   lv_label_set_text(objects.fef2575_val, "--");
    if (objects.fef50_val)     lv_label_set_text(objects.fef50_val,   "--");
    if (objects.sat_label)     lv_label_set_text(objects.sat_label,   "");
    if (objects.validity_label)lv_label_set_text(objects.validity_label, "--");

    /* Reset axis labels to placeholder */
    for (int i = 0; i < 4; i++) {
        if (objects.fvl_ylabel[i]) lv_label_set_text(objects.fvl_ylabel[i], i==3?" 0":"--");
        if (objects.fvl_xlabel[i]) lv_label_set_text(objects.fvl_xlabel[i], i==0?"0":"-");
        if (objects.vt_ylabel[i])  lv_label_set_text(objects.vt_ylabel[i],  i==3?" 0":"--");
        if (objects.vt_xlabel[i])  lv_label_set_text(objects.vt_xlabel[i],  i==0?"0":"-");
    }

    if (objects.fvl_wait_lbl) lv_obj_clear_flag(objects.fvl_wait_lbl, LV_OBJ_FLAG_HIDDEN);
    if (objects.vt_wait_lbl)  lv_obj_clear_flag(objects.vt_wait_lbl,  LV_OBJ_FLAG_HIDDEN);

    if (objects.fvl_chart) lv_obj_invalidate(objects.fvl_chart);
    if (objects.vt_chart)  lv_obj_invalidate(objects.vt_chart);
}
