/**********************************************************************************
 *
 *      Hardware routines
 *
 **********************************************************************************
 * FileName:        hardware.c
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
 * Oleg Semeniuk	 16.11.2013    	1.00		Release for v1.00
 *

 *********************************************************************************/
/*********************************************************************************/
/*                                 INCLUDES                                      */
/*********************************************************************************/
#include  "hardware.h"
#include  "main.h"
#include  "delay.h"
/*********************************************************************************/
/*                                 VARIABLES                                     */
/*********************************************************************************/
	 
extern __regvar __no_init tFlags 	f 	 @ __R4; 

static u8 i, strob, but;


/*********************************************************************************/
/*                                FUNCTIONS                                      */
/*********************************************************************************/



//--------------------------------------------------------------------------------
// Function		: __interrupt void PORT1_ISR(void)
// Parameters	: None
// Return		: None
// Description	: GPIO Interrupt routine for Soft UART & Button Press 
//--------------------------------------------------------------------------------
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void) {   
	/*
	if (BUT_IFG & BUT_BIT) {
		//BUT_IFG &= ~BUT_BIT;    		// Обнуляем флаг прерывания
        //
		if (BUT_IES & BUT_BIT) {		// Спадающий фронт
			but = 1;
			for (i = 0; i < 8; i++) {
				if (BUT_IN & BUT_BIT) {
					but = 0;
					break;
				}
				DelayUs(200);
			}
			if (but == 1) {
				BUT_IES &= ~BUT_BIT;
				fButtonDownOn = 1;
			}
		}else{
			but = 1;
			for (i = 0; i < 4; i++) {
				if ((BUT_IN & BUT_BIT) == 0) {
					but = 0;
					break;
				}
				DelayUs(100);
			}
			if (but == 1) {
				BUT_IES |= BUT_BIT;
				fButtonUpOn = 1;
			}
		}
		//
		BUT_IFG &= ~BUT_BIT;    		// Обнуляем флаг прерывания
		//
		__bic_SR_register_on_exit(LPM3_bits);               // Clear LPM3 bits from 0(SR) Просыпаемся
	}
*/
}


//--------------------------------------------------------------------------------
// Function		: __interrupt void PORT2_ISR(void)
// Parameters	: None
// Return		: None
// Description	: GPIO Interrupt routine for RF Button Press 
//--------------------------------------------------------------------------------
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void) {   
	
	//
	if ((INTER_IE & INTER_BIT) && (INTER_IFG & INTER_BIT)) {
		INTER_IFG &= ~INTER_BIT;    			// Обнуляем флаг прерывания
        //
		BCSCTL1 = CALBC1_8MHZ; 					// Используем частоту 8 MГц
		DCOCTL =  CALDCO_8MHZ;
		//
		fRxLineDownOn = 1;
		//
		__bic_SR_register_on_exit(LPM3_bits);   // Clear LPM3 bits from 0(SR) Просыпаемся
		
	}
	//
	if ((STROB_IE & STROB_BIT) && (STROB_IFG & STROB_BIT)) {
		//
		strob = 1;
		for (i = 0; i < 4; i++) {
			if ((STROB_IN & STROB_BIT) == 0) {
				strob = 0;
				break;
			}
			//DelayUs(1);
		}
		
		if (strob) {
			fStartPulse = strob;					// Set flag
			STROB_IE_Disable();
		}else{
			fStartPulse = strob;					// Set flag
		}
			
		STROB_IFG &= ~STROB_BIT;    		// Clear ISR flag
			
		__bic_SR_register_on_exit(LPM3_bits);   // Clear LPM3 bits from 0(SR) Просыпаемся
	}

	//
	if (BUT_IFG & BUT_BIT) {
			//BUT_IFG &= ~BUT_BIT;    		// Обнуляем флаг прерывания
					//
			if (BUT_IES & BUT_BIT) {		// Спадающий фронт
				but = 1;
				for (i = 0; i < 8; i++) {
					if (BUT_IN & BUT_BIT) {
						but = 0;
						break;
					}
					DelayUs(200);
				}
				if (but == 1) {
					BUT_IES &= ~BUT_BIT;
					fButtonDownOn = 1;
				}
			}else{
				but = 1;
				for (i = 0; i < 4; i++) {
					if ((BUT_IN & BUT_BIT) == 0) {
						but = 0;
						break;
					}
					DelayUs(100);
				}
				if (but == 1) {
					BUT_IES |= BUT_BIT;
					fButtonUpOn = 1;
				}
			}
			//
			BUT_IFG &= ~BUT_BIT;    		// Обнуляем флаг прерывания
			//
			__bic_SR_register_on_exit(LPM3_bits);               // Clear LPM3 bits from 0(SR) Просыпаемся
	}
}


//--------------------------------------------------------------------------------
// Function		: void STROB_IE_Enable(void)
// Parameters	: None
// Return		: None
// Description	: GPIO Interrupt routine for RF Button Press 
//--------------------------------------------------------------------------------
void STROB_IE_Enable(void) {   
	_BIC_SR(GIE);    					// Disable interrupts
	//
	STROB_IFG &= ~STROB_BIT;    		// Clear ISR flag
	STROB_IE |=   STROB_BIT;

	//fStartPulse = 0;					// Clear app flag
	
	_BIS_SR(GIE);    					// Enanble interrupts
	
}


