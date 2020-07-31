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

#include "cmsis_os.h"
#include "hal_uart.h"
#include "ada_ucmd_parser.h"
#include "uart_cmd_task.h"

osMessageQId g_UartTaskQueueId;
osThreadId g_UartTaskHandleId;
uart_cmd_process_cb_t g_uart_process_handler;

int uart_cmd_task_send(uart_event_msg_t *msg)
{
    uart_event_msg_t *lmsg = NULL;

    if (msg == NULL){
        return -1;
    }

	if (g_UartTaskQueueId == NULL)
	{
        printf("uart_task: No queue\n");
        return -1;
	}

    lmsg = malloc(sizeof(uart_event_msg_t) + msg->length);
    if (lmsg == NULL)
    {
        printf("uart_task: malloc fail\n");
        goto error;
    }

    lmsg->event = msg->event;
    lmsg->length = msg->length;
    lmsg->param = (uint8_t*)lmsg + sizeof(uart_event_msg_t);
    if (msg->length > 0)
    {
       memcpy(lmsg->param, msg->param, msg->length);
    }

    if(osMessagePut(g_UartTaskQueueId, (uint32_t)lmsg, 0) != osOK)
    {
        goto error;
    }

    return 0;

error:
    if(lmsg)
    {
        free(lmsg);
    }

    return -1;
}

void uart_cmd_task(void *pvParameters)
{
    osEvent rxEvent;
    uart_event_msg_t *rxMsg;

    tracer_log(LOG_HIGH_LEVEL, "ada_uart_task is created successfully! \r\n");

    for(;;)
    {
        /** wait event */
        rxEvent = osMessageGet(g_UartTaskQueueId, osWaitForever);
        if(rxEvent.status != osEventMessage) continue;

        rxMsg = (uart_event_msg_t *) rxEvent.value.p;
        if(rxMsg->event == UART_COMMAND_EVENT)
        {
            if (g_uart_process_handler)
            {
                g_uart_process_handler((char *)rxMsg->param, rxMsg->length);
            }
        }

        if(rxMsg != NULL)
            free(rxMsg);
    }
}

void uart_cmd_task_create(void)
{
    osMessageQDef_t uart_queue_def;
    osThreadDef_t uart_task_def;

    /** create task */
    uart_task_def.name = "uart_task";
    uart_task_def.stacksize = 256; //512;
    uart_task_def.tpriority = (osPriority)UART_CMD_TASK_PRIORITY;
    uart_task_def.pthread = uart_cmd_task;
    g_UartTaskHandleId = osThreadCreate(&uart_task_def, (void *)g_UartTaskHandleId);
    if(g_UartTaskHandleId == NULL)
    {
        tracer_log(LOG_HIGH_LEVEL, "create thread fail \r\n");
    }

    /** create message queue */
    uart_queue_def.item_sz = sizeof(uart_event_msg_t);
    uart_queue_def.queue_sz = UART_QUEUE_SIZE;
    g_UartTaskQueueId = osMessageCreate(&uart_queue_def, g_UartTaskHandleId);
    if(g_UartTaskQueueId == NULL)
    {
        tracer_log(LOG_HIGH_LEVEL, "create queue fail \r\n");
    }
}

void uart_cmd_handler_register(uart_cmd_process_cb_t handler, void *param)
{
    g_uart_process_handler = handler;
}

void uart_cmd_init(uart_port_t uart_num, uint32_t baud_rate, uart_rx_isr_cb_t isr_callback)
{
    E_UartIdx_t eUartIdx = UART_IDX_0;

    /* create uart command task */
    uart_cmd_task_create();

    switch (uart_num)
    {
        case UART_NUM_0:
            eUartIdx = UART_IDX_0;
            break;
        case UART_NUM_1:
            eUartIdx = UART_IDX_1;
            break;
        default:
            break;
    }

    /* install uart driver */
    Hal_Uart_Init(eUartIdx, baud_rate, DATA_BIT_8, PARITY_NONE, STOP_BIT_1, 0);
    Hal_Uart_RxCallBackFuncSet(eUartIdx, isr_callback);
    Hal_Uart_RxIntEn(eUartIdx, 1);
}

#endif //#ifdef ADA_REMOTE_CTRL

