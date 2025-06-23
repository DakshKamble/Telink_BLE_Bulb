/********************************************************************************************************
 * @file	protocol.h
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
#include "pair_encryption.h"
#include "app_and_gw/protocol_app_gw.h"
#else
#include "tl_common.h"
#include "log_module.h"
#include "stack/ble/ble.h"
#endif
#include "pair_encryption.h"
#include "light_driver.h"

#if WIN32
#pragma pack(1)
#endif

#ifndef SMART_HOME_CLIENT_APP_EN
#define SMART_HOME_CLIENT_APP_EN			0	// app or gateway
#endif
#ifndef SMART_HOME_CLIENT_SW_EN
#define SMART_HOME_CLIENT_SW_EN				0	// switch
#endif
#ifndef SMART_HOME_SERVER_EN
#define SMART_HOME_SERVER_EN				0
#endif
#ifndef SMART_HOME_KITE_TEST_EN
#define SMART_HOME_KITE_TEST_EN				0
#endif
#ifndef FEATURE_RELAY_EN
#define FEATURE_RELAY_EN				    0
#endif

// flash map setting

#ifndef FLASH_ADDR_SMART_HOME_PAIR_KEY
// firmware area ---------------------------(0x00000 ~ 0x1FFFF)
// OTA area --------------------------------(0x20000 ~ 0x3FFFF)
// area not used now -----------------------(0x40000 ~ 0x5FFFF)
	#if ((FLASH_SIZE_OPTION == FLASH_SIZE_OPTION_512K) || SMART_HOME_KITE_TEST_EN)
// reserve for smart home SDK 	--------	(0x60000 ~ FLASH_ADDR_SMART_HOME_START)
#define FLASH_ADDR_SMART_HOME_START			(0x6A000)	// ----- start ------

#define FLASH_ADDR_SMART_HOME_SCHEDULER_SAVE (FLASH_ADDR_SMART_HOME_START)
#define FLASH_ADDR_SMART_HOME_SCENE_SAVE	(0x6B000)
#define FLASH_ADDR_SMART_HOME_LUM_SAVE		(0x6C000)
#define FLASH_ADDR_SMART_HOME_GROUP_ID		(0x6D000)
#define FLASH_ADDR_SMART_HOME_PAIR_KEY		(0x6E000)
#define FLASH_ADR_RESET_CNT					(0x6F000)

#define FLASH_ADDR_SMART_HOME_END			(0x70000)	// -----  end -------
// reserve for generic SDK 	----------------(0x70000 ~ 0x73FFF)
// SMP1 ------------------------------------(0x74000)	// SMP_PARAM_NV_ADDR_START
// SMP2 ------------------------------------(0x75000)
// mac 	------------------------------------(0x76000)
// DC, TP ----------------------------------(0x77000)
// telink reserve --------------------------(0x78000 ~ 0x79FFF)

// USER define area: -----------------------(0x7A000 ~ 0x7FFFF)
	#elif (FLASH_SIZE_OPTION == FLASH_SIZE_OPTION_128K)
// reserve for smart home SDK 	--------	(0x14000 ~ FLASH_ADDR_SMART_HOME_START)
#define FLASH_ADDR_SMART_HOME_START			(0x16000)	// ----- start ------
#define FLASH_ADDR_SMART_HOME_SCHEDULER_SAVE (FLASH_ADDR_SMART_HOME_START)
#define FLASH_ADDR_SMART_HOME_SCENE_SAVE	(0x17000)
#define FLASH_ADDR_SMART_HOME_LUM_SAVE		(0x18000)
#define FLASH_ADDR_SMART_HOME_GROUP_ID		(0x19000)
#define FLASH_ADDR_SMART_HOME_PAIR_KEY		(0x1A000)
#define FLASH_ADR_RESET_CNT					(0x1B000)

#define FLASH_ADDR_SMART_HOME_END			(0x1C000)	// -----  end -------

// USER define area: -----------------------(0x1c000 ~ 0x1d000)
// no SMP
// DC   ------------------------------------(0x1e000)
// mac 	------------------------------------(0x1f000)
	#endif
#endif

// connectable ADV packet parameters setting
#define CONNECTABLE_ADV_INV_MS				(160)		// connectable ADV interval, unit: ms. and auto add 30ms interval.
#define CONNECTABLE_ADV_INV_RANDOM_MS		(30)		// unit: ms

// network parameters settings
#define SMART_HOME_TX_CMD_FIFO_CNT			(4)			// 
#define SMART_HOME_BLT_RX_FIFO_CNT			(16)		// 
#define SMART_HOME_BLT_TX_FIFO_CNT			(8)			// 

#if SMART_HOME_CLIENT_SW_EN
// should be better to send more quickly when tx command to nodes.
#define TX_CMD_INTERVAL_MIN					(ADV_INTERVAL_20MS)	// require >= 20ms for BLE spec.
#define TX_CMD_INTERVAL_MAX					(ADV_INTERVAL_25MS)
#define TX_CMD_PACKET_CNT					(20)		// must >= 1;  0 means no packet to send.
#else
#define TX_CMD_INTERVAL_MIN					(ADV_INTERVAL_20MS)
#define TX_CMD_INTERVAL_MAX					(ADV_INTERVAL_30MS)
#define TX_CMD_PACKET_CNT					(20)		// must >= 1;  0 means no packet to send.
#endif

#define SMART_HOME_SCAN_INTERVAL_MS			(40)		// unit: ms
#define SMART_HOME_SERVER_PAIR_KEY_MAX		(8)			// 

#define PAIR_TIMEOUT_POWER_UP_MS			(60*1000)	// unit: ms // if timeout after powerup, node will not receive "pair request" any more.
#define PAIR_RX_UNICAST_ADDR_TIMEOUT_MS		(60*1000)	// unit: ms // app may collect all "mac response" and wait for user to select and start.
#define PAIR_TX_CONFIRM_TIMEOUT_MS			(4*1000)	// unit: ms // if not receive "unicast address" message, it means that app has received "confirm".

/* group ID range 0~0xFE(SMART_HOME_GROUP_ID_MAX). 0xFF(SMART_HOME_GROUP_ALL) */
#define SMART_HOME_GROUP_CNT_MAX			(12)

