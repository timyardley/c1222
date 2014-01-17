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
// 
// this is a pretend physical layer to allow testing with both stacks in the same pc app


#include "c1222environment.h"
#include "c1222pl.h"
#include "c1222misc.h"
#include "c1222dl.h"
#include "c1222statistic.h"

#include "windows.h"


extern void DoSleep(Unsigned32 delay);


//typedef struct C1222PLTag
//{
//	Unsigned8 portID;
//	FILE* stream;
//	Unsigned16 ictoTicks;
//	Unsigned32 baudRate;
//    
//} C1222PL;

// will use port id to indicate which link to use 0 -> device to cm, 1 -> cm to device

#define LINK_BUFFER_LENGTH   500


typedef struct TestLinkTag
{
    Unsigned8 buffer[LINK_BUFFER_LENGTH];
    Unsigned16 putIndex;
    Unsigned16 getIndex;
    Unsigned16 count;
    Boolean timedOut;
    Unsigned8 which;
    Boolean inMessage;
    Boolean messageReady;
    Unsigned8 direction;
    Boolean logDisabled;
} TestLink;

TestLink testLinks[5];

void TestLink_Init(TestLink* p, Unsigned8 which)
{
    memset(p, 0, sizeof(TestLink));
    p->which = which;
}

int stackLevel = 0;


void LetOtherStackRun(TestLink* p)
{   
    Unsigned32 start = C1222Misc_GetFreeRunningTimeInMS();

    DoSleep(2);
    
    while ( p->count == 0 || !p->messageReady )
    {
        if ( (C1222Misc_GetFreeRunningTimeInMS()-start) > 500 )
            break;
            
        DoSleep(2);
    }
}




Unsigned8 TestLink_PeekByte(TestLink* p)
{
    Unsigned8 c;
        
    if ( p->count > 0 )
    {
        c = p->buffer[p->getIndex];
                
        return c;
    }

    return 0xFF;
}

Unsigned8 TestLink_PeekByteAtIndex(TestLink* p, Unsigned16 index)
{
    Unsigned8 c;
        
    if ( p->count > index )
    {
        c = p->buffer[(p->getIndex + index) % LINK_BUFFER_LENGTH];
                
        return c;
    }

    return 0xFF;
}


Unsigned16 TestLink_NumberOfBytesAvailable(TestLink* p)
{
    return p->count;
}


Unsigned8 TestLink_GetByte(TestLink* p)
{
    Unsigned8 c;
    
    if ( p->count == 0 || !p->messageReady )
        LetOtherStackRun(p);
    
    if ( p->count > 0 )
    {
        p->count--;
        c = p->buffer[p->getIndex];
        
        p->getIndex++;
        p->getIndex %= (Unsigned16)LINK_BUFFER_LENGTH;

        p->timedOut = FALSE;
        
        return c;
    }

    p->timedOut = TRUE;
    
    return 0xFF;
}


Boolean TestLink_IsByteAvailable(TestLink* p)
{    
    DoSleep(0);  // this may not be a good idea
    
    if ( !p->messageReady )
        return FALSE;
            
    return (Boolean)(p->count > 0);
}


Boolean TestLink_Are8BytesAvailable(TestLink* p)
{    
    DoSleep(0);  // this may not be a good idea
    
    if ( !p->messageReady )
        return FALSE;
            
    return (Boolean)(p->count >= 8);
}



void TestLink_PutByte(TestLink* p, Unsigned8 c)
{    
    if ( p->count < LINK_BUFFER_LENGTH )
    {
        p->buffer[p->putIndex] = c;
        p->count++;
        p->putIndex++;
        p->putIndex %= (Unsigned16)LINK_BUFFER_LENGTH;
    }
}




void C1222PL_SetHalfDuplex(C1222PL* p, Boolean b)
{
    (void)p;
    (void)b;
}


Unsigned8 C1222PL_GetByte(C1222PL* p)
{
    Unsigned8 c;
    
    TestLink* pLink = &testLinks[p->status.portID];
    
    C1222PL_RestartICTO(p);
     
    c = TestLink_GetByte(pLink);
    
    if ( !pLink->timedOut )
    {
        p->counters.numberOfBytesReceived++;
        C1222PL_IncrementStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_OCTETS_RECEIVED);

        if ( !pLink->logDisabled )
        {
        	if ( pLink->direction != 1 )
        	{
            	//C1222PL_LogTime(p);
            	pLink->direction = 1;
        	}

        	C1222PL_LogHex(p, &c, 1);
        }
    }


    return c;
}



