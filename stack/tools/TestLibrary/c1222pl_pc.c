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


// C12.22 physical layer - PC

#include "c1222environment.h"
#include "c1222pl.h"
#include "uart.h"
#include "c1222dl.h"
#include "windows.h"
#include "c1222statistic.h"
#include "c1222misc.h"


static link_direction = 0;


//typedef struct C1222PLTag
//{
//	Unsigned8 portID;
//	FILE* stream;
//	Unsigned16 ictoTicks;
//	Unsigned32 baudRate;
//    
//} C1222PL;

void C1222PL_StartMessage(C1222PL* p)
{
    (void)p;  // TO DO .. FILL UP THIS ROUTINE
    
    link_direction = 2;
    
    //C1222PL_LogTime(p);
}

void C1222PL_FinishMessage(C1222PL* p)
{
    (void)p;  // TO DO .. FILL UP THIS ROUTINE
    
    link_direction = 0;    
}


void WaitMs(Unsigned32 ms)
{
    Unsigned32 start = C1222Misc_GetFreeRunningTimeInMS();
    
    while( (Unsigned32)(C1222Misc_GetFreeRunningTimeInMS()-start) < ms )
    {
        Sleep(2);
    }
}


void C1222PL_WaitMs(Unsigned32 ms)
{
    WaitMs(ms);
}

void C1222PL_SetHalfDuplex(C1222PL* p, Boolean b)
{
    (void)p;
    (void)b;
}


Unsigned8 C1222PL_GetByte(C1222PL* p)
{
    Unsigned8 c;
    
    C1222PL_RestartICTO(p);
     
    c = UART_GetByte(p->status.portID);
    
    if ( !C1222PL_DidLastActionTimeout(p) )
    {
        p->counters.numberOfBytesReceived++;
        C1222PL_IncrementStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_OCTETS_RECEIVED);
        
        if ( link_direction != 1 )
        {
            //C1222PL_LogTime(p);
            link_direction = 1;
        }
        
        C1222PL_LogHex(p, &c, 1);
        
    }

    return c;    
}


Boolean C1222PL_Are8BytesAvailable(C1222PL* p);
Unsigned8 C1222PL_PeekByte(C1222PL* p);




Unsigned8 C1222PL_GetByteNoLog(C1222PL* p)
{
    Unsigned8 c;
    
    C1222PL_RestartICTO(p);
     
    c = UART_GetByte(p->status.portID);
    
    if ( !C1222PL_DidLastActionTimeout(p) )
    {
        p->counters.numberOfBytesReceived++;
        C1222PL_IncrementStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_OCTETS_RECEIVED);        
    }

    return c;    
}




Unsigned16 C1222PL_GetBuffer(C1222PL* p, Unsigned8* buffer, Unsigned16 lengthWanted)
{
    Unsigned16 count = 0;
    Unsigned16 bytesAvailable;
    
    while ( lengthWanted > 0 )
    {
        bytesAvailable = (Unsigned16)UART_NumberOfBytesAvailable(p->status.portID);
        if ( bytesAvailable > lengthWanted )
            bytesAvailable = lengthWanted;
        
        if ( bytesAvailable > 1 )
        {
            UART_GetBuffer(p->status.portID, buffer, bytesAvailable);
            lengthWanted -= bytesAvailable;
            buffer += bytesAvailable;
            
            if ( link_direction != 1 )
            {
                //C1222PL_LogTime(p);
                link_direction = 1;
            }
        
            C1222PL_LogHex(p, buffer, bytesAvailable);
            
            C1222PL_AddToStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_OCTETS_RECEIVED, bytesAvailable);
        }
        else
            *buffer++ = C1222PL_GetByte(p);
        
        if ( C1222PL_DidLastActionTimeout(p) )
            break;
            
        if ( bytesAvailable > 1 )
            count += bytesAvailable;
        else
            count++;
    }
    
    return count;
}




