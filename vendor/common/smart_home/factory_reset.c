/********************************************************************************************************
 * @file	factory_reset.c
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
#include "proj/tl_common.h"
#include "proj_lib/ble/blt_config.h"
#include "proj_lib/ble/service/ble_ll_ota.h"
#include "proj_lib/ble/ll//ll.h"
#include "app_beacon.h"
#else
#include "tl_common.h"
#include "stack/ble/ble.h"
//#include "stack/ble/blt_config.h"
#include "drivers/watchdog.h"
#endif
#include "protocol.h"

//FLASH_ADDRESS_EXTERN;

//////////////////Factory Reset///////////////////////////////////////////////////////////////////////
void show_ota_result(int result);
void show_factory_reset();
int factory_reset();


#define FACTORY_RESET_LOG_EN        0

#if !WIN32
static int adr_reset_cnt_idx = 0;

#if 0   // org mode
static int reset_cnt = 0;

#define SERIALS_CNT                     (5)   // must less than 7

const u8 factory_reset_serials[SERIALS_CNT * 2] = { 0, 3,    // [0]:must 0
                                                	0, 3,    // [2]:must 0
                                                	0, 3,    // [4]:must 0
                                                	3, 30,
                                                	3, 30};

#define RESET_CNT_RECOUNT_FLAG          0
#define RESET_FLAG                      0x80

void	reset_cnt_clean ()
{
	if (adr_reset_cnt_idx < 3840)
	{
		return;
	}
	flash_erase_sector (FLASH_ADR_RESET_CNT);
	adr_reset_cnt_idx = 0;
}

void write_reset_cnt (u8 cnt)
{
	flash_write_page (FLASH_ADR_RESET_CNT + adr_reset_cnt_idx, 1, (u8 *)(&cnt));
}

void clear_reset_cnt ()
{
    write_reset_cnt(RESET_CNT_RECOUNT_FLAG);
}

int reset_cnt_get_idx ()		//return 0 if unconfigured
{
	u8 *pf = (u8 *)FLASH_ADR_RESET_CNT;
	for (adr_reset_cnt_idx=0; adr_reset_cnt_idx<4096; adr_reset_cnt_idx++)
	{
	    u8 restcnt_bit = pf[adr_reset_cnt_idx];
		if (restcnt_bit != RESET_CNT_RECOUNT_FLAG)	//end
		{
        	if(((u8)(~(BIT(0)|BIT(1)|BIT(2)|BIT(3))) == restcnt_bit)  // the fourth not valid
        	 ||((u8)(~(BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5))) == restcnt_bit)){  // the fifth not valid
                clear_reset_cnt();
            }else{
			    break;
			}
		}
	}

    reset_cnt_clean();
    
	return 1;
}

u8 get_reset_cnt_bit ()
{
	if (adr_reset_cnt_idx < 0)
	{
	    reset_cnt_clean();
		return 0;
	}
	
	u8 reset_cnt;
	flash_read_page(FLASH_ADR_RESET_CNT + adr_reset_cnt_idx, 1, &reset_cnt);
	return reset_cnt;
}

void increase_reset_cnt ()
{
	u8 restcnt_bit = get_reset_cnt_bit();
	foreach(i,8){
        if(restcnt_bit & BIT(i)){
            if(i < 3){
                reset_cnt = i;
            }else if(i < 5){
                reset_cnt = 3;
            }else if(i < 7){
                reset_cnt = 4;
            }
            
            restcnt_bit &= ~(BIT(i));
            write_reset_cnt(restcnt_bit);
            break;
        }
	}
}

int factory_reset_handle ()
{
    reset_cnt_get_idx();   
    u8 restcnt_bit; 
    restcnt_bit = get_reset_cnt_bit();
	if(restcnt_bit == RESET_FLAG){
        irq_disable();
        factory_reset();
        show_ota_result(OTA_SUCCESS);
	    start_reboot();
	}else{
        increase_reset_cnt();
	}
	return 0;
}

int factory_reset_cnt_check ()
{
    static u8 clear_st = 3;
    static u32 reset_check_time;

	if(0 == clear_st) return 0;

	if(3 == clear_st){
        clear_st--;
        reset_check_time = factory_reset_serials[reset_cnt*2];
    }
    
	if((2 == clear_st) && clock_time_exceed(0, reset_check_time*1000*1000)){
	    clear_st--;
	    reset_check_time = factory_reset_serials[reset_cnt*2 + 1];
	    if(3 == reset_cnt || 4 == reset_cnt){
            increase_reset_cnt();
        }
	}
    
	if((1 == clear_st) && clock_time_exceed(0, reset_check_time*1000*1000)){
	    clear_st = 0;
        clear_reset_cnt();
	}
	
	return 0;
}

#else

/****************************************
new factory reset:
user can change any one of factory_reset_serials, and also can change SERIALS_CNT
*****************************************/

#ifndef FACTORY_RESET_SERIALS      // user can define in "user_app_config.h"
#define FACTORY_RESET_SERIALS      { 0, 3, \
                                     0, 3, \
                                     0, 3, \
                                     3, 30,\
                                     3, 30,}
#endif

const u8 factory_reset_serials[] = FACTORY_RESET_SERIALS;

