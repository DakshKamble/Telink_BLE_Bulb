/********************************************************************************************************
 * @file	light_transition_all.c
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
#include "tl_common.h"
#include "protocol.h"
#include "light_driver.h"
#if (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_KP18058)
#include "kp18058.h"
#endif
#include "light_transition_all.h"


#if LIGHT_TRANSITION_ALL_EN

light_res_sw_trans_t light_res_sw[1] ;

#define P_ST_TRANS(idx, type)		(&light_res_sw[idx].trans[type])


// transition and delay proc -------------- 
/**
 * @brief       This function get level target and make sure it is in range of min and max.
 * @param[in]   level_target	- level target
 * @param[in]   min				- level min
 * @param[in]   max				- level max
 * @param[in]   st_trans_type	- A value in the enumeration type ST_TRANS_TYPE.
 * @return      level target
 * @note        
 */
s16 get_val_with_check_range(s32 level_target, int st_trans_type)
{
	s16 min = SH_CT_MIN;
	s16 max = SH_CT_MAX;
	
#if (SMART_HOME_RGB_EN)
	if(ST_TRANS_R == st_trans_type || ST_TRANS_G == st_trans_type || ST_TRANS_B == st_trans_type){
		max = 255;
	}
#endif

    if(level_target < min){
        // lightness would be set to 0 for PTS, which is like to OFF command, and 0 will not be allowed to set to lightness->last.
        if((ST_TRANS_LIGHTNESS == st_trans_type) && (LEVEL_OFF == level_target)){
            //level_target = LEVEL_OFF; // level_target has been LEVEL_OFF.
        }else{
            level_target = min;
        }
    }else if(level_target > max){
        level_target = max;
    }
    
    return (s16)level_target;
}

/**
 * @brief       This function get next step light level
 * @param[in]   idx				- light level
 * @param[in]   st_trans_type	- A value in the enumeration type ST_TRANS_TYPE.
 * @return      next step light level
 * @note        
 */
s16 light_get_next_level(int idx, int st_trans_type)
{
    st_transition_t *p_trans = P_ST_TRANS(idx, st_trans_type);
	s32 adjust_1p32768 = p_trans->step_1p32768+ p_trans->present_1p32768;
	s32 result = p_trans->present + (adjust_1p32768 / 32768);
	p_trans->present_1p32768 = adjust_1p32768 % 32768;

    result = get_val_with_check_range(result, st_trans_type);
    if((LEVEL_OFF == result) && (ST_TRANS_LIGHTNESS == st_trans_type) && (p_trans->step_1p32768 > 0)){
    	// result = LEVEL_OFF + 1; //
    }
	
	return (s16)result;
}

void light_transition_log(int st_trans_type, s16 present_level)
{
#if 0
	if(ST_TRANS_LIGHTNESS == st_trans_type){
		LOG_MSG_LIB(TL_LOG_NODE_BASIC,0,0,"present_lightness 0x%04x", s16_to_u16(present_level));
	#if (LIGHT_TYPE_CT_EN)
	}else if(ST_TRANS_CTL_TEMP == st_trans_type){
		LOG_MSG_LIB(TL_LOG_NODE_BASIC,0,0,"present_ctl_temp 0x%04x", s16_to_u16(present_level));
	}else if(ST_TRANS_CTL_D_UV == st_trans_type){
		LOG_MSG_LIB(TL_LOG_NODE_BASIC,0,0,"present_ctl_D_UV %d", present_level);
	#endif
	#if (LIGHT_TYPE_HSL_EN)
	}else if(ST_TRANS_HSL_HUE == st_trans_type){
		LOG_MSG_LIB(TL_LOG_NODE_BASIC,0,0,"present_hsl_hue 0x%04x", s16_to_u16(present_level));
	}else if(ST_TRANS_HSL_SAT == st_trans_type){
		LOG_MSG_LIB(TL_LOG_NODE_BASIC,0,0,"present_hsl_sat 0x%04x", s16_to_u16(present_level));
	#endif
	#if (LIGHT_TYPE_SEL == LIGHT_TYPE_XYL)
	}else if(ST_TRANS_XYL_X == st_trans_type){
		LOG_MSG_LIB(TL_LOG_NODE_BASIC,0,0,"present_xyl_x 0x%04x", s16_to_u16(present_level));
	}else if(ST_TRANS_XYL_Y == st_trans_type){
		LOG_MSG_LIB(TL_LOG_NODE_BASIC,0,0,"present_xyl_y 0x%04x", s16_to_u16(present_level));
	#endif
	}else{
		LOG_MSG_LIB(TL_LOG_NODE_BASIC,0,0,"xxxx 0x%04x", s16_to_u16(present_level));
	}
#endif
}

/**
 * @brief  Set Generic Level parameters(Global variables) for light.
 * @param  level: General Level value.
 * @param  init_time_flag: Reset transition parameter flag.
 *     @arg 0: Don't reset transition parameter.
 *     @arg 1: Reset transition parameter.
 * @param  st_trans_type: A value in the enumeration type ST_TRANS_TYPE.
 * @retval None
 */
static inline void light_res_sw_g_level_set(s16 level, int init_time_flag, int st_trans_type)
{
	//set_level_current_type(0, st_trans_type);
	st_transition_t *p_trans = P_ST_TRANS(0, st_trans_type);
	p_trans->present = level;
	if(init_time_flag){
		p_trans->target = level;
		p_trans->remain_t_cnt = 0;
		#if TRANSITION_DELAY_TIME_EN
		p_trans->delay_ms = 0;
		#endif
	}
}

