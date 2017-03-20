/**********************************************************************************
 *
 *      ASD-10QR MAIN routine
 * 
 **********************************************************************************
 * FileName:        main.c
 * Version:			1.00
 *
 * Processor:       MSP430G2xxx
 * Complier:        IAR Workbench for MSP430 v4.50 or higher
 *                  
 * Company:         ARTON
 *
 * Software License Agreement
 *
 * The software supplied herewith by ARTON Incorporated
 * (the "Company") for its devices is intended and
 * supplied to you, the Company's customer, for use solely and
 * exclusively on ARTON Inc products. The
 * software is owned by the Company and/or its Author, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *
 * Author               Date      	Version	  		Comment
 *--------------------------------------------------------------------------------
 * Oleg Semeniuk	 16.01.2014    	1.00		Release for v1.00
 *
 *********************************************************************************/


/*********************************************************************************/
/*                                 INCLUDES                                      */
/*********************************************************************************/


#include <stdio.h>                    /* standard I/O .h-file                */
#include <string.h>                   /* string and memory functions         */

#include  "hardware.h"
#include  "main.h"
#include  "flash.h"
#include  "type.h"
#include  "delay.h"
#include  "soft_uart.h"


/*********************************************************************************/
/*                                 VARIABLES                                     */
/*********************************************************************************/

__regvar __no_init tFlags 	f 	 @ __R4; 	// Set of state machine flags

//
u16		timerMain;					// Текущее значение главного таймера
u16		timerLongPeriod;			// Таймер для режимa HUSH
u16		mainPeriodCounter;			// Counter of main time period
u16 	timerKeyDown;				// Для отслеживания нажатия на кнопку TEST
u16		adc_data1[ADC_CH_DATA_LEN];	// Array for ADC samples

u8 		DeviceMode; 				// Mode of device

tFault	DeviceFault = {0};			// Current Faults flags
tCalibrFault CalibrFault = {0};		// Calibration Faults flags
u16 	dark;						// Level of dark signal
u16 	delta;						// Current level signal

u16 	delta_last = 0;				// Prev sygnal of delta
u16		zero_timer = 0;				// Zero level signal timer

u8		fault_chamber_counter = 0;	// Counter of series chamber fault
u8  	FireMeasCount = 0;			// Количество замеров, превышающих порог Пожара

// Variables for definition Long Term Drift
u16  	DriftCounter = 0;
u32 	DriftSumma = 0;
u32 	DriftLevel;					// Current Long Term Drift Level
	
//u16 	FireLimit;					// Current Limit of Fire
//u16 	PreFireLimit;				// Current Limit of PreFire
	
u16		CCR1_Value;					// CCR1 PWM duty cycle for sound generation
u8 		Timer50msCounter = 0;		// Counter of 50ms ticks


s16		signal_array[SIGNAL_ARRAY_LEN];

//u8		AMP_Off_Timer = 0;


volatile u32		led_r;
volatile u32		led_y;
volatile u32		led_sh = 0;

u8 		fTimerA1_On = 0;


#define CALIBR_STAGES		6
u8 		calibr_stage;
u16		s_array[CALIBR_STAGES];
u8		q_array[CALIBR_STAGES];

/* Old
const u8 calibr_sequence[CALIBR_STAGES][3] = {
//			 Amp_No	Gain  Reference
			{  2,    3,     0 },
			{  2,    3,     1 },
			{  2,    2,     1 },
			{  2,    1,     1 },
			{  2,    0,     1 },
			{  1,    0,     1 },
			{  1,    0,     2 },
};
*/

const u8 calibr_sequence[CALIBR_STAGES][3] = {
//			 Amp_No	Gain  Reference
			{  1,    0,     2 },
			//{  1,    0,     1 },
			{  2,    0,     1 },
			{  2,    1,     1 },
			{  2,    2,     1 },
			{  2,    3,     1 },
			{  2,    3,     0 },
};


const u8 fault_sequence[4 * 2] = {MODE_FAULT, 10, MODE_NORM, 10, MODE_FIRE, 0, 0};	// 0 = ~~

u8  fault_chain_ind = 0;	// 	
u16 fault_chain_timer = 0;

s8	gain;
u8	amp_no;
u8	reference;
tCfgReg	cfg_reg;

u8	jp1_state = 0;			// 0 - JP1 Open, 1 - JP1 Close

u8	adc_process = 0;		// ADC low level semafore

u16	light_timer = 0;		// For led lighting
u8	light_sync = 0;			// For led lighting syncronisation

u16	strob_pulse_timer = 0;	// Таймер ожидания электрической синхронизации
u8	strob_fault_phase = 0;	// Фаза неисправности электрической синхронизации 0 - отключение БИ, 1 - включение БИ

u16	hi_signal_counter  = 0;	// Very Hight signal counter

u16	fault_timer = 0;		// Fault signal timer
u8  fault_phase  = 0;		// Phase of  fault signal: 1 - break of BI , 0 - enable of  BI	

u16 start_timer = 0;		// Start timer of device

u16 flash_period_timer;		// Flast period timer





/*********************************************************************************/
/*                                FUNCTIONS                                      */
/*********************************************************************************/
// --- Declarations ---
void ADC_MeasureInit(u8 input_no, u8 refer);
u8   SignalAnalysis(void);
void ADC_MeasureStart(void);
void AMP_Calibration(void);

u16  AverageData(u16 * data_ptr, u8 len);
void SetRemoteAlarmMode(u8 sound);
u8   RX_PacketParser(void);
void ADC_Measure(u16 ch, u16 refout, u8 count);
void ADC_Measure_TEMP(u16 ch, u16 refout, u8 count);

void Timer_A1_Init(void);
void Timer_A0_SetDelay(u16 period);
void TimerA1_DelayUs(u16 time);
void Timer_A0_Off(void);
void JP1_Define(void);




//--------------------------------------------------------------------------------
// Function		: void VLO_TimerCalibr(void)
// Parameters	: None
// Return		: None
// Description	: Calculation calibration value of VLO timer
//--------------------------------------------------------------------------------
void VLO_TimerCalibr(void) {
	u16 clk;
	
	// Start timer 1MHz
	TACTL = TASSEL_2 + MC_1 + ID_3;     	 	// SMCLK, up mode  / 8
	//			
	CCR0 = 62500 - 1;	                    // Period 0.5sec
	CCTL1 = 0; 			                    // CCR1 reset/set
	TACCTL0 = CCIE;							// Разрешаем прерывание таймера по достижению значения TACCCR0.
	//
	clk = 0;
	while (1) {
		if (fTimer50msOn) {
			fTimer50msOn = 0;
			clk++;
		}
		if (fTimerA_On) {
			fTimerA_On = 0;
			clk++;
			break;
		}
	}
	//
	//SoundStop();							// Disable interrupts of timer
	TACTL = 0;  
	TACCTL0 = 0;				// Запрещаем прерывание таймера по достижению значения TACCCR0.
	//
	if (clk != CONFIG->timer_calibr) {
		StoragePropertyWord(eeTIMER_CALIBR_OFFSET, clk * 2);
		#if (CRC_ENABLE == 1)
		SavePropertyCS();
		#endif
	}
	//
}


//--------------------------------------------------------------------------------
// Function		: void ClearDriftVar(void)
// Parameters	: None
// Return		: None
// Description	: Clear Long Term Drift variables
//--------------------------------------------------------------------------------
void ClearDriftVar(void) {
	DriftCounter = 0;
	DriftSumma = 0;
	DriftLevel = 0;
}



//--------------------------------------------------------------------------------
// Function		: void SetLimitCompens(void)
// Parameters	: fire_level - fire level in increment
// Return		: None
// Description	: Setting limit compensation level
//--------------------------------------------------------------------------------
void SetLimitCompens(void) {
	u16 norm = CONFIG->limit_norm;
	//
	StoragePropertyWord(eeLIMIT_LONG_DRIFT_OFFSET, norm / 2);
	//
#if (CRC_ENABLE == 1)
	SavePropertyCS();
#endif
	//
	ClearDriftVar();
	//
}



