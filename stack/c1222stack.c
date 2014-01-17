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


#include "c1222environment.h"
#include "c1222stack.h"
#include "c1222misc.h"

#define C1222STACK_WANTTRANSPARENCY     FALSE


void C1222Stack_RequireEncryptionOrAuthenticationIfKeys(C1222Stack* pStack, Boolean b)
{
    C1222AL_RequireEncryptionOrAuthenticationIfKeys(&pStack->Xapplication, b);
}


void C1222Stack_SetClientStandardVersion(C1222Stack* pStack, C1222StandardVersion version)
{
    C1222AL_SetClientStandardVersion( &pStack->Xapplication, version);   
}


void C1222Stack_SetClientSendACSEContextOption( C1222Stack* pStack, Boolean b)
{
    C1222AL_SetClientSendACSEContextOption( &pStack->Xapplication, b );
}




Boolean C1222Stack_IsMessageAvailable(C1222Stack* pStack)
{
    // returns true when a complete message is available
    
    return C1222AL_IsMessageComplete(&pStack->Xapplication);
}


void C1222Stack_Run(C1222Stack* pStack)
{
    pStack->XrunCount++;
    C1222AL_Run(&pStack->Xapplication);
}


void C1222Stack_ClearDiagnostics(C1222Stack* p, Unsigned8 which)
{
    switch(which)
    {
        case C1222STACK_CLEARALLDIAGS:
            // clear all diagnostics
            C1222AL_ClearCounters(&p->Xapplication);
            C1222AL_ClearStatusDiagnostics(&p->Xapplication);
            C1222TL_ClearCounters(&p->Xtransport);
            C1222TL_ClearStatusDiagnostics(&p->Xtransport);
            C1222DL_ClearCounters(&p->Xdatalink);
            C1222DL_ClearStatusDiagnostics(&p->Xdatalink);
            memset(&p->Xphysical.counters, 0, sizeof(p->Xphysical.counters));
            break;
        default:
            break;
    }
}


// should not send a new message until the previous is complete
// also should notify sending stack when the sent message is complete

Boolean C1222Stack_IsSendMessageComplete(C1222Stack* p)
{
    return C1222AL_IsSendMessageComplete(&p->Xapplication);
}


void C1222Stack_NoteReceivedMessageHandlingFailed(C1222Stack* p, Unsigned8 result)
{
    C1222AL_NoteReceivedMessageHandlingFailed(&p->Xapplication, result);
}


void C1222Stack_NoteReceivedMessageHandlingIsComplete(C1222Stack* p)
{
    C1222AL_NoteReceivedMessageHandlingIsComplete(&p->Xapplication);
}


Boolean C1222Stack_IsBusy(C1222Stack* pStack)
{
    return C1222AL_IsBusy(&pStack->Xapplication);
}


Boolean C1222Stack_IsRegistered(C1222Stack* p)
{
    //return C1222AL_IsRegistered(&p->Xapplication);
    return p->Xapplication.status.isRegistered;
}


ACSE_Message* C1222Stack_GetReceivedMessage(C1222Stack* p)
{
    return C1222AL_GetReceivedMessage(&p->Xapplication);
}


Boolean C1222Stack_GetReceivedMessageNativeAddress(C1222Stack* p, Unsigned8* pNativeAddressLength, Unsigned8** pNativeAddress)
{
    return C1222AL_GetReceivedMessageNativeAddress(&p->Xapplication, pNativeAddressLength, pNativeAddress);
}


Unsigned16 C1222Stack_GetNextApInvocationId(C1222Stack* p)
{
    return C1222AL_GetNextApInvocationId(&p->Xapplication);
}


Boolean C1222Stack_GetMyApTitle(C1222Stack* p, C1222ApTitle* pApTitle)
{
    return C1222AL_GetMyApTitle(&p->Xapplication, pApTitle);
}


#ifdef C1222_INCLUDE_COMMMODULE
Boolean C1222Stack_IsMyApTitleAvailable(C1222Stack* p)
{
    return C1222AL_IsMyApTitleAvailable(&p->Xapplication);
}
#endif


Unsigned8 C1222Stack_NoteDisconnectRequest(C1222Stack* pStack)
{
    return C1222AL_NoteDisconnectRequest((C1222AL*)(&pStack->Xapplication));
}



void C1222Stack_NoteDeRegisterRequest(C1222Stack* pStack)
{
    C1222AL_NoteDeRegisterRequest((C1222AL*)(&pStack->Xapplication));
}


Boolean C1222Stack_IsOptionalCommInhibited(C1222Stack* p)
{
    return C1222TL_IsOptionalCommInhibited((C1222TL*)(&p->Xtransport));
}


