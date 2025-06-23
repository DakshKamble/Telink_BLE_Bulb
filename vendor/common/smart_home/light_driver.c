/********************************************************************************************************
 * @file	light_driver.c
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
#include "proj_lib/ble/service/ble_ll_ota.h"
#include "vendor/common/lighting_model.h"
#include "vendor/common/lighting_model_HSL.h"
#include "vendor/common/lighting_model_xyL.h"
#include "vendor/common/lighting_model_LC.h"
#include "vendor/common/generic_model.h"
#include "vendor/common/scene.h"
#include "proj/mcu/watchdog_i.h"
#include "proj_lib/pm.h"
#else
#include "tl_common.h"
//#include "stack/ble/controller/ll/ll.h"
//#include "stack/ble/blt_config.h"
#include "stack/ble/service/ota/ota.h"
#include "drivers/watchdog.h"
#endif
#include "protocol.h"
#include "light_driver.h"
#if (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_KP18058)
#include "kp18058.h"
#endif


/** @addtogroup Mesh_Common
  * @{
  */
  
/** @defgroup Light
  * @brief Light Code.
  * @{
  */

#if SMART_HOME_ONOFF_EN
led_RGB_t	led_RGB = {255, 255, 255};
u8          led_lum = 100;
u8          led_ct = 100;
u8 			light_off = 0;
#if (0 == SMART_HOME_RGB_EN)
u8 			light_ct_flag = 1;		// only CT mode
#else
u8 			light_ct_flag = 0;		// default mode is RGB
#endif
u32 		lum_changed_time = 0;


// ------------- 
// 0-100%  (pwm's value index: this is pwm compare value, and the pwm cycle is 255*256)
static const u16 rgb_lumen_map[101] = {
  0,2*256+128,2*256+192,3*256,3*256+128,4*256,4*256+128,5*256,5*256+128,6*256,7*256,
      8*256,  9*256, 10*256, 11*256, 12*256, 13*256, 14*256, 15*256, 16*256, 17*256,
     18*256, 19*256, 21*256, 23*256, 25*256, 27*256, 29*256, 31*256, 33*256, 35*256,
     37*256, 39*256, 41*256, 43*256, 45*256, 47*256, 49*256, 51*256, 53*256, 55*256,
     57*256, 59*256, 61*256, 63*256, 65*256, 67*256, 69*256, 71*256, 73*256, 75*256,
     78*256, 81*256, 84*256, 87*256, 90*256, 93*256, 96*256, 99*256,102*256,105*256, 
    108*256,111*256,114*256,117*256,120*256,123*256,126*256,129*256,132*256,135*256,
    138*256,141*256,144*256,147*256,150*256,154*256,158*256,162*256,166*256,170*256,
    174*256,178*256,182*256,186*256,190*256,194*256,198*256,202*256,206*256,210*256,
    214*256,218*256,222*256,226*256,230*256,235*256,240*256,245*256,250*256,255*256,// must less or equal than (255*256)
};


void	pwm_set_lum (int id, u16 y, int pol)
{
    u32 lum = ((u32)y * PMW_MAX_TICK) / (255*256);

	pwm_set_cmp (id, pol ? PMW_MAX_TICK - lum : lum);
}

u32 get_pwm_cmp(u8 val, u8 lum)
{
    if(lum >= ARRAY_SIZE(rgb_lumen_map) - 1){
        lum = ARRAY_SIZE(rgb_lumen_map) - 1;
    }
    u16 val_lumen_map = rgb_lumen_map[lum];
    
#if LIGHT_ADJUST_STEP_EN
    light_step_correct_mod(&val_lumen_map, lum);
#endif
    u32 val_temp = val;
    return (val_temp * val_lumen_map) / 255;
}

void light_transition_init()
{
#if LIGHT_ADJUST_STEP_EN
    light_onoff_step_init();
#endif
	light_st_delay_init();
}

void light_adjust_R(u8 val, u8 lum)
{
	#if (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_KP18058)
	kp18058_light_adjust(KP18058_RED_ADDR, val, lum);
	#else // (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_PWM)
    pwm_set_lum (PWMID_R, get_pwm_cmp(val, lum), PWM_INV_R);
	#endif
}