Unsigned16 C1222PL_GetBufferNoLog(C1222PL* p, Unsigned8* buffer, Unsigned16 lengthWanted)
{
    Unsigned16 count = 0;
    Unsigned16 bytesAvailable;
    
    while ( lengthWanted > 0 )
    {
        bytesAvailable = (Unsigned16)UART_NumberOfBytesAvailable(p->status.portID);
        if ( bytesAvailable > lengthWanted )
            bytesAvailable = lengthWanted;
        
        if ( bytesAvailable > 1 )
        {
            UART_GetBuffer(p->status.portID, buffer, bytesAvailable);
            lengthWanted -= bytesAvailable;
            buffer += bytesAvailable;
                        
            C1222PL_AddToStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_OCTETS_RECEIVED, bytesAvailable);
        }
        else
            *buffer++ = C1222PL_GetByteNoLog(p);
        
        if ( C1222PL_DidLastActionTimeout(p) )
            break;
            
        if ( bytesAvailable > 1 )
            count += bytesAvailable;
        else
            count++;
    }
    
    return count;
}






Boolean C1222PL_DidLastActionTimeout(C1222PL* p)
{   
    return UART_PortTimerHasTimedout(p->status.portID,0);
}


void C1222PL_PutBuffer(C1222PL* p, Unsigned8* buffer, Unsigned16 length)
{
    if ( UART_PutBuffer(p->status.portID, buffer, length) )
    {
        C1222PL_AddToStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_OCTETS_SENT, length);

    
        C1222PL_LogHex(p, buffer, length);        
        
        return;
    }
        
    // need to log this error!
    // TODO
}



void C1222PL_PutByte_Without_Flush(C1222PL* p, Unsigned8 c)
{    
    if ( UART_PutBuffer(p->status.portID, &c, 1) )
    {
        C1222PL_AddToStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_OCTETS_SENT, 1);

    
        C1222PL_LogHex(p, &c, 1);        
        
        return;
    }
        
    // need to log this error!
    // TODO
}




void C1222PL_LogEvent(C1222PL* p, C1222EventId event)
{
    C1222DL* pDL = (C1222DL*)(p->pDataLink);
    
    if ( p->logWanted )
        C1222DL_LogEvent(pDL, event);
}



void C1222PL_LogEventAndBytes(C1222PL* p, C1222EventId event, Unsigned8 n, Unsigned8* bytes)
{
    C1222DL* pDL = (C1222DL*)(p->pDataLink);
    
    if ( p->logWanted )
        C1222DL_LogEventAndBytes(pDL, event, n, bytes);
}



void C1222PL_LogHex(C1222PL* p, unsigned char* buffer, unsigned short length)
{    
    C1222EventId event;
    unsigned short chunkLength;
    
    if ( !p->logWanted )
        return;
    
    if ( link_direction == 1 )
        event = C1222EVENT_PL_PORT1_RX + p->status.portID - 1;
    else if ( link_direction == 2 )
        event = C1222EVENT_PL_PORT1_TX + p->status.portID - 1;
    
    while( length > 0 )
    {
        chunkLength = (unsigned short)((length > 16) ? 16 : length);
        
        C1222PL_LogEventAndBytes(p, event, chunkLength, buffer);
        
        buffer += chunkLength;
        length -= chunkLength;
    }
}


void C1222PL_LogHexReceive(C1222PL* p, unsigned char* buffer, unsigned short length)
{
    link_direction = 1;
    C1222PL_LogHex(p, buffer, length);
}




void C1222PL_LogTime(C1222PL* p)
{
  unsigned char mybuffer[5];
  SYSTEMTIME systemTime;
  
  if ( !p->logWanted )
    return;

  GetLocalTime( &systemTime );

  mybuffer[0] = systemTime.wHour;
  mybuffer[1] = systemTime.wMinute;
  mybuffer[2] = systemTime.wSecond;
  mybuffer[3] = (unsigned char)(systemTime.wMilliseconds & 0xFF);
  mybuffer[4] = (unsigned char)(systemTime.wMilliseconds >> 8);
  
  C1222PL_LogEventAndBytes(p, C1222EVENT_SYSTEMTIME, 5, mybuffer);
}





