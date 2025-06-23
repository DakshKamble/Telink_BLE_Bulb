/********************************************************************************************************
 * @file	sh_time.c
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
#include "sh_time.h"
#include "sh_scheduler.h"


#if(SMART_HOME_TIME_EN)

rtc_t rtc = {
    /*.year = */	1970,
    /*.month = */	1,
    /*.day = */		1,
    /*.hour = */	0,
    /*.minute = */	0,
    /*.second = */	0,
    /*.week = */	1,
};

const u8 days_by_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define LOG_RTC_DEBUG(pbuf,len,format,...)		//LOG_MSG_LIB(TL_LOG_PROTOCOL, pbuf,len,format,##__VA_ARGS__)


static inline u8 is_leap_year(const u16 year)
{
    return ((year%4==0 && year%100!=0) || year%400==0);
}

static inline u8 get_days_by_month(const u16 year, const u8 month)
{
    if((2==month) && (is_leap_year(year))){
        return 29;
    }else{
        return days_by_month[(month-1)%12];
    }
}
static inline u8 get_week()
{
    u32 y = rtc.year;
    u32 m = rtc.month;
    u32 d = rtc.day;
    if((1 == m) || (2 == m)){
        y = y - 1;
        m = m + 12;
    }
    //sunday is 0
    u8 week = ((d+2*m+3*(m+1)/5+y+y/4-y/100+y/400)+1) % 7;
    return week;
}

void rtc_set_week()
{
    rtc.week = get_week();
}

int time_par_check(const rtc_t * rtc_set)
{
    u8 days = get_days_by_month(rtc_set->year, rtc_set->month);
    
    if((!rtc_set->year)
      ||(!rtc_set->month || (rtc_set->month > 12))
      ||(!rtc_set->day || (rtc_set->day > days))
      ||(rtc_set->hour >= 24)
      ||(rtc_set->minute >= 60)
      ||(rtc_set->second >= 60)){
            return -1;
    }

    return 0;
}

rx_cb_err_t rtc_set_time(const rtc_t *rtc_set)
{
    //check;
    if(time_par_check(rtc_set)){
        return RX_CB_ERR_TIME_INVALID;
    }
    
    memcpy(&(rtc.year), rtc_set, 7);
    rtc_set_week();
    rtc.tick_last = clock_time();
	LOG_MSG_LIB(TL_LOG_PROTOCOL, (u8 *)&rtc, OFFSETOF(rtc_t, tick_last), "time set done, rtc: ");

    return RX_CB_SUCCESS;
}

tx_cmd_err_t time_tx_rsp(u16 addr_dst, u8 st)
{
	lcmd_time_st_t cmd_status = {0};
	cmd_status.st = st;
    memcpy(&cmd_status.time.year, &rtc.year, sizeof(cmd_status.time));

	return smart_home_cmd_tx_rsp(LIGHT_CMD_TIME_STATUS, addr_dst, (u8 *)&cmd_status, sizeof(cmd_status));
}

rx_cb_err_t time_set_rx_cb(smart_home_nw_t *p_nw)
{
	lcmd_time_set_t *p_set = (lcmd_time_set_t *)p_nw->par;
	rx_cb_err_t err = rtc_set_time((rtc_t *)&p_set->year);
	
	rtc_t rtc_old, rtc_new;
	memcpy(&rtc_old, &rtc, sizeof(rtc_t));
	memcpy(&rtc_new.year, &p_set->year, TIME_SET_PAR_LEN);
	if(RX_CB_SUCCESS == err){
		//ok
		check_event_after_set_time(&rtc_new, &rtc_old);
		//cfg_led_event(LED_EVENT_FLASH_1HZ_3T);
	}else{
		//invalid params
		//cfg_led_event(LED_EVENT_FLASH_4HZ_3T);
	}
	
	if(LIGHT_CMD_TIME_SET_ACK == p_nw->op){ // response
		time_tx_rsp(p_nw->addr_src, err);
	}
	
	return err;
}

rx_cb_err_t time_status_rx_cb(smart_home_nw_t *p_nw)
{
	lcmd_time_st_t *p_status = (lcmd_time_st_t *)p_nw->par;
	rx_cb_err_t err = RX_CB_SUCCESS;

	if(is_need_sync_time()){
		err = rtc_set_time((rtc_t *)&p_status->time.year);
	}
	
	return err;
}

