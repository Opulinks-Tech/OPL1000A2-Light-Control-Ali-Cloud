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

#include "light_control.h"
#include "opl_light_driver.h"
#include "blewifi_configuration.h"

#ifdef BLEWIFI_LIGHT_DRV
#include "cmsis_os.h"
#include "blewifi_ctrl.h"
#else
#include "hal_tmr.h"
#endif

#include "hal_pwm.h"
#include "infra_config.h"

#define USER_LIGHT_FILCKER_HZ       LIGHT_FLICKER_4K_HZ

typedef struct
{
    uint32_t fade_step_period_ms;
    uint32_t fade_total_period_ms;
    uint32_t fade_step_count;
} T_fadetiming_ctrl_center;

typedef struct
{
    uint8_t fade_en;
    uint8_t fade_step_up;
    uint32_t fade_nop_count;
    uint8_t fade_delta_per_step;
    uint8_t fade_work_step_count;
    uint8_t last_round_compensation;
} T_light_fade_conf;

SHM_DATA S_Hal_Pwm_Config_t g_pwm_cconfig[MAX_Light_Ch] = {0};
T_light_fade_conf g_light_fading_config[MAX_Light_Ch] = {0};
T_fadetiming_ctrl_center g_fadetiming_ctrl = {0};

#ifdef BLEWIFI_LIGHT_DRV
osTimerId g_tLightDrvTmr = NULL;

void light_drv_tmr_cb(void const *argument)
{
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_LIGHT_DRV_TIMEOUT, NULL, 0);
}

void light_drv_tmr_init(void)
{
    if(!g_tLightDrvTmr)
    {
        osTimerDef_t tTmrDef = {0};

        tTmrDef.ptimer = light_drv_tmr_cb;
        g_tLightDrvTmr = osTimerCreate(&tTmrDef, osTimerOnce, NULL);

        if(!g_tLightDrvTmr)
        {
            printf("light_drv_tmr: osTimerCreate fail\n");
        }
    }

    return;
}
#endif

static void Hal_Pwm_ComplexConfigSetWrapForFlicker(uint8_t ubIdxMask, S_Hal_Pwm_Config_t *tConfig)
{
    S_Hal_Pwm_Config_t pwm_cfg;

    if (tConfig->ulDutyBright == 255)
    {
        pwm_cfg.ulPeriod     = 255;
        pwm_cfg.ulDutyBright = 255;
        pwm_cfg.ulDutyDull   = 255;
        pwm_cfg.ulRampUp     = 255;
        pwm_cfg.ulRampDown   = 255;
        pwm_cfg.ulHoldBright = 1;
        pwm_cfg.ulHoldDull   = 1;
    }
    else if (tConfig->ulDutyBright == 0)
    {
        pwm_cfg.ulPeriod     = 255;
        pwm_cfg.ulDutyBright = 0;
        pwm_cfg.ulDutyDull   = 0;
        pwm_cfg.ulRampUp     = 255;
        pwm_cfg.ulRampDown   = 255;
        pwm_cfg.ulHoldBright = 1;
        pwm_cfg.ulHoldDull   = 1;
    }
    else
    {
        pwm_cfg.ulPeriod     = USER_LIGHT_FILCKER_HZ;
        pwm_cfg.ulDutyBright = USER_LIGHT_FILCKER_HZ;
        pwm_cfg.ulDutyDull   = 0;
        pwm_cfg.ulRampUp     = USER_LIGHT_FILCKER_HZ;
        pwm_cfg.ulRampDown   = USER_LIGHT_FILCKER_HZ;
        pwm_cfg.ulHoldBright = tConfig->ulDutyBright;
        pwm_cfg.ulHoldDull   = 255 - tConfig->ulDutyBright;
    }

    Hal_Pwm_ComplexConfigSet(ubIdxMask, pwm_cfg);
}

