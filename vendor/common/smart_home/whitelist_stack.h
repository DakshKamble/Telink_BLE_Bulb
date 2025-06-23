/********************************************************************************************************
 * @file	whitelist_stack.h
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
#ifndef WHITELIST_STACK_H_
#define WHITELIST_STACK_H_



#define  	MAX_WHITE_LIST_SIZE    				4
#define 	MAX_WHITE_IRK_LIST_SIZE          	2   //save ramcode



#define 	IRK_REVERT_TO_SAVE_AES_TMIE_ENABLE		1	//to save time, Logitech has deleted it to prevent users ask.




typedef struct {
	u8 type;
	u8 address[BLE_ADDR_LEN];
	u8 reserved;
} wl_addr_t;

typedef struct {
	wl_addr_t  wl_addr_tbl[MAX_WHITE_LIST_SIZE];
	u8 	wl_addr_tbl_index;
	u8 	wl_irk_tbl_index;
} ll_whiteListTbl_t;


typedef struct {
	u8 type;
	u8 address[6]; //BLE_ADDR_LEN
	u8 reserved;
	u8 irk[16];
} rl_addr_t;

typedef struct {
	rl_addr_t	tbl[MAX_WHITE_IRK_LIST_SIZE];
	u8 			idx;
	u8			en;
} ll_ResolvingListTbl_t;


typedef u8 * (*ll_wl_handler_t)(u8 , u8 *);
extern ll_wl_handler_t			ll_whiteList_handler;



u8 * ll_searchAddrInWhiteListTbl(u8 type, u8 *addr);
u8 * ll_searchAddr_in_WhiteList_and_ResolvingList(u8 type, u8 *addr);



ble_sts_t  ll_resolvingList_getSize(u8 *Size);

ble_sts_t  ll_resolvingList_getPeerResolvableAddr (u8 peerIdAddrType, u8* peerIdAddr, u8* peerResolvableAddr); //not available now
ble_sts_t  ll_resolvingList_getLocalResolvableAddr(u8 peerIdAddrType, u8* peerIdAddr, u8* LocalResolvableAddr); //not available now
ble_sts_t  ll_resolvingList_setResolvablePrivateAddrTimer (u16 timeout_s);   //not available now

bool aes_resolve_irk_rpa(u8 *key, u8 *addr);






#endif /* WHITELIST_STACK_H_ */