//--------------------------------------------------------------------------------
// Function		: void SetLevels(u16 norm)
// Parameters	: norm - norm sygnal level 
// Return		: None
// Description	: Calculation and setting levels depending on background signal
//--------------------------------------------------------------------------------
void SetLevels(u16 norm) {
	u16 temp;
	u8  d;	
	StoragePropertyWord(eeLIMIT_NORM_OFFSET, norm);
	//
	d = norm * 28 / 100;
	//
	temp = norm - d;		//
	StoragePropertyWord(eeLIMIT_FIRE_OFFSET, temp);
	//
	temp = norm - d * 3 / 4;		//
	StoragePropertyWord(eeLIMIT_PREFIRE_OFFSET, temp);
	
	//
#if (CRC_ENABLE == 1)
	SavePropertyCS();
#endif
	//
	ClearDriftVar();
	//
}



//--------------------------------------------------------------------------------
// Function		: void SetLevels(u16 norm)
// Parameters	: norm - norm sygnal level, nonlinearity_corr - correction 
//				  nonlinearity in case of Hi signal (0 - none, 1 - low correction,
//				  2 - hi correction)
// Return		: None
// Description	: Calculation and setting levels depending on background signal
//--------------------------------------------------------------------------------
void SetLevelsFromCalibr(u16 norm, u8 nonlinearity_corr) {
	u16 temp;
	u8  d;	
	StoragePropertyWord(eeLIMIT_NORM_OFFSET, norm);
	//
	switch (nonlinearity_corr) {
		case 0:
			d = norm * 29 / 100;
			break;
		case 1:
			d = norm * 23 / 100;
			break;
		case 2:
			d = norm * 18 / 100;
			break;
		default:
			d = norm * 29 / 100;
	}
	//
	temp = norm - d;		//
	StoragePropertyWord(eeLIMIT_FIRE_OFFSET, temp);
	//
	temp = norm - d * 3 / 4;		//
	StoragePropertyWord(eeLIMIT_PREFIRE_OFFSET, temp);
	
	//
#if (CRC_ENABLE == 1)
	SavePropertyCS();
#endif
	//
	ClearDriftVar();
	//
}



//--------------------------------------------------------------------------------
// Function		: void DeviceStart(void)
// Parameters	: None
// Return		: None
// Description	: Function executes initialization variable at start of device
//--------------------------------------------------------------------------------
void DeviceStart(void) {

	DeviceFault.byte = 0;		// Reset faults flags
	CalibrFault.byte = 0;		// Reset faults flags
	//		
	VLO_TimerCalibr();			// Calibration VLO Timer
	//
	ClearDriftVar();
	
}


//--------------------------------------------------------------------------------
// Function		: void DefineFireLimit(void)
// Parameters	: None
// Return		: None
// Description	: Determination fire and prefire limits depending on long term drift
//--------------------------------------------------------------------------------
//#pragma optimize=none
u16 getZeroLevel(void) {
	u16 ret;
	//
	ret = CONFIG->limit_norm / 6;
	return ret;
}


//--------------------------------------------------------------------------------
// Function		: u16 getTimerValue(u16 period_sec)
// Parameters	: time_sec : time in sec (0..655)
// Return		: None
// Description	: Getting time in system ticks
//--------------------------------------------------------------------------------
static u16 getTimerValue(u16 time_sec) {
	u16 time = 100;
	
	if (time_sec > 655) return 0;
	
	time *= time_sec;	
	
	return time;

}


//--------------------------------------------------------------------------------
// Function		: void VLO_TimerCalibr(void)
// Parameters	: None
// Return		: None
// Description	: Calculation calibration value of VLO timer
//--------------------------------------------------------------------------------
u16 VLO_GetPeriod(void) {
	
	// Start timer 8MHz
	TACTL = TASSEL_2 + MC_1;           	 	// SMCLK, up mode
	//			
	CCR0 =0xFFFF;                    		// Period 2.5mS
	CCTL1 = 0; 			                    // CCR1 reset/set
	TACCTL0 = 0;							// Разрешаем прерывание таймера по достижению значения TACCCR0.
	//
	while (fTimer50msOn == 0) {}
	fTimer50msOn = 0;
	//while (fTimer50msOn == 0) {}
	
	return TAR;
	
}


//--------------------------------------------------------------------------------
// Function		: void VLO_TimerCalibr(void)
// Parameters	: None
// Return		: None
// Description	: Calculation calibration value of VLO timer
//--------------------------------------------------------------------------------
void AMP_Gain_Init(void) {
	
	AMP_SetGain(CONFIG->Gain);
	
}

#define ADC_AMP1	INCH_1		/* First AMP out */
#define ADC_AMP2	INCH_0		/* Second AMP out */
#define ADC_BUT		INCH_2		/* Tact button */
#define ADC_AE		0x13		/* Analog (Input) Enable Control Register Value + VREF+*/
#define ADC_AE_BUT	0x17		/* Analog (Input) Enable Control Register Value + Tact sw + VREF+*/

#define ADC_FLUCTATION			20
#define SIGNAL_0_LEVEL			512

#define CORREL_LEVEL			120		/* Threshold level to correlations of the start signal */



//--------------------------------------------------------------------------------
// Function		: ADC_Measure(u8 input_no, u8 refer)
// Parameters	: input_no - number AMP output (= 1 or 2), refer - reference voltage (=0 - 1.5V, =1 - 2.5V, =2 - 3.3V)
// Return		: None
// Description	: Функция производит измерения по выбранному каналу ADC и помещает их в массив adc_data1
//				   ~109 kS/s for 16 MHz 
//--------------------------------------------------------------------------------
void ADC_MeasureInit(u8 input_no, u8 refer) {
	
	ADC10CTL0 &= ~ENC;
	while (ADC10CTL1 & BUSY);             	// Wait if ADC10 core is active 
	//
	switch ((Ref_Type)refer) {
		case REF_1_5V:  
			ADC10CTL0 = SREF_1 + ADC10SHT_1 + MSC + REFOUT + REFON + ADC10ON + ADC10IE;				// Sample&hold = 8 x ADC10CLKs
			break;
		case REF_2_5V:  
			ADC10CTL0 = SREF_1 + ADC10SHT_1 + MSC + REFOUT + REFON + ADC10ON + ADC10IE + REF2_5V;	// Sample&hold = 8 x ADC10CLKs
			break;
		case REF_3_3V:  
			ADC10CTL0 = SREF_0 + ADC10SHT_1 + MSC + ADC10ON + ADC10IE  + REFOUT + REFON + REF2_5V;	// Sample&hold = 8 x ADC10CLKs
			break;
	}
	//
	DelayUs(50);	
	//
	ADC10DTC1 = SIGNAL_ARRAY_LEN;	//ADC_SAMPLES_NUMBER;			// count of conversions
	//
	if (input_no == 1) {
		ADC10CTL1 = ADC10SSEL_2 + ADC10DIV_7 + CONSEQ_2 + ADC_AMP1;     // MCLK + ADC10 Clock Divider Select 6 (DIV = 7),  Input 1
	}else{
		ADC10CTL1 = ADC10SSEL_2 + ADC10DIV_7 + CONSEQ_2 + ADC_AMP2;     // MCLK + ADC10 Clock Divider Select 6 (DIV = 7),  Input 2
	}
	
	ADC10AE0 |= ADC_AE;                     // A0 & A1 ADC option select
	//
}


//--------------------------------------------------------------------------------
// Function		: ADC_Measure(u16 ch, u16 refout, u8 count)
// Parameters	: ch - ADC channel, refout - 0 or REFOUT, count - кол-во измерений 
// Return		: None
// Description	: Функция производит измерения по выбранному каналу ADC и помещает их в массив adc_data1
//--------------------------------------------------------------------------------
void ADC_MeasureStart(void) {
	__disable_interrupt();		// Disable interrupt
	
	adc_process = 1;
	
	//ADC10SA = (u16) &signal_array[SIGNAL_ARRAY_LEN - ADC_SAMPLES_NUMBER];      	// Data buffer start - end of sygnal_array
	ADC10SA = (u16) &signal_array[0];	// Data buffer start - end of sygnal_array
	ADC10CTL0 |= ENC + ADC10SC;         // Sampling and conversion start
	//
	
	__enable_interrupt();                     // enable interrupts
}



u16 last_0_level;
u16 last_level;

//--------------------------------------------------------------------------------
// Function		: void VLO_TimerCalibr(void)
// Parameters	: None
// Return		: 0 - no valid array, 1
// Description	: The analysis of the input signal on ADC_SAMPLES_NUMBER samples
//--------------------------------------------------------------------------------
u16 AbsValue(u16 x1, u16 x2) {
	if (x1 >= x2) {
		return (x1 - x2);
	}else{
		return (x2 - x1);
	}
}