// ------ test configuration -------
#define P_TX_KEY_TEST						(&smart_home_pair_key_info[0])	// just for test now
#define SMART_HOME_ENCRYPTION_EN			(1)			// 0 for test

// ------ switch configuration -------
#if SMART_HOME_CLIENT_SW_EN
#define SWITCH_SLAVE_TIMEOUT_MS				(60*1000)
#endif

// ------
#define TX_CMD_TRANSMIT_CNT				(TX_CMD_PACKET_CNT - 1)
#define TX_CMD_INTERVAL_AVERAGE_MS		((((TX_CMD_INTERVAL_MAX + TX_CMD_INTERVAL_MIN)/2 * TX_CMD_PACKET_CNT * 5) / 8))

#define	ADDR_LOCAL						(0)
#define	ADDR_ALL_NODES					(0xFFFF)

#define	VENDOR_ID_IOS					(0x004C)
#define	VENDOR_ID_PAIR					((~ VENDOR_ID)&0xffff)
#define	VENDOR_ID_ONLINE_ST				(0x5AA5)

#define	iBEACON_FLAG					(0x1502)

#define GROUP_ID_APP_START				(0x8000)
#define GROUP_ID_APP_END				(0xEFFF)
#define GROUP_ID_SWITCH_START			(0xF000)
#define GROUP_ID_SWITCH_G_F000			(0xF000)
#define GROUP_ID_SWITCH_END				(0xF800)
#define SMART_HOME_GROUP_ALL			(0xFFFF)	// means all devices
#define INVALID_GROUP_ID_SET			(0x0000)

#define SIZE_MIC						(2)

#define SMART_HOME_MTU_SIZE      		30

#define FLAG_NO_LIGHTNESS				(255)
#define FLAG_NO_CT						(255) // no color temperature

// ---- save parameters to flash -----
#define SIZE_SAVE_FLAG					(4)
#define FLASH_CHECK_SIZE_MAX			(64)
#define	FLASH_SECTOR_SIZE       		(4096)
#define SAVE_FLAG_PRE					(0xAF)
#define SAVE_FLAG       				(0xA5)


#define GROUP_MASK						(BIT(15))

typedef enum{
	ADDR_MATCH_TYPE_NONE				= 0,
	ADDR_MATCH_TYPE_MAC					= 1,
	ADDR_MATCH_TYPE_GROUP				= 2,
}addr_match_type_t;

typedef enum{
	LED_EVENT_PAIR_SUCCESS				= 0,
	LED_EVENT_SETTING,							// include group setting, scene setting, etc.
	LED_EVENT_DEL_NODE,
	LED_EVENT_SLAVE_MODE,
	LED_EVENT_KEY_PRESS,
}led_event_t;


typedef enum{
	RX_CB_SUCCESS						= 0,
	RX_CB_ERR_DATA_TYPE					= 1,
	RX_CB_ERR_PAR_LEN					= 2,
	RX_CB_ERR_DECRYPTION_NO_KEY_FOUND	= 3,	// no 'nid' matched found.
	RX_CB_ERR_DECRYPTION_FAILED			= 4,	// found some 'nid' matched, but decryption failed.
	RX_CB_ERR_SNO_REPLAY				= 5,
	RX_CB_ERR_ADDR_SRC_SELF				= 6,
	RX_CB_ERR_ADDR_DST_NOT_MATCH		= 7,
	RX_CB_ERR_NW_LEN					= 8,
	RX_CB_ERR_PROXY_INI_TX				= 9,
	RX_CB_ERR_OUTOF_MEMORY				= 0x0a,
	RX_CB_ERR_HAVE_BEEN_PAIRED			= 0x0b,
	RX_CB_ERR_NOT_PAIR_TIME_WINDOW		= 0x0c, // flag_can_not_pair // due to time out of PAIR_TIMEOUT_POWER_UP_MS, need to power off and then power on to enter pair time window.
	RX_CB_ERR_PAIR_DEF_KEY_BUT_NOT_REQ	= 0x0d,	// use default key, but not pair request command.
	RX_CB_ERR_PAIR_ERR_REQ				= 0x0e,	// 
	RX_CB_ERR_GROUP_ID_INVALID			= 0x0f,	// 
	RX_CB_ERR_GROUP_FULL				= 0x10,	// 
	RX_CB_ERR_GROUP_ID_NOT_FOUND		= 0x11,	// 
	RX_CB_ERR_SCENE_ID_INVALID			= 0x12,	// 
	RX_CB_ERR_SCENE_ID_FULL				= 0x13,	// 
	RX_CB_ERR_SCENE_ID_NOT_FOUND		= 0x14,	// 
	RX_CB_ERR_TX_RSP_FAILED				= 0x15,	// 
	RX_CB_ERR_TIME_INVALID				= 0x16,	// 
}rx_cb_err_t;

