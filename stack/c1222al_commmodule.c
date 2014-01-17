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




// C12.22 Application Layer (comm module)

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


#ifdef C1222_INCLUDE_COMMMODULE


// static routines in this module
void RunCommModule(C1222AL* p);

static void CommModuleHandleAutoRegistration(C1222AL* p);
static void CommModuleHandleTimeToRegister(C1222AL* p);
static void CommModuleHandleRegistration(C1222AL* p);
static void CommModuleWaitForRegistrationResponse(C1222AL* p);



// run methods need to deal with:
//   - registration at startup and keepalive
//   - sending messages (segmentation)
//   - receiving messages (segmentation)
//   - monitor and be controled by interface status



Boolean CommModule_IsMessageForMe(C1222AL* p, ACSE_Message* pMsg)
{
    // if we are registered, the registration status, which we obtained as a result of the 
    // registration process will include the node aptitle
    // and the transport layer has the interface number
    // so my aptitle is hostApTitle.1.interfaceNumber
    // but I also should respond to hostApTitle.1.0 and hostApTitle.1
    
    C1222ApTitle calledApTitle;
    C1222ApTitle nodeApTitle;
    Unsigned8 branchStartIndex;    
    
    if ( ACSE_Message_GetCalledApTitle(pMsg, &calledApTitle) )
    {
        C1222ApTitle_Construct(&nodeApTitle, p->registrationStatus.nodeApTitle, C1222_APTITLE_LENGTH);
        
        if ( C1222ApTitle_Is2ndBranchOf1st(&nodeApTitle, &calledApTitle, &branchStartIndex) )
        {
            if ( calledApTitle.buffer[branchStartIndex] == 1 )
            {
                // check for nodeApTitle.1
                if ( calledApTitle.length == (branchStartIndex+1) )
                    return TRUE;
                // check for nodeApTitle.1.0
                if ( calledApTitle.length == (branchStartIndex+2) )
                {
                    if ( calledApTitle.buffer[branchStartIndex+1] == 0 )
                        return TRUE;
                        
                    // check for nodeApTitle.1.interfaceNumber
                    
                    if ( calledApTitle.buffer[branchStartIndex+1] == C1222TL_GetInterfaceNumber((C1222TL*)(p->pTransport)) )
                        return TRUE;
                }
            }
        }
    }
    
	return FALSE;
}


void CommModuleRequestServer_handleMessage(C1222AL* p, ACSE_Message* pMsg)
{
    // TODO: FILL THIS UP
    // if this is a message for the comm module, it should be a table request
    
    C1222AL_SimpleResponse(p, pMsg, C1222RESPONSE_IAR, 0);
}



void CommModuleProcessReceivedMessage(C1222AL* p, ACSE_Message* pMsg)
{
    // this routine processes a message received from the meter
    
    Boolean isSegmented;
    Unsigned16 sequence;
    
    // the message might be directed at this comm module, or might be intended to be transmitted to the network
    
    //if ( CommModule_IsMessageForMe(p, pMsg) )
    //    CommModuleRequestServer_handleMessage(p, pMsg);
    //else
    //{
    
    (void)pMsg;
    
    // if this is a segmented message, we will disable registration for a traffic timeout to avoid
    // problems with a registration request replacing an in progress segmented message down the line
    // like in a relay that is reassembling messages
    
    if ( ACSE_Message_IsSegmented(pMsg, &isSegmented) && isSegmented )
    {
        p->status.lastCMSegmentedMessagePromotion = C1222Misc_GetFreeRunningTimeInMS();
    }
    else
        p->status.lastCMSegmentedMessagePromotion = 0;
 
    // check if this was a registration response when we are already registered
    // if so, ignore it
    
    if ( p->counters.registrationAttempts > 0 )
    {
        if ( (C1222Misc_GetFreeRunningTimeInMS()-p->status.lastRegistrationAttemptTime) < 60000L )
        {
            if ( ACSE_Message_GetCalledApInvocationId( pMsg, &sequence) )
            {
                if ( sequence == p->status.registrationRequestApInvocationId )
                {
                    // we got here because we are no longer waiting for the registration response,
                    // probably because we already handled the reg response
                    // so if we get another one, ignore it
                    // anyway, the reg response should not be passed to the register
                    C1222AL_LogEvent(p, C1222EVENT_AL_DISCARDEDEXTRAREGRESPONSE);
                    return;
                }
            }
        }
    }
    
    // let the comm module handle the message
    
    C1222AL_NoteReceivedCompleteMessage(p);
        
	//}
}





