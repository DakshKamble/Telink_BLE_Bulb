/********************************************************************************************************
 * @file	log_module.h
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
#include <stdarg.h>
#include "tl_common.h"
#include "drivers/putchar.h"

#if WIN32
#pragma pack(1)
#endif

int tl_log_msg(u32 level_module,u8 *pbuf,int len,char  *format,...);
void tl_log_msg_err(u16 module,u8 *pbuf,int len,char *format,...);
void tl_log_msg_warn(u16 module,u8 *pbuf,int len,char  *format,...);
void tl_log_msg_info(u16 module,u8 *pbuf,int len,char  *format,...);
void tl_log_msg_dbg(u16 module,u8 *pbuf,int len,char  *format,...);
void user_log_info(u8 *pbuf,int len,char  *format,...);

int LogMsgModuleDlg_and_buf(u8 *pbuf,int len,char *log_str,char *format, va_list list);

#if 0// add nor base part ,we can demo from this part 
#define LOG_SRC_BEARER          (1 <<  0) /**< Receive logs from the bearer layer. */
#define LOG_SRC_NETWORK         (1 <<  1) /**< Receive logs from the network layer. */
#define LOG_SRC_TRANSPORT       (1 <<  2) /**< Receive logs from the transport layer. */
#define LOG_SRC_PROV            (1 <<  3) /**< Receive logs from the provisioning module. */
#define LOG_SRC_PACMAN          (1 <<  4) /**< Receive logs from the packet manager. */
#define LOG_SRC_INTERNAL        (1 <<  5) /**< Receive logs from the internal event module. */
#define LOG_SRC_API             (1 <<  6) /**< Receive logs from the Mesh API. */
#define LOG_SRC_DFU             (1 <<  7) /**< Receive logs from the DFU module. */
#define LOG_SRC_BEACON          (1 <<  8) /**< Receive logs from the beacon module. */
#define LOG_SRC_TEST            (1 <<  9) /**< Receive logs from unit tests. */
#define LOG_SRC_ENC             (1 << 10) /**< Receive logs from the encryption module. */
#define LOG_SRC_TIMER_SCHEDULER (1 << 11) /**< Receive logs from the timer scheduler. */
#define LOG_SRC_CCM             (1 << 12) /**< Receive logs from the CCM module. */
#define LOG_SRC_ACCESS          (1 << 13) /**< Receive logs from the access layer. */
#define LOG_SRC_APP             (1 << 14) /**< Receive logs from the application. */
#define LOG_SRC_SERIAL          (1 << 15) /**< Receive logs from the serial module. */

#define LOG_LEVEL_ASSERT ( 0) /**< Log level for assertions */
#define LOG_LEVEL_ERROR  ( 1) /**< Log level for error messages. */
#define LOG_LEVEL_WARN   ( 2) /**< Log level for warning messages. */
#define LOG_LEVEL_REPORT ( 3) /**< Log level for report messages. */
#define LOG_LEVEL_INFO   ( 4) /**< Log level for information messages. */
#define LOG_LEVEL_DBG1   ( 5) /**< Log level for debug messages (debug level 1). */
#define LOG_LEVEL_DBG2   ( 6) /**< Log level for debug messages (debug level 2). */
#define LOG_LEVEL_DBG3   ( 7) /**< Log level for debug messages (debug level 3). */
#define EVT_LEVEL_BASE   ( 8) /**< Base level for event logging. For internal use only. */
#define EVT_LEVEL_ERROR  ( 9) /**< Critical error event logging level. For internal use only. */
#define EVT_LEVEL_INFO   (10) /**< Normal event logging level. For internal use only. */
#define EVT_LEVEL_DATA   (11) /**< Event data logging level. For internal use only. */
#endif 

#define TL_LOG_LEVEL_DISABLE	  0
#define TL_LOG_LEVEL_USER         1U    // never use in library.
#define TL_LOG_LEVEL_LIB          2U    // it will not be optimized in library; some important log.
#define TL_LOG_LEVEL_ERROR        3U
#define TL_LOG_LEVEL_WARNING      4U
#define TL_LOG_LEVEL_INFO         5U
#define TL_LOG_LEVEL_DEBUG        6U
#define TL_LOG_LEVEL_MAX          TL_LOG_LEVEL_DEBUG

#define TL_LOG_LEVEL              TL_LOG_LEVEL_ERROR // TL_LOG_LEVEL_INFO	// Note firmware size

typedef enum{
	TL_LOG_PROTOCOL = 0,
	TL_LOG_PAIR ,
    TL_LOG_USER         ,	// never use in library.
	TL_LOG_MAX,
}printf_module_enum;

