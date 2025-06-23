/********************************************************************************************************
 * @file     kp18058.h
 *
 * @brief    This is the header file for BLE SDK
 *
 * @author	 BLE GROUP
 * @date         7,2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
 *******************************************************************************************************/
#pragma once

#if (WIN32 || __PROJECT_MESH_GW_NODE__)
#include "proj/tl_common.h"
#include "vendor/common/mesh_node.h"
#else
#include "tl_common.h"
#endif

#if (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_KP18058)
#define KP18058_BLUE_ADDR					0xe7 // byte 0 value with blue OUT0 address[3-4]
#define KP18058_RED_ADDR					0xeb // byte 0 value with red OUT1 address[5-6]
#define KP18058_GREEN_ADDR					0xee // byte 0 value with green OUT2 address[7-8]
#define KP18058_C_ADDR						0xf3 // byte 0 value with C OUT3 address[9-10]
#define KP18058_W_ADDR						0xf6 // byte 0 value with W OUT4 address[11-12]
#endif

typedef struct{
	u8 out_high;
	u8 out_low;
}kp18058_out_data_t;

typedef struct{
	u8 byte0;
	kp18058_out_data_t out_data;
}kp18058_out_t;

typedef struct{
	u8 byte0;
	kp18058_out_data_t out_data[5];
}kp18058_out_all_t;

int kp18058_is_parity(u8 value);
void kp18058_light_adjust(u8 byte0_val, u8 val, u8 lum);
void kp18058_light_adjust_rgbcw(u8 r_val, u8 g_val, u8 b_val, u8 c_val, u8 w_val, u8 lum);
void kp18058_light_adjust_rgb(u8 r_val, u8 g_val, u8 b_val, u8 lum);
void kp18058_light_adjust_cw(u8 c_val, u8 w_val, u8 lum);
void kp18058_init_i2c(void);


