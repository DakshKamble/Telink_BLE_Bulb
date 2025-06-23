/********************************************************************************************************
 * @file	protocol.c
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
#else
#include "stack/ble/ble.h"
//#include "stack/ble/blt_config.h"
#include "stack/ble/service/ota/ota.h"
#include "drivers/watchdog.h"
	#if __PROJECT_B80_BLE_LIGHT__
#include "vendor/8208_ble_light/app_att.h"
	#elif(__PROJECT_B80_BLE_SWITCH__)
#include "vendor/8208_ble_switch/app_att.h"	
	#else
// TODO: TO get SPP_PAIR_DP_H
	#endif
#endif
#include "protocol.h"
#include "sh_scene.h"
#include "sh_time.h"
#include "sh_scheduler.h"



/** @addtogroup Mesh_Common
  * @{
  */

MYFIFO_INIT(smart_home_tx_cmd_fifo, sizeof(smart_home_cmd_buf_t), SMART_HOME_TX_CMD_FIFO_CNT);
#if (FEATURE_RELAY_EN)
MYFIFO_INIT(mesh_adv_fifo_relay, sizeof(mesh_relay_buf_t), MESH_ADV_BUF_RELAY_CNT);
#endif

u16 smart_home_tx_cmd_sno = 1;
light_st_delay_t g_light_st_delay; // delay with parameter of onoff.

#if SMART_HOME_SERVER_EN
u16 smart_home_group_id[SMART_HOME_GROUP_CNT_MAX];
#endif

STATIC_ASSERT(sizeof(smart_home_cmd_buf_t) % 4 == 0);
STATIC_ASSERT(SIZE_MIC == 2);
//STATIC_ASSERT((TX_CMD_INTERVAL_10MS >= 1) && (TX_CMD_INTERVAL_10MS <= 256)); 	// because of u8
STATIC_ASSERT((TX_CMD_PACKET_CNT >= 1) && (TX_CMD_PACKET_CNT <= 256));			// because of u8
STATIC_ASSERT(sizeof(smart_home_nw_t) == sizeof(smart_home_pair_t));
STATIC_ASSERT(SMART_HOME_MTU_SIZE >= sizeof(smart_home_nw_t));
STATIC_ASSERT((sizeof(lcmd_group_id_list_t) == SIZEOF_MEMBER(smart_home_nw_t,par))||(sizeof(lcmd_group_id_list_t)+1 == SIZEOF_MEMBER(smart_home_nw_t,par)));
STATIC_ASSERT(sizeof(lcmd_scene_id_list_t) == SIZEOF_MEMBER(smart_home_nw_t,par));
STATIC_ASSERT(CONNECTABLE_ADV_INV_MS > 2 * TX_CMD_INTERVAL_MAX); // MY_ADV_INTERVAL_MAX // because need to send onine status message.
STATIC_ASSERT(sizeof(smart_home_adv_info_t) == 16); //because use UUID type to indicate.
STATIC_ASSERT(SNO_CACHE_CNT_MAX >= SMART_HOME_SERVER_PAIR_KEY_MAX);
STATIC_ASSERT(sizeof(smart_home_feature_mask_t) == 2);
STATIC_ASSERT(sizeof(sh_pair_confirm_t) <= 22);//(OFFSET_PAIR_PAYLOAD_PARA + sizeof(sh_pair_confirm_t) + SIZE_MIC <= ADV_MAX_DATA_LEN);
#if (FLASH_SIZE_OPTION == FLASH_SIZE_OPTION_128K)
STATIC_ASSERT(APP_SECURITY_ENABLE == 0); // because not define room in flash map to save smp key.
#endif

static inline int is_valid_adv_type(u8 adv_type)
{
	return ((LL_TYPE_ADV_NONCONN_IND == adv_type)||(LL_TYPE_ADV_IND == adv_type)||(LL_TYPE_ADV_SCAN_IND == adv_type)); // LL_TYPE_ADV_SCAN_IND reserve for android
}

#if ((MCU_CORE_TYPE == MCU_CORE_8208))
#else
u8 blt_ll_getCurrentBLEState(void)
{
	return ble_state;
}
#endif

#if 1
#if FEATURE_RELAY_EN
int my_fifo_push_relay_ll (my_fifo_t *f, smart_home_cmd_buf_t *p_in, u8 n, u8 ow)    // ow: over_write
{
	if (n > f->size){
		return -1;
	}
	
	if (((f->wptr - f->rptr) & 255) >= f->num)
	{
	    if(ow){
	        my_fifo_pop(f);
	    }else{
		    return -1;
		}
	}

    u32 r = irq_disable();
	mesh_relay_buf_t *pd = (mesh_relay_buf_t *)(f->p + (f->wptr & (f->num-1)) * f->size);
    memcpy (&pd->bear, p_in, n);
	pd->tick_relay = 0;	// ready to relay.
	pd->pop_later_flag = 0; // must clear
	f->wptr++;			// should be last for VC
    irq_restore(r);

    return 0;
}

mesh_relay_buf_t * my_fifo_get_relay(my_fifo_t *f)
{
    mesh_relay_buf_t *p_relay = 0;
    
	//u32 r = irq_disable(); // no need irq disable, because it do not matter even though push relay in irq state. and adv prepare handle will be called in irq state.
	for (u8 i = 0; (u8)(f->rptr + i) != f->wptr; i++)
	{
		mesh_relay_buf_t *p_temp = (mesh_relay_buf_t *)(f->p + ((f->rptr + i) & (f->num-1)) * f->size);
        if((0 == p_temp->pop_later_flag) && (0 == p_temp->tick_relay)){ // is_relay_transmit_ready // can not check (transmit count == 0) here, because 0 means need to tx once here, not tx completed. especially for security beacon.
            p_relay = p_temp;
            break;
        }
	}
	//irq_restore(r);
	
	return p_relay;
}

void my_fifo_poll_relay(my_fifo_t *f)
{
	//u32 r = irq_disable(); // no need irq disable, because it do not matter even though push relay in irq state. and adv prepare handle will be called in irq state.
	int pop_flag = 0;
	for (u8 i = 0; (u8)(f->rptr + i) != f->wptr; i++)
	{
		mesh_relay_buf_t *p_relay = (mesh_relay_buf_t *)(f->p + ((f->rptr + i) & (f->num-1)) * f->size);
		
		if(p_relay->pop_later_flag){
			if(i == 0){
        		pop_flag = 1;   // happen when can not pop in relay_adv_prepare_handler_ at relay complete. usually happen at cases of relay transmit parameter change or security beacon with only tx one time.
			}else{
				// do nothing, happen when relay completed, but can not pop now, and will pop in next poll relay.
			}
		}else{
			if(p_relay->tick_relay && clock_time_exceed(p_relay->tick_relay, (p_relay->bear.cmd_head.transmit_interval * 10 + 5) * 1000)){ // +5ms instead of invl_steps+1 is better for relay, because blt_advExpectTime += blta.adv_interval in blt_send_adv(), and app_advertise_prepare_handler_() interval maybe higher/lower than 10ms(). 
				p_relay->tick_relay = 0; // means time ready.
			}
		}
	}
	
	if(pop_flag){
        f->rptr++; // no need to be protected by irq disable, because my_fifo_poll_relay_ is called only one time, and it is called at the same function with pop buffer.
	}
	//irq_restore(r);
}

#define FULL_RELAY_PROC_ENABLE	1
int relay_adv_prepare_handler(rf_packet_adv_t * p, int rand_en)  // no random for relay transmit
{
    my_fifo_t *p_fifo = &mesh_adv_fifo_relay;
    my_fifo_poll_relay(p_fifo);   // must before get buffer.
    mesh_relay_buf_t *p_relay = my_fifo_get_relay(p_fifo);
    if(p_relay){ // means that need to send packet now.
		p_relay->tick_relay = clock_time() | 1; // record tick for next transmit for both transmit count is not 0 and 0.		
		
        int ret = 0;
        ret = smart_home_cmd_adv_set((u8 *)p, (u8 *)&p_relay->bear.payload);

       	if(0 == p_relay->bear.cmd_head.transmit_cnt){
       		// must keep tick_relay to be not 0, because my_fifo_get_relay_ can not check transmit count to get if need tx relay, but just can check tick is not 0.
            if((u8 *)p_relay == my_fifo_get(p_fifo)){ // means head of fifo
				// tick_relay can not set to 0 when pop buffer, because in my_fifo_poll_relay(): (transmit count == 0 && tick relay == 0) means one packet is ready to be sent, and (transmit count == 0 && tick relay != 0) means relay completed.
				my_fifo_pop(p_fifo);
			}else{
				p_relay->pop_later_flag = 1; // can not pop here, and will pop in my_fifo_poll_relay_ later, because it is not head of fifo. usually for security beacon packet.
			}
        }else{
        	// tick_relay has been set to (clock time | 1) before.
 			p_relay->bear.cmd_head.transmit_cnt--;
        }
        return ret;
    }

    return 0;
}

int my_fifo_push_relay (smart_home_cmd_buf_t *p_cmd)    // ow: over_write
{
    p_cmd->cmd_head.transmit_cnt = TX_CMD_TRANSMIT_CNT;
    p_cmd->cmd_head.transmit_interval = 0; // adv interval is equal to command interval

    return my_fifo_push_relay_ll((my_fifo_t *)&mesh_adv_fifo_relay, (smart_home_cmd_buf_t *)p_cmd, get_smart_home_cmd_buf_len(&p_cmd->payload), 0);
}
#endif

static u32 tick_adv_ind = 0;
static u32 tick_adv_ind_interval_us = 0;

static void set_connectable_adv_interval_with_random()
{
	u32 rand_ms = rand() % (CONNECTABLE_ADV_INV_RANDOM_MS + 6);
	#if 0
	if(0 == tick_adv_ind_interval_us)
	{
		LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"rand ms:%d--", rand_ms);
	}
	#endif
	
	tick_adv_ind_interval_us = (CONNECTABLE_ADV_INV_MS + rand_ms) * 1000;
}
#endif

#if SMART_HOME_ONOFF_EN
u8 get_target_value(u32 mode, u8 current, u8 adjust_value, u8 min, u8 max)
{
	int target_lum = 0;
	if(LIGHT_SET_MODE_TARGET == mode){
		target_lum = adjust_value;
	}else if(LIGHT_SET_MODE_INCREASE == mode){
		target_lum = current + adjust_value;
	}else if(LIGHT_SET_MODE_DECREASE == mode){
		target_lum = current - adjust_value;
	}else{
		target_lum = current;
	}
	
	if(target_lum > max){
		target_lum = max;
	}else if(target_lum < min){
		target_lum = min;
	}

	return (u8)target_lum;
}

u8 get_target_value_lightness(u32 mode, u8 current, u8 adjust_value)
{
	return get_target_value(mode, current, adjust_value, SH_LIGHTNESS_MIN, SH_LIGHTNESS_MAX);
}

u8 get_target_value_CT(u32 mode, u8 current, u8 adjust_value)
{
	return get_target_value(mode, current, adjust_value, SH_CT_MIN, SH_CT_MAX);
}

u8 get_target_value_RGB(u32 mode, u8 current, u8 adjust_value)
{
	return get_target_value(mode, current, adjust_value, 0, 255);
}
#endif

