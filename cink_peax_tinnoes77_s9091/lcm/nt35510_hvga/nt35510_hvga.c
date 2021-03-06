/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

#include <linux/string.h>
#if defined(BUILD_UBOOT)
#include <asm/arch/mt6577_gpio.h>
#else
#include <mach/mt6577_gpio.h>
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(320)
#define FRAME_HEIGHT 										(480)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))
/*Begin lenovo-sw wengjun1 add for p700 lcd compatile 2012-3-5*/  
#define LCM_ID       (0x55)
#define LCM_ID1       (0xC1)
#define LCM_ID2       (0x80)
/*End lenovo-sw wengjun1 add for p700 lcd compatile 2012-3-5*/  

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)         

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
		unsigned int div2_real=0;
		unsigned int cycle_time = 0;
		unsigned int ui = 0;
		unsigned int hs_trail_m, hs_trail_n;
		#define NS_TO_CYCLE(n, c)	((n) / c + (( (n) % c) ? 1 : 0))

		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

		// enable tearing-free
//		params->dbi.te_mode 			= LCM_DBI_TE_MODE_VSYNC_ONLY;
//		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

		params->dsi.mode   = CMD_MODE;

		// DSI
		/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_TWO_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		params->dsi.packet_size=256;

		// Video mode setting		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

		params->dsi.word_count=480*3;	
		params->dsi.vertical_sync_active=2;
		params->dsi.vertical_backporch=2;
		params->dsi.vertical_frontporch=2;
		params->dsi.vertical_active_line=800;
	
		params->dsi.line_byte=2180;		// 2256 = 752*3
		params->dsi.horizontal_sync_active_byte=26;
		params->dsi.horizontal_backporch_byte=206;
		params->dsi.horizontal_frontporch_byte=206;	
		params->dsi.rgb_byte=(480*3+6);	
	
		params->dsi.horizontal_sync_active_word_count=20;	
		params->dsi.horizontal_backporch_word_count=200;
		params->dsi.horizontal_frontporch_word_count=200;

		// Bit rate calculation
		params->dsi.pll_div1=38;		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
		params->dsi.pll_div2=1;			// div2=0~15: fout=fvo/(2*div2)

		div2_real=params->dsi.pll_div2 ? params->dsi.pll_div2*0x02 : 0x1;
		cycle_time = (8 * 1000 * div2_real)/ (26 * (params->dsi.pll_div1+0x01));
		ui = (1000 * div2_real)/ (26 * (params->dsi.pll_div1+0x01)) + 1;
		
		hs_trail_m=params->dsi.LANE_NUM;
		hs_trail_n=NS_TO_CYCLE(((params->dsi.LANE_NUM * 4 * ui) + 60), cycle_time);

//		params->dsi.HS_TRAIL	= ((hs_trail_m > hs_trail_n) ? hs_trail_m : hs_trail_n) + 3;//min max(n*8*UI, 60ns+n*4UI)
		params->dsi.HS_TRAIL	= 20;
		params->dsi.HS_ZERO 	= NS_TO_CYCLE((115 + 6 * ui), cycle_time);//min 105ns+6*UI
		params->dsi.HS_PRPR 	= NS_TO_CYCLE((50 + 4 * ui), cycle_time);//min 40ns+4*UI; max 85ns+6UI
		// HS_PRPR can't be 1.
		if (params->dsi.HS_PRPR < 2)
			params->dsi.HS_PRPR = 2;

		params->dsi.LPX 		= NS_TO_CYCLE(200, cycle_time);//min 50ns
		
		params->dsi.TA_SACK 	= 1;
		params->dsi.TA_GET		= 5 * params->dsi.LPX;//5*LPX
		params->dsi.TA_SURE 	= 3 * params->dsi.LPX / 2;//min LPX; max 2*LPX;
		params->dsi.TA_GO		= 4 * params->dsi.LPX;//4*LPX
	
		params->dsi.CLK_TRAIL	= NS_TO_CYCLE(70, cycle_time);//min 60ns
		// CLK_TRAIL can't be 1.
		if (params->dsi.CLK_TRAIL < 2)
			params->dsi.CLK_TRAIL = 2;
		params->dsi.CLK_ZERO	= NS_TO_CYCLE((300), cycle_time);//min 300ns-38ns
		params->dsi.LPX_WAIT	= 1;
		params->dsi.CONT_DET	= 0;
		
		params->dsi.CLK_HS_PRPR = NS_TO_CYCLE((38 + 95) / 2, cycle_time);//min 38ns; max 95ns
}

