/**********************************************************************************
 *
 *      Definitions of types
 *
 **********************************************************************************
 * FileName:        type.h
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
 * Oleg Semeniuk	 12.11.2013    	1.00		Release for v1.00
 *

 *********************************************************************************/

#ifndef __TYPE_H__
#define __TYPE_H__

#ifndef NULL
	#define NULL    ((void *)0)
#endif

#ifndef __FALSE
	#define __FALSE   (0)
#endif

#ifndef __TRUE
	#define __TRUE    (1)
#endif

#ifndef FALSE
	#define FALSE   (0)
#endif

#ifndef TRUE
	#define TRUE    (1)
#endif


typedef unsigned char      u8;
typedef signed char        s8;
typedef short              s16;
typedef unsigned short     u16;
typedef signed long        s32;
typedef unsigned long      u32;
typedef long long          s64;
typedef unsigned long long u64;
typedef unsigned char      BIT;
typedef unsigned int       BOOL;



#endif  /* __TYPE_H__ */




