/**********************************************************************************
 *
 *      Flash operation routines
 *
 **********************************************************************************
 * FileName:        flash.c
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


/*********************************************************************************/
/*                                 INCLUDES                                      */
/*********************************************************************************/

#include <string.h>
#include  "hardware.h"
#include  "flash.h"
#include  "type.h"
#include  "main.h"


/*********************************************************************************/
/*                                 VARIABLES                                     */
/*********************************************************************************/

// Data flash segment
__no_init 	unsigned char SegC[64] 			@FLASH_SEG_ADDR;


// Area in the flash memory for placing of configuration data
const tEE_CONFIG 	ee_config @ FLASH_SEG_ADDR = {
	// Common section
		.version = 001,					// v1.01
		.dev_type = 5,					// Arton-DL1
		.serial = {0xFF, 0xFF, 0xFF},
		.date_prod = 0x0810,
	// Specific section
		.config_reg = {.fDrift = 1},
		.limit_norm = 400,
		.limit_prefire = 320,
		.limit_fire = 280,
		.fire_hister = 30,
		.limit_drift = 0xFFFF,
		.limit_low = 20,	
		.limit_hi = 600,
		.calibr_fault = 0,
		.Gain = 3,
		.AMP_No = 2,
		//	
		.CS = 0xFFFF,
 };

 
u8 * SegCBackup = (u8*)&signal_array[0];			// Buffer for backup configuration data


/*********************************************************************************/
/*                                FUNCTIONS                                      */
/*********************************************************************************/


//-------------------------------------------------------------------------------------------------
// Function		: void FlashInit(void)
// Parameters	: None
// Return		: None
// Description	: Initialisation Flash for write operation
//-------------------------------------------------------------------------------------------------
void FlashInit(void) {
	_DINT(); 
	while(1) if(!(BUSY&FCTL3))break;
	FCTL2 = FWKEY | FSSEL_2 | FN1;       	// SMCLK/3
}



//-------------------------------------------------------------------------------------------------
// Function		: void FlashDeInit(void)
// Parameters	: None
// Return		: None
// Description	: DeInitialisation Flash for write operation
//-------------------------------------------------------------------------------------------------
void FlashDeInit(void) {
	_EINT();
}


//-------------------------------------------------------------------------------------------------
// Function		: void FlashEraseSegment(unsigned char *SegPtr)
// Parameters	: SegPtr - pointer on segment
// Return		: None
// Description	: Erase of flash memory segment
//-------------------------------------------------------------------------------------------------
void FlashEraseSegment(unsigned char *SegPtr) {
	_DINT();
	FCTL3 = FWKEY;					// Lock   = 0 
	while(1) if(!(BUSY&FCTL3))break;
	FCTL1 = FWKEY | ERASE;			// ERASE  = 1 
	SegPtr[0] = 0;                       
	while(1) if(!(BUSY&FCTL3))break;
	FCTL1 = FWKEY;					// ERASE  = 0 
	FCTL3 = FWKEY | LOCK;			// Lock   = 1 
	_EINT();
}



//-------------------------------------------------------------------------------------------------
// Function		: void FlashWriteByte(unsigned char *DataPtr, unsigned char byte)
// Parameters	: DataPtr - pointer on flash memory, byte - byte of data
// Return		: None
// Description	: Write byte into flash memory
//-------------------------------------------------------------------------------------------------
void FlashWriteByte(unsigned char *DataPtr, unsigned char byte) {
    _DINT();
	do {
		FCTL3 = FWKEY;			// Lock = 0 
		while(1) if(!(BUSY&FCTL3))break;
		FCTL1 = FWKEY|WRT;		// WRT  = 1 
		DataPtr[0] = byte;			// Program Flash byte 
		while(1) if(!(BUSY&FCTL3))break;
		FCTL1 = FWKEY;			// WRT  = 0 
		FCTL3 = FWKEY|LOCK;		// Lock = 1 
    }
    while(DataPtr[0] != byte);
	_EINT();
}


//-------------------------------------------------------------------------------------------------
// Function		: void FlashWriteWord(unsigned char *DataPtr, unsigned short word)
// Parameters	: DataPtr - pointer on flash memory, word - word of data
// Return		: None
// Description	: Write word (2 byte) into flash memory
//-------------------------------------------------------------------------------------------------
void FlashWriteWord(unsigned short *DataPtr, unsigned short word) {
   _DINT();
	do {
		FCTL3 = FWKEY;			// Lock = 0 
		while(1) if(!(BUSY&FCTL3))break;
		FCTL1 = FWKEY|WRT;		// WRT  = 1 
		*DataPtr = word;			// Program Flash byte 
		while(1) if(!(BUSY&FCTL3))break;
		FCTL1 = FWKEY;			// WRT  = 0 
		FCTL3 = FWKEY|LOCK;		// Lock = 1 
    }
    while(*DataPtr != word);
	_EINT();
}



