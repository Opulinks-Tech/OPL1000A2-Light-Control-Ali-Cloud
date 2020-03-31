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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cmsis_os.h"
#include "event_loop.h"
#include "wifi_api.h"
#include "wifi_event.h"
#include "wifi_event_handler.h"
#include "lwip_helper.h"
#include "mdns.h"
#include "example_mdns.h"

#define WIFI_READY_TIME         2000
#define WIFI_SSID               "Opulinks-TEST-AP"
#define WIFI_PASSWORD           "1234abcd"

osThreadId app_task_id;

bool g_connect_flag = false;
bool g_mdns_service_setup = false;

err_t mdns_example_init(void);

static void srv_txt(struct mdns_service *service, void *txt_userdata)
{
    err_t res;
    LWIP_UNUSED_ARG(txt_userdata);

	  res = mdns_resp_add_service_txtitem(service, "path=/foobar", 12);
    //change TXT item value
    res = mdns_resp_add_service_txtitem(service, "u=user", 6);    	
    res = mdns_resp_add_service_txtitem(service, "p=password", 10);    
    res = mdns_resp_add_service_txtitem(service, "board=DEVKIT", 12);	
    LWIP_ERROR("mdns add service txt failed\n", (res == ERR_OK), return);
}

err_t mdns_example_init()
{
    err_t ret; 
    printf("\r\nmdns resp init! \r\n");
    mdns_resp_init();
    
    printf("\r\nmdns resp add netif! ttl = 3600s \r\n");
    mdns_resp_add_netif(netif_default, "opl1000", 3600);
    
    printf("\r\nmdns resp add service! \r\n");
    
    // web.local._http._tcp.xx:xx:xx:xx:xx
    // The service protocol, DNSSD_PROTO_TCP for TCP ("_tcp") and DNSSD_PROTO_UDP for others ("_udp")
    ret = mdns_resp_add_service(netif_default, "opl1000", "_http", DNSSD_PROTO_TCP, 80, 3600, srv_txt, NULL);  
    return ret;  
}

int wifi_event_sta_connected_handler(wifi_event_t event, uint8_t *data, uint32_t length)
{
    return wifi_station_connected_event_handler(data);
}

int wifi_event_sta_disconnected_handler(wifi_event_t event, uint8_t *data, uint32_t length)
{
    return wifi_station_disconnected_event_handler(data);
}

void wifi_wait_ready(void)
{
    /* wait a while for system ready */
    osDelay(WIFI_READY_TIME);
}

int wifi_do_scan(int mode)
{
    wifi_scan_config_t scan_config;
    memset(&scan_config, 0, sizeof(scan_config));
    wifi_scan_start(&scan_config, NULL);
    return 0;
}

int wifi_connection(void)
{
    int iRet = -1;
    wifi_config_t wifi_config = {0};
    wifi_scan_list_t *p_scan_list = NULL;
    int i = 0;
    int isMatched = 0;
		
    p_scan_list = (wifi_scan_list_t *)malloc(sizeof(wifi_scan_list_t));

    if(p_scan_list == NULL)
    {
        goto done;
    }

    memset(p_scan_list, 0, sizeof(wifi_scan_list_t));

    /* Read Confguration */
    wifi_get_config(WIFI_MODE_STA, &wifi_config);

    /* Get APs list */
    wifi_scan_get_ap_list(p_scan_list);

    /* Search if AP matched */
    for (i=0; i< p_scan_list->num; i++) {
        if (memcmp(p_scan_list->ap_record[i].bssid, wifi_config.sta_config.bssid, WIFI_MAC_ADDRESS_LENGTH) == 0)
        {
            isMatched = 1;
            break;
        }

        if (memcmp(p_scan_list->ap_record[i].ssid, wifi_config.sta_config.ssid, wifi_config.sta_config.ssid_length) == 0)
        {
            isMatched = 1;
            break;
        }
    }

    if(isMatched == 1) {
        /* Wi-Fi Connection */
        wifi_connection_connect(&wifi_config);

    } else {
        /* Scan Again */
        wifi_do_scan(WIFI_SCAN_TYPE_MIX);
    }

    iRet = 0;

done:
    if(p_scan_list)
    {
        free(p_scan_list);
    }

    return iRet;
}