void C1222Stack_InhibitOptionalCommunication(C1222Stack* p)
{
    C1222TL_InhibitOptionalCommunication((C1222TL*)(&p->Xtransport));
}




Boolean C1222Stack_IsMessageReceiverBusy(C1222Stack* p)
{
    return (Boolean)((C1222MessageReceiver_IsMessageComplete(&p->Xapplication.messageReceiver)) ? FALSE : TRUE);
}


void C1222Stack_CancelPartiallyReceivedMessage(C1222Stack* p)
{
    C1222MessageReceiver_CancelPartialMessage(&p->Xapplication.messageReceiver);
    p->Xapplication.counters.partialMessageCanceled++;
    p->Xapplication.status.timeOfLastDiscardedSegmentedMessage = C1222Misc_GetFreeRunningTimeInMS();
}



Boolean C1222Stack_SendMessage(C1222Stack* p, ACSE_Message* pMsg, Unsigned8* destinationNativeAddress, Unsigned16 addressLength)
{
	return C1222AL_SendMessage(&p->Xapplication, pMsg, destinationNativeAddress, addressLength);
}


Boolean C1222Stack_IsInterfaceEnabled(C1222Stack* p)
{
    return p->Xapplication.status.interfaceEnabled;
}



#ifdef C1222_WANT_SHORT_MESSAGES

Boolean C1222Stack_SendShortMessage(C1222Stack* p, ACSE_Message* pMsg, Unsigned8* destinationNativeAddress, Unsigned16 addressLength)
{
	return C1222AL_SendShortMessage(&p->Xapplication, pMsg, destinationNativeAddress, addressLength);
}

#endif



void C1222Stack_SetConfiguration(C1222Stack* p, C1222TransportConfig* pConfig)
{
    C1222AL_SetConfiguration(&p->Xapplication, pConfig);
}


void C1222Stack_ForceRelativeApTitleInConfig(C1222Stack* p, Boolean b)
{
    C1222TL_ForceRelativeApTitleInConfig(&p->Xtransport, b);
}



void C1222Stack_SetRegistrationRetryLimitsFormat1(C1222Stack* p, Unsigned16 minMinutes, Unsigned16 maxMinutes)
{
    C1222AL_SetRegistrationRetryLimits(&p->Xapplication, minMinutes, maxMinutes, 1);  // use cell switch divisor of 1 for old format
}


void C1222Stack_SetRegistrationRetryLimitsFormat2(C1222Stack* p, Unsigned8 minTime, Unsigned8 maxTimeAdder, Unsigned8 unitsInMinutes, Unsigned8 cellSwitchDivisor)
{
    Unsigned16 minMinutes = minTime * unitsInMinutes;
    Unsigned16 maxMinutes = (minTime+maxTimeAdder)* unitsInMinutes;
    
    C1222AL_SetRegistrationRetryLimits(&p->Xapplication, minMinutes, maxMinutes, cellSwitchDivisor);
}


void C1222Stack_SetRegistrationMaxRetryFactor(C1222Stack* p, Unsigned8 factor)
{
    C1222AL_SetRegistrationMaxRetryFactor(&p->Xapplication, factor);
}




Boolean C1222Stack_IsInterfaceAvailable(C1222Stack* p)
{
    return C1222AL_IsInterfaceAvailable(&p->Xapplication);
}



void C1222Stack_SetPhysicalHalfDuplex(C1222Stack* p, Boolean b)
{
    C1222PL_SetHalfDuplex(&p->Xphysical, b);    
}



#define C1222AL_DEFAULT_SENDTIMEOUT   60

// interface number is the index in table 122, the interface control table

void C1222Stack_Init(C1222Stack* p, C1222LocalPortRouter* pLPR, Unsigned8 myPortID, 
                     Boolean isCommModule, Unsigned8 interfaceNumber, Boolean isLocalPort, Boolean isRelay)
{       
    // initialize the physical layer
    
    C1222PL_Init(&p->Xphysical, myPortID);
    C1222PL_SetLinks(&p->Xphysical, &p->Xdatalink);
    C1222PL_SetHalfDuplex(&p->Xphysical, FALSE);
    
    // initialize the datalink layer
    C1222DL_SetLinks(&p->Xdatalink, &p->Xphysical, &p->Xtransport, pLPR);
    C1222DL_Init(&p->Xdatalink, C1222STACK_WANTTRANSPARENCY, myPortID, isCommModule);
       
    // initialize the transport layer
    C1222TL_Init(&p->Xtransport, isCommModule, &p->Xdatalink, &p->Xapplication, interfaceNumber);
    
    C1222TL_SetIsLocalPort(&p->Xtransport, isLocalPort);
    
    // initialize the application layer
    C1222AL_Init(&p->Xapplication, &p->Xtransport, isCommModule, isLocalPort, isRelay);
    
    C1222DL_InitAtEndOfStackInit(&p->Xdatalink);
    
    C1222AL_SetSendTimeoutSeconds(&p->Xapplication, C1222AL_DEFAULT_SENDTIMEOUT);
    C1222TL_SetSendTimeoutSeconds(&p->Xtransport, C1222AL_DEFAULT_SENDTIMEOUT);
    
    //p->logfp = 0;
    p->XrunCount = 0;
}




