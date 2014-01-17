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




// C12.22 Application Layer

// the app layer needs to be a state machine to allow the run method to be called on
// more than one stack.  For example, while a segmented message is being sent through one 
// interface, this should not prevent the other interface from receiving or sending a message.

// need to handle the differences between a comm module and a device in this file (module)


#include "c1222environment.h"
#include "c1222al.h"
#include "c1222tl.h"
#include "c1222response.h"
#include "c1222misc.h"
#include "c1222statistic.h"


extern void C1222EnterCriticalRegion(void);
extern void C1222LeaveCriticalRegion(void);

extern void C1222PL_WaitMs(Unsigned16 ms);


#ifdef C1222_INCLUDE_DEVICE
extern void RunDevice(C1222AL* p);
#endif

#ifdef C1222_INCLUDE_RELAY
extern void RunRelay(C1222AL* p);
#endif



#ifdef C1222_INCLUDE_COMMMODULE
extern void RunCommModule(C1222AL* p);
extern Boolean CommModuleHandleNetworkMessageDuringRegistration(C1222AL* p, ACSE_Message* pMsg, 
                      Unsigned8 addressLength, Unsigned8* nativeAddress);
#endif                      



// this was moved from c1222al_device since it is used in tl
void C1222AL_TL_NotifyFailedLinkControl(C1222AL* p, Unsigned8 response)
{  
    p->status.linkControlRunning = FALSE;
    p->status.linkControlResponse = response;
}




void C1222AL_RequireEncryptionOrAuthenticationIfKeys(C1222AL* p, Boolean b)
{
    p->status.requireEncryptionOrAuthenticationIfKeysExist = b;
}




void C1222AL_SetClientStandardVersion(C1222AL* p, C1222StandardVersion version)
{
    p->status.clientC1222StandardVersion = version;
}



void C1222AL_SetClientSendACSEContextOption(C1222AL* p, Boolean b)
{
    p->status.clientSendACSEContext = b;
}




void C1222AL_LogReceivedMessage(C1222AL* p, ACSE_Message* pMsg)
{
    C1222MessageLog* pLog;
    C1222ApTitle aptitle;
    Unsigned16 sequence;
    
    p->receivedMessageIndex %= (Unsigned8)(ELEMENTS(p->receivedMessageLog));
    
    pLog = &p->receivedMessageLog[p->receivedMessageIndex];
    
    if ( ACSE_Message_GetCallingApInvocationId(pMsg, &sequence) )
        pLog->sequence = sequence;
    else
        pLog->sequence = 0;
        
    if ( ACSE_Message_GetCallingApTitle(pMsg, &aptitle) && (aptitle.length > 2) )
    {
        pLog->callingApTitleEnd[0] = aptitle.buffer[aptitle.length-2];
        pLog->callingApTitleEnd[1] = aptitle.buffer[aptitle.length-1];
    }
    else
    {
        pLog->callingApTitleEnd[0] = 0;
        pLog->callingApTitleEnd[1] = 0;
    }
    
    p->receivedMessageIndex++;
}


void C1222AL_LogSentMessage(C1222AL* p, ACSE_Message* pMsg)
{
    C1222MessageLog* pLog;
    C1222ApTitle aptitle;
    Unsigned16 sequence;
    
    p->sentMessageIndex %= (Unsigned8)(ELEMENTS(p->sentMessageLog));
    
    pLog = &p->sentMessageLog[p->sentMessageIndex];
    
    if ( ACSE_Message_GetCallingApInvocationId(pMsg, &sequence) )
        pLog->sequence = sequence;
    else
        pLog->sequence = 0;
        
    if ( ACSE_Message_GetCallingApTitle(pMsg, &aptitle) && (aptitle.length > 2) )
    {
        pLog->callingApTitleEnd[0] = aptitle.buffer[aptitle.length-2];
        pLog->callingApTitleEnd[1] = aptitle.buffer[aptitle.length-1];
    }
    else
    {
        pLog->callingApTitleEnd[0] = 0;
        pLog->callingApTitleEnd[1] = 0;
    }
    
    p->sentMessageIndex++;
}




Boolean C1222AL_IsProcedureRunning(C1222AL* p)
{
    return (Boolean)((p->status.inProcessProcedureState != C1222PS_IDLE) ? TRUE : FALSE);
}




void C1222AL_NoteTryingToGetRFLANRevisionInfo(C1222AL* p, Boolean b)
{
    p->status.tryingToGetRFLANRevisionInfo = b;
}




// ****************************************************************************************
// these routines are called by the C1222Stack
// ****************************************************************************************

Boolean C1222AL_GetMyApTitle(C1222AL* p, C1222ApTitle* pApTitle)
{
    Unsigned8* apTitleBuffer;
    
    if ( p->status.isRegistered )
        apTitleBuffer = p->registrationStatus.nodeApTitle;
    else
        apTitleBuffer = p->transportConfig.nodeApTitle;
        
    C1222ApTitle_Construct(pApTitle, apTitleBuffer, C1222_APTITLE_LENGTH);
        
    pApTitle->length = C1222ApTitle_GetLength(pApTitle);
    
    return C1222ApTitle_TableValidate(pApTitle);
}


 


#ifdef C1222_INCLUDE_COMMMODULE
Boolean C1222AL_IsMyApTitleAvailable(C1222AL* p)
{
    C1222ApTitle myApTitle;
    static Unsigned8 defaultApTitle[] = { 0x0d, 0x01, 0x01 };
    
    if ( C1222AL_GetMyApTitle(p, &myApTitle) )
    {
        if ( memcmp(defaultApTitle,myApTitle.buffer, sizeof(defaultApTitle)) == 0 )
            return FALSE;
            
        return TRUE;
    }
    
    return FALSE;
}
#endif // C1222_INCLUDE_COMMMODULE




void C1222AL_IncrementStatistic(C1222AL* p, Unsigned16 id)
{
    C1222AL_AddToStatistic(p, id, 1);
}



void C1222AL_ResetAllStatistics(C1222AL* p)
{
    Unsigned8 ii;
    C1222Statistic* pStat = &(p->transportStatus.statistics[0]);
    
    for ( ii=0; ii<C1222_NUMBER_OF_STATISTICS; ii++ )
    {
        if ( pStat->statisticCode != 0 )
        {
            C1222EnterCriticalRegion();
            pStat->statisticValue.lower32Bits = 0;
            pStat->statisticValue.upper16Bits = 0;
                
            p->transportStatus.changedStatistics[ii] = FALSE;
            C1222LeaveCriticalRegion();
        }
        
        pStat++;
    }
}



void C1222AL_AddToStatistic(C1222AL* p, Unsigned16 id, Unsigned32 count)
{
    Unsigned8 ii;
    C1222Statistic* pStat = &(p->transportStatus.statistics[0]);
    
    for ( ii=0; ii<C1222_NUMBER_OF_STATISTICS; ii++ )
    {   
        if ( pStat->statisticCode == id )
        {
            C1222EnterCriticalRegion();
            pStat->statisticValue.lower32Bits += count;
            if ( pStat->statisticValue.lower32Bits < count )
                pStat->statisticValue.upper16Bits++;
                
            p->transportStatus.changedStatistics[ii] = TRUE;
            C1222LeaveCriticalRegion();
            break;
        }
        else if ( pStat->statisticCode == 0 )
        {
            C1222EnterCriticalRegion();
            pStat->statisticCode = id;
            pStat->statisticValue.lower32Bits = count;
            p->transportStatus.changedStatistics[ii] = TRUE;
            C1222LeaveCriticalRegion();
            break;
        }
        
        pStat++;
    }
}



Boolean C1222AL_IsSynchronized(C1222AL* p)
{
    return p->status.isSynchronizedOrConnected;
}



void C1222AL_SetStatisticU32(C1222AL* p, Unsigned16 id, Unsigned32 value)
{
    Unsigned8 ii;
    C1222Statistic* pStat = &(p->transportStatus.statistics[0]);
    
    for ( ii=0; ii<C1222_NUMBER_OF_STATISTICS; ii++ )
    {
        if ( pStat->statisticCode == id )
        {
            C1222EnterCriticalRegion();
            pStat->statisticValue.upper16Bits = 0;
            pStat->statisticValue.lower32Bits = value;
                
            p->transportStatus.changedStatistics[ii] = TRUE;
            C1222LeaveCriticalRegion();
            break;
        }
        else if ( pStat->statisticCode == 0 )
        {
            C1222EnterCriticalRegion();
            pStat->statisticCode = id;
            pStat->statisticValue.upper16Bits = 0;
            pStat->statisticValue.lower32Bits = value;
            p->transportStatus.changedStatistics[ii] = TRUE;
            C1222LeaveCriticalRegion();
            break;
        }
        
        pStat++;
    }
}




// buffer is 6 bytes

void C1222AL_SetStatisticValueBuffer(C1222AL* p, Unsigned16 id, Unsigned8* buffer)
{
    Unsigned8 ii;
    C1222Statistic* pStat = &(p->transportStatus.statistics[0]);
    
    for ( ii=0; ii<C1222_NUMBER_OF_STATISTICS; ii++ )
    {
        if ( pStat->statisticCode == id )
        {
            C1222EnterCriticalRegion();
            memcpy(&pStat->statisticValue, buffer, sizeof(pStat->statisticValue));                
            p->transportStatus.changedStatistics[ii] = TRUE;
            C1222LeaveCriticalRegion();
            break;
        }
        else if ( pStat->statisticCode == 0 )
        {
            C1222EnterCriticalRegion();
            pStat->statisticCode = id;
            memcpy(&pStat->statisticValue, buffer, sizeof(pStat->statisticValue));
            p->transportStatus.changedStatistics[ii] = TRUE;
            C1222LeaveCriticalRegion();
            break;
        }
        
        pStat++;
    }
}





void C1222AL_SetRFLANisSynchronizedState(C1222AL* p, Boolean isSynchronized)
{
    p->status.isSynchronizedOrConnected = isSynchronized;
    
    C1222AL_SetStatisticU32(p, C1222_STATISTIC_IS_SYNCHRONIZED_NOW, isSynchronized); 
}





Boolean C1222AL_IsRegistered(C1222AL* p)
{
    return p->status.isRegistered;
}







void C1222AL_ConstructCallingApTitle(C1222AL* pAppLayer, C1222ApTitle* pApTitle)
{
    if ( ( (pAppLayer->registrationStatus.nodeApTitle[0] == 0x06) || (pAppLayer->registrationStatus.nodeApTitle[0] == 0x0d)) &&
         ( pAppLayer->registrationStatus.nodeApTitle[1] > 0) )
    {
        C1222ApTitle_Construct( pApTitle, pAppLayer->registrationStatus.nodeApTitle, C1222_APTITLE_LENGTH);
    }
    else
    {
        C1222ApTitle_Construct( pApTitle, pAppLayer->transportConfig.nodeApTitle, C1222_APTITLE_LENGTH);
    }
    
    pApTitle->length = C1222ApTitle_GetLength( pApTitle );
}





Boolean C1222AL_IsSendMessageComplete(C1222AL* p)
{
    return (Boolean)((p->status.state != ALS_SENDINGMESSAGE) ? TRUE : FALSE);
}