// -------------- TX handle -------------
/*
different between my_fifo_push and my_fifo_push_adv:

my_fifo_push:          no len in data, add len when push.
my_fifo_push_adv: have len in data,  just use for mesh_cmd_bear.
*/
int my_fifo_push_adv (my_fifo_t *f, u8 *p, u8 n, u8 ow)    // ow: over_write
{
	if (n > f->size)
	{
		return -1;
	}
	
	if (((f->wptr - f->rptr) & 255) >= f->num)
	{
	    if(ow){
	        my_fifo_pop(f);
	    }else{
		    return -1;
		}
	}
	
	u8 r = irq_disable();
	u8 *pd = f->p + (f->wptr & (f->num-1)) * f->size;
	//*pd++ = n;
	//*pd++ = n >> 8;
	memcpy (pd, p, n);
	f->wptr++;			// should be last for VC
	irq_restore(r);
	return 0;
}

int is_smart_home_tx_cmd_empty()
{
	return (0 == my_fifo_get(&smart_home_tx_cmd_fifo));
}

u8 my_fifo_data_cnt_get (my_fifo_t *f)
{
	return (f->wptr - f->rptr);
}

#if FEATURE_RELAY_EN
int is_smart_home_tx_relay_cmd_no_full()
{
	return (my_fifo_data_cnt_get(&mesh_adv_fifo_relay) < MESH_ADV_BUF_RELAY_CNT);
}
#endif

tx_cmd_err_t smart_home_cmd_tx_push_fifo(smart_home_cmd_buf_t *p_cmd)
{
	int push_err = 0;

#if !WIN32
	if(is_blt_gatt_connected()){
		u16 attHandle = (VENDOR_ID_PAIR == p_cmd->payload.vendor_id2) ? SPP_PAIR_DP_H : SPP_SERVER_TO_CLIENT_DP_H;
		int gatt_payload_len = p_cmd->payload.len - ADV_LEN_GATT_LEN_DELTA;
		push_err = blc_gatt_pushHandleValueNotify (BLS_CONN_HANDLE, attHandle, (u8 *)&p_cmd->payload.nw, gatt_payload_len);
	}// else // report by boty ADV and GATT when connected.
#endif
	{
		#if 0
		if(VENDOR_ID_PAIR == p_cmd->payload.vendor_id2){
			p_cmd->cmd_head.transmit_cnt = TX_PAIR_TRANSMIT_CNT;
			p_cmd->cmd_head.transmit_interval = TX_PAIR_INTERVAL_CNT;
		}else
		#endif
		{
			p_cmd->cmd_head.transmit_cnt = TX_CMD_TRANSMIT_CNT;
			#if SMART_HOME_GATEWAY_EN
			p_cmd->cmd_head.transmit_interval = TX_CMD_INTERVAL_CNT; // adv interval is 10ms
			#else
			p_cmd->cmd_head.transmit_interval = 0; // adv interval is equal to command interval
			#endif
		}

		push_err |= my_fifo_push_adv(&smart_home_tx_cmd_fifo, (u8 *)p_cmd, get_smart_home_cmd_buf_len(&p_cmd->payload), 0);
	}
	
	if(push_err){
		LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"push fifo error:0x%x", push_err);
	}
	
	return push_err ? TX_CMD_ERR_FIFO_FULL : TX_CMD_SUCCESS;
}

static u16 smart_home_get_device_addr(sh_server_pair_key_info_t *p_info)
{
	u16 addr = p_info->unicast_addr;
	if(0 == addr){	// from switch or unprovision state.
		u8 *tbl_mac2 = blc_ll_get_macAddrPublic();
        #if SMART_HOME_CLIENT_SW_EN
        addr = tbl_mac2[0] + (((tbl_mac2[1] & 0x7f) | 0x40) << 8);
        #else
		addr = tbl_mac2[0] + ((tbl_mac2[1] & 0x3f) << 8);
        #endif
		if(0 == addr){
			addr = 0x7fff; // 1 is usually for provisioner
		}
	}
	return addr;
}

static inline int is_publish_time_status_message(u8 op, u16 addr_dst)
{
	return ((LIGHT_CMD_TIME_STATUS == op) && (SMART_HOME_GROUP_ALL == addr_dst));
}

static inline tx_cmd_err_t smart_home_cmd_tx_2(u8 op, u16 addr_dst, u8 *par, int par_len, sh_server_pair_key_info_t *p_key)
{
	tx_cmd_err_t err = TX_CMD_SUCCESS;
	smart_home_cmd_buf_t cmd = {{0}};
	if((u32)par_len > sizeof(cmd.payload.nw.par)){
		return TX_CMD_ERR_PAR_LEN;
	}

	if(0 == p_key->valid){
		return TX_CMD_ERR_KEY_INVALID;
	}
	
	smart_home_adv_payload_t *p_payload = &cmd.payload;
	smart_home_nw_t *p_nw = &cmd.payload.nw;
	p_payload->vendor_id2 = VENDOR_ID;
	//p_nw->nid = p_key->nid;	// set inside smart_home_nw_enc_();
	if(0 == smart_home_tx_cmd_sno){smart_home_tx_cmd_sno = 1;}
	p_nw->sno = smart_home_tx_cmd_sno++;
    #if SMART_HOME_CLIENT_SW_EN
    analog_write(SMART_HOME_SW_SNO_REG, smart_home_tx_cmd_sno);
    #endif
    
    p_nw->ttl = RELAY_TTL_DEFAULT;
	p_nw->nid = p_key->nid;	// to print log
	p_nw->op = op;
	p_nw->addr_src = smart_home_get_device_addr(p_key);
	p_nw->addr_dst = addr_dst;
	memcpy(p_nw->par, par, par_len);
	
	// ----
	p_payload->type = GAP_ADTYPE_MANUFACTURER_SPECIFIC;
	p_payload->vendor_id = VENDOR_ID;
	p_payload->len = get_cmd_payload_len_val(par_len);

	if(LIGHT_CMD_SCHEDULE_ACTION_RSP == op){
		cmd.cmd_head.par_type = CMD_BUF_HEAD_TYPE_SCH_ACTION_STATUS;
	}

	#if SMART_HOME_TIME_EN
	if(!is_publish_time_status_message(op, addr_dst))
	#endif
	{
		LOG_MSG_LIB(TL_LOG_PROTOCOL, (u8 *)p_payload, p_payload->len+1-SIZE_MIC,"smart home cmd tx,src:0x%04x,dst:0x%04x,op:0x%02x(%s):", p_nw->addr_src, p_nw->addr_dst, op, get_op_string_sh(op));
	}
	
	// ---- encryption
	smart_home_nw_enc(p_nw, par_len, p_key);
	
	err = smart_home_cmd_tx_push_fifo(&cmd);
#if (0 == SMART_HOME_CLIENT_SW_EN)
	smart_home_cmd_rx_cb(p_payload, RX_SOURCE_TX);	// proxy send command before response.
#endif

	return err;
}

tx_cmd_err_t smart_home_cmd_tx(u8 op, u16 addr_dst, u8 *par, int par_len, sh_server_pair_key_info_t *p_key)
{
	tx_cmd_err_t err = smart_home_cmd_tx_2(op, addr_dst, par, par_len, p_key);
	if(err != TX_CMD_SUCCESS){
		LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"%s,tx op:0x%02x(%s),tx error No.:0x%x(%s)", __FUNCTION__, op, get_op_string_sh(op), err, get_err_number_string_sh(err));
	}
	return err;
}

/*
 * smart_home_cmd_tx_rsp: can be used only for response status after receiving request command.
*/
tx_cmd_err_t smart_home_cmd_tx_rsp(u8 op, u16 addr_dst, u8 *par, int par_len)
{
	sh_decryption_info_t *p_dec_info = &smart_home_decryption_info;
	return smart_home_cmd_tx(op, addr_dst, par, par_len, p_dec_info->p_dec_key);
}

int smart_home_cmd_adv_set(u8 *p_adv, u8 *p_payload)
{
    smart_home_adv_payload_t *p_br = (smart_home_adv_payload_t *)p_payload;
    u8 len_payload = p_br->len + 1;
    if(len_payload > ADV_MAX_DATA_LEN){
        return 0;
    }
    
    rf_packet_adv_t *p = (rf_packet_adv_t *)p_adv;
    p->header.type = LL_TYPE_ADV_NONCONN_IND;
	memcpy(p->advA,blc_ll_get_macAddrPublic(),6);
    memcpy(p->data, &p_br->len, len_payload);
    p->rf_len = 6 + len_payload;
    p->dma_len = p->rf_len + 2;

    return 1;
}


int app_advertise_prepare_handler_smart_home2 (rf_packet_adv_t * p)
{
    static u8 mesh_tx_cmd_busy_cnt;
    int ret = 0;			// default not send ADV
    my_fifo_t *p_fifo = 0;
    u8 *p_buf = 0;
	    
    p_fifo = &smart_home_tx_cmd_fifo;
    p_buf = 0;
    if(0 == mesh_tx_cmd_busy_cnt){
        p_buf = my_fifo_get(p_fifo);
        if(!p_buf){
		    return gatt_adv_prepare_handler(p, 1);
        }
    }else{
        mesh_tx_cmd_busy_cnt--;
        return gatt_adv_prepare_handler(p, 0);
    }

    if(p_buf){
		smart_home_cmd_buf_t *p_cmd = (smart_home_cmd_buf_t *)p_buf;
		mesh_tx_cmd_busy_cnt = p_cmd->cmd_head.transmit_interval;
        ret = smart_home_cmd_adv_set((u8 *)p, (u8 *)&p_cmd->payload);
        if(0 == p_cmd->cmd_head.transmit_cnt){
            my_fifo_pop(p_fifo);
        }else{
			p_cmd->cmd_head.transmit_cnt--;
        }
    }
	return ret;
}

#if (!WIN32)
int app_advertise_prepare_handler_smart_home (rf_packet_adv_t * p)
{
	int ret = app_advertise_prepare_handler_smart_home2(p);
	if(ret){
		if(pkt_adv.header.type == LL_TYPE_ADV_NONCONN_IND){
			//if(blta.adv_type != ADV_TYPE_NONCONNECTABLE_UNDIRECTED)
			{
				bls_ll_setAdvType(ADV_TYPE_NONCONNECTABLE_UNDIRECTED);
			}
		}else if(pkt_adv.header.type == LL_TYPE_ADV_IND){
			//if(blta.adv_type != ADV_TYPE_CONNECTABLE_UNDIRECTED)
			{
				bls_ll_setAdvType(ADV_TYPE_CONNECTABLE_UNDIRECTED);
			}
		}
	}

	return ret;
}

static const smart_home_connectable_adv_t SMART_HOME_CONNECTABLE_ADV = {
	.dma_len = sizeof(smart_home_connectable_adv_t) - OFFSETOF(smart_home_connectable_adv_t, header),
	.header = {LL_TYPE_ADV_IND},
	.rf_len = sizeof(smart_home_connectable_adv_t) - OFFSETOF(smart_home_connectable_adv_t, advA),
	.advA = {0},
	.len1 = 0x02,
	.type1 = 0x01,
	.adv_falg = 0x06,
	.len2 = 1 + SIZEOF_MEMBER(smart_home_connectable_adv_t, info),
	.type2 = GAP_ADTYPE_128BIT_COMPLETE,
	.info = {
		.vendor_id_pair = VENDOR_ID_PAIR,
	},
};

	#if SMART_HOME_ONLINE_ST_EN
