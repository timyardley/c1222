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


#ifndef C1222AL_RECEIVEHISTORY
#define C1222AL_RECEIVEHISTORY

#include "c1222environment.h"
#include "c1222aptitle.h"
#include "c1222al.h"
#include "acsemsg.h"

#define C1222_RECEIVE_HISTORY_LENGTH    5

typedef struct C1222ReceiveHistoryElementTag
{
    Unsigned8 callingApTitle[C1222_APTITLE_LENGTH];
    Unsigned16 sequence;
    Unsigned32 receptionTime;
} C1222ReceiveHistoryElement;

typedef struct C1222ReceiveHistoryTag
{
    C1222AL* pApplication;
    Unsigned8 addIndex;
    
    C1222ReceiveHistoryElement history[C1222_RECEIVE_HISTORY_LENGTH];
} C1222ReceiveHistory;

void C1222ALReceiveHistory_Init(C1222ReceiveHistory* p, C1222AL* pAL);
Boolean C1222ALReceiveHistory_IsMessageAlreadyReceived(C1222ReceiveHistory* p, ACSE_Message* pMsg);


#endif