void NoteReceivedCompleteMessageHandlingComplete(C1222AL* p)
{   
    Unsigned32 elapsedTime = C1222Misc_GetFreeRunningTimeInMS() - p->status.lastCompleteMessageTime;
    
    p->status.totalCompleteMessageProcessTime += elapsedTime;
    
    if ( p->counters.messagesReceived > 0 )
    {
        p->status.avgCompleteMessageProcessTime = (Unsigned16)(p->status.totalCompleteMessageProcessTime/p->counters.messagesReceived);
    }
    else
        p->status.avgCompleteMessageProcessTime = 0;
           
    if ( elapsedTime > (Unsigned32)(p->status.maxCompleteMessageProcessTime) )
        p->status.maxCompleteMessageProcessTime = (Unsigned16)elapsedTime;
            
}




// this is used by c1222 stack to notify that it is done with the last message
// received which should allow the transport to receive a new message
void C1222AL_NoteReceivedMessageHandlingIsComplete(C1222AL* p)
{  
    p->status.isReceivedMessageHandlingComplete = TRUE;
    
    if ( p->status.sendingRegistrationRequest )
        C1222AL_LogEvent(p, C1222EVENT_AL_NOTEREGISTERMESSAGEHANDLINGCOMPLETE);
    else    
        C1222AL_LogEvent(p, C1222EVENT_AL_NOTERECEIVEMESSAGEHANDLINGCOMPLETE);
        
    C1222MessageReceiver_NoteMessageHandlingComplete(&p->messageReceiver);
    
    if ( p->status.isReceivedMessageResponsePending ) // this is the normal case
    {
        p->status.isReceivedMessageHandlingComplete = TRUE;
        p->status.isReceivedMessageResponsePending = FALSE;
        
        NoteReceivedCompleteMessageHandlingComplete(p);
        
        if ( !p->status.sendingRegistrationRequest ) // it was generated here
            C1222TL_SendPendingSendMessageOKResponse((C1222TL*)(p->pTransport));
            
        p->status.sendingRegistrationRequest = FALSE;
    } 
    else
    {
        // this is an error
        
        C1222AL_LogEvent(p, C1222EVENT_AL_RECEIVEMESSAGEHANDLINGCOMPLETEBUTNOTPENDING);
    }   
}



Boolean C1222AL_IsStackWaitingOnReceivedMessageHandlingNotification(C1222AL* p)
{   
    return !p->status.isReceivedMessageComplete && p->status.isReceivedMessageResponsePending;
}



// this is used by c1222 stack to notify that it is done with the last message
// received which should allow the transport to receive a new message
// and the last message could not be sent
void C1222AL_NoteReceivedMessageHandlingFailed(C1222AL* p, Unsigned8 result)
{    
    p->status.isReceivedMessageHandlingComplete = TRUE;
    
    if ( p->status.sendingRegistrationRequest )
        C1222AL_LogEventAndBytes(p, C1222EVENT_AL_NOTEREGISTERMESSAGEHANDLINGFAILED, 1, &result);
    else
        C1222AL_LogEventAndBytes(p, C1222EVENT_AL_NOTERECEIVEMESSAGEHANDLINGFAILED, 1, &result);
        
    C1222MessageReceiver_NoteMessageHandlingComplete(&p->messageReceiver);
    
    if ( p->status.isReceivedMessageResponsePending ) // this is the normal case
    {
        p->status.isReceivedMessageHandlingComplete = TRUE;
        p->status.isReceivedMessageResponsePending = FALSE;
        
        NoteReceivedCompleteMessageHandlingComplete(p);
        
        if ( !p->status.sendingRegistrationRequest ) // it was generated here
            C1222TL_SendPendingSendMessageFailedResponse((C1222TL*)(p->pTransport), result);
            
        p->status.sendingRegistrationRequest = FALSE;
    } 
    else
    {
        // this is an error
        
        C1222AL_LogEvent(p, C1222EVENT_AL_RECEIVEMESSAGEHANDLINGFAILEDBUTNOTPENDING);
    }   
}






Boolean C1222AL_TLIsReceivedMessageHandlingComplete(C1222AL* p)
{
    return p->status.isReceivedMessageHandlingComplete;
}



void C1222AL_NoteReceivedCompleteMessage(C1222AL* p)
{    
    p->status.isReceivedMessageHandlingComplete = FALSE;
    p->status.isReceivedMessageComplete = TRUE;
    p->status.isReceivedMessageResponsePending = TRUE;
    
    p->status.lastCompleteMessageTime = C1222Misc_GetFreeRunningTimeInMS();
    
    C1222AL_LogEvent(p, C1222EVENT_AL_RECEIVEDMESSAGECOMPLETE);
}



void waitForTransport(C1222AL* p)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    Unsigned32 start = C1222Misc_GetFreeRunningTimeInMS();
    
    while( C1222TL_IsBusy(pTransport) )
    {
        C1222TL_Run(pTransport);
        
        if ( (C1222Misc_GetFreeRunningTimeInMS()-start) > 5000L )
            break;
    }
}



void C1222AL_SetSendTimeoutSeconds(C1222AL* p, Unsigned16 seconds)
{
    p->status.sendTimeOutSeconds = seconds;
}


void C1222AL_LogFailedSendMessage(C1222AL* p, C1222MessageFailureCode reason, ACSE_Message* pMsg)
{
    Unsigned8 buffer[3];
    Unsigned16 sequence;
    
    if ( !ACSE_Message_GetCallingApInvocationId(pMsg, &sequence) )
        sequence = 0;
        
    *(Unsigned16*)buffer = sequence;
    buffer[2] = (Unsigned8) reason;  // assumes the reason fits in 1 byte    
    
    p->status.consecutiveSendFailures++;
    C1222AL_LogComEventAndBytes(p, COMEVENT_C1222_SEND_MESSAGE_FAILED, buffer, sizeof(buffer));
    
    if ( p->status.consecutiveSendFailures > p->status.consecutiveSendFailureLimit )
    {
        p->status.consecutiveSendFailures = 0;
        C1222AL_LogComEvent(p, COMEVENT_C1222_CONFIGURED_SEND_FAILURE_LIMIT_EXCEEDED);
    }
}



Unsigned8 ExpandRequestResponseMessageType(ACSE_Message* pMsg, Unsigned8 messageType)
{
    Unsigned8* buffer;
    Unsigned16 length;
    Epsem epsem;
    Unsigned8* ebuffer;
    Unsigned16 elength;
    Unsigned16 table;
    Unsigned16 procedureId;
    Boolean hasTableWrites = FALSE;
    Boolean hasTableReads = FALSE;
    Boolean hasNonOKResponses = FALSE;
    Boolean hasOKResponses = FALSE;
    
    if ( messageType != C1222_MSGTYPE_REQUEST && messageType != C1222_MSGTYPE_RESPONSE )
        return C1222_MSGTYPE_UNKNOWN;
    
    if ( ACSE_Message_GetApData(pMsg, &buffer, &length) )
    {
        Epsem_Init(&epsem, buffer, length);        
                               
        while( Epsem_GetNextRequestOrResponse(&epsem, &ebuffer, &elength) )
        {
            if ( ebuffer[0] > 0 && ebuffer[0] < 32 )
                hasNonOKResponses = TRUE;
                
            if ( ebuffer[0] == 0 )
                hasOKResponses = TRUE;
                
                                    
            switch(ebuffer[0])
            {                                            
                case 0x40:
                    // full table write = .TTLL<data>C
                    table = (Unsigned16)(ebuffer[1]*256 + ebuffer[2]);
                    if ( table == 7 )
                    {
                        // procedure
                        procedureId = (Unsigned16)((ebuffer[5] + ebuffer[6]*256) & 0xFFF);
                        switch(procedureId)
                        {
                            case (39+2048):
                                return C1222_MSGTYPE_SCHEDULE_PERIODIC_READ;
                            //  maybe handle others later
                        }
                    }
                    else
                        hasTableWrites = TRUE;
                    break;
                case 0x4F:
                    // offset table write = .TTOOOLL<data>C
                    table = (Unsigned16)(ebuffer[1]*256 + ebuffer[2]);
                    switch(table)
                    {
                        default:
                            hasTableWrites = TRUE;
                            break;
                    }
                    break;
                case 0x30:
                case 0x3F:
                    hasTableReads = TRUE;
                    break;
            }
        }
    }
    
    if ( hasOKResponses )
    {
        if ( hasNonOKResponses )
            return C1222_MSGTYPE_OK_AND_ERROR_RESPONSES;
        else
            return C1222_MSGTYPE_OK_RESPONSES;
    }
    else if ( hasNonOKResponses )
        return C1222_MSGTYPE_ERROR_RESPONSES;
    
    if ( hasTableReads )
    {
        if ( hasTableWrites )
            return C1222_MSGTYPE_TABLE_READS_AND_WRITES;
        else
            return C1222_MSGTYPE_TABLE_READS;
    }
    else if ( hasTableWrites )
        return C1222_MSGTYPE_TABLE_WRITES;
        
    return C1222_MSGTYPE_UNKNOWN;
}


void C1222AL_MakeMessageLogEntry(C1222AL* p, ACSE_Message* pMsg, C1222MessageLogEntry* pEntry, Boolean sending, Unsigned8 messageType)
{
    C1222ApTitle aptitle;
    Unsigned16 sequence;
    
    (void)p;
    
    memset(pEntry, 0, sizeof(C1222MessageLogEntry));
    
    if ( sending )
    {
        if ( ACSE_Message_GetCalledApTitle(pMsg, &aptitle) )
            memcpy(pEntry->aptitle, aptitle.buffer, aptitle.length);
    }
    else
    {
        if ( ACSE_Message_GetCallingApTitle(pMsg, &aptitle) )
            memcpy(pEntry->aptitle, aptitle.buffer, aptitle.length);
    }
    
    C1222ApTitle_ConvertACSERelativeToTableRelative(pEntry->aptitle);
        
    if ( ACSE_Message_GetCallingApInvocationId(pMsg, &sequence) )
        pEntry->callingSequence = sequence;
        
    if ( ACSE_Message_GetCalledApInvocationId(pMsg, &sequence) )
        pEntry->calledSequence = sequence;
        
    pEntry->messageType = messageType;
    pEntry->requestOrResponseMessageType = ExpandRequestResponseMessageType(pMsg, messageType);
}



Boolean C1222AL_IsMsgForCommModule(C1222AL* p, ACSE_Message* pMsg)
{
    C1222ApTitle myApTitle;
    C1222ApTitle calledApTitle;
    
    // if no called aptitle assume it is for the comm module
    
    if ( ! ACSE_Message_GetCalledApTitle(pMsg, &calledApTitle) )
        return TRUE;  // not sure of this
    
    if ( C1222AL_GetMyApTitle(p, &myApTitle) )
    {   
        // explicitly my aptitle
             
        if ( C1222ApTitle_IsApTitleMyCommModule(&calledApTitle, &myApTitle) )
        {
            return TRUE;
        }
        
        // rflan default aptitle
        
        if ( calledApTitle.buffer[1] == 1 && calledApTitle.buffer[2] == 1 )
        {
            if ( calledApTitle.buffer[0] == 0x0d || calledApTitle.buffer[0] == 0x80 )
                return TRUE;
        }
    }
    
    return FALSE;
}