#define RESET_CNT_INVALID               0
#define RESET_TRIGGER_VAL               (sizeof((factory_reset_serials)))

STATIC_ASSERT(sizeof(factory_reset_serials) % 2 == 0);
STATIC_ASSERT(sizeof(factory_reset_serials) < 100);

void	reset_cnt_clean ()
{
	if (adr_reset_cnt_idx < 3840)
	{
		return;
	}
	flash_erase_sector (FLASH_ADR_RESET_CNT);
	adr_reset_cnt_idx = 0;
}

void write_reset_cnt (u8 cnt) // reset cnt value from 1 to 254, 0 is invalid cnt
{
    reset_cnt_clean ();
	flash_write_page (FLASH_ADR_RESET_CNT + adr_reset_cnt_idx, 1, (u8 *)(&cnt));
}

void clear_reset_cnt ()
{
    write_reset_cnt(RESET_CNT_INVALID);
}

void reset_cnt_get_idx ()		//return 0 if unconfigured
{
	u8 *pf = (u8 *)FLASH_ADR_RESET_CNT;
	for (adr_reset_cnt_idx=0; adr_reset_cnt_idx<4096; adr_reset_cnt_idx++)
	{
	    u8 restcnt = pf[adr_reset_cnt_idx];
		if (restcnt != RESET_CNT_INVALID)	//end
		{
		    if(0xFF == restcnt){
		        // do nothing
		    }else if((restcnt > RESET_TRIGGER_VAL) || (restcnt & BIT(0))){   // invalid state
                clear_reset_cnt();
            }
			break;
		}
	}
}

u8 get_reset_cnt () // reset cnt value from 1 to 254, 0 is invalid cnt
{
	u8 reset_cnt;
	flash_read_page(FLASH_ADR_RESET_CNT + adr_reset_cnt_idx, 1, &reset_cnt);
	return reset_cnt;
}

void increase_reset_cnt ()
{
	u8 reset_cnt = get_reset_cnt();
	if(0xFF == reset_cnt){
	    reset_cnt = 0;
	}else if(reset_cnt){
	    clear_reset_cnt();      // clear current BYTE and then use next BYTE
        adr_reset_cnt_idx++;
	}
	
	reset_cnt++;
	write_reset_cnt(reset_cnt);
	#if FACTORY_RESET_LOG_EN
    LOG_USER_MSG_INFO(0,0,"cnt %d\r\n",reset_cnt);
    #endif
}

int factory_reset_handle ()
{
    reset_cnt_get_idx();   
	if(get_reset_cnt() == RESET_TRIGGER_VAL){
        irq_disable();
        factory_reset();
        #if FACTORY_RESET_LOG_EN
        LOG_USER_MSG_INFO(0,0,"factory reset success\r\n");
        #endif
        show_factory_reset();
	    start_reboot();
	}else{
        increase_reset_cnt();
	}
	return 0;
}

#define VALID_POWER_ON_TIME_US 	(50*1000)
int factory_reset_cnt_check ()
{
    static u8 clear_st = 4;
    static u32 reset_check_time;

	if(0 == clear_st) return 0;
	if(4 == clear_st && clock_time_exceed(0, VALID_POWER_ON_TIME_US)){
		clear_st--;
		factory_reset_handle();
	}
	if(3 == clear_st){
        clear_st--;
        reset_check_time = factory_reset_serials[get_reset_cnt() - 1];
    }
    
	if((2 == clear_st) && clock_time_exceed(0, reset_check_time*1000*1000)){
	    clear_st--;
        increase_reset_cnt();
	    reset_check_time = factory_reset_serials[get_reset_cnt() - 1];
	}
    
	if((1 == clear_st) && clock_time_exceed(0, reset_check_time*1000*1000)){
	    clear_st = 0;
        clear_reset_cnt();
        #if FACTORY_RESET_LOG_EN
        LOG_USER_MSG_INFO(0,0,"cnt clear\r\n");
        #endif
	}
	
	return 0;
}

#endif
#endif


int factory_reset(){
	u8 r = irq_disable ();
	for(int i = 0; i < (FLASH_ADDR_SMART_HOME_END - FLASH_ADDR_SMART_HOME_START) / 4096; ++i){
	    u32 adr = FLASH_ADDR_SMART_HOME_START + i*0x1000;
	    if(adr != FLASH_ADR_RESET_CNT){
		    flash_erase_sector(adr);
		}
	}
	
	//for(int i = 1; i < (FLASH_ADR_PAR_USER_MAX - (CFG_SECTOR_ADR_CALIBRATION_CODE)) / 4096; ++i){
		//flash_erase_sector(CFG_SECTOR_ADR_CALIBRATION_CODE + i*0x1000);
	//}

    flash_erase_sector(FLASH_ADR_RESET_CNT);    // at last should be better, when power off during factory reset erase.

    irq_restore(r);
	return 0;
}

void kick_out(int led_en){
	#if !WIN32
	// add terminate cmd 
	if(is_blt_gatt_connected()){
		bls_ll_terminateConnection (0x13);
		sleep_us(500000);   // wait terminate send completed.
	}	
	#endif
	
	factory_reset();
    #if !WIN32
    if(led_en){
        show_factory_reset();
    }
    start_reboot();
    #endif
}