void cancel_default_breath(void)
{
        g_pwm_cconfig[Red_Light].ulPeriod = 255;
        g_pwm_cconfig[Red_Light].ulDutyBright = 0;
        g_pwm_cconfig[Red_Light].ulDutyDull = 0;
        g_pwm_cconfig[Red_Light].ulRampUp = 0;
        g_pwm_cconfig[Red_Light].ulRampDown = 255;
        g_pwm_cconfig[Red_Light].ulHoldDull = 1;
        g_pwm_cconfig[Red_Light].ulHoldBright = 1;	
				
	g_pwm_cconfig[Green_Light].ulPeriod = 255;
        g_pwm_cconfig[Green_Light].ulDutyBright = 0;
        g_pwm_cconfig[Green_Light].ulDutyDull = 0;
        g_pwm_cconfig[Green_Light].ulRampUp = 0;
        g_pwm_cconfig[Green_Light].ulRampDown = 255;
        g_pwm_cconfig[Green_Light].ulHoldDull = 1;
        g_pwm_cconfig[Green_Light].ulHoldBright = 1;	
				
        g_pwm_cconfig[Blue_Light].ulPeriod = 255;
        g_pwm_cconfig[Blue_Light].ulDutyBright = 0;
        g_pwm_cconfig[Blue_Light].ulDutyDull = 0;
        g_pwm_cconfig[Blue_Light].ulRampUp = 0;
        g_pwm_cconfig[Blue_Light].ulRampDown = 255;
        g_pwm_cconfig[Blue_Light].ulHoldDull = 1;
        g_pwm_cconfig[Blue_Light].ulHoldBright = 1;	
				
        g_pwm_cconfig[Cold_Light].ulPeriod = 255;
        g_pwm_cconfig[Cold_Light].ulDutyBright = 0;
        g_pwm_cconfig[Cold_Light].ulDutyDull = 0;
        g_pwm_cconfig[Cold_Light].ulRampUp = 0;
        g_pwm_cconfig[Cold_Light].ulRampDown = 255;
        g_pwm_cconfig[Cold_Light].ulHoldDull = 1;
        g_pwm_cconfig[Cold_Light].ulHoldBright = 1;	

        g_pwm_cconfig[Warm_Light].ulPeriod = 255;
        g_pwm_cconfig[Warm_Light].ulDutyBright = 0;
        g_pwm_cconfig[Warm_Light].ulDutyDull = 0;
        g_pwm_cconfig[Warm_Light].ulRampUp = 0;
        g_pwm_cconfig[Warm_Light].ulRampDown = 255;
        g_pwm_cconfig[Warm_Light].ulHoldDull = 1;
        g_pwm_cconfig[Warm_Light].ulHoldBright = 1;	

        Hal_Pwm_ComplexConfigSetWrapForFlicker(1 << Cold_Light, &g_pwm_cconfig[Cold_Light]);
        Hal_Pwm_ComplexConfigSetWrapForFlicker(1 << Warm_Light, &g_pwm_cconfig[Warm_Light]);
        Hal_Pwm_ComplexConfigSetWrapForFlicker(1 << Red_Light,  &g_pwm_cconfig[Red_Light]);
        Hal_Pwm_ComplexConfigSetWrapForFlicker(1 << Green_Light,&g_pwm_cconfig[Green_Light]);
        Hal_Pwm_ComplexConfigSetWrapForFlicker(1 << Blue_Light, &g_pwm_cconfig[Blue_Light]);

        g_pwm_cconfig[0].ulPeriod = 255;
        g_pwm_cconfig[0].ulDutyBright = 0;
        g_pwm_cconfig[0].ulDutyDull = 0;
        g_pwm_cconfig[0].ulRampUp = 0;
        g_pwm_cconfig[0].ulRampDown = 0;
        g_pwm_cconfig[0].ulHoldDull = 0;
        g_pwm_cconfig[0].ulHoldBright = 0;
}

void fade_timer_smart_init(uint32_t fade_total_period, uint32_t fade_step_period)
{
    opl_light_set_fade_total_period(fade_total_period);
    opl_light_set_fade_step_period(fade_step_period);
    opl_light_fade_step_cnt_calculation();

    #ifdef BLEWIFI_LIGHT_DRV
    light_drv_tmr_init();
    #else
    Hal_Tmr_Init(1);
    Hal_Tmr_CallBackFuncSet(1, fade_step_exe_callback);
    Hal_Tmr_Stop(1);
    #endif
}

void fade_timer_start(void)
{
    #ifdef BLEWIFI_LIGHT_DRV
    osTimerStop(g_tLightDrvTmr);
    osTimerStart(g_tLightDrvTmr, g_fadetiming_ctrl.fade_step_period_ms);
    #else
    Hal_Tmr_Start(1, g_fadetiming_ctrl.fade_step_period_ms * 1000);
    #endif
}


void fade_timer_init(void)
{
    #ifdef BLEWIFI_LIGHT_DRV
    light_drv_tmr_init();
    #else
    //init Timer1
    Hal_Tmr_Init(1);

    //set the timeout callback function
    Hal_Tmr_CallBackFuncSet(1, fade_step_exe_callback);
    #endif
}

