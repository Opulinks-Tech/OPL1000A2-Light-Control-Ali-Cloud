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

#include <stdio.h>
#include <string.h>
#include "opl_light_driver.h"
#include "light_control.h"
#include "cmsis_os.h"
#include "ali_linkkitsdk_decl.h"
#include "blewifi_ctrl.h"
#include "blewifi_configuration.h"
#include "infra_config.h"

#define CTB_FADE_PERIOD_MS 1000
#define CTB_FADE_STEP_MS  10
#define HSV_FADE_PERIOD_MS 400
#define HSV_FADE_STEP_MS  10
#define GENERAL_FADE_PERIOD_MS   120
#define GENERAL_FADE_STEP_MS     1

#if (LIGHT_CTRL_DEBUG)
    #define L_CTRL_DBGMSG(FORMAT, ARGS) printf(FORMAT, ARGS)
#else
    #define L_CTRL_DBGMSG(FORMAT, ARGS)
#endif

/**
 * @brief The state of the five-color light
 */
uint8_t g_light_effect_status = 0;

typedef struct
{
    uint8_t light_type;
    uint8_t on;
    uint8_t mode;
    uint8_t power_level;
    uint8_t workmode;
    uint16_t hue;
    uint8_t saturation;
    uint8_t value;
    uint16_t scene_hue;
    uint8_t scene_saturation;
    uint8_t scene_value;
    uint32_t color_temperature;
    uint8_t brightness;
    uint32_t fade_period_ms;
    uint32_t fade_step_ms;
    uint8_t rhyon;
} light_status_t;

typedef struct
{
    uint16_t hue;
    uint8_t saturation;
    uint8_t value;
} manual_light_status_t;

static light_status_t g_light_status = {0};
static manual_light_status_t g_manual_light_status={0, 100, 100};
/*SHM_DATA */light_effect_t g_light_effect = {0};
TimerHandle_t g_TimerLightEffect;

uint16_t g_u16ColorSpeedPer = 0;

#ifdef LIGHT_CTRL_MUTEX
osSemaphoreId g_tLightCtrlSem = NULL;

int light_ctrl_lock(void)
{
    int iRet = -1;
    int iStatus = 0;

    if(!g_tLightCtrlSem)
    {
        //printf("[%s %d] sem is null\n", __func__, __LINE__);
        goto done;
    }

    iStatus = osSemaphoreWait(g_tLightCtrlSem, osWaitForever);

    if(iStatus != osOK)
    {
        //printf("[%s %d] osSemaphoreWait fail, status[%d]\n", __func__, __LINE__, iStatus);
        goto done;
    }

    iRet = 0;

done:
    return iRet;
}

int light_ctrl_unlock(void)
{
    int iRet = -1;

    if(!g_tLightCtrlSem)
    {
        //printf("[%s %d] sem is null\n", __func__, __LINE__);
        goto done;
    }
    
    if(osSemaphoreRelease(g_tLightCtrlSem) != osOK)
    {
        //printf("[%s %d] osSemaphoreWait fail\n", __func__, __LINE__);
        goto done;
    }

    iRet = 0;

done:
    return iRet;
}
#else
#define light_ctrl_lock(...)
#define light_ctrl_unlock(...)
#endif

void light_state_debug_print(void)
{
    char s_mode[8]={0};
    char s_light_type[8]={0};
    char s_on[8]={0};
    char s_power_level[8]={0};
    char s_workmode[10]={0};

    switch(g_light_status.on)
    {
        case SWITCH_ON:
            sprintf(s_on,"ON");
            break;
        case SWITCH_OFF:
            sprintf(s_on,"OFF");
            break;
        default:
            sprintf(s_on,"UNDEF");
            break;
    }

    switch(g_light_status.light_type)
    {
        case LT_C:
            sprintf(s_light_type,"C");
            break;
        case LT_CW:
            sprintf(s_light_type,"CW");
            break;
        case LT_RGB:
            sprintf(s_light_type,"RGB");
            break;
        case LT_RGBC:
            sprintf(s_light_type,"RGBC");
            break;
        case LT_RGBCW:
            sprintf(s_light_type,"RGBCW");
            break;
        default:
            sprintf(s_light_type,"UNDEF");
            break;
    }

    switch(g_light_status.power_level)
    {
        case HF_PWR:
            sprintf(s_power_level,"HF");
            break;
        case FULL_PWR:
            sprintf(s_power_level,"F");
            break;
        default:
            sprintf(s_power_level,"UNDEF");
            break;
    }
    switch(g_light_status.mode)
    {
        //case MODE_NONE:
        //    sprintf(s_mode,"NONE");
        //    break;
        //case MODE_OFF:
        //    sprintf(s_mode,"OFF");
        //    break;
        case MODE_CTB:
            sprintf(s_mode,"CTB");
            break;
        case MODE_HSV:
            sprintf(s_mode,"HSV");
            break;
        default:
            sprintf(s_mode,"UNDEF");
            break;
    }

    switch(g_light_status.workmode)
    {
        case WMODE_MANUAL:
            sprintf(s_workmode,"MANUAL");
            break;
        case WMODE_READING:
            sprintf(s_workmode,"READING");
            break;
        case WMODE_CINEMA:
            sprintf(s_workmode,"CINEMA");
            break;
        case WMODE_MIDNIGHT:
            sprintf(s_workmode,"MIDNIGHT");
            break;
        case WMODE_LIVING:
            sprintf(s_workmode,"LIVING");
            break;
        case WMODE_SOFT:
            sprintf(s_workmode,"SOFT");
            break;
        default:
            sprintf(s_workmode,"UNDEF");
            break;
    }
    printf("-----g_light_status-----\n");
    printf("light_type:%s\n",s_light_type);
    printf("on:%s\n",s_on);
    printf("mode:%s\n",s_mode);
    printf("power_level:%s\n",s_power_level);
    printf("workmode:%s\n",s_workmode);
    printf("hue:%d\n",g_light_status.hue);
    printf("saturation:%d\n",g_light_status.saturation);
    printf("value:%d\n",g_light_status.value);
    printf("color_temperature:%d\n",g_light_status.color_temperature);
    printf("brightness:%d\n",g_light_status.brightness);
    printf("fade_period_ms:%d\n",g_light_status.fade_period_ms);
    printf("fade_step_ms:%d\n",g_light_status.fade_step_ms);
    printf("------------------------\n");
}


