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


// C12.22 Transport Layer


#include "c1222environment.h"
#include "c1222misc.h"
#include "c1222tl.h"
#include "c1222dl.h"
#include "c1222al.h"
#include "c1222response.h"
#include "c1222pl.h"


//typedef struct C1222TLTag
//{
//    C1222TL_State state;
//    
//    void* pDatalink;
//    void* pApplication;
//} C1222TL;


// prototypes for static routines in this module



#ifdef C1222_INCLUDE_COMMMODULE
static void ProcessReceivedLinkControlRequest(C1222TL* p);
static void ProcessReceivedGetStatusRequest(C1222TL* p);
static void ProcessReceivedGetRegistrationStatusRequest(C1222TL* p);
static void ProcessReceivedNegotiateResponse(C1222TL* p);
static void ProcessReceivedGetConfigResponse(C1222TL* p);
#endif // C1222_INCLUDE_COMMMODULE
static void ProcessReceivedSendMessageRequest(C1222TL* p);
static void ProcessReceivedSendShortMessageRequest(C1222TL* p);
static void ProcessReceivedUndefinedRequest(C1222TL* p);


static void ProcessReceivedSendMessageResponse(C1222TL* p);

#ifdef C1222_INCLUDE_DEVICE
static void ProcessReceivedGetStatusResponse(C1222TL* p);
static void ProcessReceivedLinkControlResponse(C1222TL* p);
static void ProcessReceivedGetConfigRequest(C1222TL* p);
static void ProcessReceivedNegotiateRequest(C1222TL* p);
#endif

#ifdef C1222_REGISTRATION_HANDLED_BY_COMMMODULE
static void ProcessReceivedGetRegistrationStatusResponse(C1222TL* p);
#endif // C1222_REGISTRATION_HANDLED_BY_COMMMODULE
static void ProcessReceivedSendShortMessageResponse(C1222TL* p);
//static void ProcessReceivedUndefinedResponse(C1222TL* p);

static void SelectPacketLimits(C1222TL* p, Unsigned16* pMaxPacketSize, Unsigned8* pMaxNumberOfPackets);
#ifdef C1222_INCLUDE_DEVICE
static Unsigned8 SelectBaudRate(C1222TL* p, Unsigned8 numberOfBauds, Unsigned8* baudSelectorList, Unsigned32* pBaudRate);
#endif
static void ProcessResponseMessage(C1222TL* p, Unsigned8 response);
static void ProcessCommandMessage(C1222TL* p, Unsigned8 command);
static void NoteRequest(C1222TL* p, Unsigned8 command);
static Boolean AddLengthAndItemToMessage(C1222DL* pDatalink, Unsigned8 length, Unsigned8* item);

static Unsigned8 getUIDLength(Unsigned8* uid);

static void HandleReceivedTransportMessage(C1222TL* p);
static void WaitABit(Unsigned32 delay);
static void NoteMessageSendStarted(C1222TL* p);
static void SetShortDelayEnd(C1222TL* p);

static void C1222TL_NoteOptionalCommAllowed(C1222TL* p);


#define C1222_WANT_CODE_REDUCTION


// *****************************************************************************
// services for application layer
// *****************************************************************************


Boolean C1222TL_IsBusy(C1222TL* p)
{
    if ( p->status.pendingRequest != 0 )
        return TRUE;
        
    if ( p->status.state == TLS_NORMAL )
    {
        return (Boolean)(!C1222TL_IsReady(p));
    }
    
    return TRUE;
}





void HandleDelayedRequests(C1222TL* p)
{
    if ( p->status.delayedRequests != 0 )
    {
        if ( (Unsigned32)(C1222Misc_GetFreeRunningTimeInMS()-p->status.delayStart) >= (Unsigned32)(p->status.delayMs) )
        {
            C1222TL_LogEvent(p, C1222EVENT_TL_SENDING_DELAYED_REQUEST);
            
            if ( p->status.delayedRequests & C1222TL_DELAYED_NEGOTIATE )
            {
                #ifdef C1222_INCLUDE_COMMMODULE
                C1222TL_SendNegotiateRequest(p);
                #endif
            }
            else if ( p->status.delayedRequests & C1222TL_DELAYED_GETCONFIG )
            {
                #ifdef C1222_INCLUDE_COMMMODULE
                C1222TL_SendGetConfigRequest(p);
                #endif
            }
            else if ( p->status.delayedRequests & C1222TL_DELAYED_GETSTATUS )
            {
                #ifdef C1222_INCLUDE_DEVICE
                C1222TL_SendGetStatusRequest(p, p->status.delayedStatusRequestType );
                #endif
            }
            else if ( p->status.delayedRequests & C1222TL_DELAYED_GETREGSTATUS )
            {
                #ifdef C1222_REGISTRATION_HANDLED_BY_COMMMODULE
                C1222TL_SendGetRegistrationStatusRequest(p);
                #endif
            }
            else if ( p->status.delayedRequests & C1222TL_DELAYED_LINKCONTROL )
            {
                #ifdef C1222_INCLUDE_DEVICE
                C1222TL_SendLinkControlRequest(p, p->status.delayedLinkControlType);
                #endif
            }
        }
    }
}




void C1222TL_DLNoteKeepAliveReceived(C1222TL* p)
{
#if 0  // lets try with forcing a real negotiate
    if ( !p->status.negotiateComplete )
    {
        p->status.negotiateComplete = TRUE;
        //p->lastNegotiateTime = C1222Misc_GetFreeRunningTimeInMS();
        //p->waitingForConfigRequest = TRUE;
        
        p->status.commModulePresent = TRUE;
        p->status.state = TLS_NORMAL;
        
    }
#else
    C1222TL_NoteOptionalCommAllowed(p);   
#endif    
}


static void C1222TL_NoteOptionalCommAllowed(C1222TL* p)
{
    
    if ( p->status.noOptionalCommPeriodStart != 0 )
    {
        C1222TL_LogEvent(p, C1222EVENT_DEVICE_OPTIONAL_COMM_ALLOWED);
    }
    
    p->status.noOptionalCommPeriodStart = 0;  // cancel the delay
}




static void AbortPendingRequest(C1222TL* p)
{
    Unsigned8 pendingRequest = p->status.pendingRequest;
    
    p->status.pendingRequest = 0;
            
    if ( pendingRequest == 0x74 )
    {
        p->counters.pendingSendAborted++;
        C1222AL_TL_NoteFailedSendMessage((C1222AL*)(p->pApplication));
    }
    else if ( pendingRequest != 0 )
    {
        p->counters.pendingOtherRequestAborted++;
        C1222AL_TL_NoteLinkFailure((C1222AL*)(p->pApplication));    
    }
}


void C1222TL_ResetRequested(C1222TL* p)
{
    C1222TL_LogEvent(p, C1222EVENT_TL_LINK_RESET);
    
    p->status.isResetNeeded = TRUE;
    p->counters.resetRequestedByApplication++;
}



#define C1222_TRANSPORT_TIMEOUT   (60000L)



void C1222TL_SetSendTimeoutSeconds(C1222TL* p, Unsigned16 seconds)
{
    p->status.sendTimeoutSeconds = seconds;
}


void C1222TL_Run(C1222TL* p)
{
    Unsigned32 now = C1222Misc_GetFreeRunningTimeInMS();
    Boolean resetProcessed = FALSE;
    
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    
    
    p->counters.runCount++;
    
    if ( p->status.isResetNeeded )
    {
        p->status.state = TLS_INIT;
        p->counters.numberOfResets++;
        resetProcessed = TRUE;
        
        p->status.isResetNeeded = FALSE;
        p->status.lastResetTime = now;
        
        if ( !p->status.isCommModule && !C1222TL_IsOptionalCommInhibited(p) )
        {
            p->status.negotiateComplete = FALSE;
            #ifdef C1222_INCLUDE_DEVICE
            C1222TL_SendLinkControlRequest(p, C1222TL_INTF_CTRL_RESETINTERFACE);
            #endif
        }
    }
    else if ( !p->status.negotiateComplete 
           && C1222Misc_DelayExpired(p->status.lastResetTime, 30000L )
           && !p->status.isCommModule )
    {
        WaitABit(9000L); // give the peer a traffic timeout to elicit a negotiate
    }

    HandleReceivedTransportMessage(p);
    
    // make sure a subsequent received message does not cancel a reset
    
    if ( resetProcessed )
        p->status.state = TLS_INIT;   
    

    switch(p->status.state)
    {
        case TLS_INIT:
            p->status.state = TLS_DISCONNECTED;

            if ( p->status.isCommModule )
            {
                p->status.negotiateComplete = FALSE;
                p->status.haveConfiguration = (Boolean)(p->status.isLocalPort);
                //C1222TL_SendNegotiateRequest(p);
            }
            else
            {
                p->status.haveConfiguration = TRUE;
                p->status.negotiateComplete = FALSE;
            }
            
            if ( p->status.pendingRequest != 0 )
                AbortPendingRequest(p);
            else if ( resetProcessed )
                C1222AL_TL_NoteLinkFailure((C1222AL*)(p->pApplication));
               
#ifdef C1222_INCLUDE_COMMMODULE                
            if ( p->status.isCommModule )
            {
                C1222TL_SendNegotiateRequest(p);
            }    
#endif            
                
            return;
        
        case TLS_DISCONNECTED:
            // once in a while we should send the negotiate request again
#ifdef C1222_INCLUDE_COMMMODULE            
            if ( p->status.isCommModule )
            {
                if ( !p->status.negotiateComplete )
                {
                    if ( !p->status.delayedRequests & C1222TL_DELAYED_NEGOTIATE )
                    {
                        if ( C1222Misc_DelayExpired(p->status.lastRequestTime, 3000) )  // retry once a second
                        {
                            C1222TL_SendNegotiateRequest(p);
                        }
                    }
                }
                else if ( !p->status.haveConfiguration )
                {
                    if ( !p->status.delayedRequests & C1222TL_DELAYED_GETCONFIG )
                    {
                        if ( C1222Misc_DelayExpired(p->status.lastRequestTime, 3000) )  // retry once a second
                        {
                            C1222TL_SendGetConfigRequest(p);
                        }
                    }
                }
            }
#endif // C1222_INCLUDE_COMMMODULE           

            if ( p->status.pendingRequest == 0 )
                HandleDelayedRequests(p);
                
            p->status.lastDisconnected = C1222Misc_GetFreeRunningTimeInMS();

            C1222DL_Run(pDatalink);
            break;

        case TLS_NEEDCONFIG:
#ifdef C1222_INCLUDE_COMMMODULE        
            C1222TL_SendGetConfigRequest(p);
#endif            
            p->status.state = TLS_DISCONNECTED;
            break;

        case TLS_NORMAL:
            if ( p->status.waitingForConfigRequest )
            {
                if ( (now-(p->status.lastNegotiateTime)) > 8000 )
                    p->status.waitingForConfigRequest = FALSE;
            }
            
            if ( p->status.sendMessageResponseIsPending )
            {
                if ( (C1222Misc_GetFreeRunningTimeInMS()-p->status.pendingSendResponseReceived) > (p->status.sendTimeoutSeconds*1000L) )
                {
                    p->status.sendMessageResponseIsPending = FALSE;
                    C1222AL_TL_NoteFailedSendMessage((C1222AL*)(p->pApplication));
                }
            }

            if ( p->status.pendingRequest == 0 )
                HandleDelayedRequests(p);

            C1222DL_Run(pDatalink);
            break;
    }


    HandleReceivedTransportMessage(p);


    if ( p->status.pendingRequest != 0 )
    {
        now = C1222Misc_GetFreeRunningTimeInMS();

        if ( (Unsigned32)(now-p->status.lastRequestTime) > (p->status.sendTimeoutSeconds*1000L) )        //(Unsigned32)(pDatalink->status.trafficTimeoutMs) )
        {
            AbortPendingRequest(p);
            
            C1222TL_LogEvent(p, C1222EVENT_TL_NO_RESPONSE_TO_LAST_REQUEST);
        }
    }
}


