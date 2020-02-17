/*
 * dac_test.c
 *
 *  Created on: 16.02.2020
 *      Author: pvvx
 */

#include "proj/tl_common.h"
#include "proj/drivers/audio.h"
#include "hw.h"
#include "dac.h"

s16 single_out_buf[4];

/*
 * Канал SDM_P - нечетный в буфере dfifo
 * Канал SDM_N - четный в буфере dfifo
 * Min buffer = 16 bytes (size_buff = 0) -> 8 int16_t -> 4 samples for channel
 * Min sps reg_i2s_mod = 0; reg_i2s_step = 0x81;  = 2.86 sps
 * Min sps при 1 Mhz = (1*1000000/100)/(0x8000/100)*(reg_ascl_step=1) = 30.517578 sps
 * Min sps при 8 Mhz = (8*1000000/100)/(0x8000/100)*(reg_ascl_step=1) = 244.140625 sps
 * Max sps при 8 Mhz = (8*1000000/100)/(0x8000/100)*(reg_ascl_step=4080) = 996093.75 sps
 * reg_aud_vol_ctrl = 0x3A, dac=0x7ff -> OUT +1064.1 mV
 * reg_aud_vol_ctrl = 0x3B, dac=0x7ff -> OUT +968.1 mV
 * reg_aud_vol_ctrl = 0x3C, dac=0x7ff -> OUT +871.9 mV
 * reg_aud_vol_ctrl = 0x3D, dac=0x7ff -> OUT +679.5 mV
 * reg_aud_vol_ctrl = 0x3E, dac=0x7ff -> OUT +487.3
 * reg_aud_vol_ctrl = 0x3F, dac=0x7ff -> OUT +295
 * reg_aud_vol_ctrl = 0x40, dac=0x7ff -> OUT +103 mV
 * reg_aud_vol_ctrl = 0x41, dac=0x7ff -> OUT +0.22 mV
 */

/**
 * @brief     configure the SDM buffer's address and size
 * @param[in] pbuff - the first address of buffer SDM read data from.
 * @param[in] size_buff - the size of pbuff.
 * @return    none
 */
void set_sdm_buf(signed short* pbuff, unsigned int size_buff)
{
	reg_aud_base_adr = (unsigned short)((u32)pbuff);
	reg_aud_buff_size = (size_buff>>4)-1; // min 1 , max 0xff
	reg_aud_rptr = 0; // 0..3ff;
}
/**
 *
 *	@brief	   sdm setting function, enable or disable the sdm output, configure SDM output paramaters
 *
 *	@param[in]	sdm_clk_mhz - SDM clock, 1 to 16 MHz (default 8 MHz)
 *	@param[in]	step - audio sampling rate = sdm_clk_mhz/32768/step
 *  (0x8000*sample_rate/100)/(sdm_clk_mhz*1000000/100)
 *	@return	none
 */
