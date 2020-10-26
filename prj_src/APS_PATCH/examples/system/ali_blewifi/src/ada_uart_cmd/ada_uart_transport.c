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

#if BLEWIFI_REMOTE_CTRL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cmsis_os.h"
#include "uart_cmd_task.h"


#define ADA_UART_TRANSPORT_INDICATER_LEN                2
#define ADA_COMMAND_HEADER_LEN                          5
#define ADA_COMMON_COMMAND_PKT_KEN                      14

#define STATE_SYNCBYTE1     0x1
#define STATE_SYNCBYTE2     0x2
#define STATE_HEADER        0x3
#define STATE_PAYLOAD       0x4
#define STATE_NONE          0x5

int state = STATE_SYNCBYTE1;
uart_buffer_t uart0_rx_buf;

void ada_ctrl_uart_input_impl(uint32_t data)
{
    uart_buffer_t *buf = &uart0_rx_buf;
    uint8_t rdata = data & 0xFF;

    switch (state)
    {
        case STATE_SYNCBYTE1:
            if (rdata == 0xEE)
            {
                buf->data[buf->cursor++] = rdata;
                state = STATE_SYNCBYTE2;
            }
            break;

        case STATE_SYNCBYTE2:
            if (rdata == 0xEE)
            {
                buf->data[buf->cursor++] = rdata;
                state = STATE_HEADER;
            }
            else
            {
                state = STATE_SYNCBYTE1;
                buf->cursor = 0;
                buf->length = 0;
            }
            break;

        case STATE_HEADER:
            buf->data[buf->cursor++] = rdata;
            if (buf->cursor == (ADA_COMMAND_HEADER_LEN-1))
            {
                buf->length = (buf->data[2]<<8) | buf->data[3];
                if (buf->length == ADA_COMMON_COMMAND_PKT_KEN)
                {
                    state = STATE_PAYLOAD;
                }
                else
                {
                    /* invalid packet length */
                    state = STATE_SYNCBYTE1;
                    buf->cursor = 0;
                    buf->length = 0;
                }
            }
            break;
        case STATE_PAYLOAD:
            buf->data[buf->cursor++] = rdata;
            break;

        default:
            break;
    }

    if (buf->cursor >= UART_RBUF_SIZE || (buf->length == 0 && buf->cursor >= ADA_COMMAND_HEADER_LEN))
    {
        state = STATE_SYNCBYTE1;
        buf->cursor = 0;
        buf->length = 0;
    }

    if (buf->length != 0 && buf->cursor == buf->length)
    {
        uart_event_msg_t msg = {0};
        msg.event = UART_COMMAND_EVENT;
        msg.length = buf->length;
        msg.param = (uint8_t*)buf->data;

		// push serail protocol pkt to command parser
        uart_cmd_task_send(&msg);

        state = STATE_SYNCBYTE1;
        buf->cursor = 0;
        buf->length = 0;
    }
}

#endif //#if BLEWIFI_REMOTE_CTRL