// this will be called from a different task than the c12.22 task in the register
// namely from the event manager task - should abort any pending reads or maybe other requests to the rflan

void C1222TL_PowerUpInProgress(C1222TL* p)
{
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    
    // anything else needed?
    
    p->powerUpInProgress = TRUE;
    
    C1222DL_PowerUpInProgress(pDatalink);
}


void C1222TL_NotePowerUp(C1222TL* p, Unsigned32 outageDurationInSeconds)
{
    Unsigned32 now;
    
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    
    p->powerUpInProgress = FALSE;
    
    // the comm module will reset - the register needs to deal with that
    
    if ( !p->status.isCommModule )
    {
        now = C1222Misc_GetFreeRunningTimeInMS();
        
        p->status.commModulePresent = FALSE;
        p->status.negotiateComplete = FALSE;
        p->status.waitingForConfigRequest = FALSE;
        p->status.lastResetTime = now;  // this avoids waiting 9 seconds after powerup
        
        p->status.pendingRequest = 0;
        p->status.delayedRequests = 0;

        p->status.state = TLS_INIT;
        
        p->status.noOptionalCommPeriodStart = now;
    }
    
    C1222DL_NotePowerUp(pDatalink, outageDurationInSeconds);
}






Boolean C1222TL_IsInterfaceAvailable(C1222TL* p)
{
    if ( (p->status.state != TLS_DISCONNECTED) && (p->status.state != TLS_INIT) )
        return TRUE;  // must be ok
        
    return C1222DL_IsInterfaceAvailable((C1222DL*)(p->pDatalink));
}




void C1222TL_SetIsLocalPort(C1222TL* p, Boolean isLocalPort)
{
    p->status.isLocalPort = isLocalPort;
    
    if ( isLocalPort )
    {
    }
}







// used for both normal init and init after rflan download

static void CommonInit(C1222TL* p, Unsigned8 interfaceNumber)
{
    p->status.state = TLS_INIT;
    p->rxMessage.buffer = 0;
    p->rxMessage.length = 0;
    memset(p->rxMessageBuffer, 0, MAX_TRANSPORT_MESSAGE_SIZE);
    memset(p->rxMessageCanary, 0xcd, sizeof(p->rxMessageCanary));
    p->rxMessageLength = 0;
    
    p->status.negotiateComplete = FALSE;
    
    p->status.commModulePresent = FALSE;
    //p->status.haveConfiguration = TRUE;  // this will only be called in the register
    p->status.interfaceNumber = interfaceNumber;

    p->status.waitingForConfigRequest = FALSE;
    p->status.isEnabled = TRUE;
    p->status.isResetNeeded = FALSE;

    p->status.isSendMessageOKResponsePending = FALSE;

    p->status.sendInProgress = FALSE;
    p->status.receivedMessageIsComplete = FALSE;  
    
    p->status.delayedRequests = 0;
    
    p->status.receiveInProgress = FALSE;
    
    p->status.sendMessageResponseIsPending = FALSE;  
    
}


void C1222TL_ReInitAfterRFLANFwDownload(C1222TL* p, Unsigned8 interfaceNumber)
{

    CommonInit(p, interfaceNumber);
    
    p->status.haveConfiguration = TRUE;  // this will only be called in the register    
}


void C1222TL_ForceRelativeApTitleInConfig(C1222TL* p, Boolean b)
{
    p->status.forceRelativeApTitleInConfig = b;
}


void C1222TL_Init(C1222TL* p, Boolean isCM, void* pDatalink, void* pApplication, Unsigned8 interfaceNumber)
{
    memset(&p->counters, 0, sizeof(p->counters));
    memset(&p->status, 0, sizeof(p->status));
    
    p->pDatalink = pDatalink;
    p->pApplication = pApplication;
    p->status.isCommModule = isCM;

    p->status.haveConfiguration = (Boolean)(!isCM);

    p->status.isLocalPort = FALSE;


    p->logWanted = FALSE;
    
    p->powerUpInProgress = FALSE;
    
    CommonInit(p, interfaceNumber);
}




static void NoteReceivedMessageHandlingComplete(C1222TL* p)
{
    Unsigned32 elapsedTime = C1222Misc_GetFreeRunningTimeInMS() - p->status.lastMessageReceivedTime;
    
    p->status.totalReceivedMessageProcessTime += elapsedTime;
    
    if ( p->counters.messagesReceived > 0 )
    {
        p->status.avgReceivedMessageProcessTime = (Unsigned16)(p->status.totalReceivedMessageProcessTime/p->counters.messagesReceived);
    }
    else
        p->status.avgReceivedMessageProcessTime = 0;
           
    if ( elapsedTime > (Unsigned32)(p->status.maxReceivedMessageProcessTime) )
        p->status.maxReceivedMessageProcessTime = (Unsigned16)elapsedTime;
}



void C1222TL_SendPendingSendMessageFailedResponse(C1222TL* p, Unsigned8 response)
{
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    
    C1222TL_LogEventAndBytes(p, C1222EVENT_TLSENDPENDINGFAILEDRESPONSE, 1, &response);

    if ( p->status.isSendMessageOKResponsePending )
    {
        p->status.isSendMessageOKResponsePending = FALSE;
        
        NoteReceivedMessageHandlingComplete(p);

        p->status.sendInProgress = TRUE;

        C1222DL_SendMessage(pDatalink, &response, 1);

        p->status.sendInProgress = FALSE;
    }
    else
    {
        // this is an error
        
        C1222TL_LogEvent(p, C1222EVENT_TLSENDPENDINGFAILEDRESPONSENOTPENDING);
    }
    
}




void C1222TL_SendPendingSendMessageOKResponse(C1222TL* p)
{
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    Unsigned8 response = C1222RESPONSE_OK;
    
    C1222TL_LogEventAndBytes(p, C1222EVENT_TLSENDPENDINGOKRESPONSE, 1, &p->status.isSendMessageOKResponsePending);

    if ( p->status.isSendMessageOKResponsePending )
    {
        p->status.isSendMessageOKResponsePending = FALSE;
        
        NoteReceivedMessageHandlingComplete(p);
        
        C1222TL_LogEvent(p, C1222EVENT_TL_SENDING_SEND_MSG_RESPONSE);
        
        p->status.sendInProgress = TRUE;
    
        C1222DL_SendMessage(pDatalink, &response, 1);        
        
        p->status.sendInProgress = FALSE;
    }
    else
    {
        // this is ok for example if a registration request was generated
        // in the comm module stack - so there was no message from the transport
        // that caused that message to be sent.
    }
}





void C1222TL_LogEventAndBytes(C1222TL* p, C1222EventId event, Unsigned8 n, Unsigned8* bytes)
{
    C1222AL* pAL = (C1222AL*)(p->pApplication);
    
    if ( p->logWanted )
        C1222AL_LogEventAndBytes(pAL, event, n, bytes);
    else
        C1222AL_IncrementEventCount(pAL, (Unsigned16)event);
}




void C1222TL_LogEvent(C1222TL* p, C1222EventId event)
{
    C1222AL* pAL = (C1222AL*)(p->pApplication);
    
    if ( p->logWanted )
        C1222AL_LogEvent(pAL, event);
    else
        C1222AL_IncrementEventCount(pAL, (Unsigned16)event);
}




void C1222TL_LogEventAndULongs(C1222TL* p, C1222EventId event, 
        Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4)
{
    C1222AL* pAL = (C1222AL*)(p->pApplication);
    
    if ( p->logWanted )
        C1222AL_LogEventAndULongs(pAL, event, u1, u2, u3, u4);
    else
        C1222AL_IncrementEventCount(pAL, (Unsigned16)event);
}



void SetDelayEnd(C1222TL* p)
{    
    p->status.delayStart = C1222Misc_GetFreeRunningTimeInMS();
    p->status.delayMs = (Unsigned16)C1222Misc_GetRandomDelayTime(1000, 5000);
    
    C1222TL_LogEventAndULongs(p, C1222EVENT_TL_DELAY, p->status.delayStart, p->status.delayMs, 0, 0);   
}




void SetShortDelayEnd(C1222TL* p)
{    
    p->status.delayStart = C1222Misc_GetFreeRunningTimeInMS();
    p->status.delayMs = (Unsigned16)C1222Misc_GetRandomDelayTime(1000, 2000);
    
    C1222TL_LogEventAndULongs(p, C1222EVENT_TL_DELAY, p->status.delayStart, p->status.delayMs, 0, 0);   
}



// send message service does not use a transport buffer, so the send is limited to the 
// max datalink payload size and the number of datalink packets less the transport overhead
// this will be used for segmenting messages in the application layer

Unsigned16 C1222TL_GetMaxSendMessagePayloadSize(C1222TL* p)
{
	Unsigned16 maxPayload = C1222DL_GetMaxNegotiatedTxPayload((C1222DL*)(p->pDatalink));
	
	maxPayload -= MAX_TRANSPORT_OVERHEAD;
	
    return maxPayload;
}