Unsigned8 C1222PL_GetByteNoLog(C1222PL* p)
{
    Unsigned8 c;
    
    TestLink* pLink = &testLinks[p->status.portID];
    
    C1222PL_RestartICTO(p);
     
    c = TestLink_GetByte(pLink);
    
    if ( !pLink->timedOut )
    {
        p->counters.numberOfBytesReceived++;
        C1222PL_IncrementStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_OCTETS_RECEIVED);
    }


    return c;
}


Unsigned16 C1222PL_GetBuffer(C1222PL* p, Unsigned8* buffer, Unsigned16 lengthWanted)
{
    Unsigned16 count = 0;
    TestLink* pLink = &testLinks[p->status.portID];
    Unsigned8* origBuffer = buffer;

    pLink->logDisabled = TRUE;

    while ( lengthWanted > 0 )
    {
        *buffer++ = C1222PL_GetByte(p);

        if ( C1222PL_DidLastActionTimeout(p) )
            break;

    	lengthWanted--;
        count++;
    }

    pLink->logDisabled = FALSE;

   	if ( pLink->direction != 1 )
   	{
       	//C1222PL_LogTime(p);
       	pLink->direction = 1;
   	}

   	C1222PL_LogHex(p, origBuffer, count);

    C1222PL_AddToStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_OCTETS_RECEIVED, count);
    
    return count;
}



Unsigned16 C1222PL_GetBufferNoLog(C1222PL* p, Unsigned8* buffer, Unsigned16 lengthWanted)
{
    Unsigned16 count = 0;
    //TestLink* pLink = &testLinks[p->status.portID];
    //Unsigned8* origBuffer = buffer;


    while ( lengthWanted > 0 )
    {
        *buffer++ = C1222PL_GetByteNoLog(p);

        if ( C1222PL_DidLastActionTimeout(p) )
            break;

    	lengthWanted--;
        count++;
    }



    C1222PL_AddToStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_OCTETS_RECEIVED, count);
    
    return count;
}





Boolean C1222PL_DidLastActionTimeout(C1222PL* p)
{
	TestLink* pLink = &testLinks[p->status.portID];
    
    return pLink->timedOut;
}

TestLink* GetPutLink(C1222PL* p)
{
    switch(p->status.portID)
    {
    case 1:
        return &testLinks[2];
    case 2:
        return &testLinks[1];
    case 3:
        return &testLinks[4];
    case 4:
        return &testLinks[3];
    }

    return 0;
}




void WaitMs(Unsigned32 ms)
{
    Unsigned32 start = C1222Misc_GetFreeRunningTimeInMS();
    
    while( (Unsigned32)(C1222Misc_GetFreeRunningTimeInMS()-start) < ms )
        DoSleep(2);
}


void C1222PL_WaitMs(Unsigned32 ms)
{
    WaitMs(ms);
}


void C1222PL_StartMessage(C1222PL* p)
{
    TestLink* pLink;

    pLink = GetPutLink(p);

    pLink->inMessage = TRUE;
    pLink->messageReady = FALSE;

    testLinks[p->status.portID].direction = 2;
    
    //C1222PL_LogTime(p);
}


void C1222PL_FinishMessage(C1222PL* p)
{
    TestLink* pLink = GetPutLink(p);
            
    pLink->inMessage = FALSE;
    pLink->messageReady = TRUE;    
}


void C1222PL_PutBuffer(C1222PL* p, Unsigned8* buffer, Unsigned16 length)
{
    
    TestLink* pLink = GetPutLink(p);
    Unsigned16 ii;
        
    
    for ( ii=0; ii<length; ii++ )
    {
        TestLink_PutByte(pLink, buffer[ii]);
        p->counters.numberOfBytesSent++;
    }
    
    C1222PL_AddToStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_OCTETS_SENT, length);

    
    C1222PL_LogHex(p, buffer, length);
    
}



void C1222PL_PutByte_Without_Flush(C1222PL* p, Unsigned8 c)
{
    
    TestLink* pLink = GetPutLink(p);
        
    TestLink_PutByte(pLink, c);
    p->counters.numberOfBytesSent++;
    
    C1222PL_AddToStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_OCTETS_SENT, 1);

    
    C1222PL_LogHex(p, &c, 1);    
}




