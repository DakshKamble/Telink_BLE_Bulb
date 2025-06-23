/********************************************************************************************************
 * @file	sh_time.h
 *
 * @brief	for TLSR chips
 *
 * @author	Mesh Group
 * @date	2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#pragma once

#if (WIN32 || __PROJECT_MESH_GW_NODE__)
#include "proj/tl_common.h"
#include "proj_lib/ble/ll/ll.h"
#include "proj_lib/ble/blt_config.h"
#include "proj_lib/sig_mesh/app_mesh.h"
#include "pair_encryption.h"
#include "app_and_gw/protocol_app_gw.h"
#else
#include "tl_common.h"
//#include "stack/ble/controller/ll/ll.h"
//#include "stack/ble/ble.h"
//#include "stack/ble/blt_config.h"
#include "log_module.h"
#endif
#include "protocol.h"

#define TIME_PERIOD_PUBLISH_EN				1
#if TIME_PERIOD_PUBLISH_EN
#define TIME_PERIOD_PUBLISH_INTERVAL_S		(30)
#endif

typedef struct{
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
    u8 week;
    u32 tick_last;  //if add element, must after tick_last
}rtc_t;

#define TIME_SET_PAR_LEN		(OFFSETOF(rtc_t, week))

extern rtc_t rtc;

rx_cb_err_t time_set_rx_cb(smart_home_nw_t *p_nw);
tx_cmd_err_t time_tx_rsp(u16 addr_dst, u8 st);
rx_cb_err_t time_status_rx_cb(smart_home_nw_t *p_nw);
void rtc_run();
void alarm_event_check();
void rtc_set_week();
int is_rtc_earlier(const rtc_t *rtc_new, const rtc_t *rtc_old);
int is_need_sync_time();
int is_current_time_valid();
void rtc_increase_and_check_event();