Boolean C1222TL_SendACSEMessage(C1222TL* p, Unsigned8* message, Unsigned16 length, 
            Unsigned8* destinationNativeAddress, Unsigned16 addressLength)
{
    Unsigned8 messageBuffer[2];
    Boolean ok;
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    
    C1222TL_LogEvent(p, C1222EVENT_TL_SEND_ACSE_MSG);
    
    p->counters.acseMessagesSent++;
    
    NoteMessageSendStarted(p);
    
    p->status.sendInProgress = TRUE;
        
    ok = C1222DL_StartMessageBuild(pDatalink, (Unsigned16)(2+addressLength+length));
    
    messageBuffer[0] = 0x74;
    messageBuffer[1] = addressLength;
    
    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, messageBuffer, 2);
    
    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, destinationNativeAddress, addressLength);
        
    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, message, length);
        
    if ( ok )
        ok = C1222DL_SendBuiltMessage(pDatalink);
        
    p->status.sendInProgress = FALSE;
        
    // should do something if the message fails
    
    if ( ok )
    {
        NoteRequest(p, messageBuffer[0]);
        
        C1222TL_NoteOptionalCommAllowed(p);  
    }
    else
    {
        C1222TL_LogEvent(p, C1222EVENT_TL_SEND_FAILED);
        
        // the caller will use the result of this call to act, so does not need a
        // separate notification of failure
        //C1222AL_TL_NoteLinkFailure((C1222AL*)(p->pApplication));
    }
    
    return ok;           
}


#ifdef C1222_WANT_SHORT_MESSAGES

Boolean C1222TL_SendShortMessage(C1222TL* p, Unsigned8* message, Unsigned16 length, 
            Unsigned8* destinationNativeAddress, Unsigned16 addressLength)
{
    Unsigned8 messageBuffer[2];
    Boolean ok;
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    
    C1222TL_LogEvent(p, C1222EVENT_TL_SEND_SHORT_MSG);
    
    p->counters.acseMessagesSent++;
    
    NoteMessageSendStarted(p);
    
    p->status.sendInProgress = TRUE;
        
    ok = C1222DL_StartMessageBuild(pDatalink, (Unsigned16)(2+addressLength+length));
    
    messageBuffer[0] = 0x77;
    messageBuffer[1] = addressLength;
    
    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, messageBuffer, 2);
    
    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, destinationNativeAddress, addressLength);
        
    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, message, length);
        
    if ( ok )
        ok = C1222DL_SendBuiltMessage(pDatalink);
        
    p->status.sendInProgress = FALSE;
        
    // should do something if the message fails
    
    if ( ok )
    {
        NoteRequest(p, messageBuffer[0]);
    }
    else
    {
        C1222TL_LogEvent(p, C1222EVENT_TL_SEND_SHORT_FAILED);
        
        C1222AL_TL_NoteLinkFailure((C1222AL*)(p->pApplication));
    }
    
    return ok;           
}

#endif // c1222_want_short_messages






Boolean C1222TL_IsHappy(C1222TL* p)
{
    if ( p->status.negotiateComplete && p->status.haveConfiguration &&
         (p->status.state == TLS_NORMAL) && !(p->status.isResetNeeded) )
    {
        if ( (C1222Misc_GetFreeRunningTimeInMS()-p->status.lastResetTime) > 10000 )
            return TRUE;
    }
        
    return FALSE;
}


Boolean C1222TL_IsReady(C1222TL* p)
{
    Boolean ready;
    
    ready = (Boolean)(p->status.negotiateComplete && p->status.haveConfiguration);
    
    // comm module present is set when we get the getconfig request
    
    if ( p->status.isLocalPort )
    {
        ready = (Boolean)(p->status.negotiateComplete);
        
    }
    else if ( !p->status.isCommModule )
    {
        ready = (Boolean)(ready && p->status.commModulePresent && !p->status.waitingForConfigRequest);
    }
        
    return ready;
}


Unsigned8 C1222TL_GetInterfaceNumber(C1222TL* p)
{
    return p->status.interfaceNumber;
}

            
            
// these routines send the indicated service.  They do not wait for responses.
// the responses will be handled in _PacketReceived  



static Boolean SendRequest(C1222TL* p, Unsigned8* messageBuffer, Unsigned16 length, 
                Unsigned8 delayedRequestBit, C1222EventId eventId )
{
    Boolean ok;
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    
    C1222TL_LogEvent(p, eventId);
    
    p->status.delayedRequests &= ~delayedRequestBit;
    
    NoteMessageSendStarted(p);
        
    p->status.sendInProgress = TRUE;

    ok = C1222DL_SendOneTryMessage(pDatalink, messageBuffer, length); 
    
    p->status.sendInProgress = FALSE;

    return ok;    
}


static void HandleSimpleRequestFailure( C1222TL* p, Unsigned8 delayedRequestBit, Boolean shortDelay )
{
    p->status.delayedRequests |= delayedRequestBit;
        
    if ( shortDelay )
        SetShortDelayEnd(p);
    else
        SetDelayEnd(p);
        
    C1222TL_LogEvent(p, C1222EVENT_TL_SEND_FAILED);
}              


#ifdef C1222_INCLUDE_COMMMODULE
static void SendSimpleRequest(C1222TL* p, Unsigned8* messageBuffer, Unsigned16 length, 
                Unsigned8 delayedRequestBit, Boolean shortDelay, C1222EventId eventId )
{   
    if ( SendRequest(p, messageBuffer, length, delayedRequestBit, eventId) )
    {
        NoteRequest(p, messageBuffer[0]);
    }
    else
    {
        HandleSimpleRequestFailure(p, delayedRequestBit, shortDelay);
    }       
}
#endif // C1222_INCLUDE_COMMMODULE




// here is the negotiate info from the standard
//          
//<tls-negotiate>	::=	<baud-rate-selector><rec-packet-size><rec-nbr-packet>
//		<baud-rates> <rec-nbr-of-channels>
//
//<baud-rate-selector>	::=	60H |	{No <baud rate> included in request. Stay at default baud rate}
//		61H |	{1 <baud rate> included in request}
//		62H |	{2 <baud rate>s included in request}
//		63H |	{3 <baud rate>s included in request}
//		64H |	{4 <baud rate>s included in request}
//		65H |	{5 <baud rate>s included in request}
//		66H |	{6 <baud rate>s included in request}
//		67H |	{7 <baud rate>s included in request}
//		68H |	{8 <baud rate>s included in request}
//		69H |	{9 <baud rate>s included in request}
//		6AH |	{10 <baud rate>s included in request}
//		6BH |	{11 <baud rate>s included in request}
//
//<rec-packet-size>	::=	<word16>	{Maximum packet size in bytes that can be received by the C12.22 Communication Module.  This value shall not be greater than 8192 bytes.}
//
//<rec-nbr-packet>	::=	<byte>	{Maximum number of packets the C12.22 Communication Module is able to reassemble into an upper layer data structure at one time.}
//
//<baud-rates>	::=	<baud-rate>*	{List of baud rates supported by the C12.22 Communication Module. If the C12.22 Device cannot select one of these baud rates, the original baud rate is echoed in the response.}
//
//<baud-rate>	::=	00H |	{Reserved}
//		01H |	{300 baud}
//		02H |	{600 baud}
//		03H |	{1200 baud}
//		04H |	{2400 baud}
//		05H |	{4800 baud}
//		06H |	{9600 baud}
//		07H |	{14400 baud}
//		08H |	{19200 baud}
//		09H |	{28800 baud}
//		0AH |	{57600 baud}
//		0BH |	{38400 baud}
//		0CH |	{115200 baud}
//		0DH |	[128000 baud}
//		0EH	{256000 baud}
//			{0FH - FFH reserved}
//
//<rec-nbr-of-channels> ::= <byte>	{The maximum number of concurrent channels supported by the C12.22 Communication Module.}
//

C1222CONST Unsigned32 C1222BaudList[] = { 0, 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 
                                     57600, 38400, 115200, 128000, 256000 };

#ifdef C1222_INCLUDE_COMMMODULE
void C1222TL_SendNegotiateRequest(C1222TL* p)  // params are all properties of datalink layer
{
    //Unsigned8 messageBuffer[30]; // size just a guess
    Unsigned16 packetSize;
    Unsigned8  numberOfPackets;
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    Unsigned8 numberOfBauds = 0;
    Unsigned8 ii;
    
    Unsigned8 *messageBuffer = p->tempBuffer;
    
    
    p->counters.negotiateAttempted++;
        
    SelectPacketLimits(p, &packetSize, &numberOfPackets);
    
    messageBuffer[0] = 0x60;
    messageBuffer[1] = (Unsigned8)(packetSize>>8);
    messageBuffer[2] = (Unsigned8)(packetSize & 0xFF);
    messageBuffer[3] = numberOfPackets;

    // add supported baud rates to the request (we don't support element 0 (0 baud))

    for ( ii=1; ii<ELEMENTS(C1222BaudList); ii++ )
    {
        if ( C1222DL_IsBaudRateSupported(pDatalink, C1222BaudList[ii] ) )
        {
            messageBuffer[4+numberOfBauds] = ii;
            numberOfBauds++;
            
            if ( numberOfBauds == 11 )
                break;
        }
    }

    messageBuffer[0] += numberOfBauds;
    messageBuffer[4+numberOfBauds] = 1; // only 1 channel supported
    
    SendSimpleRequest(p, messageBuffer, (Unsigned16)(5+numberOfBauds), 
                C1222TL_DELAYED_NEGOTIATE, TRUE, C1222EVENT_TL_SEND_NEGOTIATE_REQUEST );    
}
#endif // C1222_INCLUDE_COMMMODULE





#ifdef C1222_INCLUDE_DEVICE
void C1222TL_SendLinkControlRequest(C1222TL* p, Unsigned8 linkControlByte)
{
    Unsigned8 messageBuffer[2];
    
    messageBuffer[0] = 0x73;
    messageBuffer[1] = linkControlByte;
        
    if ( SendRequest(p, messageBuffer, 2, C1222TL_DELAYED_LINKCONTROL, C1222EVENT_TL_SEND_LINK_CONTROL_REQUEST) )
    {
        NoteRequest(p, messageBuffer[0]); 
    }
    else
    {
        HandleSimpleRequestFailure(p, C1222TL_DELAYED_LINKCONTROL, FALSE);
        p->status.delayedLinkControlType = linkControlByte;
    }
                
    p->status.pendingRequestU8Parameter = linkControlByte;
}
#endif


#ifdef C1222_INCLUDE_COMMMODULE
void C1222TL_SendGetConfigRequest(C1222TL* p)
{
    Unsigned8 messageBuffer[1];
        
    messageBuffer[0] = 0x72;
        
    SendSimpleRequest(p, messageBuffer, 1, 
                C1222TL_DELAYED_GETCONFIG, TRUE, C1222EVENT_TL_SEND_GET_CONFIG_REQUEST );       
}
#endif


