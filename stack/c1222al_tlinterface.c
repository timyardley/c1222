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
#include "acsemsg.h"



extern void CommModuleProcessReceivedMessage(C1222AL* p, ACSE_Message* pAssembledMessage);
extern void C1222EnterCriticalRegion(void);
extern void C1222LeaveCriticalRegion(void);


// NOTE: we may want to implement a callback for notifying the higher level entity that a message is available to
// avoid delay

void MeterProcessReceivedMessage(C1222AL* p, ACSE_Message* pMsg, Unsigned8 nativeAddressLength, Unsigned8* nativeAddress)
{
#ifdef C1222_INCLUDE_DEVICE     
    C1222MessageLogEntry logEntry;
#endif    
      
    // I think all of these will be for the c1222stack or above, so set the flag so they can get the message
    
    // the message is already stored in the message receiver
    
    // the native address is already stored in the message receiver

    (void)pMsg;
    
    (void)nativeAddressLength;
    (void)nativeAddress;
    
    // the meter has no use for notifications - if this is a notification, ignore it
    
    // might want to check this later - maybe we should respond ok to notifications
    
    //if ( ACSE_Message_IsNotification(pMsg) )
    //    return;
    
    // registration and deregistration responses do not need to be encrypted (we probably don't have keys yet)
    
    #ifdef C1222_INCLUDE_DEVICE
    if ( ( p->status.registrationInProgress || (p->status.registrationRequestApInvocationId != 0) ) &&
         C1222AL_CheckForRegistrationResponseDuringRegistration(p, pMsg, nativeAddressLength, nativeAddress) )
    {
        C1222AL_MakeMessageLogEntry(p, pMsg, &logEntry, FALSE, C1222_MSGTYPE_REGISTER_RESPONSE);  // this is a receiving log entry
        C1222AL_LogComEventAndBytes(p, COMEVENT_RECEIVED_MESSAGE_FROM, &logEntry, sizeof(logEntry));
        
        C1222MessageReceiver_NoteMessageHandlingComplete(&p->messageReceiver);
        C1222TL_SendPendingSendMessageOKResponse((C1222TL*)(p->pTransport));
        return;  // the response was consumed and is not for the device application 
    }
    else if ( ( p->status.deRegistrationInProgress || (p->status.deRegistrationRequestApInvocationId != 0) ) &&
         C1222AL_CheckForDeRegistrationResponseDuringDeRegistration(p, pMsg) )
    {
        C1222AL_MakeMessageLogEntry(p, pMsg, &logEntry, FALSE, C1222_MSGTYPE_DEREGISTER_RESPONSE);  // this is a receiving log entry
        C1222AL_LogComEventAndBytes(p, COMEVENT_RECEIVED_MESSAGE_FROM, &logEntry, sizeof(logEntry));

        C1222MessageReceiver_NoteMessageHandlingComplete(&p->messageReceiver);
        C1222TL_SendPendingSendMessageOKResponse((C1222TL*)(p->pTransport));
        return;  // the response was consumed and is not for the device application 
    }
    else if ( !ACSE_Message_IsEncryptedOrAuthenticated(pMsg) && 
              p->status.requireEncryptionOrAuthenticationIfKeysExist && 
              (C1222AL_GetNumberOfKeys(p) > 0) )
    {
        // return SME response
        // send an ok response for the pending transport layer message since the message
        // got here ok
        
        C1222AL_MakeMessageLogEntry(p, pMsg, &logEntry, FALSE, C1222_MSGTYPE_RECEIVED_INVALID_MSG);  // this is a receiving log entry
        C1222AL_LogComEventAndBytes(p, COMEVENT_RECEIVED_MESSAGE_FROM, &logEntry, sizeof(logEntry));        
        
        C1222AL_LogInvalidMessage(p, CMVR_MESSAGE_MUST_NOT_BE_PLAIN_TEXT, CMVR_OK, pMsg);
        
        //C1222MessageReceiver_NoteMessageHandlingComplete(&p->messageReceiver);
        C1222TL_SendPendingSendMessageOKResponse((C1222TL*)(p->pTransport));
        
        C1222AL_SimpleResponseWithoutReceivedMessageHandling(p, pMsg, C1222RESPONSE_SME, 0);
        C1222MessageReceiver_NoteMessageHandlingComplete(&p->messageReceiver);
        return;        
    }


    
    #endif
    
    // come and get it
    
    C1222AL_NoteReceivedCompleteMessage(p);
}



