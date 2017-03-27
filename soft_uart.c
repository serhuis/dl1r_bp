/**********************************************************************************
 *
 *      Soft UART routines
 *
 **********************************************************************************
 * FileName:        soft_uart.c
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
 * Oleg Semeniuk	 27.11.2013    	1.00		Release for v1.00
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
	 
extern __regvar __no_init tFlags 	f 	 @ __R4; 
u8  loc_buf[2];

u8		uart_timer;						// Timer for Soft UART routine
u8		uart_rx_buf[UART_BUF_LEN];		// Receiving buffer for Soft UART routine
u8		uart_rx_ind = 0;				// Current cell index of receiving buffer


// The table of packets lengths
// Index in table = packet qualifier, value = packet length
//									Q_COMMAND 	Q_GET_MEM 	Q_SET_MEM_WORD 	 Q_SET_MEM_ALL  		Q_GET_PARAM

const 	u8 packet_len_table[] = 
// 	Q_COMMAND  Q_GET_MEM    Q_SET_MEM_WORD    Q_SET_MEM_ALL  	Q_GET_PARAM  Q_GET_GRAPH  Q_SEND_GRAPH   
{0,   	2, 			3, 			    4, 	    2+PROPERTIES_LEN, 			1,        1,          2  };

/*
// The table of packets lengths
// Index in table = packet qualifier, value = packet length
//									Q_COMMAND 	Q_GET_MEM 	Q_SET_MEM_WORD 	 Q_SET_MEM_ALL  		Q_GET_PARAM
const 	u8 packet_len_table[] = {0,   	2, 			3, 			  4, 		2+PROPERTIES_LEN, 			1};
*/


/*********************************************************************************/
/*                                FUNCTIONS                                      */
/*********************************************************************************/

//-------------------------------------------------------------------------------------------------
// Function		:  void SoftUART_Init (void)
// Parameters	:  None
// Return		:  None
// Description	:  Инициализация приемопередатчика на 9600 bps 
//-------------------------------------------------------------------------------------------------
void SoftUART_Init(void) {
	//
	INTER_REN |= INTER_BIT; 		// PULL enable
	INTER_OUT |= INTER_BIT;			// PULLUP Resistor 
	
	INTER_DIR &= ~(INTER_BIT);
	INTER_IES |= INTER_BIT;  		// прерывание по переходу из 1 в 0, 
    
	INTER_IFG &= ~INTER_BIT; 		// Для предотвращения немедленного срабатывания прерывания,
             						// обнуляем его флаг для P1.3 до разрешения прерываний
	INTER_IE |= INTER_BIT;   		// Разрешаем прерывания для P1.3

}


//-------------------------------------------------------------------------------------------------
// Function		:  void SoftUART_TxMode_ON (void)
// Parameters	:  None 
// Return		:  None 
// Description	:  Перевод Soft UART в режим передачи
//-------------------------------------------------------------------------------------------------
void SoftUART_TxMode_ON(void) {

	_BIC_SR(GIE);    			// Disable interrupts
	DelayUs_8MHz(200);
	INTER_SET();				// Устанавливаем 1 для UART
	INTER_REN &= ~INTER_BIT; 	// PULL disable
	INTER_DIR |= INTER_BIT;		// Готовим UART к работе
	DelayUs_8MHz(100);
	
}



//-------------------------------------------------------------------------------------------------
// Function		:  void SoftUART_TxMode_OFF (void) 
// Parameters	:  None 
// Return		:  None  
// Description	:  Выход Soft UART из режима передачи
//-------------------------------------------------------------------------------------------------
void SoftUART_TxMode_OFF(void) {
	
	INTER_REN |= INTER_BIT; 	// PULL enable
	INTER_DIR &= ~INTER_BIT;	// Готовим UART к работе на прием
	
	_BIS_SR(GIE);    			// Enable interrupts
}



//-------------------------------------------------------------------------------------------------
// Function		:  void SoftUART_SendByte(u8 data) 
// Parameters	:  data - byte for transmite 
// Return		:  None   
// Description	:  Передача байта через Soft UART
//-------------------------------------------------------------------------------------------------
void SoftUART_SendByte(u8 data) {
	u8  sh = 0x01; 

	INTER_CLR();
	DelayUs_8MHz(UART_TICK_US);
	while (sh) {
		if (data & sh) {
			INTER_SET();
		}else{
			INTER_CLR();
		}
		DelayUs_8MHz(UART_TICK_US);
		sh <<= 1;
	}
	INTER_SET();
	DelayUs_8MHz(UART_TICK_US * 2);
	//
	fTimer50msOn = 0;		// Reset WDT timer flag
}
	


