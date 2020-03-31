/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
// modified for ali_lib by Jeff, 20190525
// fix the compile warning
#if 1   // the modified
#include "cmsis_os.h"
#else   // the original
#endif

#include "infra_types.h"
#include "infra_defs.h"
#include "wrappers_defs.h"

#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "lwip_helper.h"
#include "network_config.h"
#include "wifi_types.h"
#include "wifi_api.h"
#include "kvmgr.h"
#include "kv.h"
#include "iot_import_awss.h"

#include "sys_os_config.h"
#include "infra_config.h"
#include "mw_ota_def.h"
#include "mw_ota.h"
#include "hal_system.h"
#define __DEMO__

#define PRODUCT_KEY_LEN     (20)
#define DEVICE_NAME_LEN     (32)
#define DEVICE_ID_LEN       (64)
#define DEVICE_SECRET_LEN   (64)
#define PRODUCT_SECRET_LEN  (64)
#define PID_STRLEN_MAX              (64)
#define MID_STRLEN_MAX              (64)
#define HAL_CID_LEN                 (64 + 1)

#ifdef __DEMO__
    char _product_key[PRODUCT_KEY_LEN + 1];
    char _product_secret[PRODUCT_SECRET_LEN + 1];
    char _device_name[DEVICE_NAME_LEN + 1];
    char _device_secret[DEVICE_SECRET_LEN + 1];
    uint32_t _ProductId;
#endif



#define HAL_SEM_MAX_COUNT           (10)
#define HAL_SEM_INIT_COUNT          (0)

//#define DEFAULT_THREAD_NAME         "linkkit_task"
#define DEFAULT_THREAD_SIZE         (256)
#define TASK_STACK_ALIGN_SIZE       (4)


#ifdef ALI_SINGLE_TASK
#else
static osSemaphoreId g_tDevInfoSem = NULL;
#endif

#ifdef ALI_UNBIND_REFINE
volatile uint8_t g_u8ReportReset = 0;
#endif


int HAL_GetPartnerID(char *pid_str)
{
    memset(pid_str, 0x0, PID_STRLEN_MAX);
#ifdef __DEMO__
    strcpy(pid_str, "opulinks");
#endif
    return strlen(pid_str);
}

int HAL_GetModuleID(char *mid_str)
{
    memset(mid_str, 0x0, MID_STRLEN_MAX);
#ifdef __DEMO__
    strcpy(mid_str, "OPL1000-DevKit");
#endif
    return strlen(mid_str);
}


char *HAL_GetChipID(_OU_ char *cid_str)
{
    memset(cid_str, 0x0, HAL_CID_LEN);
#ifdef __DEMO__
    strncpy(cid_str, "OPL1000", HAL_CID_LEN);
    cid_str[HAL_CID_LEN - 1] = '\0';
#endif
    return cid_str;
}




int HAL_SetProductKey(_IN_ char *product_key)
{
    int len = strlen(product_key);
#ifdef __DEMO__
    if (len > PRODUCT_KEY_LEN) {
        return -1;
    }
    memset(_product_key, 0x0, PRODUCT_KEY_LEN + 1);
    strncpy(_product_key, product_key, len);
#endif
    return len;
}


int HAL_SetDeviceName(_IN_ char *device_name)
{
    int len = strlen(device_name);
#ifdef __DEMO__
    if (len > DEVICE_NAME_LEN) {
        return -1;
    }
    memset(_device_name, 0x0, DEVICE_NAME_LEN + 1);
    strncpy(_device_name, device_name, len);
#endif
    return len;
}


int HAL_SetDeviceSecret(_IN_ char *device_secret)
{
    int len = strlen(device_secret);
#ifdef __DEMO__
    if (len > DEVICE_SECRET_LEN) {
        return -1;
    }
    memset(_device_secret, 0x0, DEVICE_SECRET_LEN + 1);
    strncpy(_device_secret, device_secret, len);
#endif
    return len;
}


int HAL_SetProductSecret(_IN_ char *product_secret)
{
    int len = strlen(product_secret);
#ifdef __DEMO__
    if (len > PRODUCT_SECRET_LEN) {
        return -1;
    }
    memset(_product_secret, 0x0, PRODUCT_SECRET_LEN + 1);
    strncpy(_product_secret, product_secret, len);
#endif
    return len;
}

int HAL_GetProductKey(_OU_ char *product_key)
{
    int len = strlen(_product_key);
    memset(product_key, 0x0, PRODUCT_KEY_LEN);

#ifdef __DEMO__
    strncpy(product_key, _product_key, len);
#endif

    return len;
}

int HAL_GetProductSecret(_OU_ char *product_secret)
{
    int len = strlen(_product_secret);
    memset(product_secret, 0x0, PRODUCT_SECRET_LEN);

#ifdef __DEMO__
    strncpy(product_secret, _product_secret, len);
#endif

    return len;
}

