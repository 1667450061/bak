#include "comipfb.h"
#include "comipfb_dev.h"
#include "mipi_cmd.h"
#include "mipi_interface.h"

static u8 backlight_cmds[][ROW_LINE] = {
        {0x00, DCS_CMD, SW_PACK2, 0x02, 0x51, 0xBE},
};

//[0]: delay after transfer; [1]:count of data; [2]: word count ls; [3]:word count ms; [4]...: data for transfering
static u8 lcd_cmds_init[][ROW_LINE] = {
/****Start Initial Sequence ***/
	{0x0a, GEN_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01},
	{0x0a, GEN_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xBC, 0x00, 0x78, 0x1A},
	{0x0a, GEN_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xBD, 0x00, 0x78, 0x1A},
	{0x0a, GEN_CMD, LW_PACK, 0x05, 0x03, 0x00, 0xBE, 0x00, 0x4E},
	{0x0a, GEN_CMD, LW_PACK, 0x37, 0x35, 0x00, 0xD1, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x5C, 0x00, 0x80, 0x00, 0xAB, 0x00, 0xE4, 0x01, 0x15, 0x01, 0x5C, 0x01, 0x8E, 0x01, 0xD3, 0x02, 0x03, 0x02, 0x45, 0x02, 0x77, 0x02, 0x78, 0x02, 0xA4, 0x02, 0xD1, 0x02, 0xEA, 0x03, 0x09, 0x03, 0x1A, 0x03, 0x32, 0x03, 0x40, 0x03, 0x59, 0x03, 0x68, 0x03, 0x7C, 0x03, 0xB2, 0x03, 0xD8},
	{0x0a, GEN_CMD, LW_PACK, 0x37, 0x35, 0x00, 0xD2, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x5C, 0x00, 0x80, 0x00, 0xAB, 0x00, 0xE4, 0x01, 0x15, 0x01, 0x5C, 0x01, 0x8E, 0x01, 0xD3, 0x02, 0x03, 0x02, 0x45, 0x02, 0x77, 0x02, 0x78, 0x02, 0xA4, 0x02, 0xD1, 0x02, 0xEA, 0x03, 0x09, 0x03, 0x1A, 0x03, 0x32, 0x03, 0x40, 0x03, 0x59, 0x03, 0x68, 0x03, 0x7C, 0x03, 0xB2, 0x03, 0xD8},
	{0x0a, GEN_CMD, LW_PACK, 0x37, 0x35, 0x00, 0xD3, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x5C, 0x00, 0x80, 0x00, 0xAB, 0x00, 0xE4, 0x01, 0x15, 0x01, 0x5C, 0x01, 0x8E, 0x01, 0xD3, 0x02, 0x03, 0x02, 0x45, 0x02, 0x77, 0x02, 0x78, 0x02, 0xA4, 0x02, 0xD1, 0x02, 0xEA, 0x03, 0x09, 0x03, 0x1A, 0x03, 0x32, 0x03, 0x40, 0x03, 0x59, 0x03, 0x68, 0x03, 0x7C, 0x03, 0xB2, 0x03, 0xD8},
	{0x0a, GEN_CMD, LW_PACK, 0x37, 0x35, 0x00, 0xD4, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x5C, 0x00, 0x80, 0x00, 0xAB, 0x00, 0xE4, 0x01, 0x15, 0x01, 0x5C, 0x01, 0x8E, 0x01, 0xD3, 0x02, 0x03, 0x02, 0x45, 0x02, 0x77, 0x02, 0x78, 0x02, 0xA4, 0x02, 0xD1, 0x02, 0xEA, 0x03, 0x09, 0x03, 0x1A, 0x03, 0x32, 0x03, 0x40, 0x03, 0x59, 0x03, 0x68, 0x03, 0x7C, 0x03, 0xB2, 0x03, 0xD8},
	{0x0a, DCS_CMD, LW_PACK, 0x37, 0x35, 0x00, 0xD5, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x5C, 0x00, 0x80, 0x00, 0xAB, 0x00, 0xE4, 0x01, 0x15, 0x01, 0x5C, 0x01, 0x8E, 0x01, 0xD3, 0x02, 0x03, 0x02, 0x45, 0x02, 0x77, 0x02, 0x78, 0x02, 0xA4, 0x02, 0xD1, 0x02, 0xEA, 0x03, 0x09, 0x03, 0x1A, 0x03, 0x32, 0x03, 0x40, 0x03, 0x59, 0x03, 0x68, 0x03, 0x7C, 0x03, 0xB2, 0x03, 0xD8},//dcs_cmd
	{0x0a, GEN_CMD, LW_PACK, 0x37, 0x35, 0x00, 0xD6, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x5C, 0x00, 0x80, 0x00, 0xAB, 0x00, 0xE4, 0x01, 0x15, 0x01, 0x5C, 0x01, 0x8E, 0x01, 0xD3, 0x02, 0x03, 0x02, 0x45, 0x02, 0x77, 0x02, 0x78, 0x02, 0xA4, 0x02, 0xD1, 0x02, 0xEA, 0x03, 0x09, 0x03, 0x1A, 0x03, 0x32, 0x03, 0x40, 0x03, 0x59, 0x03, 0x68, 0x03, 0x7C, 0x03, 0xB2, 0x03, 0xD8},
	{0x0a, GEN_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xB0, 0x00, 0x00, 0x00},
	{0x0a, DCS_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xB6, 0x36, 0x36, 0x36},
	{0x0a, GEN_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xB8, 0x26, 0x26, 0x26},
	{0x0a, DCS_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xB1, 0x00, 0x00, 0x00},
	{0x0a, GEN_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xB7, 0x26, 0x26, 0x26},
	{0x0a, DCS_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xB9, 0x34, 0x34, 0x34},
	{0x0a, DCS_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xBA, 0x16, 0x16, 0x16},
	{0x0a, GEN_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00},
	{0x0a, DCS_CMD, LW_PACK, 0x05, 0x03, 0x00, 0xB1, 0xFC, 0x00},//delay 0A,
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0xB4, 0x10},
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0xB6, 0x07},//lw_pack
	{0x0a, GEN_CMD, LW_PACK, 0x05, 0x03, 0x00, 0xB7, 0x71, 0x71},
	{0x0a, GEN_CMD, LW_PACK, 0x07, 0x05, 0x00, 0xB8, 0x01, 0x0A, 0x0A, 0x0A},
	{0x0a, GEN_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xBC, 0x05, 0x05, 0x05},
	{0x0a, GEN_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xBD, 0x01, 0x84, 0x05, 0x31, 0x00},
	{0x0a, GEN_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xBE, 0x01, 0x84, 0x07, 0x31, 0x00},
	{0x0a, GEN_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xBF, 0x01, 0x84, 0x07, 0x31, 0x00},
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0x51, 0x00},
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0x53, 0x26},
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0x55, 0x00},
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0x35, 0x00},
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0x3A, 0x77},
	{0x0a, GEN_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00},
	{0x0a, GEN_CMD, SW_PACK2, 0x02, 0xC7, 0x02},
	{0x0a, DCS_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xC9, 0x11, 0x00, 0x00, 0x00, 0x00},
	{0xFF, DCS_CMD, SW_PACK1, 0x01, 0x21},
	{0xFF, DCS_CMD, SW_PACK1, 0x01, 0x11},//0xFF
	{0xFF, DCS_CMD, SW_PACK1, 0x01, 0x29},
};