typedef enum{
	TX_CMD_SUCCESS						= 0,
	TX_CMD_ERR_PAR_LEN					= 0x80,	// follow max value of rx_cb_err_t to print error string
	TX_CMD_ERR_KEY_INVALID				= 0x81,
	TX_CMD_ERR_INVALID_SW_GROUP_ID		= 0x82,
	TX_CMD_ERR_FIFO_FULL				= 0x83,
}tx_cmd_err_t;

typedef enum{
	RX_SOURCE_ADV						= 0,
	RX_SOURCE_GATT						= 1,
	RX_SOURCE_TX						= 2,
#if SMART_HOME_CLIENT_APP_EN
	RX_SOURCE_INI						= 3,
#endif
}rx_source_type_t;

typedef enum{
	NO_ACK		= 0,
	NEED_ACK	= 1,
}ack_flag_t; // message ack flag

// op code
#define LIGHT_CMD_RESERVE						0x00	// not use now.
#define LIGHT_CMD_ONOFF_SET						0x01
#define LIGHT_CMD_ONOFF_STATUS					0x02
#define LIGHT_CMD_LIGHTNESS_SET					0x03
#define LIGHT_CMD_LIGHTNESS_STATUS				0x04
#define LIGHT_CMD_LIGHTNESS_CT_SET				0x05	// CT: color temperature
#define LIGHT_CMD_LIGHTNESS_CT_STATUS			0x06
#define LIGHT_CMD_LIGHTNESS_RGB_SET				0x07
#define LIGHT_CMD_LIGHTNESS_RGB_STATUS			0x08
#define LIGHT_CMD_GROUP_GET_ADD_DEL				0x09
#define LIGHT_CMD_GROUP_STATUS					0x0A
#define LIGHT_CMD_GROUP_ID_LIST_GET				0x0B
#define LIGHT_CMD_GROUP_ID_LIST					0x0C
#define LIGHT_CMD_SCENE_GET_SET					0x0D	// include scene add with light status
#define LIGHT_CMD_SCENE_CALL					0x0E
#define LIGHT_CMD_SCENE_STATUS					0x0F
#define LIGHT_CMD_SCENE_ADD_NO_PAR				0x10	// scene add without parameters of light status
#define LIGHT_CMD_SCENE_ADD_NO_PAR_ST			0x11	// status of "scene add without parameters"
#define LIGHT_CMD_SCENE_ID_LIST_GET				0x12
#define LIGHT_CMD_SCENE_ID_LIST					0x13
#define LIGHT_CMD_TIME_SET_ACK					0x14
#define LIGHT_CMD_TIME_SET_NO_ACK				0x15
#define LIGHT_CMD_TIME_GET						0x16
#define LIGHT_CMD_TIME_STATUS					0x17
#define LIGHT_CMD_DEL_NODE						0x18
#define LIGHT_CMD_DEL_NODE_STATUS				0x19
#define LIGHT_CMD_LIGHT_ONLINE_STATUS			0x1A
#define LIGHT_CMD_SWITCH_CONFIG					0x1B
#define LIGHT_CMD_SWITCH_CONFIG_STATUS			0x1C
#define LIGHT_CMD_CPS_GET						0x1D	// CPS: composition data
#define LIGHT_CMD_CPS_STATUS					0x1E
//#define RESERVED								0x1F	// reserve for future use
#define LIGHT_CMD_SCHEDULE_ACTION_SET			0x20	// SCHEDULER
#define LIGHT_CMD_SCHEDULE_ACTION_GET			0x21
#define LIGHT_CMD_SCHEDULE_ACTION_RSP			0x22
#define LIGHT_CMD_SCHEDULE_LIST_GET				0x23
#define LIGHT_CMD_SCHEDULE_LIST_STATUS			0x24

// ------------------ user define command from 0xC0 to 0xFF
// 
// ------------------ user define command end


#if 1 // define of BLE stack
// --------------------- BLE stack ----------------------
#define VENDOR_ID                       		0x0211

//#define BLE_STATE_LEGADV						1
#define BLE_STATE_PRICHN_SCAN_S					2
//#define BLE_STATE_BTX_S 						4
//#define BLE_STATE_BTX_E 						5
#define BLE_STATE_BRX_S 						6
#define BLE_STATE_BRX_E 						7

#define GAP_ADTYPE_MANUFACTURER_SPECIFIC        0xFF
#define GAP_ADTYPE_128BIT_COMPLETE              0x07

#define GAP_ADTYPE_FLAGS                        0x01 //!< Discovery Mode: @ref GAP_ADTYPE_FLAGS_MODES

#define ATT_MTU_SIZE                        	23

