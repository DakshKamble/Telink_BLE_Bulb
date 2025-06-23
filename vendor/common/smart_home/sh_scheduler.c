/********************************************************************************************************
 * @file	sh_scheduler.c
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
#include "sh_scheduler.h"


#if(SMART_HOME_SCHEDULER_EN)
STATIC_ASSERT(SMART_HOME_TIME_EN == 1);
STATIC_ASSERT(ALARM_CNT_MAX <= 8 * SIZEOF_MEMBER(alarm_list_t, bit_map)); // one bit for a scheduler index.

alarm_ev_t alarm_list[ALARM_CNT_MAX];
static u16  alarm_next_pos = 0;
static u32 flash_adr_alarm = FLASH_ADDR_SMART_HOME_SCHEDULER_SAVE;

#if SCHEDULER_ACTION_POLL_QUICK_RSP_EN
static sch_action_poll_quick_rsp_proc_t sch_action_poll_quick_rsp_proc = {0};
#endif

static inline int alarm_get_flag(u32 adr, u8 cache_mode){
    u8 flag_val = 0;
    if(READ_CACHE_MODE == cache_mode){
        flag_val = *(u8 *)adr;
    }else{
        flash_read_page(adr, 1, &flag_val);
    }
    if(adr > flash_adr_alarm + ALARM_STORE_MAX_SIZE - sizeof(alarm_ev_t)){
        return ALARM_VALID_FLAG_ADR_MAX;
    }else if(flag_val == 0xff){
        return ALARM_VALID_FLAG_END;
    }else if(flag_val == ALARM_VALID){
        return ALARM_VALID_FLAG_VALID;
    }else{
        return ALARM_VALID_FLAG_INVALID;
    }
}

static inline int is_valid_alarm_flag(u8 valid_flag)
{
	return (ALARM_VALID == valid_flag);
}

static inline int is_same_alarm_par_from_index(const alarm_ev_t *p_alarm_ev1, const alarm_ev_t *p_alarm_ev2)
{
	return (0 == memcmp(&p_alarm_ev1->index, &p_alarm_ev2->index, sizeof(alarm_ev_t) - OFFSETOF(alarm_ev_t, index)));
}


int alarm_del(const alarm_ev_t *p_ev, alarm_ev_t *p_ev_rsp){
    int ret = ALARM_DEL_OK;
    u8 update_flash = 0;
    if(alarm_next_pos == 0){
        return 0;
    }

    foreach(i, ALARM_CNT_MAX){
        alarm_ev_t *p_alarm = &alarm_list[i];
        if(!is_valid_alarm_flag(p_alarm->par0.valid_flag)){
            continue;
        }
        
        if(p_ev->index == ALARM_DELETE_ALL_INDEX){//delete all
            memset(p_alarm, 0, sizeof(alarm_ev_t));
            update_flash = 1;
        }else if(p_ev->index == p_alarm->index){
            update_flash = 2;

            if((ALARM_EV_ENABLE == p_ev->par0.event) || (ALARM_EV_DISABLE == p_ev->par0.event)){
                if(p_ev_rsp){
                    memcpy(p_ev_rsp, p_alarm, sizeof(alarm_ev_t));
                    ret = ALARM_ENABLE_DISABLE_OK;
                }
            }
            memset(p_alarm, 0, sizeof(alarm_ev_t));
            
            break;
        }
    }
    
    u8 alarm_cnt = 0;
    alarm_ev_t p_alarm;
    alarm_ev_t del_val;
    memset(&del_val, 0, sizeof(alarm_ev_t));
    s16 i = alarm_next_pos - sizeof(alarm_ev_t);
    do{
        flash_read_page(flash_adr_alarm + i, sizeof(alarm_ev_t),(u8 *)&p_alarm);
        if(update_flash == 1 || ((update_flash == 2) && (is_valid_alarm_flag(p_alarm.par0.valid_flag) && (p_ev->index == p_alarm.index)))){
            flash_write_page(flash_adr_alarm + i, sizeof(alarm_ev_t), (u8 *)(&del_val));
            
            if(update_flash == 2 || ++alarm_cnt >= ALARM_CNT_MAX){
                break;
            }
        }
        if(i <= 0){
            break;
        }
        i -= sizeof(alarm_ev_t);
    }while(1);
    
    return ret;
}

void alarm_flash_clean(){
    if(alarm_next_pos >= FLASH_SECTOR_SIZE - sizeof(alarm_ev_t)){
        flash_erase_sector(flash_adr_alarm);
        flash_write_page(flash_adr_alarm, sizeof(alarm_list), (u8 *)(&alarm_list));
        alarm_next_pos = sizeof(alarm_list);
    }    
}

scheduler_st_err_t alarm_add(const alarm_ev_t *p_ev){
    u8 overwrite = 1;    

    alarm_flash_clean();
    
    alarm_ev_t *p_alarm = NULL;
    foreach(i, ALARM_CNT_MAX){
        p_alarm = &alarm_list[i];
        if(is_valid_alarm_flag(p_alarm->par0.valid_flag)
         &&(p_alarm->index == p_ev->index)){
            // exist
            if(is_same_alarm_par_from_index(p_ev, p_alarm)){
				//LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"scheduler is the same");
            	return SCHEDULER_ST_SUCCESS; // the same.
            }else{
            	return SCHEDULER_ST_CAN_NOT_REPLACE;
            }
        }else if(!is_valid_alarm_flag(p_alarm->par0.valid_flag)){
            memcpy(p_alarm, p_ev, sizeof(alarm_ev_t));
            if(p_alarm->par1.cmd == ALARM_CMD_OFF || p_alarm->par1.cmd == ALARM_CMD_ON){
                p_alarm->scene_id = 0;
            }
            p_alarm->par0.valid_flag = ALARM_VALID;
            overwrite = 0;
            break;
        }
    }
    
    if(overwrite){
        return SCHEDULER_ST_INSUFFICIENT_RES;
    }
    
    // add one group
    if(p_alarm){
        flash_write_page(flash_adr_alarm + alarm_next_pos, sizeof(alarm_ev_t), (u8 *)p_alarm);
        alarm_next_pos += sizeof(alarm_ev_t);
    }

    return SCHEDULER_ST_SUCCESS;
}

void alarm_retrieve(void){
	u8 alarm_cnt = 0;
	u32 alarm_next_adr = flash_adr_alarm;
	foreach(i, ALARM_STORE_MAX_SIZE / sizeof(alarm_ev_t)){
	    alarm_next_pos = i * sizeof(alarm_ev_t);
	    alarm_next_adr = flash_adr_alarm + i * sizeof(alarm_ev_t);
	    int flag = alarm_get_flag(alarm_next_adr, READ_CACHE_MODE);
        if((ALARM_VALID_FLAG_END == flag) || (ALARM_VALID_FLAG_ADR_MAX == flag)){
            break;
        }
        
        if(ALARM_VALID_FLAG_VALID != flag){
            continue;
        }

        // alarm address
        memcpy(&alarm_list[alarm_cnt%ALARM_CNT_MAX], (u8 *)alarm_next_adr, sizeof(alarm_ev_t));
        alarm_cnt++;
        alarm_next_pos += sizeof(alarm_ev_t);
	}
}

void alarm_init(){
    alarm_retrieve();
#if 0
	if (*(u32 *) flash_adr_alarm == 0xffffffff)
	{
        flash_write_page(flash_adr_alarm, sizeof(alarm_ev_t), &alarm0);
	}
#endif	
}

void alarm_callback(alarm_ev_t *p_alarm){
#if 1
    switch(p_alarm->par1.cmd){
        case ALARM_CMD_OFF:
            light_onoff(0);
            break;
        case ALARM_CMD_ON:
            light_onoff(1);
#if WORK_SLEEP_EN // demo code
            sleep_us(200*1000);
            light_onoff(0);
            sleep_us(200*1000);
            light_onoff(1);
            sleep_us(200*1000);
            light_onoff(0);
            sleep_us(200*1000);
            light_onoff(1);
            sleep_us(200*1000);
#endif    
            break;
        case ALARM_CMD_SCENE:
            #if SMART_HOME_SCENE_EN
            scene_load(p_alarm->scene_id);
            #endif
            break;
        default:
            break;
    }
#endif    
}

scheduler_st_err_t alarm_par_check(const alarm_ev_t* p_ev){
	//scheduler_st_err_t err = SCHEDULER_ST_SUCCESS;

    if(!is_valid_scheduler_action_index(p_ev->index)){
        return SCHEDULER_ST_INVALID_INDEX;
    }
     
    if((p_ev->par1.cmd >= ALARM_CMD_MAX)
     ||(p_ev->par1.type >= ALARM_TYPE_MAX)){
        return SCHEDULER_ST_INVALID_PAR;
     }

    if((p_ev->hour >= 24)
      ||(p_ev->minute >= 60)
      ||(p_ev->second >= 60)){
            return SCHEDULER_ST_INVALID_PAR;
    }
    
    if(ALARM_TYPE_DAY == p_ev->par1.type){
        if((!(p_ev->month)) || (p_ev->month > 12)){
            return SCHEDULER_ST_INVALID_PAR;
        }

        u8 days = days_by_month[(p_ev->month-1)%12];
        if(2 == p_ev->month){
            days = 29;
        }
        if((!(p_ev->par2.day)) || (p_ev->par2.day > days)){
            return SCHEDULER_ST_INVALID_PAR;
        }
    }
    else if(ALARM_TYPE_WEEK == p_ev->par1.type){
        if((0 == p_ev->par2.week)
         ||((p_ev->par2.week >= 0x80) || (0 == p_ev->par2.week))){
            return SCHEDULER_ST_INVALID_PAR;
        }
    } 

    return SCHEDULER_ST_SUCCESS;
}

u8 get_next_shedule_idx()
{
	for(u8 j = 0; j < ALARM_CNT_MAX; ++j)
	{
		for(u8 i = 0; i < ALARM_CNT_MAX; ++i){
			alarm_ev_t *p_save = &alarm_list[i];
			if(is_valid_alarm_flag(p_save->par0.valid_flag) && p_save->index == j){
				break;
			}else if(i == ALARM_CNT_MAX - 1){
				return j;
			}
		}
	}
	return ALARM_INVALID_INDEX;
}

int alarm_ev_callback(u8 *ev)
{
    alarm_ev_t* p_ev = (alarm_ev_t*)ev;
    scheduler_st_err_t err = SCHEDULER_ST_SUCCESS;
    switch(p_ev->par0.event){
        case ALARM_EV_ADD:
            if(ALARM_AUTO_CREATE_INDEX == p_ev->index){
                p_ev->index = get_next_shedule_idx();
                if(ALARM_INVALID_INDEX == p_ev->index){
                	return SCHEDULER_ST_INSUFFICIENT_RES;
                }
            }

        	err = alarm_par_check(p_ev);
            if(err != SCHEDULER_ST_SUCCESS){
                return err;
            }

            err = alarm_add(p_ev);
            break;
            
        case ALARM_EV_DEL:
        	if(!(is_delete_all_scheduler_action_index(p_ev->index) || is_valid_scheduler_action_index(p_ev->index))){
        		//return SCHEDULER_ST_INVALID_INDEX;
        	}
        	
            alarm_del(p_ev, NULL);
            break;
            
        case ALARM_EV_CHANGE:
        	err = alarm_par_check(p_ev);
            if(err != SCHEDULER_ST_SUCCESS){
                return err;
            }

			alarm_ev_t alarm_ev_node;
			int found = alarm_get_by_id(&alarm_ev_node, p_ev->index);
			if(found && is_same_alarm_par_from_index(&alarm_ev_node, p_ev)){
				// the same, so do nothing
				//LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"scheduler is the same");
			}else{
	            alarm_del(p_ev, NULL);
	            err = alarm_add(p_ev);
            }
            break;
               
        case ALARM_EV_ENABLE:
        case ALARM_EV_DISABLE: 
            {
                alarm_ev_t ev_rsp;
                if(ALARM_ENABLE_DISABLE_OK == alarm_del(p_ev, &ev_rsp)){
                    ev_rsp.par1.enable = ALARM_EV_ENABLE == p_ev->par0.event ? 1 : 0;
                    err = alarm_add(&ev_rsp);
                }else{
                    err = SCHEDULER_ST_NOT_FOUND_INDEX;
                }
            }
            break;
            
        default :
            break;
    }

    alarm_event_check();
    return err;
}

tx_cmd_err_t scheduler_action_tx_rsp(u8 op, u16 addr_dst, scheduler_st_err_t st, u8 sch_index_get)
{
	alarm_ev_t sch_rsp = {{{0}}};
	int par_len = sizeof(sch_rsp);
	int found = 0;

	if(sch_index_get != ALARM_AUTO_CREATE_INDEX && sch_index_get <= ALARM_INDEX_MAX){
		found = alarm_get_by_id(&sch_rsp, sch_index_get);
	}
	
	sch_rsp.par0.valid_flag = 0; // init to invalid
	sch_rsp.par0.st = st;
	if(!found){
		if(LIGHT_CMD_SCHEDULE_ACTION_GET == op && SCHEDULER_ST_SUCCESS == st){
			sch_rsp.par0.st = SCHEDULER_ST_NOT_FOUND_INDEX;
		}
		
		sch_rsp.index = sch_index_get;
		par_len = OFFSETOF(alarm_ev_t, par1);
	}

	tx_cmd_err_t err = smart_home_cmd_tx_rsp(LIGHT_CMD_SCHEDULE_ACTION_RSP, addr_dst, (u8 *)&sch_rsp, par_len);
	LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"scheduler status st: %d(%s)", sch_rsp.par0.st, get_scheduler_status_err_st_string(sch_rsp.par0.st));

	return err;
}

tx_cmd_err_t scheduler_action_set_rx_cb(smart_home_nw_t *p_nw)
{
	tx_cmd_err_t err = TX_CMD_SUCCESS;
    alarm_ev_t *p_sch_set = (alarm_ev_t *)p_nw->par;
	scheduler_st_err_t st = alarm_ev_callback((u8 *)p_sch_set);
	if(p_sch_set->par0.ack){
		err = scheduler_action_tx_rsp(p_nw->op, p_nw->addr_src, st, p_sch_set->index);
	}
	
	return err;
}

tx_cmd_err_t scheduler_action_get_rx_cb(smart_home_nw_t *p_nw)
{
	//tx_cmd_err_t err = TX_CMD_SUCCESS;
    scheduler_action_get_t *p_action_get = (scheduler_action_get_t *)p_nw->par;
    
	tx_cmd_err_t err = scheduler_action_tx_rsp(p_nw->op, p_nw->addr_src, SCHEDULER_ST_SUCCESS, p_action_get->sch_index);
	if(TX_CMD_SUCCESS == err){
		#if SCHEDULER_ACTION_POLL_QUICK_RSP_EN
		scheduler_action_quick_rsp_record_and_check(p_nw);
		#endif
	}

	return err;
}

tx_cmd_err_t scheduler_list_get_rx_cb(smart_home_nw_t *p_nw)
{
	//tx_cmd_err_t err = TX_CMD_SUCCESS;
    // *p_list_get = (alarm_ev_t *)p_nw->par;
    alarm_list_t list = {0};
	u32 max_index = alarm_get_all_id(&list);
	int len_rsp = OFFSETOF(alarm_list_t, bit_map) + (max2(ALARM_CNT_MAX, (max_index+1)) + 7)/8;
	list.max_quantity = ALARM_CNT_MAX;
	
	return smart_home_cmd_tx_rsp(LIGHT_CMD_SCHEDULE_LIST_STATUS, p_nw->addr_src, (u8 *)&list, len_rsp);
}


void alarm_event_check(){
    rtc_set_week();

    if((alarm_next_pos == 0) || (is_need_sync_time())){
        return ;
    }

    foreach(i, ALARM_CNT_MAX){
        alarm_ev_t *p_alarm = &alarm_list[i];
        if(!is_valid_alarm_flag(p_alarm->par0.valid_flag)){
            continue;
        }
        
        if(p_alarm->par1.enable){
            if((p_alarm->hour == rtc.hour)
            && (p_alarm->minute == rtc.minute)
            && (p_alarm->second == rtc.second)){//if needed, please run alarm_event_check by sencond
                u8 result = 0;
                switch(p_alarm->par1.type){
                    case ALARM_TYPE_DAY:
                        if((!p_alarm->month || (p_alarm->month == rtc.month))
                        && (!p_alarm->par2.day || (p_alarm->par2.day == rtc.day))){
                            result = 1;
                        }
                    break;
                    
                    case ALARM_TYPE_WEEK:
                        if((p_alarm->par2.week) & (u8)(1 << rtc.week)){
                            result = 1;
                        }
                    break;

                    default:
                        break;
                }
                
                if(result){
                    alarm_callback(p_alarm);
                }
            }
        }
    }
}

/**
 * @brief       This function get alarm array index
 * @param[in]   id	- alarm index
 * @return      
 * @note        
 */
