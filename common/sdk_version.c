/********************************************************************************************************
 * @file	sdk_version.c
 *
 * @brief	This is the header file for BLE SDK
 *
 * @author	BLE GROUP
 * @date	2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "sdk_version.h"
/*
 * Release Tool need to change this macro to match the release version,
 * the replace rules is: "$$$B85m_driver_sdk_"#sdk_version_num"$$$", The "#sdk_version_num"
 * will replace with this macro value.
 */
#define SDK_VER_MAJOR			(1)		// SDK version, don't change!!! 
#define SDK_VER_MINOR			(0)		// SDK version, don't change!!! 

#define VER_NUM2CHAR(num)		((((num) >= 0)&&((num) <= 9)) ? ((num) + '0') : ((((num) >= 0x0a)&&((num) <= 0x0f)) ? ((num)-0x0a + 'a') : (num)))
#define SDK_VER_FLAG			'$','$','$'
#define SDK_VER_NAME			't','e','l','i','n','k','_','b','8','0','_','l','i','g','h','t','i','n','g','_','s','d','k'
#define SDK_VER_CHAR			'V',VER_NUM2CHAR(SDK_VER_MAJOR),'.',VER_NUM2CHAR(SDK_VER_MINOR)


volatile __attribute__((section(".sdk_version"))) const unsigned char const_sdk_version[] = {SDK_VER_FLAG,SDK_VER_NAME,'_',SDK_VER_CHAR, SDK_VER_FLAG}; // can not extern, due to no loading in cstartup.
//volatile __attribute__((section(".sdk_version"))) unsigned char sdk_version[] = {SDK_VERSION(SDK_VERSION_NUM)};


