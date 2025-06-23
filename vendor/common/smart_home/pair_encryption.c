/********************************************************************************************************
 * @file	pair_encryption.c
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
#if (WIN32 || __PROJECT_MESH_GW_NODE__)
#include "proj_lib/ble/ll/ll.h"
#include "proj_lib/ble/blt_config.h"
#include "proj_lib/sig_mesh/app_mesh.h"
#include "vendor/common/remote_prov.h"
#include "vendor/common/fast_provision_model.h"
#include "vendor/common/app_heartbeat.h"
#include "vendor/common/app_health.h"
#include "vendor/common/directed_forwarding.h"
#include "vendor/common/mesh_ota.h"
#include "vendor/common/lighting_model_LC.h"
#include "vendor/common/op_agg_model.h"
#include "vendor/common/solicitation_rpl_cfg_model.h"
#include "vendor/common/version.h"
#include "proj/mcu/watchdog_i.h"
#include "proj_lib/ble/service/ble_ll_ota.h"
#include "proj_lib/ble/crypt/le_crypto.h"
#include "proj_lib/ble/crypt/aes_ccm.h"
#else
#include "tl_common.h"
//#include "stack/ble/controller/ll/ll.h"
//#include "stack/ble/ble.h"
#include "stack/ble/service/ota/ota.h"
#include "drivers/watchdog.h"
#include "algorithm/aes_ccm/aes_ccm.h"
#include "version_type.h"
#endif
#include "protocol.h"
#include "pair_encryption.h"



/** @addtogroup Mesh_Common
  * @{
  */

const u8 PAIR_REQUEST_DEF_KEY[16]     = {0xb4,0x3c,0x98,0x0e,0x18,0xea,0x8c,0x09,0xcc,0x8f,0x27,0xb2,0x12,0x04,0x3d,0x75}; // crc 16:0x3258
const u8 SMART_HOME_CMD_FACTORY_KEY[16] = {0x4e,0x40,0x64,0x75,0x06,0x83,0xfb,0x5c,0xc9,0xdc,0x1b,0x3a,0x21,0xac,0xa2,0xbf}; // crc 16:0xba16, nid:0x62
const u8 FLASH_KEY_SAVE_ENC_KEY[16]   = {0xab,0x58,0xab,0xcc,0x14,0xa1,0xa5,0x9d,0x63,0x5e,0x39,0xbc,0x16,0x29,0xc9,0x5e};


#if SMART_HOME_SERVER_EN
smart_home_pair_server_proc_t smart_home_pair_server_proc = {0};
sh_server_pair_key_info_t smart_home_pair_key_info[SMART_HOME_SERVER_PAIR_KEY_MAX];
sh_decryption_info_t smart_home_decryption_info = {0};
u8 flag_factory_key_mode = 0;		// use SMART_HOME_CMD_FACTORY_KEY
u8 flag_can_not_pair = 0; 			// pair ok or timeout after power up.


STATIC_ASSERT(sizeof(sh_server_pair_key_info_t)%4 == 0);

u32 get_dec_key_array_index(sh_server_pair_key_info_t *p_dec_key)
{
	u32 index = ((u32)p_dec_key - (u32)smart_home_pair_key_info) / sizeof(sh_server_pair_key_info_t);
	if(index >= ARRAY_SIZE(smart_home_pair_key_info)){
		// index = 0; // has make sure being valid.
	}
	return index;
}
#endif

// -------------- pair TX handle -------------
tx_cmd_err_t smart_home_pair_tx(u8 op, u8 *par, int par_len)
{
	tx_cmd_err_t err = TX_CMD_SUCCESS;
	smart_home_cmd_buf_t cmd = {{0}};
	if((u32)par_len > sizeof(cmd.payload.pair.par)){
		return TX_CMD_ERR_PAR_LEN;
	}
	
	smart_home_adv_payload_t *p_payload = &cmd.payload;
	smart_home_pair_t *p_pair = &cmd.payload.pair;
	p_payload->vendor_id2 = VENDOR_ID_PAIR;
	p_pair->op = op;
	memcpy(p_pair->par, par, par_len);

	// ----
	p_payload->type = GAP_ADTYPE_MANUFACTURER_SPECIFIC;
	p_payload->vendor_id = VENDOR_ID;
	p_payload->len = get_pair_payload_len_val(par_len);
	// ----
	if(PAIR_OP_RESPONSE_MAC == op){
		cmd.cmd_head.par_type = CMD_BUF_HEAD_TYPE_PAIR_RSP_MAC;
	}
	
	LOG_MSG_LIB(TL_LOG_PAIR, (u8 *)p_payload, p_payload->len+1-SIZE_MIC,"%s,op:0x%02x:", __FUNCTION__, op);

	// encryption
	#if (SMART_HOME_CLIENT_APP_EN || SMART_HOME_CLIENT_SW_EN)
	if(SWITCH_MODE_MASTER == switch_mode){
		sh_server_pair_key_info_t dec_key_info = {0};
		#if SMART_HOME_CLIENT_SW_EN
		const u8 *key16 = PAIR_REQUEST_DEF_KEY;	// must PAIR_OP_REQUEST
		#else
		const u8 *key16 = (PAIR_OP_REQUEST == op) ? PAIR_REQUEST_DEF_KEY : smart_home_pair_app_proc.req.key;
		#endif
		smart_home_set_key_info_and_valid(&dec_key_info, key16);
		smart_home_pair_enc(&p_payload->pair, par_len, &dec_key_info);
	}else
	#endif
	{
		smart_home_pair_enc(&p_payload->pair, par_len, &smart_home_pair_key_info[smart_home_pair_server_proc.net_index]);
	}
	
	err = smart_home_cmd_tx_push_fifo(&cmd);
#if WIN32
	rx_cb_err_t err_rx = smart_home_pair_server_rx_cb(p_payload, RX_SOURCE_TX);	// just for VC ring back test
	if(err_rx){
		LOG_MSG_ERR(TL_LOG_PROTOCOL, 0, 0,"VC RX, error No.:0x%x(%s)", err_rx, get_err_number_string_sh(err_rx));
	}else{
		smart_home_pair_client_rx_cb(p_payload, RX_SOURCE_TX, 0);	// just for VC ring back test
	}
#endif
	
	return err;
}

