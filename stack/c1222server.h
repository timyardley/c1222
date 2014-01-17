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


#ifndef C1222SERVER_H
#define C1222SERVER_H

#include "c1222stack.h"
#include "c1219table.h"
#include "epsem.h"
#include "acsemsg.h"


typedef struct C1222ServerTag
{
    C1222Stack* pStack;
    
    C1219TableServer* pTableServer;
    Boolean isCommModule;
    Boolean isRelay;
    
    Boolean sendToNonC1222Stack;
    Boolean useCalledApTitleFromRequestForCallingInResponse;
    Boolean useRelativeMacAddressFormForCallingApTitle;
    
    //Unsigned8 responseEpsemBuffer[MAX_EPSEM_MESSAGE_LENGTH];
    Unsigned8* responseEpsemBuffer;
    Unsigned8* meterResponseBuffer;
    
    Unsigned8 processMessageResponse;  // if not C1222RESPONSE_OK, there is an error to report
    
} C1222Server;

void C1222Server_Init(C1222Server* p, C1222Stack* pStack, C1219TableServer* pTableServer,
                      Unsigned8* responseBuffer); // the response buffer must be at least MAX_ACSE_MESSAGE_LENGTH
void C1222Server_SetDestinationToForeignStack(C1222Server* p, Boolean wantForeignStack);
void C1222Server_SetIsRelay(C1222Server* p, Boolean isRelay);
void C1222Server_ProcessMessage(C1222Server* p, ACSE_Message* pMsg, Boolean* pMessageWasSent, Boolean bDoNotNoteReceivedMessageHandlingIsComplete);
Unsigned8 C1222Server_HandleDisconnectRequest(C1222Server* p);
void C1222Server_GetMyIdentification(C1222Server* p, Unsigned8* pId);
Boolean C1222Server_IsMessageForMe(C1222Server* p, ACSE_Message* pMsg);
void C1222Server_NotePowerUp(C1222Server* p, Unsigned32 outageDurationInSeconds);
void C1222Server_SetIsCommModule(C1222Server* p, Boolean isCommModule);
Unsigned8 C1222Server_HandleDeRegisterRequest(C1222Server* pServer, Unsigned8* response, Unsigned16 maxResponseLength, Unsigned16* pResponseLength);
Unsigned16 C1222Server_encodeUIDTerm(Unsigned32 term, Unsigned8* uid, Unsigned16 length);
void C1222Server_MakeRelativeMacAddressFormApTitle(C1222Server* p, Unsigned8* apbuffer, Boolean wantCellID);


#endif