Boolean C1222AL_SendMessage(C1222AL* p, ACSE_Message* pMsg, Unsigned8* nativeAddress, Unsigned16 addressLength)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    C1222MessageSender* pSender = (C1222MessageSender*)(&p->messageSender);
        
    p->status.sending = TRUE;
    
    waitForTransport(p);
    
    if ( C1222TL_IsBusy(pTransport) )
    {
        p->counters.sendAbortedTransportBusy++;
        
        p->status.sending = FALSE;
        
        C1222AL_LogFailedSendMessage(p, MFC_SEND_TRANSPORT_BUSY, pMsg); // tl busy
        
        C1222AL_LogEvent(p, C1222EVENT_AL_MSG_NOT_SENT_TL_BUSY);
        return FALSE;
    }
    
    if ( !p->status.isCommModule )
    {
        C1222AL_IncrementStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_ACSE_PDU_SENT);
    } 
    
    // this routine is what is called by the stack to send a message.
    // the stack is not aware of who the message is for
    // if this is the comm module, this could be a registration response, in which case we just need
    // to handle it
    
    if ( p->status.isCommModule && (p->status.state == ALS_REGISTERING) )
    {
        #ifdef C1222_INCLUDE_COMMMODULE
        if ( CommModuleHandleNetworkMessageDuringRegistration(p, pMsg, addressLength, nativeAddress) )
        {
            p->status.sending = FALSE;
            return TRUE;
        }
        #endif
    }
    
    p->status.lastRequestTime = C1222Misc_GetFreeRunningTimeInMS();
       
    
    C1222MessageSender_SetLinks( pSender, pTransport, p); 
    if ( C1222MessageSender_StartSend( pSender, pMsg, nativeAddress, addressLength) )
    {
        C1222AL_LogEvent(p, C1222EVENT_AL_SENDING_MSG);
        p->status.state = ALS_SENDINGMESSAGE;
        p->status.sendStartTime = C1222Misc_GetFreeRunningTimeInMS();
        p->status.sending = FALSE;
        
        C1222AL_LogSentMessage(p, pMsg);
        return TRUE;
    }
    
    C1222AL_LogEvent(p, C1222EVENT_AL_START_SEND_FAILED);
    
    p->counters.sendAbortedOtherReason++;
    p->status.sending = FALSE;
    
    return FALSE;
}




#ifdef C1222_WANT_SHORT_MESSAGES

Boolean C1222AL_SendShortMessage(C1222AL* p, ACSE_Message* pMsg, Unsigned8* nativeAddress, Unsigned16 addressLength)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    C1222MessageSender* pSender = (C1222MessageSender*)(&p->messageSender);
    
    if ( C1222TL_IsBusy(pTransport) )
    {
        C1222AL_LogEvent(p, C1222EVENT_AL_MSG_NOT_SENT_TL_BUSY);
        return FALSE;
    }
    
    if ( !p->isCommModule )
    {
        C1222AL_IncrementStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_ACSE_PDU_SENT);
    } 
    
    // this routine is what is called by the stack to send a message.
    // the stack is not aware of who the message is for
    // if this is the comm module, this could be a registration response, in which case we just need
    // to handle it
    
    // but - the short message cannot be a registration response       
    
    C1222MessageSender_SetLinks( pSender, pTransport, p); 
    
    if ( C1222MessageSender_SendShortMessage( pSender, pMsg, nativeAddress, addressLength) )
    {
        // state does not change sending a short message since we do not wait for a response
        return TRUE;
    }
    
    C1222AL_LogEvent(p, C1222EVENT_AL_START_SEND_FAILED);
    
    return FALSE;
}

#endif












void RunLocalPort(C1222AL* p)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    switch( p->status.state )
    {
        case ALS_INIT:
        	p->status.state = ALS_IDLE;
            // flow through to allow faster processing
            
        case ALS_IDLE:
            C1222TL_Run( pTransport );
            break;
            
        case ALS_SENDINGMESSAGE:
            C1222TL_Run( pTransport );
            break;            
    }   
}




void HandleReceiveSegmentTimeout(C1222AL* p)
{
    if ( C1222MessageReceiver_HandleReceiveSegmentTimeout(&p->messageReceiver) )
    {
        p->counters.partialMessageTimedoutAndDiscarded++;
        p->status.timeOfLastDiscardedSegmentedMessage = C1222Misc_GetFreeRunningTimeInMS();
    }
}




void C1222AL_Run(C1222AL* p)
{    
    p->counters.runCount++;
    
    // let rx messages never time out - on rflan anyway
    // we will abort them if receive a segment for another message or if we have something to send
    
    HandleReceiveSegmentTimeout(p);
    
    if ( p->status.isLocalPort )
    {
        RunLocalPort(p);
    }
    else if ( p->status.isCommModule )
    {
        #ifdef C1222_INCLUDE_COMMMODULE
        RunCommModule(p);
        #endif
    }
    else if ( p->status.isRelay )
    {
    	#ifdef C1222_INCLUDE_RELAY
        RunRelay(p);
        #endif
    }
    else
    {
        #ifdef C1222_INCLUDE_DEVICE
        RunDevice(p);
        #endif
    }
}



ACSE_Message* StartNewUnsegmentedMessage(C1222AL* p)
{
    return C1222ALSegmenter_StartNewUnsegmentedMessage(&(p->messageReceiver.receiveSegmenter));
}

void EndNewUnsegmentedMessage(C1222AL* p)
{
    C1222ALSegmenter_EndNewUnsegmentedMessage(&p->messageReceiver.receiveSegmenter);
}


void SetMessageDestination(C1222AL* p, Unsigned8 addressLength, Unsigned8* address)
{
    // this should check parameters!!!
    p->messageReceiver.nativeAddressLength = addressLength;
    memcpy(p->messageReceiver.nativeAddress, address, addressLength);
}



ACSE_Message* C1222AL_GetReceivedMessage(C1222AL* p)
{
    ACSE_Message* pMsg;
    
	pMsg = C1222MessageReceiver_GetUnsegmentedMessage( &p->messageReceiver);
	
	p->status.isReceivedMessageComplete = FALSE;
	
	if ( !p->status.isCommModule )
	{
	    C1222AL_IncrementStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_ACSE_PDU_RECEIVED);
	}
	
	return pMsg;
}



Boolean C1222AL_GetReceivedMessageNativeAddress(C1222AL* p, Unsigned8* pNativeAddressLength, Unsigned8** pNativeAddress)
{
    *pNativeAddressLength = p->messageReceiver.nativeAddressLength;
    *pNativeAddress = p->messageReceiver.nativeAddress;
    return TRUE;
}


// this is now also used for deregistration - 9/25/06 scb
void C1222AL_SetRegistrationRetryTime(C1222AL* p)
{       
    if ( p->status.registrationRetryFactor == 0 )
        p->status.registrationRetryFactor = 1;
    else
        p->status.registrationRetryFactor *= (Unsigned16)2;
        
    if ( p->status.registrationRetryFactor > p->status.registrationMaxRetryFactor )
        p->status.registrationRetryFactor = p->status.registrationMaxRetryFactor;
        
    p->status.registrationRetrySeconds = 
        C1222Misc_GetRandomDelayTime(p->status.registrationRetryMinutesMin*60L,
                                     p->status.registrationRetryMinutesMax*60L);
                                   
    p->status.registrationRetrySeconds *= p->status.registrationRetryFactor; 
    
    if ( p->status.tryingToRegisterAfterCellSwitch && (p->status.cellSwitchRegistrationRetryTimeDivisor != 0) )
        p->status.registrationRetrySeconds /= p->status.cellSwitchRegistrationRetryTimeDivisor;                                   
}

void C1222AL_ResetRegistrationRetryFactor(C1222AL* p)
{
    p->status.registrationRetryFactor = 0;    
}


void C1222AL_SetRegistrationMaxRetryFactor(C1222AL* p, Unsigned8 factor)
{    
    if ( factor == 255 )
        p->status.registrationMaxRetryFactor = DEFAULT_MAX_REGISTRATION_RETRY_FACTOR;
    else if ( factor == 0 )
        p->status.registrationMaxRetryFactor = 256;
    else
        p->status.registrationMaxRetryFactor = factor;
}


