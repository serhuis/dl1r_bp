/*********************************************************************
 *
 *      Definitions of MAIN routine
 *
 *********************************************************************
 * FileName:        main.h
 * Version:			1.00
 *
 * Processor:       MSP430G2xxx
 * Complier:        IAR Workbench for MSP430 v5.50 or higher
 *                  
 * Company:         ARTON
 *
 * Author               Date      	Version	  		Comment
 *--------------------------------------------------------------------
 * Oleg Semeniuk	 16.01.2015    	1.00		Release for v1.00

 ********************************************************************/

#ifndef __MAIN_H__
#define __MAIN_H__

/*********************************************************************************/
/*                                 INCLUDES                                      */
/*********************************************************************************/
#include "type.h"


/*********************************************************************************/
/*                               DEFINITIONS                                     */
/*********************************************************************************/
	 
// Options for preprocessor
#define SYS_FAULT_ENABLE		1	/* 1 - detect System Fault, 0 - no detect System Fault */
#define CRC_ENABLE				1	/* 1 - control memory enable, 0 - control memory disable */
	
#define SYS_TICK_TIME			10000	/* in us */
#define sleep()						__bis_SR_register(CPUOFF + GIE);
	 
// Modes of device
#define	MODE_CALIBR				0x03
#define	MODE_NORM				0x04
#define	MODE_PREPREFIRE			0x05
#define	MODE_PREFIRE			0x06
#define	MODE_FAULT				0x07
#define	MODE_TEST				0x08
#define	MODE_FIRE				0x09
#define	MODE_FAULT_CHAIN		0x0A
	 
// Other constants
#define T50MS_DIV				152		/* DIV from T=1/3200Hz  to T=50ms */
#define VCC_DATA_LEN			8		
#define ADC_CH_DATA_LEN			16
#define TEMP_MEAS_LEN			8
#define BOOST_STOP				1


#define DRIFT_SAMPLES_MAX		(10*60*60)	/* Number samples for define long term drift factor (~10 hours)*/
//#define DRIFT_SAMPLES_MAX		(10*60)		/* Number samples for define long term drift factor (~10 min)*/
#define FIRE_MEAS_COUNT_MAX		3	 		/* Number continuous samples for define smoke */
#define FAULT_CH_COUNT_MAX		3	 		/* Number continuous samples for define chamber fault */

#define SYGNAL_ZERO_LEVEL		10
#define SYGNAL_MIN_VALUE		100
#define SYGNAL_MAX_VALUE		600




typedef enum { REF_1_5V, REF_2_5V, REF_3_3V } Ref_Type;


typedef struct  {
		u16	 bTimer50msOn 	: 1;				/* Закончился 50мс интервал от WDT	*/
		u16	 bTimerA_On		: 1;        		/* End of Timer A period	*/
		u16	 bTimerA_Repeat	: 1;        		/* Flag: Multipulse of timer	*/
		u16	 bButtonDownOn	: 1;        		/* Была нажата кнопка	*/
		u16	 bButtonUpOn	: 1;        		/* Была отжата кнопка	*/
		u16	 bEndOfSamples	: 1;        		/* Flag: End of samples of ADC */
		u16	 bLineDownOn	: 1;				/* For Soft UART */
		u16	 bInterconnect	: 1;        		/* Flag link of Interconnect	*/
		u16	 bHush			: 1;				/* Flag: HUSH mode	*/
		u16	 bWaitStartPulse: 1;				/* Wait start pulse  */
		u16	 bTimerA_Enable	: 1;				/* Flag: Timer A Enable	*/
		u16	 bSendingGraph	: 1;        		/* 	*/
		u16	 bStartPulse	: 1;        		/* Flag: Start pulse is received */
		
}tFlags;


// Низкоуровневые события & flags
#define	fTimer50msOn	f.bTimer50msOn			/* Закончился 50мс интервал от WDT	*/
#define	fTimerA_On		f.bTimerA_On         	/* End of Timer A period	*/
#define	fTimerA_Repeat	f.bTimerA_Repeat       	/* Flag: Multi pulse of timer	*/
#define	fButtonDownOn	f.bButtonDownOn         /* Была нажата кнопка	*/
#define	fButtonUpOn		f.bButtonUpOn           /* Была отжата кнопка	*/
#define	fEndOfSamples	f.bEndOfSamples         /* Flag: End of samples of ADC */
#define	fIRStartPulse	f.bIRStartPulse         /* Flag: IR Start pulse is received */
#define	fRxLineDownOn	f.bLineDownOn			/* For Soft UART */

#define	fInterconnect	f.bInterconnect         /* Flag link of Interconnect	*/
#define	fWaitStartPulse	f.bWaitStartPulse  		/* */
#define	fTimerA_Enable	f.bTimerA_Enable        /* Flag: Timer A Enable	*/

#define	fSendingGraph	f.bSendingGraph         /* Flag: Sending the graph to the PC */

#define	fStartPulse		f.bStartPulse           /* Flag: Start pulse is received */


typedef union {
	 u8	 byte;
	 struct  {
		u8	fStrobNone 		: 1;
		u8	fSignal_Hi		: 1;
		u8	fSignal_Low		: 1;
		u8	fFullOverlapping: 1;
		u8	fFaultDrift		: 1;
		u8	fFaultSWReset	: 1;
		u8	fFaultCRC	: 1;
	 };
}tFault;


typedef union {
	 u8	 byte;
	 struct  {
		u8	fCalibr_Hi		: 1;
		u8	fCalibr_Low		: 1;
	 };
}tCalibrFault;



#define SIGNAL_ARRAY_LEN	100


#define LED_PULSE_5 		0x81020408
#define LED_PULSE_4 		0x81020400
#define LED_PULSE_3 		0x81020000
#define LED_PULSE_2 		0x81000000
#define LED_PULSE_1 		0x80000000


#define LED_PULSE_1F 		0x80000000
#define LED_PULSE_1S 		0x20000000

#define LED_FULL 			0xFFFFFFFF
#define LED_NONE 			0x0



/*********************************************************************************/
/*                        EXTERNAL VARIABLES                                     */
/*********************************************************************************/

extern tFault	DeviceFault;
extern u16 		dark;
extern u16 		delta;
extern u8 		Timer50msCounter;

extern s16		signal_array[SIGNAL_ARRAY_LEN];



/*********************************************************************************/
/*                                FUNCTIONS                                      */
/*********************************************************************************/
// Function prototipes
void DeviceStart(void);
void VLO_TimerCalibr(void);
u16  DefineCurLevel(void);
void SetLimitCompens(void);
void SetLevels(u16 norm);
u8   Smoke_Process_NORM(void);
u8   Smoke_Process_FIRE(void);
void Led_Flash(u16 duration);
void Led_Flash_Fire(u16 duration);
void VREF_On(void);
void VREF_Off(void);
void ADC_Measure(u16 ch, u16 refout, u8 count);
u16  GetVCC(u8 boost_stop);
u16  Get_HV_Value(u16 refout);
void DeviceDiagnostics(void);



#endif  /* __MAIN_H__ */




