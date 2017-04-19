#ifndef __COMIPFB_DEV_H_
#define __COMIPFB_DEV_H_

#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include "mipi_interface.h"

#define ROW_LINE 64

#define COMIPFB_HSYNC_HIGH_ACT		(0x03)
#define COMIPFB_VSYNC_HIGH_ACT		(0x04)

#define MIPI_VIDEO_MODE 		(0x05)
#define MIPI_COMMAND_MODE 		(0x06)

#define ARRAY_AND_SIZE(x)		(u8 *)(x), ARRAY_SIZE(x)

/*display prefer and ce*/
#define PREFER_INIT			(0)
#define PREFER_WARM			(1)
#define PREFER_NATURE			(2)
#define PREFER_COOL			(3)

#define CE_BRIGHT 			(12)
#define CE_VELVIA			(11)
#define CE_STANDARD			(10)


enum {
	DCS_CMD = 2,
	GEN_CMD,
	SW_PACK0,
	SW_PACK1,
	SW_PACK2,
	LW_PACK,
	SHUTDOWN_SW_PACK,
};

/*Device flags*/
#define	PREFER_CMD_SEND_MONOLITHIC	(0x00000001)
#define	CE_CMD_SEND_MONOLITHIC		(0x00000002)

#define RESUME_WITH_PREFER		(0x00000010)
#define RESUME_WITH_CE			(0x00000020)


/**
 * Video stream type
 */
typedef enum {
	VIDEO_NON_BURST_WITH_SYNC_PULSES = 0,
	VIDEO_NON_BURST_WITH_SYNC_EVENTS,	// hs_freq and pck should be multipe
	VIDEO_BURST_WITH_SYNC_PULSES,
}dsih_video_mode_t;

#ifdef CONFIG_FBCON_DRAW_PANIC_TEXT
extern int kpanic_in_progress;
#endif

struct comipfb_dev_cmds {
	unsigned char *cmds;
	unsigned short n_pack;
	unsigned short n_cmds;
};

struct bl_cmds {
	struct comipfb_dev_cmds bl_cmd;
	unsigned int brightness_bit;
};

struct mipi_color_bits {
	unsigned int color_bits; // must be set !!
	unsigned int is_18bit_loosely; // optional
};

struct rw_timeout {
	unsigned int hs_rd_to_cnt;
	unsigned int lp_rd_to_cnt;
	unsigned int hs_wr_to_cnt;
	unsigned int lp_wr_to_cnt;
	unsigned int bta_to_cnt;
};

struct video_mode_info {
	unsigned int hsync;	/* Horizontal Synchronization, unit : pclk. */
	unsigned int hbp;	/* Horizontal Back Porch, unit : pclk. */
	unsigned int hfp;	/* Horizontal Front Porch, unit : pclk. */
	unsigned int vsync;	/* Vertical Synchronization, unit : line. */
	unsigned int vbp;	/* Vertical Back Porch, unit : line. */
	unsigned int vfp;	/* Vertical Front Porch, unit : line. */
	unsigned int sync_pol;
	unsigned int lp_cmd_en:1;
	unsigned int frame_bta_ack:1;
	unsigned int lp_hfp_en:1; // default should be 1
	unsigned int lp_hbp_en:1;
	unsigned int lp_vact_en:1;
	unsigned int lp_vfp_en:1;
	unsigned int lp_vbp_en:1;
	unsigned int lp_vsa_en:1;
	dsih_video_mode_t mipi_trans_type; /* burst or no burst*/
};

struct command_mode_info {
	unsigned int tear_fx_en:1;  
	unsigned int ack_rqst_en:1;
	unsigned int gen_sw_0p_tx:1;	// default should be 1
	unsigned int gen_sw_1p_tx:1;	// default should be 1
	unsigned int gen_sw_2p_tx:1;	// default should be 1
	unsigned int gen_sr_0p_tx:1;	// default should be 1
	unsigned int gen_sr_1p_tx:1;	// default should be 1
	unsigned int gen_sr_2p_tx:1;	// default should be 1
	unsigned int gen_lw_tx:1;		// default should be 1
	unsigned int dcs_sw_0p_tx:1;	// default should be 1
	unsigned int dcs_sw_1p_tx:1;	// default should be 1
	unsigned int dcs_sr_0p_tx:1;	// default should be 1
	unsigned int dcs_lw_tx:1;		// default should be 1
	unsigned int max_rd_pkt_size:1;	// default should be 1
	struct rw_timeout timeout;
};