static const smart_home_light_online_st_t LIGHT_ONLINE_ST = {
	.dma_len = sizeof(smart_home_light_online_st_t) - OFFSETOF(smart_home_light_online_st_t, header),
	.header = {LL_TYPE_ADV_NONCONN_IND},
	.rf_len = sizeof(smart_home_light_online_st_t) - OFFSETOF(smart_home_light_online_st_t, advA),
	.advA = {0},
	.len1 = sizeof(smart_home_light_online_st_t) - OFFSETOF(smart_home_light_online_st_t, type1),
	.type1 = GAP_ADTYPE_MANUFACTURER_SPECIFIC,
	.vendor_id1 = VENDOR_ID,
	.vendor_id2 = VENDOR_ID_ONLINE_ST,
	.mac = {0},
	.version = 0,
	.op = LIGHT_CMD_LIGHT_ONLINE_STATUS,
};

static u8 online_adv_send_flag;
	#endif
#endif

u32 factory_reset_version = 0;			// use random value // 0 means not be paired by any app or switch.

void set_factory_reset_version()
{
	u32 ver = rand_sh();
	if(0 == ver){
		ver = rand_sh();
	}

	#if 0
	ver |= 0x80808080; // not use type of "name"
	#else
	if(0 == ver){
		ver = 1;
	}
	#endif

	factory_reset_version = ver;
}

int is_factory_reset_version_valid()
{
	return (0 != factory_reset_version);
}

int gatt_adv_prepare_handler2(rf_packet_adv_t * p, int rand_flag)
{
#if (!WIN32)
	#if SMART_HOME_CLIENT_SW_EN
	if(SWITCH_MODE_MASTER == switch_mode){
		return 0;
	}
	#endif
	
	if(clock_time_exceed(tick_adv_ind, tick_adv_ind_interval_us)){
		tick_adv_ind = clock_time();
		set_connectable_adv_interval_with_random();
		#if SMART_HOME_ONLINE_ST_EN
		online_adv_send_flag = 1;
		#endif
		
		if(is_blt_gatt_connected()){
			return 0;
		}else{
			memcpy(p, &SMART_HOME_CONNECTABLE_ADV, min(sizeof(rf_packet_adv_t), sizeof(smart_home_connectable_adv_t)));
			memcpy(p->advA,blc_ll_get_macAddrPublic(),6);
			smart_home_connectable_adv_t *p_con_adv = (smart_home_connectable_adv_t *)p;
			memcpy(p_con_adv->info.mac, p->advA, sizeof(p_con_adv->info.mac));
			p_con_adv->info.factory_reset_ver = factory_reset_version;

			return 1;
		}
	}
	#if SMART_HOME_ONLINE_ST_EN
	else if(online_adv_send_flag){
		online_adv_send_flag = 0;
		static u8 sno_online_st;
		smart_home_light_online_st_t online_st;
		memcpy(&online_st, &LIGHT_ONLINE_ST, sizeof(online_st));
		memcpy(online_st.advA,blc_ll_get_macAddrPublic(),6);
		online_st.sno = sno_online_st++;
		memcpy(online_st.mac, online_st.advA, sizeof(online_st.mac));
		online_st.version = factory_reset_version;
		set_online_st_data(&online_st);
		smart_home_online_st_enc(&online_st);
		
		memcpy(p, &online_st, min(sizeof(rf_packet_adv_t), online_st.dma_len + sizeof(online_st.dma_len)));
		return 1;
	}
	#endif
#endif
    
	return 0;
}

int gatt_adv_prepare_handler(rf_packet_adv_t * p, int rand_flag)
{
    int ret = gatt_adv_prepare_handler2(p, rand_flag); // always return 1 for switch project, because prepare interval is equal to gatt adv interval.
    if(ret){
        return ret;
    }

#if FEATURE_RELAY_EN
    ret = relay_adv_prepare_handler(p, rand_flag); // lower than GATT ADV should be better. if not, adv can not be send when too much relay packet, such as there are many nodes that are publishing with a low period.
    if(ret){
        return ret;
    }
#endif

	return ret;
}


#if SMART_HOME_SERVER_EN
static void set_all_group_id_invalid()
{
	memset(smart_home_group_id, INVALID_GROUP_ID_SET, sizeof(smart_home_group_id));
}

static int is_group_match(u16 addr_dst)
{
	foreach_arr(i, smart_home_group_id){
		if(addr_dst == smart_home_group_id[i]){
			return 1;
		}
	}

	return 0;
}

addr_match_type_t get_cmd_addr_match(u16 addr_dst, sh_server_pair_key_info_t *p_info)
{
    if(addr_dst & GROUP_MASK){// group or broadcast
        if((addr_dst == ADDR_ALL_NODES) || is_group_match(addr_dst)){
            return ADDR_MATCH_TYPE_GROUP;
        }
    }else{// single device
		// TODO: add src type: if from ADV, group_mac_info can't be 0;
        if((addr_dst == ADDR_LOCAL) || (addr_dst == smart_home_get_device_addr(p_info))){
            return ADDR_MATCH_TYPE_MAC;
        }
    }
	return ADDR_MATCH_TYPE_NONE;
}

// ---- message cache process --------
static sno_cache_buf_t sno_cache_buf[SNO_CACHE_CNT_MAX];

void add2sno_cache(smart_home_nw_t *p_nw, int idx, int key_idx)
{
    sno_cache_buf[idx].key_idx = key_idx;
    sno_cache_buf[idx].src = p_nw->addr_src;
    sno_cache_buf[idx].sno = p_nw->sno;
}

#define CACHE_COMPARE_CNT		(4) // must >= 1, and 1 means only euqual to is replay sno.

STATIC_ASSERT(CACHE_COMPARE_CNT >= 1);

static inline int is_replay_sno(u16 sno_rx, u16 sno_cache)
{
#if 1
	u16 delta = sno_cache - sno_rx;
	return (delta < CACHE_COMPARE_CNT);
#else
	for(u32 i = 0; i < (CACHE_COMPARE_CNT); ++i){
		if(sno_rx == (u16)(sno_last - i)){
			return 1;
		}
	}

	return 0;
#endif
}

int is_exist_in_sno_cache(smart_home_nw_t *p_nw, sh_server_pair_key_info_t *p_dec_key)
{
	u32 key_idx = get_dec_key_array_index(p_dec_key);
    foreach_arr(i, sno_cache_buf){
        if((key_idx == sno_cache_buf[i].key_idx) && (p_nw->addr_src == sno_cache_buf[i].src)){
			if(is_replay_sno(p_nw->sno, sno_cache_buf[i].sno)){
            	return 1;
            }else{
                add2sno_cache(p_nw, i, key_idx);		// just update
                return 0;
            }
        }
    }

    //new device found
    static u16 sno_cache_idx = 0;
    add2sno_cache(p_nw, sno_cache_idx, key_idx);
    sno_cache_idx = (sno_cache_idx + 1) % SNO_CACHE_CNT_MAX;

    return 0;
}

void unpair_clear_cache(sh_server_pair_key_info_t *p_dec_key)
{
	u32 key_idx = get_dec_key_array_index(p_dec_key);
    foreach_arr(i, sno_cache_buf){
    	sno_cache_buf_t *p_cache = &sno_cache_buf[i];
        if(key_idx == p_cache->key_idx){
        	memset(p_cache, 0, sizeof(sno_cache_buf_t));
        	p_cache->key_idx = SMART_HOME_SERVER_PAIR_KEY_MAX; // invalid
        }
    }
}

// -------------- RX handle -------------
#if 0
/*
* filter function
*/
_attribute_ram_code_ int blc_ll_procScanPktSmartHome(u8 *raw_pkt, u8 *new_pkt, u32 tick_now)
{
	#define BLE_RCV_FIFO_MAX_CNT	(4)	// default buffer count of BLE generic SDK which is 8. but no need too much.

	u8 next_buffer =1;
#if ((MCU_CORE_TYPE == MCU_CORE_5316) || (MCU_CORE_TYPE == MCU_CORE_5317))
	u8 *p_rf_pkt =  (raw_pkt + 8);
#elif ((MCU_CORE_TYPE == MCU_CORE_8258) || (MCU_CORE_TYPE == MCU_CORE_8278) || (MCU_CORE_TYPE == MCU_CORE_8208))
	u8 *p_rf_pkt =	(raw_pkt + 0); // B85m, include B80
#elif (MCU_CORE_TYPE == MCU_CORE_8269)
	u8 *p_rf_pkt =  (raw_pkt + 8);
#endif
	
	rf_packet_adv_t * pAdv = (rf_packet_adv_t *)p_rf_pkt;
	u8 adv_type = pAdv->header.type;
	
	// "fifo_free_cnt" here means if accepte this packet, there still is the number of "fifo_free_cnt" remained, because wptr has been ++.
	u8 fifo_free_cnt = blt_rxfifo.num - ((u8)(blt_rxfifo.wptr - blt_rxfifo.rptr)&(blt_rxfifo.num-1));
	if(blc_ll_getCurrentState() == BLS_LINK_STATE_CONN){
		if(blt_ll_getCurrentBLEState() == BLE_STATE_BRX_E){
			if(fifo_free_cnt < max2(BLE_RCV_FIFO_MAX_CNT, 2)){
				next_buffer = 0;
			}
			
			if(fifo_free_cnt < 1){
                write_reg8(0x800f00, 0x80);         // stop ,just stop BLE state machine is enough 
			    //next_buffer = 0;	// should not discard
			    //LOG_MSG_LIB(TL_LOG_MESH,0,0,"FIFO FULL");
			}
		}
	}else{
		if (fifo_free_cnt < 2){	// 4
			// can not make the fifo overflow 
			next_buffer = 0;
		}else{
		}
	}

	if(0 == next_buffer){
		return 0;
	}

	int advPkt_valid = 0;
	if(is_valid_adv_type(adv_type)){
		if(parse_adv_data(pAdv->data, pAdv->rf_len - 6)){
			advPkt_valid = 1;
		}
	}

	return advPkt_valid;
}

/*
* process rx buffer that received by blc_ll_procScanPktSmartHome_.
* called by blt_sdk_main_loop.
*/
int blc_ll_procScanDataSmartHome(u8 *raw_pkt)
{
	if(blts.scan_en)
	{
		u8 type = raw_pkt[12] & 15;
		if(is_valid_adv_type(type))
		{
			event_adv_report_t * pa = (event_adv_report_t *) (raw_pkt + 9);

			u8 len = raw_pkt[13];

			pa->subcode = HCI_SUB_EVT_LE_ADVERTISING_REPORT;
			pa->nreport = 1;

			//note that: adv report event type value different from adv type
			if(type == LL_TYPE_ADV_NONCONN_IND){
				pa->event_type = ADV_REPORT_EVENT_TYPE_NONCONN_IND;
			}
			else if(type == LL_TYPE_ADV_SCAN_IND){
				pa->event_type = ADV_REPORT_EVENT_TYPE_SCAN_IND;
			}
			else{
				pa->event_type = type;
			}

			pa->adr_type = raw_pkt[12] & BIT(6) ? 1 : 0;
			memcpy (pa->mac, raw_pkt + 14, 6);

			pa->len = len - 6;
			pa->data[pa->len] = raw_pkt[4] - 110;
			//get frequency offset
			signed short  dc = pa->data[pa->len + 3] | (pa->data[pa->len + 4] << 8);
			dc = (dc + 200) >> 3;
			pa->data[pa->len + 1] = (u8)dc;
			pa->data[pa->len + 2] = (u8)(dc >> 8);

			smart_home_cmd_rx_adv_report(pa->data, pa->len);
		}
	}

	return 1;
}
#else

void set_scan_interval(u32 ms)
{
	blt_ll_setScanIntervalTick(ms * 1000 * SYSTEM_TIMER_TICK_1US); // bltScn.scnInterval_tick
}