int HAL_GetDeviceName(_OU_ char *device_name)
{
    int len = strlen(_device_name);
    memset(device_name, 0x0, DEVICE_NAME_LEN);

#ifdef __DEMO__
    strncpy(device_name, _device_name, len);
#endif

    return strlen(device_name);
}

int HAL_GetDeviceSecret(_OU_ char *device_secret)
{
    int len = strlen(_device_secret);
    memset(device_secret, 0x0, DEVICE_SECRET_LEN);

#ifdef __DEMO__
    strncpy(device_secret, _device_secret, len);
#endif

    return len;
}

int HAL_SetProductId(uint32_t ulProductId)
{
    _ProductId = ulProductId;
    return 0;
}
uint32_t HAL_GetProductId()
{
	return _ProductId;
}


int HAL_Vsnprintf(_IN_ char *str, _IN_ const int len, _IN_ const char *format, va_list ap)
{
    return vsnprintf(str, len, format, ap);
}



extern int awss_reported;
extern int awss_bind_inited;
/**
 * @brief Deallocate memory block
 *
 * @param[in] ptr @n Pointer to a memory block previously allocated with platform_malloc.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_Free(void *ptr)
{
    //vPortFree(ptr);
    free(ptr);
}

/**
 * @brief Allocates a block of size bytes of memory, returning a pointer to the beginning of the block.
 *
 * @param [in] size @n specify block size in bytes.
 * @return A pointer to the beginning of the block.
 * @see None.
 * @note Block value is indeterminate.
 */
void *HAL_Malloc(uint32_t size)
{
    return malloc(size);
}

/**
 * @brief Create a mutex.
 *
 * @retval NULL : Initialize mutex failed.
 * @retval NOT_NULL : The mutex handle.
 * @see None.
 * @note None.
 */
void *HAL_MutexCreate(void)
{
    QueueHandle_t sem;

    sem = xSemaphoreCreateMutex();
    if (0 == sem) {
        return NULL;
    }

    return sem;
}

/**
 * @brief Destroy the specified mutex object, it will release related resource.
 *
 * @param [in] mutex @n The specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_MutexDestroy(void *mutex)
{
    QueueHandle_t sem;
    if (mutex == NULL) {
        return;
    }
    sem = (QueueHandle_t)mutex;
    vSemaphoreDelete(sem);
}

/**
 * @brief Waits until the specified mutex is in the signaled state.
 *
 * @param [in] mutex @n the specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_MutexLock(void *mutex)
{
    BaseType_t ret;
    QueueHandle_t sem;
    if (mutex == NULL) {
        return;
    }

    sem = (QueueHandle_t)mutex;
    ret = xSemaphoreTake(sem, 0xffffffff);
    while (pdPASS != ret) {
        ret = xSemaphoreTake(sem, 0xffffffff);
    }
}

/**
 * @brief Releases ownership of the specified mutex object..
 *
 * @param [in] mutex @n the specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_MutexUnlock(void *mutex)
{
    QueueHandle_t sem;
    if (mutex == NULL) {
        return;
    }
    sem = (QueueHandle_t)mutex;
    (void)xSemaphoreGive(sem);
}

/**
 * @brief Writes formatted data to stream.
 *
 * @param [in] fmt: @n String that contains the text to be written, it can optionally contain embedded format specifiers
     that specifies how subsequent arguments are converted for output.
 * @param [in] ...: @n the variable argument list, for formatted and inserted in the resulting string replacing their respective specifiers.
 * @return None.
 * @see None.
 * @note None.
 */
// modified for ali_lib by Jeff, 20190525
// redefine the HAL_Printf function
#if 1   // the modified
#else   // the original
void HAL_Printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
}
#endif

/**
 * @brief   create a semaphore
 *
 * @return semaphore handle.
 * @see None.
 * @note The recommended value of maximum count of the semaphore is 255.
 */
void *HAL_SemaphoreCreate(void)
{
    QueueHandle_t sem = 0;
    sem = xSemaphoreCreateCounting(HAL_SEM_MAX_COUNT, HAL_SEM_INIT_COUNT);
    if (0 == sem) {
        return NULL;
    }

    return sem;
}

