// CDDL HEADER START
//*************************************************************************************************
//                                     Copyright © 2006-2009
//                                           Itron, Inc.
//                                      All Rights Reserved.
//
//
// The contents of this file, and the files included with this file, are subject to the current 
// version of Common Development and Distribution License, Version 1.0 only (the “License”), which 
// can be obtained at http://www.sun.com/cddl/cddl.html. You may not use this file except in 
// compliance with the License. 
//
// The original code, and all software distributed under the License, are distributed and made 
// available on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED. ITRON, 
// INC. HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF 
// MERCHANTABILITY, FITNESS FOR PARTICULAR PURPOSE, OR NON INFRINGEMENT. Please see the License 
// for the specific language governing rights and limitations under the License.
//
// When distributing Covered Software, include this CDDL Header in each file and include the 
// License file at http://www.sun.com/cddl/cddl.html.  If applicable, add the following below this 
// CDDL HEADER, with the fields enclosed by brackets “[   ]” replaced with your own identifying 
// information: 
//		Portions Copyright [yyyy]  [name of copyright owner]
//
//*************************************************************************************************
// CDDL HEADER END
               


#ifndef UART_H
#define UART_H

//#include "slbtypes.h"
//#include "proc.h"
//#include "MCU.h"
//#include "semaphore.h"

#ifdef __BCPLUSPLUS__
#define EXTERN_C    extern "C"
#else
#define EXTERN_C    extern
#endif



//*************************************
//** Min and Max definitions
//*************************************
#define MAX_PORTS           1


//** M16C uart definitions
#define UARTMR_SERIALMODE 			0x07
#define UARTMR_CLOCKDIR 			0x08
#define UARTMR_STOPBITSELECT 		0x10
#define UARTMR_PARITYSELECT 		0x20
#define UARTMR_PARITYENABLE 		0x40
#define UARTMR_IOPOLOLARITYREVERSE 	0x80
#define UARTMR_SLEEPMODE            0x80

#define UARTMR_7DATABITS			4
#define UARTMR_8DATABITS            5
#define UARTMR_MUSTBEZERO           0
#define UARTMR_ONESTOPBIT           0
#define UARTMR_TWOSTOPBITS          1
#define UARTMR_ODDPARITY            0
#define UARTMR_EVENPARITY           1
#define UARTMR_PARITYENABLED        1
#define UARTMR_PARITYDISABLED       0
#define UARTMR_NOREVERSEPOLARITY    0
#define UARTMR_REVERSEPOLARITY      1
#define UARTMR_INTERNALCLOCK        0
#define UARTMR_SLEEPMODEDESELECTED  0

#define UARTC0_BRGSOURCE 			0x03
#define UARTC0_CTSRTSSELECT 		0x04
#define UARTC0_TXEMPTYFLAG 			0x08
#define UARTC0_CTSRTSDISABLE 		0x10
#define UARTC0_DATAOUTPUTSELECT 	0x20
#define UARTC0_CLOCKPOLARITYSELECT 	0x40
#define UARTC0_TRANSFERFORMATSELECT 0x80

#define UARTC0_BRGF1 				0
#define UARTC0_BRGF8 				1
#define UARTC0_BRGF32 				2
#define UARTC0_CTSNOTSELECTED 		0
#define UARTC0_RTSNOTSELECTED 		1
#define UARTC0_CTSRTSENABLED 		0
#define UARTC0_CTSRTSDISABLED 		1
#define UARTC0_TXDPINISCMOSOUTPUT 	0
#define UARTC0_TXDPINISNCHANNELOPENDRAINOUTPUT 1
#define UARTC0_MUSTBEZERO           0
#define UARTC0_LSBFIRST             0
#define UARTC0_MSBFIRST             1

#define UARTC1_TXENABLE 			0x01
#define UARTC1_TXBUFFEREMPTY 		0x02
#define UARTC1_RXENABLE 			0x04
#define UARTC1_RXCOMPLETE 			0x08

// UART 2 ONLY
#define UARTC1_TXINTERRUPTCAUSE 	0x10
#define UARTC1_REVERSELOGICSELECT  	0x40
#define UARTC1_ERROROUTENABLE   	0x80

#define UARTC1_INTERRUPTCAUSETXBUFFEREMPTY        0
#define UARTC1_INTERRUPTCAUSETXCOMPLETE           1