static void CommModuleSendingMessage(C1222AL* p)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    if ( (C1222Misc_GetFreeRunningTimeInMS()-p->status.sendStartTime) > (p->status.sendTimeOutSeconds*1000L) )
    {
        p->counters.sendTimedOut++;
        
        C1222AL_LogEvent(p, C1222EVENT_AL_SENDTIMEDOUT);
        
        C1222AL_TL_NoteFailedSendMessage(p);
    }
    else
        C1222TL_Run( pTransport );
}




void RunCommModule(C1222AL* p)
{
	C1222TL* pTransport = (C1222TL*)(p->pTransport);
       
    switch( p->status.state )
    {
        case ALS_INIT:
        	p->status.state = ALS_IDLE;
            break;
            
        case ALS_IDLE:

            CommModuleHandleRegistration(p);
                
            if ( p->status.state == ALS_IDLE )
                C1222TL_Run( pTransport );
            break;
            
        case ALS_SENDINGMESSAGE:
            CommModuleSendingMessage(p);
            break;
            
        case ALS_REGISTERING:
            CommModuleWaitForRegistrationResponse(p);
            break;
    }
}






#define MAX_REGISTRATION_REQUEST    80


static void CommModuleSendRegistrationRequestToNetwork(C1222AL* p)
{
    Unsigned8 request[MAX_REGISTRATION_REQUEST]; 
    Unsigned16 index;
    Unsigned16 length;
    Unsigned8 epsemBuffer[100]; // need to fix this
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
	    return;
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
    
    epsemBuffer[0] = 0x90; // response required, device class included
    memcpy( &epsemBuffer[1], p->transportConfig.deviceClass, 4);
    
    Epsem_Init(&epsem, epsemBuffer, sizeof(epsemBuffer));
    
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
    
    p->status.registrationRequestApInvocationId = C1222AL_GetNextApInvocationId(p);
    
    ACSE_Message_SetCallingApInvocationId(pMsg, p->status.registrationRequestApInvocationId);
    
    // assume there is enough room in our buffers for a registration request
    (void)ACSE_Message_SetUserInformation(pMsg, &epsem, FALSE); // no mac needed
    
    EndNewUnsegmentedMessage(p);
    
    // set the native address into the message receiver
    
    length = p->transportConfig.nativeAddressLength;
    
    if ( (length > 0)  && (length <= C1222_NATIVE_ADDRESS_LENGTH) )
    {
        p->messageReceiver.nativeAddressLength = length;
    
        memcpy(p->messageReceiver.nativeAddress, p->transportConfig.nativeAddress, length);
    }
    
    // the message is ready - let the app user have it
    
    C1222AL_IncrementStatistic(p, (Unsigned16)C1222_STATISTIC_NUMBER_OF_REGISTRATIONS_SENT);    
    
    C1222AL_NoteReceivedCompleteMessage(p); 
    
    p->status.sendingRegistrationRequest = TRUE;
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


Boolean CommModuleHandleRegistrationResponse(C1222AL* p, ACSE_Message* pMsg, Unsigned8 addressLength, 
                       Unsigned8* nativeAddress)
{
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
                            
                            return TRUE;
                        } // end if response length makes sense for a registration response
                        else
                            p->status.registrationFailureReason = MFC_REG_DEREG_RESPONSE_LENGTH_INVALID;
                            
                    } // end if the response could be a registration response based on length
                    else
                        p->status.registrationFailureReason = MFC_REG_DEREG_RESPONSE_LENGTH_INVALID;
                } // end if the response was <ok>
                else
                    p->status.registrationFailureReason = MFC_REG_DEREG_RESPONSE_NOT_OK;
            } // end while more requests or responses
        } // end if the epsem validated 
        else
            p->status.registrationFailureReason = MFC_REG_DEREG_EPSEM_INVALID;
             
    } // end if found the information element
    else
        p->status.registrationFailureReason = MFC_REG_DEREG_INFORMATION_ELEMENT_MISSING;
    
    return FALSE;
}