#ifdef C1222_INCLUDE_DEVICE
void C1222TL_SendGetStatusRequest(C1222TL* p, Unsigned8 statusControlByte)
{
    Unsigned8 messageBuffer[2];
    Boolean ok = FALSE;
    
    if ( ! C1222TL_IsOptionalCommInhibited(p) )
    {         
        messageBuffer[0] = 0x75;
        messageBuffer[1] = statusControlByte;
        
        ok = SendRequest(p, messageBuffer, 2, C1222TL_DELAYED_GETSTATUS, C1222EVENT_TL_SEND_GET_STATUS_REQUEST);        
    }
    
    if ( ok )
        NoteRequest(p, messageBuffer[0]); 
    else
    {
        HandleSimpleRequestFailure( p, C1222TL_DELAYED_GETSTATUS, FALSE );
        p->status.delayedStatusRequestType = statusControlByte;
    }       
        
        
    p->status.pendingRequestU8Parameter = statusControlByte;
}
#endif


#ifdef C1222_REGISTRATION_HANDLED_BY_COMMMODULE
#ifdef C1222_INCLUDE_DEVICE
void C1222TL_SendGetRegistrationStatusRequest(C1222TL* p)
{
    Unsigned8 messageBuffer[1];
    Boolean ok = FALSE;
    
    if ( ! C1222TL_IsOptionalCommInhibited(p) )
    {        
        messageBuffer[0] = 0x76;        
        
        ok = SendRequest(p, messageBuffer, 1, C1222TL_DELAYED_GETREGSTATUS, C1222EVENT_TL_SENT_GET_REG_STATUS_REQUEST);
    }
    
    if ( ok )
        NoteRequest(p, messageBuffer[0]); 
    else
    {
        HandleSimpleRequestFailure( p, C1222TL_DELAYED_GETREGSTATUS, FALSE );
    }                  
}
#endif  // C1222_INCLUDE_DEVICE
#endif  // C1222_REGISTRATION_HANDLED_BY_COMMMODULE




// *****************************************************************************
// services for datalink layer
// *****************************************************************************



Boolean C1222TL_DLIsKeepAliveNeeded(C1222TL* p)
{
    return (Boolean)((p->status.state == TLS_NORMAL) ? TRUE : FALSE);
}





void C1222TL_DLNotePacketReceived(C1222TL* p, Unsigned8* packetData, Unsigned16 length, Unsigned16 offset)
{
    if ( p->status.receivedMessageIsComplete )
    {
        p->status.receivedMessageIsComplete = FALSE;
        p->counters.completeMessageDiscarded++;
        p->rxMessageLength = 0;
    }
    
    if ( offset == 0 )
    {
        if ( p->status.receiveInProgress )
            p->counters.incompleteMessageDiscardedCount++;
            
        p->rxMessageLength = 0;
        p->status.receiveInProgress = TRUE;
    }
    else if ( offset != p->rxMessageLength )
    {
        // packet received out of order
        p->rxMessageLength = 0;
        p->counters.incompleteMessageDiscardedCount++;
        return;
    }
    
    if ( (offset+length) <= MAX_TRANSPORT_MESSAGE_SIZE )
    {
        memcpy(&p->rxMessageBuffer[offset], packetData, length);
        
        p->rxMessageLength += length;
    }
    else
    {
        p->counters.tooLongMessageDiscarded++;
        p->rxMessageLength = 0;
        p->status.receiveInProgress = FALSE;
    }
}




void C1222TL_DLNoteLinkFailure(C1222TL* p)
{
    // comm module should try to reestablish the link
    
    C1222TL_LogEvent(p, C1222EVENT_TL_DL_NOTE_LINK_FAILURE);
    
    p->status.state = TLS_INIT;
    p->status.isResetNeeded = TRUE;
    
    p->counters.linkResetByDatalink++;
    
    // the link failure will be notified to the application layer when the reset is handled in _Run
}



void C1222TL_DLNoteMessageRxComplete(C1222TL* p)
{
    p->status.receivedMessageIsComplete = TRUE;
    p->status.receiveInProgress = FALSE;
}



static void HandleReceivedTransportMessage(C1222TL* p)
{
    // the message is in, process it
    
    Unsigned8 command;
    
    // it is possible to receive a complete message while handling another
    // message
    
    while ( p->status.receivedMessageIsComplete )
    {
        p->status.receivedMessageIsComplete = FALSE;
    
        p->counters.messagesReceived++;
    
        if ( p->rxMessageLength > 0 )
        {
            command = p->rxMessageBuffer[0];
        
            if ( command <= 0x1f )   // this is a response
                ProcessResponseMessage(p, command);
            else
                ProcessCommandMessage(p, command);
        }
    }
}


void C1222TL_AddToStatistic(C1222TL* p, Unsigned16 id, Unsigned32 count)
{
    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    
    C1222AL_AddToStatistic(pApplication, id, count);
}



void C1222TL_IncrementStatistic(C1222TL* p, Unsigned16 id)
{
    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    
    C1222AL_IncrementStatistic(pApplication, id);
}


static void NoteMessageSendStarted(C1222TL* p)
{
    p->status.lastMessageSendTime = C1222Misc_GetFreeRunningTimeInMS(); 
}


static void NoteMessageSendComplete(C1222TL* p)
{
    Unsigned32 elapsedTime = p->status.lastRequestTime - p->status.lastMessageSendTime;
    
    p->status.totalMessageSendTime += elapsedTime;
    
    p->counters.messagesSent++;   
    
    if ( p->counters.messagesSent > 0 )
    {
        p->status.avgMessageSendTime = (Unsigned16)(p->status.totalMessageSendTime/p->counters.messagesSent);
    }
    else
        p->status.avgMessageSendTime = 0;
           
    if ( elapsedTime > (Unsigned32)(p->status.maxMessageSendTime) )
        p->status.maxMessageSendTime = (Unsigned16)elapsedTime;
            
}



static void NoteRequest(C1222TL* p, Unsigned8 command)
{
    p->status.pendingRequest = command;
    p->status.lastRequestTime = C1222Misc_GetFreeRunningTimeInMS();
    
    NoteMessageSendComplete(p);
}




static void ProcessResponseMessage(C1222TL* p, Unsigned8 response)
{
    Unsigned8 pendingRequest = p->status.pendingRequest;
    
    p->status.pendingRequest = 0;

    if ( response != 0 )
    {
        C1222TL_LogEventAndULongs(p, C1222EVENT_TL_BAD_RESULT_CODE, response, pendingRequest, 0, 0);

        // might want to increment some statistic

        #ifndef C1222_COMM_MODULE
        if ( pendingRequest == 0x73 )
            C1222AL_TL_NotifyFailedLinkControl((C1222AL*)(p->pApplication), response);
        #endif
        
        if ( (pendingRequest == 0x74) && ((response == C1222RESPONSE_NETR) || (response == C1222RESPONSE_NETT)) )
        {
            // wait a bit before returning an error to the application layer
            p->status.sendMessageResponseIsPending = TRUE;
            p->status.pendingSendMessageResponse = response;  // in case we want to differentiate later
            p->status.pendingSendResponseReceived = C1222Misc_GetFreeRunningTimeInMS();
            p->counters.acseMessagesResponseDelayed++;
        }
        else if ( pendingRequest == 0x74 )
        {
            C1222AL_TL_NoteFailedSendMessage((C1222AL*)(p->pApplication));
        }
        else
            C1222AL_TL_NoteLinkFailure((C1222AL*)(p->pApplication));


        return;
    }


    C1222TL_LogEventAndBytes(p, C1222EVENT_TL_PROCESS_OK_RESPONSE_FOR_PENDING_REQUEST, 1, &pendingRequest);

    switch(pendingRequest)
    {
#ifdef C1222_INCLUDE_COMMMODULE        
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
        case 0x68:
        case 0x69:
        case 0x6A:
        case 0x6B:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_RESPONSE_TO_NEGOTIATE);
            ProcessReceivedNegotiateResponse(p);
            break;
#endif // C1222_INCLUDE_COMMMODULE            
         
#ifdef C1222_INCLUDE_COMMMODULE            
        case 0x72:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_RESPONSE_TO_GETCONFIG);
            ProcessReceivedGetConfigResponse(p);
            break;
#endif // C1222_INCLUDE_COMMMODULE            
         
#ifdef C1222_INCLUDE_DEVICE            
        case 0x73:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_RESPONSE_TO_LINKCONTROL);
            ProcessReceivedLinkControlResponse(p);
            break;
#endif            

        case 0x74:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_RESPONSE_TO_SEND_MESSAGE);
            ProcessReceivedSendMessageResponse(p);
            break;
      
#ifdef C1222_INCLUDE_DEVICE            
        case 0x75:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_RESPONSE_TO_GET_STATUS);
            ProcessReceivedGetStatusResponse(p);
            break;
#endif            
            
#ifdef C1222_REGISTRATION_HANDLED_BY_COMMMODULE
#ifdef C1222_INCLUDE_DEVICE            
        case 0x76:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_RESPONSE_TO_GET_REG_STATUS);
            ProcessReceivedGetRegistrationStatusResponse(p);
            break;
#endif // C1222_INCLUDE_DEVICE
#endif // C1222_REGISTRATION_HANDLED_BY_COMMMODULE
            
            
        case 0x77:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_RESPONSE_TO_SEND_SHORT);
            ProcessReceivedSendShortMessageResponse(p);
            break;
            
        default:
            // oh-oh
            // if there was no request pending, ...
            // if the request was undefined, ...
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_RESPONSE_TO_UNKNOWN);
            break;
    }
}



static void ProcessCommandMessage(C1222TL* p, Unsigned8 command)
{
    //char message[50];
    
    C1222TL_LogEventAndBytes(p, C1222EVENT_TL_PROCESS_COMMAND, 1, &command); 
             
    switch(command)
    {
#ifdef C1222_INCLUDE_DEVICE        
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
        case 0x68:
        case 0x69:
        case 0x6A:
        case 0x6B:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_CMD_NEGOTIATE);
            ProcessReceivedNegotiateRequest(p);
            break;
#endif            
      
#ifdef C1222_INCLUDE_DEVICE            
        case 0x72:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_CMD_GET_CONFIG);
            ProcessReceivedGetConfigRequest(p);
            break;
#endif            
            
#ifdef C1222_INCLUDE_COMMMODULE            
        case 0x73:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_CMD_LINK_CONTROL);
            ProcessReceivedLinkControlRequest(p);
            break;
#endif // C1222_INCLUDE_COMMMODULE           

        case 0x74:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_CMD_SEND_MSG);
            ProcessReceivedSendMessageRequest(p);
            break;
            
#ifdef C1222_INCLUDE_COMMMODULE            
        case 0x75:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_CMD_GET_STATUS);
            ProcessReceivedGetStatusRequest(p);
            break;
#endif // C1222_INCLUDE_COMMMODULE            
           
#ifdef C1222_INCLUDE_COMMMODULE            
        case 0x76:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_CMD_GET_LINK_STATUS);
            ProcessReceivedGetRegistrationStatusRequest(p);
            break;