u8 alarm_find(u8 id){
    foreach(i, ALARM_CNT_MAX){
        alarm_ev_t *p_alarm = &alarm_list[i];
        if(is_valid_alarm_flag(p_alarm->par0.valid_flag) && (id == p_alarm->index)){
            return i; // alarm array index
        }
    }

    return ALARM_CNT_MAX; // alarm array index
}

/**
 * @brief       This function ...
 * @param[out]  pkt_rsp	- output response data
 * @param[in]   id		- scheduler index
 * @return      1: found, 0: not found
 * @note        
 */
int alarm_get_by_id(alarm_ev_t *pkt_rsp, u8 id)
{
    u8 array_idx = alarm_find(id);
    if(array_idx < ALARM_CNT_MAX){    // found
        memcpy(pkt_rsp, &alarm_list[array_idx], sizeof(alarm_ev_t));
        return 1;	// found
    }
    
    memset(pkt_rsp, 0, sizeof(alarm_ev_t));
    return 0;		// not found
}

STATIC_ASSERT(ALARM_CNT_MAX <= sizeof(alarm_list_t) * 8);

/**
 * @brief       This function get bit map of scheduler index
 * @param[out]  list_rsp- output data
 * @return      return max index of scheduler action
 * @note        
 */
int alarm_get_all_id(alarm_list_t *list_rsp)
{
	memset(list_rsp, 0, sizeof(alarm_list_t));
	u32 max_index = 0;
    foreach(i,ALARM_CNT_MAX){
        alarm_ev_t *p_alarm = &alarm_list[i];
        if(is_valid_alarm_flag(p_alarm->par0.valid_flag)){
        	u32 index = p_alarm->index;
        	if(index < sizeof(list_rsp->bit_map) * 8){
        		list_rsp->bit_map[index/8] |= BIT(index % 8);
        		max_index = max2(max_index, index);
        	}
        }
    }
    
    return max_index;
}