void C1222AL_SetRegistrationRetryLimits(C1222AL* p, Unsigned16 minMinutes, Unsigned16 maxMinutes, Unsigned8 cellSwitchDivisor)
{    
    if ( (minMinutes < maxMinutes) && (cellSwitchDivisor != 0) )
    {
        p->status.registrationRetryMinutesMin = minMinutes;
        p->status.registrationRetryMinutesMax = maxMinutes;
        p->status.cellSwitchRegistrationRetryTimeDivisor = cellSwitchDivisor;
    }
    // else leave the registration retry at the default or the last valid configuration
    
    C1222AL_ResetRegistrationRetryFactor(p);
    
    C1222AL_SetRegistrationRetryTime(p);
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

Boolean C1222AL_NeedToReregisterAfterConfigChange(C1222AL* p, C1222TransportConfig* pCfg)
{
    Boolean needToRegister = FALSE;
    
    if ( pCfg->nodeType != p->transportConfig.nodeType )
        needToRegister = TRUE;

    if ( memcmp(pCfg->deviceClass, p->transportConfig.deviceClass, C1222_DEVICE_CLASS_LENGTH) != 0 )
        needToRegister = TRUE;

    if ( memcmp(pCfg->esn, p->transportConfig.esn, C1222_ESN_LENGTH) != 0 )
        needToRegister = TRUE;
        
    if ( pCfg->nativeAddressLength != p->transportConfig.nativeAddressLength )
        needToRegister = TRUE;

    if ( pCfg->nativeAddressLength != 0 )
    {
        if ( memcmp(pCfg->nativeAddress, p->transportConfig.nativeAddress, pCfg->nativeAddressLength) != 0 )
            needToRegister = TRUE;
    }
        
    if ( pCfg->relayAddressLength != p->transportConfig.relayAddressLength )
        needToRegister = TRUE;

    if ( pCfg->relayAddressLength != 0 )
    {
        if ( memcmp(pCfg->relayNativeAddress, p->transportConfig.relayNativeAddress, pCfg->relayAddressLength) != 0 )
            needToRegister = TRUE;
    }

    if ( memcmp(pCfg->nodeApTitle, p->transportConfig.nodeApTitle, C1222_APTITLE_LENGTH) != 0 )
        needToRegister = TRUE;
        
    if ( memcmp(pCfg->masterRelayApTitle, p->transportConfig.masterRelayApTitle, C1222_APTITLE_LENGTH) != 0 )
        needToRegister = TRUE;        

    return needToRegister;
}



void C1222AL_SetConfiguration(C1222AL* p, C1222TransportConfig* pCfg)
{    
    Boolean needToReregister = C1222AL_NeedToReregisterAfterConfigChange(p, pCfg);
    
    if ( memcmp(pCfg, &p->transportConfig, sizeof(C1222TransportConfig)) == 0 )
        return; // no change needed
    
    memcpy(&(p->transportConfig), pCfg, sizeof(C1222TransportConfig));
    
    if ( needToReregister )
    {
        C1222AL_SetRegisteredStatus(p, FALSE);
        p->status.waitingToRegister = FALSE;
        p->status.haveRegistrationStatus = FALSE;
    }
    
    //p->status.state = ALS_INIT;  // start over again?
    
    p->status.haveConfiguration = TRUE;
    
    p->status.haveInterfaceInfo = FALSE;
    p->status.configurationChanged = TRUE;
    C1222TL_InhibitOptionalCommunication((C1222TL*)(p->pTransport));
}



void C1222AL_TLNoteConfigurationSent(C1222AL* p)
{
    p->status.configurationChanged = FALSE;
    C1222AL_LogEvent(p, C1222EVENT_AL_CONFIG_SENT);
}



Boolean C1222AL_IsMessageComplete(C1222AL* p)
{
    return p->status.isReceivedMessageComplete;
}



void C1222AL_SetInterfaceName(C1222AL* p, const char* name)
{
    p->pInterfaceName = name;
}



void C1222AL_SetReassemblyOption(C1222AL* p, Boolean wantReassembly)
{
    p->status.wantReassembly = wantReassembly;
}



void C1222AL_SetLog(C1222AL* p,
	   void (*fp1)(C1222AL* p, Unsigned16 event),
       void (*fp2)(C1222AL* p, Unsigned16 event, Unsigned8 n, Unsigned8* bytes),
       void (*fp3)(C1222AL* p, Unsigned16 event, Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4))
{
    p->logEventFunction = fp1;
    p->logEventAndBytesFunction = fp2;
    p->logEventAndULongsFunction = fp3;
}


void C1222AL_SetComEventLog(C1222AL* p, void (*comEventLogFunction)(Unsigned16 eventID, void* argument, Unsigned16 argLength))
{
    p->comEventLogFunction = comEventLogFunction;
}


void C1222AL_LogComEvent(C1222AL* p, Unsigned16 event)
{
    if ( p->comEventLogFunction != 0 )
    {
        (p->comEventLogFunction)(event, 0, 0);
    }
}


void C1222AL_LogComEventAndBytes(C1222AL* p, Unsigned16 event, void* argument, Unsigned16 length)
{
    if ( p->comEventLogFunction != 0 )
    {
        (p->comEventLogFunction)(event, argument, length);
    }
}




Boolean C1222AL_IsInterfaceAvailable(C1222AL* p)
{
    return C1222TL_IsInterfaceAvailable((C1222TL*)(p->pTransport));
}


static void commonInitCode(C1222AL* p, void* pTransport)
{
    
    C1222ALSegmenter_SetLinks(&p->messageSender.transmitSegmenter, p);
    C1222ALSegmenter_SetLinks(&p->messageReceiver.receiveSegmenter, p);

    C1222MessageReceiver_SetLinks(&p->messageReceiver, p);
    C1222MessageSender_SetLinks(&p->messageSender, (C1222TL*)pTransport, p);

    // the fragment size used by the message sender is refreshed on each sent message
    // so this value should not be used
    
    C1222ALSegmenter_SetFragmentSize(&p->messageSender.transmitSegmenter, MAX_ACSE_SEGMENTED_MESSAGE_LENGTH-8);
    
    // the receive segmenter should not care about the fragment size - it uses whatever it gets from the transport 
    // layer
    
    C1222ALSegmenter_SetFragmentSize(&p->messageReceiver.receiveSegmenter, 0); // MAX_ACSE_SEGMENTED_MESSAGE_LENGTH-8);
    
    p->status.registrationMaxRetryFactor = DEFAULT_MAX_REGISTRATION_RETRY_FACTOR;
    
    C1222AL_SetRegistrationRetryLimits(p, 1, 5, 1); // by default retry at a random time in 5 minutes 
    
    p->status.tryingToRegisterAfterCellSwitch = FALSE; 
    
    p->status.consecutiveSendFailureLimit = DEFAULT_SEND_FAILURE_LIMIT;
    p->status.consecutiveSendFailures = 0;  
}




void C1222AL_SetConsecutiveFailureLimit(C1222AL* p, Unsigned16 limit)
{
    p->status.consecutiveSendFailureLimit = limit;
}



void C1222AL_ReInitAfterRFLANFwDownload(C1222AL* p, void* pTransport)
{
    
    // most of status is initialized by the general zeroing
    
    
    p->status.lastPowerUp = C1222Misc_GetFreeRunningTimeInMS();
    
    memset(&p->messageSender, 0, sizeof(p->messageSender));       // sends message through comm module/device link
    memset(&p->messageReceiver, 0, sizeof(p->messageReceiver));   // receives message through comm module/device link
    
    
    p->status.state = ALS_INIT;
    p->status.deviceIdleState = ALIS_IDLE;
    
    p->status.timeToRegister = FALSE;

    p->status.isSynchronizedOrConnected = FALSE;
    p->status.isReceivedMessageComplete = FALSE;
    p->status.isReceivedMessageResponsePending = FALSE;
    p->status.haveRegistrationStatus = FALSE;
    p->status.haveInterfaceInfo = FALSE;
    p->status.configurationChanged = FALSE;
    p->status.inProcessProcedure = 0;
    p->status.linkControlRunning = FALSE;
    p->status.linkControlResponse = 0;

    p->status.sending = FALSE;
    p->status.sendingRegistrationRequest = FALSE;
    p->status.registrationInProgress = FALSE;
    p->status.sendStartTime = 0;
    
    // if the rflan comes back on the same cell it should not reregister, else it should - so the next line should be commented out
    //p->status.currentRFLanCellId = 0;
       
    p->status.timeToDeRegister = FALSE;
    p->status.deRegistrationInProgress = FALSE;
    
    p->status.interfaceEnabled = TRUE;
    
    // this will only be used in the register for interfacing to the rflan port
    
    p->status.isCommModule = FALSE;
    p->status.isRelay = FALSE;
    p->status.isLocalPort = FALSE;


    p->status.isReceivedMessageHandlingComplete = TRUE;
    
    p->status.inProcessProcedureState = C1222PS_IDLE;
    p->status.inProcessProcedureResponse = C1222RESPONSE_OK;


    // message sender and message receiver should have init functions
    
    C1222MessageSender_Init(&p->messageSender);
    C1222MessageReceiver_Init(&p->messageReceiver); 
    
    // it would be better to not erase the ap invoc ids and reg status
    
    
    commonInitCode(p, pTransport);
    
}





void C1222AL_Init(C1222AL* p, void* pTransport, Boolean isCommModule, Boolean isLocalPort, Boolean isRelay)
{   
    memset(p, 0, sizeof(C1222AL));
    
    p->pInterfaceName = "NoInterface";

    // most of status is initialized by the general zeroing
    
    p->status.state = ALS_INIT;
    
    p->status.interfaceEnabled = TRUE;

    p->status.isRegistered = isLocalPort;  // pretend its registered if its a local port

    p->status.isCommModule = isCommModule;
    p->status.isRelay = isRelay;
    p->status.isLocalPort = isLocalPort;

    p->status.isReceivedMessageHandlingComplete = TRUE;
    
    p->status.inProcessProcedureState = C1222PS_IDLE;
    p->status.inProcessProcedureResponse = C1222RESPONSE_OK;
    
    p->status.timeLastLostRegistration = C1222Misc_GetFreeRunningTimeInMS();


    // message sender and message receiver should have init functions
    
    C1222MessageSender_Init(&p->messageSender);
    C1222MessageReceiver_Init(&p->messageReceiver);

    p->pTransport = pTransport;

    p->logEventFunction = 0;
    p->logEventAndBytesFunction = 0;
    p->logEventAndULongsFunction = 0;
    p->comEventLogFunction = 0;


    memset(&p->transportConfig, 0, sizeof(C1222TransportConfig));
    memset(&p->transportStatus, 0, sizeof(C1222TransportStatus));
    memset(&p->registrationStatus, 0, sizeof(C1222RegistrationStatus));

    p->registrationStatus.masterRelayApTitle[0] = 0x0d; // relative aptitle, 0 length
    p->registrationStatus.nodeApTitle[0] = 0x0d; // ditto

    p->status.nextApInvocationId = rand();  // should be random
    
    C1222AL_SetClientStandardVersion(p, CSV_2008);  // by default will be 2008 until told otherwise
    C1222AL_SetClientSendACSEContextOption(p, FALSE);

    // counters are initialized by the general memset

#ifdef C1222_WANT_EVENTSUMMARY
    memset(p->eventCounts, 0, sizeof(p->eventCounts));
    memset(p->eventLastTime, 0, sizeof(p->eventLastTime));
    memset(p->eventCountCanary, 0xcd, sizeof(p->eventCountCanary));
#endif // C1222_WANT_EVENTSUMMARY

    // what is the 8 for?
    
    commonInitCode(p, pTransport);
    
    p->powerUpInProgress = FALSE;

}






void C1222AL_SetRegisteredStatus(C1222AL* p, Boolean isRegistered)
{
    Boolean wasRegistered = p->status.isRegistered;
    
    p->status.isRegistered = isRegistered;
    
    if ( isRegistered )
    {
        C1222AL_ResetRegistrationRetryFactor(p);
        
        p->status.tryingToRegisterAfterCellSwitch = FALSE;
    }
        
    if ( wasRegistered && !isRegistered )
        p->status.timeLastLostRegistration = C1222Misc_GetFreeRunningTimeInMS();
}



// this will be called from a different task than the c12.22 task in the register
// namely from the event manager task - should abort any pending reads or maybe other requests to the rflan

void C1222AL_PowerUpInProgress(C1222AL* p)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    // anything else needed?
    
    p->powerUpInProgress = TRUE;
    
    C1222TL_PowerUpInProgress(pTransport);
}



void C1222AL_NotePowerUp(C1222AL* p, Unsigned32 outageDurationInSeconds)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    Unsigned16 outageMinutes = (Unsigned16)((outageDurationInSeconds / 60) + 1);
    
    p->powerUpInProgress = FALSE;
    
    // if we were registered before and it has not been too long, assume we are still registered
    // if it has been too long since we registered better do it again
    
    p->status.lastPowerUp = C1222Misc_GetFreeRunningTimeInMS();
    p->status.powerUpDelay = 0;
    
    // set the last registration attempt to now so we will wait if we are not registered
    //p->status.lastRegistrationAttemptTime = p->status.lastPowerUp; // ie, now
    //C1222AL_SetRegistrationRetryTime(p);
    
    p->status.timeToRegister = FALSE;
    p->status.inProcessProcedureState = C1222PS_IDLE;
    p->status.isSynchronizedOrConnected = FALSE;
    p->status.lastUnSynchronizedTime = p->status.lastPowerUp;
    
    p->status.haveRevInfo = FALSE;
    
    
    if ( p->status.isRegistered && !p->status.timeToRegister )
    {
        if ( p->registrationStatus.regPeriodMinutes != 0 )
        {
            if ( outageMinutes > p->registrationStatus.regCountDownMinutes )
            {
                // need to register now
                p->registrationStatus.regCountDownMinutes = 0;
                
                //p->status.timeToRegister = TRUE;
            }
            else if ( (p->registrationStatus.regCountDownMinutes - outageMinutes) < 10 )
            {
                // need to register
                
                p->registrationStatus.regCountDownMinutes = 0;
                
                //p->status.timeToRegister = TRUE;
            }
            else
            {
                p->registrationStatus.regCountDownMinutes -= outageMinutes;
                p->status.waitingToRegister = TRUE;
                
                // correct for seconds after powerup
                // caution - this is in seconds not ms
                p->status.powerUpDelay = (Unsigned16)C1222Misc_GetRandomDelayTime(0, p->registrationStatus.regPowerUpDelaySeconds);
                                
            }
        }
        
        // else we are not supposed to refresh registration
    }
    
    C1222TL_NotePowerUp(pTransport, outageDurationInSeconds);
}