/* Advertising Maximum data length */
#define ADV_MAX_DATA_LEN            			31

// Advertise channel PDU Type
typedef enum advChannelPDUType_e {
	LL_TYPE_ADV_IND 		 = 0x00,
	LL_TYPE_ADV_DIRECT_IND 	 = 0x01,
	LL_TYPE_ADV_NONCONN_IND  = 0x02,
	LL_TYPE_SCAN_REQ 		 = 0x03,		LL_TYPE_AUX_SCAN_REQ 	 = 0x03,
	LL_TYPE_SCAN_RSP 		 = 0x04,
	LL_TYPE_CONNNECT_REQ 	 = 0x05,		LL_TYPE_AUX_CONNNECT_REQ = 0x05,
	LL_TYPE_ADV_SCAN_IND 	 = 0x06,

	LL_TYPE_ADV_EXT_IND		 = 0x07,		LL_TYPE_AUX_ADV_IND 	 = 0x07,	LL_TYPE_AUX_SCAN_RSP = 0x07,	LL_TYPE_AUX_SYNC_IND = 0x07,	LL_TYPE_AUX_CHAIN_IND = 0x07,
	LL_TYPE_AUX_CONNNECT_RSP = 0x08,
} advChannelPDUType_t;


typedef struct {
	u8 type   :4;
	u8 rfu1   :2;
	u8 txAddr :1;
	u8 rxAddr :1;
}rf_adv_head_t;

typedef struct{
	u32 dma_len;            //won't be a fixed number as previous, should adjust with the mouse package number

	rf_adv_head_t  header;	//RA(1)_TA(1)_RFU(2)_TYPE(4)
	u8  rf_len;				//LEN(6)_RFU(2)

	u8	advA[6];			//address
	u8	data[31];			//0-31 byte
}rf_packet_adv_t;

typedef struct{
	u32 dma_len;            //won't be a fixed number as previous, should adjust with the mouse package number
	rf_adv_head_t  header;	//RA(1)_TA(1)_RFU(2)_TYPE(4)
	u8  rf_len;				//LEN(6)_RFU(2)

	u8	scanA[6];			//
	u8	advA[6];			//
}rf_packet_scan_req_t;

typedef struct{
    u8 macAddress_public[6];
    u8 macAddress_random[6]; //host may set this
} ll_mac_t;

extern ll_mac_t bltMac;
extern rf_packet_adv_t	pkt_adv;
extern 			rf_packet_scan_req_t	pkt_scan_req;

static inline u8* 	blc_ll_get_macAddrPublic(void) // blc_ll_readBDAddr
{
	return bltMac.macAddress_public;
}

#if (MCU_CORE_TYPE == MCU_CORE_8208) // these struct need to be updated for different chip or different SDK version.
// these are for b80 single sdk V3.4.2.1
#else
// maybe differrent from other SDK. need to check.
#endif

#if 1 // other SDK may need these API if compile error.
u8 blt_ll_getCurrentBLEState(void);
u32 blt_ll_getAdvIntervalTick(void);
void blt_ll_setScanIntervalTick(u32 intervalTick);
void blt_ll_setCurrentState(u8 state);

typedef int (*ll_procScanPkt_callback_t)(u8 *, u8 *, u32);
extern ll_procScanPkt_callback_t blc_ll_procScanPktCb;
#endif

typedef 	void (*ll_enter_scan_callback_t)(void);
extern		ll_enter_scan_callback_t		ll_enter_pri_chn_scan_cb;

void bls_set_advertise_prepare (void *p);
unsigned short crc16 (unsigned char *pD, int len);
int blc_ll_procScanDataSmartHome (u32 h, u8 *p, int n);
extern void	smemcpy(void *pd, void *ps, int len);

// --------------------- BLE stack end-------------------
#endif

#define NW_PDU_PAR_LEN_APP_MAX				(9)	// this is for control message, max size is 9 byte for Android and iOS App.
#define NW_PDU_PAR_LEN_CONTROL_OP_MAX		(NW_PDU_PAR_LEN_APP_MAX)
#define NW_PDU_PAR_LEN_STATUS_OP_MAX		(15)// status message always from node not App.

typedef struct{
	u8 nid;			// network ID
	//u8 rsv;		// reserve
	#if 1 // FEATURE_RELAY_EN // should use a common field for both light and switch project
    u8 sno;			// sequence number
	u8 ttl : 4;		// ttl
	u8 rsv : 4;
	#else
	u16 sno;		// sequence number
	#endif
	u16 addr_src;	// source address
	u16 addr_dst;	// destination address
	u8 op;			// operation code
	u8 par[NW_PDU_PAR_LEN_STATUS_OP_MAX];		// size 0~15 // size is 9 byte for Android and iOS App.
	u8 rsv_mic[SIZE_MIC];	// MIC, not a fixed position, can not use, just for get size
}smart_home_nw_t;	// nw: network layer

typedef struct{
	u8 op;
	u8 par[22];
	u8 rsv_mic[SIZE_MIC];	// MIC, not a fixed position, can not use, just for get size
}smart_home_pair_t;

typedef struct{
	u8 len;
	u8 type;
	u16 vendor_id;
	u16 vendor_id2;
	union{
		smart_home_nw_t nw;
		smart_home_pair_t pair;
	};
}smart_home_adv_payload_t;