void C1222PL_LogHex(C1222PL* p, unsigned char* buffer, unsigned short length)
{
    C1222EventId event;
    unsigned short chunkLength;
    TestLink* pLink = &testLinks[p->status.portID];
    
    if ( pLink->direction == 1 )
        event = C1222EVENT_PL_PORT1_RX + p->status.portID - 1;
    else if ( pLink->direction == 2 )
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
    C1222EventId event;
    unsigned short chunkLength;
    //TestLink* pLink = &testLinks[p->status.portID];
    
    event = C1222EVENT_PL_PORT1_RX + p->status.portID - 1;
    
    while( length > 0 )
    {
        chunkLength = (unsigned short)((length > 16) ? 16 : length);
        
        C1222PL_LogEventAndBytes(p, event, chunkLength, buffer);
        
        buffer += chunkLength;
        length -= chunkLength;
    }
}



void C1222PL_LogTime(C1222PL* p)
{
  unsigned char mybuffer[5];
  SYSTEMTIME systemTime;

  GetLocalTime( &systemTime );

  mybuffer[0] = systemTime.wHour;
  mybuffer[1] = systemTime.wMinute;
  mybuffer[2] = systemTime.wSecond;
  mybuffer[3] = (unsigned char)(systemTime.wMilliseconds & 0xFF);
  mybuffer[4] = (unsigned char)(systemTime.wMilliseconds >> 8);
  
  C1222PL_LogEventAndBytes(p, C1222EVENT_SYSTEMTIME, 5, mybuffer);
}




void C1222PL_LogEvent(C1222PL* p,C1222EventId event)
{
    C1222DL* pDL = (C1222DL*)(p->pDataLink);
    
    C1222DL_LogEvent(pDL, event);
}




void C1222PL_LogEventAndBytes(C1222PL* p,C1222EventId event, Unsigned8 n, Unsigned8* bytes)
{
    C1222DL* pDL = (C1222DL*)(p->pDataLink);
    
    C1222DL_LogEventAndBytes(pDL, event, n, bytes);
}




void C1222PL_SetLinks(C1222PL* p, void* pDatalink)
{
    p->pDataLink = pDatalink;
}



Boolean C1222PL_IsInterfaceAvailable(C1222PL* p)
{
    (void)p;
    return TRUE;  // this testlink is always available!
}



void C1222PL_ReInitAfterRFLANFwDownload(C1222PL* p)
{
    // not used in test link
    (void)p;
}



void C1222PL_Init(C1222PL* p, Unsigned8 portID)
{
	TestLink* pLink;
	
	memset(p, 0, sizeof(C1222PL));

    p->status.portID = portID;
    p->logWanted = FALSE;
    p->link_direction = 0;
    
    p->counters.isByteAvailableCheckCount = 0;
    
    p->counters.numberOfBytesReceived = 0;
    p->counters.numberOfBytesSent = 0;
    p->status.ictoTicks = 50;
    
    p->powerUpInProgress = FALSE;
    

    pLink = &testLinks[p->status.portID];
    
    C1222PL_ConfigurePort(p, 9600); // set to 9600 baud
    
    TestLink_Init(pLink, portID);
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
    
    // how does this happen on a test link?
}



void C1222PL_ConfigurePort(C1222PL* p, Unsigned32 baud)
{
    p->status.baudRate = baud;
}


void C1222PL_SetICTO(C1222PL* p, Unsigned16 ictoInMs)
{
    p->status.ictoTicks = (Unsigned16)(ictoInMs/1);
}


void C1222PL_RestartICTO(C1222PL* p)
{
    (void)p;
}



Boolean C1222PL_Are8BytesAvailable(C1222PL* p)
{
    TestLink* pLink = &testLinks[p->status.portID];
    
    p->counters.isByteAvailableCheckCount++;
    
    return TestLink_Are8BytesAvailable(pLink);
}


Unsigned8 C1222PL_PeekByte(C1222PL* p)
{
    TestLink* pLink = &testLinks[p->status.portID];
    
    return TestLink_PeekByte(pLink);    
}

Unsigned16 C1222PL_NumberOfBytesAvailable(C1222PL* p)
{
    TestLink* pLink = &testLinks[p->status.portID];
    
    return TestLink_NumberOfBytesAvailable(pLink);        
}

Unsigned8 C1222PL_PeekByteAtIndex(C1222PL* p, Unsigned16 index)
{
    TestLink* pLink = &testLinks[p->status.portID];
    
    return TestLink_PeekByteAtIndex(pLink, index);        
    
}



Boolean C1222PL_IsByteAvailable(C1222PL* p)
{
    TestLink* pLink = &testLinks[p->status.portID];
    
    p->counters.isByteAvailableCheckCount++;
    
    return TestLink_IsByteAvailable(pLink);
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
