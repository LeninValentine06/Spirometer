#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

objects_t objects;
lv_obj_t *tick_value_change_obj;

/* =========================================================================
 * make_chart_obj — creates a plain container suitable for LV_EVENT_DRAW_POST
 * graph callbacks.
 *
 * CRITICAL: the default LVGL theme sets shadow_width > 0 on every lv_obj.
 * A shadow forces LVGL 9 to allocate an intermediate compositing layer whose
 * size = w * h * 2 bytes.  For a 214×148 VT chart that is ~63 KB — far more
 * than the 32 KB heap.  Even the 214×92 FVL chart needs ~39 KB.  The
 * allocation fails silently and LVGL renders garbage pixels (mosaic pattern).
 *
 * Fix: zero every style property that can trigger intermediate layer
 * allocation (shadow, scrollbar opacity).  This makes LVGL draw the object
 * directly into the display strip buffer without any heap allocation.
 * ========================================================================= */
static lv_obj_t *make_chart_obj(lv_obj_t *parent,
                                  int32_t x, int32_t y,
                                  int32_t w, int32_t h)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj,     lv_color_hex(0xff0d1420), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj,       LV_OPA_COVER,             LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(obj, lv_color_hex(0xff1e2a40), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 1,                        LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj,       0,                        LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(obj,      0,                        LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(obj, 0,             LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_set_style_opa(obj,          LV_OPA_COVER,  LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_set_style_opa(obj,          LV_OPA_TRANSP, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    return obj;
}

/* =========================================================================
 * scr_home  (240x320)
 * ========================================================================= */
void create_screen_scr_home(void)
{
    lv_obj_t *obj = lv_obj_create(0);
    objects.scr_home = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0b0e14), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(obj,      0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj,       0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_layout(obj,          LV_LAYOUT_FLEX,      LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_flow(obj,       LV_FLEX_FLOW_COLUMN,  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START,  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(obj,    0,   LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(obj, 0,   LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;

        /* ---- Status bar 24 px ---- */
        {
            lv_obj_t *o = lv_obj_create(parent_obj);
            objects.obj0 = o;
            lv_obj_set_size(o, 240, 24);
            lv_obj_set_style_pad_left(o,8,0); lv_obj_set_style_pad_right(o,8,0);
            lv_obj_set_style_pad_top(o,0,0);  lv_obj_set_style_pad_bottom(o,0,0);
            lv_obj_set_style_border_width(o,0,0); lv_obj_set_style_radius(o,0,0);
            lv_obj_set_style_shadow_width(o,0,0);
            lv_obj_set_style_bg_color(o, lv_color_hex(0xff111620), 0);
            lv_obj_set_style_layout(o, LV_LAYOUT_FLEX, 0);
            lv_obj_set_style_flex_flow(o, LV_FLEX_FLOW_ROW, 0);
            lv_obj_set_style_flex_main_place(o, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);
            lv_obj_set_style_flex_cross_place(o, LV_FLEX_ALIGN_CENTER, 0);
            lv_obj_set_style_flex_grow(o, 0, 0);
            lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
            { lv_obj_t *l=lv_label_create(o); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"PT-0042"); }
            { lv_obj_t *l=lv_label_create(o); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"84%"); }
            { lv_obj_t *l=lv_label_create(o); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"READY"); }
        }

        /* ---- Title row 28 px ---- */
        {
            lv_obj_t *o = lv_obj_create(parent_obj);
            lv_obj_set_size(o, 240, 28);
            lv_obj_set_style_pad_left(o,8,0); lv_obj_set_style_pad_right(o,8,0);
            lv_obj_set_style_pad_top(o,0,0);  lv_obj_set_style_pad_bottom(o,0,0);
            lv_obj_set_style_bg_opa(o,0,0); lv_obj_set_style_border_width(o,0,0); lv_obj_set_style_radius(o,0,0);
            lv_obj_set_style_shadow_width(o,0,0);
            lv_obj_set_style_layout(o, LV_LAYOUT_FLEX, 0);
            lv_obj_set_style_flex_flow(o, LV_FLEX_FLOW_ROW, 0);
            lv_obj_set_style_flex_main_place(o, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);
            lv_obj_set_style_flex_cross_place(o, LV_FLEX_ALIGN_CENTER, 0);
            lv_obj_set_style_flex_grow(o, 0, 0);
            lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
            { lv_obj_t *l=lv_label_create(o); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_label_secondary(l); lv_label_set_text(l,"Spirometer"); }
            { lv_obj_t *l=lv_label_create(o); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_label_secondary(l); lv_label_set_text(l,"ADULT"); }
        }

        /* ---- 2x2 metric grid 116 px ---- */
        {
            lv_obj_t *o = lv_obj_create(parent_obj);
            objects.obj1 = o;
            lv_obj_set_size(o, 240, 116);
            lv_obj_set_style_bg_opa(o,0,0); lv_obj_set_style_border_width(o,0,0); lv_obj_set_style_radius(o,0,0);
            lv_obj_set_style_shadow_width(o,0,0);
            lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_layout(o, LV_LAYOUT_GRID, 0);
            { static lv_coord_t r[]={LV_GRID_FR(1),LV_GRID_FR(1),LV_GRID_TEMPLATE_LAST};
              lv_obj_set_style_grid_row_dsc_array(o,r,0); }
            { static lv_coord_t c[]={LV_GRID_FR(1),LV_GRID_FR(1),LV_GRID_TEMPLATE_LAST};
              lv_obj_set_style_grid_column_dsc_array(o,c,0); }
            lv_obj_set_style_pad_top(o,4,0); lv_obj_set_style_pad_bottom(o,4,0);
            lv_obj_set_style_pad_left(o,6,0); lv_obj_set_style_pad_right(o,6,0);
            lv_obj_set_style_pad_row(o,6,0); lv_obj_set_style_pad_column(o,6,0);
            lv_obj_set_style_flex_grow(o,0,0);
            {
                lv_obj_t *p = o;

#define TILE(col,row) \
    lv_obj_t *t=lv_obj_create(p); \
    lv_obj_set_style_pad_all(t,6,0); lv_obj_set_style_radius(t,0,0); \
    lv_obj_remove_flag(t,LV_OBJ_FLAG_SCROLLABLE); \
    lv_obj_set_style_shadow_width(t,0,0); \
    add_style_style_tile(t); lv_obj_set_style_border_width(t,2,0); \
    lv_obj_set_style_grid_cell_column_pos(t,col,0); lv_obj_set_style_grid_cell_row_pos(t,row,0); \
    lv_obj_set_style_grid_cell_column_span(t,1,0); lv_obj_set_style_grid_cell_row_span(t,1,0); \
    lv_obj_set_style_grid_cell_x_align(t,LV_GRID_ALIGN_STRETCH,0); \
    lv_obj_set_style_grid_cell_y_align(t,LV_GRID_ALIGN_STRETCH,0)

                /* FEV1 */
                { TILE(0,0);
                  { lv_obj_t *l=lv_label_create(t); lv_obj_set_pos(l,0,0); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"FEV1"); }
                  { lv_obj_t *l=lv_label_create(t); objects.fev1_val=l; lv_obj_set_pos(l,0,12); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); lv_obj_remove_flag(l,LV_OBJ_FLAG_SCROLLABLE); add_style_style_label_primary(l); lv_label_set_text(l,"0.00"); }
                  { lv_obj_t *l=lv_label_create(t); lv_obj_set_pos(l,60,18); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"L"); }
                  { lv_obj_t *b=lv_bar_create(t); objects.obj2=b; lv_obj_set_pos(b,0,38); lv_obj_set_size(b,LV_PCT(100),4); lv_bar_set_value(b,0,LV_ANIM_OFF); add_style_style_progress_fev1(b); lv_obj_set_style_radius(b,0,LV_PART_MAIN|LV_STATE_DEFAULT); lv_obj_set_style_height(b,4,LV_PART_INDICATOR|LV_STATE_DEFAULT); lv_obj_set_style_radius(b,0,LV_PART_INDICATOR|LV_STATE_DEFAULT); }
                  { lv_obj_t *l=lv_label_create(t); objects.obj3=l; lv_obj_set_pos(l,65,26); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"0%"); }
                }
                /* FVC */
                { TILE(1,0);
                  { lv_obj_t *l=lv_label_create(t); lv_obj_set_pos(l,0,0); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"FVC"); }
                  { lv_obj_t *l=lv_label_create(t); objects.obj4=l; lv_obj_set_pos(l,0,12); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); lv_obj_remove_flag(l,LV_OBJ_FLAG_SCROLLABLE); add_style_style_label_primary(l); lv_obj_set_style_text_color(l,lv_color_hex(0xff00d4ff),LV_PART_MAIN|LV_STATE_DEFAULT); lv_label_set_text(l,"0.00"); }
                  { lv_obj_t *l=lv_label_create(t); lv_obj_set_pos(l,60,18); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"L"); }
                  { lv_obj_t *b=lv_bar_create(t); objects.obj5=b; lv_obj_set_pos(b,0,38); lv_obj_set_size(b,LV_PCT(100),4); lv_bar_set_value(b,0,LV_ANIM_OFF); add_style_style_progress_fvc(b); lv_obj_set_style_radius(b,0,LV_PART_MAIN|LV_STATE_DEFAULT); lv_obj_set_style_height(b,4,LV_PART_INDICATOR|LV_STATE_DEFAULT); lv_obj_set_style_radius(b,0,LV_PART_INDICATOR|LV_STATE_DEFAULT); }
                }
                /* FEV1/FVC */
                { TILE(0,1);
                  { lv_obj_t *l=lv_label_create(t); lv_obj_set_pos(l,0,0); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"FEV1/FVC"); }
                  { lv_obj_t *l=lv_label_create(t); objects.obj6=l; lv_obj_set_pos(l,0,12); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); lv_obj_remove_flag(l,LV_OBJ_FLAG_SCROLLABLE); add_style_style_label_primary(l); lv_obj_set_style_text_color(l,lv_color_hex(0xffffb020),LV_PART_MAIN|LV_STATE_DEFAULT); lv_label_set_text(l,"0.0"); }
                  { lv_obj_t *l=lv_label_create(t); lv_obj_set_pos(l,60,18); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"%"); }
                  { lv_obj_t *b=lv_bar_create(t); objects.obj7=b; lv_obj_set_pos(b,0,38); lv_obj_set_size(b,LV_PCT(100),4); lv_bar_set_value(b,0,LV_ANIM_OFF); add_style_style_progress_ratio(b); lv_obj_set_style_radius(b,0,LV_PART_MAIN|LV_STATE_DEFAULT); lv_obj_set_style_height(b,4,LV_PART_INDICATOR|LV_STATE_DEFAULT); lv_obj_set_style_radius(b,0,LV_PART_INDICATOR|LV_STATE_DEFAULT); }
                }
                /* PEF */
                { TILE(1,1);
                  { lv_obj_t *l=lv_label_create(t); lv_obj_set_pos(l,0,0); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"PEF"); }
                  { lv_obj_t *l=lv_label_create(t); objects.obj8=l; lv_obj_set_pos(l,0,12); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); lv_obj_remove_flag(l,LV_OBJ_FLAG_SCROLLABLE); add_style_style_label_primary(l); lv_obj_set_style_text_color(l,lv_color_hex(0xff4a7dff),LV_PART_MAIN|LV_STATE_DEFAULT); lv_label_set_text(l,"0.00"); }
                  { lv_obj_t *l=lv_label_create(t); lv_obj_set_pos(l,55,18); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"L/s"); }
                  { lv_obj_t *b=lv_bar_create(t); objects.obj9=b; lv_obj_set_pos(b,0,38); lv_obj_set_size(b,LV_PCT(100),4); lv_bar_set_value(b,0,LV_ANIM_OFF); add_style_style_progress_pef(b); lv_obj_set_style_radius(b,0,LV_PART_MAIN|LV_STATE_DEFAULT); lv_obj_set_style_height(b,4,LV_PART_INDICATOR|LV_STATE_DEFAULT); lv_obj_set_style_radius(b,0,LV_PART_INDICATOR|LV_STATE_DEFAULT); }
                }
