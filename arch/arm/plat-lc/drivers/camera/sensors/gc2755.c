#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <plat/comip-v4l2.h>
#include <plat/camera.h>
#include "gc2755.h"

//#define GC2755_DEBUG_FUNC
//#define GC2755_ALL_SIZES 

#define GC2755_CHIP_ID_H	(0x26)
#define GC2755_CHIP_ID_L	(0x55)

#define VGA_WIDTH		640
#define VGA_HEIGHT		480
#define SXGA_WIDTH		1280 
#define SXGA_HEIGHT		960
#define MAX_WIDTH		1920
#define MAX_HEIGHT		1080
#define MAX_PREVIEW_WIDTH     MAX_WIDTH
#define MAX_PREVIEW_HEIGHT  MAX_HEIGHT


#define SETTING_LOAD
#define GC2755_REG_END		0xff
#define GC2755_REG_DELAY	0xff

#define V4L2_I2C_ADDR_8BIT	(0x0001)

static int sensor_get_aecgc_win_setting(int width, int height,int meter_mode, void **vals);

struct gc2755_format_struct;


struct gc2755_info {
	struct v4l2_subdev sd;
	struct gc2755_format_struct *fmt;
	struct gc2755_win_size *win;
	
//	int binning;
	int stream_on;
//	int frame_rate;

	unsigned short vts_address1;
	unsigned short vts_address2;
	int frame_rate;
	int binning;

};

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};

static int gc2755_s_stream(struct v4l2_subdev *sd, int enable);

static struct regval_list gc2755_init_regs[] = {
	/////////////////////////////////////////////////////
	//////////////////////  SYS	//////////////////////
	/////////////////////////////////////////////////////
	{0xfe,0x80},
	{0xfe,0x80},
	{0xfe,0x80},
	{0xf6,0x00}, //up down
	{0xfc,0x06},
	{0xfd,0x00},
  
	{0xf7,0x31}, //pll enable 
	{0xf8,0x07}, //Pll mode 2//06

	{0xf9,0x0e}, //[0] pll enable
	{0xfa,0x00}, //div
	{0xfe,0x00},

	//////////////////////////////////////////////////
	////////////////ANALOG & CISCTL////////////////
	//////////////////////////////////////////////////

	{0x03,0x00},
	{0x04,0x00}, 
	{0x05,0x02},//03 //02 //HB_H
	{0x06,0x66},//12 //e5 //HB_L
	{0x07,0x00}, //0x00 //VB_H
	{0x08,0x58},//49 //17//VB_L

	{0x0a,0x00}, //row start
	{0x0c,0x04}, //col start
	{0x0d,0x04},
	{0x0e,0x48},
	{0x0f,0x07}, 
	{0x10,0x90}, //Window setting 1936x1096
	{0x11,0x00},
	{0x12,0x0e},
	{0x13,0x11},
	{0x14,0x01},
	{0x17,0x15},//15
	{0x19,0x08},
	{0x1b,0x4b}, 
	{0x1c,0x11}, 
	{0x1d,0x10}, //double reset
	{0x1e,0xcc}, //col_r/rowclk_mode/rsthigh_en FPN
	{0x1f,0xc9}, //rsgl_s_mode/vpix_s_mode 
	{0x20,0x61},
	{0x21,0x20}, //rsg
	{0x22,0xd0},
	{0x23,0x51},
	{0x24,0x19}, 
	{0x27,0x20}, 
	{0x28,0x00},
	{0x2b,0x80}, //NBD_s_dode
	{0x2c,0x38}, 
	{0x2e,0x1f}, 
	{0x2f,0x14}, 
	{0x30,0x00},
	{0x31,0x01},
	{0x32,0x02},
	{0x33,0x03},
	{0x34,0x07},
	{0x35,0x0b},
	{0x36,0x0f},

	//////////////////////////////////////////////////
	//////////////////	gain	//////////////////
	//////////////////////////////////////////////////
	{0xb0,0x56},
	{0xb1,0x01}, 
	{0xb2,0x00}, 
	{0xb3,0x40},
	{0xb4,0x40},
	{0xb5,0x40},
	{0xb6,0x00},
	//////////////////////////////////////////////////
	/////////////////////    crop	//////////////////
	//////////////////////////////////////////////////
	{0x92,0x05},//06
	{0x94,0x04},//05
	{0x95,0x04},
	{0x96,0x38},
	{0x97,0x07},
	{0x98,0x80}, //out window set 1920x1080