#if 0 // SMART_HOME_CLIENT_APP_EN
tx_cmd_err_t smart_home_pair_tx_request()
{
	sh_pair_req_t req = {{0}};
	
	return smart_home_pair_tx(PAIR_OP_REQUEST, (u8 *)&req, sizeof(req));
}

tx_cmd_err_t smart_home_pair_tx_send_unicast_addr()
{
	sh_pair_send_unicast_addr_t send_unicast = {{0}};
	return smart_home_pair_tx(PAIR_OP_SEND_UNICAST_ADDR, (u8 *)&send_unicast, sizeof(send_unicast));
}

tx_cmd_err_t smart_home_pair_tx_unpair()
{
	return smart_home_pair_tx(PAIR_OP_UNPAIR, 0, 0);
}
#endif

#if SMART_HOME_SERVER_EN
	#if 0
tx_cmd_err_t smart_home_pair_tx_response_mac()
{
	sh_pair_rsp_mac_t rsp_mac = {{0}};
	return smart_home_pair_tx(PAIR_OP_RESPONSE_MAC, (u8 *)&rsp_mac, sizeof(rsp_mac));
}

tx_cmd_err_t smart_home_pair_tx_confirm()
{
	sh_pair_confirm_t confirm = {{0}};
	return smart_home_pair_tx(PAIR_OP_CONFIRM, (u8 *)&confirm, sizeof(confirm));
}
	#endif

static void set_pair_server_st(pair_server_st_t st)
{
	smart_home_pair_server_proc.st = st;
	smart_home_pair_server_proc.st_tick = clock_time();
}

static void smart_home_pair_server_complete(int timeout_flag)
{
	memset(&smart_home_pair_server_proc, 0, sizeof(smart_home_pair_server_proc));	// finished
	flag_can_not_pair = 1;
	LOG_MSG_LIB(TL_LOG_PAIR, 0, 0,"smart home pair success, timeout flag:%d:", timeout_flag);
}
#endif

// ----------------- crypto API --------------------
/*
 * aes_encrypt = tn_aes_128 = aes_ecb_encryption   != aes_att_encryption
 * the best is aes_encrypt without any swap
*/

static inline void aes_ecb_encryption_sh(u8 *key, u8 *plaintext, u8 *encrypted_data)
{
#if (WIN32 || __PROJECT_MESH_GW_NODE__)
	aes_encrypt(key, 16, plaintext, encrypted_data);
#else
	aes_encrypt(key, plaintext, encrypted_data); // aes_encrypt = tn_aes_128 = aes_ecb_encryption
#endif
}

u8 aes_att_encryption_packet_smart_home(u8 *key, u8 *iv, u8 *mic, u8 mic_len, u8 *ps, u16 len)
{
	u8	e[16], r[16];

	///////////// calculate mic ///////////////////////
	memset (r, 0, 16);
	memcpy (r, iv, 8);
	r[8] = (u8)len;
	aes_ecb_encryption_sh (key, r, r);

	for (int i=0; i<len; i++)
	{
		r[i & 15] ^= ps[i];

		if ((i&15) == 15 || i == len - 1)
		{
			aes_ecb_encryption_sh (key, r, r);
		}
	}
	for (int i=0; i<mic_len; i++)
	{
		mic[i] = r[i];
	}

	///////////////// calculate enc ////////////////////////
	memset (r, 0, 16);
	memcpy (r+1, iv, 8);
	for (int i=0; i<len; i++)
	{
		if ((i&15) == 0)
		{
			aes_ecb_encryption_sh (key, r, e);
			r[0]++;
		}
		ps[i] ^= e[i & 15];
	}

	return 1;
}

