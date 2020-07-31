/******************************************************************************
*  Copyright 2017 - 2018, Opulinks Technology Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2018
******************************************************************************/

#ifdef ADA_REMOTE_CTRL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "ada_ucmd_parser.h"
#include "ada_lightbulb.h"
#include "light_control.h"
#include "blewifi_ctrl.h"

#define bswap16(x) ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))


typedef void (*key_cmd_handler_fp_t)(uint8_t key_value, uint8_t keystate);

typedef struct
{
    uint8_t key_cmd;
    key_cmd_handler_fp_t cmd_handler;
} key_cmd_handler_table_t;


/**
  * @brief ADA Uart Command Table for LED control
  *
  */
key_cmd_handler_table_t g_key_cmd_handler_tbl[] =
{
    {VK_CODE_BLEADV_ON,                 ada_cmd_bleadv_on            },

    {VK_CODE_POWER_ON,                  ada_cmd_power_on             },
    {VK_CODE_POWER_OFF,                 ada_cmd_power_off            },
    {VK_CODE_BRIGHTNESS_ADD,            ada_cmd_brightness_add       },
    {VK_CODE_BRIGHTNESS_SUB,            ada_cmd_brightness_sub       },
    {VK_CODE_COLOR_ADD,                 ada_cmd_color_saturation_add },
    {VK_CODE_COLOR_SUB,                 ada_cmd_color_saturation_sub },

    {VK_CODE_NEUTRAL_LIGHT,             ada_cmd_neutral_light        },
    {VK_CODE_NIGHT_LIGHT,               ada_cmd_night_light          },

    {VK_CODE_SWITCH_MODE,               ada_cmd_switch_mode          },

    {VK_CODE_C10_W90,                   ada_cmd_c10_w90              },
    {VK_CODE_C30_W70,                   ada_cmd_c30_w70              },
    {VK_CODE_C70_W30,                   ada_cmd_c70_w30              },
    {VK_CODE_C90_W10,                   ada_cmd_c90_w10              },

    {VK_CODE_RED,                       ada_cmd_set_color_red        },
    {VK_CODE_RED1,                      ada_cmd_set_color_red        },
    {VK_CODE_RED2,                      ada_cmd_set_color_red        },
    {VK_CODE_RED3,                      ada_cmd_set_color_red        },
    {VK_CODE_RED4,                      ada_cmd_set_color_red        },

    {VK_CODE_GREEN1,                    ada_cmd_set_color_green      }, // 19
    {VK_CODE_GREEN2,                    ada_cmd_set_color_green      },
    {VK_CODE_GREEN3,                    ada_cmd_set_color_green      },
    {VK_CODE_GREEN4,                    ada_cmd_set_color_green      },
    {VK_CODE_GREEN5,                    ada_cmd_set_color_green      },

    {VK_CODE_BLUE1,                     ada_cmd_set_color_blue       },
    {VK_CODE_BLUE2,                     ada_cmd_set_color_blue       },
    {VK_CODE_BLUE3,                     ada_cmd_set_color_blue       },
    {VK_CODE_BLUE4,                     ada_cmd_set_color_blue       },
    {VK_CODE_BLUE5,                     ada_cmd_set_color_blue       },

    {VK_CODE_RGB1,                      ada_cmd_set_color_rgb        },
    {VK_CODE_RGB2,                      ada_cmd_set_color_rgb        },
    {VK_CODE_RGB3,                      ada_cmd_set_color_rgb        },
    {VK_CODE_RGB4,                      ada_cmd_set_color_rgb        },
    {VK_CODE_RGB5,                      ada_cmd_set_color_rgb        },

    {VK_CODE_RGB_8COLOR_FADE_SLOW,      ada_cmd_rgb_8color_fade_slow },
    {VK_CODE_RGB_8COLOR_FADE_FAST,      ada_cmd_rgb_8color_fade_fast },
    {VK_CODE_RGB_3COLOR_LOOP,           ada_cmd_rgb_3color_loop      },
    {VK_CODE_RGB_7COLOR_LOOP,           ada_cmd_rgb_7color_loop      },

    {VK_CODE_SPEED_ADD,                 ada_cmd_speed_add            },
    {VK_CODE_SPEED_SUB,                 ada_cmd_speed_sub            },
    {VK_CODE_LIGHT_MODE_ADD,            ada_cmd_light_mode_add       },
    {VK_CODE_LIGHT_MODE_SUB,            ada_cmd_light_mode_sub       },

    {VK_CODE_STYLE_MANUAL,              ada_cmd_style_manual         },
    {VK_CODE_STYLE_READING,             ada_cmd_style_reading        },
    {VK_CODE_STYLE_CINEMA,              ada_cmd_style_cinema         },
    {VK_CODE_STYLE_MIDNIGHT,            ada_cmd_style_midnight       },
    {VK_CODE_STYLE_LIFE,                ada_cmd_style_life           },
    {VK_CODE_STYLE_SOFTWHIT,            ada_cmd_style_softwhite      },
    {0xFF,                                                  NULL     }
};

