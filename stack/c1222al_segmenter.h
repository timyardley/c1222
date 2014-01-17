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


#ifndef C1222ALSEGMENTER_H
#define C1222ALSEGMENTER_H

#include "c1222.h"
#include "c1222environment.h"
#include "acsemsg.h"


#define MAX_C1222_DISJOINT_SEGMENTS   10

typedef struct C1222SegmentDefinitionTag
{
    Unsigned16 offset;
    Unsigned16 nextOffset;
} C1222SegmentDefinition;

typedef struct C1222ALSegmenterTag
{
    Unsigned8 state;
    
    // offset 1:
    ACSE_Message workingMessage;
    
    // offset 7:
    Unsigned16 fragmentSize;
    
    // offset 9:
    Unsigned8 workingMessageBuffer[MAX_ACSE_MESSAGE_LENGTH];  // size 643
    Unsigned8 workingMessageBufferCanary[10];
    Unsigned16 workingMessageLength;
    
    // offset 664:
    Unsigned16 numberOfSegments;
    Unsigned16 segmentPayloadLength;
    
    // offset 668:
    C1222SegmentDefinition disjointSegments[MAX_C1222_DISJOINT_SEGMENTS]; // size 40
    Unsigned8 disjointSegmentCount;
    
    // offset 709:
    Unsigned16 workingSequence;
    Unsigned8  callingApTitleBuffer[C1222_APTITLE_LENGTH]; // size 20
    // offset 731
    C1222ApTitle callingApTitle;
    
    // offset 735
    Unsigned32 rxMessageStartTime;
    
    void* pApplication;
} C1222ALSegmenter;  // size 743

Boolean C1222ALSegmenter_SetUnsegmentedMessage(C1222ALSegmenter* p, ACSE_Message* pMsg);
Boolean C1222ALSegmenter_SetUnsegmentedRxMessage(C1222ALSegmenter* p, ACSE_Message* pMsg);

Boolean C1222ALSegmenter_SetShortMessage(C1222ALSegmenter* p, ACSE_Message* pMsg);


void C1222ALSegmenter_SetFragmentSize(C1222ALSegmenter* p, Unsigned16 bytes);

void C1222ALSegmenter_SetLinks(C1222ALSegmenter* p, void* pApplication);
Unsigned16 C1222ALSegmenter_GetNumberOfSegments(C1222ALSegmenter* p);
Boolean C1222ALSegmenter_GetSegment(C1222ALSegmenter* p, Unsigned16 segIndex, Unsigned8* buffer, Unsigned16 maxLength, Unsigned16* pLength);
void C1222ALSegmenter_Clear(C1222ALSegmenter* p);
Boolean C1222ALSegmenter_SetSegment(C1222ALSegmenter* p, ACSE_Message* pMsg);
Boolean C1222ALSegmenter_IsSegmentAlreadyReceived(C1222ALSegmenter* p, ACSE_Message* pMsg);
Boolean C1222ALSegmenter_IsMessageComplete(C1222ALSegmenter* p);
ACSE_Message* C1222ALSegmenter_GetUnsegmentedMessage(C1222ALSegmenter* p); // returns null on error
ACSE_Message* C1222ALSegmenter_StartNewUnsegmentedMessage(C1222ALSegmenter* p);

void C1222ALSegmenter_EndNewUnsegmentedMessage(C1222ALSegmenter* p);  // I'm not so sure about this one

void C1222ALSegmenter_Init(C1222ALSegmenter* p);

Boolean C1222ALSegmenter_HandleSegmentTimeout(C1222ALSegmenter* p);
void C1222ALSegmenter_CancelPartialMessage(C1222ALSegmenter* p);
Boolean C1222ALSegmenter_IsBusy(C1222ALSegmenter* p);
ACSE_Message* C1222ALSegmenter_GetWorkingMessage(C1222ALSegmenter* p);


#endif