void check_event_after_set_time(const rtc_t *rtc_new, const rtc_t *rtc_old){
    u8 r = irq_disable();
    if(is_rtc_earlier(rtc_new, rtc_old)){   // should be earlier less than 1min ,
        memcpy(&rtc, rtc_old, sizeof(rtc_t));
        while(1){
            rtc_increase_and_check_event();
            if(rtc.second == rtc_new->second){
                memcpy(&rtc.year, &(rtc_new->year), TIME_SET_PAR_LEN);   // make sure recover rtc.
                break;
            }
        }
    }
    irq_restore(r);
}

#if SCHEDULER_ACTION_POLL_QUICK_RSP_EN
void scheduler_action_quick_rsp_proc()
{
	sch_action_poll_quick_rsp_proc_t *p_proc = &sch_action_poll_quick_rsp_proc;
	if(p_proc->addr_src){
		u32 timeout_us = TX_CMD_INTERVAL_AVERAGE_MS * 1000;
		if(clock_time_exceed(p_proc->tick_last_get, timeout_us)){
			memset(p_proc, 0, sizeof(sch_action_poll_quick_rsp_proc_t));
			//LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0, "scheduler get proc 3: clear");
		}
	}
}

void scheduler_action_quick_rsp_record_and_check(smart_home_nw_t *p_nw)
{
    scheduler_action_get_t *p_action_get = (scheduler_action_get_t *)p_nw->par;
	sch_action_poll_quick_rsp_proc_t *p_proc = &sch_action_poll_quick_rsp_proc;
	if(p_proc->addr_src && (p_proc->addr_src == p_nw->addr_src) &&
		p_action_get->sch_index >= p_proc->index_last_get){
		smart_home_cmd_buf_t *p_cmd = (smart_home_cmd_buf_t *)my_fifo_get(&smart_home_tx_cmd_fifo);
		if(p_cmd){
			if(CMD_BUF_HEAD_TYPE_SCH_ACTION_STATUS == p_cmd->cmd_head.par_type){
				my_fifo_pop(&smart_home_tx_cmd_fifo);
				//LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0, "scheduler get proc 2: quick pop last status message");
			}
		}
	}
	
	//LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0, "scheduler get proc 1: record");
	p_proc->tick_last_get = clock_time();
	p_proc->addr_src = p_nw->addr_src;
	p_proc->index_last_get = p_action_get->sch_index;
}
#endif

#endif

