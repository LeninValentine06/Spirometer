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

static int16_t currentScreen  = -1;
static int16_t previousScreen = -1;   /* FIX-C */

static lv_obj_t *getLvglObjectFromIndex(int32_t index)
{
    if (index < 0) return NULL;
    lv_obj_t *screen_ptrs[] = {
        objects.boot,
        objects.scr_home,
        objects.countdown,
        objects.live,
        objects.results,
        objects.fvl_screen,
        objects.vt_screen,
    };
    if (index < 7) return screen_ptrs[index];
    return NULL;
}

/* FIX-C: choose animation direction based on screen order */
void loadScreen(enum ScreensEnum screenId)
{
    int16_t nextScreen = (int16_t)(screenId - 1);
    lv_obj_t *screen   = getLvglObjectFromIndex(nextScreen);

    lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_MOVE_LEFT;   /* default: forward */
    if (previousScreen >= 0 && nextScreen < currentScreen) {
        /* Moving to a lower-indexed screen = backward = slide right */
        anim = LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    }

    if (screen) {
        lv_scr_load_anim(screen, anim, 200, 0, false);
    }

    previousScreen = currentScreen;
    currentScreen  = nextScreen;
}

void ui_init(void)
{
    create_screens();
    loadScreen(SCREEN_ID_BOOT);
}

void ui_tick(void)
{
    tick_screen(currentScreen);
}