static void NoteMessageSendComplete(C1222AL* p)
{   
    Unsigned32 elapsedTime = C1222Misc_GetFreeRunningTimeInMS() - p->status.sendStartTime;
    
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



static void ContinueMessageSend(C1222AL* p)
{
    C1222MessageSender* pSender = (C1222MessageSender*)(&p->messageSender); 
    ACSE_Message* pMsg; 
    Unsigned16 sequence;  

    if ( ! C1222MessageSender_IsMessageComplete(pSender) )
    {
        p->status.lastRequestTime = C1222Misc_GetFreeRunningTimeInMS();
        
        if ( !C1222MessageSender_ContinueSend(pSender) )
            p->counters.sendContinuationFailed++;
    }
    else
    {
        NoteMessageSendComplete(p);
        
        pMsg = C1222MessageSender_GetWorkingMessage(&p->messageSender);
        
        if ( !ACSE_Message_GetCallingApInvocationId(pMsg, &sequence) )
            sequence = 0;

        // if this message is for the comm module, don't log it
        if ( !C1222AL_IsMsgForCommModule(p, pMsg) )
        {
            C1222AL_LogComEventAndBytes(p, COMEVENT_C1222_SEND_MESSAGE_SUCCEEDED, &sequence, sizeof(sequence) );
        }
        
        p->status.consecutiveSendFailures = 0;
        
        if ( p->status.registrationInProgress )
            p->status.state = ALS_REGISTERING;
        else if ( p->status.deRegistrationInProgress )
            p->status.state = ALS_DEREGISTERING;
        else
            p->status.state = ALS_IDLE;
    }
}



void C1222AL_TL_NoteFailedSendMessage(C1222AL* p)
{
    C1222MessageSender* pSender = (C1222MessageSender*)(&p->messageSender);
    
    if ( ! C1222MessageSender_RestartSend(pSender) )
    {
        p->counters.sendAllRetriesFailed++;
        
        C1222AL_TL_NoteLinkFailure(p);
    }
}



void C1222AL_TL_NoteLinkFailure(C1222AL* p)
{   
    p->counters.linkFailure++;
    
    if ( p->status.state != ALS_INIT )
        p->status.state = ALS_IDLE;
}




#ifdef C1222_INCLUDE_COMMMODULE
void C1222AL_TLUpdateConfiguration(C1222AL* p, C1222TransportConfig* pConfig)  // called by TL (in comm module) on get config response
{
    C1222EnterCriticalRegion();
    
    if ( p->status.haveConfiguration )
    {
        // should compare the configurations and decide if we need to register again or not
        // but for now will just force a reregistration (TODO)
        
        if ( C1222AL_NeedToReregisterAfterConfigChange(p, pConfig) )
            C1222AL_SetRegisteredStatus(p, FALSE);
    }
    
    // may want to do checks?
    memcpy(&p->transportConfig, pConfig, sizeof(C1222TransportConfig));
    
    p->status.interfaceEnabled   = (Boolean)((pConfig->linkControlByte & 0x01) ? TRUE : FALSE);
    p->status.autoRegistration   = (Boolean)((pConfig->linkControlByte & 0x02) ? TRUE : FALSE);
    p->status.useDirectMessaging = (Boolean)((pConfig->linkControlByte & 0x04) ? TRUE : FALSE);
    
    if ( !p->status.interfaceEnabled )
        C1222AL_SetRegisteredStatus(p, FALSE);  // this needs work - TODO!!!
    
    p->status.haveConfiguration = TRUE;
    
    C1222LeaveCriticalRegion();
}
#endif // C1222_INCLUDE_COMMMODULE



void C1222AL_TLUpdateRegistrationStatus(C1222AL* p, C1222RegistrationStatus* pStatus)  // called by TL on get reg status response
{
    // since the register handles registration now, this should not be called
    (void)p;
    (void)pStatus;
}


#ifdef C1222_INCLUDE_DEVICE

void C1222AL_KeepAliveForEncryption(void* p)
{
    C1222AL* pAL = (C1222AL*)p;
    C1222TL* pTransport;
    
    if ( pAL != 0 )
    {
        pTransport = (C1222TL*)(pAL->pTransport);
        
        C1222TL_Run(pTransport);    
    }   
}

static Boolean Decrypt(C1222AL* p, ACSE_Message* pAssembledMessage)
{
    Unsigned8 keyId = 0;
    Unsigned8 key[24];
    Unsigned8 cipher;
    Unsigned8 iv[8];
    Unsigned8 ivLength;
        
        
    if ( ACSE_Message_GetEncryptionKeyId(pAssembledMessage, &keyId ) &&
         ACSE_Message_GetIV( pAssembledMessage, iv, &ivLength) )
    {
        if ( ! C1222AL_GetKey(p, keyId, &cipher, key) )
        {
            C1222AL_LogInvalidMessage(p, CMVR_ENCRYPTION_KEYID_MISSING, CMVR_OK, pAssembledMessage);
            return FALSE;
        }
        else if ( ! ACSE_Message_Decrypt(pAssembledMessage, cipher, key, iv, ivLength, p->tempBuffer,
                                C1222AL_KeepAliveForEncryption, p) )
        {
            C1222AL_LogInvalidMessage(p, CMVR_INFORMATION_ELEMENT_DECRYPTION_FAILED, CMVR_OK, pAssembledMessage);
            return FALSE;
        }
        
        // message decrypted ok
        return TRUE;                       
    }
    
    // something was wrong about the message structure
    C1222AL_LogInvalidMessage(p, CMVR_INVALID_MESSAGE_STRUCTURE_FOR_DECRYPTION, CMVR_OK, pAssembledMessage);
    return FALSE;    
}


static Boolean Authenticate(C1222AL* p, ACSE_Message* pAssembledMessage)
{
    Unsigned8 keyId = 0;
    Unsigned8 key[24];
    Unsigned8 cipher;
    Unsigned8 iv[8];
    Unsigned8 ivLength;    

    if ( ACSE_Message_GetEncryptionKeyId(pAssembledMessage, &keyId ) &&
         ACSE_Message_GetIV( pAssembledMessage, iv, &ivLength) )
    {
        if ( ! C1222AL_GetKey(p, keyId, &cipher, key) )
        {
            C1222AL_LogInvalidMessage(p, CMVR_ENCRYPTION_KEYID_MISSING, CMVR_OK, pAssembledMessage);
            return FALSE;
        }
        else if ( !  ACSE_Message_Authenticate(pAssembledMessage, cipher, key, iv, ivLength, p->tempBuffer) )
        {
            C1222AL_LogInvalidMessage(p, CMVR_AUTHENTICATION_FAILED, CMVR_OK, pAssembledMessage);
            return FALSE;
        }
        
        // message authenticated ok
        return TRUE;
    }
    
    // something was wrong about the message structure
    C1222AL_LogInvalidMessage(p, CMVR_INVALID_MESSAGE_STRUCTURE_FOR_AUTHENTICATION, CMVR_OK, pAssembledMessage);
    return FALSE;    
}

#endif // C1222_INCLUDE_DEVICE



Unsigned8 C1222AL_TLMessageReceived(C1222AL* p, Unsigned8 nativeAddressLength, Unsigned8* nativeAddress, BufferItem* pBufferItem)
{
    // called by transport layer when a message is received
    // the message might be segmented.  If so, need to assemble it into the complete message.
    // else just use the message
    // return value is the result code
    
    Unsigned8 response;
    ACSE_Message* pAssembledMessage = 0;

    Unsigned8 sizeofLength;
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    #ifdef C1222_INCLUDE_DEVICE
    C1222MessageLogEntry logEntry;
    #endif
    
    ACSE_Message msg;
    msg.length = msg.maxLength = C1222Misc_DecodeLength(&pBufferItem->buffer[1], &sizeofLength);
    msg.buffer = &pBufferItem->buffer[1+sizeofLength];
    
    // we've decided not to log the segments
    // C1222AL_LogComEventAndBytes(p, COMEVENT_C1222_RECEIVED_SEGMENT_BYTES, pBufferItem->buffer, pBufferItem->length);    
    
    if ( ACSE_Message_IsMarch2006FormatMessage(&msg) && ACSE_Message_IsValidFormat2006(&msg) )
    {
        ACSE_Message_SetStandardVersion(&msg, CSV_MARCH2006);
        p->counters.received2006FormatMessages++;
        C1222AL_LogEvent(p, C1222EVENT_AL_2006_FORMAT_MESSAGE_RECEIVED);
    }
        
    else if ( ACSE_Message_IsValidFormat2008(&msg) )
    {
        ACSE_Message_SetStandardVersion(&msg, CSV_2008);
        p->counters.received2008FormatMessages++;
        C1222AL_LogEvent(p, C1222EVENT_AL_2008_FORMAT_MESSAGE_RECEIVED);
    }
        
    else
    {
        // return an error
        // count invalid messages
        
        p->counters.receivedInvalidFormatMessages++;
        
        p->status.lastInvalid2006Reason = ACSE_Message_CheckFormat2006(&msg);
        p->status.lastInvalid2008Reason = ACSE_Message_CheckFormat2008(&msg);
        p->status.lastInvalidMessageTime = C1222Misc_GetFreeRunningTimeInMS();
                
        C1222AL_LogInvalidMessage(p, p->status.lastInvalid2006Reason, p->status.lastInvalid2008Reason, &msg);
        
        C1222AL_LogEventAndULongs(p, C1222EVENT_AL_INVALID_MESSAGE_RECEIVED,
                    p->status.lastInvalid2006Reason,
                    p->status.lastInvalid2008Reason,
                    p->status.lastInvalidMessageTime,
                    0
                      );
        
        // we couldn't figure out what standard the message was sent under - just ignore the message
        // but need to send a response to the transport layer
        
        C1222TL_SendPendingSendMessageFailedResponse(pTransport, C1222RESPONSE_ERR);
        
        //ACSE_Message_SetStandardVersion(&msg, p->status.clientC1222StandardVersion);
        //C1222AL_SimpleResponse(p, &msg, C1222RESPONSE_ERR, 0);
        
        return C1222RESPONSE_ERR;
    }
            
    
#if 0 // this is not a good idea - scb    
    if ( p->sending || (p->state == ALS_SENDINGMESSAGE) )
    {
        response = C1222RESPONSE_NETR;
        
        C1222AL_LogEvent(p, C1222EVENT_AL_COLLISION);
        
        C1222TL_SendPendingSendMessageFailedResponse(pTransport, response);
        return response;
    }
#endif

    if ( p->status.isReceivedMessageComplete )
    {
        C1222AL_IncrementStatistic(p, C1222_STATISTIC_NUMBER_OF_ACSE_PDU_DROPPED);
        
        p->counters.discardedCompleteMessages++;
        
        C1222AL_LogEvent(p, C1222EVENT_AL_COMPLETEMESSAGEDISCARDED);
    }
    else if ( !C1222MessageReceiver_IsMessageComplete(&p->messageReceiver) )
    {
        // this is not right - fix it before reenable
        //p->counters.discardedPartialMessages++;
        
        //C1222AL_LogEvent(p, C1222EVENT_AL_PARTIALMESSAGEDISCARDED);
    }    
    
    p->status.isReceivedMessageComplete = FALSE;
    
    p->counters.messagesReceived++;
    
    C1222AL_LogReceivedMessage(p, &msg);
    
    if ( p->status.isRelay )
    {
#ifdef C1222_INCLUDE_RELAY        
        response = C1222MessageReceiver_AddSegment(&p->messageReceiver, nativeAddressLength, nativeAddress, &msg);
    
        if ( response == C1222RESPONSE_OK )
        {
            if ( C1222MessageReceiver_IsMessageComplete(&p->messageReceiver) )
            {
                C1222AL_NoteReceivedCompleteMessage(p);
            }
            else
                C1222TL_SendPendingSendMessageOKResponse(pTransport);
        }
#endif // C1222_INCLUDE_RELAY        
    }
    else if ( !p->status.isCommModule ) // handling of message received by meter from the comm module
    {
        // must be a device
#ifdef C1222_INCLUDE_DEVICE
        response = C1222MessageReceiver_AddSegment(&p->messageReceiver, nativeAddressLength, nativeAddress, &msg);
    
        if ( response == C1222RESPONSE_OK )
        {
            if ( C1222MessageReceiver_IsMessageComplete(&p->messageReceiver) )
            {
                memset(&p->lastMessageLog, 0, sizeof(p->lastMessageLog));
                p->lastMessageLog.firstSegmentReceived = p->messageReceiver.receiveSegmenter.rxMessageStartTime;
                p->lastMessageLog.lastSegmentReceived = C1222Misc_GetFreeRunningTimeInMS();
                if ( p->lastMessageLog.firstSegmentReceived == 0 )
                    p->lastMessageLog.firstSegmentReceived = p->lastMessageLog.lastSegmentReceived;
                
                pAssembledMessage = C1222MessageReceiver_GetUnsegmentedMessage(&p->messageReceiver);
                
                response = C1222RESPONSE_SME;
                
                if ( ACSE_Message_IsEncrypted(pAssembledMessage) )
                {
                    if ( Decrypt(p, pAssembledMessage) )
                        response = C1222RESPONSE_OK;
                }
                else if ( ACSE_Message_IsAuthenticated(pAssembledMessage) )
                {
                    if ( Authenticate(p, pAssembledMessage) )
                        response = C1222RESPONSE_OK;                        
                }
                else
                    response = C1222RESPONSE_OK;
                    
                    
                
                if ( response == C1222RESPONSE_OK )
                {
                    p->lastMessageLog.availableForServer = C1222Misc_GetFreeRunningTimeInMS();
                    
                    MeterProcessReceivedMessage(p, pAssembledMessage, nativeAddressLength, nativeAddress);
                }
                else
                {
                    C1222AL_MakeMessageLogEntry(p, &msg, &logEntry, FALSE, C1222_MSGTYPE_RECEIVED_INVALID_MSG);  // this is a receiving log entry
                    C1222AL_LogComEventAndBytes(p, COMEVENT_RECEIVED_MESSAGE_FROM, &logEntry, sizeof(logEntry));
                }
            }
            else
                C1222TL_SendPendingSendMessageOKResponse(pTransport);
        }
#endif // C1222_INCLUDE_DEVICE       
    }
    else if ( p->status.wantReassembly ) // handling of a message received by comm module from the meter that needs reassembly
    {
#ifdef C1222_INCLUDE_COMMMODULE        
        response = C1222MessageReceiver_AddSegment(&p->messageReceiver, nativeAddressLength, nativeAddress, &msg);
    
        if ( response == C1222RESPONSE_OK )
        {
            if ( C1222MessageReceiver_IsMessageComplete(&p->messageReceiver) )
            {
                pAssembledMessage = C1222MessageReceiver_GetUnsegmentedMessage(&p->messageReceiver);

                #ifdef C1222_INCLUDE_COMMMODULE
                CommModuleProcessReceivedMessage(p, pAssembledMessage);
                #endif
            }
            else
                C1222TL_SendPendingSendMessageOKResponse(pTransport);        
        }
#endif // C1222_INCLUDE_COMMMODULE       
    }
    else // handling of message received by comm module from the meter
    {
#ifdef C1222_INCLUDE_COMMMODULE        
        // save the message
        
        response = C1222MessageReceiver_SetMessage(&p->messageReceiver, nativeAddressLength, nativeAddress, &msg);
        
        if ( response == C1222RESPONSE_OK )
        {
            C1222ALSegmenter_EndNewUnsegmentedMessage(&p->messageReceiver.receiveSegmenter);
            
            // get a pointer to the message
            pAssembledMessage = C1222MessageReceiver_GetUnsegmentedMessage(&p->messageReceiver);
                
            #ifdef C1222_INCLUDE_COMMMODULE
            CommModuleProcessReceivedMessage(p, pAssembledMessage);
            #endif
        }
#endif // C1222_INCLUDE_COMMMODULE        
    }
    
    if ( response != C1222RESPONSE_OK )
    {
        // send an ok response for the pending transport layer message since the message
        // got here ok
        C1222TL_SendPendingSendMessageOKResponse(pTransport);
        
        if ( pAssembledMessage != 0 )
            C1222AL_SimpleResponseWithoutReceivedMessageHandling(p, pAssembledMessage, response, 0);
        else
            C1222AL_SimpleResponseWithoutReceivedMessageHandling(p, &msg, response, 0);
    }
    

    return response;
}






#ifdef C1222_WANT_SHORT_MESSAGES

Unsigned8 C1222AL_TLShortMessageReceived(C1222AL* p, Unsigned8 nativeAddressLength, Unsigned8* nativeAddress, BufferItem* pBufferItem)
{
    // called by transport layer when a short message is received
    // the message cannot be segmented.
    // the message cannot be encrypted
    // need to expand to an acse message before doing anything else
    // return value is the result code
    
    // TODO
    
#if 0    
    
    Unsigned8 response;
    ACSE_Message* pAssembledMessage = 0;
#ifndef C1222_COMM_MODULE
    Unsigned8 keyId = 0;
    Unsigned8 key[24];
    Unsigned8 cipher;
    Unsigned8 iv[8];
    Unsigned8 ivLength;
#endif    
    Unsigned8 sizeofLength;
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    ACSE_Message msg;
    msg.length = msg.maxLength = C1222Misc_DecodeLength(&pBufferItem->buffer[1], &sizeofLength);
    msg.buffer = &pBufferItem->buffer[1+sizeofLength];
    
    p->isReceivedMessageComplete = FALSE;
    
    p->messagesReceived++;
    
    if ( p->status.isRelay )
    {
#ifdef C1222_INCLUDE_RELAY        
        response = C1222MessageReceiver_AddSegment(&p->messageReceiver, nativeAddressLength, nativeAddress, &msg);
    
        if ( response == C1222RESPONSE_OK )
        {
            if ( C1222MessageReceiver_IsMessageComplete(&p->messageReceiver) )
            {
                C1222AL_NoteReceivedCompleteMessage(p);
            }
            else
                C1222TL_SendPendingSendMessageOKResponse(pTransport);
        }
#endif        
    }
    else if ( !p->status.isCommModule ) // handling of message received by meter from the comm module
    {
#ifndef C1222_COMM_MODULE
        response = C1222MessageReceiver_AddSegment(&p->messageReceiver, nativeAddressLength, nativeAddress, &msg);
    
        if ( response == C1222RESPONSE_OK )
        {
            if ( C1222MessageReceiver_IsMessageComplete(&p->messageReceiver) )
            {
                pAssembledMessage = C1222MessageReceiver_GetUnsegmentedMessage(&p->messageReceiver);
                
                response = C1222RESPONSE_SME;
                
                if ( ACSE_Message_IsEncrypted(pAssembledMessage) )
                {
                    if ( ACSE_Message_GetEncryptionKeyId(pAssembledMessage, &keyId ) &&
                         C1222AL_GetKey(p, keyId, &cipher, key) &&
                         ACSE_Message_GetIV( pAssembledMessage, iv, &ivLength) &&
                         ACSE_Message_Decrypt(pAssembledMessage, cipher, key, iv, ivLength) 
                        )
                    {
                        response = C1222RESPONSE_OK;
                    }
                }
                else if ( ACSE_Message_IsAuthenticated(pAssembledMessage) )
                {
                    if ( ACSE_Message_GetEncryptionKeyId(pAssembledMessage, &keyId ) &&
                         C1222AL_GetKey(p, keyId, &cipher, key) &&
                         ACSE_Message_GetIV( pAssembledMessage, iv, &ivLength) &&
                         ACSE_Message_Authenticate( pAssembledMessage, cipher, key, iv, ivLength) 
                       )
                    {
                       response = C1222RESPONSE_OK;
                    }
                }
                else
                    response = C1222RESPONSE_OK;
                
                if ( response == C1222RESPONSE_OK )
                    MeterProcessReceivedMessage(p, pAssembledMessage, nativeAddressLength, nativeAddress);
            }
            else
                C1222TL_SendPendingSendMessageOKResponse(pTransport);
        }
#endif        
    }
    else if ( p->status.wantReassembly ) // handling of a message received by comm module from the meter that needs reassembly
    {
#ifdef C1222_INCLUDE_COMMMODULE        
        response = C1222MessageReceiver_AddSegment(&p->messageReceiver, nativeAddressLength, nativeAddress, &msg);
    
        if ( response == C1222RESPONSE_OK )
        {
            if ( C1222MessageReceiver_IsMessageComplete(&p->messageReceiver) )
            {
                pAssembledMessage = C1222MessageReceiver_GetUnsegmentedMessage(&p->messageReceiver);

                #ifdef C1222_INCLUDE_COMMMODULE
                CommModuleProcessReceivedMessage(p, pAssembledMessage);
                #endif
            }
            else
                C1222TL_SendPendingSendMessageOKResponse(pTransport);        
        }
#endif // C1222_INCLUDE_COMMMODULE        
    }
    else // handling of message received by comm module from the meter
    {
#ifdef C1222_INCLUDE_COMMMODULE        
        // save the message
        
        response = C1222MessageReceiver_SetMessage(&p->messageReceiver, nativeAddressLength, nativeAddress, &msg);
        
        if ( response == C1222RESPONSE_OK )
        {
            C1222ALSegmenter_EndNewUnsegmentedMessage(&p->messageReceiver.receiveSegmenter);
            
            // get a pointer to the message
            pAssembledMessage = C1222MessageReceiver_GetUnsegmentedMessage(&p->messageReceiver);
                
            #ifdef C1222_INCLUDE_COMMMODULE
            CommModuleProcessReceivedMessage(p, pAssembledMessage);
            #endif
        }
#endif // C1222_INCLUDE_COMMMODULE        
    }
    
    if ( response != C1222RESPONSE_OK )
    {
        C1222TL_SendPendingSendMessageOKResponse(pTransport);
        
        if ( pAssembledMessage != 0 )
            C1222AL_SimpleResponseWithoutReceivedMessageHandling(p, pAssembledMessage, response, 0);
        else
            C1222AL_SimpleResponseWithoutReceivedMessageHandling(p, &msg, response, 0);
    }
    

    return response;
#else
    return C1222RESPONSE_ERR;    
#endif    
}


#endif



#ifdef C1222_INCLUDE_COMMMODULE
void C1222AL_TLInterfaceControlReceived(C1222AL* p, Unsigned8 interfaceControl)
{
    Unsigned8 value;
    
    value = (Unsigned8)(interfaceControl & 0x03);
    switch( value )
    {
        case 0: // maintain current state
            break;
        case 1: // enable this interface
            p->status.interfaceEnabled = TRUE;
            break;
        case 2: // disable this interface
            p->status.interfaceEnabled = FALSE;
            break;
        case 3: // reset this interface
            C1222TL_ResetRequested((C1222TL*)(p->pTransport));
            break;
    }
    
    value = (Unsigned8)((interfaceControl >> 2) & 0x03);
    switch( value )
    {
        case 0: // maintain current state
            break;
        case 1: // disable auto registration
            p->status.autoRegistration = FALSE;
            break;
        case 2: // enable auto registration
            p->status.autoRegistration = TRUE;
            break;
        case 3: // force one time registration now then return to previous state
            p->status.timeToRegister = TRUE;
            break;
    }
    
    value = (Unsigned8)((interfaceControl >> 4) & 0x03);
    switch( value )
    {
        case 0: // maintain current state
            break;
        case 1: // enable direct messaging
            p->status.useDirectMessaging = TRUE;
            break;
        case 2: // disable direct messaging
            p->status.useDirectMessaging = FALSE;
            break;
        case 3: // undefined      
            break;
    }
    
    if ( interfaceControl & 0x40 )
    {
        // reset all statistics
        memset( p->transportStatus.statistics, 0, sizeof(p->transportStatus.statistics) );
        memset( p->transportStatus.changedStatistics, 0, sizeof(p->transportStatus.changedStatistics) );
    }
    else
    {
        // no reset of all statistics
    }
    
    if ( interfaceControl & 0x80 )
    {
        // initiate get configuration service
        // this is handled in the transport layer so do nothing here
    }
    else
    {
        // maintain current state
    }
}
#endif // C1222_INCLUDE_COMMMODULE



Unsigned8* C1222AL_TLGetNodeApTitle(C1222AL* p)
{
    return p->registrationStatus.nodeApTitle;
}



Unsigned8 C1222AL_TLGetInterfaceStatus(C1222AL* p)
{
    Unsigned8 interfaceStatus = 0;
    
    if ( p->status.interfaceEnabled )
        interfaceStatus |= C1222_TL_STATUS_INTERFACE_FLAG;
        
    if ( p->status.autoRegistration )
        interfaceStatus |= C1222_TL_STATUS_AUTO_REGISTRATION_FLAG;
        
    if ( p->status.useDirectMessaging )
        interfaceStatus |= C1222_TL_STATUS_DIRECT_MESSAGING_FLAG;
    
    return interfaceStatus;
}



void C1222AL_TLLinkControlResponse(C1222AL* p, Unsigned8 interfaceStatus, Unsigned8* pApTitle)
{
    // the meter (c12.22 device) sends a link control request to the comm module.
    // this is the response to that request
    
    //Unsigned8 length;

    (void)pApTitle;
    
    C1222EnterCriticalRegion();
    
    p->transportStatus.interfaceStatus = interfaceStatus;
    //p->configurationChanged = FALSE;
    
    // the comm module no longer knows if we are registered or not
    
    
    C1222LeaveCriticalRegion();
    
    p->status.linkControlRunning = FALSE;
    p->status.linkControlResponse = C1222RESPONSE_OK;

    // may also want to signal some process here!!!
}



void C1222AL_TLSendMessageResponse(C1222AL* p)
{
    // notifies the application layer of a successful send message response
    
    ContinueMessageSend(p);
}



const char* C1222AL_TLGetInterfaceName(C1222AL* p)  // returns asciiz string
{
    return p->pInterfaceName;
}



Unsigned16 C1222AL_TLGetNumberOfStatistics(C1222AL* p)
{
    Unsigned16 ii;
    
    for ( ii=0; ii<C1222_NUMBER_OF_STATISTICS; ii++ )
    {
        if ( p->transportStatus.statistics[ii].statisticCode == 0 )
            return ii;
    }
    
    return C1222_NUMBER_OF_STATISTICS;
}



Unsigned16 C1222AL_TLGetNumberOfChangedStatistics(C1222AL* p)
{
    Unsigned16 ii;
    Unsigned16 changeCount = 0;
    
    for ( ii=0; ii<C1222_NUMBER_OF_STATISTICS; ii++ )
    {
        if ( p->transportStatus.changedStatistics[ii] )
            changeCount++;
    }
    
    return changeCount;    
}



void C1222AL_TLGetChangedStatistic(C1222AL* p, Unsigned16 index, C1222Statistic* pStatistic)
{
    Unsigned16 ii;
    Unsigned16 changeCount = 0;
    
    for ( ii=0; ii<C1222_NUMBER_OF_STATISTICS; ii++ )
    {
        if ( p->transportStatus.changedStatistics[ii] )
        {
            if ( changeCount == index )
            {
                memcpy(pStatistic, &p->transportStatus.statistics[ii], sizeof(C1222Statistic));
                return;
            }
            
            changeCount++;
        }
    }
    
    // not found
    
    memset(pStatistic, 0, sizeof(C1222Statistic));
    
    return;       
}



void C1222AL_TLGetStatistic(C1222AL* p, Unsigned16 ii, C1222Statistic* pStatistic)
{
    memset(pStatistic, 0, sizeof(C1222Statistic));
    
    if ( ii < C1222_NUMBER_OF_STATISTICS )
    {
        memcpy(pStatistic, & p->transportStatus.statistics[ii], sizeof(C1222Statistic));
    }
}



C1222RegistrationStatus* C1222AL_TLGetRegistrationStatus(C1222AL* p)
{
    return &(p->registrationStatus);
}



void C1222AL_TLUpdateInterfaceInfo(C1222AL* p, Unsigned8 interfaceNameLength, char* interfaceName, Unsigned8 interfaceStatus)
{
    C1222EnterCriticalRegion();
    
    memset(p->transportStatus.interfaceName, 0, C1222_INTERFACE_NAME_LENGTH);
    
    if ( interfaceNameLength >= C1222_INTERFACE_NAME_LENGTH )
        interfaceNameLength = C1222_INTERFACE_NAME_LENGTH - 1;
        
    memcpy( p->transportStatus.interfaceName, interfaceName, interfaceNameLength);
    
    p->transportStatus.interfaceStatus = interfaceStatus;
    p->status.haveInterfaceInfo = TRUE;
    
    
    C1222LeaveCriticalRegion();
}


// long/short registration time -
//    during unsynchronized period, if warm start flag goes low, set a flag
//    then when synchronized, if cell switch and that flag is set, do long registration randomization and clear the flag
//    else if cell switch do short registration randomization
//    else if no cell switch clear the flag

void C1222AL_HandleStatisticChange(C1222AL* p, C1222Statistic* pStat)
{
    Boolean b;
    Unsigned32 now;
    RFLAN_FATHER_INFO* pFatherInfo;
    
    switch( pStat->statisticCode )
    {
    case C1222_STATISTIC_IS_SYNCHRONIZED_NOW:
        b = p->status.isSynchronizedOrConnected;
        
        p->status.isSynchronizedOrConnected = (Boolean)pStat->statisticValue.lower32Bits;
        
        if ( p->status.isSynchronizedOrConnected && !b )
            C1222AL_IncrementStatistic(p, C1222_STATISTIC_RESYNCHRONIZED_COUNT);        
        break;
        
    case C1222_STATISTIC_RFLAN_CURRENT_CELL_ID:
        if ( (p->status.currentRFLanCellId != (Unsigned16)pStat->statisticValue.lower32Bits) &&
             ((Unsigned16)pStat->statisticValue.lower32Bits != 0) &&
             (!p->status.disableRegisterOnCellChange) )
        {
            if ( p->status.currentRFLanCellId == 0 )
                p->status.currentRFLanCellId = (Unsigned16)pStat->statisticValue.lower32Bits;
            else
            {
                now = C1222Misc_GetFreeRunningTimeInMS();
                if ( p->status.timeOfCellIDChange == 0 )
                {
                    p->status.timeOfCellIDChange = now;
                }
                else if ( (now-p->status.timeOfCellIDChange) > 600000L ) // wait 10 minutes before registering to the new cell in case we flop back
                {
                    p->status.currentRFLanCellId = (Unsigned16)pStat->statisticValue.lower32Bits;
            
                    C1222AL_IncrementStatistic(p, C1222_STATISTIC_NUMBER_OF_RFLAN_CELL_CHANGES);
            
                    if ( p->status.isRegistered )
                        p->status.timeLastLostRegistration = now;
                        
                    p->status.isRegistered = FALSE;
                    
                    if ( ! p->status.isRFLAN_coldStart )
                        p->status.tryingToRegisterAfterCellSwitch = TRUE;
                        
                    p->status.isRFLAN_coldStart = FALSE;
                
                    p->status.timeOfCellIDChange = 0;
                    
                    p->status.lastRegistrationAttemptTime = C1222Misc_GetFreeRunningTimeInMS();
                    
                    C1222AL_SetRegistrationRetryTime(p);  
                }
            }
        }
        else
            p->status.timeOfCellIDChange = 0;
            
        break;
       
    case C1222_STATISTIC_RFLAN_CURRENT_LEVEL:
        if ( p->status.currentRFLanLevel != (Unsigned16)pStat->statisticValue.lower32Bits )
        {
            p->status.currentRFLanLevel = (Unsigned16)pStat->statisticValue.lower32Bits;
            
            C1222AL_IncrementStatistic(p, C1222_STATISTIC_NUMBER_OF_RFLAN_LEVEL_CHANGES);
        }
        break;
        
    case C1222_STATISTIC_RFLAN_STATE_INFO:
        // 
        memcpy(p->status.rflanStateInfo, &(pStat->statisticValue), sizeof(p->status.rflanStateInfo));
        p->status.haveRevInfo = TRUE;
        break;
        
    case C1222_STATISTIC_RFLAN_SYNCH_FATHER_MAC_ADDRESS:
        p->status.syncFatherMacAddress = pStat->statisticValue.lower32Bits;
        break;
        
    case C1222_STATISTIC_RFLAN_BEST_FATHER_MAC_ADDRESS:
        p->status.bestFatherMacAddress = pStat->statisticValue.lower32Bits;
        break;
        
    case C1222_STATISTIC_RFLAN_GPD:
        p->status.rflan_gpd = (Unsigned16)(pStat->statisticValue.lower32Bits);
        break;
        
    case C1222_STATISTIC_RFLAN_FATHER_COUNT:
        p->status.rflan_fatherCount = pStat->statisticValue.lower32Bits;
        break;
        
    case C1222_STATISTIC_RFLAN_SYNCH_FATHER_INFO:
        pFatherInfo = (RFLAN_FATHER_INFO*)&pStat->statisticValue;
        p->status.rflan_syncFatherRssi = pFatherInfo->m_AverageRSSI;
        break;
    case C1222_STATISTIC_RFLAN_BEST_FATHER_INFO:
        pFatherInfo = (RFLAN_FATHER_INFO*)&pStat->statisticValue;
        p->status.rflan_bestFatherRssi = pFatherInfo->m_AverageRSSI;
        break;
        
    default:
        break;
    }
}



void C1222AL_TLNoteStatisticsUpdated(C1222AL* p)
{
    RFLAN_STATE_INFO* pRFLAN_State;
    
    // check unsynchronized and warm start is low
    if ( ! p->status.isSynchronizedOrConnected )
    {
        pRFLAN_State = (RFLAN_STATE_INFO*)p->status.rflanStateInfo;
        if ( ! pRFLAN_State->m_IsWarmStart )
        {
            if ( !p->status.isRFLAN_coldStart )
                C1222AL_LogComEvent(p, COMEVENT_C1222_RFLAN_COLD_START);
                
            p->status.isRFLAN_coldStart = TRUE;
        }
    }
    else
    {
        // if no cell change in progress, clear the cold start flag
        
        if ( p->status.timeOfCellIDChange == 0 )
            p->status.isRFLAN_coldStart = FALSE;                
    }
} 



void C1222AL_TLUpdateStatistic(C1222AL* p, C1222Statistic* pStat)
{
    // assume stats are packed in the stats array
    // put this stat in the matching index if found, else in the first available slot
    
    Unsigned16 ii;
    C1222Statistic* pWorkingStat;
    
    for ( ii=0; ii<C1222_NUMBER_OF_STATISTICS; ii++ )
    {
        pWorkingStat = &p->transportStatus.statistics[ii];
        
        if ( pStat->statisticCode == pWorkingStat->statisticCode )
        {
            C1222EnterCriticalRegion();
            pWorkingStat->statisticValue = pStat->statisticValue;
            C1222AL_HandleStatisticChange(p, pStat);
            C1222LeaveCriticalRegion();
            break;
        }
        else if ( pWorkingStat->statisticCode == 0 )
        {
            C1222EnterCriticalRegion();
            pWorkingStat->statisticCode = pStat->statisticCode;
            pWorkingStat->statisticValue = pStat->statisticValue;
            C1222AL_HandleStatisticChange(p, pStat);
            C1222LeaveCriticalRegion();
            break;
        }
    }
}


 

Unsigned16 C1222AL_GetMyNativeAddress(C1222AL* p, Unsigned8* myNativeAddress, Unsigned16 maxNativeAddressLength)
{
    Unsigned16 length = maxNativeAddressLength;
    
    memset(myNativeAddress, 0, length);
    
    if ( length > p->transportConfig.nativeAddressLength )
        length = p->transportConfig.nativeAddressLength;
    
    memcpy(myNativeAddress, p->transportConfig.nativeAddress, length);
    
    return length;
}

