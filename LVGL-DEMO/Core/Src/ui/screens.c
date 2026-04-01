#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

//
// Screens
//

void create_screen_scr_home(void) {
    lv_obj_t *obj = lv_obj_create(0);
    objects.scr_home = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0b0e14), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_grow(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;

        /* ── Status bar ── */
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 240, 62);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            add_style_style_surface(obj);
            lv_obj_set_style_height(obj, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_width(obj, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff111620), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_track_place(obj, LV_FLEX_ALIGN_SPACE_AROUND, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_grow(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 179, -43);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_style_dim_label(obj);
                    lv_label_set_text(obj, "PT-0042");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 181, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_style_dim_label(obj);
                    lv_label_set_text(obj, "READY");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 88, 6);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_style_dim_label(obj);
                    lv_label_set_text(obj, "84%");
                }
            }
        }

        /* ── Title row ── */
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 12);
            lv_obj_set_size(obj, 240, 25);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_width(obj, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_height(obj, 28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_grow(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_style_label_secondary(obj);
                    lv_label_set_text(obj, "Spirometer");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -10, 2);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_style_label_secondary(obj);
                    lv_label_set_text(obj, "ADULT");
                }
            }
        }

        /* ── 2×2 metric grid ── */
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 0, 31);
            lv_obj_set_size(obj, 240, 176);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_layout(obj, LV_LAYOUT_GRID, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                static lv_coord_t dsc[] = {54, 54, LV_GRID_TEMPLATE_LAST};
                lv_obj_set_style_grid_row_dsc_array(obj, dsc, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            {
                static lv_coord_t dsc[] = {109, 109, LV_GRID_TEMPLATE_LAST};
                lv_obj_set_style_grid_column_dsc_array(obj, dsc, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_column(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_grow(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00e5a0), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;

                /* FEV1 tile (col 0, row 0) */
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_pos(obj, 1, 66);
                    lv_obj_set_size(obj, 109, 54);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    add_style_style_tile(obj);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_width(obj, 109, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_height(obj, 54, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_style_dim_label(obj);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_height(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "FEV1");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.fev1_val = obj;   /* named reference */
                            lv_obj_set_pos(obj, 0, 10);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            add_style_style_label_primary(obj);
                            lv_obj_set_style_height(obj, 28, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "0.00");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 67, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_style_dim_label(obj);
                            lv_obj_set_style_height(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "L");
                        }
                        {
                            lv_obj_t *obj = lv_bar_create(parent_obj);
                            objects.obj2 = obj;
                            lv_obj_set_pos(obj, 4, 40);
                            lv_obj_set_size(obj, 98, 3);
                            lv_bar_set_value(obj, 25, LV_ANIM_OFF);
                            add_style_style_progress_fev1(obj);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff00e5a0), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_height(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00e5a0), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_width(obj, 99, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_height(obj, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj3 = obj;
                            lv_obj_set_pos(obj, 86, 29);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_style_dim_label(obj);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff00e5a0), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "--%");
                        }
                    }
                }

                /* FVC tile (col 1, row 0) */
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 300, 200);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    add_style_style_tile(obj);
                    lv_obj_set_style_grid_cell_column_pos(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_width(obj, 109, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_height(obj, 54, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_style_dim_label(obj);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_height(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "FVC");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj4 = obj;
                            lv_obj_set_pos(obj, 0, 10);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            add_style_style_label_primary(obj);
                            lv_obj_set_style_height(obj, 28, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff00d4ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "0.00");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 67, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_style_dim_label(obj);
                            lv_obj_set_style_height(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "L");
                        }
                        {
                            lv_obj_t *obj = lv_bar_create(parent_obj);
                            objects.obj5 = obj;
                            lv_obj_set_pos(obj, 4, 40);
                            lv_obj_set_size(obj, 98, 3);
                            lv_bar_set_value(obj, 0, LV_ANIM_OFF);
                            add_style_style_progress_fvc(obj);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_height(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_height(obj, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 86, 29);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_style_dim_label(obj);
                            lv_label_set_text(obj, "--%");
                        }
                    }
                }

                /* FEV1/FVC ratio tile (col 0, row 1) */
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_pos(obj, -36, 62);
                    lv_obj_set_size(obj, 109, 54);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    add_style_style_tile(obj);
                    lv_obj_set_style_grid_cell_row_pos(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_width(obj, 109, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_height(obj, 54, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_style_dim_label(obj);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_height(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "FEV1/FVC");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj6 = obj;
                            lv_obj_set_pos(obj, 0, 10);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            add_style_style_label_primary(obj);
                            lv_obj_set_style_height(obj, 28, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffb020), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "0.0");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 67, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_style_dim_label(obj);
                            lv_obj_set_style_height(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "%");
                        }
                        {
                            lv_obj_t *obj = lv_bar_create(parent_obj);
                            objects.obj7 = obj;
                            lv_obj_set_pos(obj, 4, 40);
                            lv_obj_set_size(obj, 98, 3);
                            lv_bar_set_value(obj, 0, LV_ANIM_OFF);
                            add_style_style_progress_ratio(obj);
                            lv_obj_set_style_height(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_height(obj, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 86, 29);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_style_dim_label(obj);
                            lv_label_set_text(obj, "--%");
                        }
                    }
                }

                /* PEF tile (col 1, row 1) */
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 300, 200);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    add_style_style_tile(obj);
                    lv_obj_set_style_grid_cell_column_pos(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_grid_cell_row_pos(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_width(obj, 109, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_height(obj, 54, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_style_dim_label(obj);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_height(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "PEF");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj8 = obj;
                            lv_obj_set_pos(obj, 0, 10);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            add_style_style_label_primary(obj);
                            lv_obj_set_style_height(obj, 28, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff4a7dff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "0.00");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 67, 20);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_style_dim_label(obj);
                            lv_obj_set_style_height(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "L/s");
                        }
                        {
                            lv_obj_t *obj = lv_bar_create(parent_obj);
                            objects.obj9 = obj;
                            lv_obj_set_pos(obj, 4, 40);
                            lv_obj_set_size(obj, 98, 3);
                            lv_bar_set_value(obj, 0, LV_ANIM_OFF);
                            add_style_style_progress_pef(obj);
                            lv_obj_set_style_height(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_height(obj, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 86, 29);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            add_style_style_dim_label(obj);
                            lv_label_set_text(obj, "--%");
                        }
                    }
                }
            }
        }

        /* ── Flow-Volume chart area ── */
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, -18, 56);
            lv_obj_set_size(obj, 240, 76);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            /* Allow fvl_chart (which sits at y=-12) to render outside this
               container without being clipped. Without this flag LVGL would
               allocate an intermediate compositing layer for the overflow
               region, which exhausts the 32 KB heap and produces a mosaic. */
            lv_obj_add_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
            {
                lv_obj_t *parent_obj = obj;

                /* FVL drawing canvas — NO children so LVGL draws it directly
                   into the framebuffer without any intermediate layer. */
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.fvl_chart = obj;
                    lv_obj_set_pos(obj, 30, -12);
                    lv_obj_set_size(obj, 180, 100);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0d1420), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff1a2540), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
                    add_style_f_v(obj);
                }

                /* "Blow to see graph" — sibling of fvl_chart, NOT a child,
                   so fvl_chart remains child-free (no intermediate layer). */
                {
                    lv_obj_t *lbl = lv_label_create(parent_obj);
                    objects.fvl_wait_lbl = lbl;
                    lv_label_set_text(lbl, "Blow to see graph");
                    lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_pos(lbl, 72, 30);
                    add_style_style_dim_label(lbl);
                }

                /* Y-axis labels (top → bottom: 10, 7, 4, 1  L/s) */
                {
                    static const char *ylabels[4] = {"10", "7", "4", "1"};
                    for (int i = 0; i < 4; i++) {
                        lv_obj_t *lbl = lv_label_create(parent_obj);
                        objects.fvl_ylabel[i] = lbl;
                        lv_label_set_text(lbl, ylabels[i]);
                        lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                        lv_obj_set_pos(lbl, 18, -10 + i * 24);
                        add_style_style_dim_label(lbl);
                    }
                }

                /* X-axis labels (left → right: 0, 2, 4, 6  L) */
                {
                    static const char *xlabels[4] = {"0", "2", "4", "6"};
                    for (int i = 0; i < 4; i++) {
                        lv_obj_t *lbl = lv_label_create(parent_obj);
                        objects.fvl_xlabel[i] = lbl;
                        lv_label_set_text(lbl, xlabels[i]);
                        lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                        lv_obj_set_pos(lbl, 30 + i * 45, 90);
                        add_style_style_dim_label(lbl);
                    }
                }

                /* ── Extended parameter strip (Te / TPEF / FEF25-75 / FEF50) ── */
                /* Te */
                {
                    lv_obj_t *lbl = lv_label_create(parent_obj);
                    objects.te_val = lbl;
                    lv_label_set_text(lbl, "--");
                    lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_pos(lbl, 30, 104);
                    add_style_style_dim_label(lbl);
                }
                /* TPEF */
                {
                    lv_obj_t *lbl = lv_label_create(parent_obj);
                    objects.tpef_val = lbl;
                    lv_label_set_text(lbl, "--");
                    lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_pos(lbl, 80, 104);
                    add_style_style_dim_label(lbl);
                }
                /* FEF25-75 */
                {
                    lv_obj_t *lbl = lv_label_create(parent_obj);
                    objects.fef2575_val = lbl;
                    lv_label_set_text(lbl, "--");
                    lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_pos(lbl, 130, 104);
                    add_style_style_dim_label(lbl);
                }
                /* FEF50 */
                {
                    lv_obj_t *lbl = lv_label_create(parent_obj);
                    objects.fef50_val = lbl;
                    lv_label_set_text(lbl, "--");
                    lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_pos(lbl, 180, 104);
                    add_style_style_dim_label(lbl);
                }
                /* Saturation flag */
                {
                    lv_obj_t *lbl = lv_label_create(parent_obj);
                    objects.sat_label = lbl;
                    lv_label_set_text(lbl, "");
                    lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_pos(lbl, 30, 118);
                    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffff4040), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                /* Validity label */
                {
                    lv_obj_t *lbl = lv_label_create(parent_obj);
                    objects.validity_label = lbl;
                    lv_label_set_text(lbl, "");
                    lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_pos(lbl, 100, 118);
                    add_style_style_dim_label(lbl);
                }
            }
        }

        /* ── Bottom nav bar ── */
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj10 = obj;
            lv_obj_set_pos(obj, -54, 158);
            lv_obj_set_size(obj, 240, 32);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0b1220), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff1e2a40), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_buttonmatrix_create(parent_obj);
                    objects.obj11 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 240, 32);
                    static const char *map[5] = {
                        "Home", "History", "Patient", "Settings", NULL,
                    };
                    static lv_buttonmatrix_ctrl_t ctrl_map[4] = {
                        1 | LV_BUTTONMATRIX_CTRL_CHECKABLE | LV_BUTTONMATRIX_CTRL_CHECKED,
                        1 | LV_BUTTONMATRIX_CTRL_CHECKABLE,
                        1 | LV_BUTTONMATRIX_CTRL_CHECKABLE,
                        1 | LV_BUTTONMATRIX_CTRL_CHECKABLE,
                    };
                    lv_buttonmatrix_set_map(obj, map);
                    lv_buttonmatrix_set_ctrl_map(obj, ctrl_map);
                    lv_buttonmatrix_set_one_checked(obj, true);
                    lv_obj_add_event_cb(obj, action_change_screen, LV_EVENT_VALUE_CHANGED, NULL);
                    lv_obj_set_style_align(obj, LV_ALIGN_BOTTOM_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff4a5f80), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0b1220), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff1e2a40), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff00d4ff), LV_PART_ITEMS | LV_STATE_CHECKED);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_ITEMS | LV_STATE_CHECKED);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_ITEMS | LV_STATE_CHECKED);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_ITEMS | LV_STATE_CHECKED);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_ITEMS | LV_STATE_CHECKED);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_ITEMS | LV_STATE_CHECKED);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_ITEMS | LV_STATE_CHECKED);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff4a5f80), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
                }
            }
        }
    }

    tick_screen_scr_home();
}

