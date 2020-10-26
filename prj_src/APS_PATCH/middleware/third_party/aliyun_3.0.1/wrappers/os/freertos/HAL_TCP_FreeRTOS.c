/*
 * Copyright (c) 2014-2016 Alibaba Group. All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */



#include <stdio.h>
#include <string.h>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "iotx_hal_internal.h"
#include "coap_wrapper.h"
#include "infra_config.h"
#include "lwip/netif.h"

#if ALI_AUTO_TEST
#include "auto_test_stats.h"
#include "mbedtls/net_sockets.h"
#endif

#define hal_emerg(...)      //HAL_Printf("[prt] "), HAL_Printf(__VA_ARGS__), HAL_Printf("\r\n")
#define hal_crit(...)       //HAL_Printf("[prt] "), HAL_Printf(__VA_ARGS__), HAL_Printf("\r\n")
#define hal_err(...)        //HAL_Printf("[prt] "), HAL_Printf(__VA_ARGS__), HAL_Printf("\r\n")
#define hal_warning(...)    //HAL_Printf("[prt] "), HAL_Printf(__VA_ARGS__), HAL_Printf("\r\n")
#define hal_info(...)       //HAL_Printf("[prt] "), HAL_Printf(__VA_ARGS__), HAL_Printf("\r\n")
#define hal_debug(...)      //HAL_Printf("[prt] "), HAL_Printf(__VA_ARGS__), HAL_Printf("\r\n")

static uint64_t _platform_get_time_ms(void)
{
    return HAL_UptimeMs();
}

static uint64_t _platform_time_left(uint64_t t_end, uint64_t t_now)
{
    uint64_t t_left;

    if (t_end > t_now) {
        t_left = t_end - t_now;
    } else {
        t_left = 0;
    }

    return t_left;
}