/*
* filter function
*/
_attribute_ram_code_ int blc_ll_procScanPktSmartHome(u8 *raw_pkt, u8 *new_pkt, u32 tick_now)
{
	int	next_buffer = 0;
	rf_packet_adv_t * pAdv = (rf_packet_adv_t *) (raw_pkt + DMA_RFRX_LEN_HW_INFO);
	if(is_valid_adv_type(pAdv->header.type)){
		smart_home_adv_payload_t * p_sh = parse_adv_data(pAdv->data, pAdv->rf_len - 6);
		if(p_sh){
			if(VENDOR_ID == p_sh->vendor_id2){
				int par_len = get_cmd_para_len(p_sh);
				if(par_len >= SIZE_MIC){
					next_buffer = 1;
				}
			}else{
				next_buffer = 1; // todo check length
			}
		}
	}

	return next_buffer;
}

/**
 * @brief      callback function of HCI Controller Event
 * @param[in]  h - HCI Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
int blc_ll_procScanDataSmartHome (u32 h, u8 *p, int n)
{
	if (h &HCI_FLAG_EVENT_BT_STD)		//ble controller hci event
	{
		u8 evtCode = h & 0xff;

		if(evtCode == HCI_EVT_LE_META)
		{
			u8 subEvt_code = p[0];
			if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)	// ADV packet
			{
				DBG_CHN6_TOGGLE;
				//after controller is set to scan state, it will report all the adv packet it received by this event

				event_adv_report_t *pa = (event_adv_report_t *)p;
				//s8 rssi = (s8)pa->data[pa->len];//rssi has already plus 110.
				//printf("%ddb, l:%d\n", rssi, pa->len+11);
				//LOG_USER_MSG_INFO(pa->data, pa->len, "%ddb, l:%d, cnt: %d\n", rssi, pa->len+11);

				smart_home_cmd_rx_adv_report(pa->data, pa->len);
			}
		}
	}

	return 0;
}
#endif

#define SMART_HOME_SCAN_IN_CONNECT_ST_EN		1

void blc_ll_initScanning_moduleSmartHome(u8 *public_adr)
{
#if (!WIN32)
	#if 1
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(blc_ll_procScanDataSmartHome);
		#if 0
	SmartHome_blt_ll_initScanState();
		#else
	blt_ll_initScanState();
	blc_ll_procScanPktCb = blc_ll_procScanPktSmartHome;// filter function.
	set_scan_interval(SMART_HOME_SCAN_INTERVAL_MS);
		#endif
	blc_ll_addScanningInAdvState();  //add scan in adv state
		#if SMART_HOME_SCAN_IN_CONNECT_ST_EN
	blc_ll_addScanningInConnSlaveRole();  //add scan in conn slave role
		#endif
		#if 0//BLT_ADV_IN_CONN_SLAVE_EN
	//blc_ll_addAdvertisingInConnSlaveRole(); // no need this mode.
		#endif
	#else // legacy version
	//blc_ll_switchScanChannelCb = blc_ll_switchScanChannel;//set scan channel
	blc_ll_procScanPktCb = blc_ll_procScanPktSmartHome;//receive ADV packet and send Scan Request packet.
	blc_ll_procScanDatCb = blc_ll_procScanDataSmartHome;//handle HCI event.

	blts_scan_interval = SMART_HOME_SCAN_INTERVAL_MS * CLOCK_16M_SYS_TIMER_CLK_1MS;  //default value: 60ms

	#if 0
	if(16 == sys_tick_per_us){
		blts.T_SCAN_REQ_INTVL = 26;	// no used in library any more
	}else{
		blts.T_SCAN_REQ_INTVL = 26;	// no used in library any more
	}
	#endif

	blts.scan_extension_mask = BLS_FLAG_SCAN_IN_ADV_MODE | BLS_FLAG_SCAN_IN_SLAVE_MODE;
	blts.scan_en = 1;
	#endif
#endif
}

int is_valid_group_id(u16 group_id)
{
	return ((group_id >= 0x8000) && (group_id != 0xffff));
}

rx_cb_err_t group_get_set_rx_handle(lcmd_group_t *p_set)
{
	rx_cb_err_t err = RX_CB_SUCCESS;
	int save_flag = 0;
	if((GROUP_ADD == p_set->set_mode) || (GROUP_DEL == p_set->set_mode)){
		if(!is_valid_group_id(p_set->group_id)){
			return RX_CB_ERR_GROUP_ID_INVALID;
		}
	
		int add_ok_flag = 0;
		u16 *p_null = NULL;
		foreach_arr(i, smart_home_group_id){
			u16 *p_id = &smart_home_group_id[i];
			if(p_set->group_id == *p_id){
				if(GROUP_ADD == p_set->set_mode){
					add_ok_flag = 1;
					break;
				}if(GROUP_DEL == p_set->set_mode){
					*p_id = INVALID_GROUP_ID_SET;
					save_flag = 1;
					// break;	// no break, in case that there are same ids.
				}
			}else if((NULL == p_null) && (INVALID_GROUP_ID_SET == *p_id)){
				p_null = p_id;
			}
		}
	
		if((GROUP_ADD == p_set->set_mode) && (0 == add_ok_flag)){
			if(p_null){
				*p_null = p_set->group_id;
				save_flag = 1;
			}else{
				return RX_CB_ERR_GROUP_FULL;
			}
		}
	}else if(GROUP_DEL_ALL == p_set->set_mode){
		foreach_arr(i, smart_home_group_id){
			if(smart_home_group_id[i] != INVALID_GROUP_ID_SET){
				set_all_group_id_invalid();
				save_flag = 1;
				break;
			}
		}
	}else if(GROUP_GET == p_set->set_mode){
		err = RX_CB_ERR_GROUP_ID_NOT_FOUND;
		foreach_arr(i, smart_home_group_id){
			if(smart_home_group_id[i] == p_set->group_id){
				err = RX_CB_SUCCESS;
				break;
			}
		}
	}
		
	if(save_flag){
		smart_home_group_id_save();
	}
	if(GROUP_GET != p_set->set_mode){
		smart_home_led_event_callback(LED_EVENT_SETTING);
	}

	return err;
}

rx_cb_err_t group_get_set_rx_cb(smart_home_nw_t *p_nw)
{
	lcmd_group_t *p_set = (lcmd_group_t *)p_nw->par;
	rx_cb_err_t err = group_get_set_rx_handle(p_set);

	if((GROUP_GET == p_set->set_mode) || p_set->ack){ // response
		lcmd_group_st_t cmd_status = {0};
		cmd_status.st = err;
		cmd_status.group_id = (GROUP_DEL_ALL == p_set->set_mode) ? INVALID_GROUP_ID_SET : p_set->group_id;
		int rsp_len = sizeof(cmd_status);
		smart_home_cmd_tx_rsp(LIGHT_CMD_GROUP_STATUS, p_nw->addr_src, (u8 *)&cmd_status, rsp_len);
	}

	return err;
}

rx_cb_err_t group_id_list_get_rx_cb(smart_home_nw_t *p_nw)
{
	rx_cb_err_t err = RX_CB_SUCCESS;
	lcmd_group_id_list_get_t *p_get = (lcmd_group_id_list_get_t *)p_nw->par;
	lcmd_group_id_list_t list = {0};
	int rsp_len = OFFSETOF(lcmd_group_id_list_t, id_list);
	int total_cnt = 0;
	int index = 0;

	foreach_arr(i, smart_home_group_id){
		u16 *p_id = &smart_home_group_id[i];
		if(*p_id != INVALID_GROUP_ID_SET){
			if(total_cnt++ >= p_get->offset){
				if(index < ARRAY_SIZE(list.id_list)){
					list.id_list[index++] = *p_id;
				}
			}
		}
	}

	list.offset = p_get->offset;
	list.total_cnt = total_cnt;
	rsp_len += index * sizeof(list.id_list[0]);
	
	smart_home_cmd_tx_rsp(LIGHT_CMD_GROUP_ID_LIST, p_nw->addr_src, (u8 *)&list, rsp_len);
	
	return err;
}

rx_cb_err_t cps_get_rx_cb(smart_home_nw_t *p_nw)
{
	rx_cb_err_t err = RX_CB_SUCCESS;
	sh_composition_data_t cmd_status = {0};
	smart_home_pair_get_cps(&cmd_status);
	
	smart_home_cmd_tx_rsp(LIGHT_CMD_CPS_STATUS, p_nw->addr_src, (u8 *)&cmd_status, sizeof(cmd_status));
	#if SMART_HOME_CLIENT_SW_EN
	smart_home_switch_quick_exit_slave_mode_check();
	#endif
	
	return err;
}

rx_cb_err_t smart_home_cmd_rx_handle_op(smart_home_adv_payload_t *payload, rx_source_type_t source)
{
	rx_cb_err_t err = RX_CB_SUCCESS;
	smart_home_nw_t *p_nw = &payload->nw;
	//int par_len = get_cmd_para_len(payload);
	u8 op = p_nw->op;

	if(!is_publish_time_status_message(op, p_nw->addr_dst)) // because too much messages
	{
		LOG_MSG_LIB(TL_LOG_PROTOCOL, (u8 *)payload, payload->len+1-SIZE_MIC,"-------command RX,src:0x%04x,dst:0x%04x,op:0x%02x(%s):", p_nw->addr_src, p_nw->addr_dst, op, get_op_string_sh(op));
	}
	
	if(LIGHT_CMD_GROUP_GET_ADD_DEL == op){
		err = group_get_set_rx_cb(p_nw);
	}else if(LIGHT_CMD_GROUP_ID_LIST_GET == op){
		err = group_id_list_get_rx_cb(p_nw);
	}else if(LIGHT_CMD_DEL_NODE == op){
		sh_server_pair_key_info_t *p_info = smart_home_decryption_info.p_dec_key;
		lcmd_del_node_t *p_del = (lcmd_del_node_t *)p_nw->par;
	
		if(p_del->ack){ // response before delete key, if not, no key to send message.
			smart_home_cmd_tx_rsp(LIGHT_CMD_DEL_NODE_STATUS, p_nw->addr_src, (u8 *)NULL, 0);
		}
		
		if((DEL_NODE_MODE_NORMAL == p_del->del_mode)
		|| ((DEL_NODE_MODE_TIME_LIMIT == p_del->del_mode)&&(0 == flag_can_not_pair))){
			smart_home_unpair_and_save(p_info); // just clear key info, can not run factory reset, because other APP still need this info.
			smart_home_led_event_callback(LED_EVENT_DEL_NODE);
		}
	}else if(LIGHT_CMD_CPS_GET == op){
		err = cps_get_rx_cb(p_nw);
#if SMART_HOME_ONOFF_EN
	}else if(LIGHT_CMD_ONOFF_SET == op){
		lcmd_onoff_t *p_set = (lcmd_onoff_t *)p_nw->par;
		u8 last_light_off = light_off;
		u8 onoff_target = !light_off;

		if(p_set->set_mode <= L_TOGGLE){
			if(p_set->set_mode < L_TOGGLE){
				onoff_target = p_set->set_mode;
			}else if(p_set->set_mode == L_TOGGLE){
				onoff_target = !!light_off;
			}
			
			g_light_st_delay.delay_10ms = 0;	// init, should not clear tick_light_step_timer that are also used for light adjust step.
			if(p_set->delay_10ms && (!last_light_off != onoff_target)){
				g_light_st_delay.tick_light_step_timer = clock_time();
				g_light_st_delay.delay_10ms = p_set->delay_10ms;
				g_light_st_delay.onoff = onoff_target;
			}else{
				light_onoff(onoff_target);
			}
		}

		if((L_GET == p_set->set_mode) || p_set->ack){ // response
			lcmd_onoff_st_t cmd_status;
			cmd_status.onoff = onoff_target;
			smart_home_cmd_tx_rsp(LIGHT_CMD_ONOFF_STATUS, p_nw->addr_src, (u8 *)&cmd_status, sizeof(cmd_status));
		}

		if(light_off != last_light_off){
			#if LIGHT_SAVE_ONOFF_STATE_EN
			smart_home_lum_save();
			#endif
		}
	}else if(LIGHT_CMD_LIGHTNESS_SET == op){
		lcmd_lightness_t *p_set = (lcmd_lightness_t *)p_nw->par;
		u8 last_off = light_off;
		u8 last_lum = led_lum;
		if(LIGHT_SET_MODE_GET != p_set->set_mode){
			if(!is_valid_lightness_sh(p_set->lightness)){ // (led_lum == p_set->lightness)
				return err;
			}
			if(light_off){
				// return err;	// will trun on light
			}
			u8 lum_set = get_target_value_lightness(p_set->set_mode, last_lum, p_set->lightness);
			light_adjust_lightness(lum_set);
		}
		
		if((LIGHT_SET_MODE_GET == p_set->set_mode) || p_set->ack){ // response
			lcmd_lightness_st_t cmd_status = {0};
			cmd_status.lightness = get_current_lightness();
			smart_home_cmd_tx_rsp(LIGHT_CMD_LIGHTNESS_STATUS, p_nw->addr_src, (u8 *)&cmd_status, sizeof(cmd_status));
		}
		
		if((last_off != light_off) || (led_lum != last_lum)){
			smart_home_lum_save();
		}
#endif
#if SMART_HOME_CT_EN
	}else if(LIGHT_CMD_LIGHTNESS_CT_SET == op){
		lcmd_CTL_t *p_set = (lcmd_CTL_t *)p_nw->par;
		u8 last_ct_flag = light_ct_flag;
		u8 last_off = light_off;
		u8 last_lum = led_lum;
		u8 last_ct = led_ct;
		if(LIGHT_SET_MODE_GET != p_set->set_mode){
			if((p_set->lightness_flag && !is_valid_lightness_sh(p_set->lightness))
			|| (p_set->ct_flag && !is_valid_ct_sh(p_set->ct))){
				return err;
			}

			u8 lum_set = p_set->lightness_flag ? get_target_value_lightness(p_set->set_mode, last_lum, p_set->lightness) : last_lum;
			u8 ct_set = p_set->ct_flag ? get_target_value_CT(p_set->set_mode, last_ct, p_set->ct) : last_ct;
			light_adjust_CT(ct_set, lum_set);
		}

		// led refresh
		
		if((LIGHT_SET_MODE_GET == p_set->set_mode) || p_set->ack){ // response
			lcmd_CTL_st_t cmd_status = {0};
			cmd_status.lightness = get_current_lightness();
			cmd_status.ct = led_ct;
			smart_home_cmd_tx_rsp(LIGHT_CMD_LIGHTNESS_CT_STATUS, p_nw->addr_src, (u8 *)&cmd_status, sizeof(cmd_status));
		}
		
		if((last_ct_flag != light_ct_flag) || (last_off != light_off) || (led_ct != last_ct) || (led_lum != last_lum)){
			smart_home_lum_save();
		}
#endif
#if SMART_HOME_RGB_EN
	}else if(LIGHT_CMD_LIGHTNESS_RGB_SET == op){
		lcmd_RGBL_t *p_set = (lcmd_RGBL_t *)p_nw->par;
		u8 last_ct_flag = light_ct_flag;
		u8 last_off = light_off;
		u8 last_lum = led_lum;
		led_RGB_t last_rgb = led_RGB;

		if(LIGHT_SET_MODE_GET != p_set->set_mode){
			if(p_set->lightness_flag && !is_valid_lightness_sh(p_set->lightness)){
				return err;
			}

			u8 lum_set = p_set->lightness_flag ? get_target_value_lightness(p_set->set_mode, last_lum, p_set->lightness) : last_lum;
			led_RGB_t rgb_set = {0};
			rgb_set.R = p_set->R_flag ? get_target_value_RGB(p_set->set_mode, last_rgb.R, p_set->R) : last_rgb.R;
			rgb_set.G = p_set->G_flag ? get_target_value_RGB(p_set->set_mode, last_rgb.G, p_set->G) : last_rgb.G;
			rgb_set.B = p_set->B_flag ? get_target_value_RGB(p_set->set_mode, last_rgb.B, p_set->B) : last_rgb.B;
			light_adjust_RGB(&rgb_set, lum_set);
		}
		
		if((LIGHT_SET_MODE_GET == p_set->set_mode) || p_set->ack){ // response
			lcmd_RGBL_st_t cmd_status = {0};
			cmd_status.lightness = get_current_lightness();
			cmd_status.R = led_RGB.R;
			cmd_status.G = led_RGB.G;
			cmd_status.B = led_RGB.B;
			smart_home_cmd_tx_rsp(LIGHT_CMD_LIGHTNESS_RGB_STATUS, p_nw->addr_src, (u8 *)&cmd_status, sizeof(cmd_status));
		}
		
		if((last_ct_flag != light_ct_flag) || (last_off != light_off) || (led_lum != last_lum) || memcmp(&led_RGB, &last_rgb, sizeof(led_RGB))){
			smart_home_lum_save();
		}
#endif
#if SMART_HOME_SCENE_EN
	}else if(LIGHT_CMD_SCENE_GET_SET == op){
		err = scene_set_rx_cb(p_nw);
	}else if(LIGHT_CMD_SCENE_CALL == op){
		err = scene_call_rx_cb(p_nw);
	}else if(LIGHT_CMD_SCENE_ADD_NO_PAR == op){
		err = scene_add_no_par_rx_cb(p_nw);
	}else if(LIGHT_CMD_SCENE_ID_LIST_GET == op){
		err = scene_id_list_get_rx_cb(p_nw);
#endif
#if(SMART_HOME_TIME_EN)
	}else if((LIGHT_CMD_TIME_SET_ACK == op)||(LIGHT_CMD_TIME_SET_NO_ACK == op)){
		err = time_set_rx_cb(p_nw);
	}else if(LIGHT_CMD_TIME_GET == op){
		time_tx_rsp(p_nw->addr_src, RX_CB_SUCCESS);
	}else if(LIGHT_CMD_TIME_STATUS == op){
		time_status_rx_cb(p_nw);
#endif
#if(SMART_HOME_SCHEDULER_EN)
	}else if(LIGHT_CMD_SCHEDULE_ACTION_SET == op){
		err = scheduler_action_set_rx_cb(p_nw);
	}else if(LIGHT_CMD_SCHEDULE_ACTION_GET == op){
		err = scheduler_action_get_rx_cb(p_nw);
	}else if(LIGHT_CMD_SCHEDULE_LIST_GET == op){
		err = scheduler_list_get_rx_cb(p_nw);
#endif
	}else{
	}

	
	return err;
}

static inline rx_cb_err_t smart_home_cmd_rx_cb2(smart_home_adv_payload_t *payload, rx_source_type_t source)
{
	#if !WIN32
	if((0xFF5D == *(u16 *)6) || (0xFE5D == *(u16 *)6)){ // default value is 0x025D
		LOG_MSG_LIB(TL_LOG_PROTOCOL, (u8 *)payload, payload->len+1,"---------sniffer:");
	}
	#endif
	
	rx_cb_err_t err = RX_CB_SUCCESS;
	if(!((GAP_ADTYPE_MANUFACTURER_SPECIFIC == payload->type) && (is_valid_nw_vendor_id(payload->vendor_id2)))){
		return RX_CB_ERR_DATA_TYPE;
	}

	if(payload->len > ADV_MAX_DATA_LEN - 1){
		return RX_CB_ERR_PAR_LEN;
	}

	if(VENDOR_ID_PAIR == (payload->vendor_id2)){
		#if (SMART_HOME_SERVER_EN)// && (0 == SMART_HOME_GATEWAY_EN))	// gateway never be paired.
		err = smart_home_pair_server_rx_cb(payload, source);
		#endif
		#if SMART_HOME_CLIENT_APP_EN
		if(RX_CB_SUCCESS == err){
			err = smart_home_pair_client_rx_cb(payload, source, 0);
		}
		#endif
		
		return err;
	}

	// ---------- (VENDOR_ID == (payload->vendor_id2))
	smart_home_nw_t *p_nw = &payload->nw;
	int par_len = get_cmd_para_len(payload);

	// ---------- decryption
	#if 0 // SMART_HOME_CLIENT_APP_EN
	if(RX_SOURCE_INI == source){
		if(par_len < 0){
			return RX_CB_ERR_PAR_LEN;
		}
		smart_home_decryption_info.p_dec_key = P_TX_KEY_TEST;//
		err = RX_CB_SUCCESS;
	}else
	#endif
	{
		if(par_len < SIZE_MIC){
			return RX_CB_ERR_PAR_LEN;
		}
		err = smart_home_cmd_dec(p_nw, par_len);
	}
	
	if(err != RX_CB_SUCCESS){
		return err;
	}
	
#if !WIN32
	if(0xFE5D == *(u16 *)6){ // default value is 0x025D
		LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0, "dec ok!");
	}
#endif

	sh_decryption_info_t *p_dec_info = &smart_home_decryption_info;
	// u8 op = p_nw->op;

	if((RX_SOURCE_ADV == source)||(RX_SOURCE_GATT == source)){
		// LOG_MSG_LIB(TL_LOG_PROTOCOL, (u8 *)payload, payload->len+1,"---------sniffer RX no check dst addr,src:0x%04x,dst:0x%04x,op:0x%02x(%s):", p_nw->addr_src, p_nw->addr_dst, op, get_op_string_sh(op));
	}

	if(is_exist_in_sno_cache(p_nw, p_dec_info->p_dec_key)){		// should be after addr match, because should not add status command to cache, which may include too many nodes.
        return RX_CB_ERR_SNO_REPLAY;
	}

	if(smart_home_get_device_addr(p_dec_info->p_dec_key) == p_nw->addr_src || (0 != smart_home_get_device_addr(p_dec_info->p_dec_key) && smart_home_get_device_addr(p_dec_info->p_dec_key) == p_nw->addr_dst)){
		// TODO:  if need to RELAY, must after cache compare or relay only one after received one.
		// return RX_CB_ERR_ADDR_SRC_SELF; // no relay now, and may equal to address of switch. 
	}else{	      
        #if FEATURE_RELAY_EN
        u8 bear_enc_copy[ADV_MAX_DATA_LEN] = {0};
    	smart_home_adv_payload_t *p_bear_enc_copy = (smart_home_adv_payload_t *)bear_enc_copy;
        u32 payload_len = payload->len + 1;
        if(payload_len){
            memcpy(&p_bear_enc_copy->len, payload, payload_len);
        }
        if(is_smart_home_tx_relay_cmd_no_full() && p_nw->ttl >= 2){
            smart_home_cmd_buf_t cmd;
            p_bear_enc_copy->nw.ttl--;
            smart_home_nw_enc(&p_bear_enc_copy->nw, par_len - 2, p_dec_info->p_dec_key);
            memcpy(&cmd.payload.len, p_bear_enc_copy, payload_len);
            my_fifo_push_relay(&cmd);
        }
        #endif
	}

	if(ADDR_MATCH_TYPE_NONE == get_cmd_addr_match(p_nw->addr_dst, p_dec_info->p_dec_key)){
		return RX_CB_ERR_ADDR_DST_NOT_MATCH;
	}
	
	err = smart_home_cmd_rx_handle_op(payload, source);

	return err;
}

rx_cb_err_t smart_home_cmd_rx_cb(smart_home_adv_payload_t *payload, rx_source_type_t source)
{
#if SMART_HOME_CLIENT_SW_EN
	if(SWITCH_MODE_MASTER == switch_mode){
		LOG_MSG_ERR(TL_LOG_PROTOCOL, 0, 0,"Not handle any packet in switch master mode"); // (u8 *)payload, payload->len+1
		return RX_CB_SUCCESS;
	}
#endif

	rx_cb_err_t err = smart_home_cmd_rx_cb2(payload, source);
	if(err && (!((err == RX_CB_ERR_ADDR_DST_NOT_MATCH) || (err == RX_CB_ERR_SNO_REPLAY) || 
				  (err == RX_CB_ERR_DECRYPTION_NO_KEY_FOUND) || (err == RX_CB_ERR_DECRYPTION_FAILED)))){ // not print decyption error because too much.
		smart_home_nw_t *p_nw = &payload->nw;
		if(VENDOR_ID == (payload->vendor_id2)){
			// if decryption failed, src/dst/op may be error.
			LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"%s,source:%d,src:0x%04x,dst:0x%04x,op:0x%02x(%s), rx error No.:0x%x(%s)", __FUNCTION__, source, p_nw->addr_src, p_nw->addr_dst, p_nw->op, get_op_string_sh(p_nw->op), err, get_err_number_string_sh(err));
		}else{
			LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"%s,source:%d, error No.:0x%x(%s)", __FUNCTION__, source, err, get_err_number_string_sh(err));
		}
	}
	return err;
}

_attribute_ram_code_ smart_home_adv_payload_t * parse_adv_data(u8 *data, int len)
{
	smart_home_adv_payload_t *p_temp = (smart_home_adv_payload_t *)data;
	//int app_flag = 0;
	int offset = 0;
	while(len > (offset + 2)){
		if(GAP_ADTYPE_FLAGS == p_temp->type){
			//app_flag = 1;
		}else if(GAP_ADTYPE_MANUFACTURER_SPECIFIC == p_temp->type){
			if((VENDOR_ID == p_temp->vendor_id)&&(is_valid_nw_vendor_id(p_temp->vendor_id2))){
				return p_temp;
			}
			if(VENDOR_ID_IOS == p_temp->vendor_id){
				smart_home_adv_payload_ios_t *p_ios = (smart_home_adv_payload_ios_t *)p_temp;
				if((iBEACON_FLAG == p_ios->ibeacon_type) && is_valid_nw_vendor_id(p_ios->vendor_id2)){
					return p_temp;
				}
			}
			return 0;
		}
		offset += p_temp->len + 1;
		p_temp = (smart_home_adv_payload_t *)(data + offset);
	}
	return 0;
}

rx_cb_err_t smart_home_cmd_rx_adv_report(u8 *data, int len)
{
	//LOG_MSG_LIB(TL_LOG_PROTOCOL, (u8 *)data, len,"---------sniffer raw data RX:");
	smart_home_adv_payload_t * payload = parse_adv_data(data, len);
	if(payload){
		u32 offset = (u32)payload - (u32)data;
		if(offset){
			// remove redundancy data
			memcpy(data, payload, (len - offset));
			smart_home_adv_payload_ios_t *p_ios = (smart_home_adv_payload_ios_t *)data;
			if(VENDOR_ID_IOS == p_ios->vendor_id){
				u32 copy_len = p_ios->len + 1 - OFFSETOF(smart_home_adv_payload_ios_t, vendor_id2);
				memcpy(&p_ios->ibeacon_type, &p_ios->vendor_id2, copy_len);
				p_ios->len -= sizeof(p_ios->ibeacon_type);
			}
		}
		smart_home_adv_payload_t *p_result = (smart_home_adv_payload_t *)data;
		//LOG_MSG_LIB(TL_LOG_PROTOCOL, (u8 *)p_result, p_result->len + 1,"---------parse RX:");
		return smart_home_cmd_rx_cb(p_result, RX_SOURCE_ADV);
	}
	return RX_CB_ERR_DATA_TYPE;
}
#endif

#if SMART_HOME_CLIENT_APP_EN // -------- only for gateway or WIN 32 ------
rx_cb_err_t smart_home_rx_ini_cmd(u8 *par_ini, int par_len)
{
	rx_cb_err_t err = RX_CB_SUCCESS;
	#if 0 // for adv data test
	smart_home_cmd_rx_adv_report(par_ini, par_len);
	return err;
	#endif
	
	smart_home_cmd_buf_t cmd_buf = {{0}};
	smart_home_adv_payload_t *p_payload = &cmd_buf.payload;
	if(par_len <= SIZEOF_MEMBER(smart_home_adv_payload_t, vendor_id2) + OFFSETOF(smart_home_nw_t, rsv_mic)){
		p_payload->type = GAP_ADTYPE_MANUFACTURER_SPECIFIC;
		p_payload->vendor_id = VENDOR_ID;
		memcpy(&p_payload->vendor_id2, par_ini, par_len);
		p_payload->len = OFFSETOF(smart_home_adv_payload_t, vendor_id2) - SIZEOF_MEMBER(smart_home_adv_payload_t, len) + par_len;
		addr_match_type_t match_type = get_cmd_addr_match(p_payload->nw.addr_dst, P_TX_KEY_TEST);

		// ---- encryption
		int nw_len = get_cmd_para_len(p_payload);
		smart_home_nw_enc(&p_payload->nw, nw_len, P_TX_KEY_TEST);
		p_payload->len += SIZE_MIC;

		if(ADDR_MATCH_TYPE_MAC != match_type){
			if(TX_CMD_SUCCESS != smart_home_cmd_tx_push_fifo(&cmd_buf)){
				err = RX_CB_ERR_PROXY_INI_TX;
			}
		}
		smart_home_cmd_rx_cb(p_payload, RX_SOURCE_INI); // tx status should be after proxy sending
	}else{
		err = RX_CB_ERR_NW_LEN;
	}
	return err;
}

void smart_home_rx_ini_pair(u8 *par_ini, int par_len)
{
	smart_home_adv_payload_t *payload = CONTAINER_OF((u16 *)par_ini, smart_home_adv_payload_t, vendor_id2);
	smart_home_pair_t *p_pair = &payload->pair;
	if(VENDOR_ID_PAIR != payload->vendor_id2){	// TODO
		return ;
	}

	int pair_par_len = par_len - OFFSETOF(smart_home_pair_t, par);
	if(PAIR_OP_REQUEST == p_pair->op){
		if(pair_par_len == sizeof(sh_pair_req_t)){
			// switch
			smart_home_pair_tx(PAIR_OP_REQUEST, p_pair->par, sizeof(sh_pair_req_t));
		}else{
			smart_home_pair_start_app((sh_pair_req_t *)p_pair->par);
		}
	}
}
#endif

void smart_home_led_event_callback (led_event_t ev)
{
#ifdef WIN32
	return ;
#endif

	if(LED_EVENT_PAIR_SUCCESS == ev){
        cfg_led_event(LED_EVENT_FLASH_1HZ_4S);
    }else if(LED_EVENT_SETTING == ev){
        cfg_led_event(LED_EVENT_FLASH_2HZ_2S);
	}else if(LED_EVENT_DEL_NODE == ev){
        cfg_led_event(LED_EVENT_FLASH_1HZ_4S);
	}else if(LED_EVENT_SLAVE_MODE == ev){
        cfg_led_event(LED_EVENT_FLASH_0p25HZ_1T);
	}else if(LED_EVENT_KEY_PRESS == ev){
        cfg_led_event(LED_EVENT_FLASH_4HZ_1T);
	}
}

#if !WIN32
extern int blt_send_adv(void);

u32 adv_in_conn_tick = 0;
u8 adv_in_conn_tick_init_flag = 0;

void smart_home_gatt_connected_cb()
{
    adv_in_conn_tick = clock_time();
    LOG_MSG_LIB (TL_LOG_PROTOCOL, 0, 0, "gatt connect");
}

void smart_home_gatt_disconnected_cb()
{
    LOG_MSG_LIB (TL_LOG_PROTOCOL, 0, 0, "gatt disconnect");
}

#if ((MCU_CORE_TYPE == MCU_CORE_5316) || (MCU_CORE_TYPE == MCU_CORE_5317))
extern st_ll_conn_slave_t		bltc;

int rf_link_time_allow (u32 us)
{
	u32 t = (u32)(reg_tmr0_capt - reg_tmr0_tick) / SYS_TICK_DIV - us * sys_tick_per_us;
	return t < BIT(30);
}
#else
int rf_link_time_allow (u32 us)
{
	u32 t = reg_system_tick_irq - clock_time () - us * sys_tick_per_us;
	return t < BIT(30);
}
#endif

void adv_send_proc_GATT_connected()
{
#if 1
	if(is_blt_gatt_connected() && (BLE_STATE_BRX_E == blt_ll_getCurrentBLEState()||BLE_STATE_PRICHN_SCAN_S == blt_ll_getCurrentBLEState()) && rf_link_time_allow(4000)){ // need 3500us to send online status,
		u32 adv_intv_us = blt_ll_getAdvIntervalTick()/sys_tick_per_us;
	    if(clock_time_exceed(adv_in_conn_tick, adv_intv_us)){
	        if(adv_in_conn_tick_init_flag || clock_time_exceed(adv_in_conn_tick, (adv_intv_us + 8*1000))){ // 8*1000: because need 13ms to  handle onoff - rsp when 16MHz 
                adv_in_conn_tick = clock_time(); // fix the case that duration is too long.
                adv_in_conn_tick_init_flag = 0;
	        }else{
    		    adv_in_conn_tick += blt_ll_getAdvIntervalTick();
    		}
    		
    		// ---- blt_send_adv2scan_mode
    		#if 1
			u8 r = irq_disable();
			//static u32 cnt_gatt_adv_sent;
			//LOG_MSG_LIB (TL_LOG_PROTOCOL, 0, 0, "cnt: %d. ", ++cnt_gatt_adv_sent);
			u8 blt_st_backup = blc_ll_getCurrentState();
			blt_ll_setCurrentState(0); // set to 0 temporary to skip some time setting in blt send adv().
			blt_send_adv();
			blt_ll_setCurrentState(blt_st_backup);
			extern u8 tx_settle_slave[3];
				#if 0// (LL_FEATURE_ENABLE_LE_2M_PHY)
			tx_settle_adjust(tx_settle_slave[blt_conn_phy.conn_cur_phy]);
				#else
			tx_settle_adjust(tx_settle_slave[1]); // LL_SLAVE_TX_SETTLE_1M
				#endif
			irq_restore(r);
				#if SMART_HOME_SCAN_IN_CONNECT_ST_EN
			//if( bltParam.scan_extension_mask & BLS_FLAG_SCAN_IN_SLAVE_MODE )
			{
				//if(ll_enter_pri_chn_scan_cb) //not judge to save ramcode 4 byte
				{
					ll_enter_pri_chn_scan_cb(); // blt_setScan_enter_manaul_rx()
				}
			}
				#endif
    		#else
			CLEAR_ALL_RFIRQ_STATUS;
			u8 r = irq_disable();
			u32 exp_backup = bltc.connExpectTime;
			bltc.connExpectTime = reg_tmr0_capt / SYS_TICK_DIV;	// fix bug of blt_adjust_timer0_capt() in blt_send_adv_ that capture is not the same as expect time.
			blt_send_adv();
			bltc.connExpectTime = exp_backup;
			rf_adjust_tx_settle(LL_SLAVE_TX_SETTLE_1M);
			irq_restore(r);

			if( blts.scan_extension_mask & BLS_FLAG_SCAN_IN_SLAVE_MODE ){
				blc_ll_switchScanChannel(0, 0);
			}
			#endif
			// ---- blt_send_adv2scan_mode end
		}
    }
#endif
}
#endif

// ---- loop -----
void smart_home_loop()
{
#if !WIN32
	adv_send_proc_GATT_connected();
#endif
	proc_ui_smart_home();
	proc_led();
#if ((0 == BLE_REMOTE_PM_ENABLE) && (0 == BLE_REMOTE_PM_SIMPLE_EN) && (0 == SMART_HOME_CLIENT_SW_EN) && (!WIN32))
	factory_reset_cnt_check();
#endif
	
#if SMART_HOME_CLIENT_APP_EN
	smart_home_pair_app_loop();
#endif
#if SMART_HOME_SERVER_EN
	smart_home_pair_server_loop();
	#if SMART_HOME_ONOFF_EN
	light_onoff_step_timer_proc();
	smart_home_lum_save_proc();
	#endif
	#if SMART_HOME_TIME_EN
	rtc_run();
	#endif
#endif
#if SMART_HOME_CLIENT_SW_EN
	smart_home_switch_slave_proc();
#endif
}

#if SMART_HOME_CLIENT_SW_EN
void smart_sno_retrive()
{
    if(!analog_read(SMART_HOME_XXX_REG)){
        smart_home_tx_cmd_sno = (u16)(rand_sh());
        analog_write(SMART_HOME_SW_SNO_REG, smart_home_tx_cmd_sno);
    }else{
        smart_home_tx_cmd_sno = analog_read(SMART_HOME_SW_SNO_REG);
    }
}
#endif

// ---- init -----
void smart_home_init()
{
	LOG_MSG_LIB (TL_LOG_PROTOCOL, 0, 0, "start, init ...", __FUNCTION__); // LOG_MSG_FUNC_NAME();
	// global init
#if SMART_HOME_SERVER_EN
	set_all_group_id_invalid();
	#if 1
		#if 1//(MCU_CORE_TYPE == MCU_CORE_8208)
	bls_ll_continue_adv_after_scan_req(1);  // new API
		#else
	blt_enable_adv_after_scan_req(1);		// API for 8232
		#endif
	set_connectable_adv_interval_with_random();
	#endif
#endif

	// init
#if (0 == SMART_HOME_CLIENT_APP_EN)
	bls_set_advertise_prepare(app_advertise_prepare_handler_smart_home);
	#if SMART_HOME_SERVER_EN
	u8 *tbl_mac2 = blc_ll_get_macAddrPublic();
	blc_ll_initScanning_moduleSmartHome(tbl_mac2);
	#endif
#endif

	// read parameters
#if (SMART_HOME_CLIENT_SW_EN)
	smart_home_switch_init();
    smart_sno_retrive();
#endif
    
#if !WIN32
	if(is_state_after_ota()){
		clr_keep_onoff_state_after_ota();
	}
#endif

#if (SMART_HOME_SERVER_EN || SMART_HOME_CLIENT_SW_EN)
	smart_home_pair_key_retrieve();
#endif
#if SMART_HOME_SERVER_EN
	smart_home_group_id_retrieve();
	#if SMART_HOME_ONOFF_EN
	smart_home_lum_retrieve();
	#endif
	#if(SMART_HOME_SCENE_EN)
	smart_home_scene_retrieve();
	#endif

	#if !WIN32
	user_init_pwm(0);	// must after lum retrive
	//MTU Size
    blc_att_setRxMtuSize(SMART_HOME_MTU_SIZE);
    // blc_att_registerMtuSizeExchangeCb(mtu_size_exchange_cb);
    #endif
#endif
}


// -------------- save parameters to flash ---------
#if (SMART_HOME_SERVER_EN)
#if WIN32
#define KEY_SAVE_ENCODE_ENABLE  1
#else
#define KEY_SAVE_ENCODE_ENABLE  1
#endif

#if (0 == SMART_HOME_KITE_TEST_EN)
static u16 flash_write_err_cnt;
int mesh_flash_write_check(u32 adr, const u8 *in, u32 size)
{
	u8 data_read[FLASH_CHECK_SIZE_MAX];
	u32 len = size;
	u32 pos = 0;
	while(len){
		// just only 2 bytes different generally.because it just happen when add / del subscription adr.
		u32 len_read = len > FLASH_CHECK_SIZE_MAX ? FLASH_CHECK_SIZE_MAX : len;
		u32 adr_read = adr + pos;
		const u8 *p_in = in+pos;
		flash_read_page(adr_read, len_read, data_read);
		if(memcmp(data_read, p_in, len_read)){
			flash_write_err_cnt++;
			return -1;
		}
		
		len -= len_read;
		pos += len_read;
	}
	return 0;
}

void flash_write_with_check(u32 adr, u32 size, const u8 *in)
{
	flash_write_page(adr, size, (u8 *)in); // par "in" is in RAM, followed by const just indicating should not be changed.
	mesh_flash_write_check(adr, in, size);
}

/*
	void mesh_par_store((u8 *in, u32 *p_adr, u32 adr_base, u32 size)):
	input parameters:
	in: just parameters, no save flag
*/
void mesh_par_write_with_check(u32 addr, u32 size, const u8 *in)
{
	// make sure when change to the save_flag ,the flash must be write already .
	u32 save_flag = SAVE_FLAG_PRE;
	flash_write_with_check(addr, SIZE_SAVE_FLAG, (u8 *)&save_flag);
	flash_write_with_check(addr + SIZE_SAVE_FLAG, size, in);
	save_flag = SAVE_FLAG;
	flash_write_with_check(addr, SIZE_SAVE_FLAG, (u8 *)&save_flag);
}
#endif