/**
 * @brief  Set Generic Level for light by index.
 * @param  level: General Level value.
 * @param  init_time_flag: Reset transition parameter flag.
 *     @arg 0: Don't reset transition parameter.
 *     @arg 1: Reset transition parameter.
 * @param  st_trans_type: A value in the enumeration type ST_TRANS_TYPE.
 * @retval Whether the function executed successfully
 *   (0: success; others: error)
 */
int light_g_level_set_idx(s16 level, int init_time_flag, int st_trans_type)
{
	//st_transition_t *p_trans = P_ST_TRANS(0, st_trans_type);
	//if(level != p_trans->present){
		light_res_sw_g_level_set(level, init_time_flag, st_trans_type);
	//}
	
	return 0;
}


/**
 * @brief       This function get current value by transition type
 * @return      
 * @note        
 */
s16 light_get_current_value(int st_trans_type)
{
	st_transition_t *p_trans = P_ST_TRANS(0, st_trans_type);
	if(p_trans->remain_t_cnt){
		return p_trans->present;
	}
	
	if(ST_TRANS_LIGHTNESS == st_trans_type){
		return (light_off ? 0 : led_lum);
#if (SMART_HOME_CT_EN)
	}else if(ST_TRANS_CTL_TEMP == st_trans_type){
		return led_ct;
#endif
#if (SMART_HOME_RGB_EN)
	}if(ST_TRANS_R == st_trans_type){
		return led_RGB.R;
	}if(ST_TRANS_G == st_trans_type){
		return led_RGB.G;
	}if(ST_TRANS_B == st_trans_type){
		return led_RGB.B;
#endif
	}

	return 0;
}

/**
 * @brief       This function set level with transition time.
 * @param[in]   set_trans		- transition time parameters
 * @param[in]   st_trans_type	- transition type
 * @return      1: transition start.  2: no need transition
 * @note        
 */
int light_g_level_set_transition(s16 target_val, int st_trans_type)
{
	st_transition_t *p_trans = P_ST_TRANS(0, st_trans_type);
	s16 present_transition_backup = light_get_current_value(st_trans_type);
	
	memset(p_trans, 0, sizeof(st_transition_t));	// must after checking p_trans->remain_t_cnt 
	p_trans->present = present_transition_backup;	// keep present value
	if(p_trans->present == target_val){
		return 0; // no need transition
	}
	
	p_trans->remain_t_cnt = LIGHT_TRANSITION_STEP_CNT;
	p_trans->target = target_val;
	
	s32 delta = (p_trans->target - p_trans->present);
    p_trans->step_1p32768 = (delta * 32768) /(s32)(p_trans->remain_t_cnt);

    return 1;
}

/**
 * @brief       This function process light transition which include delay time and transition time.
 * @return      0:gradient compete; 1:gradient  not compete
 * @note        
 */
void light_transition_proc()
{
	static u32 tick_transition;
	u32 interval_us = LIGHT_TRANSITION_INTERVAL*1000;
	if(clock_time_exceed(tick_transition, interval_us)){
		tick_transition += (interval_us * sys_tick_per_us);
	}else{
		return ;
	}

	int transiting_flag = 0;
    int all_trans_ok = 1;   // include no transition
	foreach_arr(i,light_res_sw){
	    int dim_refresh_flag = 0;
		foreach_arr(trans_type,light_res_sw[i].trans){
			st_transition_t *p_trans = P_ST_TRANS(i, trans_type);
			int complete_level = 0;
			#if TRANSITION_DELAY_TIME_EN
			if(p_trans->delay_ms){
				p_trans->delay_ms--;
				transiting_flag = 1;
				if((0 == p_trans->delay_ms) && (0 == p_trans->remain_t_cnt)){
					complete_level = 1;
				}else{
				    all_trans_ok = 0;
				}
			}else
			#endif
			{
				if(p_trans->remain_t_cnt){
					transiting_flag = 1;
					if(p_trans->present != p_trans->target){
						//if(0 == (p_trans->remain_t_ms % LIGHT_TRANSITION_INTERVAL)){
                            s16 next_val = light_get_next_level(i, trans_type);
                            dim_refresh_flag = 1;
							light_g_level_set_idx(next_val, 0, trans_type);
							light_transition_log(trans_type, p_trans->present);
						//}
					}

					p_trans->remain_t_cnt--; // should keep remain time for Binary state transitions from 0x00 to 0x01,.
					
					if(0 == p_trans->remain_t_cnt){
						complete_level = 1;	// make sure the last status is same with target
					}else{
				        all_trans_ok = 0;
				    }
				}
			}

			if(complete_level){
                dim_refresh_flag = 1;
				light_g_level_set_idx(p_trans->target, 0, trans_type);
				light_transition_log(trans_type, p_trans->present);
			}
		}
		
		if(dim_refresh_flag){
            light_dim_refresh();   // dim refresh only when all level set ok
		}
	}

	//return transiting_flag;
}

void light_dim_refresh()
{
	u8 lum = (u8)light_get_current_value(ST_TRANS_LIGHTNESS);

	#if (SMART_HOME_CT_EN)
    if(light_ct_flag){
		u8 ct = (u8)light_get_current_value(ST_TRANS_CTL_TEMP);
		light_adjust_CT_hw(ct, lum);
		//LOG_MSG_LIB(TL_LOG_NODE_SDK,0,0,"pwm:0x%04x,0x%04x", warn_led_pwm, cold_led_pwm);
    }
    #endif
        
    #if (SMART_HOME_RGB_EN)
    if(!light_ct_flag){
        led_RGB_t led_RGB_temp;
        
        led_RGB_temp.R = (u8)light_get_current_value(ST_TRANS_R);
        led_RGB_temp.G = (u8)light_get_current_value(ST_TRANS_G);
        led_RGB_temp.B = (u8)light_get_current_value(ST_TRANS_B);
        
        light_adjust_RGB_hw(&led_RGB_temp, lum);
    }
	#endif
}
#endif


