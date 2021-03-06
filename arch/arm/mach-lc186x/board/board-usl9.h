#ifndef __ASM_ARCH_BOARD_USL9_H__
#define __ASM_ARCH_BOARD_USL9_H__

#include <plat/mfp.h>
#include <plat/comipfb.h>

#if defined(CONFIG_COMIP_PROJECT_USL9)

/* PMIC&CODEC-LC1160. */
#define LC1160_GPIO_MCLK		MFP_PIN_GPIO(90)
#define LC1160_GPIO_ECLK		MFP_PIN_GPIO(91)
#define LC1160_INT_PIN			MFP_PIN_GPIO(99)
#define LC1160_INT2_PIN			MFP_PIN_GPIO(223)
#define CODEC_PA_PIN			MFP_PIN_GPIO(75)
#define CODEC_JACK_POLARITY	1

/* Sensors */
#define LIGHT_INT				MFP_PIN_GPIO(135)
//#define ECOMPASS_INT_PIN		MFP_PIN_GPIO(5)
//#define ACCELER_INT_PIN		MFP_PIN_GPIO(22)

/* GPIO keypad. */
#define COMIP_GPIO_KEY_VOLUMEUP		MFP_PIN_GPIO(72)
#define COMIP_GPIO_KEY_VOLUMEDOWN		MFP_PIN_GPIO(73)

/* LCD. */
#define LCD_RESET_PIN			MFP_PIN_GPIO(154)
#define LCD_TE                  MFP_PIN_GPIO(156)

/* KEYPAD LED*/
#define KEYPAD_LED_PIN			MFP_PIN_GPIO(74)

/* Touchscreen-FT5X06. */
#define TS_INT_PIN			MFP_PIN_GPIO(166)
#define TS_RST_PIN			MFP_PIN_GPIO(165)
/* CAMERA. */

#define BCAMERA_POWERDOWN_PIN		MFP_PIN_GPIO(171)
#define BCAMERA_RESET_PIN			MFP_PIN_GPIO(169)
#define FCAMERA_POWERDOWN_PIN		MFP_PIN_GPIO(172)
#define FCAMERA_RESET_PIN			MFP_PIN_GPIO(170)
//#define CAMERA_FALSH_PIN			MFP_PIN_GPIO(175)
//#define CAMERA_LDO_PIN			MFP_PIN_GPIO(176)
#if defined(CONFIG_GPS_UBLOX)
#define UBLOX_GPS_RESET_PIN		MFP_PIN_GPIO(205)
#endif

/*rtk wifi*/
#ifdef CONFIG_RTK_WLAN_SDIO
#define RTK_WIFI_ONOFF_PIN		MFP_PIN_GPIO(199)
#define RTK_WIFI_WAKE_I_PIN		MFP_PIN_GPIO(203)
#define RTK_BTWIFI_CHIPEN_PIN		MFP_PIN_GPIO(140)
#endif

#if defined (CONFIG_BRCM_BLUETOOTH) || defined (CONFIG_RTK_BLUETOOTH)
#define UART2_CTS_PIN		MFP_PIN_GPIO(185)
#endif

/* BT-RTL8723BS. */
#ifdef CONFIG_RTK_BLUETOOTH
#define RTK_BT_ONOFF_PIN		MFP_PIN_GPIO(MFP_PIN_MAX)
#define RTK_BT_RESET_PIN		MFP_PIN_GPIO(198)
#define RTK_BT_WAKE_I_PIN		MFP_PIN_GPIO(200)
#define RTK_BT_WAKE_O_PIN		MFP_PIN_GPIO(202)
#endif

#ifdef CONFIG_RADIO_RTK8723B
#define RTK_FM_EN_PIN 			MFP_PIN_GPIO(143)
#endif

#ifdef CONFIG_BRCM_BLUETOOTH
#define BRCM_BT_ONOFF_PIN       MFP_PIN_GPIO(MFP_PIN_MAX)
#define BRCM_BT_RESET_PIN		MFP_PIN_GPIO(198)
#define BRCM_BT_WAKE_I_PIN		MFP_PIN_GPIO(200)
#define BRCM_BT_WAKE_O_PIN		MFP_PIN_GPIO(202)
#endif

#ifdef CONFIG_BRCM_WIFI
#define BRCM_WIFI_ONOFF_PIN			MFP_PIN_GPIO(199)
#define BRCM_WIFI_WAKE_I_PIN		MFP_PIN_GPIO(203)
#define BRCM_WIFI_SDIO_CLK_PIN		MFP_PIN_GPIO(191)
#define BRCM_WIFI_SDIO_CMD_PIN		MFP_PIN_GPIO(192)
#define BRCM_WIFI_SDIO_DATA3_PIN	MFP_PIN_GPIO(193)
#define BRCM_WIFI_SDIO_DATA2_PIN	MFP_PIN_GPIO(194)
#define BRCM_WIFI_SDIO_DATA1_PIN	MFP_PIN_GPIO(195)
#define BRCM_WIFI_SDIO_DATA0_PIN	MFP_PIN_GPIO(196)
#endif

#ifdef CONFIG_BRCM_GPS
#define BCM_GPS_RESET_PIN		MFP_PIN_GPIO(MFP_PIN_MAX)
#define BCM_GPS_STANDBY_PIN		MFP_PIN_GPIO(206)
#endif
#ifdef CONFIG_USB_COMIP_OTG
/* USB OTG ID PIN. */
#define USB_OTG_ID_PIN 			MFP_PIN_GPIO(161)
#endif

#endif
#endif /*__ASM_ARCH_BOARD_USL9_H__*/