typedef struct{
	u8 len;
	u8 type;
	u16 vendor_id;
	u16 ibeacon_type;
	u16 vendor_id2;
	smart_home_nw_t nw;
}smart_home_adv_payload_ios_t;

#define ADV_LEN_GATT_LEN_DELTA		(OFFSETOF(smart_home_adv_payload_t, nw) - OFFSETOF(smart_home_adv_payload_t, type))

#define OFFSET_CMD_PAYLOAD_PARA		(OFFSETOF(smart_home_adv_payload_t, nw) + OFFSETOF(smart_home_nw_t, par))
#define OFFSET_PAIR_PAYLOAD_PARA	(OFFSETOF(smart_home_adv_payload_t, pair) + OFFSETOF(smart_home_pair_t, par))
static inline int get_cmd_para_len(smart_home_adv_payload_t *p) // include MIC when decryption
{
	return (p->len + SIZEOF_MEMBER(smart_home_adv_payload_t, len) - OFFSET_CMD_PAYLOAD_PARA);
}
static inline int get_cmd_payload_len_val(int par_len)
{
	return (OFFSET_CMD_PAYLOAD_PARA - SIZEOF_MEMBER(smart_home_adv_payload_t, len) + par_len + SIZE_MIC);
}
static inline u16 get_cmd_mic_dec(smart_home_nw_t *p_nw, int par_len)  // par_len include MIC
{
	return (p_nw->par[par_len-2] + p_nw->par[par_len-1]*256);
}
static inline void set_cmd_mic_enc(smart_home_nw_t *p_nw, int par_len, u8 *mic)  // par_len do not include MIC
{
	p_nw->par[par_len] = mic[0];
	p_nw->par[par_len+1] = mic[1];
}

static inline int get_pair_para_len(smart_home_adv_payload_t *p)
{
	return (p->len + SIZEOF_MEMBER(smart_home_adv_payload_t, len) - OFFSET_PAIR_PAYLOAD_PARA);
}
static inline int get_pair_payload_len_val(int par_len)
{
	return (OFFSET_PAIR_PAYLOAD_PARA - SIZEOF_MEMBER(smart_home_adv_payload_t, len) + par_len + SIZE_MIC);
}
static inline u16 get_pair_mic_dec(smart_home_pair_t *p_pair, int par_len)  // par_len include MIC
{
	return (p_pair->par[par_len-2] + p_pair->par[par_len-1]*256);
}
static inline void set_pair_mic_enc(smart_home_pair_t *p_pair, int par_len, u8 *mic)  // par_len do not include MIC
{
	p_pair->par[par_len] = mic[0];
	p_pair->par[par_len+1] = mic[1];
}

static inline int is_valid_nw_vendor_id(u16 id)
{
	return ((VENDOR_ID == id)||(VENDOR_ID_PAIR == id));
}

static inline int is_blt_gatt_connected()
{
	return (BLS_LINK_STATE_CONN == blc_ll_getCurrentState()); // blt_state
}

static inline u32 rand_sh()
{
#if WIN32
	return clock_time_vc_hw();
#else
	return rand();
#endif
}

typedef enum{
	CMD_BUF_HEAD_TYPE_NONE				= 0,
	CMD_BUF_HEAD_TYPE_PAIR_RSP_MAC		= 1,
	CMD_BUF_HEAD_TYPE_SCH_ACTION_STATUS	= 2, // scheduler action status message
}cmd_buf_head_par_type_t;

typedef struct{
	u8 par_type;	// it's better that the first byte is not a variable in bit filed format.
	u8 val;			// value for par_type
	u8 rsv_telink[1];
	u8 transmit_cnt;
	u8 transmit_interval;
}cmd_buf_head_t;

typedef struct{
	cmd_buf_head_t cmd_head;	// parameters for tx bear that can be read in app_advertise_prepare_handler_()
	smart_home_adv_payload_t payload;
}_align_4_ smart_home_cmd_buf_t;

#define RELAY_TTL_DEFAULT           (10)
#if FEATURE_RELAY_EN
#define MESH_ADV_BUF_RELAY_CNT      (8)

typedef struct __attribute__((packed)) {
	u32 tick_relay; // 0 means ready to tx relay.
	u8 pop_later_flag; // 1 means relay complete but can not pop now, because do not locate at the header of fifo.
	u8 rsv[3];
    smart_home_cmd_buf_t bear;
}mesh_relay_buf_t;

int smart_home_cmd_adv_set(u8 *p_adv, u8 *p_payload);
#endif

static inline u8 get_smart_home_cmd_buf_len (const smart_home_adv_payload_t *p){
    return (OFFSETOF(smart_home_cmd_buf_t, payload) + sizeof(p->len) + p->len);
}

// ------ command parameters
typedef enum{
	L_OFF		= 0,
	L_ON 		= 1,
	L_TOGGLE	= 2,
	L_GET		= 3,
}onoff_set_mode_t;

typedef struct{
	u8 set_mode	:3;
	u8 rsv_bit	:4;
	u8 ack		:1;
	u16 delay_10ms;	// unit: 10ms
}lcmd_onoff_t;