static void light_duty_update(int channel, int8_t duty_delta)
{
    if (g_light_fading_config[channel].fade_step_up)
    {
        g_pwm_cconfig[channel].ulDutyBright = g_pwm_cconfig[channel].ulDutyBright + duty_delta;
        g_pwm_cconfig[channel].ulDutyDull = g_pwm_cconfig[channel].ulDutyBright;
        g_pwm_cconfig[channel].ulRampUp = g_pwm_cconfig[channel].ulDutyBright;
        g_pwm_cconfig[channel].ulRampDown = 0;
    }
    else
    {

        g_pwm_cconfig[channel].ulDutyBright = g_pwm_cconfig[channel].ulDutyBright - duty_delta;
        g_pwm_cconfig[channel].ulDutyDull = g_pwm_cconfig[channel].ulDutyBright;
        g_pwm_cconfig[channel].ulRampUp = g_pwm_cconfig[channel].ulDutyBright;
        g_pwm_cconfig[channel].ulRampDown = 0;

    }
}

void fade_step_exe_callback(uint32_t u32TmrIdx)
{
    uint8_t duty_delta = 0;

    for (int i = 1; i < MAX_Light_Ch; i++)
    {
        if (g_light_fading_config[i].fade_en == 1)
        {
            if (g_light_fading_config[i].fade_work_step_count == 0)
            {
                g_light_fading_config[i].fade_nop_count = 0;
                continue;
            }
            else
            {
                if (g_light_fading_config[i].fade_nop_count != 0)
                {
                    if ( (g_light_fading_config[i].fade_nop_count % 2 == 0 && g_light_fading_config[i].fade_work_step_count % 2 == 0) ||
                            (g_light_fading_config[i].fade_nop_count % 2 == 1 && g_light_fading_config[i].fade_work_step_count % 2 == 1)   )
                    {
                        g_light_fading_config[i].fade_nop_count--;
                        continue;
                    }
                    else
                    {
                        duty_delta = g_light_fading_config[i].fade_delta_per_step;

                        if (g_light_fading_config[i].last_round_compensation > 0)
                        {
                            duty_delta += 1;
                            g_light_fading_config[i].last_round_compensation--;
                        }

                        light_duty_update(i, duty_delta);
                        g_light_fading_config[i].fade_work_step_count--;
                    }
                }
                else
                {
                    duty_delta = g_light_fading_config[i].fade_delta_per_step;

                    if (g_light_fading_config[i].last_round_compensation > 0)
                    {
                        duty_delta += 1;
                        g_light_fading_config[i].last_round_compensation--;
                    }

                    light_duty_update(i, duty_delta);
                    g_light_fading_config[i].fade_work_step_count--;
                }
            }
        }
    }

        Hal_Pwm_ComplexConfigSetWrapForFlicker(1 << Red_Light, &g_pwm_cconfig[Red_Light]);
        Hal_Pwm_ComplexConfigSetWrapForFlicker(1 << Green_Light, &g_pwm_cconfig[Green_Light]);
        Hal_Pwm_ComplexConfigSetWrapForFlicker(1 << Blue_Light, &g_pwm_cconfig[Blue_Light]);
        Hal_Pwm_ComplexConfigSetWrapForFlicker(1 << Cold_Light, &g_pwm_cconfig[Cold_Light]);
        Hal_Pwm_ComplexConfigSetWrapForFlicker(1 << Warm_Light, &g_pwm_cconfig[Warm_Light]);

    g_fadetiming_ctrl.fade_step_count--;
    if (g_fadetiming_ctrl.fade_step_count == 0)
    {
        #ifdef BLEWIFI_LIGHT_DRV
        osTimerStop(g_tLightDrvTmr);
        #else
        Hal_Tmr_Stop(1);
        #endif
    }

}

void opl_light_set_fade_step_period(uint32_t fade_step_period_ms)
{
    g_fadetiming_ctrl.fade_step_period_ms = fade_step_period_ms;
}

void opl_light_set_fade_total_period(uint32_t fade_total_period_ms)
{
    g_fadetiming_ctrl.fade_total_period_ms = fade_total_period_ms;
}

void opl_light_fade_step_cnt_calculation(void)
{
    g_fadetiming_ctrl.fade_step_count = g_fadetiming_ctrl.fade_total_period_ms / g_fadetiming_ctrl.fade_step_period_ms;
    if (g_fadetiming_ctrl.fade_total_period_ms % g_fadetiming_ctrl.fade_step_period_ms != 0)
        g_fadetiming_ctrl.fade_step_count++;
}

void opl_light_fade_clear(uint8_t rgbcw_sel)
{
    g_light_fading_config[rgbcw_sel].fade_en = 0;
    g_light_fading_config[rgbcw_sel].fade_step_up = 0;
    g_light_fading_config[rgbcw_sel].fade_nop_count = 0;
    g_light_fading_config[rgbcw_sel].fade_delta_per_step = 0;
    g_light_fading_config[rgbcw_sel].fade_work_step_count = 0;
    g_light_fading_config[rgbcw_sel].last_round_compensation = 0;
}

