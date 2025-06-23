/********************************************************************************************************
 * @file     app_config.h
 *
 * @brief    This is the header file for BLE SDK
 *
 * @author	 BLE GROUP
 * @date         12,2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
 *******************************************************************************************************/

#pragma once

#include "vendor/common/smart_home/version_type.h"

#define PCBA_8208_C1T261A30_V1_0					1
#define PCBA_8208_KP_CL08D0040R040          		2
	
#define PCBA_8208_SEL								PCBA_8208_C1T261A30_V1_0

///////////////////////// Feature Configuration////////////////////////////////////////////////
/**
 *  @brief  Feature select in BLE Sample project
 */
#define FLASH_SIZE_OPTION							FLASH_SIZE_OPTION_128K //very important, user need confirm !!!
#define APP_SECURITY_ENABLE      					0
#define	BLE_OTA_SERVER_ENABLE						0
#define TEST_CONN_CURRENT_ENABLE					0		//test connection current, disable UI to have a pure power
#define SAVE_RAM_CODE_ENABLE						0			//Enable it if need save RAM code.
																				//Note: When this is enabled, the average power consumption increases.If user use deepsleep retention, user need to add DeepsleepRetentionEarlyWakeupTiming.
#define TELIK_SPP_SERVICE_ENABLE					1

/* firmware signature check */
#define FIRMWARES_SIGNATURE_ENABLE     		 		0

/* Power Management */
#define BLE_APP_PM_ENABLE							0
#define PM_DEEPSLEEP_ENABLE            				0 		//test connection power, should disable deepSleep
#define PM_DEEPSLEEP_RETENTION_ENABLE               0

/* Flash Protection:
 * 1. Flash protection is enabled by default in SDK. User must enable this function on their final mass production application.
 * 2. User should use "Unlock" command in Telink BDT tool for Flash access during development and debugging phase.
 * 3. Flash protection demonstration in SDK is a reference design based on sample code. Considering that user's final application may
 *    different from sample code, for example, user's final firmware size is bigger, or user have a different OTA design, or user need
 *    store more data in some other area of Flash, all these differences imply that Flash protection reference design in SDK can not
 *    be directly used on user's mass production application without any change. User should refer to sample code, understand the
 *    principles and methods, then change and implement a more appropriate mechanism according to their application if needed.
 */
#define APP_FLASH_PROTECTION_ENABLE					1

/* smart home setting -----------------------------------------------------------*/
#define SMART_HOME_SERVER_EN			1
#if SMART_HOME_SERVER_EN
#define SMART_HOME_ONOFF_EN				1
#define SMART_HOME_CT_EN				1
#define SMART_HOME_RGB_EN				1
#define SMART_HOME_SCENE_EN				1
#define SMART_HOME_TIME_EN				1
#define SMART_HOME_SCHEDULER_EN			1
#define SMART_HOME_ONLINE_ST_EN			1
#define LIGHT_ADJUST_STEP_EN    		0
#define LIGHT_TRANSITION_ALL_EN			1
#endif
#define FEATURE_RELAY_EN				1


/* PWM Configuration --------------------------------------------------*/
#if (PCBA_8208_SEL == PCBA_8208_KP_CL08D0040R040)
#define	I2C_CLK_SPEED					(200 * 1000) 	// unit: Hz
#define I2C_IO_SDA						GPIO_PB5
#define I2C_IO_SCL						GPIO_PB6

#define LED_DRIVER_MODE_KP18058_ENABLE	1
#define GPIO_LED_OUT_ADDR				(KP18058_RED_ADDR)
#else
#define PWM_R     	GPIO_PA7			//red
#define PWM_G     	GPIO_PA5			//green
#define PWM_B     	GPIO_PA4			//blue
#define PWM_W     	GPIO_PA6			//while

#define PWMID_R     0//must be different from each other//(GET_PWMID(PWM_R))
#define PWMID_G     1//must be different from each other//(GET_PWMID(PWM_G))
#define PWMID_B     2//must be different from each other//(GET_PWMID(PWM_B))
#define PWMID_W     3//must be different from each other//(GET_PWMID(PWM_W))
                    
#define PWM_INV_R   0//(GET_PWM_INVERT_VAL(PWM_R))
#define PWM_INV_G   0//(GET_PWM_INVERT_VAL(PWM_G))
#define PWM_INV_B   0//(GET_PWM_INVERT_VAL(PWM_B))
#define PWM_INV_W   0//(GET_PWM_INVERT_VAL(PWM_W))

#ifndef GPIO_LED
#define GPIO_LED	PWM_R
#endif
#endif



/**
 *  @brief  UI Configuration
 */
#if (TEST_CONN_CURRENT_ENABLE)
	#define UI_LED_ENABLE          	 				0
	#define	UI_KEYBOARD_ENABLE						0
#else
	#define UI_LED_ENABLE          	 				0	// use LED proc in light_driver.c
	#define	UI_KEYBOARD_ENABLE						0
#endif

#define BATT_CHECK_ENABLE       					0   //enable or disable battery voltage detection