#endif            
            
        case 0x77:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_CMD_SEND_SHORT);
            ProcessReceivedSendShortMessageRequest(p);
            break;
            
        default:
            C1222TL_LogEvent(p, C1222EVENT_TL_RCV_CMD_UNDEFINED);
            ProcessReceivedUndefinedRequest(p);
            break;
    }
} 





// returns selector and sets baudrate to corresponding value
#ifdef C1222_INCLUDE_DEVICE
static Unsigned8 SelectBaudRate(C1222TL* p, Unsigned8 numberOfBauds, Unsigned8* baudSelectorList, 
                        Unsigned32* pBaudRate)
{
    Unsigned8 selector;
    Unsigned32 baud;
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    
    Unsigned8 ii;
    
    Unsigned32 negotiatedBaud = 9600;

    // select the highest baud rate suggested that this datalink also supports
    // but no lower than the default baudrate of 9600.
    
    for ( ii=0; ii<numberOfBauds; ii++ )
    {
        selector = baudSelectorList[ii];
        
        if ( selector < ELEMENTS(C1222BaudList) )
        {
            baud = C1222BaudList[selector];
            
            if ( baud > negotiatedBaud )
            {
                if ( C1222DL_IsBaudRateSupported(pDatalink, baud) )
                    negotiatedBaud = baud;
            }
        }
    }
    
    // find the selector corresponding to the negotiated baud rate
    
    for ( ii=0; ii<ELEMENTS(C1222BaudList); ii++ )
    {
        if ( negotiatedBaud == C1222BaudList[ii] )
        {
            selector = ii;
            break;
        }
    }
    
    *pBaudRate = negotiatedBaud;
    
    return selector;
    
}
#endif



#define MINIMUM_PACKET_SIZE  64

static void SelectPacketLimits(C1222TL* p, Unsigned16* pMaxPacketSize, Unsigned8* pMaxNumberOfPackets)
{
    // this routine needs to protect the size of the transport receive buffer
    
#ifdef C1222_WANT_CODE_REDUCTION
    (void)p;
    *pMaxPacketSize = 87;
    *pMaxNumberOfPackets = 2;
#else    
    Unsigned16 packetSize;
    Unsigned8  numberOfPackets;
    Unsigned8 packetOverhead;
    Unsigned32 maxPayload;
    Unsigned16 maxNumberOfPackets;
    Unsigned16 maxPacketSize;
    Unsigned8 bestNumberOfPackets;
    Unsigned16 waste;
    Unsigned16 minimumWaste;
    Unsigned16 ii;
    
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    
    packetSize = C1222DL_MaxSupportedPacketSize(pDatalink);
    numberOfPackets = C1222DL_MaxSupportedNumberOfPackets(pDatalink);
    packetOverhead = C1222DL_PacketOverheadSize(pDatalink);

    if ( packetSize < MINIMUM_PACKET_SIZE )
    	packetSize = MINIMUM_PACKET_SIZE;

	if ( numberOfPackets < 1 )
    	numberOfPackets = 1;

    // this is the method of notifying the peer transport layer of how big a message
    // we can handle.  So the combination of packetSize and numberOfPackets must
    // fit within the MAX_TRANSPORT_MESSAGE_SIZE.
    
    maxPayload = ((Unsigned32)(packetSize-packetOverhead))*numberOfPackets;
    
    if ( maxPayload > MAX_TRANSPORT_MESSAGE_SIZE )
    {
        // need to adjust one or both of the parameters
        
        if ( (packetSize-packetOverhead) >= MAX_TRANSPORT_MESSAGE_SIZE )
        {
            // this is easiest - just make it 1 packet
            packetSize = (Unsigned16)(MAX_TRANSPORT_MESSAGE_SIZE + packetOverhead);
            numberOfPackets = 1;
        }
        else
        {
            // lets pick parameters that maximize the payload with the minimum number of packets
            // but with the maximum payload <= the max transport message
            // and with the packet size and number of packets within the datalink requirements
            
            // number of packets must be between 1 and how ever many are needed by the minimum packet size
            // to cover the message
            
            maxNumberOfPackets = (Unsigned16)((MAX_TRANSPORT_MESSAGE_SIZE) / (MINIMUM_PACKET_SIZE-packetOverhead)+1);
            if ( maxNumberOfPackets > numberOfPackets )
                maxNumberOfPackets = numberOfPackets;
                
            minimumWaste = MAX_TRANSPORT_MESSAGE_SIZE;
            bestNumberOfPackets = 0;
            
            for ( ii=1; ii<=maxNumberOfPackets; ii++ )
            {
                // calculate the maximum packet size that can be used with this number of packets
                // such that the total transport payload will be less than or equal to the max transport 
                // message size
                
                maxPacketSize = (Unsigned16)((MAX_TRANSPORT_MESSAGE_SIZE/ii) + packetOverhead);
                if ( maxPacketSize > packetSize )
                    maxPacketSize = packetSize;
                    
                waste = (Unsigned16)(MAX_TRANSPORT_MESSAGE_SIZE - ii*(maxPacketSize-packetOverhead));
                
                if ( bestNumberOfPackets == 0 || waste < minimumWaste )
                {
                    bestNumberOfPackets = ii;
                    minimumWaste = waste;
                }
            }
                    
            numberOfPackets = bestNumberOfPackets;
            maxPacketSize = (Unsigned16)((MAX_TRANSPORT_MESSAGE_SIZE/numberOfPackets) + packetOverhead);
            if ( maxPacketSize < packetSize )
                packetSize = maxPacketSize;
        }
    }
        
    *pMaxPacketSize = packetSize;
    *pMaxNumberOfPackets = numberOfPackets;
#endif // not  C1222_WANT_CODE_REDUCTION   
}



static void WaitABit(Unsigned32 delay)
{
    C1222PL_WaitMs(delay);
}


Boolean C1222TL_IsOptionalCommInhibited(C1222TL* p)
{
    Unsigned32 now = C1222Misc_GetFreeRunningTimeInMS();
    
    if ( p->status.noOptionalCommPeriodStart == 0 )
        return FALSE;
    
    if ( (now - p->status.noOptionalCommPeriodStart) < 15000L )
        return TRUE;
        
    return FALSE;
}



// we are always willing to negotiate

#ifdef C1222_INCLUDE_DEVICE
static void ProcessReceivedNegotiateRequest(C1222TL* p)
{
    Unsigned8* buffer = p->rxMessageBuffer;
    Unsigned8 selector;
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    Unsigned8 *responseBuffer = p->tempBuffer; // need 7 bytes
    
    Unsigned8 numberOfBauds = (Unsigned8)(buffer[0] - 0x60);
    Unsigned16 txPacketSize   = (Unsigned16)((buffer[1] << 8) | buffer[2]);
    Unsigned8 txNumberOfPackets = buffer[3];
    Boolean ok;
    
    Unsigned32 negotiatedBaud = 9600;

    // I don't care how many channels he can handle - I will only use 1
    //Unsigned8 numberOfChannels = buffer[4+numberOfBauds];

    Unsigned16 rxPacketSize;
    Unsigned8 rxNumberOfPackets;
    
    p->counters.negotiateAttempted++;
    
    
    // disable arbitrary use of the link while rflan is getting configured etc
    
    p->status.noOptionalCommPeriodStart = C1222Misc_GetFreeRunningTimeInMS();

    
    // select the highest baud rate suggested that this datalink also supports
    // but no lower than the default baudrate of 9600.
    
    selector = SelectBaudRate(p, numberOfBauds, &buffer[4], &negotiatedBaud);

    // select max packets and max packet size for reception    
    
    SelectPacketLimits(p, &rxPacketSize, &rxNumberOfPackets);
    
    // construct the response
    
    responseBuffer[0] = 0;   // ok
    responseBuffer[1] = (Unsigned8)(rxPacketSize >> 8);
    responseBuffer[2] = (Unsigned8)(rxPacketSize & 0xFF);
    responseBuffer[3] = rxNumberOfPackets;
    responseBuffer[4] = selector; // baud rate selector
    responseBuffer[5] = 1;    // we only support 1 channel
    responseBuffer[6] = p->status.interfaceNumber;
    
    // send the response and change settings
    
    C1222TL_LogEvent(p, C1222EVENT_TL_SEND_NEGOTIATE_RESPONSE);
    
    p->status.sendInProgress = TRUE;
    
    ok = C1222DL_SendOneTryMessage( pDatalink, responseBuffer, 7 );
    
    p->status.sendInProgress = FALSE;
    
    if ( ok )
    {
        if ( negotiatedBaud != C1222DL_GetCurrentBaud(pDatalink) )
            WaitABit(1000);
        
        if ( txPacketSize > C1222DL_MaxSupportedPacketSize(pDatalink) )
        {
            // pretend this peer is not insane
            
            txPacketSize = rxPacketSize;
            txNumberOfPackets = rxNumberOfPackets;                
        }
        
        C1222DL_SetParameters(pDatalink, negotiatedBaud, txPacketSize, txNumberOfPackets);
        
        C1222TL_LogEvent(p, C1222EVENT_TL_CHANGED_LINK_PARAMETERS);
        
        p->status.negotiateComplete = TRUE;
        p->status.lastNegotiateTime = C1222Misc_GetFreeRunningTimeInMS();
        //p->status.waitingForConfigRequest = TRUE;
        
        //p->commModulePresent = TRUE;
        p->status.state = TLS_NORMAL;
        
        p->counters.negotiateSucceeded++;
    }
}
#endif


#ifdef C1222_INCLUDE_COMMMODULE
static void ProcessReceivedNegotiateResponse(C1222TL* p)
{
    Unsigned8* buffer = p->rxMessageBuffer;
    Unsigned8 selector = buffer[4];
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    
    Unsigned16 txPacketSize   = (Unsigned16)((buffer[1] << 8) | buffer[2]);
    Unsigned8 txNumberOfPackets = buffer[3];
    Unsigned8 interfaceNumber = buffer[6];
    
    Unsigned32 negotiatedBaud = 9600;
    
    if ( selector < ELEMENTS(C1222BaudList) )
        negotiatedBaud = C1222BaudList[selector];
        
    p->counters.negotiateSucceeded++;

    if ( negotiatedBaud != C1222DL_GetCurrentBaud(pDatalink) )
        WaitABit(1000);
    else
        WaitABit(50);  // to make sure the ack goes out before reconfigure of the port
    
    if ( txPacketSize > C1222DL_MaxSupportedPacketSize(pDatalink) )
    {
        SelectPacketLimits(p, &txPacketSize, &txNumberOfPackets);
    }
    
    C1222DL_SetParameters(pDatalink, negotiatedBaud, txPacketSize, txNumberOfPackets);
        
    p->status.negotiateComplete = TRUE;
    p->status.interfaceNumber = interfaceNumber;
    
    if ( p->status.isCommModule )  // should be
    {
        p->status.lastNegotiateTime = C1222Misc_GetFreeRunningTimeInMS();
        
        if ( p->status.haveConfiguration )
        {
            p->status.state = TLS_NORMAL;
        }
        else
        {
            // this speeds up the rf lan getting its config
            // before I had it go to disconnected state which adds 
            // a 1 second delay before requesting config
            
            p->status.state = TLS_NEEDCONFIG; //TLS_DISCONNECTED; //TLS_NEEDCONFIG;
        }
    }
}
#endif // C1222_INCLUDE_COMMMODULE



