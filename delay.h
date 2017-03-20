/**********************************************************************************
 *
 *      Delays routine definitions
 *
 **********************************************************************************
 * FileName:        delay.h
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

#ifndef __DELAY_H
#define __DELAY_H

/*********************************************************************************/
/*                                FUNCTIONS                                      */
/*********************************************************************************/
void DelayUs_8MHz(unsigned short us);
void DelayUs(unsigned short us);
void DelayMs(unsigned short ms);
void TimerA1_DelayUs(u16 time);
void Delay30Us(void);

#endif