static int is_valid_par_addr(u32 addr)
{
    return ((addr >= FLASH_ADDR_SMART_HOME_START) && (addr < FLASH_ADDR_SMART_HOME_END));
}

void smart_home_par_store(const u8 *in, u32 *p_adr, u32 adr_base, u32 size){
    if(!is_valid_par_addr(*p_adr) || !is_valid_par_addr(adr_base) || (*p_adr < adr_base)){ // address invalid
        sleep_us(500*1000);
        while(1){ // can't reboot directly without handling sno, because sno will increase 0x80 for each reboot.
            #if(MODULE_WATCHDOG_ENABLE)
            wd_clear();
            #endif
        };
    }
    
	u32 size_save = (size + SIZE_SAVE_FLAG);
	if(*p_adr > (adr_base + FLASH_SECTOR_SIZE - size_save - SIZE_SAVE_FLAG)){	// make sure "0xffffffff" at the last for retrieve
        *p_adr = adr_base;
        flash_erase_sector(adr_base);
	}
		
	mesh_par_write_with_check(*p_adr, size, in);
	if(*p_adr >= adr_base + size_save){
    	u32 adr_last = *p_adr - size_save;
		u32 zero = 0;
    	flash_write_page(adr_last, SIZE_SAVE_FLAG, (u8 *)&zero);  // clear last flag
	}

	*p_adr += size_save;
}

