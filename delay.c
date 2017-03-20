/**********************************************************************************
 *
 *      Delays routine
 *
 **********************************************************************************
 * FileName:        delay.c
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
 * Oleg Semeniuk	 20.11.2013    	1.00		Release for v1.00
 *

 *********************************************************************************/

#include "hardware.h"



/*********************************************************************************/
/*                                FUNCTIONS                                      */
/*********************************************************************************/

//-------------------------------------------------------------------------------------------------
// Function		: void DelayUs_8MHz(unsigned short us)
// Parameters	: us = delay in us
// Return		: None
// Description	: Function of the delay for 8MHz
//-------------------------------------------------------------------------------------------------
void DelayUs_8MHz(unsigned short us) {
	while (us--){
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
	}
}


//-------------------------------------------------------------------------------------------------
// Function		: void DelayUs(unsigned short us)
// Parameters	: us = delay in us
// Return		: None
// Description	: Function of the delay for 8MHz
//-------------------------------------------------------------------------------------------------
#pragma optimize=none
void DelayUs(unsigned short us) {
	us >>= 4;
	while (us--){
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		//asm("nop");
		
		
		/*
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		*/
	}
}


#pragma optimize=none
void Delay30Us(void) {
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
}



//-------------------------------------------------------------------------------------------------
// Function		: void DelayMs(unsigned short ms)
// Parameters	: ms = delay in ms
// Return		: None
// Description	: Function of the delay for 8MHz
//-------------------------------------------------------------------------------------------------
void DelayMs(unsigned short ms) {
	while (ms--){
		DelayUs(997);
	}
}



// End delay.c