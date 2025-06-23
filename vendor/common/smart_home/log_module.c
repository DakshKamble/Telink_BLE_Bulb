/********************************************************************************************************
 * @file	log_module.c
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
//#include "stack/ble/controller/ll/ll.h"
//#include "stack/ble/blt_config.h"
#include "stack/ble/service/ota/ota.h"
#include "drivers/watchdog.h"
#include "protocol.h"
#include "log_module.h"
#include "common/printf.h"

/** @addtogroup Mesh_Common
  * @{
  */

const char  TL_LOG_STRING[TL_LOG_LEVEL_MAX][MAX_LEVEL_STRING_CNT] = {
    {"[USER]:"},
    {"[LIB]:"},    // library and important log
    {"[ERR]:"},
    {"[WARN]:"},
    {"[INFO]:"},
    {"[DEBUG]:"},
};

const char tl_log_module_mesh[TL_LOG_MAX][MAX_MODULE_STRING_CNT] ={
	"(PROTOCOL)","(PAIR)","(USER)",
};

STATIC_ASSERT(TL_LOG_LEVEL_MAX < 8); // because only 3bit, please refer to LOG_GET_LEVEL_MODULE
STATIC_ASSERT(TL_LOG_MAX < 32); // because 32bit, and LOG_GET_LEVEL_MODULE


_PRINT_FUN_RAMCODE_ int tl_log_msg_valid(char *pstr,u32 len_max,u32 module)
{
	int ret =1;
	if (module >= TL_LOG_MAX){
		ret =0;
	}else if (!(BIT(module)& TL_LOG_SEL_VAL)){
		ret =0;
	}
	if(ret){
		#if WIN32
			#if VC_APP_ENABLE
			strcat_s(pstr,len_max,tl_log_module_mesh[module]);
			#else 
			strcat(pstr,tl_log_module_mesh[module]);
			#endif 
		#else
		    u32 str_len = strlen(pstr);
		    str_len = min(str_len, MAX_LEVEL_STRING_CNT);
		    if((str_len + MAX_MODULE_STRING_CNT) <= len_max ){
			    memcpy(pstr+str_len,tl_log_module_mesh[module],MAX_MODULE_STRING_CNT);
			}
		#endif
	}
	return ret;
}


_PRINT_FUN_RAMCODE_ int tl_log_msg(u32 level_module,u8 *pbuf,int len,char  *format,...)
{
#if (WIN32 || HCI_LOG_FW_EN)
	char tl_log_str[MAX_STRCAT_BUF] = {0};
	u32 module = LOG_GET_MODULE(level_module);
	u32 log_level = LOG_GET_LEVEL(level_module);
	
	if((0 == log_level) || (log_level > TL_LOG_LEVEL_MAX)){
	    return -1;
	}else{
        memcpy(tl_log_str,TL_LOG_STRING[log_level - 1],MAX_LEVEL_STRING_CNT);
	}
	
	if(!tl_log_msg_valid(tl_log_str,sizeof(tl_log_str), module)){
	    if(log_level != TL_LOG_LEVEL_ERROR){
		    return -1;
		}
	}
	
	va_list list;
	va_start( list, format );
	LogMsgModuleDlg_and_buf((u8 *)pbuf,len,tl_log_str,format,list);	
#endif

    return 0;
}


#if 0
void tl_log_msg_err(u16 module,u8 *pbuf,int len,char  *format,...)
{
#if (WIN32 || HCI_LOG_FW_EN)
	char tl_log_str[MAX_STRCAT_BUF] = TL_LOG_ERROR_STRING;
	if(!tl_log_msg_valid(tl_log_str,sizeof(tl_log_str), module)){
		//return ;
	}
	va_list list;
	va_start( list, format );
	LogMsgModuleDlg_and_buf(pbuf,len,tl_log_str,format,list);	
#endif
}

void tl_log_msg_warn(u16 module,u8 *pbuf,int len,char  *format,...)
{
#if (WIN32 || HCI_LOG_FW_EN)
	char tl_log_str[MAX_STRCAT_BUF] = TL_LOG_WARNING_STRING;
	if(!tl_log_msg_valid(tl_log_str,sizeof(tl_log_str),module)){
		return ;
	}
	va_list list;
	va_start( list, format );
	LogMsgModuleDlg_and_buf(pbuf,len,tl_log_str,format,list);	
#endif
}

void tl_log_msg_info(u16 module,u8 *pbuf,int len,char  *format,...)
{
#if (WIN32 || HCI_LOG_FW_EN)
	char tl_log_str[MAX_STRCAT_BUF] = TL_LOG_INFO_STRING;
	if(!tl_log_msg_valid(tl_log_str,sizeof(tl_log_str),module)){
		return ;
	}
	va_list list;
	va_start( list, format );
	LogMsgModuleDlg_and_buf(pbuf,len,tl_log_str,format,list);	
#endif
}