static Boolean AddLengthAndItemToMessage(C1222DL* pDatalink, Unsigned8 length, Unsigned8* item)
{
    Boolean ok;
    
    ok = C1222DL_AddMessagePart(pDatalink, &length, 1);
        
    if ( ok && (length > 0) )
        ok = C1222DL_AddMessagePart(pDatalink, item, length);
        
    return ok;
}


//typedef struct C1222TransportConfigTag
//{
//    Boolean passthroughMode;
//    Unsigned8 linkControlByte;
//    Unsigned8 nodeType;
//    Unsigned8 deviceClass[C1222_DEVICE_CLASS_LENGTH];
//    Unsigned8 esn[C1222_ESN_LENGTH];
//    Unsigned8 nativeAddressLength;
//    Unsigned8 nativeAddress[C1222_NATIVE_ADDRESS_LENGTH];
//    Unsigned8 broadcastAddressLength;
//    Unsigned8 broadcastAddress[C1222_NATIVE_ADDRESS_LENGTH];
//    Unsigned8 relayAddressLength;
//    Unsigned8 relayNativeAddress[C1222_NATIVE_ADDRESS_LENGTH];
//    Unsigned8 nodeApTitle[C1222_APTITLE_LENGTH];
//    Unsigned8 masterRelayApTitle[C1222_APTITLE_LENGTH];
//    Boolean wantClientResponseControl;
//    Unsigned8 numberOfRetries;
//    Unsigned16 responseTimeout;
//} C1222TransportConfig;

void C1222TL_InhibitOptionalCommunication(C1222TL* p)
{
    p->status.noOptionalCommPeriodStart = C1222Misc_GetFreeRunningTimeInMS();    
}


#ifdef C1222_INCLUDE_DEVICE
static void ProcessReceivedGetConfigRequest(C1222TL* p)
{
    Unsigned8 responseBuffer[4];
    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    C1222TransportConfig* pConfig;
    Unsigned16 messageTotalLength;
    Boolean ok;
    Unsigned8 esnLength;
    Unsigned8 aptitleLength;
    Unsigned8 mrAptitleLength;
    //Unsigned8 aptitle[C1222_APTITLE_LENGTH];
    //Unsigned8 mrAptitle[C1222_APTITLE_LENGTH];
    static Unsigned8 nullAptitle[] = { 0x0d, 0x00 };
    static Unsigned8 nullESN[] = { 0x0d, 0x00 };
    Unsigned8* esn;
    C1222ApTitle myApTitle;
    Unsigned8 myRelativeAptitleBuffer[C1222_APTITLE_LENGTH];
    C1222ApTitle myRelativeAptitle;
    C1222ApTitle* pSelectedApTitle;
    
    Unsigned8* aptitle = p->tempBuffer;
    Unsigned8* mrAptitle = &p->tempBuffer[C1222_APTITLE_LENGTH];  // must have room for an aptitle
    
    // disable arbitrary use of the link while getting configuration
    
    p->status.noOptionalCommPeriodStart = C1222Misc_GetFreeRunningTimeInMS();

    pConfig = C1222AL_GetConfiguration(pApplication);
    
    // for the rflan comm module I am going to replace the aptitle in the
    // config with the registered aptitle so the rflan can know it.
    
    C1222AL_ConstructCallingApTitle(pApplication, &myApTitle);
    
    // convert my aptitle to relative for use in the comm module
    
    C1222ApTitle_Construct(&myRelativeAptitle, myRelativeAptitleBuffer, C1222_APTITLE_LENGTH);
    if ( p->status.forceRelativeApTitleInConfig && C1222ApTitle_MakeRelativeFrom(&myRelativeAptitle, &myApTitle ) )
    {
        pSelectedApTitle = &myRelativeAptitle;
    }
    else
        pSelectedApTitle = &myApTitle;  // hope for the best
    
    // get the lengths of the esn, aptitle, and master relay aptitle
    
    esnLength = getUIDLength(pConfig->esn);
    aptitleLength = getUIDLength(pSelectedApTitle->buffer);
    mrAptitleLength = getUIDLength(pConfig->masterRelayApTitle);
    
    esn = pConfig->esn;
    if ( esnLength == 0 )
    {
        esn = nullESN;
        esnLength = getUIDLength(esn);
	}
    
    if ( aptitleLength > 0 )
    {
        memcpy(aptitle, pSelectedApTitle->buffer, C1222_APTITLE_LENGTH);
    }
    else
    {
        memcpy(aptitle, nullAptitle, sizeof(nullAptitle));
        aptitleLength = sizeof(nullAptitle);
    }
    
    if ( mrAptitleLength > 0 )
    {
        memcpy(mrAptitle, pConfig->masterRelayApTitle, C1222_APTITLE_LENGTH);
    }
    else
    {
        memcpy(mrAptitle, nullAptitle, sizeof(nullAptitle));
        mrAptitleLength = sizeof(nullAptitle);
    }
    
    // get total message length
    
    messageTotalLength = (Unsigned16)(4+C1222_DEVICE_CLASS_LENGTH+esnLength+
                            1+pConfig->nativeAddressLength+
                            1+pConfig->broadcastAddressLength+
                            1+pConfig->relayAddressLength+
                            aptitleLength+mrAptitleLength);
                            
    if ( pConfig->wantClientResponseControl )
        messageTotalLength = (Unsigned16)(messageTotalLength + 3);                            
    
    
    responseBuffer[0] = 0; // ok
    responseBuffer[1] = 0; // control
    if ( pConfig->passthroughMode )
        responseBuffer[1] |= 0x01;
    responseBuffer[2] = pConfig->linkControlByte;
    responseBuffer[3] = pConfig->nodeType;
    
    p->status.sendInProgress = TRUE;
    
    ok = C1222DL_StartMessageBuild(pDatalink, messageTotalLength);
    
    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, responseBuffer, 4);
    
    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, pConfig->deviceClass, C1222_DEVICE_CLASS_LENGTH);
    
    if ( ok && (esnLength > 0) )
        ok = C1222DL_AddMessagePart(pDatalink, esn, esnLength);
        
    if ( ok )
        ok = AddLengthAndItemToMessage(pDatalink, pConfig->nativeAddressLength, pConfig->nativeAddress);
        
    if ( ok )
        ok = AddLengthAndItemToMessage(pDatalink, pConfig->broadcastAddressLength, pConfig->broadcastAddress);
        
    if ( ok )
        ok = AddLengthAndItemToMessage(pDatalink, pConfig->relayAddressLength, pConfig->relayNativeAddress);
        
    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, aptitle, aptitleLength);

    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, mrAptitle, mrAptitleLength);

    if ( pConfig->wantClientResponseControl )
    {
        responseBuffer[0] = pConfig->numberOfRetries;
        responseBuffer[1] = (Unsigned8)(pConfig->responseTimeout >> 8);
        responseBuffer[2] = (Unsigned8)(pConfig->responseTimeout & 0xFF);
    
        if ( ok )
            ok = C1222DL_AddMessagePart(pDatalink, responseBuffer, 3);
    }
        
    if ( ok )
        ok = C1222DL_SendBuiltMessage(pDatalink);
        
    p->status.sendInProgress = FALSE;
        
    if ( ok )
    {
        p->status.commModulePresent = TRUE;
        p->status.waitingForConfigRequest = FALSE;
        C1222AL_TLNoteConfigurationSent(pApplication);
        p->status.lastConfigSent = C1222Misc_GetFreeRunningTimeInMS();

    }
        
    // should do something if the message fails
    
    if ( !ok )
        C1222TL_LogEvent(p, C1222EVENT_TL_SEND_GET_CONFIG_RESPONSE_FAILED);
    
    (void)ok;
}
#endif



static Unsigned8 getUIDLength(Unsigned8* uid)
{
    return C1222Misc_GetUIDLength(uid);
}


#ifdef C1222_INCLUDE_COMMMODULE
  #define NEED_GETUIDITEM 1
#else
  #ifdef C1222_REGISTRATION_HANDLED_BY_COMMMODULE
    #define NEED_GETUIDITEM 2
  #endif
#endif

// what should it do if the length is more than maxLength ?
#ifdef NEED_GETUIDITEM 
static Unsigned8* getUIDItem(Unsigned8* buffer, Unsigned8* item, Unsigned16 maxLength)
{
    Unsigned8 length = getUIDLength(buffer);
    
    memset(item, 0, maxLength);
    
    if ( length > maxLength )
        length = maxLength;
        
    memcpy(item, buffer, length);
    
    return &buffer[length];
}
#endif // NEED_GETUIDITEM


// what should it do if the length is more than maxlength?

static Unsigned8* getLengthAndItem(Unsigned8* buffer, Unsigned8* pLength, Unsigned8* item, Unsigned8 maxLength)
{
    Unsigned8 length = buffer[0];
    
    memset(item, 0, maxLength);
    
    if ( length > maxLength )
        length = maxLength;
    
    *pLength = length;
    memcpy(item, &buffer[1], length);
    
    return &buffer[length+1];
}