static u8 lcd_cmds_suspend[][ROW_LINE] = {
	{0x64, DCS_CMD, SW_PACK1, 0x01, 0x28},
	{0xff, DCS_CMD, SW_PACK1, 0x01, 0x10},	//TODO delay is 400ms.
};

static u8 lcd_cmds_resume[][ROW_LINE] = {
	{0x0a, GEN_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01},
	{0x0a, GEN_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xBC, 0x00, 0x78, 0x1A},
	{0x0a, GEN_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xBD, 0x00, 0x78, 0x1A},
	{0x0a, GEN_CMD, LW_PACK, 0x05, 0x03, 0x00, 0xBE, 0x00, 0x4E},
	{0x0a, GEN_CMD, LW_PACK, 0x37, 0x35, 0x00, 0xD1, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x5C, 0x00, 0x80, 0x00, 0xAB, 0x00, 0xE4, 0x01, 0x15, 0x01, 0x5C, 0x01, 0x8E, 0x01, 0xD3, 0x02, 0x03, 0x02, 0x45, 0x02, 0x77, 0x02, 0x78, 0x02, 0xA4, 0x02, 0xD1, 0x02, 0xEA, 0x03, 0x09, 0x03, 0x1A, 0x03, 0x32, 0x03, 0x40, 0x03, 0x59, 0x03, 0x68, 0x03, 0x7C, 0x03, 0xB2, 0x03, 0xD8},
	{0x0a, GEN_CMD, LW_PACK, 0x37, 0x35, 0x00, 0xD2, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x5C, 0x00, 0x80, 0x00, 0xAB, 0x00, 0xE4, 0x01, 0x15, 0x01, 0x5C, 0x01, 0x8E, 0x01, 0xD3, 0x02, 0x03, 0x02, 0x45, 0x02, 0x77, 0x02, 0x78, 0x02, 0xA4, 0x02, 0xD1, 0x02, 0xEA, 0x03, 0x09, 0x03, 0x1A, 0x03, 0x32, 0x03, 0x40, 0x03, 0x59, 0x03, 0x68, 0x03, 0x7C, 0x03, 0xB2, 0x03, 0xD8},
	{0x0a, GEN_CMD, LW_PACK, 0x37, 0x35, 0x00, 0xD3, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x5C, 0x00, 0x80, 0x00, 0xAB, 0x00, 0xE4, 0x01, 0x15, 0x01, 0x5C, 0x01, 0x8E, 0x01, 0xD3, 0x02, 0x03, 0x02, 0x45, 0x02, 0x77, 0x02, 0x78, 0x02, 0xA4, 0x02, 0xD1, 0x02, 0xEA, 0x03, 0x09, 0x03, 0x1A, 0x03, 0x32, 0x03, 0x40, 0x03, 0x59, 0x03, 0x68, 0x03, 0x7C, 0x03, 0xB2, 0x03, 0xD8},
	{0x0a, GEN_CMD, LW_PACK, 0x37, 0x35, 0x00, 0xD4, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x5C, 0x00, 0x80, 0x00, 0xAB, 0x00, 0xE4, 0x01, 0x15, 0x01, 0x5C, 0x01, 0x8E, 0x01, 0xD3, 0x02, 0x03, 0x02, 0x45, 0x02, 0x77, 0x02, 0x78, 0x02, 0xA4, 0x02, 0xD1, 0x02, 0xEA, 0x03, 0x09, 0x03, 0x1A, 0x03, 0x32, 0x03, 0x40, 0x03, 0x59, 0x03, 0x68, 0x03, 0x7C, 0x03, 0xB2, 0x03, 0xD8},
	{0x0a, DCS_CMD, LW_PACK, 0x37, 0x35, 0x00, 0xD5, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x5C, 0x00, 0x80, 0x00, 0xAB, 0x00, 0xE4, 0x01, 0x15, 0x01, 0x5C, 0x01, 0x8E, 0x01, 0xD3, 0x02, 0x03, 0x02, 0x45, 0x02, 0x77, 0x02, 0x78, 0x02, 0xA4, 0x02, 0xD1, 0x02, 0xEA, 0x03, 0x09, 0x03, 0x1A, 0x03, 0x32, 0x03, 0x40, 0x03, 0x59, 0x03, 0x68, 0x03, 0x7C, 0x03, 0xB2, 0x03, 0xD8},//dcs_cmd
	{0x0a, GEN_CMD, LW_PACK, 0x37, 0x35, 0x00, 0xD6, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x5C, 0x00, 0x80, 0x00, 0xAB, 0x00, 0xE4, 0x01, 0x15, 0x01, 0x5C, 0x01, 0x8E, 0x01, 0xD3, 0x02, 0x03, 0x02, 0x45, 0x02, 0x77, 0x02, 0x78, 0x02, 0xA4, 0x02, 0xD1, 0x02, 0xEA, 0x03, 0x09, 0x03, 0x1A, 0x03, 0x32, 0x03, 0x40, 0x03, 0x59, 0x03, 0x68, 0x03, 0x7C, 0x03, 0xB2, 0x03, 0xD8},
	{0x0a, GEN_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xB0, 0x00, 0x00, 0x00},
	{0x0a, DCS_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xB6, 0x36, 0x36, 0x36},
	{0x0a, GEN_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xB8, 0x26, 0x26, 0x26},
	{0x0a, DCS_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xB1, 0x00, 0x00, 0x00},
	{0x0a, GEN_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xB7, 0x26, 0x26, 0x26},
	{0x0a, DCS_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xB9, 0x34, 0x34, 0x34},
	{0x0a, DCS_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xBA, 0x16, 0x16, 0x16},
	{0x0a, GEN_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00},
	{0x0a, DCS_CMD, LW_PACK, 0x05, 0x03, 0x00, 0xB1, 0xFC, 0x00},//delay 0A,
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0xB4, 0x10},
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0xB6, 0x07},//lw_pack
	{0x0a, GEN_CMD, LW_PACK, 0x05, 0x03, 0x00, 0xB7, 0x71, 0x71},
	{0x0a, GEN_CMD, LW_PACK, 0x07, 0x05, 0x00, 0xB8, 0x01, 0x0A, 0x0A, 0x0A},
	{0x0a, GEN_CMD, LW_PACK, 0x06, 0x04, 0x00, 0xBC, 0x05, 0x05, 0x05},
	{0x0a, GEN_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xBD, 0x01, 0x84, 0x05, 0x31, 0x00},
	{0x0a, GEN_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xBE, 0x01, 0x84, 0x07, 0x31, 0x00},
	{0x0a, GEN_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xBF, 0x01, 0x84, 0x07, 0x31, 0x00},
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0x51, 0x00},
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0x53, 0x26},
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0x55, 0x00},
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0x35, 0x00},
	{0x0a, DCS_CMD, SW_PACK2, 0x02, 0x3A, 0x77},
	{0x0a, GEN_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00},
	{0x0a, GEN_CMD, SW_PACK2, 0x02, 0xC7, 0x02},
	{0x0a, DCS_CMD, LW_PACK, 0x08, 0x06, 0x00, 0xC9, 0x11, 0x00, 0x00, 0x00, 0x00},
	{0xFF, DCS_CMD, SW_PACK1, 0x01, 0x21},
	{0xFF, DCS_CMD, SW_PACK1, 0x01, 0x11},//0xFF
	{0xFF, DCS_CMD, SW_PACK1, 0x01, 0x29},
	};

