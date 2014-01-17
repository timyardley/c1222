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

//#include "scb.hpp"

#include <winsock2.h>  // maybe not a good idea
#include "c1222.h"
#include "c1222environment.h"

#include "uart.h"

#include "network.hpp"

s2NetworkServer myNetwork;

extern s2NetworkServer* g_network = &myNetwork;

Boolean g_inhibit_log = FALSE;

//extern QtalkInfo g_params;

s2NetworkError::NetworkError g_last_network_error = s2NetworkError::OK;

Bool g_uart_timedout = FALSE;
Unsigned16 g_uart_icto = 1000;

//Unsigned8 g_portNumber = 3;
Bool g_isPortOpen = FALSE;
Bool g_want_oneByteReads = FALSE;

Bool g_485_2wire = FALSE;

Unsigned8 g_uart_buffer[5000];


extern "C"
void UART_SetOpticalProbe(int which)
{
    g_network->SetOpticalProbe(which);    
}


extern "C"
Boolean UART_PeekByteAtIndex(Unsigned8 port, Unsigned16 index, Unsigned8* pByte)
{
    (void)port;
    
    return (Boolean)g_network->PeekByteAtIndex(index, pByte);
}

extern "C"
Boolean UART_PeekByte(Unsigned8 port, Unsigned8* pByte)
{
    (void)port;
    return (Boolean)g_network->PeekByte(pByte);
}


extern "C"
unsigned long UART_NumberOfBytesAvailable(Unsigned8 Port)
{
    (void)Port;
    return (unsigned long)g_network->NumberOfBytesAvailable();
}

extern "C"
Bool UART_IsByteAvailable(Unsigned8 Port)
{
	(void)Port;
    return (Bool)g_network->IsByteAvailable();
}


extern "C"
Bool UART_IsBreakActive(Unsigned8 Port)
{
    (void)Port;
    return (Bool)g_network->IsBreakActive();
}


extern "C" 
void UART_SetICTO(Unsigned8 Port, long icto)
{
	(void)Port;
    g_uart_icto = (Unsigned16)icto;
	g_network->SetInterCharacterTimeout(icto);
    g_uart_timedout = FALSE;
}


extern "C" 
Unsigned8 UART_GetByte(Unsigned8 PortNum)
{
	Unsigned8 c;

    UART_GetBuffer(PortNum, &c, 1);

    return c;
}


extern "C" 
void UART_FlushRX(Unsigned8 Port)
{
	(void)Port;
	g_network->RxFlush();
}



extern "C" 
void UART_PurgePort(Unsigned8 Port)
{
	(void)Port;
	g_network->RxFlush();
    g_network->FlushTransmitBuffers();
    g_uart_timedout = FALSE;
}

extern "C" 
void UART_PurgePortWithInterruptsEnabled(Unsigned8 Port)
{
	UART_PurgePort(Port);
}


extern "C" 
void UART_ConfigPort(Unsigned8 Port, Unsigned32 BaudRate, Unsigned8 parityOption,
			Unsigned8 dataBitsOption, Unsigned8 stopBitsOption,
            Bool useCts)
{
	(void)Port;
    (void)dataBitsOption;
    (void)stopBitsOption;
    (void)parityOption;
    (void)useCts;
    g_network->ClosePort();
    g_uart_timedout = FALSE;
    g_last_network_error = g_network->OpenPort(Port, BaudRate, 0, 0);
    g_network->SetInterCharacterTimeout(g_uart_icto);

    g_isPortOpen = g_last_network_error == s2NetworkError::OK;

}


extern "C" 
void UART_ContinuePort( Unsigned8 Port, Unsigned32 BaudRate, Unsigned8 parityOption,
			Unsigned8 dataBitsOption, Unsigned8 stopBitsOption,
            Bool useCts)
{
	(void)Port;
    (void)dataBitsOption;
    (void)stopBitsOption;
    (void)parityOption;
    (void)useCts;
    (void)BaudRate;
    //g_network->ClosePort();
    g_uart_timedout = FALSE;
    g_last_network_error =  s2NetworkError::OK;
         //g_network->OpenPort(g_params.portNumber, BaudRate, 0, 0);
    g_network->SetInterCharacterTimeout(g_uart_icto);
}



extern "C" 
void UART_StopPortTimer(Unsigned8 Port, Unsigned8 timerId)
{
	(void)Port;
    (void)timerId;
    g_uart_timedout = FALSE;
}

extern "C" 
Bool UART_PortTimerHasTimedout(Unsigned8 Port, Unsigned8 timerId)
{
	(void)Port;
    (void)timerId;
	return (Bool)g_uart_timedout;
}


extern "C" 
void UART_ClearError(Unsigned8 Port)
{
	(void)Port;
}


extern "C" 
Bool UART_PutBuffer(Unsigned8 Port, Unsigned8* buffer, Unsigned16 length)
{
	(void)Port;

	g_last_network_error = g_network->SendBuffer( buffer, length );

    if ( g_last_network_error != s2NetworkError::OK )
    {
    	return FALSE;
    }

    if ( g_485_2wire && !g_uart_timedout )
    {
        UART_GetBuffer(Port, g_uart_buffer, length);
        if ( memcmp(buffer, g_uart_buffer, length ) != 0 )
        {
            return FALSE;
        }
    }

    return TRUE;
}

extern "C" 
Bool UART_IsTxBufferEmpty(Unsigned8 port)
{
    (void)port;
    return TRUE;
}

extern "C" 
void UART_PutByte(Unsigned8 Port, Unsigned8 c)
{
  UART_PutBuffer(Port, &c, 1);
}


extern "C" 
Unsigned8 UART_GetError(Unsigned8 Port)
{
	(void)Port;
    return 0;
}

extern "C" 
void UART_RefreshPortTimer(Unsigned8 Port, Unsigned8 timerId, Unsigned32 timeout)
{
	(void)Port;
    (void)timerId;
    (void)timeout;
    g_uart_timedout = FALSE;
}


extern void logTextAndTime(const char* text);
extern void logit(const char* text, const unsigned char* buffer, unsigned long length);



extern "C" 
void UART_GetBuffer(Unsigned8 Port, Unsigned8* buffer, Unsigned16 length)
{
	WORD endIndex;
    Unsigned8* bufferIn = buffer;

    (void)Port;

    g_network->SetInterCharacterTimeout(g_uart_icto);

	//g_last_network_error = g_network->ReceiveBuffer( buffer, -2, length,
    //                                          &endIndex );

    g_uart_timedout = FALSE;

    while(length)
    {
        Unsigned16 readLength = length;

        if ( g_want_oneByteReads )
            readLength = 1;

        g_last_network_error = g_network->ReceiveBufferPartialOK(buffer, g_uart_icto, readLength, &endIndex);

        if ( g_network->TimedOut() )
        {
            if ( bufferIn != buffer )
                logit("  Rx", bufferIn, buffer-bufferIn );

            char message[60];
            sprintf(message,"Timed out - icto=%d", g_uart_icto);

            logTextAndTime(message);

            g_uart_timedout = TRUE;
            return;
        }
        else
        {
            if ( endIndex <= length )
            {
                length -= endIndex;
                buffer += endIndex;
            }
            else
                length = 0;
        }
    }

    logit("  Rx", bufferIn, buffer-bufferIn );
}

extern "C" 
void UART_ClearPortTimers(Unsigned8 Port)
{
	(void)Port;
    g_uart_timedout = FALSE;
}