u8 aes_att_decryption_packet_smart_home(u8 *key, u8 *iv, u8 *mic, u8 mic_len, u8 *ps, u16 len)
{
	u8	e[16], r[16];

	///////////////// calculate enc ////////////////////////
	memset (r, 0, 16);
	memcpy (r+1, iv, 8);
	for (int i=0; i<len; i++)
	{
		if ((i&15) == 0)
		{
			aes_ecb_encryption_sh (key, r, e);
			r[0]++;
		}
		ps[i] ^= e[i & 15];
	}

	///////////// calculate mic ///////////////////////
	memset (r, 0, 16);
	memcpy (r, iv, 8);
	r[8] = (u8)len;
	aes_ecb_encryption_sh (key, r, r);

	for (int i=0; i<len; i++)
	{
		r[i & 15] ^= ps[i];

		if ((i&15) == 15 || i == len - 1)
		{
			aes_ecb_encryption_sh (key, r, r);
		}
	}

	for (int i=0; i<mic_len; i++)
	{
		if (mic[i] != r[i])
		{
			return 0;			//Failed
		}
	}
	return 1;
}


// -------------- pair server RX handle -------------
void smart_home_set_key_info_and_valid(sh_server_pair_key_info_t *p_info, const u8 *key16)
{
	sh_pair_req_t req = {{0}};
	memcpy(req.key, key16, sizeof(req.key));
	smart_home_copy_key_generate_other_info(p_info, &req, sizeof(req));
	p_info->valid = 1;
}

void smart_home_get_factory_key(sh_server_pair_key_info_t *p_out)
{
	smart_home_set_key_info_and_valid(p_out, SMART_HOME_CMD_FACTORY_KEY);
}

#define ENC_PAIR_PAR_OFFSET		(OFFSETOF(smart_home_pair_t, par) - OFFSETOF(smart_home_pair_t, op))
#define PAIR_IV_INIT			{'h','o','m','e','p','a','i','r'}

void smart_home_pair_enc(smart_home_pair_t *p_pair, int par_len, sh_server_pair_key_info_t *p_key)
{
#if SMART_HOME_ENCRYPTION_EN
	u8 iv[8] = PAIR_IV_INIT;
	aes_att_encryption_packet_smart_home(p_key->req.key, iv, p_pair->par+par_len, SIZE_MIC, (u8 *)&p_pair->op, par_len + ENC_PAIR_PAR_OFFSET);
#else
	set_pair_mic_enc(p_pair, par_len, (u8 *)&p_key->key_crc);	// todo
#endif
}

rx_cb_err_t smart_home_pair_dec_packet(smart_home_pair_t *p_pair, int par_len, sh_server_pair_key_info_t *p_dec_key)
{
#if SMART_HOME_ENCRYPTION_EN
	u8 iv[8] = PAIR_IV_INIT;
	if(1 == aes_att_decryption_packet_smart_home(p_dec_key->req.key, iv, p_pair->par+par_len-SIZE_MIC, SIZE_MIC, (u8 *)&p_pair->op, par_len + ENC_PAIR_PAR_OFFSET - SIZE_MIC)){
		return RX_CB_SUCCESS;
	}
#else
	if(p_dec_key->key_crc == get_pair_mic_dec(p_pair, par_len)){
		return RX_CB_SUCCESS;
	}
#endif

	return RX_CB_ERR_DECRYPTION_FAILED;
}

rx_cb_err_t smart_home_pair_dec(smart_home_adv_payload_t *payload, sh_server_pair_key_info_t *p_dec_key)
{
	rx_cb_err_t err = RX_CB_ERR_DECRYPTION_FAILED;
	smart_home_pair_t *p_pair = &payload->pair;
	int pair_par_len = get_pair_para_len(payload);	// include MIC
	smart_home_adv_payload_t adv_copy;
	memcpy(&adv_copy, payload, sizeof(adv_copy));
	
	sh_server_pair_key_info_t def_dec_key = {0};
	smart_home_set_key_info_and_valid(&def_dec_key, PAIR_REQUEST_DEF_KEY);
	err = smart_home_pair_dec_packet(&adv_copy.pair, pair_par_len, &def_dec_key);
	if(RX_CB_SUCCESS == err){
		memcpy(payload, &adv_copy, payload->len + sizeof(payload->len));	// have checked length <= 30 in smart_home_cmd_rx_cb2_
		if(PAIR_OP_REQUEST != p_pair->op){
			return RX_CB_ERR_PAIR_DEF_KEY_BUT_NOT_REQ;
		}
	}else{
		err = smart_home_pair_dec_packet(p_pair, pair_par_len, p_dec_key);
		if(RX_CB_SUCCESS == err){
			if(PAIR_OP_REQUEST == p_pair->op){
				return RX_CB_ERR_PAIR_ERR_REQ;
			}
		}
	}
	return err;
}