	//////////////////////////////////////////////////
	//////////////////////    BLK    //////////////////
	//////////////////////////////////////////////////
	{0x18,0x12},
	{0x1a,0x01},
	{0x40,0x42},
	{0x41,0x00},
	{0x4e,0x3c}, //BLK select
	{0x4f,0x00}, 
	{0x5e,0x00}, //offset ratio
	{0x66,0x20}, //dark ratio

	{0x6a,0x00}, //manual offset
	{0x6b,0x00},
	{0x6c,0x00},
	{0x6d,0x00},
	{0x6e,0x00},
	{0x6f,0x00},
	{0x70,0x00},
	{0x71,0x00},

	//////////////////////////////////////////////////
	//////////////////	 Dark sun//////////////////
	//////////////////////////////////////////////////
	{0x87,0x03}, 
	{0xe5,0x25},//dark sun en/extend mode
	{0xe6,0xb0},	
	{0xe7,0xf3},
	{0xe8,0xff},//clamp
	{0xe9,0x0f},

	//////////////////////////////////////////////////
	////////////////	MIPI	//////////////////
	//////////////////////////////////////////////////

	{0xfe,0x03},
	{0x01,0x87},
	{0x02,0x00},
	{0x03,0x10},
	{0x04,0x01},
	{0x05,0x00},
	{0x06,0xa2},
	{0x10,0x81},
	{0x11,0x2b},
	{0x12,0x60},
	{0x13,0x09},
	{0x15,0x60},//62 
	{0x20,0x40},
	{0x21,0x10},
	{0x22,0x02},
	{0x23,0x10},//20
	{0x24,0x10},//02
	{0x25,0x10},
	{0x26,0x04},
	{0x27,0x06},
	{0x29,0x02},
	{0x2a,0x08},
	{0x2b,0x04},
	{0xfe,0x00},


	{GC2755_REG_END, 0x00}, /* END MARKER */
};

