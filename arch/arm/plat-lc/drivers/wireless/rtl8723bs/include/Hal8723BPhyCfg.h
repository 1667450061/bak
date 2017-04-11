/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#ifndef __INC_HAL8723BPHYCFG_H__
#define __INC_HAL8723BPHYCFG_H__

/*--------------------------Define Parameters-------------------------------*/
#define LOOP_LIMIT				5
#define MAX_STALL_TIME			50		//us
#define AntennaDiversityValue	0x80	//(Adapter->bSoftwareAntennaDiversity ? 0x00:0x80)
#define MAX_TXPWR_IDX_NMODE_92S	63
#define Reset_Cnt_Limit			3

#define IQK_MAC_REG_NUM		4
#define IQK_ADDA_REG_NUM		16
#define IQK_BB_REG_NUM			9
#define HP_THERMAL_NUM		8

#ifdef CONFIG_PCI_HCI
#define MAX_AGGR_NUM	0x0B
#else
#define MAX_AGGR_NUM	0x07
#endif // CONFIG_PCI_HCI

#define	RF_PATH_MAX		4

/*--------------------------Define Parameters End-------------------------------*/


/*------------------------------Define structure----------------------------*/
typedef enum _SwChnlCmdID{
	CmdID_End,
	CmdID_SetTxPowerLevel,
	CmdID_BBRegWrite10,
	CmdID_WritePortUlong,
	CmdID_WritePortUshort,
	CmdID_WritePortUchar,
	CmdID_RF_WriteReg,
}SwChnlCmdID;


/* 1. Switch channel related */
typedef struct _SwChnlCmd{
	SwChnlCmdID	CmdID;
	u32			Para1;
	u32			Para2;
	u32			msDelay;
}SwChnlCmd;

typedef enum _HW90_BLOCK{
	HW90_BLOCK_MAC = 0,
	HW90_BLOCK_PHY0 = 1,
	HW90_BLOCK_PHY1 = 2,
	HW90_BLOCK_RF = 3,
	HW90_BLOCK_MAXIMUM = 4, // Never use this
}HW90_BLOCK_E, *PHW90_BLOCK_E;

typedef enum _RF_RADIO_PATH{
	RF_PATH_A = 0,			//Radio Path A
	RF_PATH_B = 1,			//Radio Path B
	RF_PATH_C = 2,			//Radio Path C
	RF_PATH_D = 3,			//Radio Path D
	//RF_PATH_MAX				//Max RF number 90 support
}RF_RADIO_PATH_E, *PRF_RADIO_PATH_E;

#define CHANNEL_MAX_NUMBER		14	// 14 is the max channel number

typedef enum _WIRELESS_MODE {
	WIRELESS_MODE_UNKNOWN = 0x00,
	WIRELESS_MODE_A 		= BIT2,
	WIRELESS_MODE_B 		= BIT0,
	WIRELESS_MODE_G 		= BIT1,
	WIRELESS_MODE_AUTO 	= BIT5,
	WIRELESS_MODE_N_24G 	= BIT3,
	WIRELESS_MODE_N_5G 	= BIT4,
	WIRELESS_MODE_AC		= BIT6
} WIRELESS_MODE;


typedef enum _PHY_Rate_Tx_Power_Offset_Area{
	RA_OFFSET_LEGACY_OFDM1,
	RA_OFFSET_LEGACY_OFDM2,
	RA_OFFSET_HT_OFDM1,
	RA_OFFSET_HT_OFDM2,
	RA_OFFSET_HT_OFDM3,
	RA_OFFSET_HT_OFDM4,
	RA_OFFSET_HT_CCK,
}RA_OFFSET_AREA,*PRA_OFFSET_AREA;