Boolean CommModuleHandleNetworkMessageDuringRegistration(C1222AL* p, ACSE_Message* pMsg, 
                      Unsigned8 addressLength, Unsigned8* nativeAddress)
{
    Unsigned16 apInvocationId;
    
    // until we are registered, we probably don't want to accept any messages
    // for one thing we probably don't have an aptitle
    
    // need to determine if this message is a registration response
    // can do that by comparing the called ap invocation id to the saved registration ap invocation id
    
    if ( ACSE_Message_GetCalledApInvocationId(pMsg, &apInvocationId) )
    {
        if ( apInvocationId == p->status.registrationRequestApInvocationId )
        {
            if ( CommModuleHandleRegistrationResponse(p, pMsg, addressLength, nativeAddress) )
            {
                p->status.lastSuccessfulRegistrationTime = C1222Misc_GetFreeRunningTimeInMS();
                p->counters.registrationSuccessCount++;
                C1222AL_SetRegisteredStatus(p, TRUE);
                p->status.registrationInProgress = FALSE;
                p->status.state = ALS_IDLE;
                C1222AL_LogEvent(p, C1222EVENT_AL_REGISTERED);
            }
            else
                C1222AL_LogEvent(p, C1222EVENT_AL_REG_RESPONSE_INVALID);
            
            return TRUE; // it was a registration response and we handled it so it does not
                         // need to be sent down the stack
        }
        
        C1222AL_LogEvent(p, C1222EVENT_AL_OTHER_MESSAGE_WAITING_FOR_REG_RESPONSE);
        
        // if not a registration response, I'm going to ignore it
    }
    
    // if no apInvocation id, cannot be a registration response - or I can't tell, so ignore it
    return FALSE;  // the message was not consumed here - send it on to the stack
}                      




static void CommModuleWaitForRegistrationResponse(C1222AL* p)
{
	Unsigned32 now;
	
	// need to check once in awhile if it has been too long and send the request again
	
	now = C1222Misc_GetFreeRunningTimeInMS();
	
	if ( (now-p->status.lastRegistrationAttemptTime) > (p->status.registrationRetrySeconds*1000L) )  // wait 20 secs //2 minutes for now
	{
	    p->status.timeToRegister = TRUE;
	    p->status.state = ALS_IDLE;
	    p->status.registrationInProgress = FALSE;
	    
	    C1222AL_SetRegistrationRetryTime(p);
	}
	else if ( !p->status.sendingRegistrationRequest )
	{
	    // let the transport layer run to keep the device happy
	    // will need to handle messages from the device now in this state!
	    
	    // until the registration request has been sent out the other stack
	    // ignore our lower layers to avoid adding a new message to the mix
	    
	    C1222TL_Run( (C1222TL*)(p->pTransport) );
	}
}