void C1222PL_SetLinks(C1222PL* p, void* pDatalink)
{
    p->pDataLink = pDatalink;
}



Boolean C1222PL_IsInterfaceAvailable(C1222PL* p)
{
    // how to know if anyone is there - check for a break?
    
    // should ask the uart which should check with the windows port
    // assume available for now

    (void)p;
    
    return TRUE;
}



void C1222PL_ReInitAfterRFLANFwDownload(C1222PL* p)
{
    // not used in pc
    (void)p;
}



void C1222PL_Init(C1222PL* p, Unsigned8 portID)
{
    p->status.portID = portID;
    
    p->counters.isByteAvailableCheckCount = 0;
    
    p->counters.numberOfBytesReceived = 0;
    p->counters.numberOfBytesSent = 0;
    p->status.ictoTicks = 50;
    
    
    p->link_direction = 0;
    p->logWanted = FALSE;
    p->powerUpInProgress = FALSE;
    
    C1222PL_ConfigurePort(p, 9600); // set to 9600 baud
}


void C1222PL_PowerUpInProgress(C1222PL* p)
{
    p->powerUpInProgress = TRUE;
}



void C1222PL_NotePowerUp(C1222PL* p, Unsigned32 outageDurationInSeconds)
{
    (void)p;
    (void)outageDurationInSeconds;
    
    p->powerUpInProgress = FALSE;
    
    // shouldn't happen on a pc
}



void C1222PL_ConfigurePort(C1222PL* p, Unsigned32 baud)
{
    p->status.baudRate = baud;
    
    UART_ConfigPort(p->status.portID, baud, 0, 8, 1, FALSE);
}


void C1222PL_SetICTO(C1222PL* p, Unsigned16 ictoInMs)
{
    p->status.ictoTicks = (Unsigned16)(ictoInMs);
}


void C1222PL_RestartICTO(C1222PL* p)
{
    UART_StopPortTimer(p->status.portID, 0);
    UART_SetICTO(p->status.portID, p->status.ictoTicks);
}




Unsigned8 C1222PL_PeekByte(C1222PL* p)
{
    Unsigned8 byte;
    
    if ( UART_PeekByte(p->status.portID, &byte) )
        return byte;  
        
    return 0xFF; 
}


Unsigned8 C1222PL_PeekByteAtIndex(C1222PL* p, Unsigned16 index)
{
    Unsigned8 byte;
    
    if ( UART_PeekByteAtIndex(p->status.portID, index, &byte) )
        return byte;  
        
    return 0xFF; 
}


Unsigned16 C1222PL_NumberOfBytesAvailable(C1222PL* p)
{
    p->counters.isByteAvailableCheckCount++;
    
    return (Unsigned16)UART_NumberOfBytesAvailable(p->status.portID); 
}



Boolean C1222PL_Are8BytesAvailable(C1222PL* p)
{
    p->counters.isByteAvailableCheckCount++;
    
    return (Boolean)(UART_NumberOfBytesAvailable(p->status.portID) >= 8);
}


Boolean C1222PL_IsByteAvailable(C1222PL* p)
{
    p->counters.isByteAvailableCheckCount++;
    
    return UART_IsByteAvailable(p->status.portID);
}



void C1222PL_IncrementStatistic(C1222PL* p, Unsigned16 id)
{
    C1222DL* pDatalink = (C1222DL*)(p->pDataLink);
    
    C1222DL_IncrementStatistic(pDatalink, id);
}

void C1222PL_AddToStatistic(C1222PL* p, Unsigned16 id, Unsigned32 count)
{
    C1222DL* pDatalink = (C1222DL*)(p->pDataLink);
    
    C1222DL_AddToStatistic(pDatalink, id, count);
}





Boolean C1222PL_HandleNonC1222Packet(C1222PL* p, Unsigned8 StartChar)
{
    (void)p;
    (void)StartChar;
    
    return FALSE;  // this is only implemented in the comm module
}

