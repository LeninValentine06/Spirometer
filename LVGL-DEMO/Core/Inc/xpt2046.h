/*
 * xpt2046.h
 *
 *  Created on: Feb 21, 2026
 *      Author: lenin
 */

#ifndef XPT2046_H
#define XPT2046_H

#include "main.h"
#include "stdbool.h"
#include "lvgl.h"

void xpt2046_init(void);
bool xpt2046_get_touch(int16_t *x, int16_t *y);
void lv_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);

#endif