/**************************************************************
WinFilter version 0.8
http://www.winfilter.20m.com
akundert@hotmail.com

Filter type: Band Pass
Filter model: Butterworth
Filter order: 8
Sampling Frequency: 109 KHz
Fc1 and Fc2 Frequencies: 3.800000 KHz and 14.000000 KHz
Coefficents Quantization: 8-bit

Z domain Zeros
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = -1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000
z = 1.000000 + j 0.000000

Z domain Poles
z = 0.644814 + j -0.260807
z = 0.644814 + j 0.260807
z = 0.756494 + j -0.196546
z = 0.756494 + j 0.196546
z = 0.587475 + j -0.386644
z = 0.587475 + j 0.386644
z = 0.840396 + j -0.195195
z = 0.840396 + j 0.195195
z = 0.588237 + j -0.523443
z = 0.588237 + j 0.523443
z = 0.901607 + j -0.202202
z = 0.901607 + j 0.202202
z = 0.951542 + j -0.212108
z = 0.951542 + j 0.212108
z = 0.642148 + j -0.658914
z = 0.642148 + j 0.658914
***************************************************************/

#define Ntap 18

// !!! See division by DCgain below
#define DCgain 512	

//const s16 FIRCoef[Ntap] = { 
const s8 FIRCoef[Ntap] = { 
          -13,
          -32,
          -52,
          -59,
          -43,
           -2,
           50,
           95,
          113,
           95,
           50,
           -2,
          -43,
          -59,
          -52,
          -32,
          -13,
           -3
};


static s16 	x[Ntap] = {0};	//, 512, 512, 512 }; 	//input samples

s16 fir_filter(s16 sample) {
    s32	 		y = 0;      //output sample
    int 		i;
	
    // Shift the old samples
    for(i = Ntap-1; i > 0; i--) {
		x[i] = x[i-1];
	}
	
    // Calculate the new output
    x[0] = sample;
    for(i = 0; i < Ntap; i++) {
		y += (s32)FIRCoef[i] * x[i];
	}
	
	if (y >= 0) {
		y = y >> 9;
	}else{
		y = y / DCgain;	
	}
	
    return y;	// / DCgain;
}




//--------------------------------------------------------------------------------
// Function		: void VLO_TimerCalibr(void)
// Parameters	: None
// Return		: 0 - no signal, 1 - signal is existing
// Description	: The analysis of the input signal on ADC_SAMPLES_NUMBER samples
//--------------------------------------------------------------------------------
u8 DefineNewMode(u16 delta) {
	
	return 0;
}


//s16	adc_back1[ADC_SAMPLES_NUMBER];

volatile s16	d1, d2, d3, d4;

#define EXT_ARRAY_LEN	7

//--------------------------------------------------------------------------------
// Function		: void VLO_TimerCalibr(void)
// Parameters	: None
// Return		: 0 - no signal, 1 - signal is existing
// Description	: The analysis of the input signal on ADC_SAMPLES_NUMBER samples
//--------------------------------------------------------------------------------
//#pragma optimize=none
u8 SignalAnalysis(void) {
	int i;
	s16	sample;
	s16 min = 2048, max = -2048;
	int i_min = 0, i_max = 0;
	u8 array_hi[EXT_ARRAY_LEN];
	u8 array_lo[EXT_ARRAY_LEN];
	u8 i_hi = 0;
	u8 i_lo = 0;
	s8 T = 0;
	u8 ret = 0;
	
	// 
    for(i = 1; i < Ntap; i++) {
		x[i] = 0;	//signal_array[i] - 220;
	}
	
	sample = AverageData((u16 *)&signal_array[0], 16);
	
	// Filtering with use FIR
	for (i = 0; i < SIGNAL_ARRAY_LEN; i++) {
		//signal_array[i] = fir_filter(adc_data1[j++] - 512);
		signal_array[i] = fir_filter(signal_array[i] - sample);	
	}
	
	
	// Searching for of importance of the amplitude of the signal on possible gap of time
	for (i = 15; i < SIGNAL_ARRAY_LEN - 2; i++) {
		sample = signal_array[i];
		if (((sample >  signal_array[i - 1]) && (sample >  signal_array[i + 1]) && (sample > signal_array[i - 2]) && (sample > signal_array[i + 2])) ||
		    ((sample >  signal_array[i - 1]) && (sample == signal_array[i + 1]) && (sample > signal_array[i - 2]) && (sample > signal_array[i + 2])) || 
			((sample == signal_array[i - 1]) && (sample == signal_array[i + 1]) && (sample > signal_array[i - 2]) && (sample > signal_array[i + 2])) )   {
	
			if (sample > 10) {	
				array_hi[i_hi] = i;
				if (i_hi < EXT_ARRAY_LEN - 1) i_hi++;
			}
			
			
		}
		
		if (((sample <  signal_array[i - 1]) && (sample <  signal_array[i + 1]) && (sample < signal_array[i - 2]) && (sample < signal_array[i + 2])) ||
		    ((sample <  signal_array[i - 1]) && (sample == signal_array[i + 1]) && (sample < signal_array[i - 2]) && (sample < signal_array[i + 2])) || 
			((sample == signal_array[i - 1]) && (sample == signal_array[i + 1]) && (sample < signal_array[i - 2]) && (sample < signal_array[i + 2])) )   {
		
			if (sample < (-10)) {	
				array_lo[i_lo] = i;
				if (i_lo < EXT_ARRAY_LEN - 1) i_lo++;
			}
			
		}
			
		if (sample < min) {
			min = sample;
			i_min = i;
		}
		if (sample > max) {
			max = sample;
			i_max = i;
		}
	}

	//
	delta = 0;
	
	// Filtering
	/*
	if (i_max > i_min) {
		fError = 1;
	}
	*/
	
	if ((i_lo < 2) || (i_hi < 3)) {
		// Low signal
		ret = 1;
	}else{
		T = array_hi[1] - array_hi[0];
		if ((T < 9) || (T > 15))  {
			// High signal
			ret = 2;
		}
	}
	
	d1 = signal_array[array_hi[0]] - signal_array[array_hi[1]];
	if (((d1 < -50) || (d1 > 50)) && (amp_no == 1)){
		// Wery high signal
		ret = 2;
	}
	
	// Definition of delta
	// Define of first low extremum after first hi extremum
	for (i = 0; i < EXT_ARRAY_LEN; i++) {
		if (array_lo[i] > array_hi[0]) {
			i_hi = 0;
			i_lo = i;
			break;
		}
	}
	//
	/* ////
	i_hi++;
	d1 = signal_array[array_hi[i_hi++]] - signal_array[array_lo[i_lo++]];
	d2 = signal_array[array_hi[i_hi]] - signal_array[array_lo[i_lo]];
	delta = (d1 + d2) / 2;
	*/
	
	//i_lo++;
	d1 = signal_array[array_hi[i_hi++]] - signal_array[array_lo[i_lo]];
	d2 = signal_array[array_hi[i_hi]] - signal_array[array_lo[i_lo++]];
	d3 = signal_array[array_hi[i_hi++]] - signal_array[array_lo[i_lo]];
	d4 = signal_array[array_hi[i_hi]] - signal_array[array_lo[i_lo]];

	delta = (d1 + d2 + d3 + d4) / 4;
	
	if (delta > 4000) {
		delta = 0;	//CONFIG->limit_norm;
	}
	
	if ((DeviceMode == MODE_CALIBR) && ret) {
		delta = 0;
	}
	//
	return ret;
}



//--------------------------------------------------------------------------------
// Function		: void Timer_A0_SetDelay(u16 period)
// Parameters	: period in us (1..65535) - for 8 MHz DCO
// Return		: None
// Description	: Function initiates delay
//--------------------------------------------------------------------------------
void Timer_A0_SetDelay(u16 period) {
	
	if (period == 0) return;
	
	_BIC_SR(GIE);    					// Запрещаем прерывания
	
	fTimerA_Enable = 1;
	fTimerA_Repeat = 0;
	//
	TA0R = 0;
	TACTL 	 = TASSEL_2 + MC_1 + ID_3;  // SMCLK, up mode, div = 8
	CCR0 	 = period - 1;      		// Period T(us) * F(MHz)
	TACCTL0 = CCIE;						// Разрешаем прерывание таймера по достижению значения TACCCR0.
	//
	_BIS_SR(GIE);    					// Разрешаем прерывания
}