void light_adjust_G(u8 val, u8 lum)
{
	#if (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_KP18058)
	kp18058_light_adjust(KP18058_GREEN_ADDR, val, lum);
	#else // (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_PWM)
    pwm_set_lum (PWMID_G, get_pwm_cmp(val, lum), PWM_INV_G);
	#endif
}

void light_adjust_B(u8 val, u8 lum)
{
	#if (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_KP18058)
	kp18058_light_adjust(KP18058_BLUE_ADDR, val, lum);
	#else // (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_PWM)
    pwm_set_lum (PWMID_B, get_pwm_cmp(val, lum), PWM_INV_B);
	#endif
}

#if (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_KP18058)
void light_adjust_C(u8 val, u8 lum)
{
	kp18058_light_adjust(KP18058_C_ADDR, val, lum);
}

void light_adjust_W(u8 val, u8 lum)
{
	kp18058_light_adjust(KP18058_W_ADDR, val, lum);
}

void light_adjust_rgbcw(u8 r_val, u8 g_val, u8 b_val, u8 c_val, u8 w_val, u8 lum)
{
	#if 1
	kp18058_light_adjust_rgbcw(r_val, g_val, b_val, c_val, w_val, lum);
	#else
	light_adjust_R(r_val, lum);
	light_adjust_G(g_val, lum);
	light_adjust_B(b_val, lum);
	light_adjust_C(c_val, lum);
	light_adjust_W(w_val, lum);
	#endif
}
#endif

static void light_change_ct_rgb_state(int ct_en)
{
	if(ct_en){
		light_ct_flag = 1;
		#if SMART_HOME_RGB_EN
		memset(&led_RGB, 0, sizeof(led_RGB));
		#endif
	}else{
		light_ct_flag = 0;
		#if SMART_HOME_CT_EN
		led_ct = 0;
		#endif
	}
}

void light_adjust_RGB_hw(led_RGB_t	*p_rgb, u8 lum)
{
#if (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_KP18058)
	kp18058_light_adjust_rgb(p_rgb->R, p_rgb->G, p_rgb->B, lum);
#else
	light_adjust_R(p_rgb->R, lum);
	light_adjust_G(p_rgb->G, lum);
	light_adjust_B(p_rgb->B, lum);
#endif
}

void light_adjust_RGB(led_RGB_t	*p_rgb, u8 lum)
{
    light_transition_init();

	#if LIGHT_TRANSITION_ALL_EN
	int transition_need = light_g_level_set_transition(lum, ST_TRANS_LIGHTNESS);
		#if (SMART_HOME_RGB_EN)
	transition_need |= light_g_level_set_transition(p_rgb->R, ST_TRANS_R);
	transition_need |= light_g_level_set_transition(p_rgb->G, ST_TRANS_G);
	transition_need |= light_g_level_set_transition(p_rgb->B, ST_TRANS_B);
		#endif
	#endif

	memcpy(&led_RGB, p_rgb, sizeof(led_RGB));
	led_lum = lum;
	light_off = 0;
	light_change_ct_rgb_state(0); // need to set to 0 even though it is lightness set command, because only when light_ct_flag is zero can run here when receive lightness set command.

	#if LIGHT_TRANSITION_ALL_EN
	if(0 == transition_need) // if not 0, will transition RGB in light_transition_proc()
	#endif
	{
		light_adjust_RGB_hw(p_rgb, lum);
	}
}

void light_adjust_CT_hw(u8 ct, u8 lum)
{
	u8 lum_warm = CEIL_DIV(((SH_CT_MAX - ct) * lum), SH_LIGHTNESS_MAX);
	u8 lum_cold = CEIL_DIV((ct * lum), SH_LIGHTNESS_MAX);
	// LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"---- G:%d, R:%d", lum_cold, lum_warm);

	light_adjust_R(0xFF, lum_warm);
	light_adjust_G(0xFF, lum_cold);
	light_adjust_B(0, 0);
}