#define UARTRB1_OVERRUN 0x10
#define UARTRB1_FRAMING 0x20
#define UARTRB1_PARITY  0x40

//*************************************
//** Configuration definitions
//*************************************

#define UART_PARITY_ODD      0
#define UART_PARITY_EVEN     1
#define UART_PARITY_NONE     2

#define UART_DATABITS_7      0
#define UART_DATABITS_8      1

#define UART_STOPBITS_1      0
#define UART_STOPBITS_2      1

#define CTS_DISABLE     0
#define CTS_ENABLE      1

#define RX_BUFF			0
#define TX_BUFF			1

//*************************************
//** UART error bits
//*************************************
//    7  6  5  4  3  2  1  0
//    |  |  |  |  |  |  |  +---- The ring buffer has overflowed
//    |  |  |  |  |  |  +-------
//    |  |  |  |  |  +----------
//    |  |  |  |  +-------------
//    |  |  |  +---------------- Byte Overrun error Intercharacter timeout has expired
//    |  |  +------------------- Byte Framing error
//    |  +---------------------- Byte Parity error
//    +------------------------- One of the errors has occured (SUM)
//
typedef Unsigned8 UARTError;

#if 0
// these are not used
#define URTERR_OVERRUN_MASK         0x01
#define URTERR_FRAME_MASK           0x02
#define URTERR_PARITY_MASK          0x04
#define URTERR_SUM_MASK             0x08
#define URTERR_TIMEOUT_MASK         0x10
#define URTERR_RINGOVRFLO_MASK      0x20
#endif

//The number of the timer for each port that is used to time
//intercharacter delays
#define UART_ICTO_TIMER     0
// timer 1 and 2 are used by psem
#define UART_RTS_TIMER      3

typedef Unsigned16 UartTimerType;

//The default number of milliseconds allowed between characters
#define UART_ICTO_TIME      500

#define UART_DEFAULT_RX_REQUEST_SIZE    128

//
//** UART Macros
#if 0
#define UART_RefreshPortTimer(PortNum,TimerNum,ticks)              \
    (UART_Timers[(PortNum)][(TimerNum)] = (UartTimerType)(ticks))
#define UART_PortHasTimedout(PortNum)                              \
    (Ports[PortNum].ExpiredTimers > 0 ? TRUE : FALSE) 
#define UART_ClearPortTimers(PortNum)                              \
	(Ports[PortNum].ExpiredTimers = 0)
#define UART_ClearPortTimer(PortNum,Timer)                         \
	(Ports[PortNum].ExpiredTimers &= ~(1 << (Timer)))
#define UART_PortTimerIsActive(PortNum,Timer)                      \
	((UART_ActiveTimers & (((Unsigned16)0x0001) << (((PortNum) * MAX_PORT_TIMERS) +  (Timer)))) > 0)
#define UART_PortTimerHasTimedout(PortNum,TimerNum)                \
	((Ports[PortNum].ExpiredTimers & (1 << (TimerNum))) > 0)
#define UART_TimerExpired(PortNum,TimerNum)                        \
    (Ports[PortNum].ExpiredTimers |= ((Unsigned16)0x0001 << (TimerNum)))
#endif

//*************************************
//** UART function prototypes
//*************************************
EXTERN_C void UART_Init(void);
EXTERN_C Unsigned8 UART_GetByte(Unsigned8 PortNum);
//EXTERN_C Unsigned8 UART_PeekByte(Unsigned8 PortNum);
EXTERN_C void UART_PutByte(Unsigned8 PortNum, Unsigned8 Byte);
EXTERN_C void UART_GetBuffer(Unsigned8 PortNum, Unsigned8 *Buffer, Unsigned16 Size);
EXTERN_C Bool UART_PutBuffer(Unsigned8 PortNum, Unsigned8 *Buffer, Unsigned16 Size);
EXTERN_C void UART_PutBufferNoTransmit(Unsigned8 PortNum, Unsigned8 *Buffer, Unsigned16 Size);
EXTERN_C UARTError UART_GetError(Unsigned8 PortNum);
EXTERN_C void UART_ClearError(Unsigned8 PortNum);
EXTERN_C void UART_PurgePort(Unsigned8 PortNum);
EXTERN_C void UART_PurgePortWithInterruptsEnabled(Unsigned8 PortNum);
EXTERN_C void UART_ConfigPort(Unsigned8 PortNum, Unsigned32 BaudRate,
                     Unsigned8 Parity, Unsigned8 DataBits,
                     Unsigned8 StopBits, Bool DisableRxDuringTx);
