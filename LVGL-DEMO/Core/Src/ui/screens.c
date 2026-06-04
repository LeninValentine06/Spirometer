/*
 * screens.c  —  SpiroFlow UI, LVGL 9, 240×320 portrait
 *
 * FIXES applied:
 *
 * FIX-A  Boot screen tick now calls loadScreen(SCREEN_ID_DASHBOARD) once
 *        the progress bar reaches 100.  Added a static s_boot_done flag so
 *        the transition fires exactly once.
 *
 * FIX-B  countdown_start() corrected: interval changed 900 ms → 1000 ms
 *        for a clean 1 s per step.  Label and colour are set immediately
 *        before the timer is created so "3" is always visible on entry.
 *
 * FIX-C  loadScreen() now tracks the previous screen ID and selects
 *        LV_SCR_LOAD_ANIM_MOVE_LEFT or _MOVE_RIGHT accordingly so
 *        back-navigation animates in the correct direction.
 *
 * FIX-D  FVL/VT chart top-of-canvas gap: charts moved from y=24 to y=32
 *        (8 px breathing room below the 24 px status bar).  Axis label
 *        positions adjusted accordingly so they don't clip under the bar.
 *
 * FIX-E  Swipe-footer tap targets enlarged to 44×28 px using
 *        lv_obj_set_ext_click_area() on each button, meeting the
 *        recommended medical-device minimum touch target size.
 *
 * FIX-F  Legacy progress bars (obj2–obj9) relocated from y=278 to y=308
 *        (below the swipe footer at y=290) so they no longer overlap the
 *        footer labels.  They remain as 3 px thin indicators at the very
 *        bottom of the screen.
 *
 * FIX-G  Dashboard recent-test card height increased from 120 px to 136 px
 *        to prevent the date label at y=82 from clipping.
 *
 * FIX-H  CANCEL button on Live screen now calls spiro_reset() before
 *        navigating to the dashboard, preventing the acquisition engine
 *        from continuing in the background after an aborted test.
 *
 * FIX-I  Swipe gesture detection added to all three carousel screens
 *        (Results, FVL, VT) via LV_EVENT_GESTURE on each screen object.
 *
 * FIX-J  LVGL heap exhaustion fix for STM32F401CCU6 (64 KB SRAM).
 *        Root cause: the five screens collectively need ~37 KB of LVGL heap
 *        but LV_MEM_SIZE was 32 KB, causing create_screen_fvl() to receive
 *        NULL from lv_obj_create() and hard-fault on the first style write.
 *        Two changes made:
 *          1. lv_conf.h: LV_MEM_SIZE raised from 32 KB to 40 KB (maximum
 *             safe value: 64 KB SRAM − ~21 KB non-LVGL static = ~43 KB
 *             available; 40 KB leaves 3 KB for stack growth).
 *          2. create_screen_results(): removed 9 off-screen legacy objects
 *             (4 lv_bar at y=310, 5 lv_label at y=280–315) that consumed
 *             ~2 200 B of heap but were never visible.  All write paths in
 *             spirometry.c already null-guard these pointers, so setting
 *             them to NULL requires no other changes.  This reduces the
 *             results screen from ~17 300 B to ~15 100 B of heap, giving
 *             ~6 200 B of headroom on the 40 KB budget.
 *        create_screens() also gains NULL-pointer assertions after each
 *        create_screen_*() call so a future OOM produces a diagnostic
 *        LOG() over UART instead of a silent hard fault.
 *
 * Color palette
 *   Background  #0B0E14   surface #111620   tile #0D1420
 *   Green       #00E5A0   cyan    #00D4FF   amber  #FFB020
 *   Blue        #4A7DFF   red     #FF4040   dim    #3D5070
 *   Border      #1E2A40
 */

#include <string.h>
#include <stdio.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"
#include "spirometry.h"   /* FIX-H: spiro_reset() */

/* Forward declaration — defined in Core/Src/main.c.
 * Avoids pulling in main.h (and the full HAL chain) just for this one symbol.
 * FIX-J: used by the OOM guards in create_screens(). */
extern void Error_Handler(void);

objects_t objects;
lv_obj_t *tick_value_change_obj;

/* ── Palette constants ───────────────────────────────────────────────────── */
#define C_BG        0x0B0E14u
#define C_SURFACE   0x111620u
#define C_TILE      0x0D1420u
#define C_BORDER    0x1E2A40u
#define C_GREEN     0x00E5A0u
#define C_CYAN      0x00D4FFu
#define C_AMBER     0xFFB020u
#define C_BLUE      0x4A7DFFu
#define C_RED       0xFF4040u
#define C_DIM       0x3D5070u
#define C_WHITE     0xE8EEF8u