//--------------------------------------------------------------------------------
// Function		: void TimerA1_DelayUs(u16 time)
// Parameters	: period in us (1..32768) - for 1 MHz DCO
// Return		: None
// Description	: Function initiates 
//--------------------------------------------------------------------------------
#pragma optimize=none
void TimerA1_DelayUs(u16 time) {
	_BIC_SR(GIE);    					// Запрещаем прерывания
	
	//
	TA1R = 0;
	TA1CTL 	 = TASSEL_2 + MC_1;     	// SMCLK, up mode, div = 8
	TA1CCR0 	 = time - 1 - 35;      	// Period T(us) * F(MHz)
	//
	_BIS_SR(GIE);    					// Разрешаем прерывания
	
	while ((TA1CCTL0 & CCIFG) == 0);
	
	TA1CTL 	 = 0;
	TA1CCTL0 = 0;

	/*
	TA1R = 0;
	TA1CTL 	 = TASSEL_2 + MC_1 + ID_0;  // SMCLK, up mode, div = 1
	TA1CCR0  = SYS_TICK_TIME - 1;   	// Period T(us) * F(MHz)
	*/
}


//--------------------------------------------------------------------------------
// Function		: void Timer_A1_Init(void)
// Parameters	: period in us (1..65535) - for 1 MHz DCO
// Return		: None
// Description	: Function initiates delay
//--------------------------------------------------------------------------------
void Timer_A1_Init(void) {
	_BIC_SR(GIE);    					// Запрещаем прерывания
	//
	TA1R = 0;
	TA1CTL 	 = TASSEL_2 + MC_1 + ID_0;  // SMCLK, up mode, div = 1
	TA1CCR0  = SYS_TICK_TIME - 1;   	// Period T(us) * F(MHz)
	TA1CCTL0 = CCIE;					// Разрешаем прерывание таймера по достижению значения TACCCR0.
	//
	_BIS_SR(GIE);    					// Разрешаем прерывания
}


//--------------------------------------------------------------------------------
// Function		: void SoundStart(u8 snd_ind)
// Parameters	: period in us (1..32768) - for 16 MHz DCO
// Return		: None
// Description	: Function initiates 
//--------------------------------------------------------------------------------
void Timer_A0_Off(void) {
	_BIC_SR(GIE);    			// Запрещаем прерывания
	//	
	fTimerA_Enable = 0;
	fTimerA_Repeat = 0;
	//
	TACTL = 0;  
	TACCTL0 = 0;				// Запрещаем прерывание таймера по достижению значения TACCCR0.
	//
	_BIS_SR(GIE);    			// Разрешаем прерывания

}


//--------------------------------------------------------------------------------
// Function		: void SetFaultMode(void)
// Parameters	: None
// Return		: None
// Description	: 
//--------------------------------------------------------------------------------
void SetFaultMode(void) {
	
	DeviceMode = MODE_FAULT;
	//
	if (fault_timer == 0) { 
		// No faults
		fault_timer = 1000;		// 10 sec
		fault_phase = 1;		// Break of BI
	}
	//
	strob_pulse_timer = 0;		// Clear strob timer because there is breaking BI
}



//--------------------------------------------------------------------------------
// Function		: void FaultSignalManager(void)
// Parameters	: None
// Return		: None
// Description	: Call one time per 10 ms
//--------------------------------------------------------------------------------
void FaultSignalManager(void) {
	
	if ((DeviceMode == MODE_FIRE) || (DeviceMode == MODE_TEST) || (DeviceMode == MODE_CALIBR)) {
		return;
	}
	//
	if (DeviceFault.byte) {
		DeviceMode = MODE_FAULT;
		led_r = 0;
		
		if (DeviceFault.fStrobNone) {
			// Electrical sync is fault
			led_y = LED_PULSE_2;
		}else
		if (DeviceFault.fSignal_Low) {
			// Level signal is very low
			led_y = LED_PULSE_1;
		}else	
		if (DeviceFault.fFaultDrift) {
			// Long Term Drift fault
			led_y = LED_PULSE_3;
		}else	
		if (DeviceFault.fSignal_Hi) {
			// Level signal is very big
			led_y = LED_PULSE_5;
		}
		
	}
	
	if (CalibrFault.byte) {
		// Calibration fault signals
		
		DeviceMode = MODE_FAULT;
		led_r = 0;
		
		if (CalibrFault.fCalibr_Low) {
			led_y = LED_PULSE_4;
		}else
		if (CalibrFault.fCalibr_Hi) {
			led_y = LED_PULSE_5;
		}
		//
		BREAK_ENABLE();				// Disable of BI
		FIRE_CLR();					// Disable Fire current consumption
		//
		fault_timer = 0;
		fault_phase = 0;
	}
		
	if ((DeviceFault.byte == 0) && (CalibrFault.byte == 0)) {
		if (DeviceMode == MODE_FAULT) {
			DeviceMode = MODE_NORM;
			
			led_r = 0;
			led_y = 0;
			
			fault_timer = 0;
			fault_phase = 0;
			
			strob_pulse_timer = 0;
			
			BREAK_DISABLE();		// Enable of BI
			FIRE_CLR();				// Disable Fire current consumption
		}
	}
	//
	if (fault_timer) {
		fault_timer--;
		//
		if (fault_timer == 0) {
			if (fault_phase) {
				fault_phase = 0;
				fault_timer = 2000;		// 20 sec
			}else{
				fault_phase = 1;
				fault_timer = 1000;		// 10 sec
			}
		}
		//
		if (fault_phase == 1) {
			BREAK_ENABLE();			// Disable of BI
		}else{
			BREAK_DISABLE();		// Enable of BI
		}

	}
}


//--------------------------------------------------------------------------------
// Function		: void LedTestValueManager(void)
// Parameters	: None
// Return		: None
// Description	: 
//--------------------------------------------------------------------------------
//#pragma optimize=none
void LedTestValueManager(void) {
	if (delta < 150) {
		led_r = LED_PULSE_1;
		led_y = LED_FULL;
	}else
	if (delta < 250) {
		led_r = LED_PULSE_2;
		led_y = LED_FULL;
	}else
	if (delta < 450) {
		led_r = LED_PULSE_3;
		led_y = LED_FULL;
	}else
	if (delta < 550) {
		led_r = LED_PULSE_4;
		led_y = LED_FULL;
	}else{
		led_r = LED_PULSE_5;
		led_y = LED_FULL;
	}
}


//---------------------------------------------------------------------------------
// Function		: void AMP_SetGain(u8 value)
// Parameters	: None
// Return		: None
// Description	: Initialization interrupts from RX Sniff CC112x (GPIO2)
//---------------------------------------------------------------------------------
void ADC_SetParam(void) {
	
	gain = CONFIG->Gain & 0x0F;
	reference = CONFIG->Gain >> 4;
	amp_no = CONFIG->AMP_No;
	
}

//---------------------------------------------------------------------------------
// Function		: void AMP_SetGain(u8 value)
// Parameters	: None
// Return		: None
// Description	: Initialization interrupts from RX Sniff CC112x (GPIO2)
//---------------------------------------------------------------------------------
#pragma optimize=none
void CalibrationResultAnalise(void) {
	int i;
	u16 prev, temp, min = 0xFFFF, max = 0;
	u8  i_ok = 0xFF, i_ok2 = 0xFF;
	u8  fault = 0;
	u16 d;
	u8  corr = 0;
	
	prev = s_array[0];
	for (i = 0; i < CALIBR_STAGES; i++) {
		temp = s_array[i];
		//
		if (temp > max) {
			max = temp;
		}
		if (temp < min) {
			min = temp;
		}
			
		if (((temp >= 90) && (temp <= 200)) || 
			((temp >= 480) && (temp <= 600))) {
			i_ok = i;
		}
		if ((temp >= 200) && (temp <= 480)) {
			i_ok2 = i;
		}
		//
		if ((prev > 100) && (temp == 0) && (i <= 4)) {
			fault = 1;	// Signal is vefy Hi
		}
		prev = temp;
	}
	//
	if (q_array[0] == 2) {
		// Signal is vefy Hi
		fault = 1;
	}
	//
		
	if (fault) {
		// End of calibration - very Hi signal - ERROR
		//DeviceFault.fSignal_Hi = 1;
		//DeviceMode = MODE_FAULT;
		CalibrFault.fCalibr_Hi = 1;
	}else
	if ((i_ok2 < CALIBR_STAGES) || (i_ok < CALIBR_STAGES)) {
		if (i_ok2 < CALIBR_STAGES) {
			i = i_ok2; 
		}else{
			i = i_ok;
		}
		//
		// Calibration Ok!!!
		//
		d = s_array[i];
		//q = q_array[i]; 
		//
		if (i == 0) {
			if (d >= 480) {
				CalibrFault.fCalibr_Hi = 1;
				StoragePropertyByte(eeCALIBR_FAULT_OFFSET, CalibrFault.byte);
				return;
			}
			//
			if (d >= 330) {
				corr = 2;
			}else
			if (d >= 170) {
				corr = 1;
			}
		}
			
		// That's Ok
		//SetLevels(d);
		SetLevelsFromCalibr(d, corr);
		SetLimitCompens();
		
		amp_no = calibr_sequence[i][0];
		gain = calibr_sequence[i][1];
		reference = calibr_sequence[i][2];
		
		StoragePropertyByte(ee_GAIN, gain + (reference << 4));
		StoragePropertyByte(ee_AMP_NO, amp_no);
		StoragePropertyWord(eeCONFIG_REG_OFFSET, cfg_reg.word);
		//
		DeviceMode = MODE_NORM;
		//...
		FIRE_CLR();				// Disable Fire current consumption
		//
		AMP_SetGain(gain);
		//
	}else
	if (max < SYGNAL_MIN_VALUE) {
		// Signal is Low
		CalibrFault.fCalibr_Low = 1;
	}else
	if (min > SYGNAL_MAX_VALUE) {
		// Signal is Hi
		CalibrFault.fCalibr_Hi = 1;
	}
	//
	FIRE_CLR();				// Disable Fire current consumption
	StoragePropertyByte(eeCALIBR_FAULT_OFFSET, CalibrFault.byte);
}
	