void rtc_increase_and_check_event()
{
    rtc.second++;
    if(rtc.second >= 60){
        rtc.second %= 60;
        rtc.minute++;

        if(60 == rtc.minute){
            rtc.minute = 0;
            rtc.hour++;
            if(24 == rtc.hour){
                rtc.hour = 0;
                rtc.day++;
                u8 days = get_days_by_month(rtc.year, rtc.month);
                if((days+1) == rtc.day){
                    rtc.day = 1;
                    rtc.month++;
                    if(12+1 == rtc.month){
                        rtc.month = 1;
                        rtc.year++;
                    }
                }
                rtc_set_week();
            }
        }
    }

    #if(SMART_HOME_SCHEDULER_EN)
    alarm_event_check();
    #endif
}

STATIC_ASSERT(TIME_PERIOD_PUBLISH_INTERVAL_S < 240);

void time_period_publish_proc()
{
	static u32 time_period_publish_cycle_us = 0;
    static u32 time_period_publish_tick = 0;
    if(clock_time_exceed(time_period_publish_tick, time_period_publish_cycle_us)){
    	time_period_publish_tick = clock_time();
		// LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"cycle: %d", time_period_publish_cycle_us);
    	u32 rand_val = rand_sh();
    	if(time_period_publish_cycle_us != 0){
    		if(is_current_time_valid()){
    			time_tx_rsp(SMART_HOME_GROUP_ALL, RX_CB_SUCCESS);
    		}
    	}

    	time_period_publish_cycle_us = (TIME_PERIOD_PUBLISH_INTERVAL_S * 1000 * 1000) + rand_val % (5000*1000);
	}
}

void rtc_run()
{
    static u32 tick_rtc_run;
    if(clock_time_exceed(tick_rtc_run, 100*1000)){  // less irq_disable() should be better.
        tick_rtc_run = clock_time();		
        
		// u8 r = irq_disable();   // no need disable, because of process command in buffer. //avoid interrupt by set time command.
		u32 t_last = rtc.tick_last; // rtc.tick_last may be set value in irq of set time command	
		u32 time = clock_time();    // get value must after t_last; 
        u32 t_delta = time - t_last;
		
        if(t_delta > SYSTEM_TIMER_TICK_1S){
            u32 second_cnt = t_delta/SYSTEM_TIMER_TICK_1S;
            rtc.tick_last += SYSTEM_TIMER_TICK_1S*second_cnt;
            
            static u8 rtc_init_flag = 1;
            if(rtc_init_flag){
                rtc_init_flag = 0;
                rtc_set_week();
                #if(SMART_HOME_SCHEDULER_EN)
                alarm_init();
                #endif
            }
            
            foreach(i,second_cnt){
                rtc_increase_and_check_event();
           	}

			LOG_RTC_DEBUG(0, 0, "%02d-%02d-%02d %02d:%02d:%02d.%03d",rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second, (t_delta%SYSTEM_TIMER_TICK_1S)/SYSTEM_TIMER_TICK_1MS); 
		}
        // irq_restore(r);
    }

	time_period_publish_proc();

#if SCHEDULER_ACTION_POLL_QUICK_RSP_EN
	scheduler_action_quick_rsp_proc();
#endif
}

static inline int is_valid_rtc_val(const rtc_t *r){
    return (r->year >= 2000);
}

int is_rtc_earlier(const rtc_t *rtc_new, const rtc_t *rtc_old){    // 
    if(is_valid_rtc_val(rtc_new) && is_valid_rtc_val(rtc_old)){
        if (rtc_new->year > rtc_old->year){
            return 1;
        }else if(rtc_new->year == rtc_old->year){
            if(rtc_new->month > rtc_old->month){
                return 1;
            }else if(rtc_new->month == rtc_old->month){
                if (rtc_new->day > rtc_old->day){
                    return 1;
                }else if(rtc_new->day == rtc_old->day){
                    if (rtc_new->hour > rtc_old->hour){
                        return 1;
                    }else if(rtc_new->hour == rtc_old->hour){
                        if(rtc_new->minute > rtc_old->minute){
                            return 1;
                        }else if(rtc_new->minute == rtc_old->minute){
                            if(rtc_new->second > rtc_old->second){
                                return 1;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int is_need_sync_time()
{
    return (!is_valid_rtc_val(&rtc));
}

int is_current_time_valid()
{
    return (is_valid_rtc_val(&rtc));
}

#if 0
int is_need_sync_rtc(rtc_t *r)
{
    return (!((!is_valid_rtc_val(r)) && is_valid_rtc_val(&rtc)));
}
#endif

void memcopy_rtc(void *out)
{
    memcpy(out, &rtc.year, TIME_SET_PAR_LEN);
}

void memcopy_rtc_hhmmss(void *out)
{
    memcpy(out, &rtc.hour, 3);
}

#endif