/**
 * @brief   destory a semaphore
 *
 * @param[in] sem @n the specified sem.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_SemaphoreDestroy(void *sem)
{
    QueueHandle_t queue;

    if (sem == NULL) {
        return;
    }
    queue = (QueueHandle_t)sem;

    vSemaphoreDelete(queue);
}

/**
 * @brief   signal thread wait on a semaphore
 *
 * @param[in] sem @n the specified semaphore.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_SemaphorePost(void *sem)
{
#ifdef ALI_SINGLE_TASK
#else
    QueueHandle_t queue;
    if (sem == NULL) {
        return;
    }
    queue = (QueueHandle_t)sem;
    (void)xSemaphoreGive(queue);
#endif
}


/**
 * @brief   wait on a semaphore
 *
 * @param[in] sem @n the specified semaphore.
 * @param[in] timeout_ms @n timeout interval in millisecond.
     If timeout_ms is PLATFORM_WAIT_INFINITE, the function will return only when the semaphore is signaled.
 * @return
   @verbatim
   =  0: The state of the specified object is signaled.
   =  -1: The time-out interval elapsed, and the object's state is nonsignaled.
   @endverbatim
 * @see None.
 * @note None.
 */
int HAL_SemaphoreWait(void *sem, uint32_t timeout_ms)
{
#ifdef ALI_SINGLE_TASK
#else
    BaseType_t ret = 0;
    QueueHandle_t queue;
    if (sem == NULL) {
        return -1;
    }

    queue = (QueueHandle_t)sem;
    ret = xSemaphoreTake(queue, timeout_ms);
    if (pdPASS != ret) {
        return -1;
    }
#endif
    return 0;
}

/**
 * @brief Sleep thread itself.
 *
 * @param [in] ms @n the time interval for which execution is to be suspended, in milliseconds.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_SleepMs(uint32_t ms)
{
    osDelay(ms);
}

/**
 * @brief Writes formatted data to string.
 *
 * @param [out] str: @n String that holds written text.
 * @param [in] len: @n Maximum length of character will be written
 * @param [in] fmt: @n Format that contains the text to be written, it can optionally contain embedded format specifiers
     that specifies how subsequent arguments are converted for output.
 * @param [in] ...: @n the variable argument list, for formatted and inserted in the resulting string replacing their respective specifiers.
 * @return bytes of character successfully written into string.
 * @see None.
 * @note None.
 */
int HAL_Snprintf(char *str, const int len, const char *fmt, ...)
{
    va_list args;
    int rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);
    return rc;
}


/**
 * @brief  create a thread
 *
 * @param[out] thread_handle @n The new thread handle, memory allocated before thread created and return it, free it after thread joined or exit.
 * @param[in] start_routine @n A pointer to the application-defined function to be executed by the thread.
        This pointer represents the starting address of the thread.
 * @param[in] arg @n A pointer to a variable to be passed to the start_routine.
 * @param[in] hal_os_thread_param @n A pointer to stack params.
 * @param[out] stack_used @n if platform used stack buffer, set stack_used to 1, otherwise set it to 0.
 * @return
   @verbatim
     = 0: on success.
     = -1: error occur.
   @endverbatim
 * @see None.
 * @note None.
 */
int HAL_ThreadCreate(
            void **thread_handle,
            void *(*work_routine)(void *),
            void *arg,
            hal_os_thread_param_t *hal_os_thread_param,
            int *stack_used)
{
#ifdef ALI_SINGLE_TASK
#else
    char *name;
    size_t stacksize;
    osThreadDef_t thread_def;

    osThreadId handle;

    if (thread_handle == NULL) {
        return -1;
    }

    if (work_routine == NULL) {
        return -1;
    }

    if (hal_os_thread_param == NULL) {
        return -1;
    }
    if (stack_used == NULL) {
        return -1;
    }

    if (stack_used != NULL) {
        *stack_used = 0;
    }

    if (!hal_os_thread_param->name) {
        name = DEFAULT_THREAD_NAME;
    } else {
        name = hal_os_thread_param->name;
    }

    if (hal_os_thread_param->stack_size == 0) {
        stacksize = DEFAULT_THREAD_SIZE;
    } else {
        stacksize = hal_os_thread_param->stack_size;
    }

    thread_def.name = name;
    thread_def.pthread = (os_pthread)work_routine;
    thread_def.tpriority = (osPriority)hal_os_thread_param->priority;
    thread_def.instances = 0;
    thread_def.stacksize = (stacksize + TASK_STACK_ALIGN_SIZE - 1) / TASK_STACK_ALIGN_SIZE;

    handle = osThreadCreate(&thread_def, arg);
    if (NULL == handle) {
        return -1;
    }
    *thread_handle = (void *)handle;
#endif
    return 0;
}

void HAL_ThreadDelete(void *thread_handle)
{
	
	osThreadTerminate((osThreadId)thread_handle); 

	return;
}

/**
 * @brief Retrieves the number of milliseconds that have elapsed since the system was boot.
 *
 * @return the number of milliseconds.
 * @see None.
 * @note None.
 */
uint64_t HAL_UptimeMs(void)
{
    return (uint64_t)xTaskGetTickCount();
}