//========================================================
//                 ---  M A I N  ----
//========================================================
//--------------------------------------------------------------------------------
// Function		: void main(void)
// Parameters	: None
// Return		: None
// Description	: Main function. Contains main loop.
//--------------------------------------------------------------------------------
void main(void) {
	u8	byte, sh, len;
	int i;
	u8  loc_buf[2];
	u8  n_counter = 0;
	u8  quality;
	u16	summa = 0;
	u8	fire_level_counter = 0;
	u8	QualityFaultCounter = 0;
	u8	led_clk;
	u16	led_timer = 0;
	u8	timerA1_blank = 0;
	
	// Initialization variables and GPIO
	WDTCTL = WDTPW + WDTHOLD;				// отключаем сторожевой таймер
	// GIPIO Init
	GPIO_Init();
		
	// Init internal RC osc.
	BCSCTL1 = CALBC1_1MHZ; 					// Используем частоту 1 MГц
	DCOCTL =  CALDCO_1MHZ;
	
	DelayMs(100);
	
	// Initialization code for VLO
	__set_R4_register(0);
	//
	BCSCTL3 |= LFXT1S_2;                    // Select VLO as low freq clock
	// End initialization code
	
	WDTCTL = WDT_ADLY_16;                   // Interval timer	/* for 50 ms */
	//WDTCTL = WDT_ADLY_1_9;                   // Interval timer	/* for 5.9 ms */
	IE1 |= WDTIE;                           // Enable WDT interrupt
	//
	if (IFG1 & WDTIFG) {
		// Reset WDT
		#if (SYS_FAULT_ENABLE == 1)
		DeviceFault.fFaultSWReset = 1;
		#endif
	}
	IFG1 = 0;
	//	
	DeviceMode = MODE_NORM;
	
	//!!!!
	TEST2_DIR |= TEST2_BIT;
		
	DelayMs(2000);
	
	Led_Flash(10);
	DelayMs(300);
	Led_Flash(10);
	
	BREAK_DISABLE();					// Enable BI
	DelayMs(7000);
	
	_BIS_SR(GIE);    					// Interrupt enable
	DeviceStart();

	AMP_SetGain(CONFIG->Gain);
	ADC_SetParam();
	cfg_reg = CONFIG->config_reg;
	
	STROB_IE_Enable();
	Timer_A1_Init();
	
	DelayMs(2000);
	
	start_timer = 400;		// 4 sec
	
// *****************************************************************
// ******************   M A I N   L O O P  *************************
// *****************************************************************
	while(1) {
		
		//-------------------------------------------------------------------------------
		//
		// ******** Обработчики событий ********
		//
		//-------------------------------------------------------------------------------
		
//-------------------------------------------------------------------------------
// TimerA0 Event		
//-------------------------------------------------------------------------------
		if (fTimerA_On) {				// Закончен период ожидания измерительных импульсов
			fTimerA_On = 0;
			
			BCSCTL1 = CALBC1_16MHZ; 	// Используем частоту 16 MГц
			DCOCTL =  CALDCO_16MHZ;
			//
			TA1CCR0  = 0xFFFF - 1;   	// ~ Compensation of frequence increase
			//
			Timer_A0_Off();				
			
			ADC_MeasureStart();
		
			timerA1_blank = 4;
		}

//-------------------------------------------------------------------------------
// fTimer50msOn Event				
//-------------------------------------------------------------------------------
		if (fTimer50msOn) {		// Получен следующий 50мс интервал
			fTimer50msOn = 0;
			//
			Timer50msCounter = 0;
		}
		
//-------------------------------------------------------------------------------
// TimerA1 Event (SysTick) 10 ms
//-------------------------------------------------------------------------------
		if (fTimerA1_On) {				// Получен следующий интервал timer
			fTimerA1_On = 0;
			
			if (start_timer) {
				start_timer--;
				if (start_timer == 0) {
					CalibrFault.byte = CONFIG->calibr_fault;
					////if (CalibrFault.byte) {
					////	DeviceMode = MODE_FAULT;
					////}
					//
					flash_period_timer = 500;	// Need flash after 5 sec
					strob_pulse_timer = 0;
				}
			}
			
			//TEST2_OUT ^= TEST2_BIT;
			//
			if (timerKeyDown) {
				timerKeyDown++;
				if (timerKeyDown > 200 ) {			// > 2 sec - Start Calibration
					// CALIBRATION Start
					//
					JP1_Define();			// Define JP1 on BVS state
					if (jp1_state == 0) {
						// JP1 is Open 
						cfg_reg.fDrift = 1;
					}else{
						cfg_reg.fDrift = 0;
					}
					//
					// Calibration mode init 
					DeviceMode = MODE_CALIBR;
					
					// Calibration from most amp to small
					FIRE_SET();				// Set Fire output key
					FAULT_CLR();			// Clear Fault Signal
					BREAK_DISABLE();		// Enable of transmitter
					
					DeviceFault.fStrobNone = 0;
					strob_pulse_timer = 0;
					//
					calibr_stage = 0;
					n_counter = 0;
					summa = 0;
					//
					amp_no = calibr_sequence[calibr_stage][0];
					gain = calibr_sequence[calibr_stage][1];
					reference = calibr_sequence[calibr_stage][2];
					//
					AMP_SetGain(gain);
					
					QualityFaultCounter = 0;
					DeviceFault.byte = 0;
					CalibrFault.byte = 0;
					fault_timer = 0;
					fault_phase = 0;
					
					RED_CLR();
					YEL_CLR();
					
					timerKeyDown = 0;
					
					flash_period_timer = 120;	// Flash after 1.2 sec
					light_timer = 7;			// Value - the power of LED light
				}
			}
			
			if (zero_timer) {
				zero_timer++;
			}
				
			if (fault_chain_timer)  {
				fault_chain_timer--;
				if (DeviceMode != MODE_FIRE) {
					if (fault_chain_timer == 0) {
						if (fault_sequence[fault_chain_ind] == 0) {
							fault_chain_ind = 0;
							DeviceMode = MODE_NORM;
						}else{
							DeviceMode = fault_sequence[fault_chain_ind++];
							fault_chain_timer = getTimerValue(fault_sequence[fault_chain_ind++]);
							//
							switch (DeviceMode) {
								case MODE_FIRE:
									FIRE_SET();
									//
									RED_SET();
									YEL_CLR();
									fault_chain_timer = 0;
									strob_pulse_timer = 0;
									zero_timer = 0;
									BREAK_DISABLE();		// Enable of transmitter
									DeviceFault.fSignal_Low = 0;
									break;
								case MODE_FAULT:
									BREAK_ENABLE();			// Disable of transmitter
									break;
								case MODE_NORM:
									strob_pulse_timer = 0;
									zero_timer = 0;
									DeviceFault.fSignal_Low = 0;
									BREAK_DISABLE();		// Enable of transmitter
							}
						}
					}
				}
			}
			
			if (timerMain) {
				timerMain--;
				if (timerMain == 0) {
					if (DeviceMode == MODE_TEST) {
						DeviceMode = MODE_NORM;
						RED_CLR();
						YEL_CLR();
						FIRE_CLR();
					}
				}
			}
			//
			// Setting signals in Fault Mode (LEDs and other pins)
			FaultSignalManager();
			//
			
			if (timerA1_blank) {
				timerA1_blank--;
			}else{
				// ---- LED Indication ----
				//
				if (light_timer) {
					light_timer--;
					//
					if ((DeviceMode == MODE_NORM) || (DeviceMode == MODE_PREFIRE) || (DeviceMode == MODE_PREPREFIRE)) {
						RED_SET();
						YEL_CLR();
					}else
					//
					if (DeviceMode == MODE_CALIBR) {
						RED_SET();
						YEL_SET();
					}
				}else{
					//
					if ((DeviceMode == MODE_NORM) || (DeviceMode == MODE_CALIBR) || (DeviceMode == MODE_PREFIRE) || (DeviceMode == MODE_PREPREFIRE)) {
						RED_CLR();
						YEL_CLR();
					}
				}
				//	
				if ((DeviceMode == MODE_FAULT) || (DeviceMode == MODE_TEST)) {
					//
					// LED management
					//
					if (light_sync >= 3) {
						light_sync = 0;
						led_sh = 0x80000000;
						//
						led_timer = 300;		// 3 sec period
					}
					
					if (led_timer) {
						led_timer--;
					}else{
						led_timer = 300;		// 3 sec period
						led_sh = 0x80000000;
						//
						light_sync = 0;
					}
					//
					if (++led_clk >= 4) {		// T = 30 ms
						led_clk = 0;
						//
						if (led_r & led_sh) {
							RED_SET();
							TEST2_SET();
						}else{
							RED_CLR();
							TEST2_CLR();
						}
						//
						if (led_y & led_sh) {
							YEL_SET(); 
						}else{
							YEL_CLR();
						}
						//
						if (DeviceMode == MODE_TEST) {
							YEL_SET(); 
						}
						//
						led_sh >>= 1;
					}
				}
			} // End indication
			//
			//
			// Strob pulse diagnostic 
			if ((strob_pulse_timer == 200) || (strob_pulse_timer == 1800)){
				STROB_IE_Enable();
			}
			if (++strob_pulse_timer >= 2000) {	// 20 sec
				strob_pulse_timer = 0;
				if (DeviceFault.fStrobNone == 0) {
					if (CalibrFault.byte == 0) {
						DeviceFault.fStrobNone = 1;
						SetFaultMode();
					}
				}
			}
			//
			if (flash_period_timer) {
				flash_period_timer--;
				if (flash_period_timer == 0) {
					flash_period_timer = 120;	// Flash after 1.2 sec
					if ((DeviceMode == MODE_NORM) || (DeviceMode == MODE_PREPREFIRE)){
						light_timer = 1;				// Value - the light power
					}else
					if (DeviceMode == MODE_CALIBR) {
						light_timer = 7;				// Value - the light power
					}
				}
			}
				
		} // if (fTimerA1_On 10 ms)
		
//-------------------------------------------------------------------------------
// End of samples of ADC Event
		if (fEndOfSamples) {	// End of samples of ADC Event
			fEndOfSamples = 0;
			
			//VREF_Off();						// Vref Disable
			ADC10CTL0 &= ~ENC;
			while (ADC10CTL1 & BUSY);           // Wait if ADC10 core is active 
			ADC10CTL0 &= ~REFON;
			
			AMP_PWR_OFF();						// AMP Power Supply Disable
			//AMP_Off_Timer = CONFIG->timer_calibr - CONFIG->timer_calibr / 4 + 1;	//CONFIG->timer_calibr - CONFIG->timer_calibr / 4 + 1;
			//
			BCSCTL1 = CALBC1_8MHZ; 					// Используем частоту 1 MГц
			DCOCTL =  CALDCO_8MHZ;
			//
			TA1CCR0  = 0xFFFF - 1;   			// ~ Compensation of frequence increase
			//
			DelayUs(1);
			//
			if (fSendingGraph) {
				loc_buf[0] = Q_SEND_GRAPH1;
				loc_buf[1] = SIGNAL_ARRAY_LEN * 2;
				SoftUART_TxString(loc_buf, 2);
				//
				SoftUART_TxString((u8*)&signal_array[0], SIGNAL_ARRAY_LEN * 2);
			}
			//
			//
			if (start_timer || CalibrFault.byte || DeviceFault.fFaultDrift) {
				goto label_light;
			}
			
			STROB_IE_Enable();
			strob_pulse_timer = 0;
		
			//TEST2_SET();
			quality = SignalAnalysis();
			//TEST2_CLR();
			//
			BCSCTL1 = CALBC1_1MHZ; 					// Используем частоту 1 MГц
			DCOCTL =  CALDCO_1MHZ;
			TA1CCR0  = 1 * SYS_TICK_TIME - 1;   	// Period T(us) * F(MHz)
			DelayUs(1);
			//
			//	
			if (DeviceMode == MODE_CALIBR) {
				//
				// -------- CALIBRATION ---------
				//
				n_counter++;
				summa += delta;
				
				if (n_counter >= 2) {
					delta = summa / n_counter;
					n_counter = 0;
					summa = 0;
					//
					s_array[calibr_stage] = delta;
					q_array[calibr_stage] = quality;
					//
					if (++calibr_stage >= CALIBR_STAGES) {
						// End of calibration
						//
						CalibrationResultAnalise();
						if (CalibrFault.byte) {
							DeviceMode = MODE_FAULT;
						}
						//
						goto label_light;
					}
					//
					//	Set new	 Amp_No	Gain  Reference
					amp_no = calibr_sequence[calibr_stage][0];
					gain = calibr_sequence[calibr_stage][1];
					reference = calibr_sequence[calibr_stage][2];
					//
					AMP_SetGain(gain);
					//
				}
			}else
			//
			// -------- NORM behavior -------------
			//
			if (quality == 2) {	
				// Very high signal
				if (++QualityFaultCounter > 10) {
					CalibrFault.fCalibr_Hi = 1;
					QualityFaultCounter = 0;
				}
				//
				goto label_light;
			}else{
				QualityFaultCounter = 0;
			}
			
			if (((DeviceMode == MODE_NORM) || (DeviceMode == MODE_PREPREFIRE)) && fault_chain_timer) {
				// Обрабатывается полное перекрытие
				if (delta > getZeroLevel()) {
					fault_chain_timer = 0;
					fault_chain_ind = 0;
					DeviceFault.fSignal_Low = 0;
				}
			}
			
			if (((DeviceMode == MODE_NORM) || (DeviceMode == MODE_PREPREFIRE)) && (fault_chain_timer == 0)) {
				if ((delta >= CONFIG->limit_fire) && (delta <= SYGNAL_MAX_VALUE)) {
					// Reset all counters
					fire_level_counter = 0;
					hi_signal_counter = 0;
					zero_timer = 0;
				}
								
				if (delta <= getZeroLevel()) {
					if (zero_timer == 0) {	
						zero_timer = 1;
					}else
					if (zero_timer > 1000) {	// 10 sec
						// Полное перекрытие 
						fault_chain_ind = 0;
						DeviceMode = fault_sequence[fault_chain_ind++];
						fault_chain_timer = getTimerValue(fault_sequence[fault_chain_ind++]);
						DeviceFault.fSignal_Low = 1;
						zero_timer = 0;
						//
						BREAK_ENABLE();			// Disable of transmitter
						//
					}
				}else{
					zero_timer = 0;
					//
					if (delta < CONFIG->limit_prefire) {
						DeviceMode = MODE_PREPREFIRE;
					}
					
					if (DeviceMode == MODE_PREPREFIRE) {
						if (delta > CONFIG->limit_prefire) {
							DeviceMode = MODE_NORM;
						}
					}
					
					if (delta < CONFIG->limit_fire) {
						fire_level_counter = 5;
						DeviceMode = MODE_PREFIRE;
					}
					
					// Hi signal management
					if (delta > SYGNAL_MAX_VALUE) {
						// Very high signal
						hi_signal_counter++;
						if (hi_signal_counter >= 10) {
							//DeviceMode = MODE_FAULT;
							CalibrFault.fCalibr_Hi = 1;
							hi_signal_counter = 0;
						}
					}
				}
			}else
			if (DeviceMode == MODE_PREFIRE) {
				//
				// Защита от плавного изменения уровня сигнала
				if ((delta == 0) && (delta_last > 0)) {
					fire_level_counter = 10;
				}
				//	
				if ((delta <= getZeroLevel()) && (fire_level_counter == 5)) {
					if (zero_timer == 0) {	
						zero_timer = 1;
						DeviceMode = MODE_NORM;
					}
				}else
				if (delta < CONFIG->limit_fire) {
					if (--fire_level_counter == 0) {
						DeviceMode = MODE_FIRE;
						FIRE_SET();
						RED_SET();
						YEL_CLR();
						DeviceFault.byte = 0;
					}
				}else{
					DeviceMode = MODE_NORM;
				}
				//
				delta_last = delta;
			}
			//
			if (CONFIG->config_reg.fDrift) {
				// if JP1 state is open
				if ((DeviceMode == MODE_NORM) && delta) {
					// Long Term Drift Adjustment
					DriftSumma += delta;
					DriftCounter++;
					if (DriftCounter >= DRIFT_SAMPLES_MAX) {
						DriftLevel = DriftSumma / DriftCounter;
						//
						if ((u16)DriftLevel < CONFIG->limit_drift) {
							//
							DeviceFault.fFaultDrift = 1;
							SetFaultMode();
							//
							SetLevels(CONFIG->limit_drift);
						}else{
							SetLevels((u16)DriftLevel);
						}
						//	
						ClearDriftVar();
						//
					}
				}
			}
			//			
			//
label_light:			
			//
			BCSCTL1 = CALBC1_1MHZ; 					// Используем частоту 1 MГц
			DCOCTL =  CALDCO_1MHZ;
			TA1CCR0  = 1 * SYS_TICK_TIME - 1;   	// Period T(us) * F(MHz)
			//
			DelayUs(1);
			//
			if (fSendingGraph) {
				//
				BCSCTL1 = CALBC1_8MHZ; 					// Используем частоту 8 MГц
				DCOCTL =  CALDCO_8MHZ;
				TA1CCR0  = 0xFFFF - 1;   				// ~ Compensation of frequence increase
				//
				//
				for (i = 0; i < SIGNAL_ARRAY_LEN; i++) {
					signal_array[i] += 350;
				}
				
				loc_buf[0] = Q_SEND_GRAPH2;
				loc_buf[1] = SIGNAL_ARRAY_LEN * 2;
				SoftUART_TxString(loc_buf, 2);
				//
				SoftUART_TxString((u8*)&signal_array[0], SIGNAL_ARRAY_LEN * 2);
				
				SoftUART_TxString((u8*)&delta, 2);
				
				fSendingGraph = 0;
				//
				BCSCTL1 = CALBC1_1MHZ; 					// Используем частоту 1 MГц
				DCOCTL =  CALDCO_1MHZ;
				TA1CCR0  = 1 * SYS_TICK_TIME - 1;   	// Period T(us) * F(MHz)
				//
			}
			//
			//
			// Indication
			//
			if (DeviceMode == MODE_NORM) {
				light_timer = 1;				// Value - the light power
				flash_period_timer = 120;		// Flash after 1.2 sec
			}
			//
			if ((DeviceMode == MODE_PREFIRE) || (DeviceMode == MODE_PREPREFIRE)) {
				light_timer = 4;				// Value - the light power
				flash_period_timer = 120;		// Flash after 1.2 sec
			}
			//
			if (DeviceMode == MODE_CALIBR) {
				light_timer = 7;				// Value - the light power
				flash_period_timer = 120;		// Flash after 1.2 sec
			}
			//
			if (DeviceMode == MODE_TEST) {
				// Indication sygnal level
				LedTestValueManager();			
				//
			}
			//
			light_sync++;
			led_timer = 400;	//4 sec
			
			STROB_IE_Enable();
		}
		
//-------------------------------------------------------------------------------
// Strobe StartPulse Event		
		if (fStartPulse) {			// Start pulse was received
			fStartPulse = 0;
			//
			AMP_PWR_ON();						// AMP Power Supply Enable
			// Vref Enable
			if (DeviceMode == MODE_CALIBR) {
				ADC_MeasureInit(amp_no, reference);
			}else{
				ADC_MeasureInit(CONFIG->AMP_No, reference);
			}
			
			//TEST2_SET();
			
			_BIC_SR(GIE);    			// Запрещаем прерывания
			Timer_A0_SetDelay(2250);
			_BIS_SR(GIE);    			// Разрешаем прерывания
			//
			BCSCTL1 = CALBC1_8MHZ; 		// Используем частоту 8 MГц
			DCOCTL =  CALDCO_8MHZ;
			//
			TA1CCR0  = 0xFFFF - 1;   	// ~ Compensation of frequence increase
			//
			// For define the presence of strob pulses
			strob_pulse_timer = 0;
			DeviceFault.fStrobNone = 0;
			//
			fault_phase = 0;
			fault_timer = 0;
		}
		

//-------------------------------------------------------------------------------
// KEY DOWN event		
		if (fButtonDownOn) {									// Нажата кнопка
			fButtonDownOn = 0;
			//...
			timerKeyDown = 1;
			//
		} // if (fButtonDownOn)
		
//-------------------------------------------------------------------------------
// KEY UP event
		if (fButtonUpOn) {			// Отжата кнопка
			fButtonUpOn = 0;

			if ((timerKeyDown > 4) && (timerKeyDown < 150))  {	// < 1.5 sec
				// The short pressure on the button
				if ((DeviceMode == MODE_NORM) || (DeviceMode == MODE_PREFIRE) || (DeviceMode == MODE_PREPREFIRE)) {
					DeviceMode = MODE_TEST;
					DeviceFault.byte = 0;	// Reset all faults
					FIRE_SET();				// Set fire mode current consumption
					RED_CLR();
					YEL_SET();
					timerMain = 12000;		// 120 sec
					led_sh = 0;
					led_timer = 0;			// Устраняем задержку оптической индикации
					led_clk = 0xFF;
				}else
				//	
				if (DeviceMode == MODE_TEST) {
					DeviceMode = MODE_NORM;
					DeviceFault.byte = 0;	// Reset all faults
					//led_timer = 0;
					RED_CLR();
					YEL_CLR();
					FIRE_CLR();
				}else
				//
				if (DeviceMode == MODE_FIRE) {
					DeviceMode = MODE_NORM;
					DeviceFault.byte = 0;	// Reset all faults
					RED_CLR();
					YEL_CLR();
					FIRE_CLR();				// Disable Fire Mode consumption
					led_r = 0;
					led_y = 0;
				}
			}
			//
			timerKeyDown = 0;
		} // if (fButtonUpOn) {

		
//-------------------------------------------------------------------------------
// Soft UART Rx Hundler
		if (fRxLineDownOn) {
			uart_rx_ind = 0;
			//
label_rx_continue:
			fRxLineDownOn = 0;
			//
			DelayUs_8MHz(25);		// Delay to center of pulse
			//
			//TEST2_SET();
			//TEST2_CLR();
			//
			if ((INTER_BIT & INTER_IN) == 0) {
				SoftUART_SetReceiveByte();
				
			}else{
				//
				BCSCTL1 = CALBC1_1MHZ; 					// Используем частоту 1 MГц
				DCOCTL =  CALDCO_1MHZ;
				//
				TA1CCR0  = 1 * SYS_TICK_TIME - 1;   	// Period T(us) * F(MHz)
				//
				continue;		// False pulse
			}
			sh = 0x01;
			byte = 0;
			while (sh) {
				if (fTimerA_On) {
					fTimerA_On = 0;
					//
					//TEST2_SET();
					//TEST2_CLR();
					//
					if (INTER_BIT & INTER_IN) {
						byte |= sh;
					}
					sh <<= 1;
				}
			}
			// Receive byte
			SoftUART_ResetReceiveByte();
			//
			if (uart_rx_ind == 0) {
				// Begin of packet
				len = packet_len_table[byte];
				if (len > UART_BUF_LEN) {
					len = UART_BUF_LEN;
				}
			}
				
			*(uart_rx_buf+uart_rx_ind++) = byte;
			if (uart_rx_ind >= len) {
				// Parse of packet
				//...
				SoftUART_RxParse();
				INTER_IFG &= ~INTER_BIT;    				// Обнуляем флаг прерывания
				fRxLineDownOn = 0;
			}else{
				//			
				uart_timer = 4;								// 200 ms timeout to next byte
				while (1) {
					//__bis_SR_register(LPM3_bits + GIE);     // Enter LPM3
					//
					while ((fRxLineDownOn || fTimer50msOn) == 0) {}
					//
					if (fRxLineDownOn) {
						goto label_rx_continue;
					}
					if (fTimer50msOn) {						// Получен следующий 50мс интервал
						fTimer50msOn = 0;
						//
						if (uart_timer) {
							uart_timer--;
						}else{
							// End of Rx timeout
							//...
							INTER_IFG &= ~INTER_BIT;    	// Обнуляем флаг прерывания
							fRxLineDownOn = 0;
							break;
						}
					}
				}
			}
			//
			//
			BCSCTL1 = CALBC1_1MHZ; 					// Используем частоту 1 MГц
			DCOCTL =  CALDCO_1MHZ;
			//
			TA1CCR0  = 1 * SYS_TICK_TIME - 1;   	// Period T(us) * F(MHz)
			//
			//
		} // Soft UART Rx Hundler
	} // while(1)
}