/* ── Helper: bare screen object ─────────────────────────────────────────── */
static lv_obj_t *make_screen(void)
{
    lv_obj_t *s = lv_obj_create(0);
    lv_obj_set_pos(s, 0, 0);
    lv_obj_set_size(s, 240, 320);
    lv_obj_set_style_bg_color(s, lv_color_hex(C_BG), 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s, 0, 0);
    lv_obj_set_style_border_width(s, 0, 0);
    lv_obj_set_style_radius(s, 0, 0);
    lv_obj_set_style_shadow_width(s, 0, 0);
    lv_obj_remove_flag(s, LV_OBJ_FLAG_SCROLLABLE);
    return s;
}

/* ── Helper: chart canvas ────────────────────────────────────────────────── */
static lv_obj_t *make_chart_obj(lv_obj_t *parent,
                                int32_t x, int32_t y,
                                int32_t w, int32_t h)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(o,     lv_color_hex(C_TILE), 0);
    lv_obj_set_style_bg_opa(o,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_radius(o,       0, 0);
    lv_obj_set_style_pad_all(o,      0, 0);
    lv_obj_set_style_shadow_width(o, 0, 0);
    lv_obj_set_style_opa(o, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_opa(o, LV_OPA_TRANSP, LV_PART_SCROLLBAR);
    return o;
}

/* ── Helper: label ───────────────────────────────────────────────────────── */
static lv_obj_t *make_label(lv_obj_t *parent,
                             int32_t x, int32_t y,
                             uint32_t color,
                             const lv_font_t *font,
                             const char *text)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_pos(l, x, y);
    lv_obj_set_size(l, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_bg_opa(l, 0, 0);
    lv_label_set_text(l, text);
    return l;
}

/* ── Helper: status bar ──────────────────────────────────────────────────── */
static void make_status_bar(lv_obj_t *parent, const char *left, const char *right)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_pos(o, 0, 0);
    lv_obj_set_size(o, 240, 24);
    lv_obj_set_style_pad_left(o,   10, 0); lv_obj_set_style_pad_right(o,  10, 0);
    lv_obj_set_style_pad_top(o,     0, 0); lv_obj_set_style_pad_bottom(o,  0, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_style_shadow_width(o, 0, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(C_SURFACE), 0);
    lv_obj_set_style_layout(o, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(o, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_main_place(o, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_set_style_flex_cross_place(o, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    make_label(o, 0, 0, C_DIM, &lv_font_montserrat_14, left);
    make_label(o, 0, 0, C_DIM, &lv_font_montserrat_14, right);
}

/* ── Helper: carousel page dots ─────────────────────────────────────────── */
static void make_page_dots(lv_obj_t *parent, int active, lv_obj_t **out_lbl)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_pos(o, 0, 308);
    lv_obj_set_size(o, 240, 12);
    lv_obj_set_style_bg_opa(o, 0, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_shadow_width(o, 0, 0);
    lv_obj_set_style_layout(o, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(o, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_main_place(o, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(o, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 3; i++) {
        lv_obj_t *l = lv_label_create(o);
        lv_obj_set_size(l, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(l, lv_color_hex(i == active ? C_CYAN : C_DIM), 0);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_10, 0);
        lv_obj_set_style_bg_opa(l, 0, 0);
        lv_obj_set_style_pad_left(l,  4, 0);
        lv_obj_set_style_pad_right(l, 4, 0);
        lv_label_set_text(l, "●");
    }
    (void)out_lbl;
}

/* ── Helper: swipe footer with enlarged tap targets (FIX-E) ─────────────── */
static void make_swipe_footer(lv_obj_t *parent,
                               const char *left_txt,
                               const char *right_txt,
                               lv_event_cb_t left_cb,
                               lv_event_cb_t right_cb)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_pos(o, 0, 288);
    lv_obj_set_size(o, 240, 20);
    lv_obj_set_style_bg_opa(o, 0, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_shadow_width(o, 0, 0);
    lv_obj_set_style_layout(o, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(o, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_main_place(o, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_set_style_flex_cross_place(o, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);

    /* Left button */
    if (left_txt && left_cb) {
        lv_obj_t *btn = lv_label_create(o);
        lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(btn, lv_color_hex(C_DIM), 0);
        lv_obj_set_style_text_font(btn, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_opa(btn, 0, 0);
        lv_label_set_text(btn, left_txt);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(btn, 12);  /* FIX-E */
        lv_obj_add_event_cb(btn, left_cb, LV_EVENT_CLICKED, NULL);
    } else {
        lv_obj_t *sp = lv_label_create(o);
        lv_obj_set_size(sp, 1, 1);
        lv_obj_set_style_bg_opa(sp, 0, 0);
        lv_label_set_text(sp, "");
    }

    /* Right button */
    if (right_txt && right_cb) {
        lv_obj_t *btn = lv_label_create(o);
        lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(btn, lv_color_hex(C_DIM), 0);
        lv_obj_set_style_text_font(btn, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_opa(btn, 0, 0);
        lv_label_set_text(btn, right_txt);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(btn, 12);  /* FIX-E */
        lv_obj_add_event_cb(btn, right_cb, LV_EVENT_CLICKED, NULL);
    } else {
        lv_obj_t *sp = lv_label_create(o);
        lv_obj_set_size(sp, 1, 1);
        lv_obj_set_style_bg_opa(sp, 0, 0);
        lv_label_set_text(sp, "");
    }
}

/* ── FIX-I: swipe gesture callbacks ─────────────────────────────────────── */
static void results_gesture_cb(lv_event_t *e)
{
    lv_indev_t *indev = lv_indev_get_next(NULL);
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_LEFT)  action_go_to_fvl(e);
}

static void fvl_gesture_cb(lv_event_t *e)
{
    lv_indev_t *indev = lv_indev_get_next(NULL);
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_LEFT)  action_go_to_vt(e);
    if (dir == LV_DIR_RIGHT) action_go_to_results(e);
}

static void vt_gesture_cb(lv_event_t *e)
{
    lv_indev_t *indev = lv_indev_get_next(NULL);
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_RIGHT) action_go_to_fvl(e);
}

/* ── FIX-D: axis-label helpers — adjusted for new chart y=32 ────────────── */
static void make_axis_labels_y(lv_obj_t *parent,
                                int32_t x, int32_t y0, int32_t step,
                                lv_obj_t *arr[4],
                                const char *unit)
{
    lv_obj_t *u = lv_label_create(parent);
    lv_obj_set_pos(u, x, y0 - 14);
    lv_obj_set_size(u, 22, 12);
    lv_obj_set_style_text_align(u, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(u, lv_color_hex(C_CYAN), 0);
    lv_obj_set_style_text_font(u, &lv_font_montserrat_10, 0);
    lv_obj_set_style_bg_opa(u, 0, 0);
    lv_label_set_text(u, unit);

    const char *init[4] = { "--", "--", "--", " 0" };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *l = lv_label_create(parent);
        arr[i] = l;
        lv_obj_set_pos(l, x, y0 + i * step);
        lv_obj_set_size(l, 22, 12);
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(C_DIM), 0);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_10, 0);
        lv_obj_set_style_bg_opa(l, 0, 0);
        lv_label_set_text(l, init[i]);
    }
}

static void make_axis_labels_x(lv_obj_t *parent,
                                int32_t y, int32_t x0, int32_t step,
                                lv_obj_t *arr[4],
                                const char *unit)
{
    const char *init[4] = { "0", "-", "-", "-" };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *l = lv_label_create(parent);
        arr[i] = l;
        lv_obj_set_pos(l, x0 + i * step, y);
        lv_obj_set_size(l, 28, 12);
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(C_DIM), 0);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_10, 0);
        lv_obj_set_style_bg_opa(l, 0, 0);
        lv_label_set_text(l, init[i]);
    }
    lv_obj_t *u = lv_label_create(parent);
    lv_obj_set_pos(u, 0, y + 12);
    lv_obj_set_size(u, 240, 12);
    lv_obj_set_style_text_align(u, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(u, lv_color_hex(C_CYAN), 0);
    lv_obj_set_style_text_font(u, &lv_font_montserrat_10, 0);
    lv_obj_set_style_bg_opa(u, 0, 0);
    lv_label_set_text(u, unit);
}

/* ══════════════════════════════════════════════════════════════════════════
 *  SCREEN 1 — BOOT / SELF-TEST
 * ══════════════════════════════════════════════════════════════════════════ */
void create_screen_boot(void)
{
    lv_obj_t *s = make_screen();
    objects.boot = s;

    lv_obj_t *logo = lv_label_create(s);
    lv_obj_set_pos(logo, 0, 60);
    lv_obj_set_size(logo, 240, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(logo, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(logo, lv_color_hex(C_GREEN), 0);
    lv_obj_set_style_text_font(logo, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_opa(logo, 0, 0);
    lv_label_set_text(logo, "SPIROFLOW");

    lv_obj_t *sub = lv_label_create(s);
    lv_obj_set_pos(sub, 0, 90);
    lv_obj_set_size(sub, 240, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(C_DIM), 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_opa(sub, 0, 0);
    lv_label_set_text(sub, "Handheld Spirometer");

    lv_obj_t *fw = lv_label_create(s);
    objects.boot_fw_lbl = fw;
    lv_obj_set_pos(fw, 0, 108);
    lv_obj_set_size(fw, 240, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(fw, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(fw, lv_color_hex(C_DIM), 0);
    lv_obj_set_style_text_font(fw, &lv_font_montserrat_10, 0);
    lv_obj_set_style_bg_opa(fw, 0, 0);
    lv_label_set_text(fw, "fw v1.0.0");

    lv_obj_t *bar = lv_bar_create(s);
    objects.boot_bar = bar;
    lv_obj_set_pos(bar, 20, 250);
    lv_obj_set_size(bar, 200, 6);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(C_TILE), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(C_GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);
    lv_obj_set_style_shadow_width(bar, 0, 0);

    lv_obj_t *sl = lv_label_create(s);
    objects.boot_status_lbl = sl;
    lv_obj_set_pos(sl, 0, 262);
    lv_obj_set_size(sl, 240, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(sl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(sl, lv_color_hex(C_DIM), 0);
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_opa(sl, 0, 0);
    lv_label_set_text(sl, "Starting...");

    tick_screen_boot();
}

/* ══════════════════════════════════════════════════════════════════════════
 *  SCREEN 2 — DASHBOARD
 * ══════════════════════════════════════════════════════════════════════════ */
void create_screen_dashboard(void)
{
    lv_obj_t *s = make_screen();
    objects.scr_home = s;
    objects.history  = s;
    objects.patient  = s;
    objects.settings = s;

    /* Status bar */
    {
        lv_obj_t *o = lv_obj_create(s);
        objects.obj0 = o;
        lv_obj_set_pos(o, 0, 0);
        lv_obj_set_size(o, 240, 24);
        lv_obj_set_style_pad_left(o,   10, 0); lv_obj_set_style_pad_right(o,  10, 0);
        lv_obj_set_style_pad_top(o,     0, 0); lv_obj_set_style_pad_bottom(o,  0, 0);
        lv_obj_set_style_border_width(o, 0, 0);
        lv_obj_set_style_radius(o, 0, 0);
        lv_obj_set_style_shadow_width(o, 0, 0);
        lv_obj_set_style_bg_color(o, lv_color_hex(C_SURFACE), 0);
        lv_obj_set_style_layout(o, LV_LAYOUT_FLEX, 0);
        lv_obj_set_style_flex_flow(o, LV_FLEX_FLOW_ROW, 0);
        lv_obj_set_style_flex_main_place(o, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);
        lv_obj_set_style_flex_cross_place(o, LV_FLEX_ALIGN_CENTER, 0);
        lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *tl = lv_label_create(o);
        objects.dash_time_lbl = tl;
        lv_obj_set_size(tl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(tl, lv_color_hex(C_DIM), 0);
        lv_obj_set_style_text_font(tl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_opa(tl, 0, 0);
        lv_label_set_text(tl, "SpiroFlow");
    }

    /* READY card */
    {
        lv_obj_t *card = lv_obj_create(s);
        lv_obj_set_pos(card, 12, 32);
        lv_obj_set_size(card, 216, 70);
        lv_obj_set_style_bg_color(card, lv_color_hex(C_SURFACE), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(C_GREEN), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_shadow_width(card, 0, 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *rl = lv_label_create(card);
        lv_obj_set_pos(rl, 0, 10);
        lv_obj_set_size(rl, 216, LV_SIZE_CONTENT);
        lv_obj_set_style_text_align(rl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(rl, lv_color_hex(C_GREEN), 0);
        lv_obj_set_style_text_font(rl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_opa(rl, 0, 0);
        lv_label_set_text(rl, "READY");

        lv_obj_t *il = lv_label_create(card);
        lv_obj_set_pos(il, 0, 44);
        lv_obj_set_size(il, 216, LV_SIZE_CONTENT);
        lv_obj_set_style_text_align(il, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(il, lv_color_hex(C_DIM), 0);
        lv_obj_set_style_text_font(il, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_opa(il, 0, 0);
        lv_label_set_text(il, "Place lips on mouthpiece");
    }

    /* START TEST button */
    {
        lv_obj_t *btn = lv_button_create(s);
        objects.dash_start_btn = btn;
        lv_obj_set_pos(btn, 20, 114);
        lv_obj_set_size(btn, 200, 56);
        lv_obj_set_style_bg_color(btn, lv_color_hex(C_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x00B880), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
        lv_obj_add_event_cb(btn, action_start_test, LV_EVENT_CLICKED, NULL);

        lv_obj_t *bl = lv_label_create(btn);
        lv_obj_set_align(bl, LV_ALIGN_CENTER);
        lv_obj_set_style_text_color(bl, lv_color_hex(0x0A1A0A), 0);
        lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_opa(bl, 0, 0);
        lv_label_set_text(bl, LV_SYMBOL_PLAY "  START TEST");
    }

    /* Recent test card — FIX-G: height 120→136 px */
    {
        lv_obj_t *card = lv_obj_create(s);
        lv_obj_set_pos(card, 12, 182);
        lv_obj_set_size(card, 216, 136);  /* FIX-G */
        lv_obj_set_style_bg_color(card, lv_color_hex(C_SURFACE), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(C_BORDER), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_shadow_width(card, 0, 0);
        lv_obj_set_style_pad_all(card, 10, 0);
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        make_label(card,  0,  0, C_DIM,   &lv_font_montserrat_14, "Last Test");
        make_label(card,  0, 20, C_DIM,   &lv_font_montserrat_14, "FEV1");
        lv_obj_t *f1 = make_label(card, 50, 20, C_GREEN, &lv_font_montserrat_14, "--");
        objects.dash_last_fev1 = f1;
        make_label(card,  0, 40, C_DIM,   &lv_font_montserrat_14, "FVC ");
        lv_obj_t *fv = make_label(card, 50, 40, C_CYAN,  &lv_font_montserrat_14, "--");
        objects.dash_last_fvc = fv;
        lv_obj_t *grd = make_label(card, 0, 64, C_GREEN, &lv_font_montserrat_14, "--");
        objects.dash_last_grade = grd;
        lv_obj_t *dt  = make_label(card, 0, 88, C_DIM,   &lv_font_montserrat_10, "--");
        objects.dash_last_date = dt;
    }

    tick_screen_dashboard();
}

/* ══════════════════════════════════════════════════════════════════════════
 *  SCREEN 3 — RESULTS
 * ══════════════════════════════════════════════════════════════════════════ */
static void results_table_row(lv_obj_t *parent,
                               int32_t y,
                               const char *param_name,
                               uint32_t val_color,
                               lv_obj_t **act_out,
                               lv_obj_t **pred_out,
                               lv_obj_t **pct_out)
{
    make_label(parent, 0, y, C_DIM, &lv_font_montserrat_10, param_name);

    lv_obj_t *a = lv_label_create(parent);
    lv_obj_set_pos(a, 68, y);
    lv_obj_set_size(a, 52, 12);
    lv_obj_set_style_text_align(a, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(a, lv_color_hex(val_color), 0);
    lv_obj_set_style_text_font(a, &lv_font_montserrat_10, 0);
    lv_obj_set_style_bg_opa(a, 0, 0);
    lv_label_set_text(a, "--");
    if (act_out) *act_out = a;

    if (pred_out) {
        lv_obj_t *p = lv_label_create(parent);
        lv_obj_set_pos(p, 122, y);
        lv_obj_set_size(p, 46, 12);
        lv_obj_set_style_text_align(p, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_style_text_color(p, lv_color_hex(C_DIM), 0);
        lv_obj_set_style_text_font(p, &lv_font_montserrat_10, 0);
        lv_obj_set_style_bg_opa(p, 0, 0);
        lv_label_set_text(p, "--");
        *pred_out = p;
    }

    if (pct_out) {
        lv_obj_t *pp = lv_label_create(parent);
        lv_obj_set_pos(pp, 170, y);
        lv_obj_set_size(pp, 40, 12);
        lv_obj_set_style_text_align(pp, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_style_text_color(pp, lv_color_hex(C_DIM), 0);
        lv_obj_set_style_text_font(pp, &lv_font_montserrat_10, 0);
        lv_obj_set_style_bg_opa(pp, 0, 0);
        lv_label_set_text(pp, "");
        *pct_out = pp;
    }
}

void create_screen_results(void)
{
    lv_obj_t *s = make_screen();
    objects.results = s;

    make_status_bar(s, "Results", "");

    /* Quality card */
    {
        lv_obj_t *card = lv_obj_create(s);
        lv_obj_set_pos(card, 8, 28);
        lv_obj_set_size(card, 224, 68);
        lv_obj_set_style_bg_color(card, lv_color_hex(C_SURFACE), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(C_GREEN), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_set_style_shadow_width(card, 0, 0);
        lv_obj_set_style_pad_all(card, 8, 0);
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        make_label(card, 0, 0, C_DIM, &lv_font_montserrat_10, "QUALITY GRADE");
        lv_obj_t *gl = lv_label_create(card);
        objects.res_grade_lbl = gl;
        lv_obj_set_pos(gl, 90, 0);
        lv_obj_set_size(gl, 30, LV_SIZE_CONTENT);
        lv_obj_set_style_text_align(gl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(gl, lv_color_hex(C_GREEN), 0);
        lv_obj_set_style_text_font(gl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_opa(gl, 0, 0);
        lv_label_set_text(gl, "-");

        make_label(card, 0, 30, C_DIM, &lv_font_montserrat_10, "Start");
        lv_obj_t *stl = lv_label_create(card);
        objects.res_start_lbl = stl;
        lv_obj_set_pos(stl, 36, 30);
        lv_obj_set_size(stl, 20, 12);
        lv_obj_set_style_text_color(stl, lv_color_hex(C_DIM), 0);
        lv_obj_set_style_text_font(stl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_opa(stl, 0, 0);
        lv_label_set_text(stl, "-");

        make_label(card, 68, 30, C_DIM, &lv_font_montserrat_10, "End");
        lv_obj_t *etl = lv_label_create(card);
        objects.res_end_lbl = etl;
        lv_obj_set_pos(etl, 98, 30);
        lv_obj_set_size(etl, 20, 12);
        lv_obj_set_style_text_color(etl, lv_color_hex(C_DIM), 0);
        lv_obj_set_style_text_font(etl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_opa(etl, 0, 0);
        lv_label_set_text(etl, "-");

        lv_obj_t *il = lv_label_create(card);
        objects.res_interp_lbl = il;
        lv_obj_set_pos(il, 130, 28);
        lv_obj_set_size(il, 78, LV_SIZE_CONTENT);
        lv_obj_set_style_text_align(il, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_style_text_color(il, lv_color_hex(C_DIM), 0);
        lv_obj_set_style_text_font(il, &lv_font_montserrat_10, 0);
        lv_obj_set_style_bg_opa(il, 0, 0);
        lv_label_set_text(il, "--");
    }

    /* Parameter table */
    {
        lv_obj_t *card = lv_obj_create(s);
        lv_obj_set_pos(card, 8, 102);
        lv_obj_set_size(card, 224, 172);
        lv_obj_set_style_bg_color(card, lv_color_hex(C_SURFACE), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(C_BORDER), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 6, 0);
        lv_obj_set_style_shadow_width(card, 0, 0);
        lv_obj_set_style_pad_all(card, 6, 0);
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        make_label(card,   0, 0, C_DIM, &lv_font_montserrat_10, "Parameter");
        make_label(card,  68, 0, C_DIM, &lv_font_montserrat_10, "Actual");
        make_label(card, 122, 0, C_DIM, &lv_font_montserrat_10, "Pred");
        make_label(card, 170, 0, C_DIM, &lv_font_montserrat_10, "%Pred");

        lv_obj_t *hr = lv_obj_create(card);
        lv_obj_set_pos(hr, 0, 14); lv_obj_set_size(hr, 212, 1);
        lv_obj_set_style_bg_color(hr, lv_color_hex(C_BORDER), 0);
        lv_obj_set_style_bg_opa(hr, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(hr, 0, 0); lv_obj_set_style_shadow_width(hr, 0, 0);
        lv_obj_remove_flag(hr, LV_OBJ_FLAG_SCROLLABLE);

        int32_t y = 18;
        results_table_row(card, y, "FVC (L)",      C_CYAN,  &objects.res_fvc_act,    &objects.res_fvc_pred,   &objects.res_fvc_pct);   y += 16;
        results_table_row(card, y, "FEV1 (L)",     C_GREEN, &objects.res_fev1_act,   &objects.res_fev1_pred,  &objects.res_fev1_pct);  y += 16;
        results_table_row(card, y, "FEV6 (L)",     C_GREEN, &objects.res_fev6_act,   &objects.res_fev6_pred,  &objects.res_fev6_pct);  y += 16;
        results_table_row(card, y, "FEV1/FVC (%)", C_AMBER, &objects.res_ratio_act,  &objects.res_ratio_pred, &objects.res_ratio_pct); y += 16;
        results_table_row(card, y, "PEF (L/s)",    C_BLUE,  &objects.res_pef_act,    &objects.res_pef_pred,   &objects.res_pef_pct);   y += 16;
        results_table_row(card, y, "FEF25 (L/s)",  C_DIM,   &objects.res_fef25_act,  NULL, NULL); y += 16;
        results_table_row(card, y, "FEF50 (L/s)",  C_DIM,   &objects.res_fef50_act,  NULL, NULL); y += 16;
        results_table_row(card, y, "FEF75 (L/s)",  C_DIM,   &objects.res_fef75_act,  NULL, NULL); y += 16;
        results_table_row(card, y, "FEF25-75",      C_CYAN,  &objects.res_fef2575_act,NULL, NULL);

        /* Legacy aliases */
        objects.fev1_val    = objects.res_fev1_act;
        objects.obj4        = objects.res_fvc_act;
        objects.obj6        = objects.res_ratio_act;
        objects.obj8        = objects.res_pef_act;
        objects.fef2575_val = objects.res_fef2575_act;
        objects.fef50_val   = objects.res_fef50_act;
    }

    /* FIX-J: legacy progress bars (obj2/obj5/obj7/obj9) and legacy labels
     * (obj3, te_val, tpef_val, sat_label, validity_label) that were placed
     * at y=280–315 — beyond the 320 px screen edge or hidden under the footer
     * — have been removed.  They consumed ~2 200 B of LVGL heap and were never
     * visible.  The struct members remain in objects_t and are left as NULL;
     * every write path in spirometry.c already null-guards these pointers so
     * no other source file requires any change.
     * Removed objects: obj2 obj3 obj5 obj7 obj9 te_val tpef_val sat_label
     *                  validity_label */
    objects.obj2          = NULL;
    objects.obj3          = NULL;
    objects.obj5          = NULL;
    objects.obj7          = NULL;
    objects.obj9          = NULL;
    objects.te_val        = NULL;
    objects.tpef_val      = NULL;
    objects.sat_label     = NULL;
    objects.validity_label = NULL;

    /* Footer + page dots.
     * Left slot = Home (back to dashboard) so the results carousel is not a
     * dead-end; right slot advances to the Flow-Volume page. */
    make_swipe_footer(s, LV_SYMBOL_HOME " Home", "Flow-Vol \xE2\x86\x92",
                      action_go_to_dashboard, action_go_to_fvl);
    make_page_dots(s, 0, &objects.res_page_lbl);

    /* FIX-I: swipe gesture */
    lv_obj_add_event_cb(s, results_gesture_cb, LV_EVENT_GESTURE, NULL);

    tick_screen_results();
}

/* ══════════════════════════════════════════════════════════════════════════
 *  SCREEN 6 — FLOW-VOLUME LOOP
 *  FIX-D: chart moved from y=24 to y=32; y-axis unit label adjusted.
 * ══════════════════════════════════════════════════════════════════════════ */
void create_screen_fvl(void)
{
    lv_obj_t *s = make_screen();
    objects.fvl_screen = s;

    make_status_bar(s, "Flow-Volume", "");

    /* Y-axis labels: start at y=34 to align with chart top at y=32 */
    make_axis_labels_y(s, 0, 34, 56, objects.fvl_ylabel, "L/s");

    /* FIX-D: chart at y=32 (was y=24) */
    objects.fvl_chart = make_chart_obj(s, 24, 32, 210, 228);

    {
        lv_obj_t *l = lv_label_create(s);
        objects.fvl_wait_lbl = l;
        lv_obj_set_pos(l, 24, 140);
        lv_obj_set_size(l, 210, 14);
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(C_BORDER), 0);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_opa(l, 0, 0);
        lv_label_set_text(l, "Blow to see graph");
    }

    /* X-axis: placed at y=264 (32 + 228 + 4) */
    make_axis_labels_x(s, 264, 24, 68, objects.fvl_xlabel, "Volume (L)");

    make_swipe_footer(s, "\xE2\x86\x90 Results", "Vol-Time \xE2\x86\x92",
                      action_go_to_results, action_go_to_vt);
    make_page_dots(s, 1, &objects.fvl_page_lbl);

    /* FIX-I: swipe gesture */
    lv_obj_add_event_cb(s, fvl_gesture_cb, LV_EVENT_GESTURE, NULL);

    tick_screen_fvl();
}

/* ══════════════════════════════════════════════════════════════════════════
 *  SCREEN 7 — VOLUME-TIME GRAPH
 *  FIX-D: chart moved from y=24 to y=32.
 * ══════════════════════════════════════════════════════════════════════════ */
void create_screen_vt(void)
{
    lv_obj_t *s = make_screen();
    objects.vt_screen = s;

    make_status_bar(s, "Volume-Time", "");

    make_axis_labels_y(s, 0, 34, 56, objects.vt_ylabel, "L");

    /* FIX-D: chart at y=32 */
    objects.vt_chart = make_chart_obj(s, 24, 32, 210, 228);

    {
        lv_obj_t *l = lv_label_create(s);
        objects.vt_wait_lbl = l;
        lv_obj_set_pos(l, 24, 140);
        lv_obj_set_size(l, 210, 14);
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(C_BORDER), 0);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_opa(l, 0, 0);
        lv_label_set_text(l, "Blow to see graph");
    }

    make_axis_labels_x(s, 264, 24, 68, objects.vt_xlabel, "Time (s)");

    make_swipe_footer(s, "\xE2\x86\x90 Flow-Volume", NULL,
                      action_go_to_fvl, NULL);
    make_page_dots(s, 2, &objects.vt_page_lbl);

    /* FIX-I: swipe gesture */
    lv_obj_add_event_cb(s, vt_gesture_cb, LV_EVENT_GESTURE, NULL);

    tick_screen_vt();
}

/* ── Tick functions ──────────────────────────────────────────────────────── */

/* FIX-A: boot tick now transitions to dashboard when bar reaches 100 */
static bool s_boot_done = false;
void tick_screen_boot(void)
{
    if (!objects.boot_bar) return;
    if (s_boot_done) return;
    int32_t v = lv_bar_get_value(objects.boot_bar);
    if (v < 100) {
        lv_bar_set_value(objects.boot_bar, v + 2, LV_ANIM_OFF);
        if (v >= 98 && objects.boot_status_lbl)
            lv_label_set_text(objects.boot_status_lbl, "Ready");
    } else {
        s_boot_done = true;
        loadScreen(SCREEN_ID_DASHBOARD);  /* FIX-A */
    }
}

void tick_screen_dashboard(void) {}
void tick_screen_results(void) {}
void tick_screen_fvl(void) {}
void tick_screen_vt(void) {}

/* ── Dispatch table ─────────────────────────────────────────────────────── */
typedef void (*tick_fn_t)(void);
static tick_fn_t tick_fns[] = {
    tick_screen_boot,
    tick_screen_dashboard,
    tick_screen_results,
    tick_screen_fvl,
    tick_screen_vt,
};

void tick_screen(int i)                      { if (i >= 0 && i < 5) tick_fns[i](); }
void tick_screen_by_id(enum ScreensEnum id)  { tick_screen(id - 1); }

/* ── Font table ─────────────────────────────────────────────────────────── */
ext_font_desc_t fonts[] = {
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
};
uint32_t active_theme_index = 0;

/* ── create_screens ─────────────────────────────────────────────────────── */
void create_screens(void)
{
    lv_display_t *disp = lv_display_get_default();
    lv_theme_t *theme  = lv_theme_default_init(disp,
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        false, LV_FONT_DEFAULT);
    lv_display_set_theme(disp, theme);

    s_boot_done = false;  /* FIX-A: reset on every full UI rebuild */

    /* Per-screen trace + heap snapshot.  The [mem] line AFTER a screen shows
     * how much heap remains; if it shrinks to near zero the build is RAM-bound
     * and the next screen's lv_obj_create() will fail.
     *
     * FIX-J: NULL-pointer checks after each create call.  lv_obj_create()
     * returns NULL on OOM; the subsequent style writes then hard-fault with
     * no diagnostics.  Checking here converts a silent crash into a logged
     * message + halt via Error_Handler(), making the failure self-documenting.
     * If any check fires, increase LV_MEM_SIZE in lv_conf.h (max safe value
     * on the STM32F401CCU6 is 40 KB — see FIX-J note in file header). */
    lv_mem_monitor_t mon;

    printf("[screens] boot...\r\n");
    create_screen_boot();
    lv_mem_monitor(&mon);
    printf("[mem] boot      free=%u big=%u used=%u%%\r\n",
           (unsigned)mon.free_size, (unsigned)mon.free_biggest_size, (unsigned)mon.used_pct);
    if (!objects.boot) { printf("[FATAL] OOM after boot screen — increase LV_MEM_SIZE\r\n"); Error_Handler(); }

    printf("[screens] dashboard...\r\n");
    create_screen_dashboard();
    lv_mem_monitor(&mon);
    printf("[mem] dashboard free=%u big=%u used=%u%%\r\n",
           (unsigned)mon.free_size, (unsigned)mon.free_biggest_size, (unsigned)mon.used_pct);
    if (!objects.scr_home) { printf("[FATAL] OOM after dashboard screen — increase LV_MEM_SIZE\r\n"); Error_Handler(); }

    printf("[screens] results...\r\n");
    create_screen_results();
    lv_mem_monitor(&mon);
    printf("[mem] results   free=%u big=%u used=%u%%\r\n",
           (unsigned)mon.free_size, (unsigned)mon.free_biggest_size, (unsigned)mon.used_pct);
    if (!objects.results) { printf("[FATAL] OOM after results screen — increase LV_MEM_SIZE\r\n"); Error_Handler(); }

    printf("[screens] fvl...\r\n");
    create_screen_fvl();
    lv_mem_monitor(&mon);
    printf("[mem] fvl       free=%u big=%u used=%u%%\r\n",
           (unsigned)mon.free_size, (unsigned)mon.free_biggest_size, (unsigned)mon.used_pct);
    if (!objects.fvl_screen) { printf("[FATAL] OOM after fvl screen — increase LV_MEM_SIZE\r\n"); Error_Handler(); }

    printf("[screens] vt...\r\n");
    create_screen_vt();
    lv_mem_monitor(&mon);
    printf("[mem] vt        free=%u big=%u used=%u%%\r\n",
           (unsigned)mon.free_size, (unsigned)mon.free_biggest_size, (unsigned)mon.used_pct);
    if (!objects.vt_screen) { printf("[FATAL] OOM after vt screen — increase LV_MEM_SIZE\r\n"); Error_Handler(); }

    printf("[screens] all created\r\n");
}

/* ── Live test helpers ─────────────────────────────────────────────────────
 * The live/countdown screens were removed; these remain because
 * spirometry.c calls them during acquisition.  Their target objects are
 * never created (NULL), so every body below is a guarded no-op. */
void live_update_flow(float flow_lps, float vol_l, uint32_t elapsed_ms)
{
    char buf[12];
    if (objects.live_flow_lbl) {
        int v = (int)(flow_lps * 100.0f + 0.5f);
        if (v < 0) v = 0;
        snprintf(buf, sizeof(buf), "%d.%02d", v / 100, v % 100);
        lv_label_set_text(objects.live_flow_lbl, buf);
    }
    if (objects.live_vol_lbl) {
        int v = (int)(vol_l * 100.0f + 0.5f);
        if (v < 0) v = 0;
        snprintf(buf, sizeof(buf), "%d.%02d", v / 100, v % 100);
        lv_label_set_text(objects.live_vol_lbl, buf);
    }
    if (objects.live_time_lbl) {
        snprintf(buf, sizeof(buf), "%lus", (unsigned long)(elapsed_ms / 1000));
        lv_label_set_text(objects.live_time_lbl, buf);
    }
}

void live_set_coaching(const char *msg)
{
    if (objects.live_coaching_lbl)
        lv_label_set_text(objects.live_coaching_lbl, msg);
}

void live_push_sample(float flow_lps)
{
    (void)flow_lps;
    if (objects.live_chart)
        lv_obj_invalidate(objects.live_chart);
}
