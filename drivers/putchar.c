/********************************************************************************************************
 * @file	putchar.c
 *
 * @brief	This is the source file for TLSR8233
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
#ifndef WIN32
//#include "driver_5316.h"
#include "putchar.h"
#include "drivers/ext_driver/ext_misc.h"
#include "common/compiler.h"
#include "tl_common.h"

#define USB_PRINT_TIMEOUT	 10		//  about 10us at 30MHz
#define USB_SWIRE_BUFF_SIZE  248	// 256 - 8

#define USB_EP_IN  		(USB_EDP_PRINTER_IN  & 0X07)	//  from the point of HOST 's view,  IN is the printf out
#define USB_EP_OUT  	(USB_EDP_PRINTER_OUT & 0X07)

int usb_putc(int c) {
  #if 0
	int i = 0;
	while(i ++ < USB_PRINT_TIMEOUT){
		if(!(reg_usb_ep8_fifo_mode & FLD_USB_ENP8_FULL_FLAG)){
			reg_usb_ep_dat(USB_EP_IN) = (unsigned char)c;
			return c;
		}
	}
  #endif
	return -1;
}

static inline void swire_set_clock(unsigned char div){
  #if 0
	reg_swire_clk_div = div;
  #endif
}

//static int swire_is_init = 0;
void swire_init()
{
#if(USB_SOMATIC_ENABLE)
    //  when USB_SOMATIC_ENABLE, USB_EDP_PRINTER_OUT disable
#else
//	r_usb.ep_adr[USB_EP_IN] = r_usb.ep_adr[USB_EP_OUT] = 0;
   #if 0
	reg_usb_ep_ptr(USB_EP_IN) = reg_usb_ep_ptr(USB_EP_OUT) = 0;
	reg_usb_ep8_send_max = 64;				// 32 * 8 == 256
   #endif

	//swire_set_clock(2);

#endif
}

int swire_putc(int c)
{
#if(USB_SOMATIC_ENABLE)
    //  when USB_SOMATIC_ENABLE, USB_EDP_PRINTER_OUT disable
#else
  #if 0
	if(!swire_is_init){
		swire_init();
		swire_is_init = 1;
	}
	int i = 0;
	while(i ++ < USB_PRINT_TIMEOUT){
		if(reg_usb_ep_ptr(USB_EP_IN) - reg_usb_ep_ptr(USB_EP_OUT) <= USB_SWIRE_BUFF_SIZE){	//  not full
			reg_usb_ep_dat(USB_EP_IN) = (unsigned char)c;
			return c;
		}
	}
  #endif
#endif
	return -1;
}

#if(SIMULATE_UART_EN)
#define	BIT_INTERVAL	  		(16000000/DEBUG_BAUDRATE)
#define DEBUG_INFO_TX_PIN		(DEBUG_TX_PIN)

/*_attribute_no_retention_bss_ */static int tx_pin_initialed = 0;

/**
 * @brief  DEBUG_INFO_TX_PIN initialize. Enable 1M pull-up resistor,
 *   set pin as gpio, enable gpio output, disable gpio input.
 * @param  None
 * @retval None
 */
_attribute_no_inline_ void debug_info_tx_pin_init()
{
    gpio_setup_up_down_resistor(DEBUG_INFO_TX_PIN,PM_PIN_PULLUP_10K);
    gpio_set_func(DEBUG_INFO_TX_PIN, AS_GPIO);
	gpio_write(DEBUG_INFO_TX_PIN, 1);
    gpio_set_output_en(DEBUG_INFO_TX_PIN, 1);
    gpio_set_input_en(DEBUG_INFO_TX_PIN, 0);	
}

/* Put it into a function independently, to prevent the compiler from 
 * optimizing different pins, resulting in inaccurate baud rates.
 */
_attribute_ram_code_ 
_attribute_no_inline_ 
static void uart_do_put_char(u32 pcTxReg, u8 *bit)
{
	int j;
#if (DEBUG_BAUDRATE == 1000000)
	/*! Make sure the following loop instruction starts at 4-byte alignment: (which is destination address of "tjne") */
	// _ASM_NOP_; 
	
	for(j = 0;j<10;j++) 
	{
	#if CLOCK_SYS_CLOCK_HZ == 16000000
		CLOCK_DLY_8_CYC;
	#elif CLOCK_SYS_CLOCK_HZ == 32000000
		CLOCK_DLY_7_CYC;CLOCK_DLY_7_CYC;CLOCK_DLY_10_CYC;
	#elif CLOCK_SYS_CLOCK_HZ == 48000000
		CLOCK_DLY_8_CYC;CLOCK_DLY_8_CYC;CLOCK_DLY_10_CYC;
		CLOCK_DLY_8_CYC;CLOCK_DLY_6_CYC;
	#else
	#error "error CLOCK_SYS_CLOCK_HZ"
	#endif
		write_reg8(pcTxReg, bit[j]); 	   //send bit0
	}
#else
	u32 t1 = 0, t2 = 0;
	t1 = reg_system_tick;
	for(j = 0;j<10;j++)
	{
		t2 = t1;
		while(t1 - t2 < BIT_INTERVAL){
			t1	= reg_system_tick;
		}
		write_reg8(pcTxReg,bit[j]); 	   //send bit0
	}
#endif
}


/**
 * @brief  Send a byte of serial data.
 * @param  byte: Data to send.
 * @retval None
 */
_attribute_ram_code_ static void uart_put_char(u8 byte){
	if (!tx_pin_initialed) {
	    debug_info_tx_pin_init();
		tx_pin_initialed = 1;
	}
	volatile u32 pcTxReg = (0x503+((DEBUG_INFO_TX_PIN>>8)<<3));//register GPIO output
	u8 tmp_bit0 = read_reg8(pcTxReg) & (~(DEBUG_INFO_TX_PIN & 0xff));
	u8 tmp_bit1 = read_reg8(pcTxReg) | (DEBUG_INFO_TX_PIN & 0xff);


	u8 bit[10] = {0};
	bit[0] = tmp_bit0;
	bit[1] = (byte & 0x01)? tmp_bit1 : tmp_bit0;
	bit[2] = ((byte>>1) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[3] = ((byte>>2) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[4] = ((byte>>3) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[5] = ((byte>>4) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[6] = ((byte>>5) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[7] = ((byte>>6) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[8] = ((byte>>7) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[9] = tmp_bit1;

	/*! Minimize the time for interrupts to close and ensure timely 
	    response after interrupts occur. */
	u8 r = 0;
	if(SIMU_UART_IRQ_EN){
		r = irq_disable();
	}
	uart_do_put_char(pcTxReg, bit);
	if(SIMU_UART_IRQ_EN){
		irq_restore(r);
	}
}

/**
 * @brief  Send serial datas.
 * @param  p: Data pointer to send.
 * @param  len: Data length to send.
 * @retval None
 * @note   Previously, the irq_disable and irq_restore functions were 
 *   placed at the beginning and end of this function, which caused the 
 *   interrupt to be turned off for a long time when sending long data, 
 *   causing some interrupted code to fail to execute in time.
 */
_attribute_ram_code_ void uart_simu_send_bytes(u8 *p,int len)
{
    while(len--){
        uart_put_char(*p++);
    }
}

#endif

//the chip of Hawk serial Chip have no USB module.
int putchar(int c)
{
  #if(SIMULATE_UART_EN)
	uart_put_char(c);
  #else
	//uart_ndma_send_byte(c);
  #endif
	return c;
}

#endif

