#ifndef _AUTO_TEST_STATS_H_
#define _AUTO_TEST_STATS_H_

#include "cmsis_os.h"
#include "auto_test_err.h"
#include "math.h"

struct post_fail_t {
    int ret;
    uint32_t dns;
    uint32_t tcp;
    uint32_t ssl;
    uint32_t write;
    uint32_t read;
};

struct post_data_t {
    uint32_t count_instance; /* create a instance by timer timeout. */
    uint32_t count;          /* per posts, not including APP retry*/
    uint32_t pass_count;
    uint32_t fail_count;
    uint32_t start_time;
    uint32_t end_time;
    uint32_t pass_re_count;  /* post successful with APP retry mechanism or one time ok */
    uint32_t fail_re_count;  /* post fail count with APP retry mechanism */
    struct post_fail_t fail;
};

struct net_connect_time_t {
    uint32_t scan_start;
    uint32_t scan_done;
    uint32_t wifi_connect_done;
    uint32_t got_ip_done;
    uint32_t sntp_done;            /* if needed */
    uint32_t dns_done;
    uint32_t tcp_done;
    uint32_t ssl_done;
    uint32_t http_done;            /* For CKS use */
    uint32_t mqtt_done;            /* For Ali/AWS use */
    uint32_t report_token_start;   /* For Ali use */
    uint32_t report_token_done;    /* For Ali use */
    uint32_t cloud_connect_done;
};

struct net_connect_t {
    int ret;
    struct net_connect_time_t time;
};

struct ats_stats_t {
    struct net_connect_t net;
    struct post_data_t post_data;
};

/* Global variable, add in you main function. */
extern struct ats_stats_t ats_stats;
extern osTimerId ats_post_timer;
extern osTimerId ats_pairing_timer;

/* Auto test timer init */
#define ATS_POST_TIMER_INIT(fn) do { \
    osTimerDef_t timer_test_def; \
    timer_test_def.ptimer = fn; \
    ats_post_timer = osTimerCreate(&timer_test_def, osTimerPeriodic, NULL); \
    if (ats_post_timer == NULL) { \
        printf("ATS: create test timer fail \r\n"); \
    } \
} while (0);

#define ATS_PAIRING_TIMER_INIT(fn) do { \
    osTimerDef_t timer_test_def; \
    timer_test_def.ptimer = fn; \
    ats_pairing_timer = osTimerCreate(&timer_test_def, osTimerOnce, NULL); \
    if (ats_pairing_timer == NULL) { \
        printf("ATS: create test timer fail \r\n"); \
    } \
} while (0);

#define ATS_POST_TIMER_START(ms) osTimerStart(ats_post_timer, ms);
#define ATS_POST_TIMER_STOP osTimerStop(ats_post_timer);
#define ATS_PAIRING_TIMER_START(ms) osTimerStart(ats_pairing_timer, ms);
#define ATS_PAIRING_TIMER_STOP osTimerStop(ats_pairing_timer);

/* Post data counter */
#define ATS_POST_INC(x) ++ats_stats.x;
#define ATS_POST_FAIL_INC(x) ++ats_stats.post_data.x;

#define ATS_POST_SET_RET(x)  ats_stats.post_data.fail.ret = x;
#define ATS_POST_SET_TIME(x) ats_stats.post_data.x = osKernelSysTick();

#define ATS_CLEAN_FAIL_STATS memset(&ats_stats.post_data.fail, 0, sizeof(struct post_fail_t));

#define ATS_PRINT_ONE_LOG do { \
    printf("P= %6d, %6d, %6d, %d, %d, %d, %d, %d, %6d, %x\r\n", \
    ats_stats.post_data.count, ats_stats.post_data.pass_count, ats_stats.post_data.fail_count, \
    ats_stats.post_data.fail.dns, ats_stats.post_data.fail.tcp, ats_stats.post_data.fail.ssl, \
    ats_stats.post_data.fail.write, ats_stats.post_data.fail.read, \
    ats_stats.post_data.end_time - ats_stats.post_data.start_time, \
    ats_stats.post_data.fail.ret \
    ); \
} while(0);

/* For net connect/pairing */
#if 1
#define ATS_NET_SET_TIME(x) \
    ats_stats.net.time.x = osKernelSysTick();
#else
#define ATS_NET_SET_TIME(x) \
    if (ats_stats.net.time.x == 0) ats_stats.net.time.x = osKernelSysTick();
#endif

#define ATS_NET_CLEAN_TIME memset(&ats_stats.net.time, 0, sizeof(struct net_connect_time_t));

#define ATS_NET_SET_RET(x) ats_stats.net.ret = x;

/* For CKS */
#define ATS_PRINT_CKS_FINAL_LOG do { \
    printf("F= %6d, %6d\r\n", ats_stats.post_data.count_instance, \
    ats_stats.post_data.fail_re_count); \
} while (0);

#if 1
#define ATS_PRINT_CKS_PAIRING_LOG do { \
    printf("C= %6d, %6d, %6d, %6d, %6d, %6d, %6d, %6d, %6d\r\n", \
    ats_stats.net.time.scan_done - ats_stats.net.time.scan_start, \
    ats_stats.net.time.wifi_connect_done - ats_stats.net.time.scan_done, \
    ats_stats.net.time.got_ip_done - ats_stats.net.time.wifi_connect_done, \
    ats_stats.net.time.sntp_done - ats_stats.net.time.got_ip_done, \
    ats_stats.net.time.dns_done - ats_stats.net.time.sntp_done, \
    ats_stats.net.time.tcp_done - ats_stats.net.time.dns_done, \
    ats_stats.net.time.ssl_done - ats_stats.net.time.tcp_done, \
    ats_stats.net.time.http_done - ats_stats.net.time.ssl_done, \
    ats_stats.net.time.http_done - ats_stats.net.time.scan_start \
    ); \
} while (0);
#else
#define ATS_PRINT_CKS_PAIRING_LOG do { \
    printf("C= %6d, %6d, %6d, %6d, %6d, %6d, %6d, %6d, %6d\r\n", \
    ats_stats.net.time.scan_done - ats_stats.net.time.scan_start, \
    ats_stats.net.time.wifi_connect_done - ats_stats.net.time.scan_done, \
    ats_stats.net.time.got_ip_done - ats_stats.net.time.wifi_connect_done, \
    ats_stats.net.time.sntp_done - ats_stats.net.time.got_ip_done, \
    ats_stats.net.time.dns_done - ats_stats.net.time.sntp_done, \
    ats_stats.net.time.tcp_done - ats_stats.net.time.dns_done, \
    ats_stats.net.time.ssl_done - ats_stats.net.time.tcp_done, \
    ats_stats.net.time.http_done - ats_stats.net.time.ssl_done, \
    (abs(ats_stats.net.time.scan_done - ats_stats.net.time.scan_start) + \
    abs(ats_stats.net.time.wifi_connect_done - ats_stats.net.time.scan_done) + \
    abs(ats_stats.net.time.got_ip_done - ats_stats.net.time.wifi_connect_done) + \
    abs(ats_stats.net.time.sntp_done - ats_stats.net.time.got_ip_done) + \
    abs(ats_stats.net.time.dns_done - ats_stats.net.time.sntp_done) + \
    abs(ats_stats.net.time.tcp_done - ats_stats.net.time.dns_done) + \
    abs(ats_stats.net.time.ssl_done - ats_stats.net.time.tcp_done) + \
    abs(ats_stats.net.time.http_done - ats_stats.net.time.ssl_done)) \
    ); \
} while (0);
#endif

#endif /* _AUTO_TEST_STATS_H_ */