void light_adjust_CT(u8 ct, u8 lum)
{
    light_transition_init();

	#if LIGHT_TRANSITION_ALL_EN
	int transition_need = light_g_level_set_transition(lum, ST_TRANS_LIGHTNESS);
		#if (SMART_HOME_RGB_EN)
	transition_need |= light_g_level_set_transition(ct, ST_TRANS_CTL_TEMP);
		#endif
	#endif

	led_ct = ct;
	led_lum = lum;
	light_off = 0;
	light_change_ct_rgb_state(1); // need to set to 0 even though it is lightness set command, because only when light_ct_flag is zero can run here when receive lightness set command.
	
	#if LIGHT_TRANSITION_ALL_EN
	if(0 == transition_need) // if not 0, will transition CT in light_transition_proc()
	#endif
	{
		light_adjust_CT_hw(led_ct, led_lum);
	}
}

void light_adjust_lightness(u8 lightness)
{
	if(!is_valid_lightness_sh(lightness)){
		return ;
	}
	
	if(light_ct_flag){
		light_adjust_CT(led_ct, lightness);
	}else{
		light_adjust_RGB(&led_RGB, lightness);
	}
}

void light_adjust_hw_all_off()
{
	led_RGB_t rgb = {0};
#if SMART_HOME_RGB_EN
	light_adjust_RGB_hw(&rgb, 0);
#endif
#if SMART_HOME_CT_EN
	light_adjust_CT_hw(0, 0);
#endif
}

void light_onoff_normal(u32 on)
{
    if(on){
    	light_adjust_lightness(led_lum);
	}else{
		#if LIGHT_TRANSITION_ALL_EN
		if(0 == light_g_level_set_transition(LEVEL_OFF, ST_TRANS_LIGHTNESS)) // if not 0, will transition CT in light_transition_proc()
		#endif
		{
        	light_adjust_hw_all_off();
        }

		light_off = 1; // should be after light_g_level_set_transition_() which will get present value that is depended on light_off.
	}
}


void light_onoff_hw(u32 on)
{
    #if LIGHT_ADJUST_STEP_EN
    light_onoff_step(on);
    #else
    light_onoff_normal(on);
	#endif
}

void light_onoff(u32 on)
{
    light_onoff_hw(on);
}

u8 get_current_lightness()
{
	return light_off ? (led_lum | BIT(7)) : led_lum;
}

#if SMART_HOME_SCENE_EN
void set_current_status2scene(lcmd_scene_set_t *p_set, u8 scene_id)
{
	memset(p_set, 0, sizeof(lcmd_scene_set_t));
	p_set->id = scene_id;
	p_set->ct_flag = light_ct_flag;
	p_set->rgb_flag = !light_ct_flag;
	p_set->lightness = light_off ? 0 : led_lum;
#if SMART_HOME_CT_EN
	p_set->ct = led_ct;
#endif
#if SMART_HOME_RGB_EN
	memcpy(&p_set->rgb, &led_RGB, sizeof(p_set->rgb));
#endif
}
#endif

void set_online_st_data(smart_home_light_online_st_t *p_st)
{
	p_st->lightness = get_current_lightness();
#if SMART_HOME_CT_EN
	p_st->ct_flag = light_ct_flag;
	p_st->ct = led_ct;
#else
	// have been initialed to be 0 before.
#endif
#if SMART_HOME_RGB_EN
	p_st->rgb_flag = !light_ct_flag;
	memcpy(&p_st->rgb, &led_RGB, sizeof(p_st->rgb));
#else
	// have been initialed to be 0 before.
#endif
}


#if (SMART_HOME_ONOFF_EN && (0 == SMART_HOME_CLIENT_APP_EN))
#if (MCU_CORE_TYPE == MCU_CORE_5316)
// if use GPIO_PC2, need to change pwm init from AS_PWM to AS_PWM0_N in user_init_pwm_().
STATIC_ASSERT((PWM_R != GPIO_PC2)&&(PWM_G != GPIO_PC2)&&(PWM_B != GPIO_PC2)&&(PWM_W != GPIO_PC2));
#endif

#if (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_PWM)
/*
disable for 8258 to reduce suspend current (1.2mA) and working current (0.4mA)
*/
void pwm_io_input_disable() 
{
	u32 pwm_pin[] = {PWM_R, PWM_G, PWM_B, PWM_W};
	foreach_arr(i,pwm_pin){
	    if(pwm_pin[i]){
	        gpio_set_input_en(pwm_pin[i], 0);
	    }
	}
}
#endif

