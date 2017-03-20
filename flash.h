/**********************************************************************************
 *
 *      Flash operation routine defines
 *
 **********************************************************************************
 * FileName:        flash.h
 * Version:			1.00
 *
 * Processor:       MSP430G2xxx
 * Complier:        IAR Workbench for MSP430 v4.50 or higher
 *                  
 * Company:         ARTON
 *
 *
 * Author               Date      	Version	  		Comment
 *-------------------------------------------------------------------------------
 * Oleg Semeniuk	 25.11.2013    	1.00		Release for v1.00
 *

 *********************************************************************************/

#ifndef __FLASH_H
#define __FLASH_H

/*********************************************************************************/
/*                                 INCLUDES                                      */
/*********************************************************************************/
#include "type.h"

/*********************************************************************************/
/*                               DEFINITIONS                                     */
/*********************************************************************************/
#define FLASH_LEN						64		/* Total bytes параметров, размещенных во флеш */
#define PROPERTIES_LEN					38		/* Количество байт properties, размещенных во флеш (without CRC) */

#define FLASH_SEG_ADDR					0x01040  /* Seg C */


// Смещения адресов параметров  относительно начала сектора FLASH_SEG_ADDR
#define	eeVERSION_OFFSET				0
#define	eeDEV_TYPE_OFFSET				2
#define	eeSERIAL_OFFSET					3
#define	eeDATE_PROD_OFFSET				6
#define	eeTIMER_CALIBR_OFFSET			8

#define	eeRF_CHANNEL_OFFSET				10
#define	eeRF_ADDR_OFFSET				11
#define	eeRF_ADDR_ADD					12

#define	eeCONFIG_REG_OFFSET				16
#define	eeLIMIT_NORM_OFFSET				18
#define	eeLIMIT_PREFIRE_OFFSET			20
#define	eeLIMIT_FIRE_OFFSET				22
#define	eeLIMIT_FIRE_LOW_OFFSET			24
#define	eeFIRE_HISTER_OFFSET			26
#define	eeLIMIT_LONG_DRIFT_OFFSET		28
#define	eeLIMIT_CHAMBER_FAULT_L_OFFSET	30
#define	eeLIMIT_CHAMBER_FAULT_H_OFFSET	32
#define	eeCALIBR_FAULT_OFFSET			34
#define	ee_GAIN							36
#define	ee_AMP_NO						37

#define	eeCRC_OFFSET					38



typedef union {
	 u16	word;
	 struct  {
		u8	fDrift 			: 1;	/* Photo Chamber Long Term Drift Adjustment Enable bit */
	 };
}tCfgReg;


typedef struct  {
	// Common section
	u16			version;			// Current firmware version 
	u8			dev_type;			// Type of device
	u8			serial[3];			// Serial No
	u16			date_prod;			// Date production
	u16			timer_calibr;		// Timer calibration value
	u8			rf_channel;			// Current RF Channel
	u8			rf_addr;			// Current RF Addr
	u8			rf_add[4];			// NU
	// Specific section
	tCfgReg		config_reg;			// Config register
	u16			limit_norm;			// Значение фонового сигнала (в чистом воздухе) (increment)
	u16			limit_prefire;		// Prefire threshold (increment)
	u16			limit_fire;			// Fire threshold (increment)
	u16			limit_fire_low;		// Low level fire threshold (increment)
	u16			fire_hister;		// Histeresis of fire mode (increment)
	u16			limit_drift;		// Long time drift fault threshold (increment)
	u16			limit_low;			// Нижний порог определения неисправности (increment)
	u16			limit_hi;			// Верхний порог определения неисправности (increment)
	u8			calibr_fault;		// Last calibration fault
	u8			nu;					// NU
	u8			Gain;				// Gain = X = 0...2 + reference value = 0x0X (1.5V), 0x1X (2.5V), 0x2X (3.3V),  
	u8			AMP_No;				// AMP Number (ADC_Input No  = 1 or 2)
	//	
	u16			CS;					// Checksumm 
}tEE_CONFIG;


#define CONFIG		((tEE_CONFIG *)(FLASH_SEG_ADDR))


#define eeBATLevelHister	6		/* Histeresis in 10 mV */


/*********************************************************************************/
/*                                FUNCTIONS                                      */
/*********************************************************************************/
void FlashInit(void);
void FlashDeInit(void);
void FlashEraseSegment(unsigned char *SegPtr);
void FlashWriteWord(unsigned short *DataPtr, unsigned short word);
void FlashWriteArray(unsigned char * DestPtr, unsigned char *SrcPtr, int len);

void FlashEraseSegmentSafe(int safe_bytes);
void FlashWriteSegmentSafe(int restore_bytes);

void StorageProperty(u16 seg_offset, unsigned char * data, u8 len);
void StoragePropertyByte(u16 seg_offset, u8 byte);
void StoragePropertyWord(u16 seg_offset, u16 word);

u16  GetPropertiesCS(void);
void SavePropertyCS(void);

#endif	/* __FLASH_H */