//--------------------------------------------------------------------------------
// Function		: __interrupt void watchdog_timer (void)
// Parameters	: None
// Return		: None
// Description	: WDT Interrupt routine
//--------------------------------------------------------------------------------
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer (void) {
	
	if (fTimer50msOn) {
		if (++Timer50msCounter == 0) {		// > ~ 12sec
			WDTCTL = WDTCTL;				// Hardware RESET
		}
	}
	fTimer50msOn = 1;
	
	__bic_SR_register_on_exit(LPM3_bits);                   // Clear LPM3 bits from 0(SR)
}



//--------------------------------------------------------------------------------
// Function		: __interrupt void CCR0_ISR(void)
// Parameters	: None
// Return		: None
// Description	: TIMER0 Interrupt routine
//--------------------------------------------------------------------------------
#pragma vector = TIMER0_A0_VECTOR
__interrupt void CCR0_ISR(void) {

	fTimerA_On = 1;
	
} // CCR0_ISR



//--------------------------------------------------------------------------------
// Function		: __interrupt void Timer_A1_ISR (void)
// Parameters	: None
// Return		: None
// Description	: TIMER1 Interrupt routine
//--------------------------------------------------------------------------------
#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer_A1_ISR (void)  {
	
	fTimerA1_On = 1;
	//__bic_SR_register_on_exit(LPM0_bits);                   // Clear LPM3 bits from 0(SR)
} 