#define MACRO_STR_CAT_PWM(str, pwm_id)	(str ## pwm_id)
#define GET_FUNC_PWM(pwm_id)			MACRO_STR_CAT_PWM(PWM, pwm_id)

#if 0 // (BLE_REMOTE_PM_ENABLE)
_attribute_ram_code_ 
#endif
void user_init_pwm(int retention_flag)
{
#if (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_PWM)
    pwm_set_cycle_and_duty (PWMID_R, PMW_MAX_TICK, PWM_INV_R ? 0 : PMW_MAX_TICK);
    pwm_set_cycle_and_duty (PWMID_G, PMW_MAX_TICK, PWM_INV_G ? 0 : PMW_MAX_TICK);
    pwm_set_cycle_and_duty (PWMID_B, PMW_MAX_TICK, PWM_INV_B ? 0 : PMW_MAX_TICK);
#endif
    
	u8 backup_onoff = light_off;
#if (LIGHT_ADJUST_STEP_EN || LIGHT_TRANSITION_ALL_EN)
	light_off = 1;
	light_adjust_hw_all_off();
#endif
    light_onoff(!backup_onoff);

#if (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_PWM)
    pwm_start (PWMID_R);
    pwm_start (PWMID_G);
    pwm_start (PWMID_B);

	#if (MCU_CORE_TYPE == MCU_CORE_8208)
    gpio_set_func (PWM_R, GET_FUNC_PWM(PWMID_R));
    gpio_set_func (PWM_G, GET_FUNC_PWM(PWMID_G));
    gpio_set_func (PWM_B, GET_FUNC_PWM(PWMID_B));
	#else
    gpio_set_func (PWM_R, AS_PWM);
    gpio_set_func (PWM_G, AS_PWM);
    gpio_set_func (PWM_B, AS_PWM);
	#endif

    pwm_io_input_disable();
#elif (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_KP18058)
	kp18058_init_i2c();
#endif
}
#endif







#define STEP_UNIT_MS				(10)

#if LIGHT_ADJUST_STEP_EN
/////////////////////////////  transition process //////////////////////////////////////
#define TYPE_FIX_STEP               1
#define TYPE_FIX_TIME               2

#define STEP_TYPE                   TYPE_FIX_TIME // TYPE_FIX_STEP // 

#if(STEP_TYPE == TYPE_FIX_STEP)
#define LIGHT_ADJUST_STEP           (2)   //unit: lum step 1--100
#define LIGHT_ADJUST_INTERVAL       (2)   // unit :10ms;     min:20ms
#else
#define LIGHT_ADJUST_TIME           (100)   //unit: 10ms
#define LIGHT_ADJUST_INTERVAL       (2)   // unit :10ms;     min:20ms
#endif

STATIC_ASSERT(STEP_UNIT_MS == 10); // must 10, because time delay unit of onoff is 10ms.


typedef struct{
    u32 time;
    s16 lum_temp;
    s16 lum_dst;
    u8 step;
    u8 step_mod;
    u8 remainder;
    u8 adjusting_flag;
}light_step_t;

static light_step_t light_step = {0};

enum{
    LUM_UP = 0,
    LUM_DOWN,
};

void get_next_lum(u8 direction)
{    
    u32 temp = light_step.remainder + light_step.step_mod;
    light_step.remainder = (u8)temp;
    
    if(LUM_UP == direction){
        light_step.lum_temp += light_step.step;
        if(temp >= 0x100){
            light_step.lum_temp += 1;
        }
        if(light_step.lum_temp >= light_step.lum_dst){
            light_step.lum_temp = light_step.lum_dst;
            light_step.remainder = 0;
        }
    }else{
        light_step.lum_temp -= light_step.step;
        if(temp >= 0x100){
            light_step.lum_temp -= 1;
        }
        if(light_step.lum_temp <= light_step.lum_dst){
            light_step.lum_temp = light_step.lum_dst;
            light_step.remainder = 0;
        }
    }
}

