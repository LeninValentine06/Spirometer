/*
 * xpt2046.c
 *
 *  Created on: Feb 21, 2026
 *      Author: lenin
 */


#include "xpt2046.h"

// Change these to match your actual SPI and GPIO handles
extern SPI_HandleTypeDef hspi2;

#define TOUCH_CS_PORT   TOUCH_CS_GPIO_Port
#define TOUCH_CS_PIN    TOUCH_CS_Pin
#define TOUCH_IRQ_PORT  TOUCH_IRQ_GPIO_Port
#define TOUCH_IRQ_PIN   TOUCH_IRQ_Pin

// Calibration values — you will update these in Step 8
#define X_MIN   200
#define X_MAX   3800
#define Y_MIN   200
#define Y_MAX   3800

#define SCREEN_W  240
#define SCREEN_H  320

static uint16_t xpt2046_read_raw(uint8_t cmd) {
    uint8_t tx[3] = {cmd, 0x00, 0x00};
    uint8_t rx[3] = {0x00, 0x00, 0x00};

    HAL_GPIO_WritePin(TOUCH_CS_PORT, TOUCH_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi2, tx, rx, 3, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(TOUCH_CS_PORT, TOUCH_CS_PIN, GPIO_PIN_SET);

    return ((rx[1] << 8) | rx[2]) >> 3; // 12-bit result
}

static uint16_t xpt2046_average(uint8_t cmd) {
    uint32_t sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += xpt2046_read_raw(cmd);
    }
    return sum / 5; // average 5 readings to reduce noise
}

void xpt2046_init(void) {
    // Make sure CS is high (deselected) at startup
    HAL_GPIO_WritePin(TOUCH_CS_PORT, TOUCH_CS_PIN, GPIO_PIN_SET);
}

bool xpt2046_get_touch(int16_t *x, int16_t *y) {
    // IRQ is active LOW — if HIGH, no touch
    if (HAL_GPIO_ReadPin(TOUCH_IRQ_PORT, TOUCH_IRQ_PIN) == GPIO_PIN_SET) {
        return false;
    }

    /* XPT2046 channel mapping for ILI9341 in portrait (ROTATE_0 / MADCTL 0x48):
     *   0xD0 = measure raw-X → maps to screen Y (axis swap required)
     *   0x90 = measure raw-Y → maps to screen X
     * Y is also inverted relative to the display origin, so we subtract from
     * max before scaling.  If touch is still wrong after flashing, try removing
     * the Y inversion line, or swap the two channel assignments.            */
    uint16_t raw_screen_y = xpt2046_average(0xD0); /* touch X channel → screen Y */
    uint16_t raw_screen_x = xpt2046_average(0x90); /* touch Y channel → screen X */

    // Clamp to calibration range
    if (raw_screen_x < X_MIN) raw_screen_x = X_MIN;
    if (raw_screen_x > X_MAX) raw_screen_x = X_MAX;
    if (raw_screen_y < Y_MIN) raw_screen_y = Y_MIN;
    if (raw_screen_y > Y_MAX) raw_screen_y = Y_MAX;

    // Map to screen coordinates (Y is inverted)
    *x = (int16_t)((uint32_t)(raw_screen_x - X_MIN) * SCREEN_W / (X_MAX - X_MIN));
    *y = (int16_t)(SCREEN_H - 1 - (uint32_t)(raw_screen_y - Y_MIN) * SCREEN_H / (Y_MAX - Y_MIN));

    return true;
}

void lv_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    int16_t x = 0, y = 0;

    if (xpt2046_get_touch(&x, &y)) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
