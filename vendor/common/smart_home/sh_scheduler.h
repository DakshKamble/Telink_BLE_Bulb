/********************************************************************************************************
 * @file	sh_scheduler.h
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

#include "tl_common.h"
#include "log_module.h"
#include "protocol.h"
#include "sh_time.h"


#define ALARM_CNT_MAX           16
#define ALARM_STORE_MAX_SIZE    FLASH_SECTOR_SIZE

#if(SMART_HOME_SCHEDULER_EN)
#define SCHEDULER_ACTION_POLL_QUICK_RSP_EN		1 // if get next index will pop the last status message to quick respond next index when poll all scheduler actions.
#endif

enum{
	ALARM_EV_ADD = 0,
	ALARM_EV_DEL,
	ALARM_EV_CHANGE,
	ALARM_EV_ENABLE,
	ALARM_EV_DISABLE,
	ALARM_EV_MAX,
};

enum{
	ALARM_CMD_OFF = 0,
	ALARM_CMD_ON,
	ALARM_CMD_SCENE,
	ALARM_CMD_MAX,
};

enum{
	ALARM_TYPE_DAY = 0,
	ALARM_TYPE_WEEK,
	ALARM_TYPE_MAX,
};

enum{
	ALARM_INVALID = 0x00,
	ALARM_VALID = 0xA5,
};

enum{
	ALARM_VALID_FLAG_VALID = 0,
	ALARM_VALID_FLAG_INVALID,
	ALARM_VALID_FLAG_END,
	ALARM_VALID_FLAG_ADR_MAX,
};

enum{
	READ_CACHE_MODE = 0,
	READ_FLASH_MODE,
};

enum{
    ALARM_DEL_OK = 0,
	ALARM_ENABLE_DISABLE_OK,
	ALARM_ENABLE_OK,
	ALARM_DISABLE_OK,
};

#define ALARM_INVALID_INDEX			(0xff)
#define ALARM_DELETE_ALL_INDEX		(0xff)
#define ALARM_AUTO_CREATE_INDEX		(0xff)
#define ALARM_INDEX_MAX				(ALARM_CNT_MAX - 1)

typedef enum{
    SCHEDULER_ST_SUCCESS 			= 0,
    SCHEDULER_ST_INVALID_PAR 		= 1,
    SCHEDULER_ST_INSUFFICIENT_RES 	= 2,
    SCHEDULER_ST_CAN_NOT_REPLACE 	= 3,
    SCHEDULER_ST_NOT_FOUND_INDEX 	= 4,
    SCHEDULER_ST_INVALID_INDEX 		= 5,
}scheduler_st_err_t;

typedef struct{ // max 9 BYTES for set command.
    union {
    	struct {
        	u8 event	:4;	// for scheduler set message: ALARM_EV_ADD/ALARM_EV_DEL/...
        	u8 rfu0		:3;	// 
        	u8 ack		:1;	// ack / no ack message
        };
    	struct {
        	scheduler_st_err_t st	:3; // for scheduler status message. 
        	u8 rfu1		:5;	// 
        	//u8 not_existed			:1;	// scheduler not existed
        };
        u8 valid_flag;		// for save in flash: ALARM_VALID
    }par0;
    u8 index;
    struct {
        u8 cmd 			:4;	// action of acheduler: ALARM_CMD_OFF/ALARM_CMD_ON/...
        u8 type 		:3;	// ALARM_TYPE_DAY/ALARM_TYPE_WEEK
        u8 enable 		:1;	// enable or disable state for scheduler.
    }par1;
    u8 month;
    union {
        u8 day;
        u8 week;    // BIT(n)
    }par2;
    u8 hour;
    u8 minute;
    u8 second;
    u8 scene_id;
    //u8 alarm total for notiffy	// smart home there is only 9 bytes(NW_PDU_PAR_LEN_CONTROL_OP_MAX) for control message, but 15 bytes(NW_PDU_PAR_LEN_STATUS_OP_MAX) for status message.
}alarm_ev_t;

typedef struct{
	u8 max_quantity;	// always equal to ALARM_CNT_MAX
	u8 bit_map[NW_PDU_PAR_LEN_STATUS_OP_MAX - 1]; // 1: sizeof(max_quantity)
}alarm_list_t;

typedef struct{
	u8 sch_index;	// scheduler index
}scheduler_action_get_t;

#if SCHEDULER_ACTION_POLL_QUICK_RSP_EN
typedef struct{
	u32 tick_last_get;
	u16 addr_src;
	u8 index_last_get;
}sch_action_poll_quick_rsp_proc_t;
#endif

static inline int is_valid_scheduler_action_index(u8 index)
{
	return (index <= ALARM_INDEX_MAX);//(ALARM_AUTO_CREATE_INDEX == index)||(ALARM_INVALID_INDEX == index)
}

static inline int is_delete_all_scheduler_action_index(u8 index)
{
	return (ALARM_DELETE_ALL_INDEX == index);//(ALARM_AUTO_CREATE_INDEX == index)||(ALARM_INVALID_INDEX == index)
}

extern const u8 days_by_month[12];

rx_cb_err_t scene_load(u8 id);
void check_event_after_set_time(const rtc_t *rtc_new, const rtc_t *rtc_old);
void alarm_init();
tx_cmd_err_t scheduler_action_set_rx_cb(smart_home_nw_t *p_nw);
tx_cmd_err_t scheduler_action_get_rx_cb(smart_home_nw_t *p_nw);
tx_cmd_err_t scheduler_list_get_rx_cb(smart_home_nw_t *p_nw);
int alarm_get_by_id(alarm_ev_t *pkt_rsp, u8 id);
int alarm_get_all_id(alarm_list_t *list_rsp);
void scheduler_action_quick_rsp_proc();
void scheduler_action_quick_rsp_record_and_check(smart_home_nw_t *p_nw);