void get_step(u8 direction)
{
    light_step.remainder = 0;       // reset
    #if(STEP_TYPE == TYPE_FIX_STEP)
    light_step.step = LIGHT_ADJUST_STEP;
    light_step.step_mod = 0;
    #else   // fix time
    if(LUM_UP == direction){
        light_step.step = (light_step.lum_dst - light_step.lum_temp)/(LIGHT_ADJUST_TIME / LIGHT_ADJUST_INTERVAL);
        light_step.step_mod = (((light_step.lum_dst - light_step.lum_temp)%(LIGHT_ADJUST_TIME / LIGHT_ADJUST_INTERVAL))*256)/(LIGHT_ADJUST_TIME / LIGHT_ADJUST_INTERVAL);
    }else{
        light_step.step = (light_step.lum_temp - light_step.lum_dst)/(LIGHT_ADJUST_TIME / LIGHT_ADJUST_INTERVAL);
        light_step.step_mod = (((light_step.lum_temp - light_step.lum_dst)%(LIGHT_ADJUST_TIME / LIGHT_ADJUST_INTERVAL))*256)/(LIGHT_ADJUST_TIME / LIGHT_ADJUST_INTERVAL);
    }
    #endif
}

void light_step_correct_mod(u16 *pwm_val, u8 lum)
{
    #if(STEP_TYPE == TYPE_FIX_TIME)
    int temp_pwm = light_step.remainder;
    
    if(light_step.adjusting_flag && (light_step.lum_dst != light_step.lum_temp)
   && ((lum > 0) && (lum < ARRAY_SIZE(rgb_lumen_map) -1))
   && (light_step.remainder)){
        if(light_step.lum_dst > light_step.lum_temp){
            temp_pwm = *pwm_val + (temp_pwm * (rgb_lumen_map[lum+1] - rgb_lumen_map[lum])) / 256;

            if(temp_pwm > U16_MAX){
                temp_pwm = U16_MAX;
            }
        }else{
            temp_pwm = *pwm_val - (temp_pwm * (rgb_lumen_map[lum] - rgb_lumen_map[lum-1])) / 256;
            if(temp_pwm < 0){
                temp_pwm = 0;
            }
        }

        *pwm_val = temp_pwm;
    }
    #endif
}

void light_onoff_step_init()
{
    //light_step.adjusting_flag = 0;
    memset((u8 *)(&light_step), 0, sizeof(light_step));
}

void light_onoff_step(u8 on)
{
    if(light_step.adjusting_flag){
        //return ;
    }

    u8 set_flag= 1;
    
    if(on){
        if(light_off){
            if(0 == light_step.adjusting_flag){
                light_step.lum_temp = 0;
            }
            light_step.lum_dst = led_lum;
            get_step(LUM_UP);
    	}else{
    	    set_flag = 0;
    	    light_onoff_normal(1); // make sure on. unnecessary.
    	}
        light_off = 0;
	}else{
        if(light_off){
    	    set_flag = 0;
    	    light_onoff_normal(0); // make sure off. unnecessary.
    	}else{
            if(0 == light_step.adjusting_flag){
                light_step.lum_temp = led_lum;
            }
            light_step.lum_dst = 0;
            get_step(LUM_DOWN);
    	}
        light_off = 1;    
	}
	
    light_step.adjusting_flag = set_flag;
    light_step.time = 0;
}

void light_onoff_step_timer()
{
    if(light_step.adjusting_flag){
        if(0 == light_step.time){
            if(light_step.lum_dst != light_step.lum_temp){
                if(light_step.lum_temp < light_step.lum_dst){
                    get_next_lum(LUM_UP);
                }else{
                    get_next_lum(LUM_DOWN);
                }
                
                if(light_ct_flag){
                	light_adjust_CT_hw(led_ct, (u8)light_step.lum_temp);
                }else{
                	light_adjust_RGB_hw(&led_RGB, (u8)light_step.lum_temp);
                }
            }else{
                light_step.adjusting_flag = 0;
                memset((u8 *)(&light_step), 0, sizeof(light_step));
            }
        }
        
        light_step.time++;
        if(light_step.time >= LIGHT_ADJUST_INTERVAL){
            light_step.time = 0;
        }
    }
}
#endif