typedef struct{
    u32 tick_light_step_timer;	// because no use irq timer.
	u16 delay_10ms;
	u8 onoff;
}light_st_delay_t;

typedef enum{
	DEL_NODE_MODE_NORMAL		= 0,
	DEL_NODE_MODE_TIME_LIMIT	= 1,	// delete node must within PAIR_TIMEOUT_POWER_UP_MS.
}del_node_mode_t;

typedef struct{
	u8 del_mode	:3;
	u8 rsv_bit	:4;
	u8 ack		:1;
	u16 delay_ms;	// unit: 1ms // no relay , so no need to delay.
}lcmd_del_node_t;

typedef struct{
	u8 onoff;
}lcmd_onoff_st_t;

typedef enum {
	LIGHT_SET_MODE_TARGET 		= 0,
	LIGHT_SET_MODE_INCREASE		= 1,
	LIGHT_SET_MODE_DECREASE		= 2,
	LIGHT_SET_MODE_GET			= 3,
}light_set_mode_t;

typedef struct{
	u8 set_mode	:3;
	u8 rsv_bit	:4;
	u8 ack		:1;
	u8 lightness;	// range: 1~100
}lcmd_lightness_t;

typedef struct{
	u8 lightness;
}lcmd_lightness_st_t;

typedef struct{
	u8 set_mode			:3;
	u8 rsv_bit			:2;
	u8 lightness_flag	:1;
	u8 ct_flag			:1;
	u8 ack				:1;
	u8 lightness;			// range: 1~100, only valid when lightness_flag is ture.
	u8 ct;					// range: 0~100, only valid when ct_flag is ture.
}lcmd_CTL_t;

typedef struct{
	u8 lightness;
	u8 ct;
}lcmd_CTL_st_t;

typedef struct{
	u8 set_mode			:3;
	u8 lightness_flag	:1;
	u8 R_flag			:1;
	u8 G_flag			:1;
	u8 B_flag			:1;
	u8 ack				:1;
	u8 lightness;			// range: 1~100, only valid when lightness_flag is ture.
	u8 R;					// range: 0~255, only valid when R_flag is ture.
	u8 G;					// range: 0~255, only valid when R_flag is ture.
	u8 B;					// range: 0~255, only valid when R_flag is ture.
}lcmd_RGBL_t;

typedef struct{
	u8 lightness;
	u8 R;
	u8 G;
	u8 B;
}lcmd_RGBL_st_t;

typedef enum {
	GROUP_GET 				= 0,	// reserve to check whether existed the group setting in "group_id" of lcmd_group_t.
	GROUP_ADD 				= 1,
	GROUP_DEL 				= 2,
	GROUP_DEL_ALL			= 3,
}group_set_mode_t;

typedef struct{
	u8 set_mode			:3;
	u8 rsv				:4;
	u8 ack				:1;
	u16 group_id;			// only valid for get, add and del mode.
}lcmd_group_t;

typedef struct{
	u8 st;
	u16 group_id;			// only valid for get, add and del mode.
}lcmd_group_st_t;

typedef struct{
	u8 offset;			// Offset of total group id list
}lcmd_group_id_list_get_t;

typedef struct{
	u8 offset;			// Offset of total group id list
	u8 total_cnt;		// number of total group id
	u16 id_list[6];		// group id list in current message.
}lcmd_group_id_list_t;

typedef enum {
	SH_SCENE_GET 			= 0,
	SH_SCENE_ADD 			= 1,
	SH_SCENE_DEL 			= 2,
	SH_SCENE_DEL_ALL		= 3,
}scene_set_mode_t;

typedef struct{
	u8 set_mode			:3;
	u8 rsv				:2;
	u8 ct_flag			:1;
	u8 rgb_flag			:1;
	u8 ack				:1;
	u8 id;				// range: 1~245. 246-255 is reserved. only valid for add and del mode.
	u8 lightness;
	u8 ct;
	led_RGB_t rgb;
}lcmd_scene_set_t;

typedef struct{
	u8 rsv				:7;
	u8 ack				:1;
	u8 id;					// range: 1~245. 246-255 is reserved. 
}lcmd_scene_call_t;

typedef struct{
	u8 rsv				:7;
	u8 ack				:1;
	u8 id;					// range: 1~245. 246-255 is reserved. 
}lcmd_scene_add_no_par_t;

typedef struct{
	u8 st;
	u8 id;
}lcmd_scene_add_no_par_st_t;

typedef struct{
	u8 st				:5;
	u8 ct_flag			:1;
	u8 rgb_flag			:1;
	u8 rsv				:1;
	u8 id;				// range: 1~245. 246-255 is reserved. only valid for add and del mode.
	u8 lightness;
	u8 ct;
	u8 R;
	u8 G;
	u8 B;
}lcmd_scene_st_t;

typedef struct{
	u8 offset;			// Offset of total scene id list
}lcmd_scene_id_list_get_t;

typedef struct{
	u8 offset;			// Offset of total scene id list
	u8 total_cnt;		// number of total scene id
	u8 id_list[13];		// scene id list in current message.
}lcmd_scene_id_list_t;

typedef struct{
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
}lcmd_time_set_t;

typedef struct{
	u8 st;
	lcmd_time_set_t time;
}lcmd_time_st_t;

