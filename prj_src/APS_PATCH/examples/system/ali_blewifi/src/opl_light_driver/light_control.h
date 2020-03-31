/******************************************************************************
*  Copyright 2017 - 2019 Opulinks Technology Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2019
******************************************************************************/

#ifndef __LIGHT_CONTROL_H__
#define __LIGHT_CONTROL_H__

#include <stdint.h>
#include "opl_light_driver.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define W_DEBUG 0

#define UPDATE_LED_STATUS 1
#define DONT_UPDATE_LED_STATUS 0

#define NO_RGB_LIGHTEN_REQ 0

/**
 * @brief define LED power level
 *
 */
enum power_level {
    HF_PWR =0,
    FULL_PWR =1,
};

enum temp_light_type {
    TMP_LT_RGB = 0,
    TMP_LT_C = 1,
    TMP_LT_CW = 2,
    TMP_LT_RGBC = 3,
    TMP_LT_RGBCW = 4,
    TMP_LT_RGBCW_FPWR = 5,
    TMP_LT_CW_FPWR = 7
};

enum light_type_fit_ali {
    LT_C = 0,
    LT_CW = 1,
    LT_RGB = 2,
    LT_RGBC = 3,
    LT_RGBCW = 4
};

enum switch_state {
    SWITCH_OFF = 0,
    SWITCH_ON = 1
};


enum work_mode {
    WMODE_MANUAL = 0,
    WMODE_READING = 1,
    WMODE_CINEMA = 2,
    WMODE_MIDNIGHT = 3,
    WMODE_LIVING = 4,
    WMODE_SOFT = 5,
    WMODE_STREAMER = 6
};

enum light_fade_switch {
    LIGHT_FADE_OFF = 0,
    LIGHT_FADE_ON = 1
};
/**
 *  brief: The mode of the five-color light
 */
enum light_mode {
    //MODE_NONE                = 0,
    //MODE_OFF                 = 1,
    MODE_CTB                 = 0,
    MODE_HSV                 = 1,
    MODE_SCENES              = 2
};

#define LIGHT_COLOR_NUM_MAX         16
#define LIGHT_COLOR_SPEED_TIME_MAX  10000 //10000ms
#define LIGHT_COLOR_SPEED_TIME_MIN  100   //100ms

enum effect_type
{
    EFFTCT_TYPE_FLASH  = 0,
    EFFTCT_TYPE_STROBE = 1,
    EFFTCT_TYPE_MAX
};

typedef struct {
    uint8_t r;  // 0-100 %
    uint8_t g;  // 0-100 %
    uint8_t b;  // 0-100 %
} rgb_t;

typedef struct {
    uint16_t hue;
    uint8_t saturation;
    uint8_t value;
} hsv_t;

/**
 * @brief light effect control
 */
typedef struct
{
    uint16_t effect_type;
    uint16_t color_num;
    uint16_t color_index;
    uint16_t transition_speed;
    uint16_t fade_step_ms;
    rgb_t    rgb_color[LIGHT_COLOR_NUM_MAX];
    hsv_t    color_arr[LIGHT_COLOR_NUM_MAX];
} light_effect_t;


void light_state_debug_print(void);
uint8_t normalize_colortemperature(uint32_t colortemp_k_unit);
void colortemperature_force_overwrite(uint32_t colortemp);

/**
 * brief:  Light initialize
 *
 * param:  config [description]
 *
 * return:
 *      	 None
 */
void light_ctrl_init(void);


/**
 * brief  Set the status of the light
 *
 *
 * return
 */
void light_ctrl_set_scenescolor(uint16_t hue,uint8_t saturation,uint8_t value);
void light_ctrl_set_mode(uint8_t mode);
void light_effect_set_status(uint8_t light_effect_status);
void light_ctrl_set_light_type(uint8_t light_type,uint8_t power_level);
void light_ctrl_set_manual_light_status(uint16_t hue,uint8_t saturation,uint8_t value);
void light_ctrl_set_workmode(uint8_t workmode);
uint8_t light_ctrl_set_hue(uint16_t hue);
uint8_t light_ctrl_set_saturation(uint8_t saturation);
uint8_t light_ctrl_set_value(uint8_t value);
uint8_t light_ctrl_set_color_temperature(uint32_t color_temperature);
uint8_t light_ctrl_set_brightness(uint8_t brightness);
uint8_t light_ctrl_set_hsv(uint16_t hue, uint8_t saturation, uint8_t value, uint8_t fade, uint8_t f_led_status_update, uint8_t rgb_lighten_per);
uint8_t light_ctrl_set_ctb(uint8_t brightness, uint8_t fade, uint8_t status_update);
uint8_t light_ctrl_set_switch(uint8_t status);
void light_ctrl_set_fade_period_ms(uint32_t fade_period_ms);
void light_ctrl_set_fade_step_ms(uint32_t fade_step_ms);

uint8_t light_ctrl_update_hsv(uint16_t hue, uint8_t saturation, uint8_t value);
uint8_t light_ctrl_update_brightness(uint8_t brightness);
uint8_t light_ctrl_update_rgb(uint8_t red, uint8_t green, uint8_t blue);
uint8_t light_ctrl_update_switch(uint8_t on);

/**
 * brief:  Get the status of the light
 */
void light_ctrl_get_scenescolor(uint16_t *hue,uint8_t *saturation,uint8_t *value);
uint8_t light_effect_get_status(void);
uint8_t light_ctrl_get_light_type(void);
void light_ctrl_get_manual_light_status(uint16_t *hue,uint8_t *saturation,uint8_t *value);
uint8_t light_ctrl_get_workmode(void);
uint16_t light_ctrl_get_hue(void);
uint8_t light_ctrl_get_saturation(void);
uint8_t light_ctrl_get_value(void);
uint8_t light_ctrl_get_hsv(uint16_t *hue, uint8_t *saturation, uint8_t *value);
uint32_t light_ctrl_get_color_temperature(void);
uint8_t light_ctrl_get_brightness(void);
uint8_t light_ctrl_get_ctb(uint8_t *brightness);
uint8_t light_ctrl_get_switch(void);
uint8_t light_ctrl_get_mode(void);


/**
 * brief:  Used to indicate the operating mode, such as configuring the network mode, upgrading mode
 *
 * note:   The state of the light is not saved in nvs
 *
 * return
 *
 */
uint8_t light_ctrl_set_rgb(uint8_t red, uint8_t green, uint8_t blue);


void light_effect_init(void);
void light_effect_config(light_effect_t *config);
void light_effect_set_color_array(hsv_t *color_arr, uint16_t color_num);
void light_effect_set_effect_type(uint16_t effect_type);
void light_effect_start(light_effect_t *handle, uint16_t color_index);
void light_effect_stop(light_effect_t *handle);
void light_effect_speed_increase(uint16_t change_time);
void light_effect_speed_decrease(uint16_t change_time);
void light_effect_set_speed(uint16_t speed_per, uint8_t u8Apply);
uint16_t light_effect_get_speed(void);
void light_effect_blink_start(uint8_t red, uint8_t green, uint8_t blue, int16_t period_ms);
void light_effect_blink_stop(void);

uint16_t light_effect_get_color_num(void);
hsv_t *light_effect_get_color_arr(void);

light_effect_t *get_light_effect_config(void);

#ifdef __cplusplus
}
#endif

#endif/**< __LIGHT_CONTROL_H__ */
