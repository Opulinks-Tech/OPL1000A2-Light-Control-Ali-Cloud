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

#if BLEWIFI_REMOTE_CTRL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "msg.h"
#include "cmsis_os.h"

#include "ada_ucmd_parser.h"
#include "ada_lightbulb.h"
#include "light_control.h"
#include "ali_linkkitsdk_decl.h"
#include "blewifi_ctrl.h"

#undef printf
#define printf(...)

#define RED      0
#define ORANGE   1
#define YELLOW   2
#define GREEN    3
#define BLUE     4
#define CYAN     5
#define PURPLE   6
#define WHITE    7

enum light_effect
{
    MODE_8COLOR_FADE         = 0,
    MODE_3COLOR_STROBE       = 1,
    MODE_7COLOR_STROBE       = 2,
};


TimerHandle_t xTimerLightEffect;
uint8_t g_counter = 0;
uint32_t g_transitions_time = 1000;
uint32_t g_effect_type = MODE_8COLOR_FADE;

uint8_t g_bleadv0x20_cnt = 0;
uint8_t g_bleadv_lock = 0;
extern uint8_t g_led_mp_mode_flag;
//extern uint8_t g_NetLedStop_Sync_bit;


static void light_ctrl_8color_fade(uint8_t param)
{
    switch (g_counter)
    {
        case RED:
            light_ctrl_set_hsv(0, 100, 100, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case ORANGE:
            light_ctrl_set_hsv(39, 100, 100, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case YELLOW:
            light_ctrl_set_hsv(60, 100, 100, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case GREEN:
            light_ctrl_set_hsv(120, 100, 100, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case BLUE:
            light_ctrl_set_hsv(210, 100, 100, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case CYAN:
            light_ctrl_set_hsv(240, 100, 100, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case PURPLE:
            light_ctrl_set_hsv(273, 100, 100, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case WHITE:
            light_ctrl_set_hsv(0, 0, 100, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        default:
            break;
    }

    g_counter++;
    if (g_counter >= 8)
        g_counter = 0;
}

static void light_ctrl_7color_strobe(uint8_t param)
{
    switch (g_counter)
    {
        case RED:
            light_ctrl_set_hsv(0, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case ORANGE:
            light_ctrl_set_hsv(39, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case YELLOW:
            light_ctrl_set_hsv(60, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case GREEN:
            light_ctrl_set_hsv(120, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case BLUE:
            light_ctrl_set_hsv(210, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case CYAN:
            light_ctrl_set_hsv(240, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case PURPLE:
            light_ctrl_set_hsv(273, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        default:
            break;
    }

    g_counter++;
    if (g_counter >= 7)
        g_counter = 0;
}

static void light_ctrl_3color_strobe(uint8_t param)
{
    switch (g_counter)
    {
        case 0: //Red
            light_ctrl_set_hsv(0, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case 1: //green
            light_ctrl_set_hsv(120, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
        case 2: //blue
            light_ctrl_set_hsv(210, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
            break;
    }

    g_counter++;
    if (g_counter >= 3)
        g_counter = 0;
}

static void light_effect_timer_cb(TimerHandle_t xTimer)
{
    switch (g_effect_type)
    {
        case MODE_8COLOR_FADE:
            light_ctrl_8color_fade(0);
            break;
        case MODE_7COLOR_STROBE:
            light_ctrl_7color_strobe(0);
            break;
        case MODE_3COLOR_STROBE:
            light_ctrl_3color_strobe(0);
            break;
        default:
            break;
    }
}

void light_effect_timer_stop()
{
    if (xTimerLightEffect)
    {
        xTimerStop(xTimerLightEffect, portMAX_DELAY);
        //restore default fade time
        light_ctrl_set_fade_period_ms(3000);
        light_ctrl_set_fade_step_ms(3);
    }
}

void light_effect_flash_start(uint8_t fade_step_ms)
{
    g_counter = 0;
    g_effect_type = MODE_8COLOR_FADE;
    light_ctrl_set_fade_period_ms(400);
    light_ctrl_set_fade_step_ms(2);
    light_ctrl_set_hsv(0, 0, 100, LIGHT_FADE_ON, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    light_ctrl_set_fade_period_ms(g_transitions_time - 5);
    light_ctrl_set_fade_step_ms(fade_step_ms);

    if (!xTimerLightEffect)
    {
        xTimerLightEffect = xTimerCreate("light_timer",
                                          g_transitions_time,
                                          pdTRUE,
                                          (void *)0,
                                          light_effect_timer_cb);
    }
    if (xTimerLightEffect != NULL)
    {
        xTimerStart(xTimerLightEffect, 0);
    }
}

void light_effect_strobe_start(int32_t type)
{
    g_counter = 1;
    g_effect_type = type;
    light_ctrl_set_fade_period_ms(400);
    light_ctrl_set_fade_step_ms(2);
    light_ctrl_set_hsv(0, 100, 100, LIGHT_FADE_OFF, DONT_UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    if (!xTimerLightEffect)
    {
        xTimerLightEffect = xTimerCreate("light_timer",
                                          g_transitions_time,
                                          pdTRUE,
                                          (void *)0,
                                          light_effect_timer_cb);
    }
    if (xTimerLightEffect != NULL)
    {
        xTimerStart(xTimerLightEffect, 0);
    }
}

void ada_cmd_bleadv_on(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    g_led_mp_mode_flag = 0;
    switch(keystate)
    {
        case C_KEY_DOWN:       //0x80
        case C_KEY_LONG:       //0x40
        case C_KEY_LT_FINISH:  //0x30
        case C_KEY_CONTINUE:   //0x20
            break;
        case C_KEY_ST_FINISH:  //0x90
            if (light_ctrl_get_switch() == SWITCH_OFF)
            {
                light_ctrl_set_switch(SWITCH_ON);
                if (light_effect_get_status())
                    light_effect_start(NULL, 0);
                SET_BIT(property_output_flag,PROPERTY_LIGHT_SWITCH);
                iot_update_light_property(property_output_flag, 0);
                printf("power on\r\n");
            }
            else
            {
                #if 0
                if (true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK))
                {
                    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_BUTTON_BLE_ADV_TIMEOUT, NULL, 0);
                    while(1)
                    {
                        if (g_NetLedStop_Sync_bit == 0)
                            break;
                        taskYIELD();
                    }
                    g_NetLedStop_Sync_bit = 1;
                }
                #endif
                
                printf("power off\r\n");
                light_ctrl_set_switch(SWITCH_OFF);
                SET_BIT(property_output_flag,PROPERTY_LIGHT_SWITCH);
                iot_update_light_property(property_output_flag, 0);
            }
            break;
        default:
                printf("wrong key state value\n");
            break;
		}
}

void ada_cmd_power_on(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (keystate == C_KEY_DOWN)
    {
        light_ctrl_set_switch(SWITCH_ON);
        if (light_effect_get_status())
            light_effect_start(NULL, 0);
        SET_BIT(property_output_flag,PROPERTY_LIGHT_SWITCH);
        iot_update_light_property(property_output_flag, 0);
        printf("power on\r\n");
    }
}

void ada_cmd_power_off(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (keystate == C_KEY_DOWN)
    {
        #if 0
        if (true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK))
        {
            BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_BUTTON_BLE_ADV_TIMEOUT, NULL, 0);
            while(1)
            {
                 if (g_NetLedStop_Sync_bit == 0)
                    break;
                 taskYIELD();
            }
            g_NetLedStop_Sync_bit = 1;
        }
        #endif
        
        printf("power off\r\n");
        light_ctrl_set_switch(SWITCH_OFF);
        SET_BIT(property_output_flag,PROPERTY_LIGHT_SWITCH);
        iot_update_light_property(property_output_flag, 0);
    }
}

void ada_cmd_brightness_add(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
        return;
    int brightness_value = 0;
    if ((keystate == (C_KEY_DOWN | C_KEY_UP)) || (keystate == (C_KEY_LONG | C_KEY_UP)))
    {
        if (light_ctrl_get_mode() == MODE_HSV)
        {
            brightness_value = light_ctrl_get_value();
            if ((brightness_value + 10) > 100)
            {
                light_ctrl_set_value(100);
            }
            else
            {
                light_ctrl_set_value(brightness_value + 10);
            }
            SET_BIT(property_output_flag,PROPERTY_HSVCOLOR);
            iot_update_light_property(property_output_flag, 0);

            printf("HSV_v +10%\r\n");
        }
        else if (light_ctrl_get_mode() == MODE_CTB && (light_ctrl_get_light_type() != LT_RGB))
        {
            brightness_value = light_ctrl_get_brightness();
            if ((brightness_value + 10) > 100)
            {
                light_ctrl_set_brightness(100);
            }
            else
            {
                light_ctrl_set_brightness(brightness_value + 10);
            }
            SET_BIT(property_output_flag,PROPERTY_BRIGHTNESS);
            iot_update_light_property(property_output_flag, 0);

            printf("brightness +10%\r\n");
        }
    }
    else if (keystate == C_KEY_CONTINUE)
    {
        if (light_ctrl_get_mode() == MODE_HSV)
        {
            brightness_value = light_ctrl_get_value();
            if ((brightness_value + 1) > 100)
            {
                light_ctrl_set_value(100);
            }
            else
            {
                light_ctrl_set_value(brightness_value + 1);
            }
            SET_BIT(property_output_flag,PROPERTY_HSVCOLOR);
            iot_update_light_property(property_output_flag, 0);

            printf("HSV_v +1%\r\n");
        }
        else if (light_ctrl_get_mode() == MODE_CTB && (light_ctrl_get_light_type() != LT_RGB))
        {
            brightness_value = light_ctrl_get_brightness();
            if ((brightness_value + 1) > 100)
            {
                light_ctrl_set_brightness(100);
            }
            else
            {
                light_ctrl_set_brightness(brightness_value + 1);
            }
            SET_BIT(property_output_flag,PROPERTY_BRIGHTNESS);
            iot_update_light_property(property_output_flag, 0);

            printf("brightness +1%\r\n");
        }
    }
}

void ada_cmd_brightness_sub(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    int brightness_value = 0;
    int colortemperature_value = 0;
    if ((keystate == (C_KEY_DOWN | C_KEY_UP)) || (keystate == (C_KEY_LONG | C_KEY_UP)))
    {
        if (light_ctrl_get_mode() == MODE_HSV)
        {
            brightness_value = light_ctrl_get_value();
            if ((brightness_value - 10) <= 1)
            {
                light_ctrl_set_value(1);
            }
            else
            {
                light_ctrl_set_value(brightness_value - 10);
            }
            SET_BIT(property_output_flag,PROPERTY_HSVCOLOR);
            iot_update_light_property(property_output_flag, 0);

            printf("HSV_v -10%\r\n");
        }
        else if (light_ctrl_get_mode() == MODE_CTB && (light_ctrl_get_light_type() != LT_RGB))
        {
            brightness_value = light_ctrl_get_brightness();
            colortemperature_value = light_ctrl_get_color_temperature();
            if ((brightness_value - 10) <= 1)
            {
                if(colortemperature_value > 4500)
                {
                    light_ctrl_set_ctb(7000, 1, LIGHT_FADE_ON, UPDATE_LED_STATUS);
                }
                else
                {
                    light_ctrl_set_ctb(2000, 1, LIGHT_FADE_ON, UPDATE_LED_STATUS);
                }
                colortemperature_force_overwrite(colortemperature_value);
            }
            else
            {
                light_ctrl_set_brightness(brightness_value - 10);
            }
            SET_BIT(property_output_flag,PROPERTY_BRIGHTNESS);
            iot_update_light_property(property_output_flag, 0);

            printf("brightness -10%\r\n");
        }
    }
    else if (keystate == C_KEY_CONTINUE)
    {
        if (light_ctrl_get_mode() == MODE_HSV)
        {
            brightness_value = light_ctrl_get_value();
            if ((brightness_value - 1) <= 1)
            {
                light_ctrl_set_value(1);
            }
            else
            {
                light_ctrl_set_value(brightness_value - 1);
            }
            SET_BIT(property_output_flag,PROPERTY_HSVCOLOR);
            iot_update_light_property(property_output_flag, 0);

            printf("HSV_v -1%\r\n");
        }
        else if (light_ctrl_get_mode() == MODE_CTB && (light_ctrl_get_light_type() != LT_RGB))
        {
            brightness_value = light_ctrl_get_brightness();
            colortemperature_value = light_ctrl_get_color_temperature();
            if ((brightness_value - 1) <= 1)
            {
                if(colortemperature_value > 4500)
                {
                    light_ctrl_set_ctb(7000, 1, LIGHT_FADE_ON, UPDATE_LED_STATUS);
                }
                else
                {
                    light_ctrl_set_ctb(2000, 1, LIGHT_FADE_ON, UPDATE_LED_STATUS);
                }
                colortemperature_force_overwrite(colortemperature_value);
            }
            else
            {
                light_ctrl_set_brightness(brightness_value - 1);
            }
            SET_BIT(property_output_flag,PROPERTY_BRIGHTNESS);
            iot_update_light_property(property_output_flag, 0);

            printf("brightness -1%\r\n");
        }
    }
}

void ada_cmd_color_saturation_add(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    int colortemp_saturation = 0;
    if ((keystate == (C_KEY_DOWN | C_KEY_UP)) || (keystate == (C_KEY_LONG | C_KEY_UP)))
    {
        if (light_ctrl_get_mode() == MODE_HSV)
        {
            colortemp_saturation = light_ctrl_get_saturation();
            if ((colortemp_saturation + 10) > 100)
            {
                light_ctrl_set_saturation(100);
            }
            else
            {
                light_ctrl_set_saturation(colortemp_saturation + 10);
            }
            SET_BIT(property_output_flag,PROPERTY_HSVCOLOR);
            iot_update_light_property(property_output_flag, 0);

            printf("HSV_s +10%\r\n");
        }
        else if (light_ctrl_get_mode() == MODE_CTB && (light_ctrl_get_light_type() != LT_RGB))
        {
            colortemp_saturation = light_ctrl_get_color_temperature();
            if ((colortemp_saturation + 500) > 7000)
            {
                light_ctrl_set_color_temperature(7000);
            }
            else
            {
                light_ctrl_set_color_temperature(colortemp_saturation + 500);
            }
            SET_BIT(property_output_flag,PROPERTY_COLORTEMPERATURE);
            iot_update_light_property(property_output_flag, 0);

            printf("CT/saturation +10%\r\n");
        }
    }
    else if (keystate == C_KEY_CONTINUE)
    {
        if (light_ctrl_get_mode() == MODE_HSV)
        {
            colortemp_saturation = light_ctrl_get_saturation();
            if ((colortemp_saturation + 1) > 100)
            {
                light_ctrl_set_saturation(100);
            }
            else
            {
                light_ctrl_set_saturation(colortemp_saturation + 1);
            }
            SET_BIT(property_output_flag,PROPERTY_HSVCOLOR);
            iot_update_light_property(property_output_flag, 0);

            printf("HSV_s +1%\r\n");
        }
        else if (light_ctrl_get_mode() == MODE_CTB && (light_ctrl_get_light_type() != LT_RGB))
        {
            colortemp_saturation = light_ctrl_get_color_temperature();
            if ((colortemp_saturation + 50) > 7000)
            {
                light_ctrl_set_color_temperature(7000);
            }
            else
            {
                light_ctrl_set_color_temperature(colortemp_saturation + 50);
            }
            SET_BIT(property_output_flag,PROPERTY_COLORTEMPERATURE);
            iot_update_light_property(property_output_flag, 0);

            printf("CT/saturation +1%\r\n");
        }
    }
}

void ada_cmd_color_saturation_sub(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
        return;
    int colortemp_saturation = 0;
    if ((keystate == (C_KEY_DOWN | C_KEY_UP)) || (keystate == (C_KEY_LONG | C_KEY_UP)))
    {
        if (light_ctrl_get_mode() == MODE_HSV)
        {
            colortemp_saturation = light_ctrl_get_saturation();
            if ((colortemp_saturation - 10) <= 1)
            {
                light_ctrl_set_saturation(1);
            }
            else
            {
                light_ctrl_set_saturation(colortemp_saturation - 10);
            }
            SET_BIT(property_output_flag,PROPERTY_HSVCOLOR);
            iot_update_light_property(property_output_flag, 0);

            printf("HSV_s -10%\r\n");
        }
        else if (light_ctrl_get_mode() == MODE_CTB && (light_ctrl_get_light_type() != LT_RGB))
        {
            colortemp_saturation = light_ctrl_get_color_temperature();
            if ((colortemp_saturation - 500) <= 2000)
            {
                light_ctrl_set_color_temperature(2000);
            }
            else
            {
                light_ctrl_set_color_temperature(colortemp_saturation - 500);
            }
            SET_BIT(property_output_flag,PROPERTY_COLORTEMPERATURE);
            iot_update_light_property(property_output_flag, 0);

            printf("CT/saturation -10%\r\n");
        }
    }
    else if (keystate == C_KEY_CONTINUE)
    {
        if (light_ctrl_get_mode() == MODE_HSV)
        {
            colortemp_saturation = light_ctrl_get_saturation();
            if ((colortemp_saturation - 1) <= 1)
            {
                light_ctrl_set_saturation(1);
            }
            else
            {
                light_ctrl_set_saturation(colortemp_saturation - 1);
            }
            SET_BIT(property_output_flag,PROPERTY_HSVCOLOR);
            iot_update_light_property(property_output_flag, 0);

            printf("HSV_s -1%\r\n");
        }
        else if (light_ctrl_get_mode() == MODE_CTB && (light_ctrl_get_light_type() != LT_RGB))
        {
            colortemp_saturation = light_ctrl_get_color_temperature();
            if ((colortemp_saturation - 50) <= 2000)
            {
                light_ctrl_set_color_temperature(2000);
            }
            else
            {
                light_ctrl_set_color_temperature(colortemp_saturation - 50);
            }
            SET_BIT(property_output_flag,PROPERTY_COLORTEMPERATURE);
            iot_update_light_property(property_output_flag, 0);

            printf("CT/saturation -1%\r\n");
        }
    }
}

/* Cold/Warm only */
void ada_cmd_neutral_light(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF || light_ctrl_get_light_type() == LT_RGB)
       return;
    light_ctrl_set_ctb(50, 85, LIGHT_FADE_ON, UPDATE_LED_STATUS);
    SET_BIT(property_output_flag,PROPERTY_BRIGHTNESS);
    SET_BIT(property_output_flag,PROPERTY_COLORTEMPERATURE);
    iot_update_light_property(property_output_flag, 0);
    printf("neutral_light\r\n");
}

void ada_cmd_night_light(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF || light_ctrl_get_light_type() == LT_RGB)
       return;
    light_ctrl_set_ctb(90, 15, LIGHT_FADE_ON, UPDATE_LED_STATUS);
    SET_BIT(property_output_flag,PROPERTY_BRIGHTNESS);
    SET_BIT(property_output_flag,PROPERTY_COLORTEMPERATURE);
    iot_update_light_property(property_output_flag, 0);
    printf("night_light\r\n");
}

void ada_cmd_switch_mode(uint8_t key_value, uint8_t keystate)
{
    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    if (keystate == C_KEY_DOWN)
    {
        uint8_t workmode = light_ctrl_get_workmode();
        if (workmode + 1 > WMODE_SOFT)
            workmode = WMODE_MANUAL;
        else
            workmode++;

        switch (workmode)
        {
            case WMODE_MANUAL:
                ada_cmd_style_manual(0, 0);
                break;
            case WMODE_READING:
                ada_cmd_style_reading(0, 0);
                break;
            case WMODE_CINEMA:
                ada_cmd_style_cinema(0, 0);
                break;
            case WMODE_MIDNIGHT:
                ada_cmd_style_midnight(0, 0);
                break;
            case WMODE_LIVING:
                ada_cmd_style_life(0, 0);
                break;
            case WMODE_SOFT:
                ada_cmd_style_softwhite(0, 0);
                break;
            default:
                break;
        }
        printf("switch light mode\r\n");
    }
}

/* Cold/Warm only */
void ada_cmd_c10_w90(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF || light_ctrl_get_light_type() == LT_RGB)
       return;
    light_ctrl_set_ctb(90, 100, LIGHT_FADE_ON, UPDATE_LED_STATUS);
    SET_BIT(property_output_flag,PROPERTY_BRIGHTNESS);
    SET_BIT(property_output_flag,PROPERTY_COLORTEMPERATURE);
    iot_update_light_property(property_output_flag, 0);
}

void ada_cmd_c30_w70(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF || light_ctrl_get_light_type() == LT_RGB)
       return;
    light_ctrl_set_ctb(80, 100, LIGHT_FADE_ON, UPDATE_LED_STATUS);
    SET_BIT(property_output_flag,PROPERTY_BRIGHTNESS);
    SET_BIT(property_output_flag,PROPERTY_COLORTEMPERATURE);
    iot_update_light_property(property_output_flag, 0);
}

void ada_cmd_c70_w30(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    light_ctrl_set_ctb(20, 100, LIGHT_FADE_ON, UPDATE_LED_STATUS);
    SET_BIT(property_output_flag,PROPERTY_BRIGHTNESS);
    SET_BIT(property_output_flag,PROPERTY_COLORTEMPERATURE);
    iot_update_light_property(property_output_flag, 0);
}

void ada_cmd_c90_w10(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF || light_ctrl_get_light_type() == LT_RGB)
       return;
    light_ctrl_set_ctb(10, 100, LIGHT_FADE_ON, UPDATE_LED_STATUS);
    SET_BIT(property_output_flag,PROPERTY_BRIGHTNESS);
    SET_BIT(property_output_flag,PROPERTY_COLORTEMPERATURE);
    iot_update_light_property(property_output_flag, 0);
}

/* RBG color control */
void ada_cmd_set_color_red(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    uint16_t H;
    uint8_t S,V;
    (void)keystate;

    if (keystate == C_KEY_DOWN)
    {
        switch (key_value)
        {
            case VK_CODE_RED:
                H = 0; S = 100; V = 100;
                break;
            case VK_CODE_RED1:
                H = 15; S = 100; V = 100;
                break;
            case VK_CODE_RED2:
                H = 30; S = 100; V = 100;
                break;
            case VK_CODE_RED3:
                H = 45; S = 100; V = 100;
                break;
            case VK_CODE_RED4:
                H = 60; S = 100; V = 100;
            default:
                break;
        }
        light_ctrl_set_hsv(H, S, V, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
        SET_BIT(property_output_flag,PROPERTY_HSVCOLOR);
        iot_update_light_property(property_output_flag, 0);
    }
}

void ada_cmd_set_color_green(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    uint16_t H;
    uint8_t S,V;
    (void)keystate;

    if (keystate == C_KEY_DOWN)
    {
        switch (key_value)
        {
            case VK_CODE_GREEN1:
                H = 120; S = 100; V = 100;
                break;
            case VK_CODE_GREEN2:
                H = 135; S = 100; V = 100;
                break;
            case VK_CODE_GREEN3:
                H = 150; S = 100; V = 100;
                break;
            case VK_CODE_GREEN4:
                H = 165; S = 100; V = 100;
                break;
            case VK_CODE_GREEN5:
                H = 180; S = 100; V = 100;
            default:
                break;
        }
        light_ctrl_set_hsv(H, S, V, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
        SET_BIT(property_output_flag,PROPERTY_HSVCOLOR);
        iot_update_light_property(property_output_flag, 0);
    }
}

void ada_cmd_set_color_blue(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    uint16_t H;
    uint8_t S,V;
    (void)keystate;

    if (keystate == C_KEY_DOWN)
    {
        switch (key_value)
        {
            case VK_CODE_BLUE1:
                H = 240; S = 100; V = 100;
                break;
            case VK_CODE_BLUE2:
                H = 255; S = 100; V = 100;
                break;
            case VK_CODE_BLUE3:
                H = 270; S = 100; V = 100;
                break;
            case VK_CODE_BLUE4:
                H = 285; S = 100; V = 100;
                break;
            case VK_CODE_BLUE5:
                H = 300; S = 100; V = 100;
            default:
                break;
        }
        light_ctrl_set_hsv(H, S, V, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
        SET_BIT(property_output_flag,PROPERTY_HSVCOLOR);
        iot_update_light_property(property_output_flag, 0);
    }
}

#if 1
void ada_cmd_set_color_rgb(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    uint16_t H;
    uint8_t S,V;
    (void)keystate;

    if (keystate == C_KEY_DOWN)
    {
        switch (key_value)
        {
            case VK_CODE_RGB1:
                if(light_ctrl_get_light_type() == LT_RGB)
                {
                    // no cold-light
                    H = 0; S = 0; V = 100;
                    break;
                }
                else
                {
                    light_ctrl_set_ctb(7000, 100, LIGHT_FADE_ON, UPDATE_LED_STATUS);
                    SET_BIT(property_output_flag, PROPERTY_BRIGHTNESS);
                    goto done;
                }
                
            case VK_CODE_RGB2:
                H = 314; S = 23; V = 83;
                break;

            case VK_CODE_RGB3:
                H = 321; S = 28; V = 78;
                break;

            case VK_CODE_RGB4:
                H = 200; S = 42; V = 80;
                break;

            case VK_CODE_RGB5:
                H = 200; S = 48; V = 75;
                break;

            default:
                goto done;
        }

        light_ctrl_set_hsv(H, S, V, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
        SET_BIT(property_output_flag, PROPERTY_HSVCOLOR);
    }

done:
    if(property_output_flag)
    {
        iot_update_light_property(property_output_flag, 0);
    }

    return;
}
#else
void ada_cmd_set_color_rgb(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    uint16_t H;
    uint8_t S,V;
    (void)keystate;

    if (keystate == C_KEY_DOWN)
    {
        switch (key_value)
        {
            case VK_CODE_RGB1:
                H = 0; S = 0; V = 100;
                break;
            case VK_CODE_RGB2:
                H = 314; S = 23; V = 83;
                break;
            case VK_CODE_RGB3:
                H = 321; S = 28; V = 78;
                break;
            case VK_CODE_RGB4:
                H = 200; S = 42; V = 80;
                break;
            case VK_CODE_RGB5:
                H = 200; S = 48; V = 75;
            default:
                break;
        }
        light_ctrl_set_hsv(H, S, V, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
        SET_BIT(property_output_flag,PROPERTY_HSVCOLOR);
        iot_update_light_property(property_output_flag, 0);
    }
}
#endif

void ada_cmd_rgb_8color_fade_slow(uint8_t key_value, uint8_t keystate)
{
    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
#if 1
    light_effect_flash_start(10);
#else
    light_effect_t effect_config = {
        .effect_type      = EFFTCT_TYPE_FLASH,
        .color_num        = 8,
        .color_index      = 0,
        .transition_speed = 5000,
        .fade_step_ms     = 10,
        .color_arr[0].hue =   0, .color_arr[0].saturation = 100, .color_arr[0].value = 100, //red
        .color_arr[1].hue =  39, .color_arr[1].saturation = 100, .color_arr[1].value = 100, //orange
        .color_arr[2].hue =  60, .color_arr[2].saturation = 100, .color_arr[2].value = 100, //yellow
        .color_arr[3].hue = 120, .color_arr[3].saturation = 100, .color_arr[3].value = 100, //green
        .color_arr[4].hue = 210, .color_arr[4].saturation = 100, .color_arr[4].value = 100, //blue
        .color_arr[5].hue = 240, .color_arr[5].saturation = 100, .color_arr[5].value = 100, //cyan
        .color_arr[6].hue = 273, .color_arr[6].saturation = 100, .color_arr[6].value = 100, //purple
        .color_arr[7].hue =   0, .color_arr[7].saturation =   0, .color_arr[7].value = 100, //white
    };

    light_effect_config(&effect_config);
    light_effect_start(NULL, 0);
#endif
    printf("RGB fading slow\r\n");
}
void ada_cmd_rgb_8color_fade_fast(uint8_t key_value, uint8_t keystate)
{
    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    light_effect_flash_start(3);
    printf("RGB fading fast\r\n");
}

void ada_cmd_rgb_3color_loop(uint8_t key_value, uint8_t keystate)
{
    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
#if 1
    light_effect_strobe_start(MODE_3COLOR_STROBE);
#else

    light_effect_t effect_config = {
        .effect_type      = EFFTCT_TYPE_STROBE,
        .color_num        = 3,
        .color_index      = 0,
        .transition_speed = 5000,
        .fade_step_ms     = 0,
        .color_arr[0].hue =   0, .color_arr[0].saturation = 100, .color_arr[0].value = 100, //red
        .color_arr[1].hue = 120, .color_arr[1].saturation = 100, .color_arr[1].value = 100, //green
        .color_arr[2].hue = 210, .color_arr[2].saturation = 100, .color_arr[2].value = 100, //blue
    };

    light_effect_config(&effect_config);
    light_effect_start(NULL, 0);
#endif
    printf("rgb_3color_loop\r\n");
}

void ada_cmd_rgb_7color_loop(uint8_t key_value, uint8_t keystate)
{
    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    light_effect_strobe_start(MODE_7COLOR_STROBE);
    printf("rgb_7color_loop\r\n");
}

void ada_cmd_speed_add(uint8_t key_value, uint8_t keystate)
{
    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;

#if 1
    if (g_transitions_time + 100 >= 10000)
        g_transitions_time = 10000;
    else
        g_transitions_time = g_transitions_time + 100;

    if(xTimerLightEffect != NULL)
    {
        light_ctrl_set_fade_period_ms(g_transitions_time - 5);
        xTimerChangePeriod( xTimerLightEffect, g_transitions_time, 0);
    }
#else
    light_effect_speed_increase(100);
#endif
    printf("Adjust color chang speed +100ms\n");

}

void ada_cmd_speed_sub(uint8_t key_value, uint8_t keystate)
{
    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
#if 1
    if (g_transitions_time - 100 <= 100)
        g_transitions_time = 100;
    else
        g_transitions_time = g_transitions_time - 100;

    if(xTimerLightEffect != NULL)
    {
        light_ctrl_set_fade_period_ms(g_transitions_time);
        xTimerChangePeriod( xTimerLightEffect, g_transitions_time, 0);
    }
#else
    light_effect_speed_decrease(100);

#endif
    printf("Adjust color chang speed -100ms\n");
}

void ada_cmd_light_mode_add(uint8_t key_value, uint8_t keystate)
{
    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    uint8_t workmode = light_ctrl_get_workmode();
    if (workmode + 1 > WMODE_SOFT)
        workmode = WMODE_MANUAL;
    else
        workmode++;

    switch (workmode)
    {
        case WMODE_MANUAL:
            ada_cmd_style_manual(0, 0);
            break;
        case WMODE_READING:
            ada_cmd_style_reading(0, 0);
            break;
        case WMODE_CINEMA:
            ada_cmd_style_cinema(0, 0);
            break;
        case WMODE_MIDNIGHT:
            ada_cmd_style_midnight(0, 0);
            break;
        case WMODE_LIVING:
            ada_cmd_style_life(0, 0);
            break;
        case WMODE_SOFT:
            ada_cmd_style_softwhite(0, 0);
            break;
        default:
            break;
    }
    printf("light mode change direct -->\n");
}

void ada_cmd_light_mode_sub(uint8_t key_value, uint8_t keystate)
{
    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    int workmode = light_ctrl_get_workmode();
    if (workmode - 1 < WMODE_MANUAL)
        workmode = WMODE_SOFT;
    else
        workmode--;

    switch (workmode)
    {
        case WMODE_MANUAL:
            ada_cmd_style_manual(0, 0);
            break;
        case WMODE_READING:
            ada_cmd_style_reading(0, 0);
            break;
        case WMODE_CINEMA:
            ada_cmd_style_cinema(0, 0);
            break;
        case WMODE_MIDNIGHT:
            ada_cmd_style_midnight(0, 0);
            break;
        case WMODE_LIVING:
            ada_cmd_style_life(0, 0);
            break;
        case WMODE_SOFT:
            ada_cmd_style_softwhite(0, 0);
            break;
        default:
            break;
    }
    printf("light mode change direct <--\n");
}

void ada_cmd_style_manual(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    uint16_t hue = 0;
    uint8_t saturation = 0;
    uint8_t value = 0;
    light_ctrl_set_workmode(WMODE_MANUAL);
    light_ctrl_get_manual_light_status(&hue, &saturation, &value);
    light_ctrl_set_hsv(hue, saturation, value, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    SET_BIT(property_output_flag,PROPERTY_WORKMODE);
    iot_update_light_property(property_output_flag, 0);
    printf("style_manual\n");
}

void ada_cmd_style_reading(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    light_ctrl_set_workmode(WMODE_READING);
    light_ctrl_set_hsv(22, 3, 100, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    SET_BIT(property_output_flag,PROPERTY_WORKMODE);
    iot_update_light_property(property_output_flag, 0);
    printf("style_reading\n");
}

void ada_cmd_style_cinema(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    light_ctrl_set_workmode(WMODE_CINEMA);
    light_ctrl_set_hsv(16, 100, 81, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    SET_BIT(property_output_flag,PROPERTY_WORKMODE);
    iot_update_light_property(property_output_flag, 0);
    printf("style_cinema\n");
}

void ada_cmd_style_midnight(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    light_ctrl_set_workmode(WMODE_MIDNIGHT);
    light_ctrl_set_hsv(37, 100, 74, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    SET_BIT(property_output_flag,PROPERTY_WORKMODE);
    iot_update_light_property(property_output_flag, 0);
    printf("style_midnight\n");
}

void ada_cmd_style_life(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    light_ctrl_set_workmode(WMODE_LIVING);
    light_ctrl_set_hsv(225, 71, 71, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    SET_BIT(property_output_flag,PROPERTY_WORKMODE);
    iot_update_light_property(property_output_flag, 0);
    printf("style_life\n");
}

void ada_cmd_style_softwhite(uint8_t key_value, uint8_t keystate)
{
    uint16_t property_output_flag = 0;

    if (light_ctrl_get_switch() == SWITCH_OFF)
       return;
    light_ctrl_set_workmode(WMODE_SOFT);
    light_ctrl_set_hsv(2, 39, 100, LIGHT_FADE_ON, UPDATE_LED_STATUS, NO_RGB_LIGHTEN_REQ);
    SET_BIT(property_output_flag,PROPERTY_WORKMODE);
    iot_update_light_property(property_output_flag, 0);
    printf("style_softwhite\n");
}

#endif //#if BLEWIFI_REMOTE_CTRL