void sdm_init(unsigned char sdm_clk_mhz, unsigned short step)
{
		reg_aud_ctrl = 0;
#if (MCU_CORE_TYPE == MCU_CORE_8266)
		/***enable SDMN pins(sdm_n)***/
		BM_CLR(reg_gpio_gpio_func(GPIO_SDMN), GPIO_SDMN & 0xff);
		BM_CLR(reg_gpio_config_func(GPIO_SDMN), GPIO_SDMN & 0xff);
		BM_CLR(reg_gpio_config_func6, BIT(4));   //clear 0x5b6[4]
		BM_SET(reg_gpio_ie(GPIO_SDMN), GPIO_SDMN & 0xff);  //enable input
#else
		/***enable SDM pins(sdm_n,sdm_p)***/
		gpio_set_func(GPIO_SDMP,AS_SDM);  //disable gpio function
		gpio_set_func(GPIO_SDMN,AS_SDM);  //disable gpio function

		gpio_set_input_en(GPIO_SDMP,1);   //in sdk, require to enable input.because the gpio_init() function in main() function.
		gpio_set_input_en(GPIO_SDMN,1);   //in sdk, require to enable input
#endif
		// enable dmic function of SDMP and SDMM
		reg_gpio_config_func4 &= (~(FLD_DMIC_CK_RX_CLK | FLD_I2S_DI_RX_DAT));
		/***configure sdm clock; ***/
		/* I2S clock = 192M * reg_i2s_step[6:0]/reg_i2s_mod[7:0] */
		reg_i2s_step = FLD_I2S_CLK_EN | sdm_clk_mhz;
		reg_i2s_mod = get_fhs_clock_mhz();			// [0x68] = 0xC0
		reg_clk_en2 |= FLD_CLK2_AUD_EN;	// enable audio clock

		/***configure interpolation ratio***/
		// sample_rate = reg_i2s_mod * 1000000/32768/reg_ascl_step
		reg_ascl_step = step; // AUD_SDM_STEP(sample_rate, sdm_clk*1000000);

		/***2. Enable PN generator as dither control, clear bit 2, 3, 6***/
		// [0x0560] = 0x08084033
//		reg_aud_ctrl = FLD_AUD_PN2_GENERATOR_EN | FLD_AUD_PN1_GENERATOR_EN;
//		reg_aud_ctrl = FLD_AUD_SHAPPING_EN | FLD_AUD_SHAPING_EN | FLD_AUD_PN2_GENERATOR_EN | FLD_AUD_PN1_GENERATOR_EN;

		reg_aud_pn1 = 0x08;  //PN generator 1 bits used
		reg_aud_pn2 = 0x08;  //PN generator 1 bits used

		// set volume
		reg_aud_vol_ctrl = 0x40; // volume = ~ 1.0 x 16 bits samples (3.3V p-p)
		// set pointer to start
		reg_aud_rptr = 0;
		// enable audio and sdm player.
#if 0
		reg_aud_ctrl = FLD_AUD_ENABLE | FLD_AUD_SDM_PLAY_EN;
#else
		// прибавка шума ?
		reg_aud_ctrl = FLD_AUD_ENABLE | FLD_AUD_SDM_PLAY_EN | FLD_AUD_PN2_GENERATOR_EN | FLD_AUD_PN1_GENERATOR_EN;
#endif
}

static void set_dac_out(signed short value) {
	single_out_buf[0] = value;
	single_out_buf[1] = value;
	single_out_buf[2] = value;
	single_out_buf[3] = value;
	reg_aud_base_adr = (unsigned short)((u32)single_out_buf);
	reg_aud_buff_size = 0; // min step
	reg_aud_rptr = 0;
}

//===== TEST! ==============
#define ADC_DFIFO_SIZE	512
extern s16 dfifo[ADC_DFIFO_SIZE];

static void gen_triangular(void) {
	u32 i;
#if 1
	u32 x = 0x8000;
	for(i = 0; i < ADC_DFIFO_SIZE/2; i+=2) {
		dfifo[i] = x;
		x+=(65536/(ADC_DFIFO_SIZE/4));
	}
	for(; i < ADC_DFIFO_SIZE; i+=2) {
		x-=(65536/(ADC_DFIFO_SIZE/4));
		dfifo[i] = x;
	}
#else
	for(i = 0; i < ADC_DFIFO_SIZE; i+=4) {
		dfifo[i] = 0x7fff;
		dfifo[i+2] = 0x8000;
	}
#endif
	sdm_set_buf(dfifo, sizeof(dfifo));
}
static void gen_u(signed short value) {
	u32 i;
	for(i = 0; i < ADC_DFIFO_SIZE; i++) {
		dfifo[i] = value;
	}
	sdm_set_buf(dfifo, sizeof(dfifo));
}

// ADC/step = 488
extern dev_adc_cfg_t cfg_adc;
extern int init_adc_dfifo(dev_adc_cfg_t * p);
// 		0409 05 10 6300
static void test(dev_dac_cfg_t *p) {
	cfg_adc.sps = p->step*488;
	init_adc_dfifo(&cfg_adc);
	sdm_set_buf(dfifo, sizeof(dfifo));
	sdm_init(p->slk_mhz, p->step);
}

//===== TEST! ============== end

unsigned int dac_cmd(dev_dac_cfg_t *p) {
	int ret = p->mode;
	switch(p->mode) {
	case 1:
		set_dac_out(p->value[0]);
		sdm_init(p->slk_mhz, p->step);
		break;
	case 2: // test! out value (adc off!)
		gen_u(p->value[0]);
		sdm_init(p->slk_mhz, p->step);
		break;
	case 3: // test! out triangular (adc off!)
		gen_triangular();
		sdm_init(p->slk_mhz, p->step);
		break;
	case 4: // test! adc->dac
		sdm_set_buf(dfifo, sizeof(dfifo));
		sdm_init(p->slk_mhz, p->step);
		break;
	case 5: // test! set adc->dac
		test(p);
		break;
//	case 0:
	default:
		sdm_off();
		break;
	}
	return ret;
}