void HAL_Srandom(uint32_t seed)
{
    srand(seed);
}

uint32_t HAL_Random(uint32_t region)
{
    return (region > 0) ? (rand() % region) : 0;
}


void *HAL_Timer_Create(const char *name, void (*func)(void *), void *user_data)
{
    osTimerDef_t timer_def;
    osTimerId hal_timer;

    /* check parameter */
    if (func == NULL) {
        return NULL;
    }

    //create cmd timer
    timer_def.ptimer =  (void (*)(void const*))func;
    hal_timer = osTimerCreate(&timer_def, osTimerOnce, NULL);
//    printf("%s HAL_Timer_Create 0x%x\r\n\r\n", name, (uint32_t)(hal_timer) );

    return (void*)hal_timer;

#if 0
    timer_t *timer = NULL;

    struct sigevent ent;

    /* check parameter */
    if (func == NULL) {
        return NULL;
    }

    timer = (timer_t *)malloc(sizeof(time_t));

    /* Init */
    memset(&ent, 0x00, sizeof(struct sigevent));

    /* create a timer */
    ent.sigev_notify = SIGEV_THREAD;
    ent.sigev_notify_function = (void (*)(union sigval))func;
    ent.sigev_value.sival_ptr = user_data;

//    printf("HAL_Timer_Create\n");

    if (timer_create(CLOCK_MONOTONIC, &ent, timer) != 0) {
        free(timer);
        return NULL;
    }

    return (void *)timer;
#endif
}

int HAL_Timer_Start(void *timer, int ms)
{
    osTimerStart(timer, ms);
//    printf("HAL_Timer_Start 0x%x\n", (uint32_t)timer);

    return 0;
#if 0
    struct itimerspec ts;

    /* check parameter */
    if (timer == NULL) {
        return -1;
    }

    /* it_interval=0: timer run only once */
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    /* it_value=0: stop timer */
    ts.it_value.tv_sec = ms / 1000;
    ts.it_value.tv_nsec = (ms % 1000) * 1000;

    return timer_settime(*(timer_t *)timer, 0, &ts, NULL);
#endif
}

int HAL_Timer_Stop(void *timer)
{
    osTimerStop(timer);
//    printf("HAL_Timer_Stop 0x%x\n", (uint32_t)timer);

    return 0;
#if 0
    struct itimerspec ts;

    /* check parameter */
    if (timer == NULL) {
        return -1;
    }

    /* it_interval=0: timer run only once */
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    /* it_value=0: stop timer */
    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = 0;

    return timer_settime(*(timer_t *)timer, 0, &ts, NULL);
#endif
}

int HAL_Timer_Delete(void *timer)
{
    osTimerDelete(timer);
//    printf("HAL_Timer_Delete 0x%x\r\n", (uint32_t)timer);

    return 0;
#if 0
    int ret = 0;

    /* check parameter */
    if (timer == NULL) {
        return -1;
    }

    ret = timer_delete(*(timer_t *)timer);

    free(timer);

    return ret;
#endif
}


uint32_t HAL_Wifi_Get_IP(char ip_str[NETWORK_ADDR_LEN], const char *ifname)
{
	    if (ip_str == NULL) {
        return 0;
    }
			
	uint8_t ubaIp[4]={0};

	struct netif *iface = netif_find("st1");

    ubaIp[0] = (iface->ip_addr.u_addr.ip4.addr >> 0) & 0xFF;
    ubaIp[1] = (iface->ip_addr.u_addr.ip4.addr >> 8) & 0xFF;
    ubaIp[2] = (iface->ip_addr.u_addr.ip4.addr >> 16) & 0xFF;
    ubaIp[3] = (iface->ip_addr.u_addr.ip4.addr >> 24) & 0xFF;

	int len=0;
	len = sprintf(ip_str, "%d.%d.%d.%d", ubaIp[0], ubaIp[1], ubaIp[2], ubaIp[3]);
	
	return len;

	
}

char *HAL_Wifi_Get_Mac(_OU_ char mac_str[HAL_MAC_LEN])
{
    uint8_t mac[6] = {0};

    if (mac_str == NULL) {
        return NULL;
    }
		
		wifi_config_get_mac_address(WIFI_MODE_STA, mac);
		sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return mac_str;
}

int HAL_Kv_Set(const char *key, const void *val, int len, int sync)
{
#if 0
    if (!kvfile) {
        kvfile = kv_open("/tmp/kvfile.db");
        if (!kvfile) {
            return -1;
        }
    }

    return kv_set_blob(kvfile, (char *)key, (char *)val, len);
#endif
    return aos_kv_set(key, val, len, 1);
    //return -1;
}

