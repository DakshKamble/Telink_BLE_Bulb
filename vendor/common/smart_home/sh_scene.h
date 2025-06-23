/********************************************************************************************************
 * @file	sh_scene.h
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


#define MAX_SCENE_CNT   			16
#define DEFAULT_EXISTED_SCENE_CNT   4

#define INVALID_SCENE_ID   			0	// id 0 is invalid.
#define SCENE_ID_MAX   				245	// id 246~255 are reserved.


rx_cb_err_t scene_set_rx_cb(smart_home_nw_t *p_nw);
rx_cb_err_t scene_call_rx_cb(smart_home_nw_t *p_nw);
rx_cb_err_t scene_id_list_get_rx_cb(smart_home_nw_t *p_nw);
rx_cb_err_t scene_add_no_par_rx_cb(smart_home_nw_t *p_nw);
void set_current_status2scene(lcmd_scene_set_t *p_set, u8 scene_id);
void smart_home_scene_save();
int smart_home_scene_retrieve();


#if 0
extern u8 scene_find(u8 id);
extern u8 scene_add(scene_t * data);
extern u8 scene_del(u8 id);
extern u8 scene_load(u8 id);

void scene_handle();
int is_scene_poll_notify_busy();
void scene_poll_notify_init(u8 need_bridge);
int scene_poll_notify(u8 *pkt_rsp);
void scene_rsp_mesh();
int scene_get_by_id(u8 *pkt_rsp, u8 id);
int scene_get_all_id(u8 *pkt_rsp);
#endif