#undef TILE
            }
        }

        /* ---- FVL chart area 120 px ---- */
        {
            lv_obj_t *o = lv_obj_create(parent_obj);
            lv_obj_set_size(o, 240, 120);
            lv_obj_set_style_pad_all(o,0,0);
            lv_obj_set_style_bg_color(o,lv_color_hex(0xff0b0e14),0);
            lv_obj_set_style_bg_opa(o,255,0);
            lv_obj_set_style_border_width(o,0,0); lv_obj_set_style_radius(o,0,0);
            lv_obj_set_style_shadow_width(o,0,0);
            lv_obj_set_style_flex_grow(o,0,0);
            lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *p = o;

                objects.fvl_chart = make_chart_obj(p, 26, 0, 210, 100);

                { lv_obj_t *l=lv_label_create(p); objects.fvl_wait_lbl=l;
                  lv_obj_set_pos(l,26,43); lv_obj_set_size(l,210,14);
                  lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);
                  lv_obj_set_style_text_color(l,lv_color_hex(0xff1e2a40),0);
                  lv_obj_set_style_text_font(l,&lv_font_montserrat_14,0);
                  lv_obj_set_style_bg_opa(l,0,0); lv_label_set_text(l,"Blow to see graph"); }

                { lv_obj_t *l=lv_label_create(p); lv_obj_set_pos(l,0,0); lv_obj_set_size(l,24,10);
                  lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_RIGHT,0);
                  lv_obj_set_style_text_color(l,lv_color_hex(0xff00d4ff),0);
                  lv_obj_set_style_text_font(l,&lv_font_montserrat_10,0);
                  lv_obj_set_style_bg_opa(l,0,0); lv_label_set_text(l,"L/s"); }

                { static const int16_t yp[4]={0,25,50,75};
                  static const char *yi[4]={"--","--","--"," 0"};
                  for(int i=0;i<4;i++){
                      lv_obj_t *l=lv_label_create(p); objects.fvl_ylabel[i]=l;
                      lv_obj_set_pos(l,0,yp[i]); lv_obj_set_size(l,24,10);
                      lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_RIGHT,0);
                      lv_obj_set_style_text_color(l,lv_color_hex(0xff3d5070),0);
                      lv_obj_set_style_text_font(l,&lv_font_montserrat_10,0);
                      lv_obj_set_style_bg_opa(l,0,0); lv_label_set_text(l,yi[i]);
                  }
                }

                { static const int16_t xp[4]={13,83,153,223};
                  static const char *xi[4]={"0","-","-","-"};
                  for(int i=0;i<4;i++){
                      lv_obj_t *l=lv_label_create(p); objects.fvl_xlabel[i]=l;
                      lv_obj_set_pos(l,xp[i],101); lv_obj_set_size(l,26,10);
                      lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);
                      lv_obj_set_style_text_color(l,lv_color_hex(0xff3d5070),0);
                      lv_obj_set_style_text_font(l,&lv_font_montserrat_10,0);
                      lv_obj_set_style_bg_opa(l,0,0); lv_label_set_text(l,xi[i]);
                  }
                }

                { lv_obj_t *l=lv_label_create(p); objects.te_val=l;       lv_obj_set_pos(l,26, 111); lv_obj_set_size(l,42,10); add_style_style_label_primary(l);   lv_obj_set_style_text_font(l,&lv_font_montserrat_10,0); lv_label_set_text(l,"--"); }
                { lv_obj_t *l=lv_label_create(p); objects.tpef_val=l;     lv_obj_set_pos(l,70, 111); lv_obj_set_size(l,42,10); add_style_style_label_primary(l);   lv_obj_set_style_text_font(l,&lv_font_montserrat_10,0); lv_label_set_text(l,"--"); }
                { lv_obj_t *l=lv_label_create(p); objects.fef2575_val=l;  lv_obj_set_pos(l,114,111); lv_obj_set_size(l,48,10); add_style_style_label_secondary(l); lv_obj_set_style_text_font(l,&lv_font_montserrat_10,0); lv_label_set_text(l,"--"); }
                { lv_obj_t *l=lv_label_create(p); objects.fef50_val=l;    lv_obj_set_pos(l,164,111); lv_obj_set_size(l,36,10); add_style_style_label_secondary(l); lv_obj_set_style_text_font(l,&lv_font_montserrat_10,0); lv_label_set_text(l,"--"); }
                { lv_obj_t *l=lv_label_create(p); objects.sat_label=l;    lv_obj_set_pos(l,202,111); lv_obj_set_size(l,24,10); lv_obj_set_style_text_font(l,&lv_font_montserrat_10,0); lv_obj_set_style_text_color(l,lv_color_hex(0xffff4040),LV_PART_MAIN|LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(l,0,0); lv_label_set_text(l,""); }
                { lv_obj_t *l=lv_label_create(p); objects.validity_label=l; lv_obj_set_pos(l,202,111); lv_obj_set_size(l,36,10); lv_obj_set_style_text_font(l,&lv_font_montserrat_10,0); lv_obj_set_style_text_color(l,lv_color_hex(0xff00e5a0),LV_PART_MAIN|LV_STATE_DEFAULT); lv_obj_set_style_bg_opa(l,0,0); lv_label_set_text(l,""); }
            }
        }

        /* ---- Bottom nav bar 32 px ---- */
        {
            lv_obj_t *o = lv_obj_create(parent_obj);
            objects.obj10 = o;
            lv_obj_set_size(o,240,32);
            lv_obj_set_style_pad_all(o,0,0); lv_obj_set_style_radius(o,0,0);
            lv_obj_set_style_bg_color(o,lv_color_hex(0xff0b1220),0);
            lv_obj_set_style_bg_opa(o,255,0);
            lv_obj_set_style_border_width(o,1,0);
            lv_obj_set_style_border_color(o,lv_color_hex(0xff1e2a40),0);
            lv_obj_set_style_shadow_width(o,0,0);
            lv_obj_set_style_flex_grow(o,0,0);
            lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *btnm=lv_buttonmatrix_create(o);
                objects.obj11=btnm;
                lv_obj_set_pos(btnm,0,0); lv_obj_set_size(btnm,240,32);
                static const char *map[5]={"Home","History","Patient","Settings",NULL};
                static lv_buttonmatrix_ctrl_t ctrl_map[4]={
                    1|LV_BUTTONMATRIX_CTRL_CHECKABLE|LV_BUTTONMATRIX_CTRL_CHECKED,
                    1|LV_BUTTONMATRIX_CTRL_CHECKABLE,
                    1|LV_BUTTONMATRIX_CTRL_CHECKABLE,
                    1|LV_BUTTONMATRIX_CTRL_CHECKABLE,
                };
                lv_buttonmatrix_set_map(btnm,map);
                lv_buttonmatrix_set_ctrl_map(btnm,ctrl_map);
                lv_buttonmatrix_set_one_checked(btnm,true);
                lv_obj_add_event_cb(btnm,action_change_screen,LV_EVENT_VALUE_CHANGED,NULL);
                lv_obj_set_style_text_color(btnm,lv_color_hex(0xff4a5f80),LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(btnm,lv_color_hex(0xff0b1220),LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(btnm,&lv_font_montserrat_14,LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(btnm,lv_color_hex(0xff1e2a40),LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_radius(btnm,0,LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(btnm,0,LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(btnm,lv_color_hex(0xff00d4ff),LV_PART_ITEMS|LV_STATE_CHECKED);
                lv_obj_set_style_text_align(btnm,LV_TEXT_ALIGN_CENTER,LV_PART_ITEMS|LV_STATE_CHECKED);
                lv_obj_set_style_text_font(btnm,&lv_font_montserrat_14,LV_PART_ITEMS|LV_STATE_CHECKED);
                lv_obj_set_style_text_align(btnm,LV_TEXT_ALIGN_CENTER,LV_PART_ITEMS|LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(btnm,lv_color_hex(0xff4a5f80),LV_PART_ITEMS|LV_STATE_DEFAULT);
            }
        }
    }
    tick_screen_scr_home();
}
void tick_screen_scr_home(void) {}

/* =========================================================================
 * History screen — Volume-Time graph (240x320)
 *   Status bar  24 px
 *   VT area    264 px
 *   Nav bar     32 px
 *   Total      320 px
 * ========================================================================= */
void create_screen_history(void)
{
    lv_obj_t *obj=lv_obj_create(0); objects.history=obj;
    lv_obj_set_pos(obj,0,0); lv_obj_set_size(obj,240,320);
    lv_obj_set_style_bg_color(obj,lv_color_hex(0xff0b0e14),0);
    lv_obj_set_style_pad_all(obj,0,0); lv_obj_set_style_border_width(obj,0,0); lv_obj_set_style_radius(obj,0,0);
    lv_obj_set_style_shadow_width(obj,0,0);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_layout(obj,LV_LAYOUT_FLEX,0);
    lv_obj_set_style_flex_flow(obj,LV_FLEX_FLOW_COLUMN,0);
    lv_obj_set_style_flex_main_place(obj,LV_FLEX_ALIGN_START,0);
    lv_obj_set_style_pad_row(obj,0,0); lv_obj_set_style_pad_column(obj,0,0);
    {
        lv_obj_t *parent_obj=obj;

        /* Status bar 24 px */
        { lv_obj_t *o=lv_obj_create(parent_obj); lv_obj_set_size(o,240,24);
          lv_obj_set_style_pad_left(o,8,0); lv_obj_set_style_pad_right(o,8,0);
          lv_obj_set_style_pad_top(o,0,0); lv_obj_set_style_pad_bottom(o,0,0);
          lv_obj_set_style_border_width(o,0,0); lv_obj_set_style_radius(o,0,0);
          lv_obj_set_style_shadow_width(o,0,0);
          lv_obj_set_style_bg_color(o,lv_color_hex(0xff111620),0);
          lv_obj_set_style_layout(o,LV_LAYOUT_FLEX,0); lv_obj_set_style_flex_flow(o,LV_FLEX_FLOW_ROW,0);
          lv_obj_set_style_flex_main_place(o,LV_FLEX_ALIGN_SPACE_BETWEEN,0);
          lv_obj_set_style_flex_cross_place(o,LV_FLEX_ALIGN_CENTER,0);
          lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);
          { lv_obj_t *l=lv_label_create(o); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"History"); }
          { lv_obj_t *l=lv_label_create(o); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_dim_label(l); lv_label_set_text(l,"V-T Graph"); }
        }

        /* VT chart area 264 px */
        { lv_obj_t *o=lv_obj_create(parent_obj); lv_obj_set_size(o,240,264);
          lv_obj_set_style_pad_all(o,0,0);
          lv_obj_set_style_bg_color(o,lv_color_hex(0xff0b0e14),0); lv_obj_set_style_bg_opa(o,255,0);
          lv_obj_set_style_border_width(o,0,0); lv_obj_set_style_radius(o,0,0);
          lv_obj_set_style_shadow_width(o,0,0);
          lv_obj_set_style_flex_grow(o,0,0); lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);
          { lv_obj_t *p=o;
            /* Y unit */
            { lv_obj_t *l=lv_label_create(p); lv_obj_set_pos(l,0,16); lv_obj_set_size(l,22,11);
              lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_RIGHT,0);
              lv_obj_set_style_text_color(l,lv_color_hex(0xff00d4ff),0);
              lv_obj_set_style_text_font(l,&lv_font_montserrat_14,0);
              lv_obj_set_style_bg_opa(l,0,0); lv_label_set_text(l,"L"); }
            /* Y labels */
            { static const int16_t yp[4]={16,56,96,136};
              static const char *yi[4]={"--","--","--"," 0"};
              for(int i=0;i<4;i++){
                  lv_obj_t *l=lv_label_create(p); objects.vt_ylabel[i]=l;
                  lv_obj_set_pos(l,0,yp[i]); lv_obj_set_size(l,22,11);
                  lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_RIGHT,0);
                  lv_obj_set_style_text_color(l,lv_color_hex(0xff3d5070),0);
                  lv_obj_set_style_text_font(l,&lv_font_montserrat_14,0);
                  lv_obj_set_style_bg_opa(l,0,0); lv_label_set_text(l,yi[i]);
              }
            }
            /* Chart canvas */
            objects.vt_chart=make_chart_obj(p,22,16,214,148);
            /* Wait label */
            { lv_obj_t *l=lv_label_create(p); objects.vt_wait_lbl=l;
              lv_obj_set_pos(l,22,82); lv_obj_set_size(l,214,14);
              lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);
              lv_obj_set_style_text_color(l,lv_color_hex(0xff1e2a40),0);
              lv_obj_set_style_text_font(l,&lv_font_montserrat_14,0);
              lv_obj_set_style_bg_opa(l,0,0); lv_label_set_text(l,"Blow to see graph"); }
            /* X labels */
            { static const int16_t xp[4]={15,86,157,228};
              static const char *xi[4]={"0","-","-","-"};
              for(int i=0;i<4;i++){
                  lv_obj_t *l=lv_label_create(p); objects.vt_xlabel[i]=l;
                  lv_obj_set_pos(l,xp[i],167); lv_obj_set_size(l,18,11);
                  lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);
                  lv_obj_set_style_text_color(l,lv_color_hex(0xff3d5070),0);
                  lv_obj_set_style_text_font(l,&lv_font_montserrat_14,0);
                  lv_obj_set_style_bg_opa(l,0,0); lv_label_set_text(l,xi[i]);
              }
            }
            /* X unit */
            { lv_obj_t *l=lv_label_create(p); lv_obj_set_pos(l,0,180); lv_obj_set_size(l,240,12);
              lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);
              lv_obj_set_style_text_color(l,lv_color_hex(0xff00d4ff),0);
              lv_obj_set_style_text_font(l,&lv_font_montserrat_14,0);
              lv_obj_set_style_bg_opa(l,0,0); lv_label_set_text(l,"Time (s)"); }
          }
        }

        /* Nav bar 32 px */
        { lv_obj_t *o=lv_obj_create(parent_obj); lv_obj_set_size(o,240,32);
          lv_obj_set_style_pad_all(o,0,0); lv_obj_set_style_radius(o,0,0);
          lv_obj_set_style_bg_color(o,lv_color_hex(0xff0b1220),0); lv_obj_set_style_bg_opa(o,255,0);
          lv_obj_set_style_border_width(o,1,0); lv_obj_set_style_border_color(o,lv_color_hex(0xff1e2a40),0);
          lv_obj_set_style_shadow_width(o,0,0);
          lv_obj_set_style_flex_grow(o,0,0); lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);
          { lv_obj_t *btnm=lv_buttonmatrix_create(o);
            lv_obj_set_pos(btnm,0,0); lv_obj_set_size(btnm,240,32);
            static const char *map[5]={"Home","History","Patient","Settings",NULL};
            static lv_buttonmatrix_ctrl_t ctrl_map[4]={
                1|LV_BUTTONMATRIX_CTRL_CHECKABLE,
                1|LV_BUTTONMATRIX_CTRL_CHECKABLE|LV_BUTTONMATRIX_CTRL_CHECKED,
                1|LV_BUTTONMATRIX_CTRL_CHECKABLE,
                1|LV_BUTTONMATRIX_CTRL_CHECKABLE
            };
            lv_buttonmatrix_set_map(btnm,map); lv_buttonmatrix_set_ctrl_map(btnm,ctrl_map);
            lv_buttonmatrix_set_one_checked(btnm,true);
            lv_obj_add_event_cb(btnm,action_change_screen,LV_EVENT_VALUE_CHANGED,NULL);
            lv_obj_set_style_text_color(btnm,lv_color_hex(0xff4a5f80),LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btnm,lv_color_hex(0xff0b1220),LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(btnm,&lv_font_montserrat_14,LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(btnm,lv_color_hex(0xff1e2a40),LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_radius(btnm,0,LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(btnm,0,LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(btnm,lv_color_hex(0xff00d4ff),LV_PART_ITEMS|LV_STATE_CHECKED);
            lv_obj_set_style_text_align(btnm,LV_TEXT_ALIGN_CENTER,LV_PART_ITEMS|LV_STATE_CHECKED);
            lv_obj_set_style_text_font(btnm,&lv_font_montserrat_14,LV_PART_ITEMS|LV_STATE_CHECKED);
            lv_obj_set_style_text_align(btnm,LV_TEXT_ALIGN_CENTER,LV_PART_ITEMS|LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(btnm,lv_color_hex(0xff4a5f80),LV_PART_ITEMS|LV_STATE_DEFAULT);
          }
        }
    }
    tick_screen_history();
}
void tick_screen_history(void) {}

/* =========================================================================
 * Patient screen (240x320)
 * ========================================================================= */
void create_screen_patient(void)
{
    lv_obj_t *obj=lv_obj_create(0); objects.patient=obj;
    lv_obj_set_pos(obj,0,0); lv_obj_set_size(obj,240,320);
    lv_obj_set_style_bg_color(obj,lv_color_hex(0xff0b0e14),0);
    lv_obj_set_style_pad_all(obj,0,0);
    lv_obj_set_style_border_width(obj,0,0);
    lv_obj_set_style_radius(obj,0,0);
    lv_obj_set_style_shadow_width(obj,0,0);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_SCROLLABLE);
    { lv_obj_t *l=lv_label_create(obj); lv_obj_set_pos(l,80,128); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_label_primary(l); lv_label_set_text(l,"Patient"); }
    tick_screen_patient();
}
void tick_screen_patient(void) {}

/* =========================================================================
 * Settings screen (240x320)
 * ========================================================================= */
void create_screen_settings(void)
{
    lv_obj_t *obj=lv_obj_create(0); objects.settings=obj;
    lv_obj_set_pos(obj,0,0); lv_obj_set_size(obj,240,320);
    lv_obj_set_style_bg_color(obj,lv_color_hex(0xff0b0e14),0);
    lv_obj_set_style_pad_all(obj,0,0);
    lv_obj_set_style_border_width(obj,0,0);
    lv_obj_set_style_radius(obj,0,0);
    lv_obj_set_style_shadow_width(obj,0,0);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_SCROLLABLE);
    { lv_obj_t *l=lv_label_create(obj); lv_obj_set_pos(l,80,128); lv_obj_set_size(l,LV_SIZE_CONTENT,LV_SIZE_CONTENT); add_style_style_label_primary(l); lv_label_set_text(l,"Settings"); }
    tick_screen_settings();
}
void tick_screen_settings(void) {}

typedef void (*tick_screen_func_t)(void);
static tick_screen_func_t tick_screen_funcs[]={
    tick_screen_scr_home,tick_screen_history,tick_screen_patient,tick_screen_settings
};
void tick_screen(int i)               { tick_screen_funcs[i](); }
void tick_screen_by_id(enum ScreensEnum id){ tick_screen_funcs[id-1](); }

ext_font_desc_t fonts[]={
#if LV_FONT_MONTSERRAT_14
    {"MONTSERRAT_14",&lv_font_montserrat_14},
#endif
};
uint32_t active_theme_index=0;

void create_screens(void)
{
    lv_display_t *dispp=lv_display_get_default();
    lv_theme_t *theme=lv_theme_default_init(dispp,
        lv_palette_main(LV_PALETTE_BLUE),lv_palette_main(LV_PALETTE_RED),
        false,LV_FONT_DEFAULT);
    lv_display_set_theme(dispp,theme);
    create_screen_scr_home();
    create_screen_history();
    create_screen_patient();
    create_screen_settings();
}
