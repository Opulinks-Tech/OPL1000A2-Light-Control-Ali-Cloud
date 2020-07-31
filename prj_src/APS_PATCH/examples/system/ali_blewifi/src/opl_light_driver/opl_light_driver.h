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

#ifndef __OPL_LIGHT_DRIVER_H__
#define __OPL_LIGHT_DRIVER_H__

#include <stdint.h>

#define MAX_Light_Ch 6
#define Red_Light 5
#define Green_Light 4
#define Blue_Light 3
#define Cold_Light 2
#define Warm_Light 1

typedef enum
{
    LIGHT_FLICKER_MIN_HZ = 1,
    LIGHT_FLICKER_300_HZ = LIGHT_FLICKER_MIN_HZ,
    LIGHT_FLICKER_1K_HZ  = 86,
    LIGHT_FLICKER_2K_HZ  = 43,
    LIGHT_FLICKER_4K_HZ  = 22,
    LIGHT_FLICKER_8K_HZ  = 11,
    LIGHT_FLICKER_10K_HZ = 8,    
    LIGHT_FLICKER_15K_HZ = 6,
    LIGHT_FLICKER_20K_HZ = 4,
    LIGHT_FLICKER_40K_HZ = 2,
    LIGHT_FLICKER_80K_HZ = 255,
    LIGHT_FLICKER_MAX_HZ = LIGHT_FLICKER_80K_HZ,
    
} LIGHT_FLICKER_E;

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

#endif