void light_effect_set_status(uint8_t light_effect_status)
{
    light_ctrl_lock();

    g_light_effect_status = light_effect_status;

    light_ctrl_unlock();
}

uint8_t light_effect_get_status(void)
{
#ifdef LIGHT_CTRL_MUTEX
    uint8_t u8Value = 0;

    light_ctrl_lock();

    u8Value = g_light_effect_status;

    light_ctrl_unlock();

    return u8Value;
#else
    return g_light_effect_status;
#endif
}

void colortemperature_force_overwrite(uint32_t colortemp)
{
    light_ctrl_lock();

    g_light_status.color_temperature = colortemp;

    light_ctrl_unlock();
}

uint8_t normalize_colortemperature(uint32_t colortemp_k_unit)
{
    return ((colortemp_k_unit-2000)*100/5000);
}

void light_ctrl_set_light_type(uint8_t light_type,uint8_t power_level)
{
    light_ctrl_lock();

    g_light_status.light_type = light_type;
    g_light_status.power_level = power_level;

    light_ctrl_unlock();
}

uint8_t light_ctrl_get_light_type(void)
{
#ifdef LIGHT_CTRL_MUTEX
    uint8_t u8Value = 0;

    light_ctrl_lock();

    u8Value = g_light_status.light_type;

    light_ctrl_unlock();

    return u8Value;
#else
    return g_light_status.light_type;
#endif
}


void light_ctrl_set_manual_light_status(uint16_t hue,uint8_t saturation,uint8_t value)
{
    light_ctrl_lock();

    g_manual_light_status.hue = hue;
    g_manual_light_status.saturation = saturation;
    g_manual_light_status.value = value;

    light_ctrl_unlock();
}

void light_ctrl_get_manual_light_status(uint16_t *hue,uint8_t *saturation,uint8_t *value)
{
    light_ctrl_lock();

    *hue = g_manual_light_status.hue;
    *saturation = g_manual_light_status.saturation;
    *value = g_manual_light_status.value;

    light_ctrl_unlock();
}

void light_ctrl_set_fade_period_ms(uint32_t fade_period_ms)
{
    light_ctrl_lock();

    g_light_status.fade_period_ms = fade_period_ms;

    light_ctrl_unlock();
}

void light_ctrl_set_fade_step_ms(uint32_t fade_step_ms)
{
    light_ctrl_lock();

    g_light_status.fade_step_ms = fade_step_ms;

    light_ctrl_unlock();
}

void light_ctrl_set_scenescolor(uint16_t hue, uint8_t saturation, uint8_t value)
{
    light_ctrl_lock();

    g_light_status.scene_hue = hue;
    g_light_status.scene_saturation = saturation;
    g_light_status.scene_value = value;

    light_ctrl_unlock();
}

void light_ctrl_get_scenescolor(uint16_t *hue, uint8_t *saturation, uint8_t *value)
{
    light_ctrl_lock();

    *hue = g_light_status.scene_hue;
    *saturation = g_light_status.scene_saturation;
    *value = g_light_status.scene_value;

    light_ctrl_unlock();
}

void light_ctrl_set_workmode(uint8_t workmode)
{
    light_ctrl_lock();

    g_light_status.workmode = workmode;

    light_ctrl_unlock();
}

uint8_t light_ctrl_get_workmode(void)
{
#ifdef LIGHT_CTRL_MUTEX
    uint8_t u8Value = 0;

    light_ctrl_lock();

    u8Value = g_light_status.workmode;

    light_ctrl_unlock();

    return u8Value;
#else
    return g_light_status.workmode;
#endif
}

