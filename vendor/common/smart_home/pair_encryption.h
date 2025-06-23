/********************************************************************************************************
 * @file	pair_encryption.h
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
#else
#include "tl_common.h"
#include "log_module.h"
#endif
#include "light_driver.h"

#if WIN32
#pragma pack(1)
#endif

// pair op code
#define PAIR_OP_UNPAIR				0x00
#define PAIR_OP_REQUEST				0x01
#define PAIR_OP_RESPONSE_MAC		0x02
#define PAIR_OP_SEND_UNICAST_ADDR	0x03
#define PAIR_OP_CONFIRM				0x04
// #define PAIR_OP_CONFIRM_ACK			0x05	// no need, because it will increase the pair time.


typedef struct{
	u8 key[16];
	u16 group_id_sw;	// only for switch. range from GROUP_ID_SWITCH_START to GROUP_ID_SWITCH_END.
}sh_pair_req_t;

typedef struct{
	u8 mac_src[6];
}sh_pair_rsp_mac_t;

typedef struct{
	u8 mac_dst[6];
	u16 unicast_addr;
}sh_pair_send_unicast_addr_t;

typedef struct{
	u16 vendor_id;		// little endianness	// CID, company ID.
	u16 product_id;		// little endianness
	u16 version_id;		// little endianness	// VID,
}sh_composition_data_t;

// ------ feature mask
#define SMART_HOME_FEATURE_MASK		(   SMART_HOME_ONOFF_EN 					<<0		|  \
   	   	   	   	   	   	   	   	   	   	SMART_HOME_CT_EN 						<<1		|  \
   	   	   	   	   	   	   	   	   	   	SMART_HOME_RGB_EN 	   					<<2		|  \
   	   	   	   	   	   	   	   	   	   	SMART_HOME_SCENE_EN 					<<3		|  \
   	   	   	   	   	   	   	   	   	   	SMART_HOME_TIME_EN 						<<4		|  \
   	   	   	   	   	   	   	   	   	   	SMART_HOME_SCHEDULER_EN 		   		<<5		|  \
   	   	   	   	   	   	   	   	   	   	SMART_HOME_ONLINE_ST_EN 				<<6	)

typedef struct {
	u8 server_0onoff   			:1;
	u8 server_1ct   			:1;
	u8 server_2rgb   			:1;
	u8 server_3scece   			:1;
	u8 server_4time   			:1;
	u8 server_5scheduler   		:1;
	u8 server_6online_status	:1;
	u8 rsv						:1;
	u8 rsv2;
}smart_home_feature_mask_t;

typedef struct{
	u8 mac_src[6];
	u16 dev_addr;
	sh_composition_data_t cps;
	u32 factory_reset_ver;
	u16 feature_mask;	// smart_home_feature_mask_t
}sh_pair_confirm_t;

typedef struct{
	u16 dst_addr;
	u8 rsv[6];
}sh_pair_unpair_t;

// ---------------- pair proc ------------
typedef struct{
	u8 valid;
	u8 nid;
	u16 unicast_addr;	// 0 means paired with switch
	u16 key_crc;		// just test now.
	u8 rsv[4];
	sh_pair_req_t req;
}sh_server_pair_key_info_t;

// ---------- packet decryption  ----------
typedef struct{
	sh_server_pair_key_info_t *p_dec_key;	// if NULL, means decryption failed.
}sh_decryption_info_t;


// ----------------- extern --------------
void smart_home_pair_server_loop();
u8 smart_home_get_nid(u8 *key);
void smart_home_copy_key_generate_other_info(sh_server_pair_key_info_t *p_info, sh_pair_req_t *p_req, int req_len_no_mic);
void smart_home_set_key_info_and_valid(sh_server_pair_key_info_t *p_info, const u8 *key16);
int smart_home_unpair_and_save(sh_server_pair_key_info_t *p_info);
u32 get_dec_key_array_index(sh_server_pair_key_info_t *p_dec_key);
void smart_home_pair_key_switch_init();
void smart_home_get_factory_key(sh_server_pair_key_info_t *p_out);
u8 aes_att_encryption_packet_smart_home(u8 *key, u8 *iv, u8 *mic, u8 mic_len, u8 *ps, u16 len);
u8 aes_att_decryption_packet_smart_home(u8 *key, u8 *iv, u8 *mic, u8 mic_len, u8 *ps, u16 len);
void smart_home_pair_get_cps(sh_composition_data_t *p_cps_out);


extern sh_decryption_info_t smart_home_decryption_info;
extern sh_server_pair_key_info_t smart_home_pair_key_switch;
extern const u8 PAIR_REQUEST_DEF_KEY[16];
extern const u8 SMART_HOME_CMD_FACTORY_KEY[16];
extern const u8 FLASH_KEY_SAVE_ENC_KEY[16];
extern u8 flag_factory_key_mode;
extern u8 flag_can_not_pair;
extern const char printf_arrb2t[];