int HAL_Kv_Get(const char *key, void *buffer, int *buffer_len)
{
#if 0
    if (!kvfile) {
        kvfile = kv_open("/tmp/kvfile.db");
        if (!kvfile) {
            return -1;
        }
    }

   return kv_get_blob(kvfile, (char *)key, buffer, buffer_len);
#endif
//    printf("HAL_Kv_Get\r\n");
    return aos_kv_get(key, buffer, buffer_len);
    //return -1;

}

int HAL_Kv_Del(const char *key)
{
#if 0
    if (!kvfile) {
        kvfile = kv_open("/tmp/kvfile.db");
        if (!kvfile) {
            return -1;
        }
    }

    return kv_del(kvfile, (char *)key);
#endif
    return aos_kv_del(key);
    //return -1;
}

#ifdef WIFI_PROVISION_ENABLED
int HAL_Wifi_Get_Ap_Info(char ssid[HAL_MAX_SSID_LEN],char passwd[HAL_MAX_PASSWD_LEN],uint8_t bssid[ETH_ALEN])
{
	return 0;
}
int HAL_Awss_Connect_Ap(
            _IN_ uint32_t connection_timeout_ms,
            _IN_ char ssid[HAL_MAX_SSID_LEN],
            _IN_ char passwd[HAL_MAX_PASSWD_LEN],
            _IN_OPT_ enum AWSS_AUTH_TYPE auth,
            _IN_OPT_ enum AWSS_ENC_TYPE encry,
            _IN_OPT_ uint8_t bssid[ETH_ALEN],
            _IN_OPT_ uint8_t channel)
{
						
	return (int)1;
}
#endif

void HAL_Reboot(void)
{
    Hal_Sys_SwResetAll();
}

static const uint16_t HDR_LEN=24;    
static char g_tmpbuffer[HDR_LEN];
static uint32_t g_offset=0;    
static uint16_t g_tdx=0;
static uint16_t g_IsDataIn=0;
static uint16_t g_FirstTimeToWrite=0;
static uint16_t g_RcvRemainingOTAhdr=0;
static int idx=0;

volatile uint8_t g_u8OtaIgnored = 0;
    
void HAL_Firmware_Persistence_Start(void) {
    
//    printf("HAL_Firmware_Persistence_Start\r\n");    
    memset(g_tmpbuffer,0,sizeof(char)*HDR_LEN);
    g_offset=0;
    g_tdx=0;
    g_IsDataIn=0;
    g_FirstTimeToWrite=0;
    g_RcvRemainingOTAhdr=0;
    idx=0;

    g_u8OtaIgnored = 0;
}