int smart_home_par_retrieve(u8 *out, u32 *p_adr, u32 adr_base, u32 size)
{
    int err = -1;
    for(int i = 0; i < (FLASH_SECTOR_SIZE); i += (size + SIZE_SAVE_FLAG)){
		*p_adr = adr_base + i;

        u8 save_flag;
        flash_read_page(*p_adr, 1, &save_flag);
		if(SAVE_FLAG == save_flag){
            flash_read_page(*p_adr + SIZE_SAVE_FLAG, size, out);
            err = 0;
		}else if (SAVE_FLAG_PRE == save_flag){
			continue;
		}else if(save_flag != 0xFF){
		    continue;   //invalid
		}else{
		    break;      // 0xFF: end
		}
	}
	return err;
}

STATIC_ASSERT(sizeof(smart_home_pair_key_info) % 4 == 0);
STATIC_ASSERT(sizeof(pair_key_save_t) % 4 == 0);


static u32 smart_home_key_addr = FLASH_ADDR_SMART_HOME_PAIR_KEY;

#if KEY_SAVE_ENCODE_ENABLE
#define SAVE_ENCODE_SK_USER         "0123456smartxl9c"    // MAX 16 BYTE

#define KEY_SAVE_ENCODE_NONCE  		"TlnkSH22"