/* Simulate uart debug Interface ---------------------------------------------*/
#define DEBUG_LOG_SETTING_DEVELOP_MODE_EN		1
#ifndef HCI_LOG_FW_EN
#define HCI_LOG_FW_EN   		(0 || DEBUG_LOG_SETTING_DEVELOP_MODE_EN)
#endif
#define SIMULATE_UART_EN       	HCI_LOG_FW_EN
#if SIMULATE_UART_EN
#define DEBUG_TX_PIN           	GPIO_PD3
#define DEBUG_BAUDRATE         	(1000000) // (115200)
#define SIMU_UART_IRQ_EN    	(1)		// 1: disable irq while sending UART to keep baudrate stable.
#endif


/**
 *  @brief  DEBUG  Configuration
 */
#define DEBUG_GPIO_ENABLE							0


///////////////////////// System Clock  Configuration /////////////////////////////////////////
#define CLOCK_SYS_CLOCK_HZ      					16000000

#if (CLOCK_SYS_CLOCK_HZ == 16000000)
	#define SYS_CLK_TYPE  							SYS_CLK_16M_Crystal
#elif (CLOCK_SYS_CLOCK_HZ == 24000000)
	#define SYS_CLK_TYPE  							SYS_CLK_24M_Crystal
#elif(CLOCK_SYS_CLOCK_HZ == 32000000)
	#define SYS_CLK_TYPE  							SYS_CLK_32M_Crystal
#elif(CLOCK_SYS_CLOCK_HZ == 48000000)
	#define SYS_CLK_TYPE  							SYS_CLK_48M_Crystal
#else
	#error "unsupported system clock !"
#endif

enum{
	CLOCK_SYS_CLOCK_1S = CLOCK_SYS_CLOCK_HZ,
	CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
	CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};


/**
 *  @brief  watchdog enable and timeout setting
 */
#define MODULE_WATCHDOG_ENABLE						0
#define WATCHDOG_INIT_TIMEOUT						500  //Unit:ms


/**
 *  @brief  software uart enable and setting
 */
#define 	SOFT_UART_BAUD_RATE                     9600
#define 	SOFT_UART_TX_IO                   		GPIO_PD5
#define 	SOFT_UART_RX_IO                   		GPIO_PD6


#if (UI_KEYBOARD_ENABLE)   // if test pure power, kyeScan GPIO setting all disabled
	//---------------  KeyMatrix PA0/PD4/PF0/PF1 -----------------------------
	#define	MATRIX_ROW_PULL					PM_PIN_PULLDOWN_100K
	#define	MATRIX_COL_PULL					PM_PIN_PULLUP_10K

	#define	KB_LINE_HIGH_VALID				0   //dirve pin output 0 when keyscan, scanpin read 0 is valid

	#define			CR_VOL_UP				0xf0
	#define			CR_VOL_DN				0xf1

	/**
	 *  @brief  Normal keyboard map
	 */
	#define		KB_MAP_NORMAL	{	{CR_VOL_UP,		VK_1},	 \
									{CR_VOL_DN,		VK_2}, }

	//////////////////// KEY CONFIG (EVK board) ///////////////////////////
	#define  KB_DRIVE_PINS  {GPIO_PF0, GPIO_PF1}
	#define  KB_SCAN_PINS   {GPIO_PA0, GPIO_PD4}

	//drive pin as gpio
	#define	PF0_FUNC				AS_GPIO
	#define	PF1_FUNC				AS_GPIO

	//drive pin need 100K pulldown
	#define	PULL_WAKEUP_SRC_PF0		MATRIX_ROW_PULL
	#define	PULL_WAKEUP_SRC_PF1		MATRIX_ROW_PULL

	//drive pin open input to read gpio wakeup level
	#define PF0_INPUT_ENABLE		1
	#define PF1_INPUT_ENABLE		1

	//scan pin as gpio
	#define	PA0_FUNC				AS_GPIO
	#define	PD4_FUNC				AS_GPIO

	//scan  pin need 10K pullup
	#define	PULL_WAKEUP_SRC_PA0		MATRIX_COL_PULL
	#define	PULL_WAKEUP_SRC_PD4		MATRIX_COL_PULL

	//scan pin open input to read gpio level
	#define PA0_INPUT_ENABLE		1
	#define PD4_INPUT_ENABLE		1

	#define		KB_MAP_NUM		KB_MAP_NORMAL
	#define		KB_MAP_FN			KB_MAP_NORMAL
#endif


#if(UI_LED_ENABLE)
	/**
	 *  @brief  Definition gpio for led
	 */
	#define	GPIO_LED_BLUE  	GPIO_PA4
	#define	GPIO_LED_GREEN	GPIO_PA5
	#define	GPIO_LED_WHITE	GPIO_PA6
	#define	GPIO_LED_RED	GPIO_PA7
	#define LED_ON_LEVAL 			1 		//gpio output high voltage to turn on led

	#define PA4_FUNC				AS_GPIO
	#define PA5_FUNC				AS_GPIO
	#define PA6_FUNC				AS_GPIO
	#define PA7_FUNC				AS_GPIO

	#define	PA4_OUTPUT_ENABLE		1
	#define	PA5_OUTPUT_ENABLE		1
	#define PA6_OUTPUT_ENABLE		1
	#define	PA7_OUTPUT_ENABLE		1