EXTERN_C void UART_ContinuePort(Unsigned8 PortNum, Unsigned32 BaudRate,
                     Unsigned8 Parity, Unsigned8 DataBits,
                     Unsigned8 StopBits, Bool DisableRxDuringTx);
EXTERN_C Unsigned16 UART_GetCount(Unsigned8 PortNum);
EXTERN_C void _UART_TxIntHandler(Unsigned8 PortNum);
EXTERN_C void _UART_RxIntHandler(Unsigned8 PortNum);
EXTERN_C void _UART_TimedOut(Unsigned8 PortNum);
EXTERN_C void UART_SetPortTimer(Unsigned8 PortNum, Unsigned8 TimerNum, Signed32 mSecs);
EXTERN_C void UART_StartPortTimer(Unsigned8 PortNum, Unsigned8 TimerNum);
EXTERN_C void UART_StopPortTimer(Unsigned8 PortNum, Unsigned8 TimerNum);
EXTERN_C void UART_UpdatePortTimers(void);
EXTERN_C void UART_SetTxBufferEmptyInterrupt(Unsigned8 PortNum);
EXTERN_C void UART_SetTxCompleteInterrupt(Unsigned8 PortNum);
EXTERN_C void UART_ClearStatistics(Unsigned8 PortNum);
//EXTERN_C void UART_SetICTO(Unsigned8 PortNum, Unsigned16 ticks);
EXTERN_C void UART_SwitchActiveTimerForPSEM(Unsigned8 PortNum, Unsigned8 from, Unsigned8 to, Signed32 timeout);
EXTERN_C void UART_WaitForTimer(Unsigned8 PortNum, Unsigned8 TimerNum, Signed32 ticks);
EXTERN_C void UART_fixPortRxErrors(Unsigned16 PortNum);
EXTERN_C void UART_setHWHandshake(Unsigned8 PortNum, Bool wantOn, Unsigned8 onPercent, Unsigned8 offPercent);
EXTERN_C Bool UART_IsTxBufferEmpty(Unsigned8 PortNum);
EXTERN_C void UART_SetDisableRxDuringTx(Unsigned8 PortNum, Bool b);
EXTERN_C void UART_Tick(void);
EXTERN_C void UART_TurnAroundDelay(Unsigned8 PortNum);
EXTERN_C void UART_SetDisableTransmitInIsrAfterTx(Unsigned8 PortNum, Bool b);
EXTERN_C void UART_ServiceUnusedPort(Unsigned8 PortNum);

EXTERN_C void UART_StopPortTimer(Unsigned8 Port, Unsigned8 timerId);

EXTERN_C Bool UART_PortTimerHasTimedout(Unsigned8 Port, Unsigned8 timerId);


EXTERN_C void UART_ClearError(Unsigned8 Port);


EXTERN_C Bool UART_PutBuffer(Unsigned8 Port, Unsigned8* buffer, Unsigned16 length);

EXTERN_C Unsigned8 UART_GetError(Unsigned8 Port);

EXTERN_C void UART_RefreshPortTimer(Unsigned8 Port, Unsigned8 timerId, Unsigned32 timeout);

EXTERN_C void UART_ClearPortTimers(Unsigned8 Port);

EXTERN_C void UART_SetICTO(Unsigned8 port, long timeout);

EXTERN_C void UART_FlushRX(Unsigned8 Port);

EXTERN_C Bool UART_IsByteAvailable(Unsigned8 Port);

EXTERN_C unsigned long UART_NumberOfBytesAvailable(Unsigned8 Port);

EXTERN_C void UART_SetOpticalProbe(int which);

EXTERN_C Boolean UART_PeekByte(Unsigned8 port, Unsigned8* pByte);
EXTERN_C Boolean UART_PeekByteAtIndex(Unsigned8 port, Unsigned16 index, Unsigned8* pByte);
EXTERN_C Bool UART_IsBreakActive(Unsigned8 Port);


//*************************************
//** Data definitions
//*************************************
//*************************************
//** Data declarations
//*************************************

#endif  // included