static void init_lcm_registers(void)
{
	unsigned int data_array[16];
//*************Enable CMD2 Page1  *******************//
	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000108;
	dsi_set_cmdq(&data_array, 3, 1);
	MDELAY(10);	

//************* AVDD: manual  *******************//
	data_array[0]=0x00043902;
	data_array[1]=0x343434B6;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);	

	data_array[0]=0x00043902;
	data_array[1]=0x090909B0;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);	

	data_array[0]=0x00043902;//AVEE: manual, -6V 
	data_array[1]=0x242424B7;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);	

	data_array[0]=0x00043902;//AVEE voltage, Set AVEE -6V
	data_array[1]=0x090909B1;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);	

	//Power Control for VCL
	data_array[0]=0x34B81500;
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(10);	

	data_array[0]=0x00B21500;
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(10);	

	data_array[0]=0x00043902;//VGH: Clamp Enable
	data_array[1]=0x242424B9;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);

	data_array[0]=0x00043902;
	data_array[1]=0x050505B3;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);

	data_array[0]=0x01BF1500;
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(10);

	data_array[0]=0x00043902;//VGL(LVGL)
	data_array[1]=0x343434BA;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);	

	//VGL_REG(VGLO)
	data_array[0]=0x00043902;
	data_array[1]=0x0B0B0BB5;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);	

	//VGMP/VGSP
	data_array[0]=0x00043902;
	data_array[1]=0x00A300BC;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);	

	data_array[0]=0x00043902;//VGMN/VGSN  
	data_array[1]=0x00A300BD;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);	

	data_array[0]=0x00033902;//VCOM=-0.1
	data_array[1]=0x005000BE;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);	

	data_array[0]=0x00353902;
	data_array[1]=0x003700D1;
	data_array[2]=0x007B0052;
	data_array[3]=0x00B10099;
	data_array[4]=0x01F600D2;
	data_array[5]=0x014E0127;
	data_array[6]=0x02BE018C;
	data_array[7]=0x0248020B;
	data_array[8]=0x027E024A;
	data_array[9]=0x03E102BC;
	data_array[10]=0x03310310;
	data_array[11]=0x0373035A;
	data_array[12]=0x039F0394;
	data_array[13]=0x03B903B3;
	data_array[14]=0x000000C1;
	dsi_set_cmdq(&data_array, 15, 1);
	MDELAY(10);	

	data_array[0]=0x00353902;
	data_array[1]=0x003700D2;
	data_array[2]=0x007B0052;
	data_array[3]=0x00B10099;
	data_array[4]=0x01F600D2;
	data_array[5]=0x014E0127;
	data_array[6]=0x02BE018C;
	data_array[7]=0x0248020B;
	data_array[8]=0x027E024A;
	data_array[9]=0x03E102BC;
	data_array[10]=0x03310310;
	data_array[11]=0x0373035A;
	data_array[12]=0x039F0394;
	data_array[13]=0x03B903B3;
	data_array[14]=0x000000C1;
	dsi_set_cmdq(&data_array, 15, 1);
	MDELAY(10);	
	
	data_array[0]=0x00353902;
	data_array[1]=0x003700D3;
	data_array[2]=0x007B0052;
	data_array[3]=0x00B10099;
	data_array[4]=0x01F600D2;
	data_array[5]=0x014E0127;
	data_array[6]=0x02BE018C;
	data_array[7]=0x0248020B;
	data_array[8]=0x027E024A;
	data_array[9]=0x03E102BC;
	data_array[10]=0x03310310;
	data_array[11]=0x0373035A;
	data_array[12]=0x039F0394;
	data_array[13]=0x03B903B3;
	data_array[14]=0x000000C1;
	dsi_set_cmdq(&data_array, 15, 1);
	MDELAY(10);	

	data_array[0]=0x00353902;
	data_array[1]=0x003700D4;
	data_array[2]=0x007B0052;
	data_array[3]=0x00B10099;
	data_array[4]=0x01F600D2;
	data_array[5]=0x014E0127;
	data_array[6]=0x02BE018C;
	data_array[7]=0x0248020B;
	data_array[8]=0x027E024A;
	data_array[9]=0x03E102BC;
	data_array[10]=0x03310310;
	data_array[11]=0x0373035A;
	data_array[12]=0x039F0394;
	data_array[13]=0x03B903B3;
	data_array[14]=0x000000C1;
	dsi_set_cmdq(&data_array, 15, 1);
	MDELAY(10);	

	data_array[0]=0x00353902;
	data_array[1]=0x003700D5;
	data_array[2]=0x007B0052;
	data_array[3]=0x00B10099;
	data_array[4]=0x01F600D2;
	data_array[5]=0x014E0127;
	data_array[6]=0x02BE018C;
	data_array[7]=0x0248020B;
	data_array[8]=0x027E024A;
	data_array[9]=0x03E102BC;
	data_array[10]=0x03310310;
	data_array[11]=0x0373035A;
	data_array[12]=0x039F0394;
	data_array[13]=0x03B903B3;
	data_array[14]=0x000000C1;
	dsi_set_cmdq(&data_array, 15, 1);
	MDELAY(10);	

	data_array[0]=0x00353902;
	data_array[1]=0x003700D6;
	data_array[2]=0x007B0052;
	data_array[3]=0x00B10099;
	data_array[4]=0x01F600D2;
	data_array[5]=0x014E0127;
	data_array[6]=0x02BE018C;
	data_array[7]=0x0248020B;
	data_array[8]=0x027E024A;
	data_array[9]=0x03E102BC;
	data_array[10]=0x03310310;
	data_array[11]=0x0373035A;
	data_array[12]=0x039F0394;
	data_array[13]=0x03B903B3;
	data_array[14]=0x000000C1;
	dsi_set_cmdq(&data_array, 15, 1);
	MDELAY(10);	
	