void C1222Stack_SetSendTimeoutSeconds(C1222Stack* p, Unsigned16 seconds)
{
    C1222AL_SetSendTimeoutSeconds(&p->Xapplication, seconds);
    C1222TL_SetSendTimeoutSeconds(&p->Xtransport, seconds);
}
 




#ifdef C1222_INCLUDE_DEVICE
void C1222Stack_ReInitAfterRFLANFwDownload(C1222Stack* p, Unsigned8 interfaceNumber, Unsigned8 portID)
{       
    // initialize the physical layer
    
    C1222PL_ReInitAfterRFLANFwDownload(&p->Xphysical);
    
    // initialize the datalink layer
    C1222DL_ReInitAfterRFLANFwDownload(&p->Xdatalink, portID);
       
    // initialize the transport layer
    C1222TL_ReInitAfterRFLANFwDownload(&p->Xtransport, interfaceNumber);
    
    // initialize the application layer
    C1222AL_ReInitAfterRFLANFwDownload(&p->Xapplication, &p->Xtransport);
    
    C1222DL_InitAtEndOfStackInit(&p->Xdatalink);
    
    //p->logfp = 0;
    //p->XrunCount = 0;
}
#endif



void C1222Stack_SetDatalinkCRCByteOrderMode(C1222Stack* p, Boolean detectOk, Boolean isReversed)
{
    C1222DL_SetCRCByteOrderMode(&p->Xdatalink, detectOk, isReversed);
}


Boolean C1222Stack_IsStackWaitingOnReceivedMessageHandlingNotification(C1222Stack* pStack)
{
    return C1222AL_IsStackWaitingOnReceivedMessageHandlingNotification(&pStack->Xapplication);
}




void C1222Stack_NotePowerUp(C1222Stack* pStack, Unsigned32 outageDurationInSeconds)
{
    C1222AL_NotePowerUp((C1222AL*)(&pStack->Xapplication), outageDurationInSeconds);
}


// this will be called from a different task than the c12.22 task in the register
// namely from the event manager task - should abort any pending reads or maybe other requests to the rflan
void C1222Stack_PowerUpInProgress(C1222Stack* p)
{
    C1222AL_PowerUpInProgress((C1222AL*)(&p->Xapplication));
}



void C1222Stack_SetConsecutiveFailureLimit(C1222Stack* p, Unsigned16 limit)
{
    C1222AL_SetConsecutiveFailureLimit((C1222AL*)(&p->Xapplication), limit);
}


void C1222Stack_SetCommLog(C1222Stack* p, void (*comEventLogFunction)(Unsigned16 eventID, void* argument, Unsigned16 argLength))
{
    C1222AL_SetComEventLog(&p->Xapplication, comEventLogFunction);
}


void C1222Stack_SetLog(C1222Stack* p,
	   void (*fp1)(C1222AL* p, Unsigned16 event),
       void (*fp2)(C1222AL* p, Unsigned16 event, Unsigned8 n, Unsigned8* bytes),
       void (*fp3)(C1222AL* p, Unsigned16 event, Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4))
{
    C1222AL_SetLog(&p->Xapplication, fp1, fp2, fp3);
}


void C1222Stack_EnableLog(C1222Stack* p, Boolean enable)
{
    p->Xapplication.logWanted = enable;
    p->Xtransport.logWanted = enable;
    p->Xdatalink.logWanted = enable;
    p->Xphysical.logWanted = enable;
}



void C1222Stack_SimpleResponse(C1222Stack* p, ACSE_Message* pMsg, Unsigned8 response)
{
    C1222AL_SimpleResponse((C1222AL*)(&p->Xapplication), pMsg, response, 0);
}



#ifdef C1222_INCLUDE_COMMMODULE
void C1222Stack_CommModuleResetOnInterrupt(C1222Stack* p)
{
    if ( p->Xapplication.status.isCommModule )
    {
#ifdef C1222_INCLUDE_COMMMODULE
        C1222TL_ResetRequested((C1222TL*)(&p->Xtransport));
#endif        
    }
}
#endif // C1222_INCLUDE_COMMMODULE



void C1222Stack_ResetTransport(C1222Stack* p)
{
    C1222TL_ResetRequested((C1222TL*)(&p->Xtransport));
}