#ifdef C1222_INCLUDE_COMMMODULE
static void ProcessReceivedGetConfigResponse(C1222TL* p)
{
    Unsigned8* buffer = p->rxMessageBuffer;
    C1222TransportConfig* pConfig = (C1222TransportConfig*)p->tempBuffer;
    Unsigned8* pBufferLoc;
    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    
    memset(pConfig, 0, sizeof(C1222TransportConfig));
    
    pConfig->passthroughMode = (Boolean)((buffer[1] & 0x01) ? TRUE : FALSE);
    pConfig->linkControlByte = buffer[2];
    pConfig->nodeType = buffer[3];
    memcpy( pConfig->deviceClass, &buffer[4], C1222_DEVICE_CLASS_LENGTH);
    
    if ( getUIDLength(&buffer[4+C1222_DEVICE_CLASS_LENGTH]) == 0 )
    {
        C1222TL_LogEvent(p, C1222EVENT_TL_ESN_INVALID);
        p->status.configFailureReason = 1;
        return;
    }
    
    pBufferLoc = getUIDItem(&buffer[4+C1222_DEVICE_CLASS_LENGTH], pConfig->esn, C1222_ESN_LENGTH);
    pBufferLoc = getLengthAndItem(pBufferLoc, &pConfig->nativeAddressLength, pConfig->nativeAddress, C1222_NATIVE_ADDRESS_LENGTH);
    pBufferLoc = getLengthAndItem(pBufferLoc, &pConfig->broadcastAddressLength, pConfig->broadcastAddress, C1222_NATIVE_ADDRESS_LENGTH);
    pBufferLoc = getLengthAndItem(pBufferLoc, &pConfig->relayAddressLength, pConfig->relayNativeAddress, C1222_NATIVE_ADDRESS_LENGTH);
    
    if ( p->rxMessageLength < (pBufferLoc-buffer) )
    {
        C1222TL_LogEventAndULongs(p, C1222EVENT_TL_CONFIG_MESSAGE_TOO_SHORT, p->rxMessageLength, (Unsigned32)(pBufferLoc-buffer), 0, 0);
        
        p->status.configFailureReason = 2;
        return;
    }
    
    if ( p->rxMessageLength > (pBufferLoc-buffer) )
    {
        pBufferLoc = getUIDItem(pBufferLoc, pConfig->nodeApTitle, C1222_APTITLE_LENGTH);
        
        if ( p->rxMessageLength > (pBufferLoc-buffer) ) 
        {   
            pBufferLoc = getUIDItem(pBufferLoc, pConfig->masterRelayApTitle, C1222_APTITLE_LENGTH);
    
            // get retry info if there
    
            if ( p->rxMessageLength >= ((pBufferLoc-buffer)+3) )
            {
                pConfig->numberOfRetries = pBufferLoc[0];
                pConfig->responseTimeout = (Unsigned16)((pBufferLoc[1] << 8) | pBufferLoc[2]);  // I'm not sure this is correct
            }
        }
    }
    
    // if node aptitle is null, esn must be present
    
    if ( (pConfig->nodeType & C1222_NODETYPE_END_DEVICE) != 0 )
    {
        if ( (getUIDLength(pConfig->esn) <= 2) && (getUIDLength(pConfig->nodeApTitle) <= 2) )
        {
            C1222TL_LogEvent(p, C1222EVENT_TL_BOTH_ESN_AND_APTITLE_ARE_MISSING);
            p->status.configFailureReason = 3;
            
            // before I was returning here and considered this an error - it is only an
            // error if the connected peer will be trying to register through this
            // interface.  So now I check in the registration handling in the
            // application layer
        }
    }
    
    // update the configuration in the application layer
    
    C1222AL_TLUpdateConfiguration(pApplication, pConfig);
    
    p->status.haveConfiguration = TRUE;
    p->status.state = TLS_NORMAL;
        
    C1222TL_LogEvent(p, C1222EVENT_TL_CONFIG_UPDATED);
}
#endif // C1222_INCLUDE_COMMMODULE


static Boolean SendMessage( C1222TL* p, Unsigned8* responseBuffer, Unsigned16 length)
{
    C1222DL* pDatalink    = (C1222DL*)(p->pDatalink);
    Boolean ok;
    
    p->status.sendInProgress = TRUE;
    
    ok = C1222DL_SendMessage(pDatalink, responseBuffer, length);
    
    p->status.sendInProgress = FALSE;
    
    return ok;    
}


#ifdef C1222_INCLUDE_COMMMODULE
static void ProcessReceivedLinkControlRequest(C1222TL* p)
{
    Unsigned8* buffer = p->rxMessageBuffer;
    Unsigned8* responseBuffer = p->tempBuffer;

    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    
    Unsigned8* pApTitle;
    Unsigned8 length;

    Unsigned8 interfaceControl = buffer[1];
    
    C1222AL_TLInterfaceControlReceived(pApplication, interfaceControl);
    
    responseBuffer[0] = 0; // ok
    responseBuffer[1] = C1222AL_TLGetInterfaceStatus(pApplication);
    
    p->status.isEnabled = (Boolean)((responseBuffer[1] & 0x01) ? TRUE : FALSE);
    
    pApTitle = C1222AL_TLGetNodeApTitle(pApplication);
    length = getUIDLength(pApTitle);
    
    if ( length > C1222_APTITLE_LENGTH )
        length = C1222_APTITLE_LENGTH;
        
    if ( length > 2 )
    {
        memcpy( &responseBuffer[2], pApTitle, length);
    }
    else
    {
        responseBuffer[2] = 0x0d;
        responseBuffer[3] = 0;
        length = 2;
    }
    
    C1222TL_LogEvent(p, C1222EVENT_TL_SENDING_LINK_CONTROL_RESPONSE);
    
    SendMessage(p, responseBuffer, (Unsigned16)(2+length));
    
    if ( (interfaceControl & 0x03) == C1222TL_INTF_CTRL_RESETINTERFACE )
    {
        p->status.isResetNeeded = TRUE;
    }
    
    else if ( interfaceControl & 0x80 ) // they want us to reload the config
    {
        p->status.haveConfiguration = FALSE;
        if ( p->status.state == TLS_NORMAL )
            p->status.state = TLS_NEEDCONFIG;
    }
    else
    {
#if 0
        switch( interfaceControl & 0x03 )
        {
        case 1:  // enable interface
            if ( p->status.state == TLS_DISABLED )
                p->status.state = TLS_ENABLED;
            break;
        case 2:  // disable interface
            if ( p->status.state == TLS_ENABLED )
                p->status.state = TLS_DISABLED;
            break;
        case 3:  // reset interface
            p->status.state = TLS_INIT;
            break;
        }
#endif
    }
}
#endif // C1222_INCLUDE_COMMMODULE



#ifdef C1222_INCLUDE_DEVICE
static void ProcessReceivedLinkControlResponse(C1222TL* p)
{
    Unsigned8* buffer = p->rxMessageBuffer;
    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    
    Unsigned8* pApTitle = &buffer[2];
    Unsigned8 interfaceStatus = buffer[1];
    
    C1222AL_TLLinkControlResponse(pApplication, interfaceStatus, pApTitle);
    
    C1222TL_LogEvent(p, C1222EVENT_TL_RECEIVED_LINK_CONTROL_RESPONSE);
    
}
#endif


static void ProcessReceivedSendMessageRequest(C1222TL* p)
{
    Unsigned8* buffer = p->rxMessageBuffer;
    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    Unsigned8 *nativeAddress = p->tempBuffer;
    Unsigned8 nativeAddressLength;
    Unsigned8 response = C1222RESPONSE_NETR;
    BufferItem bufferItem;
    
    bufferItem.buffer = getLengthAndItem(&buffer[1], &nativeAddressLength, nativeAddress, C1222_NATIVE_ADDRESS_LENGTH);
    bufferItem.length = (Unsigned16)(p->rxMessageLength - (1+nativeAddressLength));
    
    // ignore send messages before we get configuration
    
    if ( ! p->status.haveConfiguration )
    {
        C1222TL_LogEvent(p, C1222EVENT_TL_FAILINGSENDMESSAGEBEFORECONFIG);
        
        response = C1222RESPONSE_ISSS;
        
        SendMessage(p, &response, 1);
        
        return;
    }
    
    if ( ! p->status.isSendMessageOKResponsePending )
    {
        p->status.isSendMessageOKResponsePending = TRUE;
        C1222TL_LogEvent(p, C1222EVENT_DELAYINGSENDMESSAGERESPONSE);
        
        p->status.lastMessageReceivedTime = C1222Misc_GetFreeRunningTimeInMS();
        
        if ( C1222AL_TLMessageReceived(pApplication, nativeAddressLength, nativeAddress, &bufferItem) != C1222RESPONSE_OK )
            p->status.isSendMessageOKResponsePending = FALSE;
    }
    else
    {
        C1222TL_LogEvent(p, C1222EVENT_TL_SENDING_NETR_RESPONSE_HANDLING_NOT_COMPLETE);
        
        SendMessage(p, &response, 1);
    }
}


static void ProcessReceivedSendMessageResponse(C1222TL* p)
{
    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    
    C1222AL_TLSendMessageResponse(pApplication);
    
    C1222TL_LogEvent(p, C1222EVENT_TL_RECEIVED_SEND_MSG_RESPONSE_AL_NOTIFIED);
}


#ifdef C1222_INCLUDE_COMMMODULE
static void ProcessReceivedGetStatusRequest(C1222TL* p)
{
    Unsigned8* buffer = p->rxMessageBuffer;
    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    Unsigned8 response;
    Boolean ok;
    
    const char* interfaceName;
    Unsigned8 interfaceStatus;
    
    Unsigned16 messageTotalLength;
    
    Unsigned8 statusControl = buffer[1];
    Unsigned16 numberOfStatistics = 0;
    Unsigned8 interfaceNameLength = 0;

    Unsigned16 ii;
    
    C1222Statistic statistic;
    
    Boolean wantInterfaceInfo = (Boolean)((statusControl==0) || (statusControl==1));
    
    messageTotalLength = 1; // ok result code
    
    if ( wantInterfaceInfo )
    {
        // get interface name and interface status from app layer
        interfaceStatus = C1222AL_TLGetInterfaceStatus(pApplication);
        interfaceName = C1222AL_TLGetInterfaceName(pApplication);
        
        interfaceNameLength = (Unsigned8)strlen(interfaceName);
        
        messageTotalLength += (Unsigned16)(interfaceNameLength + 1 + 1);   // interface name len, interface name, interface status
    }
    
    if ( (statusControl == 0) || (statusControl==2) )
    {
        // will be returning all statistics
        numberOfStatistics = C1222AL_TLGetNumberOfStatistics(pApplication);
    }
    else if ( statusControl == 3 )
    {
        numberOfStatistics = C1222AL_TLGetNumberOfChangedStatistics(pApplication);
    }

    messageTotalLength += (Unsigned16)(numberOfStatistics * (sizeof(Unsigned16)+sizeof(C1222Signed48)));
    
    p->status.sendInProgress = TRUE;
    
    ok = C1222DL_StartMessageBuild(pDatalink, messageTotalLength);
    
    response = 0; // ok
    
    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, &response, 1);
        
    if ( wantInterfaceInfo )
    {   
        if ( ok )
            ok = C1222DL_AddMessagePart(pDatalink, &interfaceNameLength, 1);
            
        if ( ok )
            ok = C1222DL_AddMessagePart(pDatalink, (Unsigned8*)interfaceName, interfaceNameLength);
            
        if ( ok )
            ok = C1222DL_AddMessagePart(pDatalink, &interfaceStatus, 1);
    }
    
    if ( numberOfStatistics > 0 )
    {
        for ( ii=0; ii<numberOfStatistics; ii++ )
        {
            if ( statusControl == 3 )
                C1222AL_TLGetChangedStatistic(pApplication, ii, &statistic);
            else
                C1222AL_TLGetStatistic(pApplication, ii, &statistic);
                
            C1222Misc_ReverseBytes(&statistic.statisticCode, sizeof(Unsigned16));
            C1222Misc_ReverseBytes(&statistic.statisticValue, sizeof(C1222Signed48));
                
            if ( ok )
                ok = C1222DL_AddMessagePart(pDatalink, (Unsigned8*)&statistic.statisticCode, sizeof(Unsigned16));

            if ( ok )
                ok = C1222DL_AddMessagePart(pDatalink, (Unsigned8*)&statistic.statisticValue, sizeof(C1222Signed48));
        }
    }
    
    if ( ok )
        ok = C1222DL_SendBuiltMessage(pDatalink);
        
    p->status.sendInProgress = FALSE;
        
    // should do something if the message fails
    
    if ( !ok )
        C1222TL_LogEvent(p, C1222EVENT_TL_SEND_GET_STATUS_RESPONSE_FAILED);
    
    (void)ok;        
}
#endif // C1222_INCLUDE_COMMMODULE