int HAL_Firmware_Persistence_Write(_IN_ char *buffer, _IN_ uint32_t length)
{
    int iRet = -1;
    T_MwOtaFlashHeader *ota_hdr = NULL;
    int Parser_hdr=0;
    int tt;

    if(g_u8OtaIgnored)
    {
        goto done;
    }

    if(g_IsDataIn==0)
    {
        for(idx=0;idx<length;idx++)
        {
            g_offset++;
            if(g_offset==MW_OTA_HEADER_ADDR_1)
            {
                for(tt=idx+1;tt<length;tt++)
                {            
                    g_tmpbuffer[g_tdx]=buffer[tt];            
                    if(g_tdx==(HDR_LEN)-1)
                    {
                        Parser_hdr=1;
//                        printf("g_tdx=%d\r\n", g_tdx);                
                        break;
                    }
                    g_tdx++;
                }
                g_RcvRemainingOTAhdr=1;
                g_offset += (length - idx - 1);
                iRet = 0;
                goto done;
            }
            else if(g_offset==MW_OTA_IMAGE_ADDR_1)
            {
                g_IsDataIn=1;
                g_FirstTimeToWrite=1;
                break;
            }
            
            if(g_RcvRemainingOTAhdr==1)
            {
                g_tmpbuffer[g_tdx]=buffer[idx];            
                if(g_tdx==(HDR_LEN)-1)
                {
                    Parser_hdr=1;
                    g_RcvRemainingOTAhdr=0;
                    g_offset += (length - idx - 1);
                    break;
                }
                g_tdx++;
                
            }
            
         }
    }
         
    
    
    if((Parser_hdr==1)&&(g_tdx==(HDR_LEN)-1))
    {
        uint16_t u16ProjectId = 0;
        uint16_t u16ChipId = 0;
        uint16_t u16FirmwareId = 0;
    
        if(MwOta_VersionGet(&u16ProjectId, &u16ChipId, &u16FirmwareId) != MW_OTA_OK)
        {
            printf("[%s %d] MwOta_VersionGet fail\n", __func__, __LINE__);
            iRet = -2;
            goto done;
        }
          
        ota_hdr = (T_MwOtaFlashHeader*)g_tmpbuffer;        
        printf("\r\nproj_id=%d, chip_id=%d, fw_id=%d, checksum=%d, total_len=%d\r\n",
        ota_hdr->uwProjectId, ota_hdr->uwChipId, ota_hdr->uwFirmwareId, ota_hdr->ulImageSum, ota_hdr->ulImageSize);
              
        printf("\r\ndevice proj_id=%d, chip_id=%d, fw_id=%d\r\n",
               u16ProjectId, u16ChipId, u16FirmwareId);

        if((ota_hdr->uwProjectId != u16ProjectId) || 
           (ota_hdr->uwChipId != u16ChipId))
        {
            printf("[%s %d] invalid firmware\n", __func__, __LINE__);
            iRet = -3;
            goto done;
        }

        /*
        if(ota_hdr->uwFirmwareId == u16FirmwareId)
        {
            printf("[%s %d] ignore same firmware\n", __func__, __LINE__);
            iRet = -4;
            goto done;
        }
        */
              
        if (MwOta_Prepare(ota_hdr->uwProjectId, ota_hdr->uwChipId, ota_hdr->uwFirmwareId, ota_hdr->ulImageSize, ota_hdr->ulImageSum) != MW_OTA_OK)
        {
            Parser_hdr=0;
            g_offset=0;
            g_tdx=0;
            if (MwOta_DataGiveUp() != MW_OTA_OK)
            {
                printf("ota_abort error.\r\n");
            }
            printf("ota_prepare fail\r\n");
            goto done;
        }
        Parser_hdr=0;
    }
    
    
    if(g_IsDataIn==1)
    {
        if(g_FirstTimeToWrite==1)
        {
              
            if (MwOta_DataIn((uint8_t*)(buffer+(idx+1)), (length-(idx+1))) != MW_OTA_OK)
            {
                if (MwOta_DataGiveUp() != MW_OTA_OK)
                {
                    printf("ota_abort error.\r\n");
                }
                printf("ota_data_write fail\r\n");
                goto done;
            }
            g_FirstTimeToWrite=0;           
        }
        else
        {
            if (MwOta_DataIn((uint8_t*)buffer, length) != MW_OTA_OK)
            {
                if (MwOta_DataGiveUp() != MW_OTA_OK)
                {
                    printf("ota_abort error.\r\n");
                }
                printf("ota_data_write fail\r\n");
                goto done;
            }
        }
    }

    iRet = 0;

done:
    if(iRet)
    {
        if(!g_u8OtaIgnored)
        {
            g_u8OtaIgnored = 1;
        }
    }
    
    return iRet;
}

int HAL_Firmware_Persistence_Stop(void)
{
//    printf("HAL_Firmware_Persistence_Stop\r\n");
    if (MwOta_DataFinish() != MW_OTA_OK)
    {
        printf("ota_data_finish error.\r\n");
    }
    return 0;
}    


osThreadId g_tAliNetlinkTaskId;
osMessageQId g_tAliNetlinkQueueId;
	
static T_Ali_Netlink_EvtHandlerTbl g_tAliNetlinkTxTaskEvtHandlerTbl[] =
{
		{ALI_NETLINK_AWSS_REPORT_TOKEN_TO_CLOUD,  Ali_Netlink_Awss_Report_Token_To_Cloud},
		{ALI_NETLINK_AWSS_DEV_BIND_NOTIFY,        Ali_Netlink_Awss_Dev_Bind_Notify},
		{ALI_NETLINK_AWSS_DEV_BIND_NOTIFY_FUN,   __Ali_Netlink_Awss_Dev_Bind_Notify},
		{ALI_NETLINK_AWSS_SUCCESS_NOTIFY,   			Ali_Netlink_AliSendAwssSucNotify},
		{ALI_NETLINK_AWSS_PROCESS_GET_DEVINFO,   	Ali_Netlink_Awss_Process_Get_Devinfo},
        {ALI_NETLINK_AWSS_REPORT_RESET_TO_CLOUD,   	Ali_Netlink_Awss_Report_Reset_To_Cloud},
//		{ALI_NETLINK_AWSS_DEV_INFO_NOTIFY,             Iot_Awss_Dev_Info_Notify},    
    {0xFFFFFFFF,                            NULL}
};	

extern int awss_report_token_to_cloud(void);
void Ali_Netlink_Awss_Report_Token_To_Cloud(uint32_t evt_type, void *data, int len)
{
    
			awss_report_token_to_cloud();

}
extern int awss_dev_bind_notify(void);
void Ali_Netlink_Awss_Dev_Bind_Notify(uint32_t evt_type, void *data, int len)
{
    awss_dev_bind_notify();
			
}

