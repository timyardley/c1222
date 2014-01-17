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

#ifndef C1222TRANSPORT_H
#define C1222TRANSPORT_H

#include "c1222.h"
#include "acsemsg.h"
#include "c1222event.h"

#define C1222TL_AUTOREGISTRATIONCONTROL_SHIFT   2
#define C1222TL_INTERFACECONTROL_SHIFT          0
#define C1222TL_DIRECTMESSAGINGCONTROL_SHIFT    4
#define C1222TL_RESETSTATISTIC_SHIFT            6
#define C1222TL_RELOADCONFIG_SHIFT              7

#define C1222TL_INTF_CTRL_MAINTAIN_CURRENT_STATE            (0<<C1222TL_INTERFACECONTROL_SHIFT)
#define C1222TL_INTF_CTRL_ENABLEINTERFACE                   (1<<C1222TL_INTERFACECONTROL_SHIFT)
#define C1222TL_INTF_CTRL_DISABLEINTERFACE                  (2<<C1222TL_INTERFACECONTROL_SHIFT)
#define C1222TL_INTF_CTRL_RESETINTERFACE                    (3<<C1222TL_INTERFACECONTROL_SHIFT)

#define C1222TL_AUTOREG_CTRL_DISABLE_AUTO_REGISTRATION      (1<<C1222TL_AUTOREGISTRATIONCONTROL_SHIFT)
#define C1222TL_AUTOREG_CTRL_ENABLE_AUTO_REGISTRATION       (2<<C1222TL_AUTOREGISTRATIONCONTROL_SHIFT)
#define C1222TL_AUTOREG_CTRL_FORCE_ONETIME_REGISTRATION     (3<<C1222TL_AUTOREGISTRATIONCONTROL_SHIFT)

#define C1222TL_DIRECT_MESSAGING_MAINTAIN_CURRENT_STATE (0<<C1222TL_DIRECTMESSAGINGCONTROL_SHIFT)

#define C1222TL_RELOAD_CONFIG_FLAG                      (1<<C1222TL_RELOADCONFIG_SHIFT)


// max transport message size is 150 bytes for payload plus 6 byte native address plus 2 bytes 
// for send message tag and native address length
#define MAX_TRANSPORT_OVERHEAD        8
#define MAX_TRANSPORT_PAYLOAD       150
#define MAX_TRANSPORT_MESSAGE_SIZE  (MAX_TRANSPORT_PAYLOAD+MAX_TRANSPORT_OVERHEAD)

// collision handling
#define C1222TL_DELAYED_GETCONFIG    0x01
#define C1222TL_DELAYED_GETSTATUS    0x02
#define C1222TL_DELAYED_GETREGSTATUS 0x04
#define C1222TL_DELAYED_NEGOTIATE    0x08
#define C1222TL_DELAYED_LINKCONTROL  0x10


// interface status bits
#define C1222_TL_STATUS_INTERFACE_FLAG          0x01
#define C1222_TL_STATUS_AUTO_REGISTRATION_FLAG  0x02
#define C1222_TL_STATUS_DIRECT_MESSAGING_FLAG   0x04


typedef struct C1222TLCountersTag
{
    Unsigned16 negotiateSucceeded;
    Unsigned16 negotiateAttempted;
    
    Unsigned32 messagesSent;
    Unsigned32 messagesReceived;

    Unsigned16 numberOfResets;
    
    Unsigned16 runCount;
    
    Unsigned16 incompleteMessageDiscardedCount;
    Unsigned16 completeMessageDiscarded;
    Unsigned16 tooLongMessageDiscarded;
    
    Unsigned16 linkResetByDatalink;
    Unsigned16 pendingSendAborted;
    Unsigned16 pendingOtherRequestAborted;
    
    Unsigned32 acseMessagesSent;
    
    Unsigned16 resetRequestedByApplication;
    
    Unsigned32 acseMessagesResponseDelayed;
    
} C1222TLCounters;


typedef enum C1222TL_StateTag {TLS_INIT, TLS_DISCONNECTED, TLS_NORMAL, TLS_NEEDCONFIG
#if 0 
    ,TLS_CM_DISABLED, TLS_CM_ENABLED, TLS_DEVICE_COMMMODULEPRESENT, TLS_DEVICE_COMMMODULEDISABLED      
#endif
} C1222TL_State;


typedef struct C1222TLStatusTag
{
    C1222TL_State state;
    
    Boolean negotiateComplete;
    Boolean haveConfiguration;
    Boolean commModulePresent;   // for the c12.22 device
    Boolean waitingForConfigRequest; // for the c12.22 device
    Boolean isEnabled; // for the comm module
    
    Boolean isResetNeeded;
        
    Unsigned8 pendingRequest;
    Unsigned8 pendingRequestU8Parameter;
    
    Boolean isCommModule;
    Boolean isLocalPort;
    Unsigned8 interfaceNumber;
    
    Unsigned32 lastRequestTime;
    Unsigned32 lastNegotiateTime;    
    
    Unsigned8 delayedRequests;
    Unsigned8 delayedStatusRequestType;
    Unsigned8 delayedLinkControlType;
    
    Unsigned32 delayStart;
    Unsigned16 delayMs;
    
    Boolean isSendMessageOKResponsePending;
    
    Unsigned8 configFailureReason;
    
    Boolean sendInProgress;
    Boolean receivedMessageIsComplete;
    Boolean receiveInProgress;
    
    Unsigned32 lastResetTime;
    
    Unsigned32 lastMessageReceivedTime;
    Unsigned16 maxReceivedMessageProcessTime;
    Unsigned16 avgReceivedMessageProcessTime;
    Unsigned32 totalReceivedMessageProcessTime;
    
    Unsigned32 lastMessageSendTime;
    Unsigned16 maxMessageSendTime;
    Unsigned16 avgMessageSendTime;
    Unsigned32 totalMessageSendTime;
    
    Unsigned32 lastDisconnected;
    Unsigned32 lastConfigSent;
    
    Boolean    sendMessageResponseIsPending;
    Unsigned8  pendingSendMessageResponse;
    Unsigned32 pendingSendResponseReceived;
    
    Unsigned32 noOptionalCommPeriodStart; // to allow comm module config time to happen
    
    Unsigned16 sendTimeoutSeconds;
    Boolean    forceRelativeApTitleInConfig;
    
} C1222TLStatus;




