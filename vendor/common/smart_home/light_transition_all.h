/********************************************************************************************************
 * @file	light_transition_all.h
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

#define LIGHT_TRANSITION_INTERVAL    	(20)   	// unit :ms;     min:20ms; max 100ms
#define LIGHT_TRANSITION_DURATION    	(1000)	// unit :ms;
#define LIGHT_TRANSITION_STEP_CNT    	(LIGHT_TRANSITION_DURATION / LIGHT_TRANSITION_INTERVAL)

	#if (LIGHT_ADJUST_STEP_EN)
#error can not enable both of LIGHT_TRANSITION_ALL_EN and LIGHT_ADJUST_STEP_EN
	#endif

enum ST_TRANS_TYPE{					// type of State transition
	ST_TRANS_LIGHTNESS  	= 0,	// share with power level
	#if (SMART_HOME_CT_EN)
	ST_TRANS_CTL_TEMP,
	#endif
	#if (SMART_HOME_RGB_EN)
	ST_TRANS_R,
	ST_TRANS_G,
	ST_TRANS_B,
	#endif
	ST_TRANS_MAX,
};

#define TRANSITION_DELAY_TIME_EN		0 // use delay proc of light_onoff_step_timer_proc_()

typedef struct{
	s16 target_val;		// include onoff and level
	s16 present_val;
}mesh_set_trans_t;

typedef struct{
	s32 step_1p32768;   // (1 / 32768 level unit)
	u32 remain_t_cnt;	// unit LIGHT_TRANSITION_INTERVAL: max 26bit: 38400*1000ms
	#if TRANSITION_DELAY_TIME_EN
	u16 delay_ms;		// unit ms
	#endif
    s16 present;        // all value transfer into level, include CT.
    s16 present_1p32768;// (1 / 32768 level unit)
    s16 target;
}st_transition_t;

typedef struct{
	st_transition_t trans[ST_TRANS_MAX];	// state transition
	// may add member here later
}light_res_sw_trans_t;	// no need save

#define LEVEL_OFF				(0)	// don't change // unsigned char for level



void light_dim_refresh();
void light_transition_proc();
int light_g_level_set_transition(s16 target_val, int st_trans_type);

