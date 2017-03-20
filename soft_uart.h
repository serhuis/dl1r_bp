/**********************************************************************************
 *
 *      Definitions for Soft UART routine
 *
 **********************************************************************************
 * FileName:        soft_uart.h
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
#ifndef __SOFT_UART_H__
#define __SOFT_UART_H__

/*********************************************************************************/
/*                                 INCLUDES                                      */
/*********************************************************************************/
#include "type.h"


/*********************************************************************************/
/*                               DEFINITIONS                                     */
/*********************************************************************************/
#define UART_BUF_LEN			40   /* Length input buffer in bytes */
#define UART_TICK_US			104  /* Value for 9600 bps soft UART mode*/ 

	 
// Qualifiers of packet (first byte of packet)
#define Q_COMMAND				1	/* + command (dignostic, reset, flash led, etc.) */
#define Q_GET_MEM				2	/* + addr, len */
#define Q_SET_MEM_WORD			3	/* + addr, byte_lo, byte_hi */
#define Q_SET_MEM_ALL			4	/* + addr, byte1, ..., ... byte_32 */
#define Q_GET_PARAM				5   /* VCC(normal), VCC(load), dark, delta, DriftFactor */
#define Q_GET_GRAPH				6
#define Q_SEND_GRAPH1			7
#define Q_SEND_GRAPH2			8



	 
// Commands of qualifiers Q_COMMAND
#define COMMAND_RESET			1
#define COMMAND_DIAGN			2
#define COMMAND_PROGRAM_ON		3
#define COMMAND_PROGRAM_OFF		4
#define COMMAND_GET_DELTA		5
#define COMMAND_CALIBR_10		7
#define COMMAND_FLASH_LED		8
#define COMMAND_CLEAR_FAULTS	9	
#define COMMAND_CALIBR_00		10
#define COMMAND_GET_RSSI		11



/*********************************************************************************/
/*                        EXTERNAL VARIABLES                                     */
/*********************************************************************************/

extern u8		uart_timer;						// Timer for Soft UART routine
extern u8		uart_rx_buf[UART_BUF_LEN];      // Receiving buffer for Soft UART routine
extern u8		uart_rx_ind;                    // Current cell index of receiving buffer
extern const u8 packet_len_table[];				// The table of packets lengths



/*********************************************************************************/
/*                                FUNCTIONS                                      */
/*********************************************************************************/
void SoftUART_Init(void);	 
void SoftUART_TxMode_ON(void);
void SoftUART_TxMode_OFF(void);
void SoftUART_SendByte(u8 data);
void SoftUART_TxString(u8 * data_ptr, u8 len);
void SoftUART_SetReceiveByte(void);
void SoftUART_ResetReceiveByte(void);
void SoftUART_RxParse(void);




#endif  /* __SOFT_UART_H__ */