//extern void EnterLogCriticalSection(void);
//extern void LeaveLogCriticalSection(void);

void C1222AL_LogEventAndULongs(C1222AL* p, C1222EventId event,
        Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4)
{
	if ( (p->logEventAndULongsFunction != NULL) && p->logWanted )
    {
    	(p->logEventAndULongsFunction)(p, (Unsigned16)event, u1, u2, u3, u4);
    }
    
    C1222AL_IncrementEventCount(p, (Unsigned16)event);
}


void C1222AL_LogEventAndBytes(C1222AL* p, C1222EventId event, Unsigned8 n, Unsigned8* bytes)
{
	if ( (p->logEventAndBytesFunction != NULL) && p->logWanted )
    {
    	(p->logEventAndBytesFunction)(p, (Unsigned16)event, n, bytes);
    }
    
    C1222AL_IncrementEventCount(p, (Unsigned16)event);
}


void C1222AL_LogEvent(C1222AL* p, C1222EventId event)
{
	if ( (p->logEventFunction != NULL) && p->logWanted )
    {
    	(p->logEventFunction)(p, (Unsigned16)event);
    }
    
    C1222AL_IncrementEventCount(p, (Unsigned16)event);

}



struct DebugEventLogItem
{
    Unsigned8 eventid;
    Unsigned16 time;
} debugEventLog[80];

Unsigned8 debugEventIndex = 0;
Unsigned8 debugEventMode = 0;
Unsigned32 debugEventSendDelay = 13000;

void ResetDebugEventLog(void)
{
    debugEventIndex = 0;
    debugEventMode = 0;
    debugEventSendDelay = 13000;
    memset(debugEventLog,0,sizeof(debugEventLog));
}

void SetDebugEventSendMode(Unsigned32 delay)
{
    debugEventMode = 1;
    debugEventSendDelay = delay;
}

Unsigned8* C1222AL_GetDebugEventLog(void)
{
    return (Unsigned8*)(debugEventLog);
}

Unsigned16 C1222AL_GetDebugEventLogLength(void)
{
    return sizeof(debugEventLog);
}


static void LogDebugEvent(Unsigned16 eventid)
{
    if ( debugEventIndex < (ELEMENTS(debugEventLog) ) )
    {
        //debugEventIndex %= (Unsigned8)(ELEMENTS(debugEventLog));
        debugEventLog[debugEventIndex].eventid = (Unsigned8)eventid;
        debugEventLog[debugEventIndex].time = (Unsigned16)(C1222Misc_GetFreeRunningTimeInMS() & 0xFFFF);
        debugEventIndex++;
        
        if ( debugEventIndex == ELEMENTS(debugEventLog) )
        {
            if ( debugEventMode == 1 )
                debugEventIndex = 0;
        }
    }
    
}

void C1222AL_IncrementEventCount(C1222AL* p, Unsigned16 event)
{
#ifdef C1222_WANT_EVENTSUMMARY    
    if ( event<C1222EVENT_NUMBEROF )
    {
        p->eventCounts[event]++;    
        p->eventLastTime[event] = C1222Misc_GetFreeRunningTimeInMS();
    }
#endif // C1222_WANT_EVENTSUMMARY    
    
    if ( debugEventMode == 0 )
        LogDebugEvent(event);
    else if ( debugEventMode == 1 )
    {
        if ( (C1222Misc_GetFreeRunningTimeInMS()-p->status.sendStartTime) < debugEventSendDelay )
            LogDebugEvent(event);
    }
}





// these routines are called by C1222TL (transport layer)
C1222TransportConfig* C1222AL_GetConfiguration(C1222AL* p)
{
    return &(p->transportConfig);
}




Unsigned16 C1222AL_GetNextApInvocationId(C1222AL* p)
{
    Unsigned16 sequence;
    
    sequence = p->status.nextApInvocationId++;
    
    if ( sequence == 0 )
        sequence = p->status.nextApInvocationId++;    
    
    return sequence;
}








Boolean C1222AL_IsBusy(C1222AL* p)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    if ( p->status.state != ALS_IDLE )
        return TRUE;
        
    // if sending a segmented message or receiving a segmented message, say we are busy
    
    if ( !C1222MessageSender_IsMessageComplete(&p->messageSender) && C1222MessageSender_TriesLeft(&p->messageSender) )
        return TRUE;
        
    if ( !C1222MessageReceiver_IsMessageComplete(&p->messageReceiver) )
        return TRUE;
        
    if ( p->status.linkControlRunning )
        return TRUE;
    
    // if the transport layer is busy, say we are busy
        
    return C1222TL_IsBusy(pTransport);
}




Boolean C1222AL_IsBusyForRFLANAccess(C1222AL* p)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    if ( p->status.state != ALS_IDLE )
        return TRUE;
        
    // if sending a segmented message or receiving a segmented message, say we are busy
    
    if ( !C1222MessageSender_IsMessageComplete(&p->messageSender) && C1222MessageSender_TriesLeft(&p->messageSender) )
        return TRUE;
        
    // since this is an rflan table access, a partially received message will be aborted
    
    //if ( !C1222MessageReceiver_IsMessageComplete(&p->messageReceiver) )
    //    return TRUE;
        
    if ( p->status.linkControlRunning )
        return TRUE;
    
    // if the transport layer is busy, say we are busy
        
    return C1222TL_IsBusy(pTransport);
}




// this is used to send an error response when the epsem could not be parsed for some reason
// like a segmentation error or an encryption or authentication error

void C1222AL_MakeSimpleResponseMessage(C1222AL* p, ACSE_Message* pMsg, Unsigned8 response, Unsigned32 maxRequestOrResponse, ACSE_Message* pResponseMessage)
{
    //Unsigned8 nativeAddressLength;
    //Unsigned8* nativeAddress;
    
    // assume no encryption or authentication
    
    C1222ApTitle apTitle;
    //ACSE_Message responseMessage;
    Epsem responseEpsem;
    Unsigned16 sequence;
    Unsigned8 responseEpsemBuffer[7];
    Unsigned8 qualByte;
    Unsigned8 responseLength;
    
    C1222AL_LogEventAndBytes( p, C1222EVENT_AL_SIMPLE_RESPONSE, 1, &response);
    
    ACSE_Message_SetBuffer( pResponseMessage, p->tempBuffer, sizeof(p->tempBuffer) );
    
    // respond using the same standard version as the request
    
    ACSE_Message_SetStandardVersion( pResponseMessage, ACSE_Message_GetStandardVersion(pMsg) );
    
    if ( ACSE_Message_HasContext(pMsg) )
    {
        ACSE_Message_SetContext( pResponseMessage );   
    }
    
    if ( pMsg->standardVersion == CSV_MARCH2006 )
    {
        if ( ACSE_Message_GetCallingApTitle( pMsg, &apTitle) )
        {
            ACSE_Message_SetCalledApTitle( pResponseMessage, &apTitle );
        }
        
        C1222AL_ConstructCallingApTitle(p, &apTitle);
            
        ACSE_Message_SetCallingApTitle( pResponseMessage, &apTitle);
        
        if ( ACSE_Message_GetCallingAeQualifier( pMsg, &qualByte) )
        {
            // we support test and notification, not urgent
                        
            qualByte &= C1222QUAL_TEST | C1222QUAL_NOTIFICATION;
                        
            ACSE_Message_SetCallingAeQualifier( pResponseMessage, qualByte);
        }
        
            
        if ( ACSE_Message_GetCallingApInvocationId( pMsg, &sequence ) )
        {
            ACSE_Message_SetCalledApInvocationId( pResponseMessage, sequence );
        }
        
        sequence = C1222AL_GetNextApInvocationId(p);
        
        // Tables test bench errors out when this element is included
        // but it is currently required by the standard
        // TODO: add this back in when ttb supports it
        
        #ifndef AVOID_TTB_BUG_SKIPCALLINGSEQONRESPONSE
        ACSE_Message_SetCallingApInvocationId( pResponseMessage, sequence );
        #endif
    }
    else // 2008 version
    {
        // order for 2008 is A2 A4 A6 A7 A8 
        
        if ( ACSE_Message_GetCallingApTitle( pMsg, &apTitle) )
        {
            ACSE_Message_SetCalledApTitle( pResponseMessage, &apTitle );
        }

        if ( ACSE_Message_GetCallingApInvocationId( pMsg, &sequence ) )
        {
            ACSE_Message_SetCalledApInvocationId( pResponseMessage, sequence );
        }
        
        C1222AL_ConstructCallingApTitle(p, &apTitle);
            
        ACSE_Message_SetCallingApTitle( pResponseMessage, &apTitle);
        
        if ( ACSE_Message_GetCallingAeQualifier( pMsg, &qualByte) )
        {
            // we support test and notification, not urgent
                        
            qualByte &= C1222QUAL_TEST | C1222QUAL_NOTIFICATION;
                        
            ACSE_Message_SetCallingAeQualifier( pResponseMessage, qualByte);
        }
        
        sequence = C1222AL_GetNextApInvocationId(p);
        
        // Tables test bench errors out when this element is included
        // but it is currently required by the standard
        // TODO: add this back in when ttb supports it
        
        #ifndef AVOID_TTB_BUG_SKIPCALLINGSEQONRESPONSE
        ACSE_Message_SetCallingApInvocationId( pResponseMessage, sequence );
        #endif
    }
    
    responseEpsemBuffer[0] = 0x82;
    responseEpsemBuffer[1] = 1;
    responseEpsemBuffer[2] = response;
    responseLength = 2;
    if ( (response == C1222RESPONSE_RSTL) || (response == C1222RESPONSE_RQTL) )
    {
        responseEpsemBuffer[1] = 5;
        responseEpsemBuffer[3] = (Unsigned8)((maxRequestOrResponse >> 24) & 0xFF);
        responseEpsemBuffer[4] = (Unsigned8)((maxRequestOrResponse >> 16) & 0xFF);
        responseEpsemBuffer[5] = (Unsigned8)((maxRequestOrResponse >>  8) & 0xFF);
        responseEpsemBuffer[6] = (Unsigned8)((maxRequestOrResponse >>  0) & 0xFF);
        responseLength = 6;
    } 
    
    Epsem_Init( &responseEpsem, responseEpsemBuffer, (Unsigned16)sizeof(responseEpsemBuffer));
            
    responseEpsem.length = responseLength;            
     
    (void)ACSE_Message_SetUserInformation( pResponseMessage, &responseEpsem, FALSE);
    
    //if ( C1222AL_GetReceivedMessageNativeAddress(p, &nativeAddressLength, &nativeAddress) )  
    //    C1222AL_SendMessage(p, &responseMessage, nativeAddress, nativeAddressLength);   
}