struct external_info {
	unsigned char crc_rx_en:1;
	unsigned char ecc_rx_en:1;
	unsigned char eotp_rx_en:1;
	unsigned char eotp_tx_en:1;
	unsigned int dev_read_time;	//HSBYTECLK is danwe 
};

struct phy_time_info {
	unsigned char lpx;
	//unsigned char clk_lpx;
	unsigned char clk_tprepare;
	unsigned char clk_hs_zero;
	unsigned char clk_hs_trail;
	unsigned char clk_hs_exit;
	unsigned char clk_hs_post;
	
	//unsigned char data_lpx;
	unsigned char data_tprepare;
	unsigned char data_hs_zero;
	unsigned char data_hs_trail;
	unsigned char data_hs_exit;
	unsigned char data_hs_post;
};

struct te_info {
	unsigned int te_source;
	unsigned int te_trigger_mode;
	unsigned int te_en;
	unsigned int te_sync_en; // In command mode should set 1,  video should set 0
	unsigned int te_cps;	// te count per second
};

struct comipfb_dev_timing_mipi {
	unsigned int hs_freq; /*PHY output freq, bytes KHZ*/
	unsigned int lp_freq; /*default is 10MHZ*/
	unsigned int no_lanes; /*lane numbers*/
	unsigned int display_mode; //video mode or command mode.
	unsigned int auto_stop_clklane_en;
	unsigned int im_pin_val; /*IM PIN val, if use gpio_im, default config is 1  ?? */
	struct mipi_color_bits color_mode; /*color bits*/
	struct video_mode_info videomode_info;
	struct command_mode_info commandmode_info;
	struct phy_time_info phytime_info;
	struct te_info teinfo;
	struct external_info ext_info;
};

struct common_id_info {
	unsigned char pack_type;
	unsigned char id[6];
	unsigned char id_count;
	unsigned char cmd;
};

struct comipfb_id_info {
	unsigned char num_id_info;
	struct common_id_info *id_info;
	struct comipfb_dev_cmds prepare_cmd;
};

struct prefer_ce_info {
	int type;
	struct comipfb_dev_cmds cmds;
};

struct comipfb_prefer_ce {
	int types;
	struct prefer_ce_info *info;
};

struct comipfb_dev {
	const char* name;	/* Device name. */
	unsigned int interface_info;//interface infomation  MIPI or RGB
	unsigned int lcd_id;
	unsigned int refresh_en;	/* Refresh enable. */
	unsigned int pclk;		/* Pixel clock in HZ. */
	unsigned int bpp;		/* Bits per pixel. */
	unsigned int xres;		/* Device resolution. */
	unsigned int yres;
	unsigned int width;		/* Width of device in mm. */
	unsigned int height;		/* Height of device in mm. */
	unsigned int flags;		/* Device flags. */
	unsigned int auto_fps;		/* auto adjust frame rate flag*/
	union {
		//struct comipfb_dev_timing_rgb rgb;
		struct comipfb_dev_timing_mipi mipi;
	} timing;

	struct comipfb_id_info panel_id_info;
	struct comipfb_id_info esd_id_info;
	struct comipfb_dev_cmds cmds_init;
	struct comipfb_dev_cmds cmds_suspend;
	struct comipfb_dev_cmds cmds_resume;
	struct comipfb_dev_cmds cmds_pre_suspend;
	struct bl_cmds backlight_info;

	struct comipfb_prefer_ce resume_prefer_info;
	struct comipfb_prefer_ce display_prefer_info;
	struct comipfb_prefer_ce display_ce_info;
	int init_last;		/*when resume, send gamma/ce cmd before init cmd*/

	int (*power)(struct comipfb_info *fbi, int onoff);
	int (*reset)(struct comipfb_info *fbi);
	int (*suspend)(struct comipfb_info *fbi);
	int (*resume)(struct comipfb_info *fbi);
};

extern int comipfb_dev_register(struct comipfb_dev* dev);
extern int comipfb_dev_unregister(struct comipfb_dev* dev);
extern struct comipfb_dev* comipfb_dev_get(struct comipfb_info *fbi);

#endif /*__COMIPFB_DEV_H_*/