extern int __awss_dev_bind_notify(void);
void __Ali_Netlink_Awss_Dev_Bind_Notify(uint32_t evt_type, void *data, int len)
{
    
//    printf("Enter __Iot_Awss_Dev_Bind_Notify\r\n");
    __awss_dev_bind_notify();

}

/*extern int __awss_devinfo_notify(void);
static void Ali_Netlink_Awss_Dev_Info_Notify(uint32_t evt_type, void *data, int len)
{
    
			__awss_devinfo_notify();

}
*/	

extern int __awss_suc_notify(void);
void Ali_Netlink_AliSendAwssSucNotify(uint32_t evt_type, void *data, int len)
{
	__awss_suc_notify();
}

extern int awss_process_get_devinfo(void);	
void Ali_Netlink_Awss_Process_Get_Devinfo(uint32_t evt_type, void *data, int len)
{
//		printf("Enter Ali_Netlink_Awss_Process_Get_Devinfo\r\n");    
		awss_process_get_devinfo();

}
extern int awss_report_reset_to_cloud(void);	
void Ali_Netlink_Awss_Report_Reset_To_Cloud(uint32_t evt_type, void *data, int len)
{
//    printf("Enter Ali_Netlink_Awss_Report_Reset_To_Cloud\r\n");
	awss_report_reset_to_cloud();
}

void ali_netlink_TaskEvtHandler(uint32_t evt_type, void *data, int len)
{
    uint32_t i = 0;

    while (g_tAliNetlinkTxTaskEvtHandlerTbl[i].ulEventId != 0xFFFFFFFF)
    {
        // match
        if (g_tAliNetlinkTxTaskEvtHandlerTbl[i].ulEventId == evt_type)
        {
            g_tAliNetlinkTxTaskEvtHandlerTbl[i].fpFunc(evt_type, data, len);
            break;
        }

        i++;
    }

    // not match
    if (g_tAliNetlinkTxTaskEvtHandlerTbl[i].ulEventId == 0xFFFFFFFF)
    {
    }
}	
	
#ifdef ALI_SINGLE_TASK
void ali_netlink_Task(void *args)
{
    osEvent rxEvent;
    xAliNetlinkDataMessage_t *rxMsg;

    {
        /* Wait event */
        rxEvent = osMessageGet(g_tAliNetlinkQueueId, ALI_TASK_POLLING_PERIOD);
        if(rxEvent.status != osEventMessage)
        {
            goto done;
        }

        rxMsg = (xAliNetlinkDataMessage_t *)rxEvent.value.p;
        ali_netlink_TaskEvtHandler(rxMsg->event, rxMsg->ucaMessage, rxMsg->length);

        /* Release buffer */
        if (rxMsg != NULL)
            free(rxMsg);
    }

done:
    return;
}
#else //#ifdef ALI_SINGLE_TASK
void ali_netlink_Task(void *args)
{
    osEvent rxEvent;
    xAliNetlinkDataMessage_t *rxMsg;

    while (1)
    {
        /* Wait event */
        rxEvent = osMessageGet(g_tAliNetlinkQueueId, osWaitForever);
        if(rxEvent.status != osEventMessage)
            continue;

        rxMsg = (xAliNetlinkDataMessage_t *)rxEvent.value.p;
        ali_netlink_TaskEvtHandler(rxMsg->event, rxMsg->ucaMessage, rxMsg->length);

        /* Release buffer */
        if (rxMsg != NULL)
            free(rxMsg);
    }
}	
#endif //#ifdef ALI_SINGLE_TASK
	
	
void hal_ali_netlink_task_init()
{
    osMessageQDef_t queue_def;

#ifdef ALI_SINGLE_TASK
#else
	osThreadDef_t task_def;

    /* Create IoT Tx task */
    task_def.name = "netlink task";
    task_def.stacksize = 512;
    task_def.tpriority = osPriorityRealtime; //OS_TASK_PRIORITY_APP;
    task_def.pthread = ali_netlink_Task;
    g_tAliNetlinkTaskId = osThreadCreate(&task_def, (void*)NULL);
    if(g_tAliNetlinkTaskId == NULL)
    {
        printf("linkkitsdk: netlink task create fail \r\n");
    }
    else
    {
        printf("linkkitsdk: netlink task create successful \r\n");
    }
#endif //#ifdef ALI_SINGLE_TASK

    /* Create IoT Tx message queue*/
    queue_def.item_sz = sizeof(xAliNetlinkDataMessage_t);
    queue_def.queue_sz = 20;
    g_tAliNetlinkQueueId = osMessageCreate(&queue_def, NULL);
    if(g_tAliNetlinkQueueId == NULL)
    {
        printf("linkkitsdk: netlink create queue fail \r\n");
    }
	
#ifdef ALI_SINGLE_TASK
#else
    if(g_tDevInfoSem == NULL)
    {
        osSemaphoreDef_t tSemDef = {0};

        g_tDevInfoSem = osSemaphoreCreate(&tSemDef, 1);
    
        if(g_tDevInfoSem == NULL)
        {
            printf("[%s %d] osSemaphoreCreate fail\n", __func__, __LINE__);
        }
    }
#endif //#ifdef ALI_SINGLE_TASK
}
	