void C1222AL_SimpleResponse(C1222AL* p, ACSE_Message* pMsg, Unsigned8 response, Unsigned32 maxRequestOrResponse)
{
    ACSE_Message responseMessage;
    Unsigned8 nativeAddressLength;
    Unsigned8* nativeAddress;

    C1222AL_LogComEventAndBytes(p, COMEVENT_C1222_SENT_SIMPLE_ERROR_RESPONSE, &response, 1);
    
    C1222AL_MakeSimpleResponseMessage(p, pMsg, response, maxRequestOrResponse, &responseMessage);
    
    p->lastMessageLog.responseBuiltPreEncryption = C1222Misc_GetFreeRunningTimeInMS();
    
    if ( C1222AL_GetReceivedMessageNativeAddress(p, &nativeAddressLength, &nativeAddress) ) 
    {
        // need to notify that the received message is complete
        C1222AL_NoteReceivedMessageHandlingIsComplete(p);
        
        p->lastMessageLog.responseReadyToSend = C1222Misc_GetFreeRunningTimeInMS();
         
        C1222AL_SendMessage(p, &responseMessage, nativeAddress, nativeAddressLength); 
    }
    else
        C1222AL_NoteReceivedMessageHandlingIsComplete(p);
}




void C1222AL_SimpleResponseWithoutReceivedMessageHandling(C1222AL* p, ACSE_Message* pMsg, Unsigned8 response, Unsigned32 maxRequestOrResponse)
{
    ACSE_Message responseMessage;
    Unsigned8 nativeAddressLength;
    Unsigned8* nativeAddress;

    C1222AL_LogComEventAndBytes(p, COMEVENT_C1222_SENT_SIMPLE_ERROR_RESPONSE, &response, 1);
    
    C1222AL_MakeSimpleResponseMessage(p, pMsg, response, maxRequestOrResponse, &responseMessage);
    
    p->lastMessageLog.responseBuiltPreEncryption = C1222Misc_GetFreeRunningTimeInMS();
    
    if ( C1222AL_GetReceivedMessageNativeAddress(p, &nativeAddressLength, &nativeAddress) ) 
    {
        // need to notify that the received message is complete
        //C1222AL_NoteReceivedMessageHandlingIsComplete(p);
        
        p->lastMessageLog.responseReadyToSend = C1222Misc_GetFreeRunningTimeInMS();
         
        C1222AL_SendMessage(p, &responseMessage, nativeAddress, nativeAddressLength); 
    }
    //else
    //    C1222AL_NoteReceivedMessageHandlingIsComplete(p);
}






#define MAX_REGISTRATION_REQUEST    80


ACSE_Message* C1222AL_MakeRegistrationMessage(C1222AL* p)
{
    Unsigned8 request[MAX_REGISTRATION_REQUEST]; 
    Unsigned16 index;
    Unsigned16 length;
    //Unsigned8 epsemBuffer[100]; // need to fix this
    Epsem epsem;
    Unsigned8 relayAddressLength;
    Unsigned8* relayAddress;
    ACSE_Message* pMsg;
    C1222ApTitle apTitle;
    
    // <register>	::=	27H <node-type> <connection-type> <device-class>
	//	                    <ap-title> <serial-number> <address-length>
	//                  	<native-address> <registration-period>
	//
	// max size = 1 + 1 + 1 + 4
	//            + 20 + 20 + 1 + 20 + 3 = 71
	
	// before we blow the request buffer, check that the registration request will fit
	
	length = (Unsigned16)(1 + 1 + 1 + C1222_DEVICE_CLASS_LENGTH);
	length += C1222Misc_GetUIDLength(p->transportConfig.nodeApTitle);
	length += C1222Misc_GetUIDLength(p->transportConfig.esn);
	length += (Unsigned16)(1 + p->transportConfig.nativeAddressLength);
	length += (Unsigned16)3;
	
	if ( length > MAX_REGISTRATION_REQUEST )
	{
	    // set some kind of error!
	    return 0;
	}
    
    request[0] = 0x27;
    // <node-type> ::= <byte>
    request[1] = p->transportConfig.nodeType;
    // <connection-type> ::= <byte>
    request[2] = 0;  // assume we are connectionless until told otherwise
    
    memcpy(&request[3], p->transportConfig.deviceClass, C1222_DEVICE_CLASS_LENGTH);
    
    index = 3 + C1222_DEVICE_CLASS_LENGTH;
    
    // node aptitle
    
    length = C1222Misc_GetUIDLength(p->transportConfig.nodeApTitle);
    
    if ( length == 0 ) // normal
    {
        request[index++] = 0x0d;  // relative aptitle
        request[index++] = 0x00;  // zero length - is this how to do it?   
    }
    else // max length = 20
    {
        memcpy(&request[index], p->transportConfig.nodeApTitle, length);
        index += length;
    }
    
    // serial number
    
    length = C1222Misc_GetUIDLength(p->transportConfig.esn);
    
    if ( length == 0 ) // not good - we should have a serial number
    {
        request[index++] = 0x0d;  // relative aptitle
        request[index++] = 0x00;  // zero length - is this how to do it?   
    }
    else // max length = 20
    {
        memcpy(&request[index], p->transportConfig.esn, length);
        index += length;
    }
    
    // native address length and address (length is a byte not encoded ber)
    
    length = p->transportConfig.nativeAddressLength;
    request[index++] = length;
    memcpy(&request[index], p->transportConfig.nativeAddress, length);
    index += length;
    
    // suggested maximum re-registration period
    // software treats a suggested re-registration period of 0 as meaning the meter does not
    // support re-registration - so to be compatible I will set this to a large number.
    
    request[index++] = 0xFF;
    request[index++] = 0xFF;
    request[index++] = 0xFF;
    
    p->tempBuffer[0] = 0x90; // response required, device class included
    memcpy( &p->tempBuffer[1], p->transportConfig.deviceClass, 4);
    
    Epsem_Init(&epsem, p->tempBuffer, sizeof(p->tempBuffer));
    
    Epsem_AddRequestOrResponse(&epsem, request, index);

    relayAddressLength = p->registrationStatus.relayAddressLength;
    relayAddress = p->registrationStatus.relayNativeAddress;
    
    // now need to build acse message
    // calling aptitle is my aptitle (0 length until I get one)
    // called aptitle is the aptitle of the master relay (0 length if not yet assigned)
    // cannot be encrypted or authenticated since comm module does not know the keys
    
    pMsg = StartNewUnsegmentedMessage(p);
    
    pMsg->standardVersion = p->status.clientC1222StandardVersion;

    SetMessageDestination(p, relayAddressLength, relayAddress);
    
    // set context if desired
    
    if ( p->status.clientSendACSEContext )
    {
        ACSE_Message_SetContext(pMsg);
    }
    
    // need called aptitle if not 0 length
    // need calling aptitle if not 0 length
    // need calling apinvocation id
    // need user information ( not encrypted )
    
    length = C1222Misc_GetUIDLength(p->transportConfig.masterRelayApTitle);
    if ( length > 2 )
    {
        C1222ApTitle_Construct(&apTitle, p->transportConfig.masterRelayApTitle, C1222_APTITLE_LENGTH);
        apTitle.length = length;
        
        ACSE_Message_SetCalledApTitle(pMsg, &apTitle);
    }

    length = C1222Misc_GetUIDLength(p->transportConfig.nodeApTitle);
    if ( length > 2 )
    {
        C1222ApTitle_Construct(&apTitle, p->transportConfig.nodeApTitle, C1222_APTITLE_LENGTH);
        apTitle.length = length;
        
        ACSE_Message_SetCallingApTitle(pMsg, &apTitle);
    }
    
    p->status.lastRegistrationRequestApInvocationId = p->status.registrationRequestApInvocationId;
    p->status.registrationRequestApInvocationId = C1222AL_GetNextApInvocationId(p);
    
    ACSE_Message_SetCallingApInvocationId(pMsg, p->status.registrationRequestApInvocationId);
    
    // assume there is enough room in our buffers for a registration request
    (void)ACSE_Message_SetUserInformation(pMsg, &epsem, FALSE); // no mac needed
    
    EndNewUnsegmentedMessage(p);
    
    
    // the message is ready - let the app user have it
    
    C1222AL_IncrementStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_REGISTRATIONS_SENT);    
    
    //C1222AL_NoteReceivedCompleteMessage(p); 
    
    //p->status.sendingRegistrationRequest = TRUE;
    
    return pMsg;
}






ACSE_Message* C1222AL_MakeDeRegistrationMessage(C1222AL* p)
{
    Unsigned8 request[1+C1222_APTITLE_LENGTH]; 
    //Unsigned16 index;
    Unsigned16 length;
    //Unsigned8 epsemBuffer[100]; // need to fix this
    Epsem epsem;
    Unsigned8 relayAddressLength;
    Unsigned8* relayAddress;
    ACSE_Message* pMsg;
    C1222ApTitle apTitle;
    
    // <deregister>	::=	24H <ap-title>
	//
	// max size = 1 + 20
	
    
    request[0] = 0x24;
    
    // node aptitle
    
    length = C1222Misc_GetUIDLength(p->registrationStatus.nodeApTitle);
    
    memcpy(&request[1], p->registrationStatus.nodeApTitle, length);
       
    p->tempBuffer[0] = 0x90; // response required, device class included
    memcpy( &p->tempBuffer[1], p->transportConfig.deviceClass, 4);
    
    Epsem_Init(&epsem, p->tempBuffer, sizeof(p->tempBuffer));
    
    Epsem_AddRequestOrResponse(&epsem, request, (Unsigned16)(length+1));

    relayAddressLength = p->registrationStatus.relayAddressLength;
    relayAddress = p->registrationStatus.relayNativeAddress;
    
    // now need to build acse message
    // calling aptitle is my aptitle (0 length until I get one)
    // called aptitle is the aptitle of the master relay (0 length if not yet assigned)
    // cannot be encrypted or authenticated since comm module does not know the keys
    
    pMsg = StartNewUnsegmentedMessage(p);
    
    pMsg->standardVersion = p->status.clientC1222StandardVersion;

    SetMessageDestination(p, relayAddressLength, relayAddress);
    
    // set context if desired
    
    if ( p->status.clientSendACSEContext )
    {
        ACSE_Message_SetContext(pMsg);
    }
    
    
    // need called aptitle if not 0 length
    // need calling aptitle if not 0 length
    // need calling apinvocation id
    // need user information ( not encrypted )
    
    length = C1222Misc_GetUIDLength(p->registrationStatus.masterRelayApTitle);
    if ( length > 2 )
    {
        C1222ApTitle_Construct(&apTitle, p->registrationStatus.masterRelayApTitle, C1222_APTITLE_LENGTH);
        apTitle.length = length;
        
        ACSE_Message_SetCalledApTitle(pMsg, &apTitle);
    }

    length = C1222Misc_GetUIDLength(p->registrationStatus.nodeApTitle);
    if ( length > 2 )
    {
        C1222ApTitle_Construct(&apTitle, p->registrationStatus.nodeApTitle, C1222_APTITLE_LENGTH);
        apTitle.length = length;
        
        ACSE_Message_SetCallingApTitle(pMsg, &apTitle);
    }
    
    p->status.deRegistrationRequestApInvocationId = C1222AL_GetNextApInvocationId(p);
    
    ACSE_Message_SetCallingApInvocationId(pMsg, p->status.deRegistrationRequestApInvocationId);
    
    // assume there is enough room in our buffers for a registration request
    (void)ACSE_Message_SetUserInformation(pMsg, &epsem, FALSE); // no mac needed
    
    EndNewUnsegmentedMessage(p);
    
    
    // the message is ready - let the app user have it
    
    C1222AL_IncrementStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_DEREGISTRATIONS_REQUESTED);    
    
    //C1222AL_NoteReceivedCompleteMessage(p); 
    
    //p->status.sendingRegistrationRequest = TRUE;
    
    return pMsg;
}