/* BB/RF related */
typedef	enum _RF_TYPE_8190P{
	RF_TYPE_MIN,	// 0
	RF_8225=1,			// 1 11b/g RF for verification only
	RF_8256=2,			// 2 11b/g/n
	RF_8258=3,			// 3 11a/b/g/n RF
	RF_6052=4,		// 4 11b/g/n RF
	//RF_6052=5,		// 4 11b/g/n RF
	// TODO: We sholud remove this psudo PHY RF after we get new RF.
	RF_PSEUDO_11N=5,	// 5, It is a temporality RF.
}RF_TYPE_8190P_E,*PRF_TYPE_8190P_E;


typedef struct _BB_REGISTER_DEFINITION{
	u32 rfintfs;			// set software control:
							//		0x870~0x877[8 bytes]

	u32 rfintfi;			// readback data:
							//		0x8e0~0x8e7[8 bytes]

	u32 rfintfo; 		// output data:
							//		0x860~0x86f [16 bytes]

	u32 rfintfe; 		// output enable:
							//		0x860~0x86f [16 bytes]

	u32 rf3wireOffset;	// LSSI data:
							//		0x840~0x84f [16 bytes]

	u32 rfLSSI_Select;	// BB Band Select:
							//		0x878~0x87f [8 bytes]

	u32 rfTxGainStage;	// Tx gain stage:
							//		0x80c~0x80f [4 bytes]

	u32 rfHSSIPara1; 	// wire parameter control1 :
							//		0x820~0x823,0x828~0x82b, 0x830~0x833, 0x838~0x83b [16 bytes]

	u32 rfHSSIPara2; 	// wire parameter control2 :
							//		0x824~0x827,0x82c~0x82f, 0x834~0x837, 0x83c~0x83f [16 bytes]

	u32 rfSwitchControl; //Tx Rx antenna control :
							//		0x858~0x85f [16 bytes]

	u32 rfAGCControl1; 	//AGC parameter control1 :
							//		0xc50~0xc53,0xc58~0xc5b, 0xc60~0xc63, 0xc68~0xc6b [16 bytes]

	u32 rfAGCControl2; 	//AGC parameter control2 :
							//		0xc54~0xc57,0xc5c~0xc5f, 0xc64~0xc67, 0xc6c~0xc6f [16 bytes]

	u32 rfRxIQImbalance; //OFDM Rx IQ imbalance matrix :
							//		0xc14~0xc17,0xc1c~0xc1f, 0xc24~0xc27, 0xc2c~0xc2f [16 bytes]

	u32 rfRxAFE;  		//Rx IQ DC ofset and Rx digital filter, Rx DC notch filter :
							//		0xc10~0xc13,0xc18~0xc1b, 0xc20~0xc23, 0xc28~0xc2b [16 bytes]

	u32 rfTxIQImbalance; //OFDM Tx IQ imbalance matrix
							//		0xc80~0xc83,0xc88~0xc8b, 0xc90~0xc93, 0xc98~0xc9b [16 bytes]

	u32 rfTxAFE; 		//Tx IQ DC Offset and Tx DFIR type
							//		0xc84~0xc87,0xc8c~0xc8f, 0xc94~0xc97, 0xc9c~0xc9f [16 bytes]

	u32 rfLSSIReadBack; 	//LSSI RF readback data SI mode
								//		0x8a0~0x8af [16 bytes]

	u32 rfLSSIReadBackPi; 	//LSSI RF readback data PI mode 0x8b8-8bc for Path A and B

}BB_REGISTER_DEFINITION_T, *PBB_REGISTER_DEFINITION_T;

typedef struct _R_ANTENNA_SELECT_OFDM{
	u32			r_tx_antenna:4;
	u32			r_ant_l:4;
	u32			r_ant_non_ht:4;
	u32			r_ant_ht1:4;
	u32			r_ant_ht2:4;
	u32			r_ant_ht_s1:4;
	u32			r_ant_non_ht_s1:4;
	u32			OFDM_TXSC:2;
	u32			Reserved:2;
}R_ANTENNA_SELECT_OFDM;

