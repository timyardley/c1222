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

#ifndef C1222PL_H
#define C1222PL_H

#include "c1222environment.h"
#include "C1222.h"
#include "c1222event.h"

typedef struct C1222PLCountersTag
{
	Unsigned32 numberOfBytesReceived;
	Unsigned32 numberOfBytesSent;
	
	Unsigned16 isByteAvailableCheckCount;

} C1222PLCounters;


typedef struct C1222PLStatusTag
{
	Unsigned8 portID;
	Unsigned16 ictoTicks;
	Unsigned32 baudRate;
	Boolean    isHalfDuplex;
	
} C1222PLStatus;



typedef struct C1222PLTag
{
    C1222PLStatus status;
    
	FILE* stream;
	
	Unsigned8 link_direction;
	Boolean logWanted;
	
	void* pDataLink;
	
	C1222PLCounters counters;
	
	Boolean powerUpInProgress;
    
} C1222PL;


Unsigned8 C1222PL_GetByte(C1222PL* p);
Unsigned8 C1222PL_GetByteNoLog(C1222PL* p);
Unsigned16 C1222PL_GetBuffer(C1222PL* p, Unsigned8* buffer, Unsigned16 lengthWanted);
Unsigned16 C1222PL_GetBufferNoLog(C1222PL* p, Unsigned8* buffer, Unsigned16 lengthWanted);
void C1222PL_PutBuffer(C1222PL* p, Unsigned8* buffer, Unsigned16 length);
void C1222PL_PutByte_Without_Flush(C1222PL* p, Unsigned8 c);
void C1222PL_SetLinks(C1222PL* p, void* pDataLink);
void C1222PL_Init(C1222PL* p, Unsigned8 portID);
void C1222PL_SetICTO(C1222PL* p, Unsigned16 ictoInMs);
Boolean C1222PL_IsByteAvailable(C1222PL* p);
void C1222PL_RestartICTO(C1222PL* p);
void C1222PL_ConfigurePort(C1222PL* p, Unsigned32 baud); // rest of physical config is fixed
Boolean C1222PL_DidLastActionTimeout(C1222PL* p);
void C1222PL_StartMessage(C1222PL* p);
void C1222PL_FinishMessage(C1222PL* p);
void C1222PL_NotePowerUp(C1222PL* p, Unsigned32 outageDurationInSeconds);

void C1222PL_IncrementStatistic(C1222PL* p, Unsigned16 id);
void C1222PL_AddToStatistic(C1222PL* p, Unsigned16 id, Unsigned32 count);

Boolean C1222PL_IsInterfaceAvailable(C1222PL* p);

void C1222PL_LogEvent(C1222PL* p, C1222EventId event);
void C1222PL_LogEventAndBytes(C1222PL* p, C1222EventId event, Unsigned8 n, Unsigned8* bytes);
void C1222PL_LogHex(C1222PL* p, unsigned char* buffer, unsigned short length);
void C1222PL_LogHexReceive(C1222PL* p, unsigned char* buffer, unsigned short length);
void C1222PL_LogTime(C1222PL* p);
void C1222PL_WaitMs(Unsigned32 ms);

Boolean C1222PL_HandleNonC1222Packet(C1222PL* p, Unsigned8 StartChar);
Boolean C1222PL_Are8BytesAvailable(C1222PL* p);
Unsigned8 C1222PL_PeekByte(C1222PL* p);
Unsigned16 C1222PL_NumberOfBytesAvailable(C1222PL* p);
Unsigned8 C1222PL_PeekByteAtIndex(C1222PL* p, Unsigned16 index);

void C1222PL_ReInitAfterRFLANFwDownload(C1222PL* p);

void C1222PL_PowerUpInProgress(C1222PL* p);

void C1222PL_SetHalfDuplex(C1222PL* p, Boolean b);


#endif