// if the comm module has messages to send, it could always send them to the c12.22 device
// which could encrypt/authenticate before sending on its way, back through the comm module perhaps

// similarly on message rx, the message could be encrypted/authenticated if it went to the
// c12.22 device first, and then was sent back to the comm module after being decrypted or 
// authenticated.  If the message fails encryption/authentication, it would be dropped by or
// responded by the c12.22 device.

// to do this, the c12.22 device would need to
//   - on receiving a complete message, check the called aptitle
//      if the called aptitle is not a branch of the c12.22 device and it came from the comm module, 
//         encrypt/authenticate the message,
//         send the message on to the called aptitle
//             BUT - this conflicts with using no called aptitle as addressed to the device!!!!


Boolean C1222AL_HandleRegistrationResponse(C1222AL* p, ACSE_Message* pMsg, Unsigned8 addressLength, 
                       Unsigned8* nativeAddress)
{
    //Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned8* buffer;
    Epsem epsem;
    Unsigned8 registrationResponse;
    Unsigned8 regApTitleLength;
    C1222RegistrationStatus* pRegStatus;
    Unsigned32 regPeriod;
    C1222ApTitle apTitle;
    Unsigned16 length;
    Unsigned8* data;
    
    // this appears to be a registration response in that the apInvocation id matches what we
    // sent in the registration request
    
    // find the app-data
    
    if ( ACSE_Message_GetApData(pMsg, &data, &tagLength) )
    {
        // again this cannot be encrypted or authenticated since registration is supposed to be 
        // handled in the comm module and encryption is not handled in the comm module (but see above)
        // for the moment we will assume no encryption and no authentication on registration req/resp
        
        Epsem_Init(&epsem, data, tagLength);
        
        if ( Epsem_Validate(&epsem) )
        {
            p->status.registrationFailureReason = MFC_NO_REQUEST_OR_RESPONSE_IN_EPSEM; // in case there is no request/response
            
            while( Epsem_GetNextRequestOrResponse( &epsem, &buffer, &length) )
            {
                // there should only be 1 response
                // the reg response should contain: 
                //      <ok> <reg-ap-title> <reg-delay> <reg-period> <reg-info>
                
                registrationResponse = buffer[0];
                
                if ( registrationResponse == C1222RESPONSE_OK )
                {
                    if ( length >= 9 )  // a valid reg response must be at least 9 bytes
                    {
                        regApTitleLength = C1222Misc_GetUIDLength(&buffer[1]);
                        
                        if ( (regApTitleLength <= 2) || (regApTitleLength > C1222_APTITLE_LENGTH) )
                        {
                            // invalid registered aptitle
                            C1222AL_LogEvent(p, C1222EVENT_AL_REGISTEREDAPTITLEINVALID);
                            
                            p->status.registrationFailureReason = MFC_REG_DEREG_APTITLE_INVALID;
                        }
                        
                        else if ( length == (7+regApTitleLength) )
                        {
                            // yes this is a registration response
                            // fill up the reg info
                            
                            pRegStatus = &p->registrationStatus;
                            
                            if ( addressLength > C1222_NATIVE_ADDRESS_LENGTH )
                                addressLength = C1222_NATIVE_ADDRESS_LENGTH;
                                
                            pRegStatus->relayAddressLength = addressLength;
                            
                            memcpy(pRegStatus->relayNativeAddress, nativeAddress, addressLength);
                            
                            memset(pRegStatus->nodeApTitle, 0, C1222_APTITLE_LENGTH);
                            memcpy(pRegStatus->nodeApTitle, &buffer[1], regApTitleLength);
                            
                            C1222ApTitle_ConvertACSERelativeToTableRelative(pRegStatus->nodeApTitle);
                            
                            buffer += 1 + regApTitleLength;
                            
                            pRegStatus->regPowerUpDelaySeconds = (Unsigned16)((buffer[0]<<8) | buffer[1]);
                            
                            buffer += 2;
                            
                            regPeriod = (Unsigned32)( (( (Unsigned32)buffer[0] )<<16) | 
                                                      (( (Unsigned32)buffer[1] )<<8)  | 
                                                      (buffer[2]) );
                            
                            pRegStatus->regPeriodMinutes = (Unsigned16)(regPeriod / 60);
                            pRegStatus->regCountDownMinutes = pRegStatus->regPeriodMinutes;
                            
                            buffer += 3;
                            
                            p->status.directMessagingAvailable = (Boolean)((buffer[0] & 0x01) ? TRUE : FALSE);
                            
                            memset(pRegStatus->masterRelayApTitle, 0, C1222_APTITLE_LENGTH);
                            
                            pRegStatus->masterRelayApTitle[0] = 0x0d;  // relative aptitle, 0 length
                            
                            if ( ACSE_Message_GetCallingApTitle(pMsg, &apTitle) )
                            { 
                                if ( apTitle.length <= C1222_APTITLE_LENGTH )
                                {   
                                    memcpy(pRegStatus->masterRelayApTitle, apTitle.buffer, apTitle.length);
                                    C1222ApTitle_ConvertACSERelativeToTableRelative(pRegStatus->masterRelayApTitle);
                                }
                                    
                                // otherwise the strucure would be wrong, so just don't store it
                            }  // end if calling aptitle found
                            
                            p->status.registrationFailureReason = MFC_OK;
                            
                            C1222AL_LogRegistrationResult(p, p->status.registrationFailureReason);
                            
                            return TRUE;
                        } // end if response length makes sense for a registration response
                        else
                            p->status.registrationFailureReason = MFC_REG_DEREG_RESPONSE_LENGTH_INVALID;
                            
                    } // end if the response could be a registration response based on length
                    else
                        p->status.registrationFailureReason = MFC_REG_DEREG_RESPONSE_LENGTH_INVALID;
                } // end if the response was <ok>
                else
                {
                    p->status.registrationFailureReason = MFC_REG_DEREG_RESPONSE_NOT_OK;
                    p->status.registrationResponse = registrationResponse;
                }
            } // end while more requests or responses
        } // end if the epsem validated 
        else
            p->status.registrationFailureReason = MFC_REG_DEREG_EPSEM_INVALID;
             
    } // end if found the information element
    else
        p->status.registrationFailureReason = MFC_REG_DEREG_INFORMATION_ELEMENT_MISSING;
        
    C1222AL_LogRegistrationResult(p, p->status.registrationFailureReason);
    
    return FALSE;
}





void C1222AL_HandleDeRegisterRequest(C1222AL* p)
{
    // need a deregister response ?? this needs to be delayed
        
    C1222AL_RestartNormalRegistrationProcess(p);
    
    p->status.lastSuccessfulDeRegistrationTime = C1222Misc_GetFreeRunningTimeInMS();
    p->counters.deRegistrationSuccessCount++;
                                    
    p->status.deRegistrationFailureReason = MFC_OK;
    
    p->status.deRegistrationInProgress = FALSE;

    C1222AL_IncrementStatistic(p, C1222_STATISTIC_DEREGISTRATION_SUCCEEDED);

    C1222AL_LogEvent(p, C1222EVENT_AL_DEREGISTERED);
    
}


Unsigned8 C1222AL_NoteDisconnectRequest(C1222AL* p)
{
    // this should go offline - so is just like an interface disable
    
    #ifdef C1222_INCLUDE_DEVICE
    return C1222AL_DeviceSendLinkControlNow(p, C1222TL_INTF_CTRL_DISABLEINTERFACE);
    #else
    (void)p;
    return C1222RESPONSE_ERR;
    #endif
}


void C1222AL_NoteDeRegisterRequest(C1222AL* p)
{
    // todo 
    // need to change state after the response is sent I think
    C1222AL_HandleDeRegisterRequest(p);
}


void C1222AL_LogDeRegistrationResult(C1222AL* p, C1222MessageFailureCode reason)
{
    C1222AL_LogComEventAndBytes(p, COMEVENT_C1222_DEREGISTRATION_RESULT, &reason, sizeof(reason));
}


void C1222AL_LogRegistrationResult(C1222AL* p, C1222MessageFailureCode reason)
{
    C1222AL_LogComEventAndBytes(p, COMEVENT_C1222_REGISTRATION_RESULT, &reason, sizeof(reason));
}

void C1222AL_LogInvalidMessage(C1222AL* p, C1222MessageValidationResult reason, 
            C1222MessageValidationResult reason2, ACSE_Message* pMsg)
{
    Unsigned8 buffer[24];
    C1222ApTitle aptitle;
    Unsigned16 sequence;
    
    memset(buffer, 0, sizeof(buffer));
    
    if ( ACSE_Message_GetCallingApTitle(pMsg, &aptitle) )
        memcpy(buffer, aptitle.buffer, aptitle.length);
        
    C1222ApTitle_ConvertACSERelativeToTableRelative(buffer);
        
    if ( ACSE_Message_GetCallingApInvocationId(pMsg, &sequence) )
        *(Unsigned16*)&buffer[20] = sequence;

    //C1222MessageFailureCode code = (C1222MessageFailureCode)(reason + C1222_VALIDATION_RESULT_OFFSET);
    
    buffer[22] = (Unsigned8)reason;
    buffer[23] = (Unsigned8)reason2;
    
    C1222AL_LogComEventAndBytes(p, COMEVENT_C1222_RECEIVED_INVALID_MESSAGE, buffer, sizeof(buffer));
}