int encode_decode_password(pair_key_save_t *p_save, int encode)
{
	int err = 0;
    u8 sk_user[16+1] = {SAVE_ENCODE_SK_USER};
    u8 iv[8+1] = {KEY_SAVE_ENCODE_NONCE};
    u16 len = OFFSETOF(pair_key_save_t, mic_val);
    int mic_len = sizeof(p_save->mic_val);
    if(encode){
		aes_att_encryption_packet_smart_home(sk_user, iv, p_save->mic_val, mic_len, (u8 *)p_save, len);
    }else{
		err = (0 == aes_att_decryption_packet_smart_home(sk_user, iv, p_save->mic_val, mic_len, (u8 *)p_save, len));
    }
	return err;
}
#endif

void smart_home_pair_key_save()
{
	pair_key_save_t key_save = {{0}};
	memcpy(key_save.key_info, smart_home_pair_key_info, sizeof(key_save.key_info));
	#if SMART_HOME_ONLINE_ST_EN
	key_save.online_st_ver = factory_reset_version;
	#endif
	#if SMART_HOME_CLIENT_SW_EN
	memcpy(&key_save.key_sw_master, &smart_home_pair_key_switch, sizeof(key_save.key_sw_master));
	#endif
    #if KEY_SAVE_ENCODE_ENABLE
    encode_decode_password(&key_save, 1);
    key_save.encode_flag = KEY_SAVE_ENCODE_FLAG;
    #endif
    smart_home_par_store((u8 *)&key_save, &smart_home_key_addr, FLASH_ADDR_SMART_HOME_PAIR_KEY, sizeof(key_save));
}

int smart_home_pair_key_retrieve()
{
	pair_key_save_t key_save = {{0}};
    int err = smart_home_par_retrieve((u8 *)&key_save, &smart_home_key_addr, FLASH_ADDR_SMART_HOME_PAIR_KEY, sizeof(key_save));
    
    #if KEY_SAVE_ENCODE_ENABLE
    if((0 == err) && KEY_SAVE_ENCODE_FLAG == key_save.encode_flag){
        err = encode_decode_password(&key_save, 0);
    }
    #endif

	if(err){
		sh_server_pair_key_info_t *p_info_1st = &smart_home_pair_key_info[0];
		smart_home_get_factory_key(p_info_1st);
		// p_info_1st->unicast_addr = ;	// no need. will auto get mac if 0.
		flag_factory_key_mode = 1;
	}else{
		memcpy(smart_home_pair_key_info, key_save.key_info, sizeof(smart_home_pair_key_info));
		#if SMART_HOME_CLIENT_SW_EN
		memcpy(&smart_home_pair_key_switch, &key_save.key_sw_master, sizeof(smart_home_pair_key_switch));
		#endif
		#if SMART_HOME_ONLINE_ST_EN
		factory_reset_version = key_save.online_st_ver;
		#endif
	}
	
    return err;
}

static u32 smart_home_group_id_addr = FLASH_ADDR_SMART_HOME_GROUP_ID;

void smart_home_group_id_save()
{
    smart_home_par_store((u8 *)&smart_home_group_id, &smart_home_group_id_addr, FLASH_ADDR_SMART_HOME_GROUP_ID, sizeof(smart_home_group_id));
}