int hal_ali_netlink_Task_MsgSend(int msg_type, uint8_t *data, int data_len)
{
	xAliNetlinkDataMessage_t *pMsg = 0;

	if (NULL == g_tAliNetlinkQueueId)
	{
        printf("linkkitsdk: No IoT Tx task queue \r\n");
        return -1;
	}

    /* Mem allocate */
    pMsg = malloc(sizeof(xAliNetlinkDataMessage_t) + data_len);
    if (pMsg == NULL)
	{
        printf("BLEWIFI: IoT Tx task pmsg allocate fail \r\n");
	    goto error;
    }
    
    pMsg->event = msg_type;
    pMsg->length = data_len;
    if (data_len > 0)
    {
        memcpy(pMsg->ucaMessage, data, data_len);
    }

    #ifdef ALI_SINGLE_TASK
    if (osMessagePut(g_tAliNetlinkQueueId, (uint32_t)pMsg, 0) != osOK)
    #else
    if (osMessagePut(g_tAliNetlinkQueueId, (uint32_t)pMsg, osWaitForever) != osOK)
    #endif
    {
        printf("BLEWIFI: IoT Tx task message send fail \r\n");
        goto error;
    }

    return 0;

error:
	if (pMsg != NULL)
	{
	    free(pMsg);
    }
	
	return -1;
}

#ifdef ALI_SINGLE_TASK
#else
int hal_dev_info_lock(void)
{
    int iRet = -1;

    if(g_tDevInfoSem == NULL)
    {
        printf("[%s %d] g_tDevInfoSem is null\n", __func__, __LINE__);
        goto done;
    }
    
    if(osSemaphoreWait(g_tDevInfoSem, osWaitForever) != osOK)
    {
        printf("[%s %d] osSemaphoreWait fail\n", __func__, __LINE__);
        goto done;
    }

    iRet = 0;

done:
    return iRet;
}

int hal_dev_info_unlock(void)
{
    int iRet = -1;

    if(g_tDevInfoSem == NULL)
    {
        printf("[%s %d] g_tDevInfoSem is null\n", __func__, __LINE__);
        goto done;
    }
    
    if(osSemaphoreRelease(g_tDevInfoSem) != osOK)
    {
        printf("[%s %d] osSemaphoreRelease fail\n", __func__, __LINE__);
        goto done;
    }

    iRet = 0;

done:
    return iRet;
}
#endif //#ifdef ALI_SINGLE_TASK


/*
 * This need to be same with app version as in uOTA module (ota_version.h)

    #ifndef SYSINFO_APP_VERSION
    #define SYSINFO_APP_VERSION "app-1.0.0-20180101.1000"
    #endif
 *
 */
static char g_FrimwareVer[IOTX_FIRMWARE_VER_LEN+1];

int HAL_GetFirmwareVersion(_OU_ char *version)
{
//    char *ver = "app-1.0.0-20191213.1200";
    int len = strlen(g_FrimwareVer);
    memset(version, 0x0, IOTX_FIRMWARE_VER_LEN);
//#ifdef __DEMO__
    strncpy(version, g_FrimwareVer, len);
    version[len] = '\0';
//#endif
    return strlen(version);
}


void HAL_SetFirmwareVersion(_IN_ char *FwVersion) 
{
    int len = strlen(FwVersion);
    memset(g_FrimwareVer, 0x0, IOTX_FIRMWARE_VER_LEN);

    strncpy(g_FrimwareVer, FwVersion, len);
    g_FrimwareVer[len] = '\0';

}

//Netlink Kevin modify for rebinding
void HAL_ResetAliBindflag()
{
    awss_reported = 0;
    awss_bind_inited = 0;
}    

int HAL_GetDeviceID(_OU_ char *device_id)
{
    memset(device_id, 0x0, DEVICE_ID_LEN);
#ifdef __DEMO__
    HAL_Snprintf(device_id, DEVICE_ID_LEN, "%s.%s", _product_key, _device_name);
    device_id[DEVICE_ID_LEN - 1] = '\0';
#endif

    return strlen(device_id);
}

#ifdef ALI_UNBIND_REFINE
uint8_t HAL_GetReportReset(void)
{
    return g_u8ReportReset;
}

void HAL_SetReportReset(uint8_t u8Reset)
{
    g_u8ReportReset = (u8Reset)?(1):(0);
}
#endif