void light_onoff_step_timer_proc()
{
	u32 step_unit_us = STEP_UNIT_MS*1000;
	if((g_light_st_delay.delay_10ms
	#if LIGHT_ADJUST_STEP_EN
	 || light_step.adjusting_flag
	#endif
	 ) && clock_time_exceed(g_light_st_delay.tick_light_step_timer, step_unit_us)){
		if((u32)(g_light_st_delay.tick_light_step_timer - clock_time()) > (step_unit_us * 2 * SYSTEM_TIMER_TICK_1US)){
			g_light_st_delay.tick_light_step_timer = clock_time();
		}else{
			g_light_st_delay.tick_light_step_timer += step_unit_us * SYSTEM_TIMER_TICK_1US;
		}

		if(0 != g_light_st_delay.delay_10ms){
			g_light_st_delay.delay_10ms--;
			if(0 == g_light_st_delay.delay_10ms){
				light_onoff(g_light_st_delay.onoff);
				#if LIGHT_SAVE_ONOFF_STATE_EN
				smart_home_lum_save();
				#endif
			}else{
				return ;
			}
		}
		
		#if LIGHT_ADJUST_STEP_EN
		light_onoff_step_timer();
		#endif
	}

	#if LIGHT_TRANSITION_ALL_EN
	light_transition_proc();
	#endif
}

// ---- save parameters to flash -----
typedef struct{
    u8 light_off;
    u8 ct_flag;
    u8 lum;
    u8 ct;
    led_RGB_t rgb;
    u8 rsv[5];
}lum_save_t;

STATIC_ASSERT(sizeof(lum_save_t)%4 == 0);

static u32 smart_home_lum_save_addr = FLASH_ADDR_SMART_HOME_LUM_SAVE;

void smart_home_lum_save2flash()
{
	lum_save_t lum_save = {0};
	lum_save.lum = led_lum;
	lum_save.light_off = light_off;
	lum_save.ct_flag = light_ct_flag;
	lum_save.ct = led_ct;
	memcpy(&lum_save.rgb, &led_RGB, sizeof(lum_save.rgb));
    smart_home_par_store((u8 *)&lum_save, &smart_home_lum_save_addr, FLASH_ADDR_SMART_HOME_LUM_SAVE, sizeof(lum_save));
}

int smart_home_lum_retrieve()
{
	lum_save_t lum_save = {0};
    int err = smart_home_par_retrieve((u8 *)&lum_save, &smart_home_lum_save_addr, FLASH_ADDR_SMART_HOME_LUM_SAVE, sizeof(lum_save));
	if(err){
	}else{
		light_ct_flag = lum_save.ct_flag;
		led_lum = lum_save.lum;
		led_ct = lum_save.ct;
		memcpy(&led_RGB, &lum_save.rgb, sizeof(led_RGB));

		#if !WIN32
		if(is_ota_reboot_with_off_flag()){
			// LOG_MSG_LIB(TL_LOG_PROTOCOL, 0, 0,"---------is_state_after_ota");
			light_off = 1;
			clr_ota_reboot_with_off_flag();
		}else{
			#if LIGHT_SAVE_ONOFF_STATE_EN
			light_off = lum_save.light_off;
			#else
			// always on
			#endif
		}
		#endif
	}
	
    return err;
}

void smart_home_lum_save()
{
	lum_changed_time = clock_time() | 1;
}

void smart_home_lum_save_proc()
{
	// save current lum-val
	if(lum_changed_time && clock_time_exceed(lum_changed_time, LIGHT_SAVE_DELAY_MS * 1000)){
		lum_changed_time = 0;
		smart_home_lum_save2flash();
	}
}
#else
	#if 0 // (BLE_REMOTE_PM_ENABLE)
_attribute_ram_code_ 
	#endif
void user_init_pwm(int retention_flag)
{
	led_onoff_gpio(GPIO_LED, 0);
}
#endif


// ---------- LED proc ------------

#if 1
static u32 led_event_pending;
static int led_count = 0;

void cfg_led_event (u32 e)
{
	led_event_pending = e;
}

void cfg_led_event_stop ()
{
	led_event_pending = led_count = 0;
}

int is_led_busy()
{
    return (!(!led_count && !led_event_pending));
}