//--------------------------------------------------------------------------------
// Function		: void STROB_IE_Enable(void)
// Parameters	: None
// Return		: None
// Description	: GPIO Interrupt routine for RF Button Press 
//--------------------------------------------------------------------------------
void STROB_IE_Disable(void) {   
	//
	STROB_IE &= ~ STROB_BIT;
	
}



//---------------------------------------------------------------------------------
// Function		: void TEST_BUT_INT_Init(void)
// Parameters	: None
// Return		: None
// Description	: Initialization interrupts from TEST Button
//---------------------------------------------------------------------------------
void BUT_INT_Init(void) {
	//
	BUT_REN |= BUT_BIT; 		// PULL enable
	BUT_OUT |= BUT_BIT;			// PULLUP Resistor 
	
	BUT_DIR &= ~BUT_BIT;
	BUT_IES |= BUT_BIT;  		// прерывание по переходу из 1 в 0, 
                				// устанавливается соответствующим битом IES.x = 1.
	BUT_IFG &= ~BUT_BIT; 		// Для предотвращения немедленного срабатывания прерывания,
                				// обнуляем его флаг для P1.3 до разрешения прерываний
	BUT_IE  |= BUT_BIT;   		// Разрешаем прерывания для P1.3
}


//---------------------------------------------------------------------------------
// Function		: void INTER_INT_Init(void)
// Parameters	: None
// Return		: None
// Description	: Initialization interrupts from Intercom (Soft UART)
//---------------------------------------------------------------------------------
void INTER_INT_Init(void) {
	//
	INTER_REN |= INTER_BIT; 		// PULL enable
	INTER_OUT |= INTER_BIT;			// PULLUP Resistor 
	
	INTER_DIR &= ~INTER_BIT;
	INTER_IES |= INTER_BIT;  		// прерывание по переходу из 1 в 0, 
                					// устанавливается соответствующим битом IES.x = 1.
	INTER_IFG &= ~INTER_BIT; 		// Для предотвращения немедленного срабатывания прерывания,
                					// обнуляем его флаг для P1.3 до разрешения прерываний
	INTER_IE  |= INTER_BIT;   		// Разрешаем прерывания для P1.3
}



//---------------------------------------------------------------------------------
// Function		: void SNIFF_IRQ_INT_Init(void)
// Parameters	: None
// Return		: None
// Description	: Initialization interrupts from RX Sniff CC112x (GPIO2)
//---------------------------------------------------------------------------------
void STROB_IRQ_Init(void) {
	//
	//STROB_REN |= STROB_BIT; 		// PULL enable
	//STROB_OUT &= ~STROB_BIT;		// PULLDOWN Resistor 
			
	STROB_DIR &= ~STROB_BIT;		
	STROB_IES &= ~STROB_BIT;  		// прерывание по переходу из 0 в 1, 

	STROB_IFG &= ~STROB_BIT; 		// Для предотвращения немедленного срабатывания прерывания,
	//STROB_IE  |= STROB_BIT;   		// Разрешаем прерывания
	STROB_IE_Disable();
}



//---------------------------------------------------------------------------------
// Function		: void AMP_SetGain(u8 value)
// Parameters	: None
// Return		: None
// Description	: Initialization interrupts from RX Sniff CC112x (GPIO2)
//---------------------------------------------------------------------------------
void AMP_SetGain(u8 value) {
	
	GAIN_1_CLR();
	GAIN_2_CLR();
	
	if (value & 1) GAIN_1_SET();
	if (value & 2) GAIN_2_SET();
	
	/*
	GAIN_1_OUT &= ~(GAIN_1_BIT | GAIN_2_BIT);
	
	bits =  (value & 1) ? GAIN_1_BIT : 0;
	bits |= (value & 2) ? GAIN_2_BIT : 0;
	
	GAIN_1_OUT |= bits;
	*/
}


//---------------------------------------------------------------------------------
// Function		: void GPIO_Init(void)
// Parameters	: None
// Return		: None
// Description	: Initialization GPIO 
//---------------------------------------------------------------------------------
void GPIO_Init(void) {

	P1OUT = 0;	
	P2OUT = 0;
	
	P1DIR = 0;
	P2DIR = 0;
	
	AMP_PWR_DIR 	|= AMP_PWR_BIT;
	VREF_DIR		|= VREF_BIT;
	RED_DIR 		|= RED_BIT;
	YEL_DIR 		|= YEL_BIT;
	FIRE_DIR 		|= FIRE_BIT;
	FAULT_DIR 		|= FAULT_BIT;
	BREAK_DIR 		|= BREAK_BIT;
	VREF_DIR 		|= VREF_BIT;
	
	VREF_OUT 		|= VREF_BIT;

	GAIN_1_DIR 		|= GAIN_1_BIT;
	GAIN_2_DIR 		|= GAIN_2_BIT;
	
	P1SEL = 0;
	P2SEL = 0;
	
	BUT_INT_Init();			// Init interrupts from TEST button 
	INTER_INT_Init();				// Init interrupts from Intercom
	STROB_IRQ_Init();
}

// End hardware.c
