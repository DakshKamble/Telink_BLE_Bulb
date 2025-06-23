/********************************************************************************************************
 * @file	putchar.h
 *
 * @brief	This is the header file for TLSR8233
 *
 * @author	junwei.lu
 * @date	May 8, 2018
 *
 * @par     Copyright (c) 2018, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

//#include "driver_config.h"
#include "common/config/user_config.h"

#ifndef WIN32

#ifndef SIMULATE_UART_EN
	#define SIMULATE_UART_EN          0
#endif

#ifndef DEBUG_TX_PIN
	#define DEBUG_TX_PIN        GPIO_PD3
#endif

#ifndef DEBUG_BAUDRATE
	#define DEBUG_BAUDRATE      (115200)
#endif

int putchar(int c);

#endif


