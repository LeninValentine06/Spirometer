#include "ui.h"
#include "screens.h"
#include "images.h"
#include "actions.h"
#include "vars.h"

#include <string.h>

static int16_t currentScreen = -1;

static lv_obj_t *getLvglObjectFromIndex(int32_t index)
{
    if (index < 0) return NULL;
    /* Map screen index (0-based) to the correct screen object */
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

void loadScreen(enum ScreensEnum screenId)
{
    currentScreen = (int16_t)(screenId - 1);
    lv_obj_t *screen = getLvglObjectFromIndex(currentScreen);
    if (screen)
        lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
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