//-------------------------------------------------------------------------------------------------
// Function		: void FlashWriteArray(unsigned char * DestPtr, unsigned char *SrcPtr, int len)
// Parameters	: DestPtr - pointer to flash memory, SrcPtr - pointer to data for write, len - length of data
// Return		: None
// Description	: Write array into flash memory
//-------------------------------------------------------------------------------------------------
void FlashWriteArray(unsigned char * DestPtr, unsigned char *SrcPtr, int len) {
    _DINT();
	do {
		FCTL3 = FWKEY;			// Lock = 0 
		while(1) if(!(BUSY&FCTL3))break;
		FCTL1 = FWKEY | WRT;		// WRT  = 1 
		memcpy(DestPtr, SrcPtr, len);
		while(1) if(!(BUSY&FCTL3))break;
		FCTL1 = FWKEY;			// WRT  = 0 
		FCTL3 = FWKEY | LOCK;		// Lock = 1 
    }
    while(DestPtr[len-1] != SrcPtr[len-1]);
	_EINT();
}


//-------------------------------------------------------------------------------------------------
// Function		: void FlashEraseSegmentSafe(int safe_bytes)
// Parameters	: safe_bytes - number of bytes saved over backup
// Return		: None
// Description	: Erase one segment of flash with saving content into backup
//-------------------------------------------------------------------------------------------------
void FlashEraseSegmentSafe(int safe_bytes) {
	memcpy(SegCBackup, (void *)FLASH_SEG_ADDR, safe_bytes);
	FlashEraseSegment((void *)FLASH_SEG_ADDR);
}

//-------------------------------------------------------------------------------------------------
// Function		: void FlashEraseSegmentSafe(int safe_bytes)
// Parameters	: restore_bytes - number of bytes restored from backup
// Return		: None
// Description	: Write one segment of flash with restoring content from backup
//-------------------------------------------------------------------------------------------------
void FlashWriteSegmentSafe(int restore_bytes) {
	FlashWriteArray((unsigned char *)FLASH_SEG_ADDR, SegCBackup, restore_bytes);
}



//-------------------------------------------------------------------------------------------------
// Function		: void StorageProperty(u16 seg_offset, unsigned char * data, u8 len)
// Parameters	: seg_offset - offset from begin of datasegment, data - ptr to data array, len - length of data
// Return		: None
// Description	: Write array property into flash
//-------------------------------------------------------------------------------------------------
void StorageProperty(u16 seg_offset, unsigned char * data, u8 len) {		 
	FlashInit();
	//
	FlashEraseSegmentSafe(FLASH_LEN);		// Кол-во сохраненных байт
	memcpy(((u8 *)(&SegCBackup[0] + seg_offset)), data, len);
	FlashWriteSegmentSafe(FLASH_LEN);
	//
	FlashDeInit();
}


//-------------------------------------------------------------------------------------------------
// Function		: void StoragePropertyByte(u16 seg_offset, u8 byte)
// Parameters	: seg_offset - offset from begin of datasegment, byte - byte for write to flash
// Return		: None
// Description	: Write word property into flash
//-------------------------------------------------------------------------------------------------
void StoragePropertyByte(u16 seg_offset, u8 byte) {
	u8	data = byte;
	StorageProperty(seg_offset, (u8 *)&data, 1);
}



//-------------------------------------------------------------------------------------------------
// Function		: void StoragePropertyWord(u16 seg_offset, u16 word)
// Parameters	: seg_offset - offset from begin of datasegment, word - word for write to flash
// Return		: None
// Description	: Write word property into flash
//-------------------------------------------------------------------------------------------------
void StoragePropertyWord(u16 seg_offset, u16 word) {
	u16	data = word;
	StorageProperty(seg_offset, (u8 *)&data, 2);
}



//-------------------------------------------------------------------------------------------------
// Function		: u16 GetPropertiesCRC(void)
// Parameters	: None
// Return		: Value of flash data checksum
// Description	: Calculate checksum of the flash data 
//-------------------------------------------------------------------------------------------------
u16 GetPropertiesCS(void) {
	u16		summa = 0;
	u8		i = PROPERTIES_LEN / 2;
	u16 * 	ptr = (u16 *) &ee_config;
	while (i--) {
		summa ^= *ptr++;
	}
	//
	return summa;
}



//-------------------------------------------------------------------------------------------------
// Function		: void SavePropertyCRC(void)
// Parameters	: None
// Return		: None
// Description	: Calculate checksum of the flash data and write into flash
//-------------------------------------------------------------------------------------------------
void SavePropertyCS(void) {
	u16	summa = GetPropertiesCS() ;
	StorageProperty(eeCRC_OFFSET, (u8 *)&summa, 2);
}

// End flash.c