#if SMART_HOME_SERVER_EN
int smart_home_unpair_and_save(sh_server_pair_key_info_t *p_info)
{
	memset(p_info, 0, sizeof(sh_server_pair_key_info_t));
	unpair_clear_cache(p_info);
	smart_home_pair_key_save();
	return 0;
}

static void smart_home_pair_success_save(sh_server_pair_key_info_t *p_info)
{
	p_info->valid = 1;
	smart_home_pair_key_save();
	smart_home_led_event_callback(LED_EVENT_PAIR_SUCCESS);
}

void smart_home_pair_get_cps(sh_composition_data_t *p_cps_out)
{
	p_cps_out->vendor_id = VENDOR_ID;
	p_cps_out->product_id = SH_PID;
	p_cps_out->version_id = SH_VID;
}

rx_cb_err_t smart_home_pair_server_rx_cb(smart_home_adv_payload_t *payload, rx_source_type_t source)
{	
	smart_home_pair_t *p_pair = &payload->pair;
	smart_home_pair_server_proc_t *p_proc = &smart_home_pair_server_proc;
	u8 *p_mac_own = blc_ll_get_macAddrPublic();
	int pair_par_len = get_pair_para_len(payload);						// include MIC
	int pair_par_len_no_mic = pair_par_len - SIZE_MIC;	// exclude MIC

	// pair decryption
	rx_cb_err_t err = smart_home_pair_dec(payload, &smart_home_pair_key_info[p_proc->net_index]);	// no pair rx for switch, so always use key of slave.
	if(RX_CB_SUCCESS != err){
		return err;
	}
	
	if(flag_can_not_pair && (PAIR_OP_REQUEST != p_pair->op)){
		#if (!WIN32)
		return RX_CB_ERR_NOT_PAIR_TIME_WINDOW;
		#endif
	}
	
	if(PAIR_OP_REQUEST == p_pair->op){
		if(pair_par_len_no_mic < SIZEOF_MEMBER(sh_pair_req_t, key)){
			return RX_CB_ERR_PAR_LEN;
		}
		
		#if 1
		if(flag_factory_key_mode){
			flag_factory_key_mode = 0;
			sh_server_pair_key_info_t *p_info_1st = &smart_home_pair_key_info[0];
			memset(p_info_1st, 0, sizeof(sh_server_pair_key_info_t));
		}
		
		sh_pair_req_t *p_req = (sh_pair_req_t *)p_pair->par;
		sh_server_pair_key_info_t *p_pair_null = 0;	// the first one of null pair info.
		unsigned int i = 0;
		for(; i < ARRAY_SIZE(smart_home_pair_key_info); ++i){
			sh_server_pair_key_info_t *p_pair_info = &smart_home_pair_key_info[i];
			if(p_pair_info->valid){
				if(0 == memcmp(p_pair_info->req.key, p_req->key, sizeof(p_req->key))){	// the same provisioner
					if(p_pair_info->valid){
						return RX_CB_ERR_HAVE_BEEN_PAIRED;	// no response for the same provisioner
					}
				}
			}else{
				if(NULL == p_pair_null){
					p_pair_null = p_pair_info;
					p_proc->net_index = i;
				}
			}
		}
		
		if(flag_can_not_pair){
			#if (!WIN32)
			return RX_CB_ERR_NOT_PAIR_TIME_WINDOW;	// should be better to report "have been paired" priority.
			#endif
		}
		
		if(NULL == p_pair_null){
			return RX_CB_ERR_OUTOF_MEMORY;
		}
		#endif

		sh_server_pair_key_info_t *p_info = &smart_home_pair_key_info[p_proc->net_index];
		if(PAIR_SERVER_ST_IDLE == p_proc->st){	// copy once is enough.
			smart_home_copy_key_generate_other_info(p_info, p_req, pair_par_len_no_mic);
			LOG_MSG_LIB(TL_LOG_PAIR, (u8 *)p_req, get_pair_para_len(payload),"pair server rx, op:0x%02x, -----------------par:", p_pair->op);
		}

		if(sizeof(sh_pair_req_t) == pair_par_len_no_mic){
			// switch pair mode
			smart_home_pair_server_complete(0);
			lcmd_group_t cmd = {0};
			cmd.set_mode = GROUP_ADD;
			//cmd.ack = 0;	// has been initialized to 0.
			cmd.group_id = p_req->group_id_sw;
			group_get_set_rx_handle(&cmd);	// save group
			
			smart_home_pair_success_save(p_info);
		}else{
			if((PAIR_SERVER_ST_IDLE == p_proc->st)
			 || ((PAIR_SERVER_ST_RSP_MAC == p_proc->st) && is_smart_home_tx_cmd_empty())){
				set_pair_server_st(PAIR_SERVER_ST_RSP_MAC);
				sh_pair_rsp_mac_t rsp = {{0}};
				memcpy(rsp.mac_src, p_mac_own, sizeof(rsp.mac_src));
				smart_home_pair_tx(PAIR_OP_RESPONSE_MAC, (u8 *)&rsp, sizeof(rsp));
			}
		}
	}else if(PAIR_OP_SEND_UNICAST_ADDR == p_pair->op){
		if(pair_par_len_no_mic < sizeof(sh_pair_send_unicast_addr_t)){
			return RX_CB_ERR_PAR_LEN;
		}
		
		sh_pair_send_unicast_addr_t *p_addr = (sh_pair_send_unicast_addr_t *)p_pair->par;
		if(0 == memcmp(p_mac_own, p_addr->mac_dst, sizeof(p_addr->mac_dst))){
			if(PAIR_SERVER_ST_RSP_MAC == p_proc->st){
				sh_server_pair_key_info_t *p_info = &smart_home_pair_key_info[p_proc->net_index];
				p_info->unicast_addr = p_addr->unicast_addr;
				if(!is_factory_reset_version_valid()){
					set_factory_reset_version();
				}

				smart_home_pair_success_save(p_info);
				
				// TODO: save key, unicast address	// save once only.
				u8 r = irq_disable();
				smart_home_cmd_buf_t *p_cmd = (smart_home_cmd_buf_t *)my_fifo_get(&smart_home_tx_cmd_fifo);
				if(p_cmd){
					if(CMD_BUF_HEAD_TYPE_PAIR_RSP_MAC == p_cmd->cmd_head.par_type){
						my_fifo_pop(&smart_home_tx_cmd_fifo);	// no need to send any more to save time. app has received.
					}
				}
				irq_restore(r);
				LOG_MSG_LIB(TL_LOG_PAIR, (u8 *)p_addr, get_pair_para_len(payload),"pair server rx, op:0x%02x, -----------------par:", p_pair->op);
			}
			
			if((PAIR_SERVER_ST_RSP_MAC == p_proc->st)
			 || ((PAIR_SERVER_ST_CONFIRM == p_proc->st) && is_smart_home_tx_cmd_empty())){
				set_pair_server_st(PAIR_SERVER_ST_CONFIRM);
				sh_pair_confirm_t confirm = {{0}};
				memcpy(confirm.mac_src, p_mac_own, sizeof(confirm.mac_src));
				sh_server_pair_key_info_t *p_info = &smart_home_pair_key_info[p_proc->net_index];
				confirm.dev_addr = p_info->unicast_addr;
				smart_home_pair_get_cps(&confirm.cps);
				confirm.factory_reset_ver = factory_reset_version;
				confirm.feature_mask = SMART_HOME_FEATURE_MASK;
				smart_home_pair_tx(PAIR_OP_CONFIRM, (u8 *)&confirm, sizeof(confirm));
			}
		}
	}else if(PAIR_OP_UNPAIR == p_pair->op){
		if(pair_par_len_no_mic < sizeof(sh_pair_unpair_t)){
			return RX_CB_ERR_PAR_LEN;
		}

		//sh_pair_unpair_t *p_unpair = (sh_pair_unpair_t *)p_pair->par;
	}
	
	return RX_CB_SUCCESS;
}