void tick_screen_scr_home(void) {
}

/* ── History screen ── */
void create_screen_history(void) {
    lv_obj_t *obj = lv_obj_create(0);
    objects.history = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0b0e14), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;

        /* Volume-Time chart — no children, OVERFLOW_VISIBLE so it can
           render without an intermediate layer. */
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.vt_chart = obj;
            lv_obj_set_pos(obj, 30, 40);
            lv_obj_set_size(obj, 180, 100);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0d1420), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff1a2540), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        }

        /* Wait label as sibling of vt_chart (not child) */
        {
            lv_obj_t *lbl = lv_label_create(parent_obj);
            objects.vt_wait_lbl = lbl;
            lv_label_set_text(lbl, "Blow to see graph");
            lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_pos(lbl, 72, 82);
            add_style_style_dim_label(lbl);
        }

        /* VT Y-axis labels */
        {
            static const char *ylabels[4] = {"6", "4", "2", "0"};
            for (int i = 0; i < 4; i++) {
                lv_obj_t *lbl = lv_label_create(parent_obj);
                objects.vt_ylabel[i] = lbl;
                lv_label_set_text(lbl, ylabels[i]);
                lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_obj_set_pos(lbl, 18, 40 + i * 24);
                add_style_style_dim_label(lbl);
            }
        }

        /* VT X-axis labels */
        {
            static const char *xlabels[4] = {"0", "2", "4", "6"};
            for (int i = 0; i < 4; i++) {
                lv_obj_t *lbl = lv_label_create(parent_obj);
                objects.vt_xlabel[i] = lbl;
                lv_label_set_text(lbl, xlabels[i]);
                lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_obj_set_pos(lbl, 30 + i * 45, 145);
                add_style_style_dim_label(lbl);
            }
        }

        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 80, 180);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_style_label_primary(obj);
            lv_label_set_text(obj, "History");
        }
    }

    tick_screen_history();
}

void tick_screen_history(void) {
}

/* ── Patient screen ── */
void create_screen_patient(void) {
    lv_obj_t *obj = lv_obj_create(0);
    objects.patient = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0b0e14), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 80, 128);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_style_label_primary(obj);
            lv_label_set_text(obj, "Patient");
        }
    }

    tick_screen_patient();
}

void tick_screen_patient(void) {
}

/* ── Settings screen ── */
void create_screen_settings(void) {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 240, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0b0e14), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 80, 128);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_style_label_primary(obj);
            lv_label_set_text(obj, "Settings");
        }
    }

    tick_screen_settings();
}

void tick_screen_settings(void) {
}

/* ── Tick dispatcher ── */
typedef void (*tick_screen_func_t)(void);
static tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_scr_home,
    tick_screen_history,
    tick_screen_patient,
    tick_screen_settings,
};

void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}

void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
// Top-level screen factory
//

void create_screens(void) {
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(
        dispp,
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        false,
        LV_FONT_DEFAULT
    );
    lv_display_set_theme(dispp, theme);

    create_screen_scr_home();
    create_screen_history();
    create_screen_patient();
    create_screen_settings();
}
