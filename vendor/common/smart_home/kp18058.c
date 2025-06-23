/********************************************************************************************************
 * @file	kp18058.c
 *
 * @brief	for TLSR chips
 *
 * @author	Mesh Group
 * @date	2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "kp18058.h"
#include "light_driver.h"
#include "protocol.h"


#if (LED_DRIVER_MODE_USE == LED_DRIVER_MODE_KP18058)
/**
 * @brief       This function judge data parity of kp18058
 * @param[in]   value	- write byte data
 * @return      bool value: 1 ture, 0 false
 * @note        
 */
int kp18058_is_parity(u8 value)
{
	int num = 0;
	for(int i = 1; i < 8; ++i){
		if((value >> i) & 0x01){
			num++;
		}
	}

	return (num & 1); // bool type
}

/**
 * @brief       This function get value for kp18058 out value.
 * @param[out]  out_buff- pointer to output buffer
 * @param[in]   val		- color value, range from 0-255
 * @param[in]   lum		- lightness or lumina, range from 0-100
 * @return      none
 * @note        
 */
void kp18058_get_out_value(kp18058_out_data_t *p_out_data, u8 val, u8 lum)
{
	u16 hue_level = get_pwm_cmp(val, lum) >> 6;
	u8 val_high = (hue_level >> 5) << 1;
	u8 val_low = (hue_level & 0x1f) << 1;
	
	val_high |= kp18058_is_parity(val_high);
	val_low |= kp18058_is_parity(val_low);
	
	p_out_data->out_high = val_high;
	p_out_data->out_low = val_low;
}

/**
 * @brief       This function precess kp18058 set led
 * @param[in]   byte0_val	- select set led
 * @param[in]   val			- color value, range from 0-255
 * @param[in]   lum			- lightness or lumina, range from 0-100
 * @return      none
 * @note        
 */
void kp18058_light_adjust(u8 byte0_val, u8 val, u8 lum)
{
	kp18058_out_t kp_out = {0};

	kp_out.byte0 = byte0_val;
	kp18058_get_out_value(&kp_out.out_data, val, lum);
	
	i2c_write_series(0, 0, (u8 *)&kp_out, sizeof(kp_out));
}

/**
 * @brief       This function write rgbcw 10 byte
 * @param[in]   r_val	- red value, range from 0-255
 * @param[in]   g_val	- green value, range from 0-255
 * @param[in]   b_val	- blue value, range from 0-255
 * @param[in]   c_val	- c value, range from 0-255
 * @param[in]   w_val	- w value, range from 0-255
 * @param[in]   lum		- lightness or lumina, range from 0-100
 * @param[in]   output_cnt	- output count, start from r_val, max value is 5.
 * @return      none
 * @note        
 */
static void kp18058_light_adjust_rgbcw_driver(u8 r_val, u8 g_val, u8 b_val, u8 c_val, u8 w_val, u8 lum, int output_cnt)
{
	kp18058_out_all_t kp_out_all = {0};
	u8 rgbcw_data[ARRAY_SIZE(kp_out_all.out_data)] = {0};
	rgbcw_data[0] = b_val;
	rgbcw_data[1] = r_val;
	rgbcw_data[2] = g_val;
	rgbcw_data[3] = c_val;
	rgbcw_data[4] = w_val;

	kp_out_all.byte0 = KP18058_BLUE_ADDR; // start from OUT0

	if(output_cnt > ARRAY_SIZE(kp_out_all.out_data)){
		output_cnt = ARRAY_SIZE(kp_out_all.out_data);
	}
	
	foreach(i, output_cnt){
		kp18058_get_out_value(&kp_out_all.out_data[i], rgbcw_data[i], lum);
	}

	i2c_write_series(0, 0, (u8 *)&kp_out_all, output_cnt * 2 + OFFSETOF(kp18058_out_all_t, out_data));
}

/**
 * @brief       This function write rgbcw 10 byte
 * @param[in]   r_val	- red value, range from 0-255
 * @param[in]   g_val	- green value, range from 0-255
 * @param[in]   b_val	- blue value, range from 0-255
 * @param[in]   c_val	- c value, range from 0-255
 * @param[in]   w_val	- w value, range from 0-255
 * @param[in]   lum		- lightness or lumina, range from 0-100
 * @return      none
 * @note        
 */
void kp18058_light_adjust_rgbcw(u8 r_val, u8 g_val, u8 b_val, u8 c_val, u8 w_val, u8 lum)
{
	kp18058_light_adjust_rgbcw_driver(r_val, g_val, b_val, c_val, w_val, lum, 5);
}

/**
 * @brief       This function write rgb 6 byte
 * @param[in]   r_val	- red value, range from 0-255
 * @param[in]   g_val	- green value, range from 0-255
 * @param[in]   b_val	- blue value, range from 0-255
 * @param[in]   lum		- lightness or lumina, range from 0-100
 * @return      none
 * @note        
 */
void kp18058_light_adjust_rgb(u8 r_val, u8 g_val, u8 b_val, u8 lum)
{
	kp18058_light_adjust_rgbcw_driver(r_val, g_val, b_val, 0, 0, lum, 3);
}

/**
 * @brief       This function write cw 4 byte
 * @param[in]   c_val	- c value, range from 0-255
 * @param[in]   w_val	- w value, range from 0-255
 * @param[in]   lum		- lightness or lumina, range from 0-100
 * @return      none
 * @note        
 */
void kp18058_light_adjust_cw(u8 c_val, u8 w_val, u8 lum)
{
	kp18058_out_all_t kp_out_all = {0};

	kp_out_all.byte0 = KP18058_C_ADDR; // start from OUT3
	kp18058_get_out_value(&kp_out_all.out_data[0], c_val, lum);
	kp18058_get_out_value(&kp_out_all.out_data[1], w_val, lum);

	i2c_write_series(0, 0, (u8 *)&kp_out_all, 2 * sizeof(kp_out_all.out_data[0]) + OFFSETOF(kp18058_out_all_t, out_data));
}

/**
 * @brief       This function initialize kp18058
 * @return      none
 * @note        
 */
void kp18058_init_i2c(void)
{
	u8 kp18058_init_data[] = {0xe1, 0xa3, 0x81, 0x82}; // set kp18058 configuration, user can refer to datasheet to modify
	i2c_gpio_set(I2C_IO_SDA, I2C_IO_SCL);
	i2c_master_init(NULL, (unsigned char)(CLOCK_SYS_CLOCK_HZ/(4*I2C_CLK_SPEED))); 
	i2c_write_series(0, 0, kp18058_init_data, sizeof(kp18058_init_data));
}
#endif