typedef struct{
    u16 vendor_id_pair;
    u8 mac[6];		// for iOS
    u32 factory_reset_ver;
    u8 rsv[4];
}smart_home_adv_info_t;

typedef struct{
	u32 dma_len;
	rf_adv_head_t  header;	//RA(1)_TA(1)_RFU(2)_TYPE(4)
	u8  rf_len;				//LEN(6)_RFU(2)
	u8	advA[6];			//mac address
	u8 len1;
	u8 type1;
	u8 adv_falg;
	u8 len2;
	u8 type2;				// information
	smart_home_adv_info_t info;
}smart_home_connectable_adv_t;

typedef struct{
	u32 dma_len;
	rf_adv_head_t  header;	//RA(1)_TA(1)_RFU(2)_TYPE(4)
	u8  rf_len;				//LEN(6)_RFU(2)
	u8	advA[6];			//mac address
	u8 len1;
	u8 type1;
	u16 vendor_id1;
	u16 vendor_id2;
	u8 sno;					// must, because iOS will filter the same packet that next to each other, and not report.
	u8 mac[6];
	u32 version;
	u8 op;			// operation code
	u8 rsv1				:5;
	u8 ct_flag			:1;
	u8 rgb_flag			:1;
	u8 rsv2				:1;
	u8 lightness;
	u8 ct;
	led_RGB_t rgb;
	u8 mic[SIZE_MIC];
}smart_home_light_online_st_t;

// ---- save -------------------------
#define KEY_SAVE_ENCODE_FLAG    (0x3A)
extern sh_server_pair_key_info_t smart_home_pair_key_info[SMART_HOME_SERVER_PAIR_KEY_MAX];

typedef struct{
#if SMART_HOME_SERVER_EN
	u8 key_info[sizeof(smart_home_pair_key_info)];
	u32 online_st_ver;
#endif
#if SMART_HOME_CLIENT_SW_EN
	sh_server_pair_key_info_t key_sw_master;
#endif
	u8 rsv[5];
	u8 mic_val[2];
	u8 encode_flag;
}pair_key_save_t;


// ---- message cache process --------
#define SNO_CACHE_CNT_MAX		(16)	// responses to command of "getting all" will not add to cache because address not match.
typedef struct{
	u8 key_idx;				// need to record key index for cache, because different APP may have the same source address.
	// u8 op;				// TODO,  
	u16 src;
	u16 sno;
}sno_cache_buf_t;


// ---- pair proc for server --------
typedef enum{
	PAIR_SERVER_ST_IDLE		= 0,
	//PAIR_SERVER_ST_REQUEST,
	PAIR_SERVER_ST_RSP_MAC,
	//PAIR_SERVER_ST_SEND_UNICAST_ADDR,
	PAIR_SERVER_ST_CONFIRM,
	PAIR_SERVER_ST_MAX,
}pair_server_st_t;

typedef struct{
	u32 st_tick;
	u8 net_index;
	pair_server_st_t st;
}smart_home_pair_server_proc_t;

typedef enum{
    FLD_OTA_REBOOT_FLAG                 = BIT(0),
    FLD_OTA_REBOOT_WITH_OFF_FLAG        = BIT(1),	// keep light off after OTA reboot.
    FLD_OTA_SUCCESS_REBOOT_FLAG         = BIT(2),
}smart_home_reboot_flag_t;

#define SMART_HOME_XXX_REG				(DEEP_ANA_REG0)	// DEEP_ANA_REG0 has been used by BLE stack library.
#define SMART_HOME_REBOOT_FLAG_REG		(DEEP_ANA_REG1)	// 
#define SMART_HOME_SW_SNO_REG           (DEEP_ANA_REG6)

#if SMART_HOME_CLIENT_SW_EN
#if (MCU_CORE_TYPE == MCU_CORE_8208)
#define SMART_HOME_SW_KEY_PAR_REG		(DEEP_ANA_REG7)	// 
#else
#define SMART_HOME_SW_KEY_PAR_REG		(DEEP_ANA_REG2)	// 
#endif
#endif

// ----- switch project -----
typedef enum{
	SWITCH_MODE_MASTER			= 0,	// as provisioner, can pair other devices, such as lights.
	SWITCH_MODE_SLAVE			= 1,	// as slave, can be paired by APP, and can be GATT OTA.
	SWITCH_MODE_SLAVE_LOWPOWER	= 2,	// as slave with low power, can be GATT OTA.
}smart_home_switch_mode_t;

void smart_home_toggle_switch_mode();
void test_smart_home_cmd_tx_onoff_toggle();


int is_test_key_function_2();
void test_smart_home_cmd_tx_scene_add();
void test_smart_home_cmd_tx_scene_call();

extern smart_home_switch_mode_t switch_mode;
extern u32 switch_slave_mode_tick;
extern u8 key_pressing_cnt;

// ----- inline -----
extern light_st_delay_t g_light_st_delay;
static inline void light_st_delay_init()
{
	memset(&g_light_st_delay, 0, sizeof(g_light_st_delay));
}