static void CommModuleHandleAutoRegistration(C1222AL* p)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    Unsigned32 now = C1222Misc_GetFreeRunningTimeInMS();
    Unsigned16 minutes;
    C1222TransportConfig* pConfig = &p->transportConfig;
    
    if ( !C1222TL_IsReady(pTransport) )
        return;
        
    if ( C1222AL_IsBusy(p) )
        return; 
        
    if ( !p->status.isSynchronizedOrConnected )
    {
        p->status.registrationFailureReason = MFC_REG_NOT_SYNCHRONIZED;
        p->status.timeToRegister = FALSE;
        return;
    }       
    
    if ( !p->status.isRegistered )
    {
        // in order to register, need an esn or an aptitle
        
        if ( (C1222Misc_GetUIDLength(pConfig->esn) <= 2) && (C1222Misc_GetUIDLength(pConfig->nodeApTitle) <= 2) )
        {
            // an event here would be too busy since this will be called lots
            
            return;
        }
        
        
        // if not registered and auto registration is on, better register
        // we might want to implement a random delay here
        // TODO: !!
        
        if ( (now-(p->status.lastRegistrationAttemptTime)) > (p->status.registrationRetrySeconds*1000L) )
        {
            C1222AL_LogEvent(p, C1222EVENT_AL_TIME_TO_REGISTER);
            p->status.timeToRegister = TRUE;
        }
    }
    else if ( p->registrationStatus.regPeriodMinutes != 0  )
    {
        // currently registered - handle keep alive
        
        if ( p->registrationStatus.regCountDownMinutes == 0 )
        {
            if ( p->status.state != ALS_REGISTERING )
            {
                C1222AL_LogEvent(p, C1222EVENT_AL_TIME_TO_REFRESH_REGISTRATION);
                p->status.timeToRegister = TRUE;
            }
        }
        else
        {
            now = C1222Misc_GetFreeRunningTimeInMS();
            
            minutes = (Unsigned16)((now - p->status.lastSuccessfulRegistrationTime)/60000L);

            if ( minutes < p->registrationStatus.regPeriodMinutes )
                p->registrationStatus.regCountDownMinutes = (Unsigned16)(p->registrationStatus.regPeriodMinutes - minutes);
            else
                p->registrationStatus.regCountDownMinutes = 0;
        }
    }
    // else don't need to refresh registration
}




static void CommModuleHandleTimeToRegister(C1222AL* p)
{
    C1222TransportConfig* pConfig = &p->transportConfig;
    
    if ( (C1222Misc_GetFreeRunningTimeInMS()-p->status.lastCMSegmentedMessagePromotion) < 20000 )
    {
        return;  // wait a bit to prevent overwriting a downstream partially assembled message
                 // with the registration request
    }
    
           
    if ( p->status.timeToRegister && p->status.haveConfiguration && (!C1222AL_IsBusy(p))
         && p->status.isReceivedMessageHandlingComplete && !p->status.isReceivedMessageComplete )
    {
        if ( (C1222Misc_GetUIDLength(pConfig->esn) <= 2) && (C1222Misc_GetUIDLength(pConfig->nodeApTitle) <= 2) )
        {
            p->status.registrationFailureReason = MFC_REG_ESN_INVALID;
            C1222AL_LogEvent(p, C1222EVENT_AL_ESN_INVALID);
            // should log something
            return;
        }
        
        if ( (C1222Misc_GetUIDLength(pConfig->esn) > C1222_ESN_LENGTH) || 
             (C1222Misc_GetUIDLength(pConfig->nodeApTitle) > C1222_APTITLE_LENGTH) )
        {
            p->status.registrationFailureReason = MFC_REG_ESN_OR_APTITLE_TOO_LONG;
            C1222AL_LogEvent(p, C1222EVENT_AL_ESN_OR_APTITLE_TOOLONG);
            // should log something
            return;
        }
           
        CommModuleSendRegistrationRequestToNetwork(p);
        
        p->status.state = ALS_REGISTERING;
        p->status.registrationInProgress = TRUE;
        p->status.registrationFailureReason = MFG_REG_DEREG_PENDING_OR_TIMEOUT;  // pending or timeout
        
        p->status.lastRegistrationAttemptTime = C1222Misc_GetFreeRunningTimeInMS();
        p->status.timeToRegister = FALSE;
    }
}



static void CommModuleHandleRegistration(C1222AL* p)
{
    if ( p->status.autoRegistration ) // auto-registration
        CommModuleHandleAutoRegistration(p);
                
    if ( p->status.timeToRegister )
        CommModuleHandleTimeToRegister(p);
}


#endif // C1222_INCLUDE_COMMMODULE
