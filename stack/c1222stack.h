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


#ifndef C1222STACK_H
#define C1222STACK_H

#include "c1222al.h"
#include "c1222tl.h"
#include "c1222dl.h"
#include "c1222pl.h"
#include "c1222event.h"


typedef struct C1222StackTag
{   
    C1222AL Xapplication;
    C1222TL Xtransport;
    C1222DL Xdatalink;
    C1222PL Xphysical;
    
    Unsigned16 XrunCount;
    
    //FILE* logfp;
} C1222Stack;

// public methods of C1222Stack
void C1222Stack_Run(C1222Stack* p);
Boolean C1222Stack_IsMessageAvailable(C1222Stack* p);
Boolean C1222Stack_SendMessage(C1222Stack* p, ACSE_Message* pMsg, Unsigned8* destinationNativeAddress, Unsigned16 addressLength);
Boolean C1222Stack_SendShortMessage(C1222Stack* p, ACSE_Message* pMsg, Unsigned8* destinationNativeAddress, Unsigned16 addressLength);
Boolean C1222Stack_ForwardMessage(C1222Stack* p, ACSE_Message* pMsg, Unsigned8* destinationNativeAddress, Unsigned16 addressLength);
ACSE_Message* C1222Stack_GetReceivedMessage(C1222Stack* p);
void C1222Stack_SetConfiguration(C1222Stack* p, C1222TransportConfig* pConfig);
void C1222Stack_Init(C1222Stack* p, C1222LocalPortRouter* pLPR, Unsigned8 myPortID, Boolean isCommModule,
              Unsigned8 interfaceNumber, Boolean isLocalPort, Boolean isRelay);
Boolean C1222Stack_GetReceivedMessageNativeAddress(C1222Stack* p, Unsigned8* pNativeAddressLength, Unsigned8** pNativeAddress);
Unsigned16 C1222Stack_GetNextApInvocationId(C1222Stack* p);
Boolean C1222Stack_IsBusy(C1222Stack* pStack);
Boolean C1222Stack_IsRegistered(C1222Stack* p);
Boolean C1222Stack_IsSendMessageComplete(C1222Stack* p);
void C1222Stack_NoteReceivedMessageHandlingIsComplete(C1222Stack* p);
void C1222Stack_NoteReceivedMessageHandlingFailed(C1222Stack* p, Unsigned8 result);
Boolean C1222Stack_IsMessageReceiverBusy(C1222Stack* p);
void C1222Stack_CancelPartiallyReceivedMessage(C1222Stack* p);

void C1222Stack_NotePowerUp(C1222Stack* pStack, Unsigned32 outageDurationInSeconds);

void C1222Stack_LogEvent(C1222Stack* p, C1222EventId event); 

void C1222Stack_SetLog(C1222Stack* p,
	   void (*fp1)(C1222AL* p, Unsigned16 event),
       void (*fp2)(C1222AL* p, Unsigned16 event, Unsigned8 n, Unsigned8* bytes),
       void (*fp3)(C1222AL* p, Unsigned16 event, Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4));
 

void C1222Stack_SimpleResponse(C1222Stack* p, ACSE_Message* pMsg, Unsigned8 response);

Boolean C1222Stack_IsInterfaceAvailable(C1222Stack* p);  // returns true if a comm module or device is present on this interface

void C1222Stack_EnableLog(C1222Stack* p, Boolean enable);

void C1222Stack_SetRegistrationRetryLimitsFormat1(C1222Stack* p, Unsigned16 minMinutes, Unsigned16 maxMinutes);

void C1222Stack_SetRegistrationRetryLimitsFormat2(C1222Stack* p, Unsigned8 minTime, Unsigned8 maxTimeAdder, Unsigned8 timeUnits, Unsigned8 cellSwitchDivisor);

Boolean C1222Stack_GetMyApTitle(C1222Stack* p, C1222ApTitle* pApTitle);
Boolean C1222Stack_IsMyApTitleAvailable(C1222Stack* p);

Boolean C1222Stack_IsInterfaceEnabled(C1222Stack* p);

void C1222Stack_NoteDeRegisterRequest(C1222Stack* pStack);

#define C1222STACK_CLEARALLDIAGS   0

void C1222Stack_ClearDiagnostics(C1222Stack* p, Unsigned8 which);

void C1222Stack_CommModuleResetOnInterrupt(C1222Stack* p);

void C1222Stack_SetRegistrationMaxRetryFactor(C1222Stack* p, Unsigned8 factor);

void C1222Stack_ResetTransport(C1222Stack* p);

void C1222Stack_ReInitAfterRFLANFwDownload(C1222Stack* p, Unsigned8 interfaceNumber, Unsigned8 portID);

Boolean C1222Stack_IsOptionalCommInhibited(C1222Stack* p);

void C1222Stack_PowerUpInProgress(C1222Stack* p);

Boolean C1222Stack_IsStackWaitingOnReceivedMessageHandlingNotification(C1222Stack* pStack);

void C1222Stack_SetSendTimeoutSeconds(C1222Stack* p, Unsigned16 seconds);

void C1222Stack_InhibitOptionalCommunication(C1222Stack* p);

void C1222Stack_SetDatalinkCRCByteOrderMode(C1222Stack* p, Boolean detectOk, Boolean isReversed);

void C1222Stack_SetClientStandardVersion(C1222Stack* p, C1222StandardVersion version);

void C1222Stack_RequireEncryptionOrAuthenticationIfKeys(C1222Stack* p, Boolean b);

void C1222Stack_SetPhysicalHalfDuplex(C1222Stack* p, Boolean b);

Unsigned8 C1222Stack_NoteDisconnectRequest(C1222Stack* p);

void C1222Stack_SetCommLog(C1222Stack* p, void (*comEventLogFunction)(Unsigned16 eventID, void* argument, Unsigned16 argLength));

void C1222Stack_SetConsecutiveFailureLimit(C1222Stack* p, Unsigned16 limit);

void C1222Stack_ForceRelativeApTitleInConfig(C1222Stack* p, Boolean b);




#ifdef C1222_INCLUDE_COMMMODULE
Boolean C1222Stack_SendMessageToNonC1222Stack(ACSE_Message* pMsg, Unsigned8* destinationNativeAddress, Unsigned16 addressLength);
#endif


#endif