int wifi_event_handler_cb(wifi_event_id_t event_id, void *data, uint16_t length)
{
    switch(event_id) {
    case WIFI_EVENT_STA_START:
        printf("\r\nWi-Fi Start \r\n");
        wifi_wait_ready();
        wifi_do_scan(WIFI_SCAN_TYPE_MIX);
        break;
    case WIFI_EVENT_STA_CONNECTED:
        lwip_net_start(WIFI_MODE_STA);
        printf("\r\nWi-Fi Connected \r\n");
        g_connect_flag = true;
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        printf("\r\nWi-Fi Disconnected \r\n");
        wifi_do_scan(WIFI_SCAN_TYPE_MIX);
        g_connect_flag = false;
        break;
    case WIFI_EVENT_SCAN_COMPLETE:
        printf("\r\nWi-Fi Scan Done \r\n");
        wifi_connection();
        break;
    case WIFI_EVENT_STA_GOT_IP:
        printf("\r\nWi-Fi Got IP \r\n");
        lwip_get_ip_info("st1");
        break;
    case WIFI_EVENT_STA_CONNECTION_FAILED:
        printf("\r\nWi-Fi Connected failed\r\n");
        wifi_do_scan(WIFI_SCAN_TYPE_MIX);
        break;
    default:
        printf("\r\n Unknown Event %d \r\n", event_id);
        break;
    }
    return 0;
}

void user_wifi_app_entry(void *args)
{
	  err_t ret;
    /* Tcpip stack and net interface initialization,  dhcp client process initialization. */
    lwip_network_init(WIFI_MODE_STA);

    /* Waiting for connection & got IP from DHCP server */
    lwip_net_ready();

    while (1) {
        if(g_connect_flag == true )
        {
            printf("OPL1000 is connected to AP \r\n");
            osDelay(2000);

            if (g_mdns_service_setup == false)
            {
                ret = mdns_example_init();

                if (ret == ERR_OK)
                {
                    g_mdns_service_setup = true; 
                }
            }
        }
        else 
        {
            printf("OPL1000 is not connected to AP \r\n");
        } 
        osDelay(1000);
    }
}

void WifiAppInit(void)
{
    osThreadDef_t task_def;
    wifi_init_config_t int_cfg = {.event_handler = (wifi_event_notify_cb_t)&wifi_event_loop_send, .magic = 0x1F2F3F4F};
    wifi_config_t wifi_config = {0};

    unsigned char bssid[WIFI_MAC_ADDRESS_LENGTH] = {0x74, 0xda, 0xda, 0xe7, 0x08, 0xf1};

    /* Register internal wifi event default handler */
    wifi_register_event_handler(WIFI_EVENT_STA_CONNECTED, (wifi_event_handler_t)wifi_event_sta_connected_handler);
    wifi_register_event_handler(WIFI_EVENT_STA_DISCONNECTED, (wifi_event_handler_t)wifi_event_sta_disconnected_handler);

    /* Event Loop Initialization */
    wifi_event_loop_init((wifi_event_cb_t)wifi_event_handler_cb);

    /* Initialize wifi stack and register wifi init complete event handler */
    wifi_init(&int_cfg, NULL);

    /* Set user's configuration */
    strcpy((char *)wifi_config.sta_config.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.sta_config.password, WIFI_PASSWORD);
    wifi_config.sta_config.ssid_length = strlen(WIFI_SSID);
    wifi_config.sta_config.password_length = strlen(WIFI_PASSWORD);
    memcpy(wifi_config.sta_config.bssid, bssid, WIFI_MAC_ADDRESS_LENGTH);

    wifi_set_config(WIFI_MODE_STA, &wifi_config);

    /* Wi-Fi operation start */
    wifi_start();

    /* Create task */
    task_def.name = "user_app";
    task_def.stacksize = OS_TASK_STACK_SIZE_APP;
    task_def.tpriority = OS_TASK_PRIORITY_APP;
    task_def.pthread = user_wifi_app_entry;
    app_task_id = osThreadCreate(&task_def, (void*)NULL);
    if(app_task_id == NULL)
    {
        printf("user_app Task create fail \r\n");
    }
    else
    {
        printf("user_app Task create successful \r\n");
    }
}