uint8_t opl_light_duty_get(uint8_t rgbcw_sel)
{
    return g_pwm_cconfig[rgbcw_sel].ulDutyBright;
}

uint8_t opl_light_duty_set(uint8_t rgbcw_sel, uint8_t duty_ticks, uint8_t invert)
{

    if (rgbcw_sel > MAX_Light_Ch)
    {
        printf("Invalid Value[rgbcw_sel]:%d\n", rgbcw_sel);
    }

    if (invert)
    {
        g_pwm_cconfig[rgbcw_sel].ulDutyBright = HAL_PWM_MAX_PERIOD - duty_ticks;
        g_pwm_cconfig[rgbcw_sel].ulDutyDull = HAL_PWM_MAX_PERIOD - duty_ticks;
        g_pwm_cconfig[rgbcw_sel].ulRampUp = HAL_PWM_MAX_PERIOD - duty_ticks;
    }
    else
    {
        g_pwm_cconfig[rgbcw_sel].ulDutyBright = duty_ticks;
        g_pwm_cconfig[rgbcw_sel].ulDutyDull = duty_ticks;
        g_pwm_cconfig[rgbcw_sel].ulRampUp = duty_ticks;
    }
    g_pwm_cconfig[rgbcw_sel].ulPeriod = HAL_PWM_MAX_PERIOD;
    g_pwm_cconfig[rgbcw_sel].ulHoldBright = 1;
    g_pwm_cconfig[rgbcw_sel].ulHoldDull = 1;

    Hal_Pwm_ComplexConfigSetWrapForFlicker(1 << rgbcw_sel, &g_pwm_cconfig[rgbcw_sel]);

    return 0;
}

SHM_DATA uint8_t opl_light_fade_with_time(uint8_t rgbcw_sel, uint8_t target_duty, uint32_t fade_total_period_ms, uint32_t fade_step_period_ms, uint8_t invert)
{
    int tick_step_balance = 0;
    uint32_t fade_step_count = 0;

    if (rgbcw_sel > MAX_Light_Ch)   //value check
    {
        printf("Invalid Value[rgbcw_sel]:%d\n", rgbcw_sel);
    }

    if (fade_total_period_ms == 0 || (fade_total_period_ms < fade_step_period_ms))
    {
        g_light_fading_config[rgbcw_sel].fade_en = 0;
        return opl_light_duty_set(rgbcw_sel, target_duty, invert);
    }

    //clear all variable related with fading
    opl_light_fade_clear(rgbcw_sel);

    uint8_t current_duty = opl_light_duty_get(rgbcw_sel);
    int duty_delta = target_duty - current_duty;

    if (duty_delta > 0)
    {
        g_light_fading_config[rgbcw_sel].fade_step_up = 1;
    }
    else
    {
        duty_delta = 0 - duty_delta;
        g_light_fading_config[rgbcw_sel].fade_step_up = 0;
    }

    fade_step_count = fade_total_period_ms / fade_step_period_ms;
    if (fade_total_period_ms % fade_step_period_ms != 0)
        fade_step_count++;

    g_light_fading_config[rgbcw_sel].fade_delta_per_step = duty_delta / fade_step_count;


    if (g_light_fading_config[rgbcw_sel].fade_delta_per_step < 1)
    {
        for (tick_step_balance = fade_step_count ; tick_step_balance > 0 ; tick_step_balance--)
        {
            if (duty_delta / tick_step_balance == 1)
                break;
        }

        // tick_step_balance = min(fade_step_count, duty_delta)
        if (tick_step_balance != 0)
        {
            g_light_fading_config[rgbcw_sel].fade_en = 1;
            g_light_fading_config[rgbcw_sel].fade_delta_per_step = 1;
            g_light_fading_config[rgbcw_sel].fade_nop_count = fade_step_count - tick_step_balance;
            g_light_fading_config[rgbcw_sel].fade_work_step_count = tick_step_balance;
        }
        else
        {
            g_light_fading_config[rgbcw_sel].fade_en = 0;
            return opl_light_duty_set(rgbcw_sel, target_duty, invert);
        }
    }
    else
    {
        g_light_fading_config[rgbcw_sel].fade_en = 1;
        g_light_fading_config[rgbcw_sel].fade_work_step_count = fade_step_count;
        if (duty_delta % fade_step_count != 0)
            g_light_fading_config[rgbcw_sel].last_round_compensation = duty_delta % fade_step_count;
    }

    return 0;
}

uint8_t opl_light_fade_with_time_central_fade_ctrl(uint8_t rgbcw_sel, uint8_t target_duty, uint8_t invert)
{
    return opl_light_fade_with_time(rgbcw_sel, target_duty, g_fadetiming_ctrl.fade_total_period_ms, g_fadetiming_ctrl.fade_step_period_ms, invert);
}