void user_log_info(u8 *pbuf,int len,char  *format,...)
{
    char tl_log_str[MAX_STRCAT_BUF] = TL_LOG_INFO_STRING;
	if(!tl_log_msg_valid(tl_log_str,sizeof(tl_log_str),TL_LOG_USER)){
		return ;
	}
	va_list list;
	va_start( list, format );
	LogMsgModuleDlg_and_buf(pbuf,len,tl_log_str,format,list);	
}

void tl_log_msg_dbg(u16 module,u8 *pbuf,int len,char  *format,...)
{
#if (WIN32 || HCI_LOG_FW_EN)
	char tl_log_str[MAX_STRCAT_BUF] = TL_LOG_DEBUG_STRING;
	if(!tl_log_msg_valid(tl_log_str,sizeof(tl_log_str),module)){
		return ;
	}
	va_list list;
	va_start( list, format );
	LogMsgModuleDlg_and_buf(pbuf,len,tl_log_str,format,list);	
#endif
}
#endif

#if !WIN32
#if HCI_LOG_FW_EN
/*_attribute_no_retention_bss_*/ char log_dst[256];// make sure enough RAM
u32 LOG_LEN_MAX = sizeof(log_dst);
#if 0
int LogMsgModdule_uart_mode(u8 *pbuf,int len,char *log_str,char *format, va_list list)
{
    #if (GATEWAY_ENABLE)
	return 1;
	#endif
	#if (HCI_ACCESS == HCI_USE_UART)    
	char *p_buf;
	char **pp_buf;
	p_buf = log_dst+2;
	pp_buf = &(p_buf);

	memset(log_dst, 0, sizeof(log_dst));
	u32 head_len = print(pp_buf,log_str, 0)+print(pp_buf,format,list);   // log_dst[] is enough ram.
	head_len += 2;  // sizeof(log_dst[0]) + sizeof(log_dst[1]) 
	if(head_len > sizeof(log_dst)){
	    while(1){   // assert, RAM conflict, 
	        show_ota_result(OTA_DATA_CRC_ERR);
	    }
	}
	
	if(head_len > HCI_TX_FIFO_SIZE_USABLE){
		return 0;
	}

	log_dst[0] = HCI_LOG; // type
	log_dst[1] = head_len;
	u8 data_len_max = HCI_TX_FIFO_SIZE_USABLE - head_len;
	if(len > data_len_max){
		len = data_len_max;
	}
	memcpy(log_dst+head_len, pbuf, len);
	my_fifo_push_hci_tx_fifo((u8 *)log_dst, head_len+len, 0, 0);
	if(is_lpn_support_and_en){
		blc_hci_tx_to_uart ();
			#if PTS_TEST_EN
		while(uart_tx_is_busy ());
			#endif
	}
	#endif
    return 1;
}
#endif

_PRINT_FUN_RAMCODE_ int LogMsgModule_io_simu(u8 *pbuf,int len,char *log_str,char *format, va_list list)
{
	char *p_buf;
	char **pp_buf;
	p_buf = log_dst;
	pp_buf = &(p_buf);
	u32 head_len = print(pp_buf,log_str, 0)+print(pp_buf,format,list);   // log_dst[] is enough ram.
	if((head_len + get_len_Bin2Text(len))> sizeof(log_dst)){
        // no need, have been check buf max in printf_Bin2Text. // return 0;
	}
	u32 dump_len = printf_Bin2Text((char *)(log_dst+head_len), sizeof(log_dst) - head_len, (char *)(pbuf), len);
	uart_simu_send_bytes((u8 *)log_dst, head_len+dump_len);
	return 1;
}
#endif


_PRINT_FUN_RAMCODE_ int LogMsgModuleDlg_and_buf(u8 *pbuf,int len,char *log_str,char *format, va_list list)
{
    #if (0 == HCI_LOG_FW_EN)
    return 1;
    #else
        #if 1
    return LogMsgModule_io_simu(pbuf,len,log_str,format,list);
        #else
    return LogMsgModdule_uart_mode(pbuf,len,log_str,format,list);
        #endif
    #endif
}
#endif	


// -------------- command string -------------------
#if LOG_FW_FUNC_EN
typedef struct{
    u8 op;
    char *op_string;	// should be at last
}smart_home_op_t;
#define OP_WITH_STR(op)		{op, #op}

