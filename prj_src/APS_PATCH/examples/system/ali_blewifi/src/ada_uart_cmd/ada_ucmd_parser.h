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

#ifndef __ADA_UCMD_PARSER_H__
#define __ADA_UCMD_PARSER_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DO_CHKSUM       1
#define DUMP_CMD_PACKET 0

/**
 * @brief define key states
 *
 */
#define C_KEY_UP                0x10
#define C_KEY_CONTINUE          0x20
#define C_KEY_LT_FINISH         0x30
#define C_KEY_LONG              0x40
#define C_KEY_DOWN              0x80
#define C_KEY_ST_FINISH         0x90
#define C_KEY_NONE              0x00

/**
 * @brief define virtual key
 *
 */
#define VK_CODE_BLEADV_ON       0

#define VK_CODE_POWER_ON        1
#define VK_CODE_POWER_OFF       2
#define VK_CODE_BRIGHTNESS_ADD  3
#define VK_CODE_BRIGHTNESS_SUB  4
#define VK_CODE_COLOR_ADD       5
#define VK_CODE_COLOR_SUB       6

/* Cold/Warm only */
#define VK_CODE_NEUTRAL_LIGHT   7
#define VK_CODE_NIGHT_LIGHT     8

#define VK_CODE_SWITCH_MODE     9

/* Cold/Warm only */
#define VK_CODE_C10_W90         10
#define VK_CODE_C30_W70         11
#define VK_CODE_C70_W30         12
#define VK_CODE_C90_W10         13

/* RBG color control */
#define VK_CODE_RED             21
#define VK_CODE_RED1            22
#define VK_CODE_RED2            23
#define VK_CODE_RED3            24
#define VK_CODE_RED4            25

#define VK_CODE_GREEN1          26
#define VK_CODE_GREEN2          27
#define VK_CODE_GREEN3          28
#define VK_CODE_GREEN4          29
#define VK_CODE_GREEN5          30

#define VK_CODE_BLUE1           31
#define VK_CODE_BLUE2           32
#define VK_CODE_BLUE3           33
#define VK_CODE_BLUE4           34
#define VK_CODE_BLUE5           35

#define VK_CODE_RGB1            36
#define VK_CODE_RGB2            37
#define VK_CODE_RGB3            38
#define VK_CODE_RGB4            39
#define VK_CODE_RGB5            40

/* RBG color control */
#define VK_CODE_RGB_8COLOR_FADE_SLOW   41
#define VK_CODE_RGB_8COLOR_FADE_FAST   42
#define VK_CODE_RGB_3COLOR_LOOP 43
#define VK_CODE_RGB_7COLOR_LOOP 44

/* other function */
#define VK_CODE_SPEED_ADD       65
#define VK_CODE_SPEED_SUB       66
#define VK_CODE_LIGHT_MODE_ADD  67
#define VK_CODE_LIGHT_MODE_SUB  68

#define VK_CODE_STYLE_MANUAL    70
#define VK_CODE_STYLE_READING   71
#define VK_CODE_STYLE_CINEMA    72
#define VK_CODE_STYLE_MIDNIGHT  73
#define VK_CODE_STYLE_LIFE      74
#define VK_CODE_STYLE_SOFTWHIT  75


/**
 * @brief uart command packet header
 */
typedef struct ucmd_hdr_tag
{
    uint8_t  sync1;
    uint8_t  sync2;
    uint16_t len;       //big-endian
    uint8_t  sn;
    uint8_t  data[];
}__attribute__((packed)) ucmd_hdr_t;


/**
 * @brief uart command packet data
 */
typedef struct ucmd_opcode_tag
{
    uint16_t opcode;
    uint8_t  addr[4];
    uint8_t  key_state;
    uint8_t  key_value;
    uint8_t  chk_sum;
}__attribute__((packed)) ucmd_opcode_t;


#define ucmd_printf(fmt,...)          printf(fmt,##__VA_ARGS__)
#define ucmd_strlen(s)                strlen((char*)(s))
#define ucmd_strcpy(dest,src)         strcpy((char*)(dest),(char*)(src))
#define ucmd_strncpy(dest,src,n)      strncpy((char*)(dest),(char*)(src),n)
#define ucmd_strcmp(s1,s2)            strcmp((char*)(s1),(char*)(s2))
#define ucmd_strncmp(s1,s2,n)         strncmp((char*)(s1),(char*)(s2),n)
#define ucmd_strstr(s1,s2)            strstr((char*)(s1),(char*)(s2))
#define ucmd_sprintf(s,...)           sprintf((char*)(s), ##__VA_ARGS__)
#define ucmd_snprintf(s,size,...)     snprintf((char*)(s),size, ##__VA_ARGS__)
#define ucmd_memset(dest,x,n)         memset(dest,x,n)
#define ucmd_memcpy(dest,src,n)       memcpy(dest,src,n)
#define ucmd_memcmp(s1,s2,n)          memcmp(s1,s2,n)
#define ucmd_malloc(size)             malloc(size)
#define ucmd_free(ptr)                free(ptr)


int uart_cmd_process(void *pbuf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __ADA_UCMD_PARSER_H__ */
