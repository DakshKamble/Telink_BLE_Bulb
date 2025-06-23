/********************************************************************************************************
 * @file	version_type.h
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


#if WIN32
#pragma pack(1)
#endif

#define SH_LITTLE_END_VID(major, minor, fix)	(fix + (minor << 4) + (major << 8))	// little endianness for software.
#define SH_BIG_END_VID(major, minor, fix)		(major + (minor << 12) + (fix << 8))	// big endianness only at the fixed place of firmware. other places use little endianness.


// ------------------------------- PID ---------------------------------------
#define PID_UNKNOW              (0x0000)
// ------ light ------
#define PID_SH_LIGHT_CT_RGB   	(0x0001)
// ------ switch (RCU) ------
#define PID_SH_SWITCH          	(0x0101)

// smart home light product ID. filled in "sh_pair_confirm_t"
#if (WIN32 || __PROJECT_B80_BLE_LIGHT__)
#define SH_PID					PID_SH_LIGHT_CT_RGB
#elif (__PROJECT_B80_BLE_SWITCH__)
#define SH_PID					PID_SH_SWITCH
#else
#define SH_PID					PID_UNKNOW
#endif


// smart home light soft version ID : filled in "sh_pair_confirm_t"
#define SW_VERSION_MAJOR		(1)		//
#define SW_VERSION_MINOR		(0)		// 
#define SW_VERSION_FIX			(3)		// 
#define SH_VID					SH_LITTLE_END_VID(SW_VERSION_MAJOR, SW_VERSION_MINOR, SW_VERSION_FIX)


#define BUILD_VER				(SH_PID + (SH_BIG_END_VID(SW_VERSION_MAJOR, SW_VERSION_MINOR, SW_VERSION_FIX) << 16))	// only big endianness of Version ID used at the fixed place of firmware. other cases use little endianness.