Boolean C1222AL_HandleDeRegistrationResponse(C1222AL* p, ACSE_Message* pMsg )
{
    //Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned8* buffer;
    Epsem epsem;
    Unsigned8 deRegistrationResponse;
    Unsigned8 regApTitleLength;
    C1222RegistrationStatus* pRegStatus;
    C1222ApTitle apTitle;
    Unsigned16 length;
    C1222ApTitle myApTitle;
    int difference;
    Unsigned8* data;
    C1222StandardVersion stdVersion; 
    
    // for March 2006 there is an aptitle sent with the deregistration response
    // for 2008 there is no aptitle - only the response code
    
    stdVersion = ACSE_Message_GetStandardVersion(pMsg);
    
    // get the pointer to the reg status in case we need it below (fix for severe bug - was not initialized in 2008 handling)
    pRegStatus = &p->registrationStatus;
    
    // this appears to be a deregistration response in that the apInvocation id matches what we
    // sent in the deregistration request
    
    // find the app-data
    
    if ( ACSE_Message_GetApData(pMsg, &data, &tagLength) )
    {
        // again this cannot be encrypted or authenticated since registration is supposed to be 
        // handled in the comm module and encryption is not handled in the comm module (but see above)
        // for the moment we will assume no encryption and no authentication on registration req/resp
        
        Epsem_Init(&epsem, data, tagLength);
        
        if ( Epsem_Validate(&epsem) )
        {
            p->status.deRegistrationFailureReason = MFC_NO_REQUEST_OR_RESPONSE_IN_EPSEM; // in case there is no request/response
            
            while( Epsem_GetNextRequestOrResponse( &epsem, &buffer, &length) )
            {
                // there should only be 1 response
                // the reg response should contain: 
                //      <ok> <reg-ap-title> <reg-delay> <reg-period> <reg-info>
                
                deRegistrationResponse = buffer[0];
                
                if ( deRegistrationResponse == C1222RESPONSE_OK )
                {
                    if ( stdVersion != CSV_MARCH2006 )  // 2008 and presumably later
                    {
                        p->status.deRegistrationFailureReason = MFC_OK;
                                    
                        C1222AL_LogDeRegistrationResult(p, p->status.deRegistrationFailureReason);
                                    
                        memset(pRegStatus->nodeApTitle, 0, C1222_APTITLE_LENGTH);
                                    
                        return TRUE;                        
                    }
                    
                    if ( length >= 4 )  // a valid reg response must be at least 9 bytes
                    {
                        regApTitleLength = C1222Misc_GetUIDLength(&buffer[1]);
                        
                        if ( (regApTitleLength <= 2) || (regApTitleLength > C1222_APTITLE_LENGTH) )
                        {
                            // invalid registered aptitle
                            C1222AL_LogEvent(p, C1222EVENT_AL_DEREGISTEREDAPTITLEINVALID);
                            
                            p->status.deRegistrationFailureReason = MFC_REG_DEREG_APTITLE_INVALID;
                        }
                        
                        else if ( length == (1+regApTitleLength) )
                        {
                            // yes this is a deregistration response
                            // fill up the reg info
                            
                            // check that the deregistered aptitle is me
                            
                            
                            C1222ApTitle_Construct(&apTitle, &buffer[1], (Unsigned8)(length-1));
                            apTitle.length = C1222ApTitle_GetLength(&apTitle);
                                                        
                            C1222ApTitle_Construct(&myApTitle, (Unsigned8*)&pRegStatus->nodeApTitle, C1222_APTITLE_LENGTH);
                            myApTitle.length = C1222ApTitle_GetLength(&apTitle);
                            
                            if ( C1222ApTitle_Compare(&apTitle, &myApTitle, &difference) )
                            {
                                if ( difference == 0 )
                                {
                                    // deregistration now is like soft deregistration before
                                    // go back to initial registration algorithm                                    
                                    
                                    p->status.deRegistrationFailureReason = MFC_OK;
                                    
                                    C1222AL_LogDeRegistrationResult(p, p->status.deRegistrationFailureReason);
                                    
                                    memset(pRegStatus->nodeApTitle, 0, C1222_APTITLE_LENGTH);
                                    
                                    return TRUE;
                                }
                                else
                                    p->status.deRegistrationFailureReason = MFC_DEREG_APTITLE_MISMATCH; // better search
                            }
                            else
                                p->status.deRegistrationFailureReason = MFC_REG_DEREG_APTITLE_COMPARE_FAILED;
                   
                        } // end if response length makes sense for a registration response
                        else
                            p->status.deRegistrationFailureReason = MFC_REG_DEREG_RESPONSE_LENGTH_INVALID;
                            
                    } // end if the response could be a registration response based on length
                    else
                        p->status.deRegistrationFailureReason = MFC_REG_DEREG_RESPONSE_LENGTH_INVALID;
                } // end if the response was <ok>
                else
                    p->status.deRegistrationFailureReason = MFC_REG_DEREG_RESPONSE_NOT_OK;
            } // end while more requests or responses
        } // end if the epsem validated 
        else
            p->status.deRegistrationFailureReason = MFC_REG_DEREG_EPSEM_INVALID;
             
    } // end if found the information element
    else
        p->status.deRegistrationFailureReason = MFC_REG_DEREG_INFORMATION_ELEMENT_MISSING;
        
    C1222AL_LogDeRegistrationResult(p, p->status.deRegistrationFailureReason);
    
    return FALSE;
}




void C1222AL_RestartNormalRegistrationProcess(C1222AL* p)
{
    C1222AL_SetRegisteredStatus(p, FALSE);
    
    p->status.configurationChanged = TRUE; // force rflan to get config
    
    C1222TL_InhibitOptionalCommunication((C1222TL*)(&p->pTransport));
            
    p->status.lastRegistrationAttemptTime = C1222Misc_GetFreeRunningTimeInMS();
    
    C1222AL_ResetRegistrationRetryFactor(p);
    C1222AL_SetRegistrationRetryTime(p); 
    
    p->status.state = ALS_IDLE;
    p->status.haveRegistrationStatus = FALSE;   
}


#ifdef C1222_INCLUDE_DEVICE
Boolean C1222AL_CheckForRegistrationResponseDuringRegistration(C1222AL* p, ACSE_Message* pMsg, 
                      Unsigned8 addressLength, Unsigned8* nativeAddress)
{
    Unsigned16 apInvocationId;
    
    // until we are registered, we probably don't want to accept any messages
    // for one thing we probably don't have an aptitle
    
    // need to determine if this message is a registration response
    // can do that by comparing the called ap invocation id to the saved registration ap invocation id
    
    if ( ACSE_Message_GetCalledApInvocationId(pMsg, &apInvocationId) )
    {
        if ( (apInvocationId == p->status.registrationRequestApInvocationId) ||
             (apInvocationId == p->status.lastRegistrationRequestApInvocationId) )
        { 
            if ( C1222AL_HandleRegistrationResponse(p, pMsg, addressLength, nativeAddress) )
            {
                p->status.lastSuccessfulRegistrationTime = C1222Misc_GetFreeRunningTimeInMS();
                p->counters.registrationSuccessCount++;
                C1222AL_SetRegisteredStatus(p, TRUE);
                
                p->status.registrationRequestApInvocationId = 0;
                p->status.lastRegistrationRequestApInvocationId = 0;
                
                p->status.haveRegistrationStatus = TRUE;
                
                if ( p->status.registrationInProgress )
                {
                    C1222AL_DeviceNoteRegistrationAttemptComplete(p, C1222RESPONSE_OK);
                    p->status.registrationInProgress = FALSE;
                }
                                
                p->status.configurationChanged = TRUE; // to get the registered aptitle to the comm module
                C1222TL_InhibitOptionalCommunication((C1222TL*)(p->pTransport));
                p->status.state = ALS_IDLE;
                C1222AL_IncrementStatistic(p, C1222_STATISTIC_REGISTRATION_SUCCEEDED);
                
                C1222AL_LogEvent(p, C1222EVENT_AL_REGISTERED);
            }
            else
            {
                C1222AL_IncrementStatistic(p, C1222_STATISTIC_NUMBER_OF_REGISTRATIONS_FAILED);
                
                if ( p->status.deRegistrationInProgress )
                    C1222AL_DeviceNoteRegistrationAttemptComplete(p, C1222RESPONSE_ERR);
                C1222AL_LogEvent(p, C1222EVENT_AL_REG_RESPONSE_INVALID);
            }
            
            return TRUE; // it was a registration response and we handled it so it does not
                         // need to be sent down the stack
        }
        
        if ( p->status.registrationInProgress )
            C1222AL_LogEvent(p, C1222EVENT_AL_OTHER_MESSAGE_WAITING_FOR_REG_RESPONSE);
        
        // if not a registration response, I'm going to ignore it
    }
    
    // if no apInvocation id, cannot be a registration response - or I can't tell, so ignore it
    return FALSE;  // the message was not consumed here - send it on to the stack
}






Boolean C1222AL_CheckForDeRegistrationResponseDuringDeRegistration(C1222AL* p, ACSE_Message* pMsg)
{
    Unsigned16 apInvocationId;
    
    // until we are registered, we probably don't want to accept any messages
    // for one thing we probably don't have an aptitle
    
    // need to determine if this message is a registration response
    // can do that by comparing the called ap invocation id to the saved registration ap invocation id
    
    if ( ACSE_Message_GetCalledApInvocationId(pMsg, &apInvocationId) )
    {
        if ( apInvocationId == p->status.deRegistrationRequestApInvocationId )
        { 
            if ( C1222AL_HandleDeRegistrationResponse(p, pMsg) )
            {
                p->status.lastSuccessfulDeRegistrationTime = C1222Misc_GetFreeRunningTimeInMS();
                p->counters.deRegistrationSuccessCount++;
                
                C1222AL_RestartNormalRegistrationProcess(p);
                
                p->status.deRegistrationRequestApInvocationId = 0;
                
                if ( p->status.deRegistrationInProgress )
                {
                    C1222AL_DeviceNoteDeRegistrationAttemptComplete(p, C1222RESPONSE_OK);
                    p->status.deRegistrationInProgress = FALSE;
                }
                
                C1222AL_IncrementStatistic(p, C1222_STATISTIC_DEREGISTRATION_SUCCEEDED);
                
                C1222AL_LogEvent(p, C1222EVENT_AL_DEREGISTERED);
            }
            else
            {  
                if ( p->status.deRegistrationInProgress )              
                    C1222AL_DeviceNoteDeRegistrationAttemptComplete(p, C1222RESPONSE_ERR);
                C1222AL_LogEvent(p, C1222EVENT_AL_DEREG_RESPONSE_INVALID);
            }
            
            return TRUE; // it was a registration response and we handled it so it does not
                         // need to be sent down the stack
        }
        
        if ( p->status.deRegistrationInProgress )
            C1222AL_LogEvent(p, C1222EVENT_AL_OTHER_MESSAGE_WAITING_FOR_DEREG_RESPONSE);
        
        // if not a registration response, I'm going to ignore it
    }
    
    // if no apInvocation id, cannot be a registration response - or I can't tell, so ignore it
    return FALSE;  // the message was not consumed here - send it on to the stack
}                      


                     

#endif




void C1222AL_ClearCounters(C1222AL* p)
{
    memset(&p->counters, 0, sizeof(p->counters));
}

void C1222AL_ClearStatusDiagnostics(C1222AL* p)
{   
    p->status.maxCompleteMessageProcessTime = 0;
    p->status.avgCompleteMessageProcessTime = 0;
    p->status.totalCompleteMessageProcessTime = 0;
    
    p->status.maxMessageSendTime = 0;
    p->status.avgMessageSendTime = 0;
    p->status.totalMessageSendTime = 0;
    
}