//--------------------------------------------------------------------------------
// Function		: void Led_Flash(u16 duration)
// Parameters	: duration - duration Red LED flash in ms
// Return		: None
// Description	: Flashing red LED
//--------------------------------------------------------------------------------
void Led_Flash(u16 duration) {
	RED_SET();
	DelayMs(duration);
	RED_CLR();
}





//--------------------------------------------------------------------------------
// Function		: u16 GetVCC(u8 boost_stop)
// Parameters	: boost_stop = 1 - if need call BoostStop() after measure
// Return		: Value in 10mV (for example 250 = 2.50V)
// Description	: Measurement the voltage VCC
//--------------------------------------------------------------------------------
u16 GetVCC(u8 boost_stop) {
	u16 res;	
	ADC_Measure(ADC_CH_VCC, 0, VCC_DATA_LEN);
	//~~~
	VREF_Off();
	//
	//	
	res = AverageData(adc_data1, VCC_DATA_LEN);
	res = res * 64 / 218 + 2;						//~~res = ((u32)res * 301) / 1024 & Compensation dV(R38)=20mV (max=302)
	return (res);			
}




//--------------------------------------------------------------------------------
// Function		: void VREF_On(void)
// Parameters	: None 
// Return		: None
// Description	: Enable VREF
//--------------------------------------------------------------------------------
void VREF_On(void) {

	ADC10CTL0 = REFOUT + REFON + SREF_1 + MSC + ADC10ON;
	ADC10AE0 |= 0x10;                         // P1.4 ADC option select (VRef Out)

}