//-------------------------------------------------------------------------------------------------
// Function		:  void SoftUART_SetReceiveByte(void)
// Parameters	:  None
// Return		:  None
// Description	:  Установка режима приема Soft UARt
//-------------------------------------------------------------------------------------------------
void SoftUART_SetReceiveByte(void) {
	
	INTER_IE &= ~INTER_BIT;   		// Disable Port Interrupt
	
	TA0R = 0;
	// Start timer 8MHz
	TACTL 	= TASSEL_2 + MC_1;          // SMCLK, up mode
	//			
	CCR0 	= 104 * 8 - 1;            // Period 104 us
	CCTL1 	= 0; 			          // CCR1 reset/set disable

	fTimerA_Repeat = 1;
	
	TACCTL0 = CCIE;					  // Разрешаем прерывание таймера по достижению значения TACCCR0.

}
	


//-------------------------------------------------------------------------------------------------
// Function		: void SoftUART_ResetReceiveByte(void)
// Parameters	: None
// Return		: None
// Description	: Выход из режима приема Soft UARt
//-------------------------------------------------------------------------------------------------
void SoftUART_ResetReceiveByte(void) {
	//
	TACCTL0 = 0;					// Disable прерывание таймера по достижению значения TACCCR0.
	TACTL = 0;     
	//
	INTER_IFG &= ~INTER_BIT; 		// Для предотвращения немедленного срабатывания прерывания,
             						// обнуляем его флаг до разрешения прерываний
	INTER_IE |= INTER_BIT;   		// Enable Port Interrupt
	
	fTimerA_Repeat = 0;
}



//-------------------------------------------------------------------------------------------------
// Function		: void SoftUART_TxString(u8 * data_ptr, u8 len)
// Parameters	: data_ptr - pointer to data for transmitting, len - trnasmitting length (in bytes)
// Return		: None
// Description	: Передача буфера с данными с указанной длинной через Soft UART
//-------------------------------------------------------------------------------------------------
void SoftUART_TxString(u8 * data_ptr, u8 len) {
	
	SoftUART_TxMode_ON();
	//
	while (len--) {
		SoftUART_SendByte(*data_ptr++);
	}
	//
	SoftUART_TxMode_OFF();
}

//-------------------------------------------------------------------------------------------------
// Function		: void SoftUART_TxString(u8 * data_ptr, u8 len)
// Parameters	: data_ptr - pointer to data for transmitting, len - trnasmitting length (in bytes)
// Return		: None
// Description	: Передача буфера с данными с указанной длинной через Soft UART
//-------------------------------------------------------------------------------------------------
void SoftUART_TxChar(u8 ch) {
	
	SoftUART_TxMode_ON();
	//
	SoftUART_SendByte(ch);
	//
	SoftUART_TxMode_OFF();
}



//-------------------------------------------------------------------------------------------------
// Function		: void SoftUART_RxParse(void)
// Parameters	: None
// Return		: None
// Description	: Parsing of the input package
//-------------------------------------------------------------------------------------------------
void SoftUART_RxParse(void) {
	u8 	qualif = uart_rx_buf[0];
	u8 	len;
	u8 	tmp1 = uart_rx_buf[1];
	//
	fInterconnect = 1;
	//
	switch (qualif) {
		case Q_COMMAND:
			switch (tmp1) {
				case COMMAND_RESET:
					// Hardware RESET
					(*(void(*)(void))(*(unsigned int*)0xfffe))();
					break;
				case COMMAND_DIAGN:
					//BoostStop();
					DeviceDiagnostics();
					SoftUART_TxString((u8*)&DeviceFault, 20);
					break;
				case COMMAND_GET_DELTA:
					SoftUART_TxString((u8*)&dark, 4);
					break;
				case COMMAND_CALIBR_10:
					//res = DefineCurLevel();		// Define Norm level
					//SetFireLevels(res);
					break;
				case COMMAND_CALIBR_00:
					VLO_TimerCalibr();
					SetLevels(delta);
					SetLimitCompens();
					break;
			}
			break;
		case Q_GET_MEM:
			len = uart_rx_buf[2];
			SoftUART_TxString(((u8*)CONFIG + tmp1), len);
			break;
		case Q_SET_MEM_ALL:
			StorageProperty(tmp1, &uart_rx_buf[2], PROPERTIES_LEN);
#if (CRC_ENABLE == 1)	
			SavePropertyCS();
#endif
			break;
		case Q_GET_GRAPH:
			
			fSendingGraph = 1;
			
			
			loc_buf[0] = Q_SEND_GRAPH1;
			loc_buf[1] = SIGNAL_ARRAY_LEN * 2;
			SoftUART_TxString(loc_buf, 2);
			//
			SoftUART_TxString((u8*)&signal_array[0], SIGNAL_ARRAY_LEN * 2);
			
			
			break;
	}
	//
	fInterconnect = 0;
	//
}


// End soft_uart.c

