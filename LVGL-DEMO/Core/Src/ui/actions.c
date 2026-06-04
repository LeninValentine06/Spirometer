/*
 * actions.c  —  SpiroFlow navigation actions
 */

#include "actions.h"
#include "ui.h"
#include "screens.h"
#include "spirometry.h"   /* spiro_reset() on a fresh test */

/* ── Dashboard ──────────────────────────────────────────────────────────── */

void action_start_test(lv_event_t *e)
{
    (void)e;
    /* Trimmed UI: no countdown or live screen.  Clear the previous result
     * and arm the engine; the user blows from the dashboard and the results
     * screen is shown automatically when the maneuver completes. */
    spiro_reset();
}

void action_go_to_dashboard(lv_event_t *e)
{
    (void)e;
    loadScreen(SCREEN_ID_DASHBOARD);
}

/* ── Results carousel ────────────────────────────────────────────────────── */

void action_go_to_results(lv_event_t *e)
{
    (void)e;
    loadScreen(SCREEN_ID_RESULTS);
}

void action_go_to_fvl(lv_event_t *e)
{
    (void)e;
    loadScreen(SCREEN_ID_FVL);
}

void action_go_to_vt(lv_event_t *e)
{
    (void)e;
    loadScreen(SCREEN_ID_VT);
}

/* ── Legacy shims ────────────────────────────────────────────────────────── */

void action_change_screen(lv_event_t *e)
{
    /* Old buttonmatrix nav — redirect all to dashboard */
    (void)e;
    loadScreen(SCREEN_ID_DASHBOARD);
}

void action_go_to_history(lv_event_t *e)
{
    (void)e;
    loadScreen(SCREEN_ID_VT);   /* History → Volume-Time screen */
}