static struct regval_list gc2755_win_vga[] = {
	{GC2755_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list gc2755_win_SXGA[] = {
	{GC2755_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list gc2755_win_2M[] = {
    {GC2755_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list gc2755_stream_on[] = {
	{0xfe, 0x03},
	{0x10, 0x91},  // 93 line_sync_mode
    {0xfe, 0x00},

    {GC2755_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list gc2755_stream_off[] = {
	{0xfe, 0x03},
	{0x10, 0x81},  // 93 line_sync_mode       
    {0xfe, 0x00},

    {GC2755_REG_END, 0x00},     /* END MARKER */
};


static int sensor_get_exposure(void **vals, int val)
{
	switch (val) {
	case EXPOSURE_L6:
		*vals = isp_exposure_l6;
		return ARRAY_SIZE(isp_exposure_l6);
	case EXPOSURE_L5:
		*vals = isp_exposure_l5;
		return ARRAY_SIZE(isp_exposure_l5);
	case EXPOSURE_L4:
		*vals = isp_exposure_l4;
		return ARRAY_SIZE(isp_exposure_l4);
	case EXPOSURE_L3:
		*vals = isp_exposure_l3;
		return ARRAY_SIZE(isp_exposure_l3);
	case EXPOSURE_L2:
		*vals = isp_exposure_l2;
		return ARRAY_SIZE(isp_exposure_l2);
	case EXPOSURE_L1:
		*vals = isp_exposure_l1;
		return ARRAY_SIZE(isp_exposure_l1);
	case EXPOSURE_H0:
		*vals = isp_exposure_h0;
		return ARRAY_SIZE(isp_exposure_h0);
	case EXPOSURE_H1:
		*vals = isp_exposure_h1;
		return ARRAY_SIZE(isp_exposure_h1);
	case EXPOSURE_H2:
		*vals = isp_exposure_h2;
		return ARRAY_SIZE(isp_exposure_h2);
	case EXPOSURE_H3:
		*vals = isp_exposure_h3;
		return ARRAY_SIZE(isp_exposure_h3);
	case EXPOSURE_H4:
		*vals = isp_exposure_h4;
		return ARRAY_SIZE(isp_exposure_h4);
	case EXPOSURE_H5:
		*vals = isp_exposure_h5;
		return ARRAY_SIZE(isp_exposure_h5);
	case EXPOSURE_H6:
		*vals = isp_exposure_h6;
		return ARRAY_SIZE(isp_exposure_h6);
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_get_iso(void **vals, int val)
{
	switch(val) {
	case ISO_100:
		*vals = isp_iso_100;
		return ARRAY_SIZE(isp_iso_100);
	case ISO_200:
		*vals = isp_iso_200;
		return ARRAY_SIZE(isp_iso_200);
	case ISO_400:
		*vals = isp_iso_400;
		return ARRAY_SIZE(isp_iso_400);
	case ISO_800:
		*vals = isp_iso_800;
		return ARRAY_SIZE(isp_iso_800);
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_get_contrast(void **vals, int val)
{
	switch (val) {
	case CONTRAST_L4:
		*vals = isp_contrast_l4;
		return ARRAY_SIZE(isp_contrast_l4);
	case CONTRAST_L3:
		*vals = isp_contrast_l3;
		return ARRAY_SIZE(isp_contrast_l3);
	case CONTRAST_L2:
		*vals = isp_contrast_l2;
		return ARRAY_SIZE(isp_contrast_l2);
	case CONTRAST_L1:
		*vals = isp_contrast_l1;
		return ARRAY_SIZE(isp_contrast_l1);
	case CONTRAST_H0:
		*vals = isp_contrast_h0;
		return ARRAY_SIZE(isp_contrast_h0);
	case CONTRAST_H1:
		*vals = isp_contrast_h1;
		return ARRAY_SIZE(isp_contrast_h1);
	case CONTRAST_H2:
		*vals = isp_contrast_h2;
		return ARRAY_SIZE(isp_contrast_h2);
	case CONTRAST_H3:
		*vals = isp_contrast_h3;
		return ARRAY_SIZE(isp_contrast_h3);
	case CONTRAST_H4:
		*vals = isp_contrast_h4;
		return ARRAY_SIZE(isp_contrast_h4);
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_get_saturation(void **vals, int val)
{
	switch (val) {
	case SATURATION_L2:
		*vals = isp_saturation_l2;
		return ARRAY_SIZE(isp_saturation_l2);
	case SATURATION_L1:
		*vals = isp_saturation_l1;
		return ARRAY_SIZE(isp_saturation_l1);
	case SATURATION_H0:
		*vals = isp_saturation_h0;
		return ARRAY_SIZE(isp_saturation_h0);
	case SATURATION_H1:
		*vals = isp_saturation_h1;
		return ARRAY_SIZE(isp_saturation_h1);
	case SATURATION_H2:
		*vals = isp_saturation_h2;
		return ARRAY_SIZE(isp_saturation_h2);
	default:
		return -EINVAL;
	}

	return 0;
}


static int sensor_get_white_balance(void **vals, int val)
{
	switch (val) {
	case WHITE_BALANCE_AUTO:
		printk("[2755]:sensor_get_white_balance>>>WHITE_BALANCE_AUTO\n");
		*vals = isp_white_balance_auto;
		return ARRAY_SIZE(isp_white_balance_auto);
	case WHITE_BALANCE_INCANDESCENT:
		*vals = isp_white_balance_incandescent;
		return ARRAY_SIZE(isp_white_balance_incandescent);
	case WHITE_BALANCE_FLUORESCENT:
		*vals = isp_white_balance_fluorescent;
		return ARRAY_SIZE(isp_white_balance_fluorescent);
	case WHITE_BALANCE_WARM_FLUORESCENT:
		*vals = isp_white_balance_warm_fluorescent;
		return ARRAY_SIZE(isp_white_balance_warm_fluorescent);
	case WHITE_BALANCE_DAYLIGHT:
		*vals = isp_white_balance_daylight;
		return ARRAY_SIZE(isp_white_balance_daylight);
	case WHITE_BALANCE_CLOUDY_DAYLIGHT:
		*vals = isp_white_balance_cloudy_daylight;
		return ARRAY_SIZE(isp_white_balance_cloudy_daylight);
	case WHITE_BALANCE_TWILIGHT:
		*vals = isp_white_balance_twilight;
		return ARRAY_SIZE(isp_white_balance_twilight);
	case WHITE_BALANCE_SHADE:
		*vals = isp_white_balance_shade;
		return ARRAY_SIZE(isp_white_balance_shade);
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_get_brightness(void **vals, int val)
{
	switch (val) {
	case BRIGHTNESS_L3:
		*vals = isp_brightness_l3;
		return ARRAY_SIZE(isp_brightness_l3);
	case BRIGHTNESS_L2:
		*vals = isp_brightness_l2;
		return ARRAY_SIZE(isp_brightness_l2);
	case BRIGHTNESS_L1:
		*vals = isp_brightness_l1;
		return ARRAY_SIZE(isp_brightness_l1);
	case BRIGHTNESS_H0:
		*vals = isp_brightness_h0;
		return ARRAY_SIZE(isp_brightness_h0);
	case BRIGHTNESS_H1:
		*vals = isp_brightness_h1;
		return ARRAY_SIZE(isp_brightness_h1);
	case BRIGHTNESS_H2:
		*vals = isp_brightness_h2;
		return ARRAY_SIZE(isp_brightness_h2);
	case BRIGHTNESS_H3:
		*vals = isp_brightness_h3;
		return ARRAY_SIZE(isp_brightness_h3);
	default:
		return -EINVAL;
	}

	return 0;
}
static int sensor_get_sharpness(void **vals, int val)
{
	switch (val) {
	case SHARPNESS_L2:
		*vals = isp_sharpness_l2;
		return ARRAY_SIZE(isp_sharpness_l2);
	case SHARPNESS_L1:
		*vals = isp_sharpness_l1;
		return ARRAY_SIZE(isp_sharpness_l1);
	case SHARPNESS_H0:
		*vals = isp_sharpness_h0;
		return ARRAY_SIZE(isp_sharpness_h0);
	case SHARPNESS_H1:
		*vals = isp_sharpness_h1;
		return ARRAY_SIZE(isp_sharpness_h1);
	case SHARPNESS_H2:
		*vals = isp_sharpness_h2;
		return ARRAY_SIZE(isp_sharpness_h2);
	default:
		return -EINVAL;
	}

	return 0;
}


static struct isp_effect_ops sensor_effect_ops = {
	.get_exposure = sensor_get_exposure,
	.get_iso = sensor_get_iso,
	.get_contrast = sensor_get_contrast,
	.get_saturation = sensor_get_saturation,
	.get_white_balance = sensor_get_white_balance,
	.get_brightness = sensor_get_brightness,
	.get_sharpness = sensor_get_sharpness,
	.get_aecgc_win_setting = sensor_get_aecgc_win_setting,
};
static int gc2755_read(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		return ret;

	*value = (unsigned char)ret;

	return 0;
}

static int gc2755_write(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);
	if (ret < 0)
		return ret;

	return 0;
}

static int gc2755_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;

	while (vals->reg_num != GC2755_REG_END) {
		if (vals->reg_num == GC2755_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = gc2755_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

#ifdef GC2755_DEBUG_FUNC
static int gc2755_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char tmpv;

	while (vals->reg_num != GC2755_REG_END) {
		if (vals->reg_num == GC2755_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value},
			else
				mdelay(vals->value},
		} else {
			ret = gc2755_read(sd, vals->reg_num, &tmpv);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}
#endif

static int gc2755_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int gc2755_init(struct v4l2_subdev *sd, u32 val)
{
	struct gc2755_info *info = container_of(sd, struct gc2755_info, sd);
	int ret=0;

	info->fmt = NULL;
	info->win = NULL;

	//this var is set according to sensor setting,can only be set 1,2 or 4
	//1 means no binning,2 means 2x2 binning,4 means 4x4 binning
	info->binning = 1;

	ret=gc2755_write_array(sd, gc2755_init_regs);

	msleep(100);

	if (ret < 0)
		return ret;

	return ret;
}

static int gc2755_detect(struct v4l2_subdev *sd)
{
	unsigned char v;
	int ret;

	ret = gc2755_read(sd, 0xf0, &v);
	if (ret < 0)
		return ret;
	printk("CHIP_ID_H=0x%x ", v);
	if (v != GC2755_CHIP_ID_H)
		return -ENODEV;

	ret = gc2755_read(sd, 0xf1, &v);
	if (ret < 0)
		return ret;
	
	printk("CHIP_ID_L=0x%x \n", v);
	if (v != GC2755_CHIP_ID_L)
		return -ENODEV;

	return 0;
}

static struct gc2755_format_struct {
	enum v4l2_mbus_pixelcode mbus_code;
	enum v4l2_colorspace colorspace;
	struct regval_list *regs;
} gc2755_formats[] = {
	{
		.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= NULL,
	},
};
#define N_GC2755_FMTS ARRAY_SIZE(gc2755_formats)

static struct gc2755_win_size
#if 0
{
		int width;
		int height;
		int vts;
		int framerate;
		int framerate_div;
		unsigned short max_gain_dyn_frm;
		unsigned short min_gain_dyn_frm;
		unsigned short max_gain_fix_frm;
		unsigned short min_gain_fix_frm;
		//unsigned char  vts_gain_ctrl_1;
		//unsigned char  vts_gain_ctrl_2;
		//unsigned char  vts_gain_ctrl_3;
		//unsigned short gain_ctrl_1;
		//unsigned short gain_ctrl_2;
		//unsigned short gain_ctrl_3;
		//unsigned short spot_meter_win_width;
		//unsigned short spot_meter_win_height;
		struct regval_list *regs; /* Regs to tweak */
		struct isp_regb_vals *regs_aecgc_win_matrix;
		struct isp_regb_vals *regs_aecgc_win_center;
		struct isp_regb_vals *regs_aecgc_win_central_weight;
	}
#endif
{
	int	width;
	int	height;
	int	vts;
	int	framerate;
	int	framerate_div;
	unsigned short max_gain_dyn_frm;
	unsigned short min_gain_dyn_frm;
	unsigned short max_gain_fix_frm;
	unsigned short min_gain_fix_frm;
	unsigned char  vts_gain_ctrl_1;
	unsigned char  vts_gain_ctrl_2;
	unsigned char  vts_gain_ctrl_3;
	unsigned short gain_ctrl_1;
	unsigned short gain_ctrl_2;
	unsigned short gain_ctrl_3;
	unsigned short spot_meter_win_width;
	unsigned short spot_meter_win_height;
	struct regval_list *regs; /* Regs to tweak */
	struct isp_regb_vals *regs_aecgc_win_matrix;
	struct isp_regb_vals *regs_aecgc_win_center;
	struct isp_regb_vals *regs_aecgc_win_central_weight;
}gc2755_win_sizes[] = {

	/* 1920*1080 */
	{
		.width		= MAX_WIDTH,
		.height		= MAX_HEIGHT,
		.vts		= 0x4B0,//0x7b6
		.framerate	= 271,//10,
		.framerate_div	  = 10,
		.max_gain_dyn_frm = 0xa0,
		.min_gain_dyn_frm = 0x10,
		.max_gain_fix_frm = 0xa0,
		.min_gain_fix_frm = 0x10,
		.vts_gain_ctrl_1  = 0x41,
		.vts_gain_ctrl_2  = 0x61,
		.vts_gain_ctrl_3  = 0x61,
		.gain_ctrl_1	  = 0x20,
		.gain_ctrl_2	  = 0x40,
		.gain_ctrl_3	  = 0xff,
		.spot_meter_win_width  = 400,
		.spot_meter_win_height = 300,
		.regs 			  = gc2755_win_2M,
		.regs_aecgc_win_matrix			= isp_aecgc_win_2M_matrix,
		.regs_aecgc_win_center			= isp_aecgc_win_2M_center,
		.regs_aecgc_win_central_weight	= isp_aecgc_win_2M_central_weight,
		//
		//.regs 		= gc2755_win_2M,
		//.regs_aecgc_win_matrix	  		= isp_aecgc_win_2M_matrix,
		//.regs_aecgc_win_center    		= isp_aecgc_win_2M_center,
		//.regs_aecgc_win_central_weight  = isp_aecgc_win_2M_central_weight,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(gc2755_win_sizes))

static int gc2755_set_aecgc_parm(struct v4l2_subdev *sd, struct v4l2_aecgc_parm *parm)
{
	u16 gain, exp, vb, temp,v,v1;
 
	printk(">>>>parm->gain = 0x%x,parm->exp = 0x%x\n",parm->gain,parm->exp);
    gain = parm->gain* 64 / 0x10;

	exp = parm->exp;
	vb = parm->vts - MAX_PREVIEW_HEIGHT;
   
	//exp
	if(exp > 16383) exp = 16383;
//	if(exp == 1312) exp = 1344;
	if(exp < 8) exp = 8;
	
	gc2755_write(sd, 0x04, exp & 0xff);
	gc2755_write(sd, 0x03, (exp >> 8) & 0x3f);

	//gain
		if(gain < 0x40) {
			gain = 0x40;
		}
	if(gain > 512)
		gain = 512;
	if((64 <= gain) &&(gain < 86) ){
		gc2755_write(sd, 0xb6, 0x00);
		temp = gain;
		gc2755_write(sd, 0xb1, temp>>6);
		gc2755_write(sd, 0xb2, (temp<<2)&0xfc);
	}
	else if((86 <= gain) &&(gain < 117) ){
		gc2755_write(sd, 0xb6, 0x01);
		temp = 64*gain/86;
		gc2755_write(sd, 0xb1, temp>>6);
		gc2755_write(sd, 0xb2, (temp<<2)&0xfc);
	}
	else if((117 <= gain) &&(gain < 159)) {
		gc2755_write(sd, 0xb6, 0x02);
		temp = 64*gain/117;
		gc2755_write(sd, 0xb1, temp>>6);
		gc2755_write(sd, 0xb2, (temp<<2)&0xfc);
	}
	else  {
		gc2755_write(sd, 0xb6, 0x03);
		temp = 64*gain/159;
		gc2755_write(sd, 0xb1, temp>>6);
		gc2755_write(sd, 0xb2, (temp<<2)&0xfc);
	}
	return 0;
}

static int gc2755_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned index,
					enum v4l2_mbus_pixelcode *code)
{
	if (index >= N_GC2755_FMTS)
		return -EINVAL;


	*code = gc2755_formats[index].mbus_code;
	return 0;
}

static int gc2755_try_fmt_internal(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt,
		struct gc2755_format_struct **ret_fmt,
		struct gc2755_win_size **ret_wsize)
{
	int index;
	struct gc2755_win_size *wsize;

	for (index = 0; index < N_GC2755_FMTS; index++)
		if (gc2755_formats[index].mbus_code == fmt->code)
			break;
	if (index >= N_GC2755_FMTS) {
		/* default to first format */
		index = 0;
		fmt->code = gc2755_formats[0].mbus_code;
	}
	if (ret_fmt != NULL)
		*ret_fmt = gc2755_formats + index;

	fmt->field = V4L2_FIELD_NONE;


	for (wsize = gc2755_win_sizes; wsize < gc2755_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (fmt->width <= wsize->width && fmt->height <= wsize->height)
			{
			break;
            }
	if (wsize >= gc2755_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the biggest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;

	fmt->width = wsize->width;
	fmt->height = wsize->height;

	fmt->colorspace = gc2755_formats[index].colorspace;
	return 0;
}

static int gc2755_try_mbus_fmt(struct v4l2_subdev *sd,
			    struct v4l2_mbus_framefmt *fmt)
{
	return gc2755_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int gc2755_fill_i2c_regs(struct regval_list *sensor_regs, struct v4l2_fmt_data *data)
{
	while (sensor_regs->reg_num != GC2755_REG_END) {
		data->reg[data->reg_num].addr = sensor_regs->reg_num;
		data->reg[data->reg_num].data = sensor_regs->value;
		data->reg_num++;
		sensor_regs++;
	}

	return 0;
}

static int gc2755_s_mbus_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *fmt)
{
	struct gc2755_info *info = container_of(sd, struct gc2755_info, sd);
	struct v4l2_fmt_data *data = v4l2_get_fmt_data(fmt);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct gc2755_format_struct *fmt_s;
	struct gc2755_win_size *wsize;
	struct v4l2_aecgc_parm aecgc_parm;
	int ret;
	
	static int last_frame_rate = 0;
	int framerate = 0;

	ret = gc2755_try_fmt_internal(sd, fmt, &fmt_s, &wsize);
	if (ret)
		return ret;

	if ((info->fmt != fmt_s) && fmt_s->regs) {
		ret = gc2755_write_array(sd, fmt_s->regs);
		if (ret)
			return ret;
	}


		if ((info->win != wsize)&& wsize->regs) {
			printk("######wsize->width = %d,wsize->height= %d#####\n",wsize->width,wsize->height);
			memset(data, 0, sizeof(*data));
			data->flags = V4L2_I2C_ADDR_8BIT;
			data->slave_addr = client->addr;
			data->vts = wsize->vts;
			data->mipi_clk = 312; //282; //TBD/* Mbps. */
			data->max_gain_dyn_frm = wsize->max_gain_dyn_frm;
			data->min_gain_dyn_frm = wsize->min_gain_dyn_frm;
			data->max_gain_fix_frm = wsize->max_gain_fix_frm;
			data->min_gain_fix_frm = wsize->min_gain_fix_frm;
			data->vts_gain_ctrl_1 = wsize->vts_gain_ctrl_1;
			data->vts_gain_ctrl_2 = wsize->vts_gain_ctrl_2;
			data->vts_gain_ctrl_3 = wsize->vts_gain_ctrl_3;
			data->gain_ctrl_1 = wsize->gain_ctrl_1;
			data->gain_ctrl_2 = wsize->gain_ctrl_2;
			data->gain_ctrl_3 = wsize->gain_ctrl_3;
			data->spot_meter_win_width = wsize->spot_meter_win_width;
			data->spot_meter_win_height = wsize->spot_meter_win_height;
			data->framerate = wsize->framerate;
			data->framerate_div = wsize->framerate_div;
			gc2755_fill_i2c_regs(wsize->regs, data);
			//ov8865_set_exp_ratio(sd, wsize, data);
		}
	
		info->fmt = fmt_s;
		info->win = wsize;
	
		return 0;
	}

static int gc2755_set_framerate(struct v4l2_subdev *sd, int framerate)
{
	struct gc2755_info *info = container_of(sd, struct gc2755_info, sd);
	if (framerate < FRAME_RATE_AUTO)
		framerate = FRAME_RATE_AUTO;
	else if (framerate > 30)
		framerate = 30;
	info->frame_rate = framerate;
	return 0;
}

static int gc2755_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct gc2755_info *info = container_of(sd, struct gc2755_info, sd);
	int ret = 0;

	if (enable) {
		ret = gc2755_write_array(sd, gc2755_stream_on);
		info->stream_on = 1;
	}
	else {
		ret = gc2755_write_array(sd, gc2755_stream_off);
		info->stream_on = 0;
	}

	#ifdef GC2755_DEBUG_FUNC
	gc2755_read_array(sd,gc2755_init_regs );//debug only
	#endif

	return ret;
}

static int gc2755_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	a->c.left	= 0;
	a->c.top	= 0;
	a->c.width	= MAX_WIDTH;
	a->c.height	= MAX_HEIGHT;
	a->type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;

	return 0;
}

static int gc2755_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
{
	a->bounds.left			= 0;
	a->bounds.top			= 0;
	//snapshot max size
	a->bounds.width			= MAX_WIDTH;
	a->bounds.height		= MAX_HEIGHT;
	//snapshot max size end
	a->defrect.left			= 0;
	a->defrect.top			= 0;
	//preview max size
	a->defrect.width		= MAX_PREVIEW_WIDTH;
	a->defrect.height		= MAX_PREVIEW_HEIGHT;
	//preview max size end
	a->type				= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	a->pixelaspect.numerator	= 1;
	a->pixelaspect.denominator	= 1;

	return 0;
}

static int gc2755_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int gc2755_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int gc2755_frame_rates[] = { 30, 15, 10, 5, 1 };

static int gc2755_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *interval)
{
	if (interval->index >= ARRAY_SIZE(gc2755_frame_rates))
		return -EINVAL;
	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator = gc2755_frame_rates[interval->index];
	return 0;
}

static int gc2755_enum_framesizes(struct v4l2_subdev *sd,
		struct v4l2_frmsizeenum *fsize)
{
	int i;
	int num_valid = -1;
	__u32 index = fsize->index;

	/*
	 * If a minimum width/height was requested, filter out the capture
	 * windows that fall outside that.
	 */
	for (i = 0; i < N_WIN_SIZES; i++) {
		struct gc2755_win_size *win = &gc2755_win_sizes[index];
		if (index == ++num_valid) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = win->width;
			fsize->discrete.height = win->height;
			return 0;
		}
	}

	return -EINVAL;
}

static int gc2755_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	return 0;
}

static int gc2755_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct v4l2_isp_setting isp_setting;
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_ISP_SETTING:
		isp_setting.setting = (struct v4l2_isp_regval *)&gc2755_isp_setting;
		isp_setting.size = ARRAY_SIZE(gc2755_isp_setting);
		memcpy((void *)ctrl->value, (void *)&isp_setting, sizeof(isp_setting));
		break;
	case V4L2_CID_ISP_PARM:
		ctrl->value = (int)&gc2755_isp_parm;
		break;
	case V4L2_CID_GET_EFFECT_FUNC:
		ctrl->value = (int)&sensor_effect_ops;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int gc2755_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int ret = 0;
	struct v4l2_aecgc_parm *parm = NULL;

	switch (ctrl->id) {
	case V4L2_CID_AECGC_PARM:
		memcpy(&parm, &(ctrl->value), sizeof(struct v4l2_aecgc_parm*));
		ret = gc2755_set_aecgc_parm(sd, parm);
		break;
	case V4L2_CID_FRAME_RATE:
		ret = gc2755_set_framerate(sd, ctrl->value);
		break;
	default:
		ret = -ENOIOCTLCMD;
	}

	return ret;
}


static int gc2755_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_GC2755, 0);
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int gc2755_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = gc2755_read(sd, reg->reg & 0xff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int gc2755_s_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	gc2755_write(sd, reg->reg & 0xff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops gc2755_core_ops = {
	.g_chip_ident = gc2755_g_chip_ident,
	.g_ctrl = gc2755_g_ctrl,
	.s_ctrl = gc2755_s_ctrl,
	.queryctrl = gc2755_queryctrl,
	.reset = gc2755_reset,
	.init = gc2755_init,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = gc2755_g_register,
	.s_register = gc2755_s_register,
#endif
};

static const struct v4l2_subdev_video_ops gc2755_video_ops = {
	.enum_mbus_fmt = gc2755_enum_mbus_fmt,
	.try_mbus_fmt = gc2755_try_mbus_fmt,
	.s_mbus_fmt = gc2755_s_mbus_fmt,
	.s_stream = gc2755_s_stream,
	.cropcap = gc2755_cropcap,
	.g_crop	= gc2755_g_crop,
	.s_parm = gc2755_s_parm,
	.g_parm = gc2755_g_parm,
	.enum_frameintervals = gc2755_enum_frameintervals,
	.enum_framesizes = gc2755_enum_framesizes,
};

static const struct v4l2_subdev_ops gc2755_ops = {
	.core = &gc2755_core_ops,
	.video = &gc2755_video_ops,
};

static int gc2755_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct gc2755_info *info;
	int ret;

	info = kzalloc(sizeof(struct gc2755_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &gc2755_ops);

	/* Make sure it's an gc2755 */
	ret = gc2755_detect(sd);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an gc2755 chip.\n",
			client->addr, client->adapter->name);
		kfree(info);
		return ret;
	}
	v4l_info(client, "gc2755 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);

	return 0;
}
#if 1
static int sensor_get_aecgc_win_setting(int width, int height,int meter_mode, void **vals)
{
	int i = 0;
	int ret = 0;

	for(i = 0; i < N_WIN_SIZES; i++) {
		if (width == gc2755_win_sizes[i].width && height == gc2755_win_sizes[i].height) {
			switch (meter_mode){
				case METER_MODE_MATRIX:
					*vals = gc2755_win_sizes[i].regs_aecgc_win_matrix;
					break;
				case METER_MODE_CENTER:
					*vals = gc2755_win_sizes[i].regs_aecgc_win_center;
					break;
				case METER_MODE_CENTRAL_WEIGHT:
					*vals = gc2755_win_sizes[i].regs_aecgc_win_central_weight;
					break;
				default:
					break;
			}
			break;
		}
	}
	return ret;
}
#endif
static int gc2755_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct gc2755_info *info = container_of(sd, struct gc2755_info, sd);

	v4l2_device_unregister_subdev(sd);
	kfree(info);
	return 0;
}

static const struct i2c_device_id gc2755_id[] = {
	{ "gc2755", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, gc2755_id);

static struct i2c_driver gc2755_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "gc2755",
	},
	.probe		= gc2755_probe,
	.remove		= gc2755_remove,
	.id_table	= gc2755_id,
};

static __init int init_gc2755(void)
{
	return i2c_add_driver(&gc2755_driver);
}

static __exit void exit_gc2755(void)
{
	i2c_del_driver(&gc2755_driver);
}

fs_initcall(init_gc2755);
module_exit(exit_gc2755);

MODULE_DESCRIPTION("A low-level driver for gcoreinc gc2755 sensors");
MODULE_LICENSE("GPL");