int smart_home_group_id_retrieve()
{
    int err = smart_home_par_retrieve((u8 *)&smart_home_group_id, &smart_home_group_id_addr, FLASH_ADDR_SMART_HOME_GROUP_ID, sizeof(smart_home_group_id));
	if(err){
	}
	
    return err;
}
#endif


/////////////////////////////////////////spp/////////////////////////////////////
#if (TELIK_SPP_SERVICE_ENABLE)
#if SMART_HOME_SERVER_EN
rx_cb_err_t smart_home_GATT_rx_handle(void *p, int pair_flag)
{
	rx_cb_err_t err = RX_CB_SUCCESS;
	rf_packet_att_write_t *p_rx = (rf_packet_att_write_t *)p;
	u8 len = p_rx->l2capLen - 3;
	// LOG_MSG_LIB(TL_LOG_PROTOCOL, (u8 *)&p_rx->value, len,"---------GATT rx:");
	if(len <= sizeof(smart_home_nw_t)){
		smart_home_adv_payload_t adv_payload = {0};
		smart_home_adv_payload_t *p_payload = &adv_payload;
		p_payload->type = GAP_ADTYPE_MANUFACTURER_SPECIFIC;
		p_payload->vendor_id = VENDOR_ID;
		p_payload->vendor_id2 = pair_flag ? VENDOR_ID_PAIR : VENDOR_ID;
		memcpy(&p_payload->pair, &p_rx->value, len);
		p_payload->len = ADV_LEN_GATT_LEN_DELTA + len;
		err = smart_home_cmd_rx_cb(p_payload, RX_SOURCE_GATT);
	}else{
		err = RX_CB_ERR_PAR_LEN;
	}

	return err;
}
#else
rx_cb_err_t smart_home_GATT_rx_handle(void *p, int pair_flag)
{
	return RX_CB_SUCCESS;
}
#endif

int smart_home_GATT_rx_command(void *p)
{
	u8 *data = ((rf_packet_att_write_t *)p)->value;

	switch (data[0]){

	case 0x01: light_onoff(1); break;
	case 0x02: light_onoff(0); break;
	case 0x03: light_adjust_lightness(data[1]); break;
	case 0x04: light_adjust_CT(data[2], data[1]); break;
	}

	return 0;
}

int smart_home_GATT_rx_pair(void *p)
{
	smart_home_GATT_rx_handle(p, 1);

	return 0;
}
#endif




// ------------- smart home switch ----------------
#if SMART_HOME_CLIENT_SW_EN
smart_home_switch_mode_t switch_mode = SWITCH_MODE_MASTER;
u32 switch_slave_mode_tick	= 0;
u8 switch_quick_exit_flag	= 0;		// exit from slave mode
static u8 sw_onoff_cnt;

#if (PCBA_8208_SEL == C1T261A30_V1)
void test_smart_home_cmd_tx_onoff_toggle()
{
	lcmd_onoff_t cmd = {0};
	cmd.set_mode = (sw_onoff_cnt++) & 1;	// light OFF first
	cmd.delay_10ms = 0;
	cmd.ack = 0;
	
#if 0 // use factory key
	smart_home_cmd_tx_switch_factory_key(LIGHT_CMD_ONOFF_SET, SMART_HOME_GROUP_ALL, (u8 *)&cmd, sizeof(cmd));
#else
	smart_home_cmd_tx_switch(LIGHT_CMD_ONOFF_SET, smart_home_pair_key_switch.req.group_id_sw, (u8 *)&cmd, sizeof(cmd));
#endif

	smart_home_led_event_callback(LED_EVENT_KEY_PRESS);
}
#endif

#if 1
/**
 * @brief       This function switch send onoff cmd
 * @param[in]   addr_dst	- destination address
 * @param[in]   onoff_mode	- onoff action
 * @param[in]   delay_10ms	- Command response delay
 * @param[in]   ack			- acknowledged, 1: need node to respond; 0: no need.
 * @return      
 * @note        
 */
tx_cmd_err_t smart_home_cmd_tx_onff(u16 addr_dst, onoff_set_mode_t onoff_mode, u16 delay_10ms, ack_flag_t ack)
{
	lcmd_onoff_t cmd = {0};
	cmd.set_mode = onoff_mode;
	cmd.delay_10ms = delay_10ms;
	cmd.ack = ack;
	
	return smart_home_cmd_tx_switch(LIGHT_CMD_ONOFF_SET, addr_dst, (u8 *)&cmd, sizeof(cmd));
}

/**
 * @brief       This function switch send lightness cmd
 * @param[in]   addr_dst	- destination address
 * @param[in]   lightness	- lightness value
 * @param[in]   set_mode	- linght action
 * @param[in]   ack			- acknowledged, 1: need node to respond; 0: no need.
 * @return      
 * @note        
 */
tx_cmd_err_t smart_home_cmd_tx_lightness(u16 addr_dst, u8 lightness, light_set_mode_t set_mode, ack_flag_t ack)
{
	lcmd_lightness_t cmd = {0};
	cmd.ack = ack;
	cmd.lightness = lightness;
	cmd.set_mode = set_mode;
	//cmd.rsv_bit = 0; // has been init

	return smart_home_cmd_tx_switch(LIGHT_CMD_LIGHTNESS_SET, addr_dst, (u8 *)&cmd, sizeof(cmd));
}

/**
 * @brief       This function switch send lightness and ct cmd
 * @param[in]   addr_dst	- destination address
 * @param[in]   lightness	- lightness value
 * @param[in]   ct			- ct value
 * @param[in]   set_mode	- linght and ct action
 * @param[in]   ack			- acknowledged, 1: need node to respond; 0: no need.
 * @return      
 * @note        
 */
static tx_cmd_err_t smart_home_cmd_tx_lightness_ct_2(u16 addr_dst, u8 lightness, u8 ct, light_set_mode_t set_mode, ack_flag_t ack)
{
	lcmd_CTL_t cmd = {0};
	cmd.ack = ack;
	cmd.lightness = lightness;
	cmd.lightness_flag = (FLAG_NO_LIGHTNESS != lightness);
	cmd.ct = ct;
	cmd.ct_flag = (FLAG_NO_CT != ct);
	//cmd.rsv_bit = 0; // has been init
	cmd.set_mode = set_mode;
	
	return smart_home_cmd_tx_switch(LIGHT_CMD_LIGHTNESS_CT_SET, addr_dst, (u8 *)&cmd, sizeof(cmd));
}

/**
 * @brief       This function switch send lightness and ct cmd
 * @param[in]   addr_dst	- destination address
 * @param[in]   lightness	- lightness value
 * @param[in]   ct			- ct value
 * @param[in]   set_mode	- linght and ct action
 * @param[in]   ack			- acknowledged, 1: need node to respond; 0: no need.
 * @return      
 * @note        
 */
tx_cmd_err_t smart_home_cmd_tx_lightness_and_ct(u16 addr_dst, u8 lightness, u8 ct, light_set_mode_t set_mode, ack_flag_t ack)
{
	return smart_home_cmd_tx_lightness_ct_2(addr_dst, lightness, ct, set_mode, ack);
}

/**
 * @brief       This function switch only send ct cmd
 * @param[in]   addr_dst- destination address
 * @param[in]   ct		- ct value
 * @param[in]   set_mode- ct action
 * @param[in]   ack		- acknowledged, 1: need node to respond; 0: no need.
 * @return      
 * @note        
 */
tx_cmd_err_t smart_home_cmd_tx_ct(u16 addr_dst, u8 ct, light_set_mode_t set_mode, ack_flag_t ack)
{
	return smart_home_cmd_tx_lightness_ct_2(addr_dst, FLAG_NO_LIGHTNESS, ct, set_mode, ack);
}
#endif


#if TEST_SWITCH_KEY_FUNCTION_2_EN
void test_smart_home_cmd_tx_scene_add()
{
	lcmd_scene_add_no_par_t cmd = {0};
	cmd.id = 0x64;
	cmd.ack = 0;
	
	smart_home_cmd_tx_switch(LIGHT_CMD_SCENE_ADD_NO_PAR, smart_home_pair_key_switch.req.group_id_sw, (u8 *)&cmd, sizeof(cmd));
	smart_home_led_event_callback(LED_EVENT_KEY_PRESS);
}

void test_smart_home_cmd_tx_scene_call()
{
	lcmd_scene_add_no_par_t cmd = {0};
	cmd.id = 0x64;
	cmd.ack = 0;
	
	smart_home_cmd_tx_switch(LIGHT_CMD_SCENE_CALL, smart_home_pair_key_switch.req.group_id_sw, (u8 *)&cmd, sizeof(cmd));
	smart_home_led_event_callback(LED_EVENT_KEY_PRESS);
}

int is_test_key_function_2()
{
	return (0xA3A3A3A3 == *(u32 *)0x60000);
}
#endif

void led_event_slave_mode_check_and_trigger()
{
	if(SWITCH_MODE_MASTER != switch_mode){
		flag_can_not_pair = 0;
		switch_slave_mode_tick = clock_time() |  1;
		smart_home_led_event_callback(LED_EVENT_SLAVE_MODE); // will call LED_EVENT_SLAVE_MODE again in smart_home_loop_.
	}
}

void smart_home_set_switch_mode(smart_home_switch_mode_t mode)
{
	switch_mode = mode;	// may call LED_EVENT_SLAVE_MODE in smart_home_loop_.
	led_event_slave_mode_check_and_trigger();
}

void smart_home_toggle_switch_mode()
{
	switch_mode = !switch_mode;
	led_event_slave_mode_check_and_trigger();
}

void smart_home_switch_quick_exit_slave_mode_check()
{
	if(switch_quick_exit_flag && switch_slave_mode_tick && (SWITCH_MODE_MASTER != switch_mode)){
		switch_quick_exit_flag = 0;
		switch_slave_mode_tick = (clock_time() - (SWITCH_SLAVE_TIMEOUT_MS - 3000)*(1000*sys_tick_per_us))|1;
	}
}

void smart_home_switch_slave_proc()
{
	if(SWITCH_MODE_MASTER == switch_mode){
		#if BLE_REMOTE_PM_SIMPLE_EN
		//LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"---------enter sleep check: %d,%d", !key_pressing_cnt, is_smart_home_tx_cmd_empty());
		if(!key_pressing_cnt && is_smart_home_tx_cmd_empty()){
			//LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"---------enter deep sleep");
			analog_write(SMART_HOME_SW_KEY_PAR_REG, sw_onoff_cnt);
			cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  //deepsleep
		}
		#endif
	}else{
		if(switch_slave_mode_tick && clock_time_exceed(switch_slave_mode_tick, SWITCH_SLAVE_TIMEOUT_MS * 1000)){
			if(is_smart_home_tx_cmd_empty() && !is_blt_gatt_connected()){
				switch_slave_mode_tick = 0;
				smart_home_set_switch_mode(SWITCH_MODE_MASTER);
			}
		}
		
		if(!is_led_busy()){
			smart_home_led_event_callback(LED_EVENT_SLAVE_MODE);
		}
	}
}

void smart_home_switch_init()
{
	// LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"---------switch init");
    if(is_ota_success_reboot_flag()){
		// LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"---------is_ota_success_reboot_flag");
    	clr_ota_success_reboot_flag();
		smart_home_set_switch_mode(SWITCH_MODE_SLAVE);	// to receive composition data get after OTA success.
		switch_quick_exit_flag = 1;
    }
    
	sw_onoff_cnt = analog_read(SMART_HOME_SW_KEY_PAR_REG);
}

#endif


/**
  * @}
  */

