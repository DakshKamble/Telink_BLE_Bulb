/********************************************************************************************************
 * @file	light_driver.h
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
#include "vendor/common/mesh_node.h"
#else
#include "tl_common.h"
#endif
#include "light_transition_all.h"

/** @addtogroup Mesh_Common
  * @{
  */
  
/** @defgroup Light
  * @brief Light Code.
  * @{
  */

// -------------------- configuration ----------------
#ifndef LIGHT_ADJUST_STEP_EN
#define LIGHT_ADJUST_STEP_EN    			0
#endif

#define LIGHT_SAVE_DELAY_MS    				(3000)
#define LIGHT_SAVE_ONOFF_STATE_EN    		0		// 0 means always being ON after power up.

#define SH_LED_INDICATE_VAL    				(0xff)

#define SH_LIGHTNESS_MIN					(1)		// 0 means OFF
#define SH_LIGHTNESS_MAX					(100)
#define SH_CT_MIN							(0)
#define SH_CT_MAX							(100)

#define LED_DRIVER_MODE_PWM					1
#define LED_DRIVER_MODE_KP18058				2

#if LED_DRIVER_MODE_KP18058_ENABLE // can not compare PCBA SEL here, because different app_config_h use different PCBA define value.
#define LED_DRIVER_MODE_USE					LED_DRIVER_MODE_KP18058
#else
#define LED_DRIVER_MODE_USE					LED_DRIVER_MODE_PWM
#endif

//////////////////////////////////////////////////////
#define LED_MASK							0x07
#define	config_led_event(on,off,n,sel)		(on | (off<<8) | (n<<16) | (sel<<24))

#define	LED_EVENT_FLASH_4HZ_10S				config_led_event(2,2,40,LED_MASK)
#define	LED_EVENT_FLASH_STOP				config_led_event(1,1,1,LED_MASK)
#define	LED_EVENT_FLASH_2HZ_2S				config_led_event(4,4,4,LED_MASK)
#define	LED_EVENT_FLASH_1HZ_1S				config_led_event(8,8,1,LED_MASK)
#define	LED_EVENT_FLASH_1HZ_2S				config_led_event(8,8,2,LED_MASK)
#define	LED_EVENT_FLASH_1HZ_3S				config_led_event(8,8,3,LED_MASK)
#define	LED_EVENT_FLASH_1HZ_4S				config_led_event(8,8,4,LED_MASK)
#define	LED_EVENT_FLASH_4HZ					config_led_event(2,2,0,LED_MASK)
#define	LED_EVENT_FLASH_1HZ					config_led_event(8,8,0,LED_MASK)
#define	LED_EVENT_FLASH_4HZ_1T				config_led_event(2,2,1,LED_MASK)
#define	LED_EVENT_FLASH_4HZ_3T				config_led_event(2,2,3,LED_MASK)
#define	LED_EVENT_FLASH_1HZ_3T				config_led_event(8,8,3,LED_MASK)
#define	LED_EVENT_FLASH_0p25HZ_1T			config_led_event(4,60,1,LED_MASK)
#define	LED_EVENT_FLASH_2HZ_1T				config_led_event(4,4,1,LED_MASK)
#define	LED_EVENT_FLASH_2HZ_2T				config_led_event(4,4,2,LED_MASK)

// ----------------------------
#ifndef CEIL_DIV
#define CEIL_DIV(A, B)                  		(((A) + (B) - 1) / (B))
#endif


/*
freq = CLOCK_SYS_CLOCK_HZ / (PMW_MAX_TICK)
*/
#define         PMW_MAX_TICK_BASE	            (255)   // must 255
#define         PMW_MAX_TICK_MULTI	            (209)   // 209: freq = 600.4Hz
#define         PMW_MAX_TICK_1		            (PMW_MAX_TICK_BASE*PMW_MAX_TICK_MULTI)  // must less or equal than (255*256)
#if (CLOCK_SYS_CLOCK_HZ == 16000000)
#define         PMW_MAX_TICK		            (PMW_MAX_TICK_1 / 2)
#elif (CLOCK_SYS_CLOCK_HZ == 48000000)
//TBD
#else   // default 32M
#define         PMW_MAX_TICK		            (PMW_MAX_TICK_1)
#endif


typedef struct{
    u8 R;
    u8 G;
    u8 B;
}led_RGB_t;


// -------------------------------
static inline int is_valid_lightness_sh(u8 lightness)
{
	return ((lightness >= SH_LIGHTNESS_MIN) && (lightness <= SH_LIGHTNESS_MAX));
}
static inline int is_valid_ct_sh(u8 ct)
{
	return (ct <= SH_CT_MAX);
}


// ----------------- extern --------------
void light_onoff(u32 on);
void user_init_pwm(int retention_flag);
u8 get_current_lightness();
void light_adjust_RGB_hw(led_RGB_t	*p_rgb, u8 lum);
void light_adjust_RGB(led_RGB_t *p_rgb, u8 lum);
void light_adjust_CT_hw(u8 ct, u8 lum);
void light_adjust_CT(u8 ct, u8 lum);
void light_adjust_lightness(u8 lightness);
void smart_home_lum_save();
int smart_home_lum_retrieve();
void smart_home_lum_save_proc();
void proc_led(void);
void cfg_led_event (u32 e);
void cfg_led_event_stop ();
int is_led_busy();
void show_ota_result(int result);
void show_factory_reset();
void led_onoff_gpio(u32 gpio, u8 on);
void set_keep_onoff_state_after_ota();
void clr_keep_onoff_state_after_ota();
int is_state_after_ota();
void set_ota_reboot_with_off_flag();
void clr_ota_reboot_with_off_flag();
int is_ota_reboot_with_off_flag();
void set_ota_success_reboot_flag();
void clr_ota_success_reboot_flag();
int is_ota_success_reboot_flag();
void light_step_correct_mod(u16 *pwm_val, u8 lum);
void light_onoff_step_init();
void light_onoff_step(u8 on);
void light_onoff_step_timer();
void light_onoff_step_timer_proc();
u32 get_pwm_cmp(u8 val, u8 lum);

extern u8	light_ct_flag;
extern u8	led_lum;
extern u8	led_ct;
extern u8	light_off;
extern led_RGB_t led_RGB;

/**
  * @}
  */
    
/**
  * @}
  */

