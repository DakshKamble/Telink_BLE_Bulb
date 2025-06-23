/********************************************************************************************************
 * @file	whitelist.c
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
#include "drivers.h"
#include "stack/ble/controller/ble_controller.h"
#include "whitelist_stack.h"

#if (0 == APP_SECURITY_ENABLE) // when no smp, replace whitelist.c in library to save code size.
//_attribute_data_retention_	_attribute_aligned_(4)	ll_whiteListTbl_t		ll_whiteList_tbl;
//_attribute_data_retention_	_attribute_aligned_(4)	ll_ResolvingListTbl_t	ll_resolvingList_tbl;


_attribute_data_retention_	ll_wl_handler_t			ll_whiteList_handler = NULL;



/*
Sample data:
#if (IRK_REVERT_TO_SAVE_AES_TMIE_ENABLE)  //revert irk
u8 test_irk[16]  = {0xa0, 0x2a, 0xf5, 0x82, 0x51, 0x50, 0x98, 0x57, 0x0c, 0x69, 0x88, 0xf6, 0x3d, 0x57, 0x4a, 0x71};
#else
u8 test_irk[16]  = {0x71, 0x4a ,0x57 ,0x3d,  0xf6 ,0x88, 0x69 ,0x0c,  0x57, 0x98, 0x50, 0x51, 0x82 ,0xf5 ,0x2a, 0xa0};
#endif
u8 test_mac[6]  = {0x3b, 0x3f, 0xfb, 0xeb, 0x1e, 0x78};

u8 result = smp_quickResolvPrivateAddr(test_irk, test_mac);

 */

bool smp_quickResolvPrivateAddr(u8 *key, u8 *addr) {
	return 0;
}

bool smp_resolvPrivateAddr(u8 *key, u8 *addr) {
	return 0;
}

bool aes_resolve_irk_rpa(u8 *key, u8 *addr) { // rename from smp_resolvPrivateAddr at V3.4.1.3 ?
	return 0;
}


/**
 * @brief      reset whitelist
 * @param[in]  none
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	ll_whiteList_reset(void)
{
	return BLE_SUCCESS;
}



/**
 * @brief      add a device form whitelist
 * @param[in]  type - device mac address type
 * @param[in]  addr - device mac address
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t ll_whiteList_add(u8 type, u8 *addr)
{
	return HCI_ERR_MEM_CAP_EXCEEDED; //LL_ERR_WHITE_LIST_PUBLIC_ADDR_TABLE_FULL;
}



u8 * ll_searchAddrInWhiteListTbl(u8 type, u8 *addr)
{
	return NULL;
}



/**
 * @brief      delete a device from whitelist
 * @param[in]  type - device mac address type
 * @param[in]  addr - device mac address
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t  ll_whiteList_delete(u8 type, u8 *addr)
{
	return BLE_SUCCESS;
}





/**
 * @brief      get whitelist size
 * @param[out] pointer to size
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t ll_whiteList_getSize(u8 *returnPublicAddrListSize) {
	*returnPublicAddrListSize = 0;

	return BLE_SUCCESS;
}


/**
 * @brief      delete a device from resolvinglist
 * @param[in]  peerIdAddrType - device mac address type
 * @param[in]  peerIdAddr - device mac address
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t  ll_resolvingList_delete(u8 peerIdAddrType, u8 *peerIdAddr)
{
	//return LL_ERR_ADDR_NOT_EXIST_IN_WHITE_LIST;
	return BLE_SUCCESS;  //ͬwhite listһɾһlist豸success
}


/**
 * @brief      add a device to resolvinglist
 * @param[in]  peerIdAddrType - device mac address type
 * @param[in]  peerIdAddr - device mac address
 * @param[in]  peer_irk - peer IRK pointer
 * @param[in]  local_irk - local IRK pointer
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t  ll_resolvingList_add(u8 peerIdAddrType, u8 *peerIdAddr, u8 *peer_irk, u8 *local_irk)
{
	return HCI_ERR_MEM_CAP_EXCEEDED;
}




u8 * ll_searchAddrInResolvingListTbl(u8 *addr)
{
	return NULL;
}


/**
 * @brief      reset resolvinglist
 * @param[in]  none
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	ll_resolvingList_reset(void)
{
	return BLE_SUCCESS;
}

ble_sts_t ll_resolvingList_getSize(u8 *Size)
{
	*Size = 0;
	return BLE_SUCCESS;
}



ble_sts_t ll_resolvingList_getPeerResolvableAddr(u8 peerIdAddrType, u8* peerIdAddr, u8* peerResolvableAddr)
{
	return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t ll_resolvingList_getLocalResolvableAddr(u8 peerIdAddrType, u8* peerIdAddr, u8* LocalResolvableAddr)
{
	return BLE_SUCCESS;
}


/**
 * @brief      enable resolution of Resolvable Private Addresses in the Controller
 * @param[in]  resolutionEn - 1: enable; 0:disable
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t ll_resolvingList_setAddrResolutionEnable (u8 resolutionEn)
{
	return BLE_SUCCESS;
}


/*
 * @brief 	API to set the length of time the controller uses a Resolvable Private Address
 * 			before a new resolvable	private address is generated and starts being used.
 * 			This timeout applies to all addresses generated by the controller
 *
 * */
ble_sts_t  ll_resolvingList_setResolvablePrivateAddrTimer (u16 timeout_s)
{
	return BLE_SUCCESS;
}




/*********************************************************************
 * @fn      ll_searchAddr_in_WhiteList_and_ResolvingList
 *
 * @brief   API to search  list entry through specified address
 *
 * @param   type - Specified BLE address type
 *                addr - Specified BLE address
 *                table - Table to be searched
 *                tblSize - Table size
 *
 * @return  Pointer to the found entry. NULL means not found
 */
u8 * ll_searchAddr_in_WhiteList_and_ResolvingList(u8 type, u8 *addr)
{
	return NULL;
}
#endif

