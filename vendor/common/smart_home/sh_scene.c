/********************************************************************************************************
 * @file	sh_scene.c
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
#include "sh_scene.h"


#if(SMART_HOME_SCENE_EN)

static lcmd_scene_set_t scene_data[MAX_SCENE_CNT];

STATIC_ASSERT(MAX_SCENE_CNT >= DEFAULT_EXISTED_SCENE_CNT);	// because there are 4 default scenes.


static int is_valid_scene_id(u32 id)
{
	return ((INVALID_SCENE_ID != id) && (id <= SCENE_ID_MAX));
}

void scene_copy_par(lcmd_scene_set_t *p_node, lcmd_scene_set_t *p_set_scene)
{
	memcpy(p_node, p_set_scene, sizeof(lcmd_scene_set_t));
	p_node->set_mode = 0;
	p_node->rsv = 0;
	p_node->ack = 0;
}

rx_cb_err_t scene_set_rx_handle(lcmd_scene_set_t *p_set)
{
	rx_cb_err_t err = RX_CB_SUCCESS;
	int save_flag = 0;

	if((SH_SCENE_ADD == p_set->set_mode) || (SH_SCENE_DEL == p_set->set_mode)){
		if(!is_valid_scene_id(p_set->id)){
			return RX_CB_ERR_SCENE_ID_INVALID;
		}
	
		int add_ok_flag = 0;
		lcmd_scene_set_t *p_null = NULL;
		foreach_arr(i, scene_data){
			lcmd_scene_set_t *p_node = &scene_data[i];
			if(p_set->id == p_node->id){
				if(SH_SCENE_ADD == p_set->set_mode){
					if((0 != memcmp(&p_node->id, &p_set->id, sizeof(lcmd_scene_set_t) - OFFSETOF(lcmd_scene_set_t, id)))
					|| (p_node->ct_flag != p_set->ct_flag)
					|| (p_node->rgb_flag != p_set->rgb_flag)){
						scene_copy_par(p_node, p_set);
						save_flag = 1;
					}
					add_ok_flag = 1;
					break;
				}if(SH_SCENE_DEL == p_set->set_mode){
					memset(p_node, 0, sizeof(scene_data[0]));
					save_flag = 1;
					// break;	// no break, in case that there are same ids.
				}
			}else if((NULL == p_null) && (INVALID_SCENE_ID == p_node->id)){
				p_null = p_node;
			}
		}
	
		if((SH_SCENE_ADD == p_set->set_mode) && (0 == add_ok_flag)){
			if(p_null){
				scene_copy_par(p_null, p_set);
				save_flag = 1;
			}else{
				return RX_CB_ERR_GROUP_FULL;
			}
		}
	}else if(SH_SCENE_DEL_ALL == p_set->set_mode){
		foreach_arr(i, scene_data){
			if(scene_data[i].id != INVALID_SCENE_ID){
				memset(scene_data, 0, sizeof(scene_data));	// delete all
				save_flag = 1;
				break;
			}
		}
	}
	
	if(save_flag){
		smart_home_scene_save();
	}
	
	smart_home_led_event_callback(LED_EVENT_SETTING);

	return err;
}

tx_cmd_err_t scene_tx_rsp(u16 addr_dst, u8 scene_id, rx_cb_err_t st, int report_value_flag)
{
	lcmd_scene_st_t cmd_status = {0};
	int rsp_len = OFFSETOF(lcmd_scene_st_t, lightness);
	cmd_status.id = scene_id;
	if((RX_CB_SUCCESS == st) && report_value_flag){
		if(is_valid_scene_id(scene_id)){
			st = RX_CB_ERR_SCENE_ID_NOT_FOUND;
			foreach_arr(i, scene_data){
				if(scene_data[i].id == scene_id){
					memcpy(&cmd_status, &scene_data[i], sizeof(cmd_status));
					rsp_len = sizeof(lcmd_scene_st_t);
					st = RX_CB_SUCCESS;
					break;
				}
			}
		}else{
			st = RX_CB_ERR_SCENE_ID_INVALID;
		}
	}
	cmd_status.st = st;

	return smart_home_cmd_tx_rsp(LIGHT_CMD_SCENE_STATUS, addr_dst, (u8 *)&cmd_status, rsp_len);
}

rx_cb_err_t scene_set_rx_cb(smart_home_nw_t *p_nw)
{
	rx_cb_err_t err = RX_CB_SUCCESS;
	lcmd_scene_set_t *p_set = (lcmd_scene_set_t *)p_nw->par;
	if(SH_SCENE_GET == p_set->set_mode){
		if(!is_valid_scene_id(p_set->id)){
			err = RX_CB_ERR_SCENE_ID_INVALID;
		}
	}else{
		err = scene_set_rx_handle(p_set);
	}
	
	if((SH_SCENE_GET == p_set->set_mode) || p_set->ack){ // response
		int report_value_flag = ((SH_SCENE_GET == p_set->set_mode)||(SH_SCENE_ADD == p_set->set_mode));
		u8 scene_id = (SH_SCENE_DEL_ALL == p_set->set_mode) ? 0 : p_set->id;
		scene_tx_rsp(p_nw->addr_src, scene_id, err, report_value_flag);
	}
	
	return err;
}

rx_cb_err_t scene_load(u8 id_call)
{
	rx_cb_err_t err = RX_CB_ERR_SCENE_ID_NOT_FOUND;
	if(!is_valid_scene_id(id_call)){
		err = RX_CB_ERR_SCENE_ID_INVALID;
	}else{
		foreach_arr(i, scene_data){
			lcmd_scene_set_t *p_node = &scene_data[i];
			if(id_call == p_node->id){
				// action
				if((p_node->lightness == 0) || (p_node->rgb_flag && (p_node->rgb.R == 0) && (p_node->rgb.G == 0) && (p_node->rgb.B == 0))){
					light_onoff(0);
				}else{
					#if SMART_HOME_CT_EN
					if(p_node->ct_flag){
						light_adjust_CT(p_node->ct, p_node->lightness);
					}
					#endif
					#if SMART_HOME_RGB_EN
					if(p_node->rgb_flag){
						light_adjust_RGB(&p_node->rgb, p_node->lightness);
					}
					#endif
					if((0 == p_node->ct_flag) && (0 == p_node->rgb_flag)){
						light_adjust_lightness(p_node->lightness);
					}
				}
				
				err = RX_CB_SUCCESS;
				break;
			}
		}
	}

	return err;
}

rx_cb_err_t scene_call_rx_cb(smart_home_nw_t *p_nw)
{
	rx_cb_err_t err = RX_CB_ERR_SCENE_ID_NOT_FOUND;
	lcmd_scene_call_t *p_call = (lcmd_scene_call_t *)p_nw->par;

	err = scene_load(p_call->id);
	
	if(p_call->ack){ // response
		scene_tx_rsp(p_nw->addr_src, p_call->id, err, 1);	// need to report value because APP need to refresh light status of scene that have been changed by other phones.
	}
	
	return err;
}

rx_cb_err_t scene_add_no_par_rx_cb(smart_home_nw_t *p_nw)
{
	rx_cb_err_t err = RX_CB_SUCCESS;
	lcmd_scene_add_no_par_t *p_add = (lcmd_scene_add_no_par_t *)p_nw->par;

	if(!is_valid_scene_id(p_add->id)){
		err = RX_CB_ERR_SCENE_ID_INVALID;
	}else{
		lcmd_scene_set_t *p_save = NULL;
		foreach_arr(i, scene_data){
			lcmd_scene_set_t *p_node = &scene_data[i];
			if(p_add->id == p_node->id){
				p_save = p_node;
				break;
			}else if((NULL == p_save) && (INVALID_SCENE_ID == p_node->id)){
				p_save = p_node;	// the first NULL position.
			}
		}

		if(p_save){
			set_current_status2scene(p_save, p_add->id);
			smart_home_scene_save();
			smart_home_led_event_callback(LED_EVENT_SETTING);
		}else{
			err = RX_CB_ERR_GROUP_FULL;
		}
	}
	
	if(p_add->ack){ // response
		lcmd_scene_add_no_par_st_t cmd_status = {0};
		cmd_status.id = err;
		cmd_status.id = p_add->id;
		
		tx_cmd_err_t err_tx = smart_home_cmd_tx_rsp(LIGHT_CMD_SCENE_ADD_NO_PAR_ST, p_nw->addr_src, (u8 *)&cmd_status, sizeof(cmd_status));
		if(err_tx){
			err = RX_CB_ERR_TX_RSP_FAILED;
		}
	}
	
	return err;
}

rx_cb_err_t scene_id_list_get_rx_cb(smart_home_nw_t *p_nw)
{
	rx_cb_err_t err = RX_CB_SUCCESS;
	lcmd_scene_id_list_get_t *p_get = (lcmd_scene_id_list_get_t *)p_nw->par;
	lcmd_scene_id_list_t list = {0};
	int rsp_len = OFFSETOF(lcmd_scene_id_list_t, id_list);
	int total_cnt = 0;
	int index = 0;

	foreach_arr(i, scene_data){
		lcmd_scene_set_t *p_node = &scene_data[i];
		if(p_node->id != INVALID_SCENE_ID){
			if(total_cnt++ >= p_get->offset){
				if(index < ARRAY_SIZE(list.id_list)){
					list.id_list[index++] = p_node->id;
				}
			}
		}
	}

	list.offset = p_get->offset;
	list.total_cnt = total_cnt;
	rsp_len += index * sizeof(list.id_list[0]);
	
	smart_home_cmd_tx_rsp(LIGHT_CMD_SCENE_ID_LIST, p_nw->addr_src, (u8 *)&list, rsp_len);
	
	return err;
}


static u32 smart_home_scene_addr = FLASH_ADDR_SMART_HOME_SCENE_SAVE;

void smart_home_scene_save()
{
    smart_home_par_store((u8 *)scene_data, &smart_home_scene_addr, FLASH_ADDR_SMART_HOME_SCENE_SAVE, sizeof(scene_data));
}

int smart_home_scene_retrieve()
{
    int err = smart_home_par_retrieve((u8 *)scene_data, &smart_home_scene_addr, FLASH_ADDR_SMART_HOME_SCENE_SAVE, sizeof(scene_data));
	if(err){
		// DEFAULT_EXISTED_SCENE_CNT
		{
			lcmd_scene_set_t *p_scene1 = &scene_data[0];	//  Leave Home, ID = 1
			p_scene1->id = 1;
			//p_scene1->ct_flag = 0;
			//p_scene1->rgb_flag = 0;
			p_scene1->lightness = 0;
		}

		{
			lcmd_scene_set_t *p_scene2 = &scene_data[1];	//  Back Home, ID = 2
			p_scene2->id = 2;
			//p_scene2->ct_flag = 0;
			//p_scene2->rgb_flag = 0;
			p_scene2->lightness = 100;
		}

		{
			lcmd_scene_set_t *p_scene3 = &scene_data[2];	//  Rest, ID = 3
			p_scene3->id = 3;
			//p_scene3->ct_flag = 0;
			//p_scene3->rgb_flag = 0;
			p_scene3->lightness = 5;
		}

		{
			lcmd_scene_set_t *p_scene4 = &scene_data[3];	//  Watch TV, ID = 4
			p_scene4->id = 4;
			//p_scene4->ct_flag = 0;
			//p_scene4->rgb_flag = 0;
			p_scene4->lightness = 10;
		}
	}
	
    return err;
}

#endif