typedef struct C1222TLTag
{
    C1222TLStatus status;
    
    ACSE_Message  rxMessage;   // temp - need to allocate later
    Unsigned8     rxMessageBuffer[MAX_TRANSPORT_MESSAGE_SIZE];
    Unsigned8     rxMessageCanary[10];
    Unsigned16    rxMessageLength;    
    
    void* pDatalink;
    void* pApplication;
        
    Boolean logWanted;
    
    Boolean powerUpInProgress;
    
    C1222TLCounters counters;
    
    Unsigned8 tempBuffer[100];  // needs to be at least as much as transport config which is 92 bytes
    
} C1222TL;



// services for application layer

void C1222TL_Run(C1222TL* p);

Boolean C1222TL_IsBusy(C1222TL* p);

void C1222TL_Init(C1222TL* p, Boolean isCM, void* pDatalink, void* pApplication, Unsigned8 interfaceNumber);

Boolean C1222TL_SendACSEMessage(C1222TL* p, Unsigned8* message, Unsigned16 length, 
            Unsigned8* destinationNativeAddress, Unsigned16 addressLength);

Boolean C1222TL_SendShortMessage(C1222TL* p, Unsigned8* message, Unsigned16 length, 
            Unsigned8* destinationNativeAddress, Unsigned16 addressLength);
            
Unsigned16 C1222TL_GetMaxPayloadSize(C1222TL* p); 

Boolean C1222TL_IsReady(C1222TL* p);           
            
// these routines send the indicated service.  They do not wait for responses.
// the responses will be handled in _DLNoteMessageRxComplete            
void C1222TL_SendNegotiateRequest(C1222TL* p);  // params are all properties of datalink layer
void C1222TL_SendLinkControlRequest(C1222TL* p, Unsigned8 linkControlByte);
void C1222TL_SendGetConfigRequest(C1222TL* p);
void C1222TL_SendGetStatusRequest(C1222TL* p, Unsigned8 statusControlByte);
void C1222TL_SendGetRegistrationStatusRequest(C1222TL* p);

void C1222TL_SendPendingSendMessageOKResponse(C1222TL* p);
void C1222TL_SendPendingSendMessageFailedResponse(C1222TL* p, Unsigned8 result);


Boolean C1222TL_IsInterfaceAvailable(C1222TL* p);

void C1222TL_NotePowerUp(C1222TL* p, Unsigned32 outageDurationInSeconds);

Unsigned8 C1222TL_GetInterfaceNumber(C1222TL* p);

Boolean C1222TL_DLIsKeepAliveNeeded(C1222TL* p);

// services for datalink layer

void C1222TL_DLNotePacketReceived(C1222TL* p, Unsigned8* packetData, Unsigned16 length, Unsigned16 offset);
void C1222TL_DLNoteLinkFailure(C1222TL* p);
void C1222TL_DLNoteMessageRxComplete(C1222TL* p);

void C1222TL_IncrementStatistic(C1222TL* p, Unsigned16 id);
void C1222TL_AddToStatistic(C1222TL* p, Unsigned16 id, Unsigned32 count);

void C1222TL_LogEvent(C1222TL* p, C1222EventId event);
void C1222TL_LogEventAndBytes(C1222TL* p, C1222EventId event, Unsigned8 n, Unsigned8* bytes);
void C1222TL_LogEventAndULongs(C1222TL* p, C1222EventId event, 
        Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4);


void C1222TL_SetIsLocalPort(C1222TL* p, Boolean isLocalPort);

void C1222TL_DLNoteKeepAliveReceived(C1222TL* p);

Boolean C1222TL_IsHappy(C1222TL* p);

void C1222TL_ClearCounters(C1222TL* p);
void C1222TL_ClearStatusDiagnostics(C1222TL* p); 

void C1222TL_ResetRequested(C1222TL* p); 

//void C1222TL_ReInitAfterRFLANFwDownload(C1222TL* p);
void C1222TL_ReInitAfterRFLANFwDownload(C1222TL* p, Unsigned8 interfaceNumber); 
Boolean C1222TL_IsOptionalCommInhibited(C1222TL* p); 

void C1222TL_PowerUpInProgress(C1222TL* p); 

void C1222TL_SetSendTimeoutSeconds(C1222TL* p, Unsigned16 seconds); 

void C1222TL_InhibitOptionalCommunication(C1222TL* p); 

Unsigned16 C1222TL_GetMaxSendMessagePayloadSize(C1222TL* p);

void C1222TL_ForceRelativeApTitleInConfig(C1222TL* p, Boolean b);            



#endif