void light_ctrl_init()
{
    memset(&g_light_status, 0, sizeof(light_status_t));
    g_light_status.mode              = MODE_CTB;
    g_light_status.on                = SWITCH_ON;
    g_light_status.light_type        = LT_RGBC;
    g_light_status.power_level       = HF_PWR;
    g_light_status.hue               = 0;
    g_light_status.saturation        = 100;
    g_light_status.value             = 100;
    g_light_status.color_temperature = 2000;
    g_light_status.brightness        = 100;
    g_light_status.fade_period_ms    = GENERAL_FADE_PERIOD_MS;
    g_light_status.fade_step_ms      = GENERAL_FADE_STEP_MS;
    g_light_status.workmode          = WMODE_MANUAL;

    #ifdef LIGHT_CTRL_MUTEX
    if(g_tLightCtrlSem == NULL)
    {
        osSemaphoreDef_t tSemDef = {0};

        g_tLightCtrlSem = osSemaphoreCreate(&tSemDef, 1);
    }
    #endif

    /* initialize effect control*/
    light_effect_init();

    // initialize manual light status
    light_ctrl_set_manual_light_status(g_light_status.hue, g_light_status.saturation, g_light_status.value);
}

static uint8_t light_ctrl_hsv2rgb(uint16_t hue, uint8_t saturation, uint8_t value,
                                  uint8_t *red, uint8_t *green, uint8_t *blue)
{
    uint16_t hi = (hue / 60) % 6;
    uint16_t F = 100 * hue / 60 - 100 * hi;
    uint16_t P = value * (100 - saturation) / 100;
    uint16_t Q = value * (10000 - F * saturation) / 10000;
    uint16_t T = value * (10000 - saturation * (100 - F)) / 10000;

    switch (hi)
    {
        case 0:
            *red   = value;
            *green = T;
            *blue  = P;
            break;

        case 1:
            *red   = Q;
            *green = value;
            *blue  = P;
            break;

        case 2:
            *red   = P;
            *green = value;
            *blue  = T;
            break;

        case 3:
            *red   = P;
            *green = Q;
            *blue  = value;
            break;

        case 4:
            *red   = T;
            *green = P;
            *blue  = value;
            break;

        case 5:
            *red   = value;
            *green = P;
            *blue  = Q;
            break;

        default:
            return 0;
    }

    *red   = *red * 255 / 100;
    *green = *green * 255 / 100;
    *blue  = *blue * 255 / 100;

    return 0;
}

static void light_ctrl_rgb2hsv(uint16_t red, uint16_t green, uint16_t blue,
                                 uint16_t *h, uint8_t *s, uint8_t *v)
{
    double hue, saturation, value;
    double m_max = MAX(red, MAX(green, blue));
    double m_min = MIN(red, MIN(green, blue));
    double m_delta = m_max - m_min;

    value = m_max / 255.0;

    if (m_delta == 0) {
        hue = 0;
        saturation = 0;
    } else {
        saturation = m_delta / m_max;

        if (red == m_max) {
            hue = (green - blue) / m_delta;
        } else if (green == m_max) {
            hue = 2 + (blue - red) / m_delta;
        } else {
            hue = 4 + (red - green) / m_delta;
        }

        hue = hue * 60;

        if (hue < 0) {
            hue = hue + 360;
        }
    }

    *h = (int)(hue + 0.5);
    *s = (int)(saturation * 100 + 0.5);
    *v = (int)(value * 100 + 0.5);
}

uint8_t light_ctrl_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    uint16_t hue = 0;

    uint8_t saturation = 0, value = 0;

    light_ctrl_lock();

    fade_timer_smart_init(HSV_FADE_PERIOD_MS, HSV_FADE_STEP_MS);

    opl_light_fade_with_time_central_fade_ctrl(Red_Light, red, 0);

    opl_light_fade_with_time_central_fade_ctrl(Green_Light, green, 0);

    opl_light_fade_with_time_central_fade_ctrl(Blue_Light, blue, 0);

    opl_light_fade_with_time(Cold_Light, 0, 0, 0, 0);

    opl_light_fade_with_time(Warm_Light, 0, 0, 0, 0);

    fade_timer_start();

    light_ctrl_rgb2hsv(red, green, blue, &hue, &saturation, &value);

    g_light_status.hue = hue;

    g_light_status.saturation = saturation;

    g_light_status.value = value;

    g_light_status.mode = MODE_HSV;

    light_ctrl_unlock();

#if W_DEBUG
    printf("light_ctrl_set_rgb()\n");
    light_state_debug_print();
#endif
    return 0;
}