SHM_DATA uintptr_t HAL_TCP_Establish(const char *host, uint16_t port)
{
    struct addrinfo hints;
    struct addrinfo *addrInfoList = NULL;
    struct addrinfo *cur = NULL;
    int fd = 0;
    int rc = -1;
    char service[6];
    int sockopt = 1;

    memset(&hints, 0, sizeof(hints));

    hal_info("establish tcp connection with server(host=%s port=%u)", host, port);

    hints.ai_family = AF_INET; /* only IPv4 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    sprintf(service, "%u", port);

    if ((rc = getaddrinfo(host, service, &hints, &addrInfoList)) != 0) {
        hal_err("getaddrinfo error");

        #if (ALI_AUTO_TEST == 2)
        ATS_NET_SET_RET(-MBEDTLS_ERR_NET_UNKNOWN_HOST)
        #endif

        #if (ALI_AUTO_TEST == 3)
        ATS_POST_FAIL_INC(fail.dns)
        ATS_POST_SET_RET(-MBEDTLS_ERR_NET_UNKNOWN_HOST)
        #endif
        
        return (uintptr_t)(-1);
    }

    #if (ALI_AUTO_TEST == 2)
    ATS_NET_SET_TIME(dns_done)
    printf("dns_done[%u]\n", ats_stats.net.time.dns_done);
    #endif

    for (cur = addrInfoList; cur != NULL; cur = cur->ai_next) {
        if (cur->ai_family != AF_INET) {
            hal_err("socket type error");
            rc = -1;
            continue;
        }

        fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
        if (fd < 0) {
            hal_err("create socket error");
            rc = -1;
            continue;
        }

#if 1
#define TCP_LOCAL_PORT_RANGE_START        0xc000
#define TCP_LOCAL_PORT_RANGE_END          0xffff
#define TCP_ENSURE_LOCAL_PORT_RANGE(port) ((u16_t)(((port) & ~TCP_LOCAL_PORT_RANGE_START) + TCP_LOCAL_PORT_RANGE_START))

        struct sockaddr_in local_addr = {0};
        struct netif *iface = netif_find("st1");

        srand(reg_read(0x40003044));
        
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(TCP_ENSURE_LOCAL_PORT_RANGE((u32_t)rand()));
        local_addr.sin_addr.s_addr = iface->ip_addr.u_addr.ip4.addr;
        if (bind(fd, (struct sockaddr*)&local_addr, sizeof(local_addr)) != 0) {
            printf("bind failed\n");
        }
        
        printf("local_addr.sin_port=%d\n", local_addr.sin_port);
#endif
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
        setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &sockopt, sizeof(sockopt));

#if 1
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        if ( connect( fd, cur->ai_addr, cur->ai_addrlen ) == 0 ) {
            rc = fd; // connected!
            printf("\nHAL_TCP_Establish----fd=%d\n", fd);
            //ret = 0;
            fcntl( fd, F_SETFL, fcntl( fd, F_GETFL, 0 ) & ~O_NONBLOCK );

            #if (ALI_AUTO_TEST == 2)
            ATS_NET_SET_TIME(tcp_done)
            printf("tcp_done[%u]\n", ats_stats.net.time.tcp_done);
            ATS_NET_SET_TIME(ssl_done)
            printf("ssl_done[%u]\n", ats_stats.net.time.ssl_done);
            #endif
            
            break;
        } else {
            if (errno == EINPROGRESS) {
                fd_set rfds, wfds;
                struct timeval tv;
                
                FD_ZERO(&rfds);
                FD_ZERO(&wfds);
                FD_SET(fd, &rfds);
                FD_SET(fd, &wfds);
                
                tv.tv_sec = ALI_NET_CONNECT_TIMEOUT;
                tv.tv_usec = 0;
                //fcntl( fd, F_SETFL, fcntl( fd, F_GETFL, 0 ) & ~O_NONBLOCK );
                int selres = select(fd + 1, &rfds, &wfds, NULL, &tv);

                if (selres > 0) {
                    //if (FD_ISSET(fd, &rfds) || FD_ISSET(fd, &wfds))
                    if (FD_ISSET(fd, &wfds)) {
                        printf("\nHAL_TCP_Establish----fd=%d\n", fd);
                        rc = fd; // connected!
                        //ret = 0;
                        fcntl( fd, F_SETFL, fcntl( fd, F_GETFL, 0 ) & ~O_NONBLOCK );

                        #if (ALI_AUTO_TEST == 2)
                        ATS_NET_SET_TIME(tcp_done)
                        printf("tcp_done[%u]\n", ats_stats.net.time.tcp_done);
                        ATS_NET_SET_TIME(ssl_done)
                        printf("ssl_done[%u]\n", ats_stats.net.time.ssl_done);
                        #endif
            
                        break;
                    }
                }
            }
        }
#else
        if (connect(fd, cur->ai_addr, cur->ai_addrlen) == 0) {
            printf("\n----fd=%d\n", fd);
            rc = fd;
            break;
        }
#endif
        #if (ALI_AUTO_TEST == 2)
        ATS_NET_SET_RET(-MBEDTLS_ERR_NET_CONNECT_FAILED)
        #endif

        #if (ALI_AUTO_TEST == 3)
        ATS_POST_FAIL_INC(fail.tcp)
        ATS_POST_SET_RET(-MBEDTLS_ERR_NET_CONNECT_FAILED)
        #endif

        close(fd);
        hal_err("connect error");
        rc = -1;
    }

    if (-1 == rc) {
        hal_err("fail to establish tcp");
    } else {
        hal_info("success to establish tcp, fd=%d", rc);
    }
    freeaddrinfo(addrInfoList);

    return (uintptr_t)rc;
}

int HAL_TCP_Destroy(uintptr_t fd)
{
    int rc;

    hal_err("HAL_TCP_Destroy: fd[%d]", fd);

    /* Shutdown both send and receive operations. */
    rc = shutdown((int) fd, 2);
    if (0 != rc) {
        hal_err("shutdown error: fd[%d] rc[%d] %s", fd, rc, strerror(errno));
        //return -1;
    }

    rc = close((int) fd);
    if (0 != rc) {
        hal_err("closesocket error: fd[%d] rc[%d] %s", fd, rc, strerror(errno));
        return -1;
    }

    return 0;
}

