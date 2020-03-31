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

#include <stdint.h>

#define MAX_Light_Ch 6
#define Red_Light 5
#define Green_Light 4
#define Blue_Light 3
#define Cold_Light 2
#define Warm_Light 1


//fading control related function
void cancel_default_breath(void);
void fade_timer_init(void);
void fade_timer_start(void);
void fade_timer_smart_init(uint32_t fade_total_period,uint32_t fade_step_period);
void fade_step_exe_callback(uint32_t u32TmrIdx);
void opl_light_set_fade_step_period(uint32_t fade_step_period_ms);
void opl_light_set_fade_total_period(uint32_t fade_total_period_ms);
void opl_light_fade_step_cnt_calculation(void);
void opl_light_fade_clear(uint8_t rgbcw_sel);

//low level light control driver
uint8_t opl_light_duty_get(uint8_t rgbcw_sel);
uint8_t opl_light_duty_set(uint8_t rgbcw_sel,uint8_t duty_ticks,uint8_t invert);
uint8_t opl_light_fade_with_time(uint8_t rgbcw_sel,uint8_t target_duty,uint32_t fade_total_period_ms,uint32_t fade_step_period_ms,uint8_t invert);
uint8_t opl_light_fade_with_time_central_fade_ctrl(uint8_t rgbcw_sel,uint8_t target_duty,uint8_t invert);