// ********************  EABLE CMD2 PAGE 0 **************//

	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000008;
	dsi_set_cmdq(&data_array, 3, 1);
	MDELAY(10);	
	
	data_array[0]=0x00063902;//I/F Setting
	data_array[1]=0x020500B0;
	data_array[2]=0x00000205;
	dsi_set_cmdq(&data_array, 3, 1);
	MDELAY(10);	

	data_array[0]=0x0AB61500;//SDT
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(10);		

	data_array[0]=0x00033902;//Set Gate EQ 
	data_array[1]=0x000000B7;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);

	data_array[0]=0x00053902;//Set Source EQ
	data_array[1]=0x050501B8;
	data_array[2]=0x00000005;
	dsi_set_cmdq(&data_array, 3, 1);
	MDELAY(10);

	data_array[0]=0x00043902;//Inversion: Column inversion (NVT)
	data_array[1]=0x020202BC;//0x000000BC
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);	

	data_array[0]=0x00043902;//BOE's Setting (default)
	data_array[1]=0x000003CC;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(10);	
	
	data_array[0]=0x00063902;//Display Timing
	data_array[1]=0x078401BD;
	data_array[2]=0x00000031;
	dsi_set_cmdq(&data_array, 3, 1);
	MDELAY(10);

	data_array[0]=0x01BA1500;
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(10);

	data_array[0]=0x00053902;
	data_array[1]=0x2555AAF0;
	data_array[2]=0x00000001;
	dsi_set_cmdq(&data_array, 3, 1);
	MDELAY(10);	

/*
	data_array[0]=0x00053902;//Enable Test mode
	data_array[1]=0x2555AAFF;
	data_array[2]=0x00000001;
	dsi_set_cmdq(&data_array, 3, 1);
	MDELAY(10);	
*/

	data_array[0]=0x773A1500;//TE ON 
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(10);	

