#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "fonts.h"

/*
 * screens enum
 */
enum ScreensEnum {
    _SCREEN_ID_FIRST   = 1,
    SCREEN_ID_SCR_HOME = 1,
    SCREEN_ID_HISTORY  = 2,
    SCREEN_ID_PATIENT  = 3,
    SCREEN_ID_SETTINGS = 4,
    _SCREEN_ID_LAST    = 4
};

/*
 * All LVGL objects referenced from main.c / screens.c
 */
typedef struct _objects_t {
    /* ── Screens ── */
    lv_obj_t *scr_home;
    lv_obj_t *history;
    lv_obj_t *patient;
    lv_obj_t *settings;

    /* ── Status bar / header containers ── */
    lv_obj_t *obj0;   /* status bar container */
    lv_obj_t *obj1;   /* 2×2 grid container   */

    /* ── FEV1 tile ── */
    lv_obj_t *fev1_val;  /* live FEV1 value label  "0.00" */
    lv_obj_t *obj2;      /* FEV1 progress bar              */
    lv_obj_t *obj3;      /* FEV1 % label                   */

    /* ── FVC tile ── */
    lv_obj_t *obj4;   /* FVC value label   */
    lv_obj_t *obj5;   /* FVC progress bar  */

    /* ── FEV1/FVC ratio tile ── */
    lv_obj_t *obj6;   /* ratio value label  */
    lv_obj_t *obj7;   /* ratio progress bar */

    /* ── PEF tile ── */
    lv_obj_t *obj8;   /* PEF value label   */
    lv_obj_t *obj9;   /* PEF progress bar  */

    /* ── Flow-Volume plot widget + axis labels ──
       fvl_chart is a plain lv_obj (not lv_chart) — main.c registers a
       LV_EVENT_DRAW_POST callback on it to draw smooth anti-aliased lines. */
    lv_obj_t *fvl_chart;       /* plot container                  */
    lv_obj_t *fvl_wait_lbl;    /* "Blow to see graph" placeholder */
    lv_obj_t *fvl_ylabel[4];   /* Y-axis labels top→bot           */
    lv_obj_t *fvl_xlabel[4];   /* X-axis labels left→right        */

    /* ── Volume-Time plot widget + axis labels (History screen) ── */
    lv_obj_t *vt_chart;        /* plot container                  */
    lv_obj_t *vt_wait_lbl;     /* "Blow to see graph" placeholder */
    lv_obj_t *vt_ylabel[4];    /* Y-axis labels top→bot           */
    lv_obj_t *vt_xlabel[4];    /* X-axis labels left→right        */

    /* ── Bottom nav ── */
    lv_obj_t *obj10;  /* nav bar container */
    lv_obj_t *obj11;  /* buttonmatrix      */

    /* ── Extended parameter label widgets (info strip below FVL chart) ── */
    lv_obj_t *te_val;          /* Expiratory Time value label, e.g. "5.8s"  */
    lv_obj_t *tpef_val;        /* TPEF value label, e.g. "0.12s"            */
    lv_obj_t *fef2575_val;     /* FEF25-75 value label, e.g. "3.67"         */
    lv_obj_t *fef50_val;       /* FEF50 value label, e.g. "4.21"            */
    lv_obj_t *sat_label;       /* Saturation flag label ("SAT!" or "")      */
    lv_obj_t *validity_label;  /* Maneuver validity label ("OK"/"TOO SHORT") */
} objects_t;

extern objects_t objects;

/* Screen creation / tick functions */
void create_screens(void);
void create_screen_scr_home(void);
void create_screen_history(void);
void create_screen_patient(void);
void create_screen_settings(void);

void tick_screen(int screen_index);
void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen_scr_home(void);
void tick_screen_history(void);
void tick_screen_patient(void);
void tick_screen_settings(void);

/* Font table (defined in screens.c) */
extern ext_font_desc_t fonts[];

/* Theme */
extern uint32_t active_theme_index;

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/
