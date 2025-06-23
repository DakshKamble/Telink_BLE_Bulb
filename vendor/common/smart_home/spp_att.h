/********************************************************************************************************
 * @file	spp_att.h
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
//#include <stack/ble/ble.h>
#include "protocol.h"

int smart_home_GATT_rx_command(void *p);
int smart_home_GATT_rx_pair(void *p);


#if 1 // SPP_ATT_LIST_INCLUDE_EN
static u8 TelinkSppServiceUUID[16]	      = TELINK_SPP_UUID_SERVICE;
static u8 TelinkSppDataServer2ClientUUID[16]    = TELINK_SPP_DATA_SERVER2CLIENT;
static u8 TelinkSppDataClient2ServerUUID[16]    = TELINK_SPP_DATA_CLIENT2SERVER;
static u8 TelinkSppDataPairUUID[16]             = TELINK_SPP_DATA_PAIR;

// Spp data from Server to Client characteristic variables
static u8 SppDataServer2ClientProp = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
static u8 SppDataServer2ClientData[ATT_MTU_SIZE - 3];
static u8 SppDataServer2ClientDataCCC[2] = {0};

// Spp data from Client to Server characteristic variables
//CHAR_PROP_WRITE: Need response from slave, low transmission speed
static u8 SppDataClient2ServerProp = CHAR_PROP_READ | CHAR_PROP_WRITE_WITHOUT_RSP; //CHAR_PROP_WRITE;
static u8 SppDataClient2ServerData[ATT_MTU_SIZE - 3];

// Spp data Pair characteristic variables
static u8 SppDataPairProp = CHAR_PROP_WRITE_WITHOUT_RSP | CHAR_PROP_NOTIFY;
static u8 SppDataPairData[ATT_MTU_SIZE - 3];
static u8 SppDataPairDataCCC[2] = {0};

//SPP data descriptor
static const u8 TelinkSPPS2CDescriptor[] = "Telink SPP: Module->Phone";
static const u8 TelinkSPPC2SDescriptor[] = "Telink SPP: Phone->Module";
static const u8 TelinkSPPPairDescriptor[] = "Telink SPP: Pair";


#define SPP_ATT_LIST		\
{12,ATT_PERMISSIONS_READ,2,16,(u8*)(&my_primaryServiceUUID), 	(u8*)(&TelinkSppServiceUUID), 0},\
\
{0,ATT_PERMISSIONS_READ,2,1,(u8*)(&my_characterUUID),		(u8*)(&SppDataServer2ClientProp), 0},			/*prop*/	\
{0,ATT_PERMISSIONS_READ,16,sizeof(SppDataServer2ClientData),(u8*)(&TelinkSppDataServer2ClientUUID), (u8*)(SppDataServer2ClientData), 0},	/*value*/	\
{0,ATT_PERMISSIONS_RDWR,2,2,(u8*)&clientCharacterCfgUUID,(u8*)(&SppDataServer2ClientDataCCC)},\
{0,ATT_PERMISSIONS_READ,2,sizeof(TelinkSPPS2CDescriptor),(u8*)&userdesc_UUID,(u8*)(&TelinkSPPS2CDescriptor)},\
\
{0,ATT_PERMISSIONS_READ,2,1,(u8*)(&my_characterUUID),		(u8*)(&SppDataClient2ServerProp), 0},				/*prop*/	\
{0,ATT_PERMISSIONS_RDWR,16,sizeof(SppDataClient2ServerData),(u8*)(&TelinkSppDataClient2ServerUUID), (u8*)(SppDataClient2ServerData), &smart_home_GATT_rx_command}, /*value*/	\
{0,ATT_PERMISSIONS_READ,2,sizeof(TelinkSPPC2SDescriptor),(u8*)&userdesc_UUID,(u8*)(&TelinkSPPC2SDescriptor)},\
\
{0,ATT_PERMISSIONS_READ,2,1,(u8*)(&my_characterUUID),		(u8*)(&SppDataPairProp), 0},			/*prop*/	\
{0,ATT_PERMISSIONS_RDWR,16,sizeof(SppDataPairData),(u8*)(&TelinkSppDataPairUUID), (u8*)(SppDataPairData), &smart_home_GATT_rx_pair},	/*value*/	\
{0,ATT_PERMISSIONS_RDWR,2,2,(u8*)&clientCharacterCfgUUID,(u8*)(&SppDataPairDataCCC)},\
{0,ATT_PERMISSIONS_READ,2,sizeof(TelinkSPPPairDescriptor),(u8*)&userdesc_UUID,(u8*)(&TelinkSPPPairDescriptor)},

#endif

