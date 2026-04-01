/*
 * spirometry.c
 *
 * Spirometry engine: acquisition → computation → GUI update.
 *
 * Designed for STM32 + LVGL 9.x, 240×320 display.
 * The engine is entirely poll-driven; no RTOS required.
 *
 * Integration checklist
 * ─────────────────────
 *  1. Add spirometry.c / spirometry.h to your IDE project.
 *  2. In main.c call spiro_init() once after ui_init().
 *  3. In main.c while(1) loop call spiro_process() after lv_timer_handler().
 *  4. If you use DMA continuous conversion, call spiro_push_sample() from
 *     your HAL_ADC_ConvCpltCallback / HAL_ADC_ConvHalfCpltCallback.
 *     Alternatively, leave the ADC calls in spiro_process() (polling mode —
 *     default in this file; see poll_adc_sample()).
 *  5. Adjust the #defines in spirometry.h for your sensor calibration.
 */

#include "spirometry.h"
#include "ui/screens.h"   /* objects_t objects  */

#include "stm32f4xx_hal.h"
#include "adc.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

/* ── Forward declarations ───────────────────────────────────────────────── */
static void  do_compute(void);
static void  gui_update_metrics(void);
static void  gui_update_fvl_graph(void);
static void  gui_update_vt_graph(void);
static void  gui_reset_display(void);
static float compute_fef_at_volume_fraction(float fraction, float fvc);
static uint16_t poll_adc_sample(void);

/* ── Internal state ─────────────────────────────────────────────────────── */

/* Static sample buffers — one maneuver at a time.
 * raw ADC uint16_t  × 1232 = 2464 bytes
 * volume  float     × 1232 = 4928 bytes
 * Total BSS                = 7392 bytes (~7.2 KB)               */
static uint16_t s_raw[SPIRO_BUF_MAX_SAMPLES];   /* raw ADC counts          */
static float    s_vol[SPIRO_BUF_MAX_SAMPLES];   /* integrated volume (L)   */

/* Inline helper: ADC count -> flow in L/s */
static inline float raw_to_lps(uint16_t raw)
{
    int32_t delta = (int32_t)raw - (int32_t)SPIRO_ZERO_COUNTS;
    if (delta < 0) delta = 0;   /* clamp negative (below zero-flow) */
    return (float)delta / SPIRO_COUNTS_PER_LPS;
}

static uint32_t      s_n          = 0;        /* samples filled   */
static spiro_state_t s_state      = SPIRO_STATE_IDLE;
static spiro_result_t s_result;
static bool          s_has_result = false;

/* Maneuver timing */
static uint32_t s_start_tick = 0;   /* HAL_GetTick() at first sample   */
static uint32_t s_quiet_since = 0;  /* HAL_GetTick() when flow fell low */
static bool     s_in_quiet    = false;
static bool     s_saturated   = false;

/* dt between samples */
#define DT_S  (1.0f / (float)SPIRO_ADC_FS_HZ)

/* ── FVL draw-post callback ─────────────────────────────────────────────── */
/*
 * LVGL calls this after every redraw of fvl_chart.
 * We draw the flow-volume curve on top of the background.
 */