SHM_DATA uint8_t light_ctrl_set_hsv(uint16_t hue, uint8_t saturation, uint8_t value, uint8_t fade, uint8_t led_status_update, uint8_t rgb_lighten_per)
{
    if (hue > 360 || saturation > 100 || value > 100 || rgb_lighten_per > 100)
    {
        L_CTRL_DBGMSG("Invalid Value: [hue:%d],[saturation:%d],[value:%d],[rgb_lighten_per:%d]\n", (hue, saturation, value, rgb_lighten_per));
        return 0;
    }

    uint8_t red   = 0;
    uint8_t green = 0;
    uint8_t blue  = 0;

    light_ctrl_hsv2rgb(hue, saturation, value, &red, &green, &blue);

    if(rgb_lighten_per)
    {
        red = (red*rgb_lighten_per)/100;
        green = (green*rgb_lighten_per)/100;
        blue = (blue*rgb_lighten_per)/100;
    }

    light_ctrl_lock();

    if(fade)
    {
        fade_timer_smart_init(HSV_FADE_PERIOD_MS, HSV_FADE_STEP_MS);

        opl_light_fade_with_time_central_fade_ctrl(Red_Light, red, 0);

        opl_light_fade_with_time_central_fade_ctrl(Green_Light, green, 0);

        opl_light_fade_with_time_central_fade_ctrl(Blue_Light, blue, 0);

        //if (g_light_status.mode == MODE_CTB)
        {
            opl_light_fade_with_time_central_fade_ctrl(Cold_Light, 0, 0);

            opl_light_fade_with_time_central_fade_ctrl(Warm_Light, 0, 0);
        }
        fade_timer_start();
    }
    else
    {
        opl_light_fade_with_time(Red_Light, red, 0, 0, 0);

        opl_light_fade_with_time(Green_Light, green, 0, 0, 0);

        opl_light_fade_with_time(Blue_Light, blue, 0, 0, 0);

        //if (g_light_status.mode == MODE_CTB)
        {
            opl_light_fade_with_time(Cold_Light, 0, 0, 0, 0);

            opl_light_fade_with_time(Warm_Light, 0, 0, 0, 0);
        }
    }

    if (led_status_update)
    {
        g_light_status.mode       = MODE_HSV;
        g_light_status.on         = SWITCH_ON;
        g_light_status.hue        = hue;
        g_light_status.value      = value;
        g_light_status.saturation = saturation;
    }

    light_ctrl_unlock();

#if W_DEBUG
    printf("light_ctrl_set_hsv()\n");
    light_state_debug_print();
#endif

    return 0;
}

