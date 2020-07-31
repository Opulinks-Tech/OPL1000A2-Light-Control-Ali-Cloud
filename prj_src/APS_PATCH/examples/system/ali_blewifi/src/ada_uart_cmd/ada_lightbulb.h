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

#ifndef __ADA_LIGHTBULB_H__
#define __ADA_LIGHTBULB_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void light_effect_timer_stop(void);

void ada_cmd_bleadv_on(uint8_t key_value, uint8_t keystate);

void ada_cmd_power_on(uint8_t key_value, uint8_t keystate);
void ada_cmd_power_off(uint8_t key_value, uint8_t keystate);
void ada_cmd_brightness_add(uint8_t key_value, uint8_t keystate);
void ada_cmd_brightness_sub(uint8_t key_value, uint8_t keystate);
void ada_cmd_color_saturation_add(uint8_t key_value, uint8_t keystate);
void ada_cmd_color_saturation_sub(uint8_t key_value, uint8_t keystate);

/* Cold/Warm only */
void ada_cmd_neutral_light(uint8_t key_value, uint8_t keystate);
void ada_cmd_night_light(uint8_t key_value, uint8_t keystate);
void ada_cmd_switch_mode(uint8_t key_value, uint8_t keystate);

/* Cold/Warm only */
void ada_cmd_c10_w90(uint8_t key_value, uint8_t keystate);
void ada_cmd_c30_w70(uint8_t key_value, uint8_t keystate);
void ada_cmd_c70_w30(uint8_t key_value, uint8_t keystate);
void ada_cmd_c90_w10(uint8_t key_value, uint8_t keystate);

/* RBG color control */
void ada_cmd_set_color_red(uint8_t key_value, uint8_t keystate);
void ada_cmd_set_color_green(uint8_t key_value, uint8_t keystate);
void ada_cmd_set_color_blue(uint8_t key_value, uint8_t keystate);
void ada_cmd_set_color_rgb(uint8_t key_value, uint8_t keystate);

void ada_cmd_rgb_8color_fade_slow(uint8_t key_value, uint8_t keystate);
void ada_cmd_rgb_8color_fade_fast(uint8_t key_value, uint8_t keystate);
void ada_cmd_rgb_3color_loop(uint8_t key_value, uint8_t keystate);
void ada_cmd_rgb_7color_loop(uint8_t key_value, uint8_t keystate);

void ada_cmd_speed_add(uint8_t key_value, uint8_t keystate);
void ada_cmd_speed_sub(uint8_t key_value, uint8_t keystate);
void ada_cmd_light_mode_add(uint8_t key_value, uint8_t keystate);
void ada_cmd_light_mode_sub(uint8_t key_value, uint8_t keystate);

void ada_cmd_style_manual(uint8_t key_value, uint8_t keystate);
void ada_cmd_style_reading(uint8_t key_value, uint8_t keystate);
void ada_cmd_style_cinema(uint8_t key_value, uint8_t keystate);
void ada_cmd_style_midnight(uint8_t key_value, uint8_t keystate);
void ada_cmd_style_life(uint8_t key_value, uint8_t keystate);
void ada_cmd_style_softwhite(uint8_t key_value, uint8_t keystate);

#ifdef __cplusplus
}
#endif

#endif /* __ADA_LIGHTBULB_H__ */