static void fvl_draw_cb(lv_event_t *e)
{
    if (!s_has_result) return;

    lv_obj_t   *obj   = lv_event_get_target(e);
    lv_layer_t *layer = lv_event_get_layer(e);   /* LVGL 9 */

    lv_area_t obj_area;
    lv_obj_get_coords(obj, &obj_area);

    int32_t w = lv_obj_get_width(obj);
    int32_t h = lv_obj_get_height(obj);

    /* Map flow/volume sample → pixel */
    #define VOL_TO_X(v)  (obj_area.x1 + (int32_t)((v) / SPIRO_FVL_XMAX_L  * w))
    #define FLW_TO_Y(f)  (obj_area.y2 - (int32_t)((f) / SPIRO_FVL_YMAX_LPS * h))

    bool first = true;
    lv_point_precise_t prev = {0, 0};

    for (uint32_t i = 0; i < s_result.n_samples; i++) {
        float fl = raw_to_lps(s_result.raw_buf[i]);
        float vl = s_result.vol_buf[i];
        if (fl < 0.0f) fl = 0.0f;     /* expiratory portion only */
        if (vl < 0.0f) vl = 0.0f;

        lv_point_precise_t pt = {
            .x = VOL_TO_X(vl),
            .y = FLW_TO_Y(fl)
        };

        /* clamp to object bounds */
        if (pt.x < obj_area.x1) pt.x = obj_area.x1;
        if (pt.x > obj_area.x2) pt.x = obj_area.x2;
        if (pt.y < obj_area.y1) pt.y = obj_area.y1;
        if (pt.y > obj_area.y2) pt.y = obj_area.y2;

        if (!first) {
            /* LVGL 9: points live inside lv_draw_line_dsc_t */
            lv_draw_line_dsc_t line_dsc;
            lv_draw_line_dsc_init(&line_dsc);
            line_dsc.color       = lv_color_hex(0x00E5A0);  /* green */
            line_dsc.width       = 2;
            line_dsc.round_start = 1;
            line_dsc.round_end   = 1;
            line_dsc.p1          = prev;
            line_dsc.p2          = pt;
            lv_draw_line(layer, &line_dsc);
        }
        prev  = pt;
        first = false;
    }
    #undef VOL_TO_X
    #undef FLW_TO_Y
}

/* ── VT draw-post callback ──────────────────────────────────────────────── */
static void vt_draw_cb(lv_event_t *e)
{
    if (!s_has_result) return;

    lv_obj_t   *obj   = lv_event_get_target(e);
    lv_layer_t *layer = lv_event_get_layer(e);   /* LVGL 9 */

    lv_area_t obj_area;
    lv_obj_get_coords(obj, &obj_area);

    int32_t w = lv_obj_get_width(obj);
    int32_t h = lv_obj_get_height(obj);

    float t_max = (float)s_result.duration_ms / 1000.0f;
    if (t_max < 0.1f) return;

    #define TIME_TO_X(t)  (obj_area.x1 + (int32_t)((t) / SPIRO_VT_XMAX_S * w))
    #define VOL_TO_Y(v)   (obj_area.y2 - (int32_t)((v) / SPIRO_VT_YMAX_L  * h))

    bool first = true;
    lv_point_precise_t prev = {0, 0};

    for (uint32_t i = 0; i < s_result.n_samples; i++) {
        float t  = (float)i * DT_S;
        float vl = s_result.vol_buf[i];
        if (vl < 0.0f) vl = 0.0f;

        lv_point_precise_t pt = {
            .x = TIME_TO_X(t),
            .y = VOL_TO_Y(vl)
        };
        if (pt.x < obj_area.x1) pt.x = obj_area.x1;
        if (pt.x > obj_area.x2) pt.x = obj_area.x2;
        if (pt.y < obj_area.y1) pt.y = obj_area.y1;
        if (pt.y > obj_area.y2) pt.y = obj_area.y2;

        if (!first) {
            lv_draw_line_dsc_t line_dsc;
            lv_draw_line_dsc_init(&line_dsc);
            line_dsc.color       = lv_color_hex(0x00D4FF);  /* cyan */
            line_dsc.width       = 2;
            line_dsc.round_start = 1;
            line_dsc.round_end   = 1;
            line_dsc.p1          = prev;
            line_dsc.p2          = pt;
            lv_draw_line(layer, &line_dsc);
        }
        prev  = pt;
        first = false;
    }
    #undef TIME_TO_X
    #undef VOL_TO_Y
}

