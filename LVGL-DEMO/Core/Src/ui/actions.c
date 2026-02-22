/*
 * actions.c
 *
 *  Created on: Feb 23, 2026
 *      Author: lenin
 */


#include "actions.h"
#include "ui.h"
#include "screens.h"

// Called by the nav buttonmatrix on scr_home
// Routes each button index to the correct screen
void action_change_screen(lv_event_t *e)
{
    lv_obj_t *btnmatrix = lv_event_get_target(e);
    uint32_t btn_id = lv_buttonmatrix_get_selected_button(btnmatrix);

    switch (btn_id) {
        case 0: loadScreen(SCREEN_ID_SCR_HOME);  break;
        case 1: loadScreen(SCREEN_ID_HISTORY);   break;
        case 2: loadScreen(SCREEN_ID_PATIENT);   break;
        case 3: loadScreen(SCREEN_ID_SETTINGS);  break;
        default: break;
    }
}

// Convenience shortcut — navigate directly to History screen
void action_go_to_history(lv_event_t *e)
{
    (void)e;
    loadScreen(SCREEN_ID_HISTORY);
}
