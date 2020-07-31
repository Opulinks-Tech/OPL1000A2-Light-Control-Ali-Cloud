/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#ifndef __AWSS_DEV_RESET_H__
#define __AWSS_DEV_RESET_H__

#include <stdio.h>
#include "infra_compat.h"

#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif
    
int awss_report_reset_to_cloud(iotx_vendor_dev_reset_type_t *reset_type);
int awss_report_reset(iotx_vendor_dev_reset_type_t *reset_type);
int awss_check_reset(iotx_vendor_dev_reset_type_t *reset_type);
int awss_stop_report_reset(void);
int awss_handle_reset_cloud_reply(void);
#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
}
#endif
#endif