/* ── spiro_init ─────────────────────────────────────────────────────────── */
void spiro_init(void)
{
    memset(&s_result, 0, sizeof(s_result));
    s_state      = SPIRO_STATE_IDLE;
    s_has_result = false;
    s_n          = 0;

    /* Register draw callbacks on the chart containers */
    if (objects.fvl_chart) {
        lv_obj_add_event_cb(objects.fvl_chart, fvl_draw_cb,
                            LV_EVENT_DRAW_POST, NULL);
    }
    if (objects.vt_chart) {
        lv_obj_add_event_cb(objects.vt_chart, vt_draw_cb,
                            LV_EVENT_DRAW_POST, NULL);
    }

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

/* ── spiro_get_result / spiro_get_state ─────────────────────────────────── */
const spiro_result_t *spiro_get_result(void)
{
    return s_has_result ? &s_result : NULL;
}

spiro_state_t spiro_get_state(void) { return s_state; }

/* ── poll_adc_sample ────────────────────────────────────────────────────── */
/*
 * Blocking single-conversion poll.
 * Replace with your DMA path if preferred.
 * hadc1 must already be initialised by MX_ADC1_Init().
 */
extern ADC_HandleTypeDef hadc1;

static uint16_t poll_adc_sample(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 2);   /* 2 ms timeout */
    uint16_t raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return raw;
}

bool spiro_push_sample(uint16_t adc_raw)
{
    if (s_n >= SPIRO_BUF_MAX_SAMPLES) return false;

    if (adc_raw >= SPIRO_ADC_MAX) s_saturated = true;

    float flow = raw_to_lps(adc_raw);
    float vol  = (s_n > 0) ? s_vol[s_n - 1] + flow * DT_S : 0.0f;

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
            s_n          = 0;
            s_saturated  = false;
            s_in_quiet   = false;
            s_has_result = false;
            s_start_tick = now;
            s_state      = SPIRO_STATE_ACQUIRING;

            if (objects.fvl_wait_lbl)
                lv_obj_add_flag(objects.fvl_wait_lbl, LV_OBJ_FLAG_HIDDEN);
            if (objects.vt_wait_lbl)
                lv_obj_add_flag(objects.vt_wait_lbl, LV_OBJ_FLAG_HIDDEN);

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
            if (!s_in_quiet) {
                s_in_quiet    = true;
                s_quiet_since = now;
            } else if ((now - s_quiet_since) >= SPIRO_END_QUIET_MS) {
                s_state = SPIRO_STATE_COMPUTING;
            }
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
            s_n          = 0;
            s_saturated  = false;
            s_in_quiet   = false;
            s_has_result = false;
            s_start_tick = now;
            s_state      = SPIRO_STATE_ACQUIRING;
            spiro_push_sample(raw);
        }
        break;
    }

    default:
        s_state = SPIRO_STATE_IDLE;
        break;
    }
}