static int lcd_yt_ld35510s_reset(struct comipfb_info *fbi)
{
	int gpio_rst = fbi->pdata->gpio_rst;

	if (gpio_rst >= 0) {
		gpio_request(gpio_rst, "LCD Reset");
		gpio_direction_output(gpio_rst, 1);
		mdelay(20);
		gpio_direction_output(gpio_rst, 0);
		mdelay(20);
		gpio_direction_output(gpio_rst, 1);
		mdelay(120);
		gpio_free(gpio_rst);
	}
	return 0;
}

static int lcd_yt_ld35510s_suspend(struct comipfb_info *fbi)
{
	int ret=0;
	struct comipfb_dev_timing_mipi *mipi;

	mipi = &(fbi->cdev->timing.mipi);

	if (mipi->display_mode == MIPI_VIDEO_MODE) {
		mipi_dsih_hal_mode_config(fbi, 1);
	}
	if (!(fbi->pdata->flags & COMIPFB_SLEEP_POWEROFF))
		ret = comipfb_if_mipi_dev_cmds(fbi, &fbi->cdev->cmds_suspend);
	msleep(20);
	mipi_dsih_dphy_enable_hs_clk(fbi, 0);
	if(fbi->cdev->reset)
		fbi->cdev->reset(fbi);

	mipi_dsih_dphy_ulps_en(fbi, 1);
	mipi_dsih_dphy_reset(fbi, 0);
	mipi_dsih_dphy_shutdown(fbi, 0);

	return ret;
}