typedef struct _R_ANTENNA_SELECT_CCK{
	u8			r_cckrx_enable_2:2;
	u8			r_cckrx_enable:2;
	u8			r_ccktx_enable:4;
}R_ANTENNA_SELECT_CCK;

typedef enum _BaseBand_Config_Type{
	BaseBand_Config_PHY_REG = 0,			//Radio Path A
	BaseBand_Config_AGC_TAB = 1,			//Radio Path B
}BaseBand_Config_Type, *PBaseBand_Config_Type;

/*------------------------------Define structure End----------------------------*/

/*--------------------------Exported Function prototype---------------------*/
u32
PHY_QueryBBReg_8723B(
	IN	PADAPTER	Adapter,
	IN	u32		RegAddr,
	IN	u32		BitMask
	);

VOID
PHY_SetBBReg_8723B(
	IN	PADAPTER	Adapter,
	IN	u32		RegAddr,
	IN	u32		BitMask,
	IN	u32		Data
	);

u32
PHY_QueryRFReg_8723B(
	IN	PADAPTER			Adapter,
	IN	RF_RADIO_PATH_E	eRFPath,
	IN	u32				RegAddr,
	IN	u32				BitMask
	);

VOID
PHY_SetRFReg_8723B(
	IN	PADAPTER			Adapter,
	IN	RF_RADIO_PATH_E	eRFPath,
	IN	u32				RegAddr,
	IN	u32				BitMask,
	IN	u32				Data
	);

#define PHY_QueryBBReg(Adapter, RegAddr, BitMask) PHY_QueryBBReg_8723B((Adapter), (RegAddr), (BitMask))
#define PHY_SetBBReg(Adapter, RegAddr, BitMask, Data) PHY_SetBBReg_8723B((Adapter), (RegAddr), (BitMask), (Data))
#define PHY_QueryRFReg(Adapter, eRFPath, RegAddr, BitMask) PHY_QueryRFReg_8723B((Adapter), (eRFPath), (RegAddr), (BitMask))
#define PHY_SetRFReg(Adapter, eRFPath, RegAddr, BitMask, Data) PHY_SetRFReg_8723B((Adapter), (eRFPath), (RegAddr), (BitMask), (Data))


/* MAC/BB/RF HAL config */
int PHY_BBConfig8723B(PADAPTER	Adapter	);

int PHY_RFConfig8723B(PADAPTER	Adapter	);

s32 PHY_MACConfig8723B(PADAPTER padapter);

int
PHY_ConfigRFWithParaFile_8723B(
	IN	PADAPTER			Adapter,
	IN	u8* 				pFileName,
	RF_RADIO_PATH_E		eRFPath
);
int
PHY_ConfigRFWithHeaderFile_8723B(
	IN	PADAPTER			Adapter,
	RF_RADIO_PATH_E		eRFPath
);

int
PHY_ConfigRFWithTxPwrTrackParaFile(
	IN	PADAPTER			Adapter,
	IN	s8 			* pFileName
);

VOID
storePwrIndexDiffRateOffset(
	IN	PADAPTER	Adapter,
	IN	u32		Band,
	IN	u32		RfPath,
	IN	u32		TxNum,
	IN	u32		RegAddr,
	IN	u32		BitMask,
	IN	u32		Data
	);

void PHY_SetTxPowerLevel8723B(PADAPTER	 Adapter, u8 channel);

VOID
PHY_SetTxPowerLevel8723B(
	IN	PADAPTER		Adapter,
	IN	u8			channel
	);

VOID
PHY_SetBWMode8723B(
	IN	PADAPTER					Adapter,
	IN	HT_CHANNEL_WIDTH	Bandwidth,	// 20M or 40M
	IN	unsigned char	Offset		// Upper, Lower, or Don't care
);

VOID
PHY_SwChnl8723B(	// Call after initialization
	IN	PADAPTER	Adapter,
	IN	u8		channel
	);

/*--------------------------Exported Function prototype End---------------------*/

#endif