void smart_home_pair_server_loop()
{
	smart_home_pair_server_proc_t *p_proc = &smart_home_pair_server_proc;
	if(flag_can_not_pair){
		return ;
	}
	
	if(PAIR_SERVER_ST_IDLE == p_proc->st){
		u32 tick_start_pair = 0;
		#if SMART_HOME_CLIENT_SW_EN
		tick_start_pair = switch_slave_mode_tick;
		#endif
		if(clock_time_exceed(tick_start_pair, PAIR_TIMEOUT_POWER_UP_MS * 1000)){
			flag_can_not_pair = 1;
		}
		return ;
	}

	if(PAIR_SERVER_ST_RSP_MAC == p_proc->st){
		if(clock_time_exceed(p_proc->st_tick, PAIR_RX_UNICAST_ADDR_TIMEOUT_MS * 1000)){
			// failed to receive PAIR_OP_SEND_UNICAST_ADDR.
			smart_home_pair_server_complete(1);	// init
		}
	}else if(PAIR_SERVER_ST_CONFIRM == p_proc->st){
		if(clock_time_exceed(p_proc->st_tick, PAIR_TX_CONFIRM_TIMEOUT_MS * 1000)){
			// pair success by timeout. app is suppose to have received PAIR_OP_CONFIRM.
			smart_home_pair_server_complete(0);	// init
		}
	}
}
#endif


// --------------- encryption API -------------------
#define tn_aes_128		aes_encrypt