#endif




/**
 *  @brief  Battery_check Configuration
 */
#if (BATT_CHECK_ENABLE)
	#define ADC_INPUT_PCHN					VBAT    //corresponding  ADC_InputPchTypeDef in adc.h

	#define VBAT_ALRAM_THRES_MV				2000   // 2000 mV low battery alarm

	/////////////////// DEEP SAVE FLG //////////////////////////////////
	#define USED_DEEP_ANA_REG                   DEEP_ANA_REG0 //u8,can save 8 bit info when deep
	#define	LOW_BATT_FLG					    BIT(0)

	//////////////////////////// FEATURE PM GPIO	(EVK board) /////////////////////////////////
	#define GPIO_WAKEUP_FEATURE				GPIO_PB6   //mcu wakeup module
	#define	PB6_FUNC							AS_GPIO
	#define PB6_INPUT_ENABLE					1
	#define	PB6_OUTPUT_ENABLE					0
	#define	PB6_DATA_OUT						0
	#define GPIO_WAKEUP_FEATURE_HIGH				gpio_setup_up_down_resistor(GPIO_WAKEUP_FEATURE, PM_PIN_PULLUP_10K);
	#define GPIO_WAKEUP_FEATURE_LOW				gpio_setup_up_down_resistor(GPIO_WAKEUP_FEATURE, PM_PIN_PULLDOWN_100K);
#endif

/**
 *  @brief  Definition for gpio debug
 */

#if(DEBUG_GPIO_ENABLE)
	#define PD0_FUNC				AS_GPIO //debug gpio chn1 : PD0
	#define PB1_FUNC				AS_GPIO //debug gpio chn2 : PB1
	#define PB2_FUNC				AS_GPIO //debug gpio chn3 : PB2
	#define PB3_FUNC                AS_GPIO //debug gpio chn4 : PB3
	#define PB4_FUNC				AS_GPIO //debug gpio chn5 : PB4
	#define PB5_FUNC				AS_GPIO //debug gpio chn6 : PB5
	#define PB6_FUNC				AS_GPIO //debug gpio chn7 : PB6
	#define PB7_FUNC				AS_GPIO //debug gpio chn0 : PB7

	#define PC0_FUNC				AS_GPIO //debug gpio chn8 : PC0
	#define PC1_FUNC                AS_GPIO //debug gpio chn9 : PC1
	#define PD2_FUNC				AS_GPIO //debug gpio chn10: PD2
	#define PD3_FUNC                AS_GPIO //debug gpio chn11: PD3
	#define PC4_FUNC				AS_GPIO //debug gpio chn12: PC4
	#define PC5_FUNC                AS_GPIO //debug gpio chn13: PC5
	#define PC6_FUNC				AS_GPIO //debug gpio chn14: PC6
	#define PC7_FUNC                AS_GPIO //debug gpio chn15: PC7

	#define GPIO_CHN0				GPIO_PD0
	#define GPIO_CHN1				GPIO_PB1
	#define GPIO_CHN2				GPIO_PB2
	#define GPIO_CHN3				GPIO_PB3
	#define GPIO_CHN4				GPIO_PB4
	#define GPIO_CHN5				GPIO_PB5
	#define GPIO_CHN6				GPIO_PB6
	#define GPIO_CHN7				GPIO_PB7

	#define GPIO_CHN8 				GPIO_PC0
	#define GPIO_CHN9 				GPIO_PC1
	#define GPIO_CHN10				GPIO_PD2
	#define GPIO_CHN11				GPIO_PD3
	#define GPIO_CHN12				GPIO_PC4
	#define GPIO_CHN13				GPIO_PC5
	#define GPIO_CHN14				GPIO_PC6
	#define GPIO_CHN15				GPIO_PC7

	#define PD0_OUTPUT_ENABLE					1
	#define PB1_OUTPUT_ENABLE					1
	#define PB2_OUTPUT_ENABLE					1
	#define PB3_OUTPUT_ENABLE					1
	#define PB4_OUTPUT_ENABLE					1
	#define PB5_OUTPUT_ENABLE					1
	#define PB6_OUTPUT_ENABLE					1
	#define PB7_OUTPUT_ENABLE					1

	#define PC0_OUTPUT_ENABLE					1
	#define PC1_OUTPUT_ENABLE					1
	#define PD2_OUTPUT_ENABLE					1
	#define PD3_OUTPUT_ENABLE					1
	#define PC4_OUTPUT_ENABLE					1
	#define PC5_OUTPUT_ENABLE					1
	#define PC6_OUTPUT_ENABLE					1
	#define PC7_OUTPUT_ENABLE					1
#endif  //end of DEBUG_GPIO_ENABLE








/////////////////// set default   ////////////////

#include "../common/default_config.h"