// ----- extern -----
rx_cb_err_t smart_home_rx_ini_cmd(u8 *par_ini, int par_len);
void smart_home_rx_ini_pair(u8 *par_ini, int par_len);
int is_smart_home_tx_cmd_empty();
tx_cmd_err_t smart_home_cmd_tx(u8 op, u16 addr_dst, u8 *par, int par_len, sh_server_pair_key_info_t *p_key);
tx_cmd_err_t smart_home_cmd_tx_rsp(u8 op, u16 addr_dst, u8 *par, int par_len);
addr_match_type_t get_cmd_addr_match(u16 addr_dst, sh_server_pair_key_info_t *p_info);
const char * get_op_string_sh(u8 op);
const char * get_err_number_string_sh(int err);
const char * get_scheduler_status_err_st_string(int err_st);
tx_cmd_err_t smart_home_cmd_tx_push_fifo(smart_home_cmd_buf_t *p_cmd);
rx_cb_err_t smart_home_pair_client_rx_cb(smart_home_adv_payload_t *payload, rx_source_type_t source, int need_dec);
rx_cb_err_t smart_home_pair_server_rx_cb(smart_home_adv_payload_t *payload, rx_source_type_t source);
tx_cmd_err_t smart_home_pair_tx_request();
tx_cmd_err_t smart_home_pair_tx_send_unicast_addr();
tx_cmd_err_t smart_home_pair_tx_unpair();
tx_cmd_err_t smart_home_pair_tx_response_mac();
tx_cmd_err_t smart_home_pair_tx_confirm();
tx_cmd_err_t smart_home_pair_tx(u8 op, u8 *par, int par_len);
tx_cmd_err_t smart_home_tx_pair_request_switch();
tx_cmd_err_t smart_home_cmd_tx_switch(u8 op, u16 addr_dst, u8 *par, int par_len);
tx_cmd_err_t smart_home_cmd_tx_switch_factory_key(u8 op, u16 addr_dst, u8 *par, int par_len);
tx_cmd_err_t smart_home_tx_del_node_switch();

void smart_home_pair_enc(smart_home_pair_t *p_pair, int par_len, sh_server_pair_key_info_t *p_key);
rx_cb_err_t smart_home_pair_dec(smart_home_adv_payload_t *payload, sh_server_pair_key_info_t *p_dec_key);
rx_cb_err_t smart_home_cmd_dec(smart_home_nw_t *p_nw, int par_len);
void smart_home_nw_enc(smart_home_nw_t *p_nw, int par_len, sh_server_pair_key_info_t *p_key);
void smart_home_online_st_enc(smart_home_light_online_st_t *p_st);
void smart_home_online_st_dec(smart_home_light_online_st_t *p_st);
void set_factory_reset_version();
int is_factory_reset_version_valid();

void smart_home_pair_key_save();
int smart_home_pair_key_retrieve();
void smart_home_group_id_save();
int smart_home_group_id_retrieve();
rx_cb_err_t group_get_set_rx_handle(lcmd_group_t *p_set);
rx_cb_err_t group_get_set_rx_cb(smart_home_nw_t *p_nw);

void mesh_par_write_with_check(u32 addr, u32 size, const u8 *in);
void smart_home_par_store(const u8 *in, u32 *p_adr, u32 adr_base, u32 size);
int smart_home_par_retrieve(u8 *out, u32 *p_adr, u32 adr_base, u32 size);
void smart_home_led_event_callback (led_event_t ev);
void unpair_clear_cache(sh_server_pair_key_info_t *p_dec_key);
void kick_out(int led_en);
int factory_reset_cnt_check ();
void set_online_st_data(smart_home_light_online_st_t *p_st);
void smart_home_switch_slave_proc();
void smart_home_gatt_connected_cb();
void smart_home_gatt_disconnected_cb();
void smart_home_set_switch_mode(smart_home_switch_mode_t mode);
void smart_home_switch_quick_exit_slave_mode_check();
void smart_home_switch_init();


extern _align_4_ my_fifo_t smart_home_tx_cmd_fifo;
extern u32 factory_reset_version;

// ----- protocol adapt API for firmware SDK------
void smart_home_loop();
void smart_sno_retrive();
void smart_home_init();
rx_cb_err_t smart_home_cmd_rx_adv_report(u8 *data, int len);
rx_cb_err_t smart_home_cmd_rx_cb(smart_home_adv_payload_t *payload, rx_source_type_t source);
int gatt_adv_prepare_handler(rf_packet_adv_t * p, int rand_flag);
int app_advertise_prepare_handler_smart_home (rf_packet_adv_t * p);
smart_home_adv_payload_t * parse_adv_data(u8 *data, int len);
void proc_ui_smart_home();
int rf_link_time_allow (u32 us);

tx_cmd_err_t smart_home_cmd_tx_onff(u16 addr_dst, onoff_set_mode_t onoff_mode, u16 delay_10ms, ack_flag_t ack);
tx_cmd_err_t smart_home_cmd_tx_lightness(u16 addr_dst, u8 lightness, light_set_mode_t set_mode, ack_flag_t ack);
tx_cmd_err_t smart_home_cmd_tx_lightness_and_ct(u16 addr_dst, u8 lightness, u8 ct, light_set_mode_t set_mode, ack_flag_t ack);
tx_cmd_err_t smart_home_cmd_tx_ct(u16 addr_dst, u8 ct, light_set_mode_t set_mode, ack_flag_t ack);