uint8_t light_ctrl_set_hue(uint16_t hue)
{
    return light_ctrl_set_hsv(hue, g_light_status.saturation, g_light_status.value, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
}

uint8_t light_ctrl_set_saturation(uint8_t saturation)
{
    return light_ctrl_set_hsv(g_light_status.hue, saturation, g_light_status.value, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
}

uint8_t light_ctrl_set_value(uint8_t value)
{
    return light_ctrl_set_hsv(g_light_status.hue, g_light_status.saturation, value, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
}

uint8_t light_ctrl_get_hsv(uint16_t *hue, uint8_t *saturation, uint8_t *value)
{
    if (hue == NULL || saturation == NULL || value == NULL)
    {
        L_CTRL_DBGMSG("*hue,*saturation,*value can not be NULL\n",());
        return 0;
    }

    light_ctrl_lock();

    *hue        = g_light_status.hue;
    *saturation = g_light_status.saturation;
    *value      = g_light_status.value;

    light_ctrl_unlock();

    return 0;
}

uint16_t light_ctrl_get_hue()
{
#ifdef LIGHT_CTRL_MUTEX
    uint16_t u16Value = 0;

    light_ctrl_lock();

    u16Value = g_light_status.hue;

    light_ctrl_unlock();

    return u16Value;
#else
    return g_light_status.hue;
#endif
}

uint8_t light_ctrl_get_saturation()
{
#ifdef LIGHT_CTRL_MUTEX
    uint8_t u8Value = 0;

    light_ctrl_lock();

    u8Value = g_light_status.saturation;

    light_ctrl_unlock();

    return u8Value;
#else
    return g_light_status.saturation;
#endif
}

uint8_t light_ctrl_get_value()
{
#ifdef LIGHT_CTRL_MUTEX
    uint8_t u8Value = 0;

    light_ctrl_lock();

    u8Value = g_light_status.value;

    light_ctrl_unlock();

    return u8Value;
#else
    return g_light_status.value;
#endif
}

uint8_t light_ctrl_get_mode()
{
#ifdef LIGHT_CTRL_MUTEX
    uint8_t u8Value = 0;

    light_ctrl_lock();

    u8Value = g_light_status.mode;

    light_ctrl_unlock();

    return u8Value;
#else
    return g_light_status.mode;
#endif
}

void light_ctrl_set_mode(uint8_t mode)
{
    light_ctrl_lock();

    g_light_status.mode = mode;

    light_ctrl_unlock();
}

SHM_DATA uint8_t light_ctrl_set_ctb(uint32_t color_temperature, uint8_t brightness, uint8_t fade, uint8_t status_update)
{
    uint8_t u8WarmLightEnabled = 0;
    uint32_t brightness_255 = 0;
    uint8_t warm_tmp = 0;
    uint8_t cold_tmp = 0;

    if (brightness > 100)
    {
        L_CTRL_DBGMSG("brightness out of range\n",());
        return 0;
    }

    if (color_temperature < 2000 || color_temperature > 7000)
    {
        L_CTRL_DBGMSG("color_temperature or brightness out of range\n",());
        return 0;
    }

    if((g_light_status.light_type == LT_CW) || 
       (g_light_status.light_type == LT_RGBCW))
    {
        u8WarmLightEnabled = 1;
    }

    if(u8WarmLightEnabled)
    {
        uint8_t colortemp_percent = normalize_colortemperature(color_temperature);

        #if 1
        cold_tmp = colortemp_percent * brightness / 100;
        warm_tmp = (100 - colortemp_percent) * brightness / 100;
        #else
        warm_tmp = colortemp_percent * brightness / 100;
        cold_tmp = (100 - colortemp_percent) * brightness / 100;
        #endif
        
        warm_tmp = warm_tmp < 15 ? warm_tmp : 14 + warm_tmp * 86 / 100;
        cold_tmp = cold_tmp < 15 ? cold_tmp : 14 + cold_tmp * 86 / 100;
    }
    else
    {
        if (brightness>0)
        {
            brightness_255 = ((brightness*242)/100)+13;
        }
    }

    light_ctrl_lock();
			
    if (fade)
    {
        fade_timer_smart_init(CTB_FADE_PERIOD_MS, CTB_FADE_STEP_MS);

        if(u8WarmLightEnabled)
        {
            opl_light_fade_with_time_central_fade_ctrl(Cold_Light, cold_tmp * 255 / 100, 0);
    
            opl_light_fade_with_time_central_fade_ctrl(Warm_Light, warm_tmp * 255 / 100, 0);
        }
        else
        {
            opl_light_fade_with_time_central_fade_ctrl(Cold_Light, brightness_255, 0);        //Control 4th way  (No matter Cold or Warm)
        }

        //if (g_light_status.mode != MODE_CTB)
        {
            opl_light_fade_with_time_central_fade_ctrl(Red_Light, 0, 0);

            opl_light_fade_with_time_central_fade_ctrl(Green_Light, 0, 0);

            opl_light_fade_with_time_central_fade_ctrl(Blue_Light, 0, 0);
        }

        fade_timer_start();
    }
    else
    {
        if(u8WarmLightEnabled)
        {
            opl_light_fade_with_time(Cold_Light, cold_tmp * 255 / 100, 0, 0, 0);
    
            opl_light_fade_with_time(Warm_Light, warm_tmp * 255 / 100, 0, 0, 0);
        }
        else
        {
            opl_light_fade_with_time(Cold_Light, brightness_255, 0, 0, 0);        //Control 4th way  (No matter Cold or Warm)
        }

        //if (g_light_status.mode != MODE_CTB)
        {
            opl_light_fade_with_time(Red_Light, 0, 0, 0, 0);

            opl_light_fade_with_time(Green_Light, 0, 0, 0, 0);

            opl_light_fade_with_time(Blue_Light, 0, 0, 0, 0);
        }
    }

    if(status_update)
    {
        g_light_status.mode              = MODE_CTB;
        g_light_status.on                = SWITCH_ON;
        g_light_status.brightness        = brightness;
        g_light_status.color_temperature = color_temperature;
    }

    light_ctrl_unlock();

#if W_DEBUG
    printf("light_ctrl_set_ctb()\n");
    light_state_debug_print();
#endif

    return 0;
}

uint8_t light_ctrl_set_color_temperature(uint32_t color_temperature)
{
    return light_ctrl_set_ctb(color_temperature, g_light_status.brightness, LIGHT_FADE_ON, UPDATE_LED_STATUS);
}

uint8_t light_ctrl_set_brightness(uint8_t brightness)
{
    return light_ctrl_set_ctb(g_light_status.color_temperature, brightness, LIGHT_FADE_ON, UPDATE_LED_STATUS);
}

uint8_t light_ctrl_get_ctb(uint8_t *brightness)
{
    if (brightness == NULL)
    {
        L_CTRL_DBGMSG("*brightness can not be NULL\n",());
        return 0;
    }

    light_ctrl_lock();

    *brightness        = g_light_status.brightness;

    light_ctrl_unlock();

    return 0;
}

uint32_t light_ctrl_get_color_temperature()
{
#ifdef LIGHT_CTRL_MUTEX
    uint32_t u32Value = 0;

    light_ctrl_lock();

    u32Value = g_light_status.color_temperature;

    light_ctrl_unlock();

    return u32Value;
#else
    return g_light_status.color_temperature;
#endif
}

uint8_t light_ctrl_get_brightness()
{
#ifdef LIGHT_CTRL_MUTEX
    uint32_t u32Value = 0;

    light_ctrl_lock();

    u32Value = g_light_status.brightness;

    light_ctrl_unlock();

    return u32Value;
#else
    return g_light_status.brightness;
#endif
}

uint8_t light_ctrl_set_rhyon(uint8_t on)
{
    light_ctrl_lock();

    g_light_status.rhyon = on;

    light_ctrl_unlock();
    return 0;
}

uint8_t light_ctrl_set_switch(uint8_t on)
{
    //uint8_t workmode_tmp = 0;
    uint32_t tmp_color_temperature = g_light_status.color_temperature;

    g_light_status.on = on;

    if (!g_light_status.on)
    {
        light_ctrl_lock();

        fade_timer_smart_init(g_light_status.fade_period_ms, g_light_status.fade_step_ms);

        opl_light_fade_with_time_central_fade_ctrl(Red_Light, 0, 0);

        opl_light_fade_with_time_central_fade_ctrl(Green_Light, 0, 0);

        opl_light_fade_with_time_central_fade_ctrl(Blue_Light, 0, 0);

        opl_light_fade_with_time_central_fade_ctrl(Cold_Light, 0, 0);

        if((g_light_status.light_type == LT_CW) || 
           (g_light_status.light_type == LT_RGBCW))
        {
            opl_light_fade_with_time_central_fade_ctrl(Warm_Light, 0, 0);
        }

        fade_timer_start();

        light_ctrl_unlock();
    }
    else
    {
        switch (g_light_status.mode)
        {
            case MODE_HSV:
                g_light_status.value = (g_light_status.value) ? g_light_status.value : 100;
                light_ctrl_set_hsv(g_light_status.hue, g_light_status.saturation, g_light_status.value, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
                break;

            case MODE_CTB:
                if(light_ctrl_get_light_type() != LT_RGB)
                {
                    g_light_status.brightness = (g_light_status.brightness) ? g_light_status.brightness : 100;
                    if (g_light_status.brightness == 1)
                    {
                        if(tmp_color_temperature > 4500)
                        {
                            light_ctrl_set_ctb(7000, 1, LIGHT_FADE_ON, UPDATE_LED_STATUS);
                        }
                        else
                        {
                            light_ctrl_set_ctb(2000, 1, LIGHT_FADE_ON, UPDATE_LED_STATUS);
                        }
                        //g_light_status.color_temperature = tmp_color_temperature;
                    }
                    else
                    {
                        light_ctrl_set_ctb(g_light_status.color_temperature, g_light_status.brightness, LIGHT_FADE_ON, UPDATE_LED_STATUS);
                    }
                }

                break;
						
            case MODE_SCENES:
                // Do not set ScenesColor ("iot_set_scenes_color()") here.
                // ScenesColor in App (scene page) would be overwritten by first color of ColorArr immediately when ColorArr is active.
                break;

            default:
                L_CTRL_DBGMSG("This operation is not supported",());
                break;
        }
    }

#if W_DEBUG
    printf("light_ctrl_set_switch()\n");
    light_state_debug_print();
#endif

    return 0;
}

uint8_t light_ctrl_get_rhyon()
{
#ifdef LIGHT_CTRL_MUTEX
    uint8_t u8Value = 0;

    light_ctrl_lock();

    u8Value = g_light_status.rhyon;

    light_ctrl_unlock();

    return u8Value;
#else
    return g_light_status.rhyon;
#endif
}


uint8_t light_ctrl_get_switch()
{
#ifdef LIGHT_CTRL_MUTEX
    uint8_t u8Value = 0;

    light_ctrl_lock();

    u8Value = g_light_status.on;

    light_ctrl_unlock();

    return u8Value;
#else
    return g_light_status.on;
#endif
}

#ifdef BLEWIFI_LIGHT_CTRL
void light_effect_update(void)
{
    uint16_t hue;
    uint8_t saturation, value;
    uint8_t index = g_light_effect.color_index;

    if((!light_ctrl_get_switch()) || (light_ctrl_get_mode() != MODE_SCENES))
    {
        goto done;
    }

    hue = g_light_effect.color_arr[index].hue;
    saturation = g_light_effect.color_arr[index].saturation;
    value = g_light_effect.color_arr[index].value;

    if (g_light_effect.effect_type == EFFTCT_TYPE_FLASH)
    {
        light_ctrl_set_hsv(hue, saturation, value, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    }
    else
    {
        light_ctrl_set_hsv(hue, saturation, value, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);

    }

    g_light_effect.color_index++;

    if (g_light_effect.color_index >= g_light_effect.color_num)
        g_light_effect.color_index = 0;

done:
    return;
}

static void light_effect_timer_cb(void* timer)
{
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_LIGHT_CTRL_TIMEOUT, NULL, 0);
}
#else
static void light_effect_timer_cb(void* timer)
{
    uint16_t hue;
    uint8_t saturation, value;
    uint8_t index = g_light_effect.color_index;

    if((!light_ctrl_get_switch()) || (light_ctrl_get_mode() != MODE_SCENES))
    {
        goto done;
    }

    hue = g_light_effect.color_arr[index].hue;
    saturation = g_light_effect.color_arr[index].saturation;
    value = g_light_effect.color_arr[index].value;

    if (g_light_effect.effect_type == EFFTCT_TYPE_FLASH)
    {
        light_ctrl_set_hsv(hue, saturation, value, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    }
    else
    {
        light_ctrl_set_hsv(hue, saturation, value, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);

    }

    g_light_effect.color_index++;

    if (g_light_effect.color_index >= g_light_effect.color_num)
        g_light_effect.color_index = 0;

done:
    return;
}
#endif

void light_effect_init(void)
{
    //TODO: load default from flash
    memset(&g_light_effect, 0 , sizeof(light_effect_t));

    g_light_effect.effect_type      = EFFTCT_TYPE_FLASH;
    g_light_effect.color_num        = 0;
    g_light_effect.color_index      = 0;
    g_light_effect.transition_speed = LIGHT_COLOR_SPEED_TIME_MAX >> 1;
    g_light_effect.fade_step_ms     = 3;

    g_u16ColorSpeedPer = 100;
}

void light_effect_config(light_effect_t *config)
{
    g_light_effect.effect_type      = config->effect_type;
    g_light_effect.color_num        = config->color_num;
    //g_light_effect.color_index      = config->color_index;
    g_light_effect.transition_speed = config->transition_speed;
    g_light_effect.fade_step_ms     = config->fade_step_ms;

    memcpy(&g_light_effect.color_arr[0], &config->color_arr[0], config->color_num * sizeof(hsv_t));

    L_CTRL_DBGMSG("effect :%d\r\n", g_light_effect.effect_type);
    L_CTRL_DBGMSG("color_num :%d\r\n", g_light_effect.color_num);
    L_CTRL_DBGMSG("color_index :%d\r\n", g_light_effect.color_index);
    L_CTRL_DBGMSG("transition_speed :%d\r\n", g_light_effect.transition_speed);
    L_CTRL_DBGMSG("fade_step_ms :%d\r\n", g_light_effect.fade_step_ms);

    for (int i=0;i<g_light_effect.color_num; i++)
    {
       L_CTRL_DBGMSG("h[%d]:%d s[%d]:%d s[%d]:%d \r\n", (i, g_light_effect.color_arr[i].hue,
                                                 i, g_light_effect.color_arr[i].saturation,
                                                 i, g_light_effect.color_arr[i].value));
    }

}

void light_effect_set_color_array(hsv_t *color_arr, uint16_t color_num)
{
    g_light_effect.color_num = color_num;

    memcpy(&g_light_effect.color_arr[0], color_arr, color_num * sizeof(hsv_t));

    for (int i=0;i<g_light_effect.color_num; i++)
    {
       L_CTRL_DBGMSG("h[%d]:%d s[%d]:%d s[%d]:%d \r\n", (i, g_light_effect.color_arr[i].hue,
                                                 i, g_light_effect.color_arr[i].saturation,
                                                 i, g_light_effect.color_arr[i].value));
    }
}

void light_effect_set_effect_type(uint16_t    effect_type)
{
    g_light_effect.effect_type = effect_type;
}

void light_effect_start(light_effect_t *handle, uint16_t color_index)
{
    if (g_light_effect.effect_type == EFFTCT_TYPE_FLASH)
    {

        /* change to specified first color with fading */
        light_ctrl_set_fade_period_ms(400);
        light_ctrl_set_fade_step_ms(2);
        light_ctrl_set_hsv(g_light_effect.color_arr[color_index].hue,
                           g_light_effect.color_arr[color_index].saturation,
                           g_light_effect.color_arr[color_index].value, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);

        color_index++;
        color_index = color_index % g_light_effect.color_num;
        g_light_effect.color_index = color_index;


        /* color transiton speed and fading step setting */
        light_ctrl_set_fade_period_ms(g_light_effect.transition_speed - 5);
        light_ctrl_set_fade_step_ms(g_light_effect.fade_step_ms);
    }
    else
    {
        /* change to specified first color immediately */
        g_light_effect.color_index = color_index;
        light_ctrl_set_hsv(g_light_effect.color_arr[color_index].hue,
                           g_light_effect.color_arr[color_index].saturation,
                           g_light_effect.color_arr[color_index].value, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);

        color_index++;
        color_index = color_index % g_light_effect.color_num;
        g_light_effect.color_index = color_index;
    }

    L_CTRL_DBGMSG("effect :%d\r\n", g_light_effect.effect_type);
    L_CTRL_DBGMSG("color_num :%d\r\n", g_light_effect.color_num);
    L_CTRL_DBGMSG("color_index :%d\r\n", g_light_effect.color_index);
    L_CTRL_DBGMSG("transition_speed :%d\r\n", g_light_effect.transition_speed);
    L_CTRL_DBGMSG("fade_step_ms :%d\r\n", g_light_effect.fade_step_ms);

    if (!g_TimerLightEffect)
    {
        g_TimerLightEffect = xTimerCreate("light_timer",
                                          g_light_effect.transition_speed,
                                          pdTRUE, (void *)0, light_effect_timer_cb);
    }

    if (g_TimerLightEffect != NULL)
    {
        xTimerStart(g_TimerLightEffect, 0);
    }
}

void light_effect_stop(light_effect_t *handle)
{
    if (g_TimerLightEffect)
    {
        xTimerStop(g_TimerLightEffect, portMAX_DELAY);
        //restore default fade time
        light_ctrl_set_fade_period_ms(GENERAL_FADE_PERIOD_MS);
        light_ctrl_set_fade_step_ms(GENERAL_FADE_STEP_MS);
    }
}

void light_effect_speed_increase(uint16_t change_time)
{
    g_light_effect.transition_speed += change_time;

    if (g_light_effect.transition_speed >= 10000)
        g_light_effect.transition_speed = 10000;

    if(g_TimerLightEffect != NULL)
    {
        light_ctrl_set_fade_period_ms(g_light_effect.transition_speed - 5);
        xTimerChangePeriod( g_TimerLightEffect, g_light_effect.transition_speed, 0);
    }

    L_CTRL_DBGMSG("g_light_effect.transition_speed =%d ms\n", g_light_effect.transition_speed);
}

void light_effect_speed_decrease(uint16_t change_time)
{
    g_light_effect.transition_speed -= change_time;

    if (g_light_effect.transition_speed <= 100)
        g_light_effect.transition_speed = 100;

    if(g_TimerLightEffect != NULL)
    {
        light_ctrl_set_fade_period_ms(g_light_effect.transition_speed - 5);
        xTimerChangePeriod( g_TimerLightEffect, g_light_effect.transition_speed, 0);
    }
    L_CTRL_DBGMSG("g_light_effect.transition_speed =%d ms\n", g_light_effect.transition_speed);
}

void light_effect_set_speed(uint16_t speed_per, uint8_t u8Apply)
{
    uint16_t speed_time;
    int16_t speed_time_max = LIGHT_COLOR_SPEED_TIME_MAX - 4000;
    int16_t speed_time_min = LIGHT_COLOR_SPEED_TIME_MIN * 6;

    // mapping speed 0%~100% --> 6s~0.6s
    // (y1,x1) = (6000, 0), (y2,x2) = (600, 100)

    if (speed_per >= 100) {
        speed_per = 100;
    } else if (speed_per <= 0) {
        speed_per = 0;
    }

    g_u16ColorSpeedPer = speed_per;

    speed_time = speed_per * -(speed_time_max - speed_time_min)/100 + speed_time_max;
    g_light_effect.transition_speed = speed_time;

    light_ctrl_set_fade_period_ms(g_light_effect.transition_speed - 5);

    if(u8Apply)
    {
        if(g_TimerLightEffect != NULL)
        {
            xTimerChangePeriod( g_TimerLightEffect, g_light_effect.transition_speed, 0);
        }
    }
}

uint16_t light_effect_get_speed(void)
{
    return g_u16ColorSpeedPer;
}

void light_effect_blink_start(uint8_t red, uint8_t green, uint8_t blue, int16_t period_ms)
{
    uint16_t hue = 0;
    uint8_t saturation = 0, value = 0;

    light_effect_t effect_config = {
        .effect_type      = EFFTCT_TYPE_STROBE,
        .color_num        = 2,
        .color_index      = 0,
        .transition_speed = period_ms,
        .fade_step_ms     = 2,
    };

    light_ctrl_rgb2hsv(red, green, blue, &hue, &saturation, &value);

    effect_config.color_arr[0].hue = hue;
    effect_config.color_arr[0].saturation = saturation;
    effect_config.color_arr[0].value = value;

    effect_config.color_arr[1].hue = 0;
    effect_config.color_arr[1].saturation = 0;
    effect_config.color_arr[1].value = 0;

    light_effect_config(&effect_config);
    light_effect_start(NULL, 0);
}

void light_effect_blink_stop()
{
    uint16_t hue = 0;
    uint8_t saturation = 0, value = 0;

    light_effect_t *status = get_light_effect_config();

    hue = status->color_arr[0].hue;
    saturation = status->color_arr[0].saturation;
    value = status->color_arr[0].value;

    light_ctrl_set_hsv(hue, saturation, value, LIGHT_FADE_OFF, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    light_effect_stop(NULL);
}

light_effect_t *get_light_effect_config(void)
{
    return &g_light_effect;
}

uint16_t light_effect_get_color_num(void)
{
    return g_light_effect.color_num;
}

hsv_t *light_effect_get_color_arr(void)
{
    return g_light_effect.color_arr;
}

uint8_t light_ctrl_update_hsv(uint16_t hue, uint8_t saturation, uint8_t value)
{
    g_light_status.hue        = hue;
    g_light_status.value      = value;
    g_light_status.saturation = saturation;
    return 0;
}

uint8_t light_ctrl_update_brightness(uint8_t brightness)
{
    g_light_status.brightness        = brightness;
    return 0;
}

uint8_t light_ctrl_update_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    uint16_t hue = 0;
    uint8_t saturation = 0, value = 0;

    light_ctrl_rgb2hsv(red, green, blue, &hue, &saturation, &value);

    g_light_status.hue = hue;
    g_light_status.saturation = saturation;
    g_light_status.value = value;

    return 0;
}

uint8_t light_ctrl_update_switch(uint8_t on)
{
    g_light_status.on = on;

    return 0;
}