const smart_home_op_t smart_home_op_group[] = {
	OP_WITH_STR(LIGHT_CMD_RESERVE),
	OP_WITH_STR(LIGHT_CMD_ONOFF_SET),
	OP_WITH_STR(LIGHT_CMD_ONOFF_STATUS),
	OP_WITH_STR(LIGHT_CMD_LIGHTNESS_SET),
	OP_WITH_STR(LIGHT_CMD_LIGHTNESS_STATUS),
	OP_WITH_STR(LIGHT_CMD_LIGHTNESS_CT_SET),
	OP_WITH_STR(LIGHT_CMD_LIGHTNESS_CT_STATUS),
	OP_WITH_STR(LIGHT_CMD_LIGHTNESS_RGB_SET),
	OP_WITH_STR(LIGHT_CMD_LIGHTNESS_RGB_STATUS),
	OP_WITH_STR(LIGHT_CMD_GROUP_GET_ADD_DEL),
	OP_WITH_STR(LIGHT_CMD_GROUP_STATUS),
	OP_WITH_STR(LIGHT_CMD_GROUP_ID_LIST_GET),
	OP_WITH_STR(LIGHT_CMD_GROUP_ID_LIST),
	OP_WITH_STR(LIGHT_CMD_SCENE_GET_SET),
	OP_WITH_STR(LIGHT_CMD_SCENE_CALL),
	OP_WITH_STR(LIGHT_CMD_SCENE_STATUS),
	OP_WITH_STR(LIGHT_CMD_SCENE_ADD_NO_PAR),
	OP_WITH_STR(LIGHT_CMD_SCENE_ADD_NO_PAR_ST),
	OP_WITH_STR(LIGHT_CMD_SCENE_ID_LIST_GET),
	OP_WITH_STR(LIGHT_CMD_SCENE_ID_LIST),
	OP_WITH_STR(LIGHT_CMD_TIME_SET_ACK),
	OP_WITH_STR(LIGHT_CMD_TIME_SET_NO_ACK),
	OP_WITH_STR(LIGHT_CMD_TIME_GET),
	OP_WITH_STR(LIGHT_CMD_TIME_STATUS),
	OP_WITH_STR(LIGHT_CMD_DEL_NODE),
	OP_WITH_STR(LIGHT_CMD_LIGHT_ONLINE_STATUS),
	OP_WITH_STR(LIGHT_CMD_SWITCH_CONFIG),
	OP_WITH_STR(LIGHT_CMD_SWITCH_CONFIG_STATUS),
	OP_WITH_STR(LIGHT_CMD_CPS_GET),
	OP_WITH_STR(LIGHT_CMD_SCHEDULE_ACTION_SET),
	OP_WITH_STR(LIGHT_CMD_SCHEDULE_ACTION_GET),
	OP_WITH_STR(LIGHT_CMD_SCHEDULE_ACTION_RSP),
	OP_WITH_STR(LIGHT_CMD_SCHEDULE_LIST_GET),
	OP_WITH_STR(LIGHT_CMD_SCHEDULE_LIST_STATUS),
};

static const char OP_STRING_NOT_FOUND[] = {"?"};
static const char * get_op_string_aligned_sh(const char *p_str_ret)
{
	static char s_op_string_buff[40] = {0};
	#define OP_STRING_BUFF_LEN_MAX		(sizeof(s_op_string_buff) - 1)
	#define OP_STRING_BUFF_LEN_ALIGN2	(20) // for "LIGHTNESS_STATUS"
	if(p_str_ret){
		int str_len_log = min(strlen(p_str_ret), OP_STRING_BUFF_LEN_MAX);
		memcpy(s_op_string_buff, p_str_ret, str_len_log);
		
		if((OP_STRING_BUFF_LEN_MAX >= OP_STRING_BUFF_LEN_ALIGN2) && (OP_STRING_BUFF_LEN_ALIGN2 > str_len_log)){
			memset(s_op_string_buff + str_len_log, ' ', OP_STRING_BUFF_LEN_ALIGN2 - str_len_log);
			s_op_string_buff[OP_STRING_BUFF_LEN_ALIGN2] = '\0';
		}else{
			s_op_string_buff[OP_STRING_BUFF_LEN_MAX] = '\0';
		}
		p_str_ret = s_op_string_buff;
	}

	return p_str_ret;
}

const char * get_op_string_sh(u8 op)
{
	const char *p_str_ret = 0;
	foreach_arr(i, smart_home_op_group){
		const smart_home_op_t *p = &smart_home_op_group[i];
		if(p->op == op){
			p_str_ret = p->op_string;
			break;
		}
	}

	#if 1 // WIN32	// aligned
	if(p_str_ret){
		p_str_ret = get_op_string_aligned_sh(p_str_ret + 10);	// offset of "LIGHT_CMD_"
	}
	#endif
	
	return p_str_ret ? p_str_ret : OP_STRING_NOT_FOUND;
}