////////////////////////////////////////////////////////////////////////////////////
//		AES_CMAC
/////////////////////////////////////////////////////////////////////////////////////
const unsigned char const_Zero_Rb[20] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
  };

#define		const_Rb			(const_Zero_Rb + 4)

void tn_aes_xor_128(unsigned char *a, unsigned char *b, unsigned char *out)
{
	int i;
	for (i=0;i<16; i++)
	{
		out[i] = a[i] ^ b[i];
	}
}

  /* AES-CMAC Generation Function */

void tn_aes_leftshift_onebit(unsigned char *input,unsigned char *output)
{
	int i;
	unsigned char overflow = 0;
	for ( i=15; i>=0; i-- ) {
		unsigned char in = input[i];
		output[i] = in << 1;
		output[i] |= overflow;
		overflow = (in & 0x80)?1:0;
	}
	return;
}

void tn_aes_padding ( unsigned char *lastb, unsigned char *pad, int length )
{
   int j;
	/* original last block */
   for ( j=0; j<16; j++ ) {
      if ( j < length ) {
         pad[j] = lastb[j];
      } else if ( j == length ) {
         pad[j] = 0x80;
      } else {
              pad[j] = 0x00;
      }
   }
}

void tn_aes_cmac ( unsigned char *key, unsigned char *input, int length,
                  unsigned char *mac )
{
	unsigned char       K[16], L[16], X[16];
   	int         n, i, flag;

   	n = (length+15) / 16;       /* n is number of rounds */

   	if ( n == 0 ) {
       n = 1;
       flag = 0;
   	} else {
       if ( (length%16) == 0 ) { /* last block is a complete block */
           flag = 1;
       } else { /* last block is not complete block */
           flag = 0;
       }

    }

	  //////////////// Generate K1 or K2 => K ///////////////
	tn_aes_128(key,(unsigned char *)const_Zero_Rb,L);

	if ( (L[0] & 0x80) == 0 ) { /* If MSB(L) = 0, then K1 = L << 1 */
		tn_aes_leftshift_onebit(L,K);
	} else {    /* Else K1 = ( L << 1 ) (+) Rb */

		tn_aes_leftshift_onebit(L,K);
		tn_aes_xor_128(K,(unsigned char *)const_Rb,K);
	}

	if (!flag)
	{
		if ( (K[0] & 0x80) == 0 ) {
			tn_aes_leftshift_onebit(K,K);
		} else {
			tn_aes_leftshift_onebit(K,L);
			tn_aes_xor_128(L,(unsigned char *)const_Rb,K);
		}
	}
	  ///////////////////////////////////////////////////////

      if ( flag ) { /* last block is complete block */
          tn_aes_xor_128(&input[16*(n-1)],K,K);
      } else {
          tn_aes_padding(&input[16*(n-1)],L,length%16);
          tn_aes_xor_128(L,K,K);
      }

      for ( i=0; i<16; i++ ) X[i] = 0;
      for ( i=0; i<n-1; i++ ) {
          tn_aes_xor_128(X,&input[16*i],X); /* Y := Mi (+) X  */
          tn_aes_128(key,X,X);      /* X := AES-128(KEY, Y); */
      }

      tn_aes_xor_128(X,K,X);
      tn_aes_128(key,X,X);
	  memcpy (mac, X, 16);
}