static int lcd_yt_ld35510s_resume(struct comipfb_info *fbi)
{
	int ret=0;
	struct comipfb_dev_timing_mipi *mipi;

	mipi = &(fbi->cdev->timing.mipi);

	mipi_dsih_dphy_shutdown(fbi, 1);
	mipi_dsih_dphy_reset(fbi, 1);
	mipi_dsih_dphy_ulps_en(fbi, 0);

	if (fbi->cdev->reset)
		fbi->cdev->reset(fbi);

#ifdef CONFIG_FBCON_DRAW_PANIC_TEXT
	if (unlikely(kpanic_in_progress == 1)) {
		ret = comipfb_if_mipi_dev_cmds(fbi, &fbi->cdev->cmds_init);
	}
	else
		ret = comipfb_if_mipi_dev_cmds(fbi, &fbi->cdev->cmds_resume);
#else
	ret = comipfb_if_mipi_dev_cmds(fbi, &fbi->cdev->cmds_resume);
#endif
	msleep(20);
	if (mipi->display_mode == MIPI_VIDEO_MODE) {
		mipi_dsih_hal_mode_config(fbi, 0);
	}
	mipi_dsih_dphy_enable_hs_clk(fbi, 1);

	return ret;
}

struct comipfb_dev lcd_yt_ld35510s_dev = {
	.name = "lcd_yt_ld35510s",
	.interface_info = COMIPFB_MIPI_IF,
	.lcd_id = 0,
	.refresh_en = 1,
	.bpp = 32,
	.xres = 480,
	.yres = 800,
	.flags = 0,
	.pclk = 25000000,
	.timing = {
		.mipi = {
			.hs_freq = 41000,		//Kbyte
			.lp_freq = 8000,		//KHZ
			.no_lanes = 2,
			.display_mode = MIPI_VIDEO_MODE,
			.im_pin_val = 1,
			.color_mode = {
				.color_bits = COLOR_CODE_24BIT,
			},
			.videomode_info = {
				.hsync = 4,
				.hbp = 10,
				.hfp = 10,
				.vsync = 4,
				.vbp = 7,
				.vfp = 5,
				.sync_pol = COMIPFB_VSYNC_HIGH_ACT,
				.lp_cmd_en = 1,
				.lp_hfp_en = 1,
				.lp_hbp_en = 1,
				.lp_vact_en = 1,
				.lp_vfp_en = 1,
				.lp_vbp_en = 1,
				.lp_vsa_en = 1,
				.mipi_trans_type = VIDEO_BURST_WITH_SYNC_PULSES,
			},
			.phytime_info = {
				.clk_tprepare = 3, //HSBYTECLK
			},
			.teinfo = {
				.te_source = 1, //external signal
				.te_trigger_mode = 0,
				.te_en = 0,
				.te_sync_en = 0,
			},
			.ext_info = {
				.eotp_tx_en = 1,
			},
		},
	},
	.cmds_init = {ARRAY_AND_SIZE(lcd_cmds_init)},
	.cmds_suspend = {ARRAY_AND_SIZE(lcd_cmds_suspend)},
	.cmds_resume = {ARRAY_AND_SIZE(lcd_cmds_resume)},
	.reset = lcd_yt_ld35510s_reset,
	.suspend = lcd_yt_ld35510s_suspend,
	.resume = lcd_yt_ld35510s_resume,
	.backlight_info = {ARRAY_AND_SIZE(backlight_cmds),
				.brightness_bit = 5,
				},
};

static int __init lcd_yt_ld35510s_init(void)
{
	return comipfb_dev_register(&lcd_yt_ld35510s_dev);
}

subsys_initcall(lcd_yt_ld35510s_init);