int32_t HAL_TCP_Write(uintptr_t fd, const char *buf, uint32_t len, uint32_t timeout_ms)
{
    int ret;
    uint32_t len_sent;
    uint64_t t_end, t_left;
    fd_set sets;
    int net_err = 0;

    t_end = _platform_get_time_ms() + timeout_ms;
    len_sent = 0;
    ret = 1; /* send one time if timeout_ms is value 0 */

    do {
        t_left = _platform_time_left(t_end, _platform_get_time_ms());

        if (0 != t_left) {
            struct timeval timeout;

            FD_ZERO(&sets);
            FD_SET(fd, &sets);

            timeout.tv_sec = t_left / 1000;
            timeout.tv_usec = (t_left % 1000) * 1000;

            ret = select(fd + 1, NULL, &sets, NULL, &timeout);
            if (ret > 0) {
                if (0 == FD_ISSET(fd, &sets)) {
                    hal_err("Should NOT arrive");
                    /* If timeout in next loop, it will not sent any data */
                    ret = 0;
                    continue;
                }
            } else if (0 == ret) {
                hal_err("select-write timeout: fd[%d] %s", fd, strerror(errno));
                break;
            } else {
                //if (EINTR == errno) {
                //    hal_err("EINTR be caught");
                //    continue;
                //}

                hal_err("select-write fail");
                net_err = 1;

                #if (ALI_AUTO_TEST == 3)
                ATS_POST_FAIL_INC(fail.write)
                ATS_POST_SET_RET(ATS_SOCK_WRITE_FAILED)
                #endif
                
                break;
            }
        }

        if (ret > 0) {
            ret = send(fd, buf + len_sent, len - len_sent, 0);
            if (ret > 0) {
                len_sent += ret;
            } else if (0 == ret) {
                hal_err("No data be sent");
            } else {
                //if (EINTR == errno) {
                //    hal_err("EINTR be caught");
                //    continue;
                //}

                hal_err("send fail: fd[%d] ret[%d] %s", fd, ret, strerror(errno));
                net_err = 1;

                #if (ALI_AUTO_TEST == 3)
                ATS_POST_FAIL_INC(fail.write)
                ATS_POST_SET_RET(ATS_SOCK_WRITE_FAILED)
                #endif
                
                break;
            }
        }
    } while (!net_err && (len_sent < len) && (_platform_time_left(t_end, _platform_get_time_ms()) > 0));


    if (net_err) {
        return -1;
    } else {
    return len_sent;
    }
}

int32_t HAL_TCP_Read(uintptr_t fd, char *buf, uint32_t len, uint32_t timeout_ms)
{
    int ret, err_code;
    uint32_t len_recv;
    uint64_t t_end, t_left;
    fd_set sets;
    struct timeval timeout;

    t_end = _platform_get_time_ms() + timeout_ms;
    len_recv = 0;
    err_code = 0;

    do {
        t_left = _platform_time_left(t_end, _platform_get_time_ms());
        if (0 == t_left) {
            break;
        }
        FD_ZERO(&sets);
        FD_SET(fd, &sets);

        timeout.tv_sec = t_left / 1000;
        timeout.tv_usec = (t_left % 1000) * 1000;

        ret = select(fd + 1, &sets, NULL, NULL, &timeout);
        if (ret > 0) {
            ret = recv(fd, buf + len_recv, len - len_recv, 0);
            if (ret > 0) {
                len_recv += ret;
            } else if (0 == ret) {
                hal_err("connection is closed");
                err_code = -1;
                break;
            } else {
                //if (EINTR == errno) {
                //    hal_err("EINTR be caught");
                //    continue;
                //}
                hal_err("recv fail: fd[%d] ret[%d] %s", fd, ret, strerror(errno));
                err_code = -2;

                #if (ALI_AUTO_TEST == 3)
                ATS_POST_FAIL_INC(fail.read)
                ATS_POST_SET_RET(ATS_SOCK_READ_FAILED)
                #endif
                
                break;
            }
        } else if (0 == ret) {
            break;
        } else {
            //if (EINTR == errno) {
            //    continue;
            //}
            hal_err("select-recv fail: fd[%d] ret[%d] %s", fd, ret, strerror(errno));
            err_code = -2;

            #if (ALI_AUTO_TEST == 3)
            ATS_POST_FAIL_INC(fail.read)
            ATS_POST_SET_RET(ATS_SOCK_READ_FAILED)
            #endif
            break;
        }
    } while ((len_recv < len));

    /* priority to return data bytes if any data be received from TCP connection. */
    /* It will get error code on next calling */
    return (0 != len_recv) ? len_recv : err_code;
}