/* ── do_compute ─────────────────────────────────────────────────────────── */
static void do_compute(void)
{
    if (s_n == 0) return;

    uint32_t duration_ms = (uint32_t)(s_n * 1000u / SPIRO_ADC_FS_HZ);

    /* ── FVC: total exhaled volume (sum positive flow * dt) ── */
    float fvc = 0.0f;
    for (uint32_t i = 0; i < s_n; i++) {
        float fl = raw_to_lps(s_raw[i]);
        if (fl > 0.0f) fvc += fl * DT_S;
    }

    /* ── PEF: maximum instantaneous flow ── */
    float    pef     = 0.0f;
    uint32_t pef_idx = 0;
    for (uint32_t i = 0; i < s_n; i++) {
        float fl = raw_to_lps(s_raw[i]);
        if (fl > pef) { pef = fl; pef_idx = i; }
    }

    /* ── FEV1: volume exhaled in first 1 s ── */
    uint32_t fev1_samples = SPIRO_ADC_FS_HZ;
    if (fev1_samples > s_n) fev1_samples = s_n;
    float fev1 = 0.0f;
    for (uint32_t i = 0; i < fev1_samples; i++) {
        float fl = raw_to_lps(s_raw[i]);
        if (fl > 0.0f) fev1 += fl * DT_S;
    }

    float ratio = (fvc > 0.01f) ? (fev1 / fvc * 100.0f) : 0.0f;
    float tpef  = (float)pef_idx * DT_S;

    /* ── Te: last sample index with flow above end threshold ── */
    uint32_t te_idx = 0;
    for (uint32_t i = 0; i < s_n; i++) {
        if (raw_to_lps(s_raw[i]) > SPIRO_END_THRESH_LPS) te_idx = i;
    }
    float te = (float)(te_idx + 1) * DT_S;

    /* ── FEF25-75: mean flow between 25% and 75% of FVC ── */
    float fef2575 = 0.0f;
    {
        float    v25 = 0.25f * fvc, v75 = 0.75f * fvc;
        uint32_t i25 = 0, i75 = 0;
        for (uint32_t i = 0; i < s_n; i++) {
            if (s_vol[i] <= v25) i25 = i;
            if (s_vol[i] <= v75) i75 = i;
        }
        if (i75 > i25) {
            float sum = 0.0f;
            for (uint32_t i = i25; i <= i75; i++) sum += raw_to_lps(s_raw[i]);
            fef2575 = sum / (float)(i75 - i25 + 1);
        }
    }

    /* ── FEF50: flow at 50% of FVC (interpolated) ── */
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

/* Helper: interpolate flow at a given FVC fraction */
static float compute_fef_at_volume_fraction(float fraction, float fvc)
{
    float target = fraction * fvc;
    for (uint32_t i = 1; i < s_n; i++) {
        if (s_vol[i] >= target) {
            float t  = (target - s_vol[i-1]) / (s_vol[i] - s_vol[i-1] + 1e-9f);
            float f0 = raw_to_lps(s_raw[i-1]);
            float f1 = raw_to_lps(s_raw[i]);
            return f0 + t * (f1 - f0);
        }
    }
    return 0.0f;
}

/* ── gui_update_metrics ─────────────────────────────────────────────────── */
static void gui_update_metrics(void)
{
    char buf[16];

    /* FEV1 */
    if (objects.fev1_val) {
        snprintf(buf, sizeof(buf), "%.2f", s_result.fev1);
        lv_label_set_text(objects.fev1_val, buf);
    }
    if (objects.obj2) {
        /* FS1015 max 100 SLPM = 1.667 L/s; integrated over ~1s → max FEV1 ~1.5 L */
        int32_t pct = (int32_t)(s_result.fev1 / 1.5f * 100.0f);
        if (pct > 100) pct = 100;
        lv_bar_set_value(objects.obj2, pct, LV_ANIM_ON);
    }
    /* FEV1 % label (obj3 shows the ratio %) — reuse for FEV1 % predicted
       (we don't have predicted here, so show ratio) */
    if (objects.obj3) {
        snprintf(buf, sizeof(buf), "%.0f%%", s_result.ratio);
        lv_label_set_text(objects.obj3, buf);
    }

    /* FVC */
    if (objects.obj4) {
        snprintf(buf, sizeof(buf), "%.2f", s_result.fvc);
        lv_label_set_text(objects.obj4, buf);
    }
    if (objects.obj5) {
        /* FS1015 max 100 SLPM over ~6s maneuver → max FVC ~10 L; clamp to sensor limit */
        int32_t pct = (int32_t)(s_result.fvc / 1.667f * 100.0f);
        if (pct > 100) pct = 100;
        lv_bar_set_value(objects.obj5, pct, LV_ANIM_ON);
    }

    /* FEV1/FVC ratio */
    if (objects.obj6) {
        snprintf(buf, sizeof(buf), "%.1f", s_result.ratio);
        lv_label_set_text(objects.obj6, buf);
    }
    if (objects.obj7) {
        int32_t pct = (int32_t)s_result.ratio;
        if (pct > 100) pct = 100;
        lv_bar_set_value(objects.obj7, pct, LV_ANIM_ON);
    }

    /* PEF */
    if (objects.obj8) {
        snprintf(buf, sizeof(buf), "%.2f", s_result.pef);
        lv_label_set_text(objects.obj8, buf);
    }
    if (objects.obj9) {
        /* FS1015 max = 100 SLPM = 1.667 L/s; scale bar to that ceiling */
        int32_t pct = (int32_t)(s_result.pef / 1.667f * 100.0f);
        if (pct > 100) pct = 100;
        lv_bar_set_value(objects.obj9, pct, LV_ANIM_ON);
    }

    /* Extended parameter strip */
    if (objects.te_val) {
        snprintf(buf, sizeof(buf), "Te:%.1fs", s_result.te);
        lv_label_set_text(objects.te_val, buf);
    }
    if (objects.tpef_val) {
        snprintf(buf, sizeof(buf), "Tp:%.2fs", s_result.tpef);
        lv_label_set_text(objects.tpef_val, buf);
    }
    if (objects.fef2575_val) {
        snprintf(buf, sizeof(buf), "2575:%.2f", s_result.fef2575);
        lv_label_set_text(objects.fef2575_val, buf);
    }
    if (objects.fef50_val) {
        snprintf(buf, sizeof(buf), "F50:%.2f", s_result.fef50);
        lv_label_set_text(objects.fef50_val, buf);
    }

    /* Saturation flag */
    if (objects.sat_label) {
        lv_label_set_text(objects.sat_label,
                          s_result.saturated ? "SAT!" : "");
    }

    /* Validity */
    if (objects.validity_label) {
        lv_label_set_text(objects.validity_label,
                          s_result.valid ? "OK" : "TOO SHORT");
    }
}

/* ── gui_update_fvl_graph ───────────────────────────────────────────────── */
static void gui_update_fvl_graph(void)
{
    if (!objects.fvl_chart) return;
    /* Invalidate the canvas — fvl_draw_cb will repaint it */
    lv_obj_invalidate(objects.fvl_chart);
}

/* ── gui_update_vt_graph ────────────────────────────────────────────────── */
static void gui_update_vt_graph(void)
{
    if (!objects.vt_chart) return;
    lv_obj_invalidate(objects.vt_chart);
}

/* ── gui_reset_display ──────────────────────────────────────────────────── */
static void gui_reset_display(void)
{
    /* Metric tiles */
    if (objects.fev1_val)  lv_label_set_text(objects.fev1_val,  "0.00");
    if (objects.obj2)      lv_bar_set_value(objects.obj2, 0, LV_ANIM_OFF);
    if (objects.obj3)      lv_label_set_text(objects.obj3,       "--%");
    if (objects.obj4)      lv_label_set_text(objects.obj4,       "0.00");
    if (objects.obj5)      lv_bar_set_value(objects.obj5, 0, LV_ANIM_OFF);
    if (objects.obj6)      lv_label_set_text(objects.obj6,       "0.0");
    if (objects.obj7)      lv_bar_set_value(objects.obj7, 0, LV_ANIM_OFF);
    if (objects.obj8)      lv_label_set_text(objects.obj8,       "0.00");
    if (objects.obj9)      lv_bar_set_value(objects.obj9, 0, LV_ANIM_OFF);

    /* Extended strip */
    if (objects.te_val)        lv_label_set_text(objects.te_val,       "--");
    if (objects.tpef_val)      lv_label_set_text(objects.tpef_val,     "--");
    if (objects.fef2575_val)   lv_label_set_text(objects.fef2575_val,  "--");
    if (objects.fef50_val)     lv_label_set_text(objects.fef50_val,    "--");
    if (objects.sat_label)     lv_label_set_text(objects.sat_label,    "");
    if (objects.validity_label) lv_label_set_text(objects.validity_label, "");

    /* Show wait labels */
    if (objects.fvl_wait_lbl)
        lv_obj_clear_flag(objects.fvl_wait_lbl, LV_OBJ_FLAG_HIDDEN);
    if (objects.vt_wait_lbl)
        lv_obj_clear_flag(objects.vt_wait_lbl, LV_OBJ_FLAG_HIDDEN);

    /* Clear graph canvases */
    if (objects.fvl_chart) lv_obj_invalidate(objects.fvl_chart);
    if (objects.vt_chart)  lv_obj_invalidate(objects.vt_chart);
}
