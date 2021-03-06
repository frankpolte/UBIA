/*
 * app.c
 *
 *  Created on: 10.11.2019
 *      Author: pvvx
 */

#include "proj/tl_common.h"
#if (USE_BLE)
#include "ble.h"
#else
#include "inits.h"
#endif
#include "flash_eep.h"
#include "eep_id.h"
#if (USE_USB_CDC)
#include "usb.h"
#endif
#if (USE_I2C_DEV)
#include "i2c_dev.h"
#endif
#if (USE_ADC_DEV)
#include "adc_dev.h"
#endif
#if (USE_HX711)
#include "hx711.h"
#endif
#if (USE_DAC_DEV)
#include "dac.h"
#endif
#if	(USE_UART_DEV)
#include "uart_dev.h"
#endif

#if (BATT_SERVICE_ENABLE)
	extern u8  my_batVal[20];
	extern u16 batteryValueInCCC;
	extern u8 adc_hw_initialized;
#endif


void ExtDevPowerOff() {
#ifdef EXT_POWER_OFF
	gpio_set_pull_resistor(EXT_POWER_OFF, PM_PIN_PULLUP_10K);
#endif
	gpio_set_pull_resistor(EXT_POWER_4MA, PM_PIN_PULLDOWN_100K);
	gpio_set_data_strength(EXT_POWER_4MA, 0);
	gpio_set_output_en(EXT_POWER_4MA, 0);
	gpio_write(EXT_POWER_4MA, 0);
}

void ExtDevPowerOn() {
#ifdef EXT_POWER_OFF
	gpio_set_pull_resistor(EXT_POWER_OFF, PM_PIN_PULLDOWN_100K);
#endif
	gpio_set_pull_resistor(EXT_POWER_4MA, PM_PIN_PULLUP_10K);
	gpio_write(EXT_POWER_4MA, 1);
//	gpio_set_data_strength(EXT_POWER_4MA, 0);
	gpio_set_output_en(EXT_POWER_4MA, 1);
	gpio_set_data_strength(EXT_POWER_4MA, 1);
}

void app_suspend_exit(void);

void led_init(void) {
#ifdef GREEN_LED
	gpio_write(GREEN_LED,OFF);
#endif
#ifdef BLUE_LED
	gpio_write(BLUE_LED,OFF);
#endif
}

void eep_init(void)
{
#if (USE_BLE)
	if (flash_read_cfg(&ble_cfg_ini, EEP_ID_BLE_CFG, sizeof(ble_cfg_ini)) != sizeof(ble_cfg_ini)) {
		memcpy(&ble_cfg_ini, &def_ble_cfg_ini, sizeof(ble_cfg_ini));
	}
	if (flash_read_cfg(&ble_con_ini, EEP_ID_CON_CFG, sizeof(ble_con_ini)) != sizeof(ble_con_ini)) {
		memcpy(&ble_con_ini, &def_ble_con_ini, sizeof(ble_con_ini));
	}
#endif
#if (BATT_SERVICE_ENABLE)
	if (flash_read_cfg(&dev_cfg, EEP_ID_DEV_CFG, sizeof(dev_cfg)) != sizeof(dev_cfg)) {
		dev_cfg = dev_cfg_def;
//		flash_write_cfg(&dev_cfg, EEP_ID_DEV_CFG, sizeof(dev_cfg));
	}
	if (dev_cfg.vbat_check_step_ms < 50)
		dev_cfg.vbat_check_step_ms = 50;
#endif
#if USE_I2C_DEV
	if (flash_read_cfg(&cfg_i2c, EEP_ID_I2C_CFG, sizeof(cfg_i2c)) != sizeof(cfg_i2c)) {
		memcpy(&cfg_i2c, &def_cfg_i2c, sizeof(cfg_i2c));
	}
	cfg_i2c.pktcnt = 0; // read off
//	I2CDevSleep();
#endif
#if USE_ADC_DEV
	if (flash_read_cfg(&cfg_adc, EEP_ID_ADC_CFG, sizeof(cfg_adc)) != sizeof(cfg_adc)) {
		memcpy(&cfg_adc, &def_cfg_adc, sizeof(cfg_adc));
	}
//	cfg_adc.pktcnt = 0;
	ADC_Stop();
#endif
#if USE_DAC_DEV
	if (flash_read_cfg(&cfg_dac, EEP_ID_DAC_CFG, sizeof(cfg_dac)) != sizeof(cfg_dac)) {
		memcpy(&cfg_dac, &def_cfg_dac, sizeof(cfg_dac));
	}
#endif
#if USE_UART_DEV
	if (flash_read_cfg(&cfg_uart, EEP_ID_UART_CFG, sizeof(cfg_uart)) != sizeof(cfg_uart)) {
		memcpy(&cfg_uart, &def_cfg_uart, sizeof(cfg_uart));
	}
#endif
}

void user_init()
{
//	gpio_set_pull_resistor(KEY_BLE_USB, PM_PIN_PULLDOWN_100K);
#if 0
	REG_ADDR8(0x74) = 0x53;
#if	(CHIP_TYPE == CHIP_TYPE_8269)
	REG_ADDR16(0x7e) = 0x08d1;
#else
	REG_ADDR16(0x7e) = 0x08d0; // = 0x5325
#endif
	REG_ADDR8(0x74) = 0x00;
#endif
	eep_init();
/////////////////////////////////////////////////////////////
#if	(USE_BLE && USE_USB_CDC)
	if (tst_usb_actived()) {
		usb_init();
	} else {
		ble_init();
	}
#elif (USE_BLE)
	ble_init();
#elif (USE_USB_CDC)
	usb_init();
#endif
}