#define MAX_MODULE_STRING_CNT	16  // don't set too large to save firmware size
#define MAX_LEVEL_STRING_CNT	12  // don't set too large to save firmware size


#ifndef TL_LOG_SEL_VAL
#define MAX_STRCAT_BUF		(MAX_MODULE_STRING_CNT + MAX_LEVEL_STRING_CNT + 4)  //
	#if (1 && (0 == DEBUG_LOG_SETTING_DEVELOP_MODE_EN))
#define TL_LOG_SEL_VAL  (BIT(TL_LOG_USER))	// only user log as default
	#else
#define TL_LOG_SEL_VAL  (BIT(TL_LOG_USER)|BIT(TL_LOG_PROTOCOL)|BIT(TL_LOG_PAIR))
	#endif
#endif

#define LOG_FW_FUNC_EN      	(1)

#define LOG_GET_LEVEL_MODULE(level, module)     ((level << 5) | module) // use 8bit to decrease firmware size
#define LOG_GET_MODULE(level_module)            (level_module & 0x1F)
#define LOG_GET_LEVEL(level_module)             ((level_module >> 5) & 0x07)

#define LOG_MSG_FUNC_EN(log_type_err, module)	(LOG_FW_FUNC_EN && (log_type_err || (BIT(module) & TL_LOG_SEL_VAL)))

#if (LOG_FW_FUNC_EN && HCI_LOG_FW_EN && (TL_LOG_LEVEL >= TL_LOG_LEVEL_USER))
#define LOG_USER_MSG_INFO(pbuf,len,format,...)  do{int val; val = (LOG_MSG_FUNC_EN(0,TL_LOG_USER) && tl_log_msg(LOG_GET_LEVEL_MODULE(TL_LOG_LEVEL_USER,TL_LOG_USER),pbuf,len,format,##__VA_ARGS__));}while(0)
#else
#define LOG_USER_MSG_INFO(pbuf,len,format,...)
#endif

#if (TL_LOG_LEVEL >= TL_LOG_LEVEL_LIB)
#define LOG_MSG_LIB(module,pbuf,len,format,...)  do{int val; val = (LOG_MSG_FUNC_EN(0,module) && tl_log_msg(LOG_GET_LEVEL_MODULE(TL_LOG_LEVEL_LIB,module),pbuf,len,format,##__VA_ARGS__));}while(0)
#else
#define LOG_MSG_LIB(module,pbuf,len,format,...) 
#endif 

#if (TL_LOG_LEVEL >= TL_LOG_LEVEL_ERROR)
#define LOG_MSG_ERR(module,pbuf,len,format,...)  do{int val; val = (LOG_MSG_FUNC_EN(1,module) && tl_log_msg(LOG_GET_LEVEL_MODULE(TL_LOG_LEVEL_ERROR,module),pbuf,len,format,##__VA_ARGS__));}while(0)
#else
#define LOG_MSG_ERR(module,pbuf,len,format,...) 
#endif 

#if (TL_LOG_LEVEL >= TL_LOG_LEVEL_WARNING)
#define LOG_MSG_WARN(module,pbuf,len,format,...) do{int val; val = (LOG_MSG_FUNC_EN(0,module) && tl_log_msg(LOG_GET_LEVEL_MODULE(TL_LOG_LEVEL_WARNING,module),pbuf,len,format,##__VA_ARGS__));}while(0)
#else
#define LOG_MSG_WARN(module,pbuf,len,format,...) 
#endif 

#if (TL_LOG_LEVEL >= TL_LOG_LEVEL_INFO)
#define LOG_MSG_INFO(module,pbuf,len,format,...) do{int val; val = (LOG_MSG_FUNC_EN(0,module) && tl_log_msg(LOG_GET_LEVEL_MODULE(TL_LOG_LEVEL_INFO,module),pbuf,len,format,##__VA_ARGS__));}while(0)
#else
#define LOG_MSG_INFO(module,pbuf,len,format,...) 
#endif

#if (TL_LOG_LEVEL >= TL_LOG_LEVEL_DEBUG)
#define LOG_MSG_DBG(module,pbuf,len,format,...)	do{int val; val = (LOG_MSG_FUNC_EN(0,module) && tl_log_msg(LOG_GET_LEVEL_MODULE(TL_LOG_LEVEL_DEBUG,module),pbuf,len,format,##__VA_ARGS__));}while(0)
#else
#define LOG_MSG_DBG(module,pbuf,len,format,...) 
#endif 

#define LOG_MSG_FUNC_NAME()     do{LOG_MSG_LIB (TL_LOG_PROTOCOL, 0, 0, "%s", __FUNCTION__);}while(0)