int smart_home_sec_func_s1m (unsigned char *s1, char * m)
{
	unsigned char zero[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int n = (int )strlen (m);
	tn_aes_cmac (zero, (unsigned char *)m, n, s1);
	return 0;
}

static inline u8 smart_home_sec_func_k2 (unsigned char * key, unsigned char * p, int np)
{
	unsigned char t[16], s1[16];
	unsigned char tp[32];
	smart_home_sec_func_s1m (s1, "smk2");
	tn_aes_cmac (s1, key, 16, t);

	memcpy (tp, p, np);
	tp[np] = 0x01;
	tn_aes_cmac (t, tp, np + 1, tp);
	return tp[15]; //*nid = tp[15] & 0x7f;
}

int smart_home_sec_func_k3 (unsigned char *k3, unsigned char n[16])
{
	unsigned char t[16];
	smart_home_sec_func_s1m (t, "smk3");
	tn_aes_cmac (t, n, 16, t);
	k3[0] = 'i';
	k3[1] = 'd';
	k3[2] = '6';
	k3[3] = '4';
	k3[4] = 0x01;
	
	tn_aes_cmac (t, k3, 5, t);
	memcpy (k3, t + 8, 8);
	return 0;
}

u8 smart_home_get_nid(u8 *key)
{
    u8 k2_p[1] = {0};
    return smart_home_sec_func_k2(key, k2_p, sizeof(k2_p));
}


void bin2char(u8 *bin, int len, char *out)
{
	int idx = 0;
	for(int i=0;i<len;i++){
		out[idx++] = printf_arrb2t [(bin[i]>>4) & 15];
		out[idx++] = printf_arrb2t [bin[i] & 15];
	}
}

void smart_home_get_networkid_string(char *str_out, u8 *key)
{
	u8 nid[8];// = {0};
	smart_home_sec_func_k3(nid, key);
	bin2char(nid, sizeof(nid), str_out);
}

void smart_home_copy_key_generate_other_info(sh_server_pair_key_info_t *p_info, sh_pair_req_t *p_req, int req_len_no_mic)
{
	memcpy(&p_info->req, p_req, min(sizeof(p_info->req), req_len_no_mic));
	if(req_len_no_mic >= sizeof(sh_pair_req_t)){
		// p_info->req.group_id_sw = SMART_HOME_GROUP_ALL; // take "whether p_info->unicast_addr is 0" as switch flag
	}
	p_info->key_crc = crc16(p_info->req.key, sizeof(p_info->req.key));
	p_info->nid = smart_home_get_nid(p_info->req.key);
}

#define ENC_CMD_PAR_OFFSET		(OFFSETOF(smart_home_nw_t, par) - OFFSETOF(smart_home_nw_t, addr_dst))

static inline void set_smart_home_cmd_iv(u8 *iv, smart_home_nw_t *p_nw)
{
	// PAIR_IV_INIT
	iv[0] = 'c';
	iv[1] = 'm';
	iv[2] = 'd';
	memcpy(iv+3, &p_nw->nid, 5);
}

void smart_home_nw_enc(smart_home_nw_t *p_nw, int par_len, sh_server_pair_key_info_t *p_key)
{
	p_nw->nid = p_key->nid;
#if SMART_HOME_ENCRYPTION_EN
	u8 iv[8];// = {0};
	set_smart_home_cmd_iv(iv, p_nw);
	aes_att_encryption_packet_smart_home(p_key->req.key, iv, p_nw->par+par_len, SIZE_MIC, (u8 *)&p_nw->addr_dst, par_len + ENC_CMD_PAR_OFFSET);
#else
	set_cmd_mic_enc(p_nw, par_len, (u8 *)&p_key->key_crc);	// todo
#endif
}

/*
* ret val: 0 means decryption success.
*/
rx_cb_err_t smart_home_nw_dec(smart_home_nw_t *p_nw, int par_len, sh_server_pair_key_info_t *p_dec_key)
{
#if SMART_HOME_ENCRYPTION_EN
	u8 iv[8];// = {0};
	set_smart_home_cmd_iv(iv, p_nw);
	if(1 == aes_att_decryption_packet_smart_home(p_dec_key->req.key, iv, p_nw->par+par_len-SIZE_MIC, SIZE_MIC, (u8 *)&p_nw->addr_dst, par_len + ENC_CMD_PAR_OFFSET - SIZE_MIC)){
		return RX_CB_SUCCESS;
	}
#else
	if(p_dec_key->key_crc == get_cmd_mic_dec(p_nw, par_len)){
		return RX_CB_SUCCESS;
	}
#endif

	return RX_CB_ERR_DECRYPTION_FAILED;
}

rx_cb_err_t smart_home_cmd_dec(smart_home_nw_t *p_nw, int par_len)
{
	rx_cb_err_t err = RX_CB_ERR_DECRYPTION_NO_KEY_FOUND;
	sh_decryption_info_t *p_dec_info = &smart_home_decryption_info;	// no command rx for master mode of switch, so always use key of slave.
	p_dec_info->p_dec_key = NULL; // init to failed.
	foreach_arr(i, smart_home_pair_key_info){
		sh_server_pair_key_info_t *p_info = &smart_home_pair_key_info[i];
		if(p_info->valid){
			if(p_nw->nid == p_info->nid){
				err = smart_home_nw_dec(p_nw, par_len, p_info);
				if(RX_CB_SUCCESS == err){
					p_dec_info->p_dec_key = p_info;
					return RX_CB_SUCCESS;
				}
			}
		}
	}

	return err;
}

static inline int smart_home_online_st_enc_dec(smart_home_light_online_st_t *p_st, int enc_flag)
{
#if SMART_HOME_ENCRYPTION_EN
	u8 key[16];
	memcpy(key, PAIR_REQUEST_DEF_KEY, sizeof(key));
	u8 iv[8] = {0,0,0,'o','n','l','i','n'};
	u8 *p_vd = (u8 *)&p_st->vendor_id2;
	iv[0] = p_vd[0];
	iv[1] = p_vd[1];
	iv[2] = p_st->sno;
	u8 len = OFFSETOF(smart_home_light_online_st_t, mic) - OFFSETOF(smart_home_light_online_st_t, mac);
	if(enc_flag){
		aes_att_encryption_packet_smart_home(key, iv, p_st->mic, SIZE_MIC, (u8 *)&p_st->mac, len);
		return 0;
	}else{
		return aes_att_decryption_packet_smart_home(key, iv, p_st->mic, SIZE_MIC, (u8 *)&p_st->mac, len);
	}
#endif

	return 0;
}

void smart_home_online_st_enc(smart_home_light_online_st_t *p_st)
{
	smart_home_online_st_enc_dec(p_st, 1);
}

#if (SMART_HOME_CLIENT_APP_EN)
void smart_home_online_st_dec(smart_home_light_online_st_t *p_st)
{
	smart_home_online_st_enc_dec(p_st, 0);
}
#endif

// ------------- smart home switch ----------------
#if SMART_HOME_CLIENT_SW_EN
sh_server_pair_key_info_t smart_home_pair_key_switch = {0};

static inline int is_valid_group_id_sw(u16 group_id)
{
	return ((group_id >= GROUP_ID_SWITCH_START) && (group_id <= GROUP_ID_SWITCH_END));
}

void smart_home_pair_key_switch_init()
{
	sh_server_pair_key_info_t *p_info = &smart_home_pair_key_switch;
	sh_pair_req_t req = {{0}};
	// get key
	u8 *tbl_mac2 = blc_ll_get_macAddrPublic();
	u8 key_swtich[16] = {0};
	memcpy(key_swtich, tbl_mac2, 6);
	u32 r1 = rand();
	memcpy(key_swtich+6, &r1, 4);
	r1 = rand();
	memcpy(key_swtich+10, &r1, 4);
	// LOG_MSG_LIB(TL_LOG_PAIR, (u8 *)key_swtich, sizeof(key_swtich),"sw pair key random:");
	aes_ecb_encryption_sh((u8 *)FLASH_KEY_SAVE_ENC_KEY, key_swtich, key_swtich);
	key_swtich[12] = 'h';
	key_swtich[13] = 'l';
	key_swtich[14] = 's';
	key_swtich[15] = 'w';

	memcpy(req.key, key_swtich, sizeof(req.key));
	req.group_id_sw = GROUP_ID_SWITCH_G_F000;
	smart_home_copy_key_generate_other_info(p_info, &req, sizeof(req));
	p_info->valid = 1;
	// LOG_MSG_LIB(TL_LOG_PAIR, (u8 *)p_info, sizeof(sh_server_pair_key_info_t),"sw pair key init:");
	
	smart_home_pair_key_save();
}

tx_cmd_err_t smart_home_tx_pair_request_switch()
{
	tx_cmd_err_t err = TX_CMD_SUCCESS;
	if(0 == smart_home_pair_key_switch.valid){
		smart_home_pair_key_switch_init();
	}
	
	if(smart_home_pair_key_switch.valid && is_valid_group_id_sw(smart_home_pair_key_switch.req.group_id_sw)){
		err = smart_home_pair_tx(PAIR_OP_REQUEST, (u8 *)&smart_home_pair_key_switch.req, sizeof(smart_home_pair_key_switch.req));
	}else{
		err = TX_CMD_ERR_INVALID_SW_GROUP_ID;
	}

	if(err != TX_CMD_SUCCESS){
		LOG_MSG_LIB(TL_LOG_PAIR, 0, 0,"%s,tx error No.:0x%x(%s)", __FUNCTION__, err, get_err_number_string_sh(err));
	}

	return err;
}

tx_cmd_err_t smart_home_cmd_tx_switch(u8 op, u16 addr_dst, u8 *par, int par_len)
{
	sh_server_pair_key_info_t *p_key_info = &smart_home_pair_key_switch;
	return smart_home_cmd_tx(op, addr_dst, par, par_len, p_key_info);
}

tx_cmd_err_t smart_home_cmd_tx_switch_factory_key(u8 op, u16 addr_dst, u8 *par, int par_len)
{
	sh_server_pair_key_info_t tx_cmd_factory_key_info = {0};
	smart_home_get_factory_key(&tx_cmd_factory_key_info);
	return smart_home_cmd_tx(op, addr_dst, par, par_len, &tx_cmd_factory_key_info);
}

tx_cmd_err_t smart_home_tx_del_node_switch()
{
	tx_cmd_err_t err = TX_CMD_SUCCESS;
	u8 cmd[sizeof(lcmd_del_node_t)] = {0};
	// lcmd_del_node_t *p_cmd = (lcmd_del_node_t *)&cmd;
	// p_cmd->del_mode = DEL_NODE_MODE_NORMAL;
	// p_cmd->ack = 0;
	#if 1
	u16 addr_dst = SMART_HOME_GROUP_ALL;
	#else
	u16 addr_dst = smart_home_pair_key_switch.req.group_id_sw;
	#endif
	err = smart_home_cmd_tx_switch(LIGHT_CMD_DEL_NODE, addr_dst, cmd, sizeof(cmd));

	return err;
}

#endif


/**
  * @}
  */

