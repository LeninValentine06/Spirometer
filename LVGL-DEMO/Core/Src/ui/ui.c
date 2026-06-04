/*
 * ui.c  —  SpiroFlow UI entry point and screen loader
 *
 * FIX-C: loadScreen() now tracks the previous screen ID and selects
 *         LV_SCR_LOAD_ANIM_MOVE_LEFT or LV_SCR_LOAD_ANIM_MOVE_RIGHT
 *         based on whether we are moving to a higher or lower screen ID.
 *         This gives correct left/right animation throughout the carousel
 *         and when navigating dashboard ↔ results.
 */

#include "ui.h"
#include "screens.h"
#include "images.h"
#include "actions.h"
#include "vars.h"

#include <string.h>
#include <stdio.h>   /* diagnostic traces */

static int16_t currentScreen  = -1;
static int16_t previousScreen = -1;   /* FIX-C */

static lv_obj_t *getLvglObjectFromIndex(int32_t index)
{
    if (index < 0) return NULL;
    lv_obj_t *screen_ptrs[] = {
        objects.boot,        /* 0 → SCREEN_ID_BOOT      */
        objects.scr_home,    /* 1 → SCREEN_ID_DASHBOARD */
        objects.results,     /* 2 → SCREEN_ID_RESULTS   */
        objects.fvl_screen,  /* 3 → SCREEN_ID_FVL       */
        objects.vt_screen,   /* 4 → SCREEN_ID_VT        */
    };
    if (index < 5) return screen_ptrs[index];
    return NULL;
}

/* FIX-C: choose animation direction based on screen order */
void loadScreen(enum ScreensEnum screenId)
{
    int16_t nextScreen = (int16_t)(screenId - 1);
    lv_obj_t *screen   = getLvglObjectFromIndex(nextScreen);

    if (screen) {
        if (currentScreen < 0) {
            /* First screen load after boot.  The previously-active screen is
             * the calibration screen, which touch_cal_run() has already
             * deleted.  lv_scr_load_anim() would animate a 200 ms transition
             * FROM that now-freed object (use-after-free → hard fault), so
             * load directly with no transition for the very first screen. */
            lv_scr_load(screen);
        } else {
            lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_MOVE_LEFT; /* forward */
            if (previousScreen >= 0 && nextScreen < currentScreen) {
                /* Moving to a lower-indexed screen = backward = slide right */
                anim = LV_SCR_LOAD_ANIM_MOVE_RIGHT;
            }
            lv_scr_load_anim(screen, anim, 200, 0, false);
        }
    }

    previousScreen = currentScreen;
    currentScreen  = nextScreen;
}

void ui_init(void)
{
    printf("[ui] create_screens...\r\n");
    create_screens();
    printf("[ui] create_screens done; loadScreen(BOOT)...\r\n");
    loadScreen(SCREEN_ID_BOOT);
    printf("[ui] loadScreen(BOOT) done\r\n");
}

void ui_tick(void)
{
    tick_screen(currentScreen);
}