void dump_cmd_packet(char *pbuf, uint16_t len)
{
    int i;

    ucmd_printf("packet buf addr:%#08x, packet size:%d\r\n",(uint32_t)pbuf, len);
    for (i=0; i<len; i++)
    {
        ucmd_printf("0x%02x ", pbuf[i]);
    }
    ucmd_printf("\r\n");
}

void key_cmd_handler(uint8_t key_value , uint8_t key_state)
{
    int i =0;
    while (g_key_cmd_handler_tbl[i].key_cmd != 0xFF)
    {
        // match
        if (g_key_cmd_handler_tbl[i].key_cmd == key_value)
        {
            /* stop effect operation triggered by key controller*/
            light_effect_timer_stop();

            /* stop effect operation triggered by APP*/
            light_effect_stop(NULL);

            if (g_key_cmd_handler_tbl[i].cmd_handler)
                g_key_cmd_handler_tbl[i].cmd_handler(key_value, key_state);

            break;
        }
        i++;
    }

    // not match
    if (g_key_cmd_handler_tbl[i].key_cmd == 0xFF)
    {
    }
}

uint8_t cal_checksum(uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint8_t sum = 0;

    for (i = 0; i < len; i++)
    {
        sum += data[i];
    }

    sum = !sum + 1;

    return sum;
}

int uart_cmd_parser(void *data, uint16_t len)
{
    ucmd_opcode_t *cmd = NULL;
    uint8_t key_state = C_KEY_NONE;
    uint8_t key_value = 0;

    cmd = (ucmd_opcode_t*)((uint8_t*)data + sizeof(ucmd_hdr_t));

#if DUMP_CMD_PACKET
    {
        ucmd_hdr_t *hdr = (ucmd_hdr_t*)data;
    
        //dump_cmd_packet(data, len);
        ucmd_printf("\r\n");
        ucmd_printf("sync byte 1: 0x%02x\r\n", hdr->sync1);
        ucmd_printf("sync byte 2: 0x%02x\r\n", hdr->sync2);
        ucmd_printf("packet len : %d\r\n", bswap16(hdr->len));
        ucmd_printf("packet num : %d\r\n", hdr->sn);
        ucmd_printf("key state: 0x%02x\r\n", cmd->key_state);
        ucmd_printf("key value: %d\r\n\r\n", cmd->key_value);
    }
#endif

    key_state = cmd->key_state;
    key_value = cmd->key_value;

    key_cmd_handler(key_value, key_state);

    return 0;
}

int uart_cmd_process(void *pbuf, uint16_t len)
{
#if DO_CHKSUM
    uint8_t sum;
    uint8_t *data = (uint8_t*)pbuf;

    sum = cal_checksum(pbuf, 13);

    if (sum == data[13])
        return -1;
#endif

    if (true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_NETWORK))
    {
        //ucmd_printf("newtorking is true: ignore key input\n");
    }
    else
    {
        uart_cmd_parser(pbuf, len);
    }

    return 0;
}

#endif //#ifdef ADA_REMOTE_CTRL