typedef struct{
    u8 err;
    char *op_string;
}smart_home_err_number_t;
#define ERR_WITH_STR(err)		{err, #err}

const smart_home_err_number_t smart_home_err_number_group[] = { // for TX and RX
	// ---------- rx error number -------------
	ERR_WITH_STR(RX_CB_ERR_DATA_TYPE),
	ERR_WITH_STR(RX_CB_ERR_PAR_LEN),
	ERR_WITH_STR(RX_CB_ERR_DECRYPTION_NO_KEY_FOUND),
	ERR_WITH_STR(RX_CB_ERR_DECRYPTION_FAILED),
	ERR_WITH_STR(RX_CB_ERR_SNO_REPLAY),
	ERR_WITH_STR(RX_CB_ERR_ADDR_SRC_SELF),
	ERR_WITH_STR(RX_CB_ERR_ADDR_DST_NOT_MATCH),
	ERR_WITH_STR(RX_CB_ERR_NW_LEN),
	ERR_WITH_STR(RX_CB_ERR_PROXY_INI_TX),
	ERR_WITH_STR(RX_CB_ERR_OUTOF_MEMORY),
	ERR_WITH_STR(RX_CB_ERR_HAVE_BEEN_PAIRED),
	ERR_WITH_STR(RX_CB_ERR_NOT_PAIR_TIME_WINDOW),
	ERR_WITH_STR(RX_CB_ERR_PAIR_DEF_KEY_BUT_NOT_REQ),
	ERR_WITH_STR(RX_CB_ERR_PAIR_ERR_REQ),
	ERR_WITH_STR(RX_CB_ERR_GROUP_ID_INVALID),
	ERR_WITH_STR(RX_CB_ERR_GROUP_FULL),
	ERR_WITH_STR(RX_CB_ERR_GROUP_ID_NOT_FOUND),
	ERR_WITH_STR(RX_CB_ERR_SCENE_ID_INVALID),
	ERR_WITH_STR(RX_CB_ERR_SCENE_ID_FULL),
	ERR_WITH_STR(RX_CB_ERR_SCENE_ID_NOT_FOUND),
	ERR_WITH_STR(RX_CB_ERR_TX_RSP_FAILED),
	ERR_WITH_STR(RX_CB_ERR_TIME_INVALID),

	// ---------- tx error number -------------
	ERR_WITH_STR(TX_CMD_ERR_PAR_LEN),
	ERR_WITH_STR(TX_CMD_ERR_KEY_INVALID),
	ERR_WITH_STR(TX_CMD_ERR_INVALID_SW_GROUP_ID),
	ERR_WITH_STR(TX_CMD_ERR_FIFO_FULL),
};

const char * get_err_number_string_sh(int err)
{
	const char *p_str_ret = 0;
	foreach_arr(i, smart_home_err_number_group){
		const smart_home_err_number_t *p = &smart_home_err_number_group[i];
		if(p->err == err){
			p_str_ret = p->op_string;
			break;
		}
	}
	
	return p_str_ret ? p_str_ret : OP_STRING_NOT_FOUND;
}

typedef struct{
    char *op_string;
}smart_home_err_st_string_t;
#define ERR_WITH_ONLY_STR(err)		{#err}

const smart_home_err_st_string_t smart_home_err_st_scheduler_status[] = {
	ERR_WITH_ONLY_STR(SCHEDULER_ST_SUCCESS),
	ERR_WITH_ONLY_STR(SCHEDULER_ST_INVALID_PAR),
	ERR_WITH_ONLY_STR(SCHEDULER_ST_INSUFFICIENT_RES),
	ERR_WITH_ONLY_STR(SCHEDULER_ST_CAN_NOT_REPLACE),
	ERR_WITH_ONLY_STR(SCHEDULER_ST_NOT_FOUND_INDEX),
	ERR_WITH_ONLY_STR(SCHEDULER_ST_INVALID_INDEX),
};

const char * get_scheduler_status_err_st_string(int err_st)
{
	if(err_st < ARRAY_SIZE(smart_home_err_st_scheduler_status)){
		return smart_home_err_st_scheduler_status[err_st].op_string;
	}
	
	return OP_STRING_NOT_FOUND;
}
#else
const char * get_op_string_sh(u16 op, const char *str_in){return OP_STRING_NOT_FOUND;}
const char * get_err_number_string_sh(u8 op){return OP_STRING_NOT_FOUND;}
const char * get_scheduler_status_err_st_string(int err_st){return OP_STRING_NOT_FOUND;};
#endif


/**
  * @}
  */