//	data_array[0] = 0x00351500;// TE ON
//	dsi_set_cmdq(&data_array, 1, 1);
//	MDELAY(10);

	data_array[0] = 0x00110500;		// Sleep Out
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(200);
	
	data_array[0] = 0x00290500;		// Display On
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(10);

	
	data_array[0] = 0x002C0500; 	// Display On
		dsi_set_cmdq(&data_array, 1, 1);
		MDELAY(10);
	
//******************* ENABLE PAGE0 **************//
	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000008;
	dsi_set_cmdq(&data_array, 3, 1);
	MDELAY(10);	
	
/*	data_array[0]=0x02C71500;
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(10);	
	
	data_array[0]=0x00053902;
	data_array[1]=0x000011C9;
	data_array[2]=0x00000000;
	dsi_set_cmdq(&data_array, 3, 1);
	MDELAY(10);
	
	data_array[0]=0x00211500;
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(120);	
*/
	data_array[0] = 0x00351500;// TE ON
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(10);
	data_array[0]= 0x00033902;
	data_array[1]= 0x0000E8B1;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(50);

	data_array[0]= 0x00023902;
	data_array[1]= 0x0051;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(50);

	data_array[0]= 0x00023902;
	data_array[1]= 0x2453;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(50);

	data_array[0]= 0x00023902;
	data_array[1]= 0x0155;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(50);

	data_array[0]= 0x00023902;
	data_array[1]= 0x705e;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(50);

	data_array[0]= 0x00033902;
	data_array[1]= 0x000301E0;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(50);

}

static void lcm_init(void)
{
    SET_RESET_PIN(1);
    MDELAY(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(150);

    init_lcm_registers();
}


static void lcm_suspend(void)
{
	//push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	unsigned int data_array[16];
	
	data_array[0]=0x00280500;
	dsi_set_cmdq(&data_array, 1, 1);
	//MDELAY(50);
	
	data_array[0]=0x00100500;
	dsi_set_cmdq(&data_array, 1, 1);	
	MDELAY(150);
}


static void lcm_resume(void)
{
	//push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
	unsigned int data_array[16];

	data_array[0]=0x00110500;
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(150);
	
	data_array[0]=0x00290500;
	dsi_set_cmdq(&data_array, 1, 1);	

}


static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}


void lcm_setbacklight(unsigned int level)
{
	unsigned int data_array[16];

#if defined(BUILD_UBOOT)
        printf("%s,  \n", __func__);
#endif

	if(level > 255) 
	    level = 255;

	data_array[0]= 0x00023902;
	data_array[1] =(0x51|(level<<8));
	dsi_set_cmdq(&data_array, 2, 1);
}


void lcm_setpwm(unsigned int divider)
{
	// TBD
}


unsigned int lcm_getpwm(unsigned int divider)
{
	// ref freq = 15MHz, B0h setting 0x80, so 80.6% * freq is pwm_clk;
	// pwm_clk / 255 / 2(lcm_setpwm() 6th params) = pwm_duration = 23706
	unsigned int pwm_clk = 23706 / (1<<divider);	
	return pwm_clk;
}

static unsigned int lcm_compare_id()
{
	unsigned int id = 0, id2 = 0;
	unsigned char buffer[2];

	unsigned int data_array[16];
	
	SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);	

/*	
	data_array[0] = 0x00110500;		// Sleep Out
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(120);
*/
		
//*************Enable CMD2 Page1  *******************//
	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000108;
	dsi_set_cmdq(&data_array, 3, 1);
	MDELAY(10); 

	data_array[0] = 0x00023700;// read id return two byte,version and id
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10); 
	
	read_reg_v2(0xC5, buffer, 2);
	id = buffer[0]; //we only need ID
	id2= buffer[1]; //we test buffer 1

        return (LCM_ID == id)?1:0;
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------

LCM_DRIVER nt35510_hvga_lcm_drv = 
{
	.name			= "nt35510_hvga",
        .set_util_funcs = lcm_set_util_funcs,
        .get_params     = lcm_get_params,
        .init           = lcm_init,
        .suspend        = lcm_suspend,
        .resume         = lcm_resume,
        .set_backlight	= lcm_setbacklight,
		//.set_pwm        = lcm_setpwm,
		//.get_pwm        = lcm_getpwm,
	.compare_id    = lcm_compare_id,
        .update         = lcm_update

};