//--------------------------------------------------------------------------------
// Function		: void VREF_Off(void)
// Parameters	: None
// Return		: None
// Description	: Disable VREF
//--------------------------------------------------------------------------------
void VREF_Off(void) {

	ADC10CTL0 = 0;							// Disable ADC & +VREF
	ADC10CTL0 = 0;							//
}



//--------------------------------------------------------------------------------
// Function		: void ADC10_ISR(void)
// Parameters	: None
// Return		: None
// Description	: ADC10 interrupt service routine
//--------------------------------------------------------------------------------
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void) {

	//ADC10AE0 &= ~0x0F;                      // Save only VRef Out
	
	adc_process = 0;
	
	fEndOfSamples = 1;
	
	//__bic_SR_register_on_exit(CPUOFF);      // Clear CPUOFF bit from 0(SR)
	
}



//--------------------------------------------------------------------------------
// Function		: u16 AverageData(u16 * data_ptr, u8 len)
// Parameters	: data_ptr - data array pointer, len - length of data array
// Return		: Average value of data
// Description	: Fuction calculate the average value of data array
//--------------------------------------------------------------------------------
u16 AverageData(u16 * data_ptr, u8 len) {
	u16 summa = 0;
	u8  i = len;
	while (i--) {
		summa += *data_ptr++;
	}
	//
	return (summa/len);
}


//--------------------------------------------------------------------------------
// Function		: void DeviceDiagnostics(void)
// Parameters	: None
// Return		: None
// Description	: Procedure of the diagnostics device
//--------------------------------------------------------------------------------
void DeviceDiagnostics(void) {
	//
	DelayMs(100);
	//
	//
		
#if (CRC_ENABLE == 1)
	// Check CS of Memory
	DeviceFault.fFaultCRC = 0;
	if (GetPropertiesCS() != CONFIG->CS) {
		DeviceFault.fFaultCRC = 1;
	}
#endif
}

//--------------------------------------------------------------------------------
// Function		: u16 ADC_Measure_Simple(u16 ch, u16* buf, u8 count)
// Parameters	: refout = 0 or REFOUT, vref = REF1_5V , count - кол-во измерений 
// Return		: None
// Description	: Measurement of the current temperature of MCU
//--------------------------------------------------------------------------------
#pragma optimize=none
u16 ADC_Measure_Simple(u16 ch, u16* buf, u8 count) {
	u16 res;

	while(adc_process) {}
	
	BUT_IE &= ~BUT_BIT;
	BUT_REN &= ~BUT_BIT; 		// PULL disable
	
	ADC10CTL0 = SREF_0 + ADC10SHT_1 + MSC  + ADC10ON + ADC10IE;	// Sample&hold = 8 x ADC10CLKs
	
	DelayUs(50);	
	//
	ADC10CTL1 = ADC10SSEL_2 + ADC10DIV_7 + CONSEQ_2 + ch;	// ADC_BUT;     // 
	//
	DelayUs(200);							// Delay to allow Ref to settle

	ADC10DTC1 = count;						// count of conversions
	ADC10AE0 = ADC_AE_BUT;                 // A3 & A7 ADC option select
	//
	ADC10SA = (u16) &buf[0];          		// Data buffer start
	
	fEndOfSamples = 0;
	ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
		
	__bis_SR_register(CPUOFF + GIE);        // LPM0, ADC10_ISR will force exit
	
	while (fEndOfSamples == 0){};
	fEndOfSamples = 0;

	res = AverageData(buf, count);	

	ADC10AE0 = ADC_AE;          // A3 & A7 ADC option select
	
	BUT_IFG &= ~BUT_BIT;    	// Обнуляем флаг прерывания
	BUT_IE |= BUT_BIT;
	BUT_REN |= BUT_BIT; 		// PULL enable
	
	return res;
}


//--------------------------------------------------------------------------------
// Function		: void u8 JP1_Define(void)
// Parameters	: None
// Return		: 0 - JP1 Open, 1 - JP1 Close
// Description	: Definition of JP1 state
//--------------------------------------------------------------------------------
void JP1_Define(void) {
	u16 buf[4];
	u16 res;
	
	res = ADC_Measure_Simple(ADC_BUT, buf, 4);
	
	if (res > 600) return; 
	if (res > 100) {
		jp1_state = 0;		// JP1 is Open 
	}else{
		jp1_state = 1;		// JP1 is Close
	}	
	//
	return;	
}


// End of main.c