void led_onoff_gpio(u32 gpio, u8 on){
#if 0 // (PM_DEEPSLEEP_RETENTION_ENABLE)
    gpio_set_func (gpio, AS_GPIO);
    gpio_set_output_en (gpio, 0);
    gpio_write(gpio, 0);
    gpio_setup_up_down_resistor(gpio, on ? PM_PIN_PULLUP_10K : PM_PIN_PULLDOWN_100K);
#else
    gpio_set_func (gpio, AS_GPIO);
    gpio_set_output_en (gpio, 1);
    gpio_write(gpio, on);
#endif
}


#if (BLE_REMOTE_PM_ENABLE || (0 == SMART_HOME_ONOFF_EN))
void proc_led()
{
	static	u32 led_ton;
	static	u32 led_toff;
	static	int led_sel;						//
	static	u32 led_tick;
	static	int led_no;
	static	int led_is_on;

	if(!led_count && !led_event_pending) {
		return;  //led flash finished
	}

	if (led_event_pending)
	{
		// new event
		led_ton = (led_event_pending & 0xff) * 64000 * SYSTEM_TIMER_TICK_1US;
		led_toff = ((led_event_pending>>8) & 0xff) * 64000 * SYSTEM_TIMER_TICK_1US;
		led_count = (led_event_pending>>16) & 0xff;
		led_sel = led_event_pending>>24;

		led_event_pending = 0;
		led_tick = clock_time () + 30000000 * SYSTEM_TIMER_TICK_1US;
		led_no = 0;
		led_is_on = 0;
	}

	if( 1 ){
		if( (u32)(clock_time() - led_tick) >= (led_is_on ? led_ton : led_toff) ){
			led_tick = clock_time ();
			
			led_is_on = !led_is_on;
			if (led_is_on)
			{
				led_no++;
				if (led_no - 1 == led_count)
				{
					led_count = led_no = 0;
					led_onoff_gpio(GPIO_LED, 0);	// make sure off
					return ;
				}
			}
			
			int led_off = (!led_is_on || !led_ton) && led_toff;
			int led_on = led_is_on && led_ton;
			
			if( led_off || led_on  ){
				if (led_sel & BIT(0))
				{
					led_onoff_gpio(GPIO_LED, led_on);
				}
            }
        }
	}

}
#else

void proc_led(void)
{
	static	u32 led_ton;
	static	u32 led_toff;
	static	int led_sel;						//
	static	u32 led_tick;
	static	int led_no;
	static	int led_is_on;

	if(!led_count && !led_event_pending) {
		return;  //led flash finished
	}

	if (led_event_pending)
	{
		// new event
		led_ton = (led_event_pending & 0xff) * 64000 * SYSTEM_TIMER_TICK_1US;
		led_toff = ((led_event_pending>>8) & 0xff) * 64000 * SYSTEM_TIMER_TICK_1US;
		led_count = (led_event_pending>>16) & 0xff;
		led_sel = led_event_pending>>24;

		led_event_pending = 0;
		led_tick = clock_time () + 30000000 * SYSTEM_TIMER_TICK_1US;
		led_no = 0;
		led_is_on = 0;
	}

	if( 1 ){
		if( (u32)(clock_time() - led_tick) >= (led_is_on ? led_ton : led_toff) ){
			led_tick = clock_time ();
			int led_off = (led_is_on || !led_ton) && led_toff;
			int led_on = !led_is_on && led_ton;

			led_is_on = !led_is_on;
			if (led_is_on)
			{
				led_no++;
				//led_dbg++;
				if (led_no - 1 == led_count)
				{
					led_count = led_no = 0;
					light_onoff_hw(!light_off); // should not report online status again
					return ;
				}
			}
			
			if( led_off || led_on  ){
                if (led_sel & BIT(0))
                {
                    light_adjust_G (SH_LED_INDICATE_VAL * led_on, led_lum);
                }
                if (led_sel & BIT(1))
                {
                    light_adjust_B (SH_LED_INDICATE_VAL * led_on, led_lum);
                }
                if (led_sel & BIT(2))
                {
                    light_adjust_R (SH_LED_INDICATE_VAL * led_on, led_lum);
                }
            }
        }
	}
}
#endif 
#endif

#ifndef WIN32

/**
 * @brief       This function led sleep pro
 * @param[in]   level_out_en- open or close len
 * @return      none
 * @note        
 */