#ifdef C1222_INCLUDE_DEVICE
static void ProcessReceivedGetStatusResponse(C1222TL* p)
{
    Unsigned8* buffer = p->rxMessageBuffer;
    C1222AL* pApplication = (C1222AL*)(p->pApplication);

    C1222Statistic statistic;

    Unsigned8 interfaceNameLength;
    Unsigned8 interfaceStatus;

    C1222Statistic* pStats;
    
    Unsigned8 statusControl = p->status.pendingRequestU8Parameter;
    
    if ( statusControl == 0 || statusControl == 1 )
    { 
        interfaceNameLength = buffer[1];
        interfaceStatus = buffer[2+interfaceNameLength];
        C1222AL_TLUpdateInterfaceInfo(pApplication, interfaceNameLength, (char*)(&buffer[2]), interfaceStatus);
        C1222TL_LogEvent(p, C1222EVENT_TL_INTERFACE_INFO_UPDATED);
        pStats = (C1222Statistic*)&buffer[3+interfaceNameLength];
    }
    else
        pStats = (C1222Statistic*)&buffer[1];
        
    // update all statistics sent
    
    while ( (Unsigned8*)pStats < &buffer[p->rxMessageLength] )
    {
        statistic.statisticCode = pStats->statisticCode;
        statistic.statisticValue = pStats->statisticValue;
        
        C1222Misc_ReverseBytes(&statistic.statisticCode, sizeof(Unsigned16));
        C1222Misc_ReverseBytes(&statistic.statisticValue, sizeof(C1222Signed48));
        
        C1222AL_TLUpdateStatistic(pApplication, &statistic);

        pStats++;
    }
    
    C1222AL_TLNoteStatisticsUpdated(pApplication);
}
#endif


#ifdef C1222_INCLUDE_COMMMODULE
static void ProcessReceivedGetRegistrationStatusRequest(C1222TL* p)
{
    Unsigned8* responseBuffer = p->tempBuffer;
    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    C1222RegistrationStatus* pRegStatus;
    Unsigned16 messageTotalLength;
    Boolean ok;
    Unsigned8 aptitleLength;
    Unsigned8 mrAptitleLength;

    pRegStatus = C1222AL_TLGetRegistrationStatus(pApplication);

    // get the lengths of the aptitle, and master relay aptitle
    
    aptitleLength = getUIDLength(pRegStatus->nodeApTitle);
    mrAptitleLength = getUIDLength(pRegStatus->masterRelayApTitle);
    
    // get total message length
    
    messageTotalLength = (Unsigned16)(1+
                            1+pRegStatus->relayAddressLength+
                            aptitleLength+mrAptitleLength+
                            3*sizeof(Unsigned16));    
                            
    p->status.sendInProgress = TRUE;
    
    responseBuffer[0] = 0; // ok
    
    ok = C1222DL_StartMessageBuild(pDatalink, messageTotalLength);
    
    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, responseBuffer, 1);
            
    if ( ok )
        ok = AddLengthAndItemToMessage(pDatalink, pRegStatus->relayAddressLength, pRegStatus->relayNativeAddress);

    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, pRegStatus->masterRelayApTitle, mrAptitleLength);
        
    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, pRegStatus->nodeApTitle, aptitleLength);

    responseBuffer[0] = (Unsigned8)(pRegStatus->regPowerUpDelaySeconds >> 8);
    responseBuffer[1] = (Unsigned8)(pRegStatus->regPowerUpDelaySeconds & 0xFF);

    responseBuffer[2] = (Unsigned8)(pRegStatus->regPeriodMinutes >> 8);
    responseBuffer[3] = (Unsigned8)(pRegStatus->regPeriodMinutes & 0xFF);

    responseBuffer[4] = (Unsigned8)(pRegStatus->regCountDownMinutes >> 8);
    responseBuffer[5] = (Unsigned8)(pRegStatus->regCountDownMinutes & 0xFF);
    
    if ( ok )
        ok = C1222DL_AddMessagePart(pDatalink, responseBuffer, 6);
        
    if ( ok )
        ok = C1222DL_SendBuiltMessage(pDatalink);
        
    p->status.sendInProgress = FALSE;
        
    // should do something if the message fails
    
    if ( !ok )
        C1222TL_LogEvent(p, C1222EVENT_TL_SEND_GET_REG_STATUS_RESPONSE_FAILED);
}
#endif // C1222_INCLUDE_COMMMODULE


#ifdef C1222_REGISTRATION_HANDLED_BY_COMMMODULE
#ifdef C1222_INCLUDE_DEVICE
// since the register handles registration there is no point in sending this to the cm
static void ProcessReceivedGetRegistrationStatusResponse(C1222TL* p)
{   
    C1222RegistrationStatus* pRegStatus = (C1222RegistrationStatus*)p->tempBuffer;
    
    Unsigned8* buffer = p->rxMessageBuffer;
    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    Unsigned8* pBufferLoc;

    memset(pRegStatus, 0, sizeof(C1222RegistrationStatus));
    
    pBufferLoc = getLengthAndItem(&buffer[1], &pRegStatus->relayAddressLength, pRegStatus->relayNativeAddress, C1222_NATIVE_ADDRESS_LENGTH);
    pBufferLoc = getUIDItem(pBufferLoc, pRegStatus->masterRelayApTitle, C1222_APTITLE_LENGTH);
    pBufferLoc = getUIDItem(pBufferLoc, pRegStatus->nodeApTitle, C1222_APTITLE_LENGTH);
    
    pRegStatus->regPowerUpDelaySeconds = (Unsigned16)((pBufferLoc[0] << 8) | (pBufferLoc[1] & 0xFF));
    pRegStatus->regPeriodMinutes = (Unsigned16)(((Unsigned16)(pBufferLoc[2]) << 8) | (pBufferLoc[3] & 0xFF));
    pRegStatus->regCountDownMinutes = (Unsigned16)(((Unsigned16)(pBufferLoc[4]) << 8) | (pBufferLoc[5] & 0xFF));

    if ( getUIDLength(pRegStatus->masterRelayApTitle) < 2 )
    {
        memset(pRegStatus->masterRelayApTitle, 0, sizeof(pRegStatus->masterRelayApTitle));
        pRegStatus->masterRelayApTitle[0] = 0x0d;
    }    

    if ( getUIDLength(pRegStatus->nodeApTitle) < 2 )
    {
        memset(pRegStatus->nodeApTitle, 0, sizeof(pRegStatus->nodeApTitle));
        pRegStatus->nodeApTitle[0] = 0x0d;
    }
    
    if ( pRegStatus->relayAddressLength > C1222_NATIVE_ADDRESS_LENGTH )
        pRegStatus->relayAddressLength = C1222_NATIVE_ADDRESS_LENGTH;    
    
    // update the regStatus in the application layer
    
    if ( p->rxMessageLength >= (&pBufferLoc[6]-buffer) )
    {
        C1222AL_TLUpdateRegistrationStatus(pApplication, pRegStatus);
        C1222TL_LogEvent(p, C1222EVENT_TL_REG_STATUS_UPDATED);
    }
    
    // else there should be some effect
}
#endif // C1222_INCLUDE_DEVICE
#endif // C1222_REGISTRATION_HANDLED_BY_COMMMODULE






static void ProcessReceivedSendShortMessageRequest(C1222TL* p)
{

#ifdef C1222_WANT_SHORT_MESSAGES
    // if the user is sending the short message , he is not expecting a response
    
    // depending on how gas meters are handled, this might be needed

    Unsigned8* buffer = p->rxMessageBuffer;
    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    C1222DL* pDatalink = (C1222DL*)(p->pDatalink);
    Unsigned8* nativeAddress = p->tempBuffer;
    Unsigned8 nativeAddressLength;
    Unsigned8 response = C1222RESPONSE_OK;
    BufferItem bufferItem;
    
    bufferItem.buffer = getLengthAndItem(&buffer[1], &nativeAddressLength, nativeAddress, C1222_NATIVE_ADDRESS_LENGTH);
    bufferItem.length = (Unsigned16)(p->rxMessageLength - (1+nativeAddressLength));
    
    // TODO - the short message needs to be expanded somewhere into an acse message
    //      should be notification write (qualifier)
    //      no response wanted (epsem control)
    //      need to expand the calling aptitle to be ok for the full network
    //      need to add a called aptitle
    //      need to add a calling apinvocation id
    // most of this stuff needs to be done in the application layer
    
    // no response is expected for short messages

    (void)C1222AL_TLShortMessageReceived(pApplication, nativeAddressLength, nativeAddress, &bufferItem);
#else // not want short messages
    (void)p;
#endif
}




static void ProcessReceivedSendShortMessageResponse(C1222TL* p)
{
    // not supported
    
    // since I'm not sending the short message request, don't need to handle
    // the response
    
    // depending on how gas meters are handled, this might be needed

    (void)p;
}



static void ProcessReceivedUndefinedRequest(C1222TL* p)
{
    Unsigned8 response = C1222RESPONSE_IAR;   // correct code?
    
    SendMessage(p, &response, 1);
}


void C1222TL_ClearCounters(C1222TL* p)
{
    memset(&p->counters, 0, sizeof(p->counters));
}

void C1222TL_ClearStatusDiagnostics(C1222TL* p)
{
    p->status.maxReceivedMessageProcessTime = 0;
    p->status.avgReceivedMessageProcessTime = 0;
    p->status.totalReceivedMessageProcessTime = 0;
    
    p->status.maxMessageSendTime = 0;
    p->status.avgMessageSendTime = 0;
    p->status.totalMessageSendTime = 0;    
}