static inline void led_driver_output(int level_out_en)
{
	#if (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_KP18058)
	kp18058_light_adjust(GPIO_LED_OUT_ADDR, 0xff, (level_out_en ? SH_LIGHTNESS_MAX : 0));
	#elif(LED_DRIVER_MODE_USE == LED_DRIVER_MODE_PWM)
	gpio_write(GPIO_LED, level_out_en);
	#endif
}

void light_ev_with_sleep(u32 count, u32 half_cycle_us)
{
#if(LED_DRIVER_MODE_USE == LED_DRIVER_MODE_PWM)
	gpio_set_func(GPIO_LED, AS_GPIO);
	gpio_set_output_en(GPIO_LED, 1);
	gpio_set_input_en(GPIO_LED, 0);
#elif (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_KP18058)
	light_adjust_hw_all_off(); // light_adjust_rgbcw(0, 0, 0, 0, 0, 0); // init to off
#endif

	for(u32 i=0; i< count;i++){
#if(MODULE_WATCHDOG_ENABLE)
        wd_clear();
#endif
		led_driver_output(0);
		sleep_us(half_cycle_us);
#if(MODULE_WATCHDOG_ENABLE)
        wd_clear();
#endif
		led_driver_output(1);
		sleep_us(half_cycle_us);
	}

	led_driver_output(0);
}

void set_keep_onoff_state_after_ota()
{
	analog_write(SMART_HOME_REBOOT_FLAG_REG, analog_read(SMART_HOME_REBOOT_FLAG_REG) | FLD_OTA_REBOOT_FLAG);
}

void clr_keep_onoff_state_after_ota()
{
	analog_write(SMART_HOME_REBOOT_FLAG_REG, analog_read(SMART_HOME_REBOOT_FLAG_REG) & (~ FLD_OTA_REBOOT_FLAG));
}

int is_state_after_ota()
{
	return (analog_read(SMART_HOME_REBOOT_FLAG_REG) & FLD_OTA_REBOOT_FLAG);
}

void set_ota_reboot_with_off_flag()
{
	analog_write(SMART_HOME_REBOOT_FLAG_REG, analog_read(SMART_HOME_REBOOT_FLAG_REG) | FLD_OTA_REBOOT_WITH_OFF_FLAG);
}

void clr_ota_reboot_with_off_flag()
{
	analog_write(SMART_HOME_REBOOT_FLAG_REG, analog_read(SMART_HOME_REBOOT_FLAG_REG) & (~ FLD_OTA_REBOOT_WITH_OFF_FLAG));
}

int is_ota_reboot_with_off_flag()
{
	return (analog_read(SMART_HOME_REBOOT_FLAG_REG) & FLD_OTA_REBOOT_WITH_OFF_FLAG);
}

void set_ota_success_reboot_flag()
{
	analog_write(SMART_HOME_REBOOT_FLAG_REG, analog_read(SMART_HOME_REBOOT_FLAG_REG) | FLD_OTA_SUCCESS_REBOOT_FLAG);
}

void clr_ota_success_reboot_flag()
{
	analog_write(SMART_HOME_REBOOT_FLAG_REG, analog_read(SMART_HOME_REBOOT_FLAG_REG) & (~ FLD_OTA_SUCCESS_REBOOT_FLAG));
}

int is_ota_success_reboot_flag()
{
	return (analog_read(SMART_HOME_REBOOT_FLAG_REG) & FLD_OTA_SUCCESS_REBOOT_FLAG);
}


void show_ota_result(int result)
{
	set_keep_onoff_state_after_ota();
	#if SMART_HOME_ONOFF_EN
	if(light_off){
		set_ota_reboot_with_off_flag();
	}
	#endif
	if(result == OTA_REBOOT_NO_LED){
		// nothing
	}else if(result == OTA_SUCCESS){
		set_ota_success_reboot_flag();
		light_ev_with_sleep(3, 1000*1000);	//0.5Hz shine for  6 second
	}else{
		light_ev_with_sleep(30, 100*1000);	//5Hz shine for  6 second
		//write_reg8(0x8000,result); ;while(1);  //debug which err lead to OTA fail
	}
}

void show_factory_reset()
{
	light_ev_with_sleep(8, 500*1000);	//1Hz shine for  8 second
}
#endif



/**
  * @}
  */
    
/**
  * @}
  */


