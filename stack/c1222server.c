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


#include "c1222al.h"
#include "c1222server.h"
#include "c1222response.h"
#include "c1222misc.h"


//#define DEFAULT_C1222_USER_ID   100

extern Unsigned8 Table122_GetNumberOfMulticastAddresses(void);
extern Boolean Table122_GetMulticastAddress(Unsigned8 index, C1222ApTitle* pApTitle);
extern void Table00_GetMeterDeviceClass(Unsigned8* deviceClass);
extern void Table06_GetMyIdentification(Unsigned8* pmyId);
extern Unsigned8 Table46_GetNumberOfKeys(void);


// static routines in this module

Boolean BuildResponse(C1222Server* pServer, ACSE_Message* pMsg, Epsem* pResponseEpsem, ACSE_Message* pResponseMessage, 
            C1222AL* pAppLayer, Boolean wantMac, Boolean wantAuthenticate, Unsigned8 keyId, 
            Unsigned8* iv, Unsigned8 ivLength, Unsigned8 cipher, Unsigned8* key);


Boolean ProcessEpsemRequests(C1222Server* pServer, Epsem* pEpsem, Unsigned16 userId,
            Unsigned8* password, Boolean isTest, Boolean isNotification, 
            Epsem* pResponseEpsem, C1222StandardVersion requestStandard);             




Unsigned8 C1222Server_HandleDisconnectRequest(C1222Server* p)
{
    return C1222Stack_NoteDisconnectRequest(p->pStack);
}


Unsigned8 C1222Server_HandleDeRegisterRequest(C1222Server* p, Unsigned8* response, Unsigned16 maxResponseLength, Unsigned16* pResponseLength)
{
    C1222ApTitle apTitle;
    
    if ( C1222Stack_IsRegistered(p->pStack) && C1222Stack_GetMyApTitle(p->pStack, &apTitle) &&
         ((apTitle.length+1) <= maxResponseLength) )
    {
        memcpy(&response[1],apTitle.buffer, apTitle.length);
        *pResponseLength = (Unsigned16)(apTitle.length+1);
        C1222Stack_NoteDeRegisterRequest(p->pStack);
        return C1222RESPONSE_OK;
    }
    else
    {
        return C1222RESPONSE_IAR;
    }
}


void C1222Server_GetMyIdentification(C1222Server* p, Unsigned8* pId)
{
    // get identification
    
    (void)p;
    
    Table06_GetMyIdentification(pId); 
}



Unsigned8 C1222SecurityMechanism_2006[] = { 0x06, 0x07, 0x2a, 0x86, 0x48, 0xCE, 0x52, 0x02, 0x01 };
Unsigned8 C1222SecurityMechanism_2008[] = { 0x06, 0x09, 0x60, 0x7C, 0x86, 0xF7, 0x54, 0x01, 0x16, 0x02, 0x01 };

Unsigned16 C1222PsemIdentResponse(Unsigned8* response, Unsigned8* myId)
{
    // I guess we will respond with 2008 format until told otherwise - I think only the collection engine will ask and we should
    // respond in 2008 format for it.
    
    Unsigned16 responseLength = 0;
    
    *response++ = 0;      // ok
    responseLength++;
    
    *response++ = 0x03;   // std == c12.22
    *response++ = 0x01;   // version 1
    *response++ = 0x00;   // rev 0
    responseLength += (Unsigned16)3;
    
    *response++ = 0x04;   // security mechanism supported
    memcpy(response, (void*)C1222SecurityMechanism_2008, sizeof(C1222SecurityMechanism_2008));
    response += sizeof(C1222SecurityMechanism_2008);
    responseLength += (Unsigned16)(1 + sizeof(C1222SecurityMechanism_2008));
    
    *response++ = 0x05;  // session control
    *response++ = 0x80;  // 0 sessions and sessionless supported
    responseLength += (Unsigned16)2;
    
    *response++ = 0x06;  // device class
    *response++ = 0x0d;   // relative encoding of device class
    *response++ = 4;     // 4 byte length of device class
    Table00_GetMeterDeviceClass(response);
    response += 4;
    responseLength += (Unsigned16)7;
    
    *response++ = 0x07; // identity
    *response++ = 21;   // identity length
    *response++ = 0x01; // char-format is 7 bit ascii
    memcpy(response, myId, 20);
    response += 20;
    responseLength += (Unsigned16)23;
    
    *response = 0;     // end of feature list
    responseLength++;

    // return length of encoding of this response
    
    return responseLength;
}


Boolean C1222Server_ProcessPsemRequest( C1222Server* pServer, Unsigned8* request, Unsigned16 requestLength, Unsigned8* response, Unsigned16 maxResponseLength, Unsigned16* pResponseLength)
{
    Unsigned8 command;
    Unsigned16 tableId;
    Unsigned8 myId[20];
    Unsigned16 length;
    Unsigned8 result = C1222RESPONSE_OK;
    Unsigned32 offset;
    
    C1219TableServer* pTableServer = pServer->pTableServer;
    
    *pResponseLength = 1;   // by default - reads will override this
    
    // psem can now read and write chunks larger than 65535 bytes
    // but there is no way the meter can handle that
    
    command = request[0];
    
    switch( command )
    {
    case 0x20:  // ident
        if ( requestLength == 1 )
        {
            C1222Server_GetMyIdentification( pServer, myId );
            *pResponseLength = C1222PsemIdentResponse(response, myId);
        }
        else
            result = C1222RESPONSE_IAR; // ?? right error?
        break;
        
    case 0x22: // disconnect
        // need to notify stack to disconnect!!!
        if ( requestLength == 1 )
        {
            result = C1222Server_HandleDisconnectRequest(pServer);
        }
        else
            result = C1222RESPONSE_IAR;
        break;
        
    case 0x24: // deregister
        result = C1222Server_HandleDeRegisterRequest(pServer, response, maxResponseLength, pResponseLength);
        //result = C1222RESPONSE_SNS; // deregister over the network is not supported
        break;
        
    case 0x30:  // full table read
        tableId = (Unsigned16)((request[1]<<8) | request[2]);
        
        result = C1219TableServer_Read(pTableServer, tableId, response, maxResponseLength, pResponseLength);
        
        break;
        
    case 0x3E: // default table read - should I support this?
        result = C1222RESPONSE_SNS;
        break;
        
    case 0x3f: // offset read
        tableId = (Unsigned16)((request[1]<<8) | request[2]);
        offset  = (Unsigned32)( ( ((Unsigned32)(request[3]))<<16) | (((Unsigned32)request[4])<<8) | ((Unsigned32)request[5]) );
        length  = (Unsigned16)((request[6]<<8) | request[7]);
        
        if ( (length+4) > maxResponseLength )
            result = C1222RESPONSE_ONP;
        else
            result = C1219TableServer_ReadOffset(pTableServer, tableId, offset, length, response, pResponseLength);
                
        break;

    case 0x40:  // full table write
        tableId = (Unsigned16)((request[1]<<8) | request[2]);
        length = (Unsigned16)((request[3]<<8) | (request[4]));
        
        result = C1219TableServer_Write(pTableServer, tableId, length, &request[5]);
        
        break;
        
    case 0x4F:
        tableId = (Unsigned16)((request[1]<<8) | request[2]);
        offset  = (Unsigned32)( ( ((Unsigned32)(request[3]))<<16) | (((Unsigned32)request[4])<<8) | ((Unsigned32)request[5]) );
        length  = (Unsigned16)((request[6]<<8) | request[7]);
        
        result = C1219TableServer_WriteOffset(pTableServer, tableId, offset, length, &request[8]);

        break;        
        
    default:
        result = C1222RESPONSE_SNS;
        break;
    }
    
    response[0] = result;
    
    return (Boolean)((result == C1222RESPONSE_OK) ? TRUE : FALSE);    
}





void C1222Server_SimpleResponse(C1222Server* pServer, ACSE_Message* pMsg, Unsigned8 response)
{
    C1222Stack_SimpleResponse(pServer->pStack, pMsg, response);
}




Boolean C1222Server_ProcessMeterMessage(C1222Server* pServer, ACSE_Message* pMsg, ACSE_Message* pResponseMessage)
{
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    
    Epsem epsem;
    Unsigned8 responseMode;
    Epsem responseEpsem;
        
    C1222AL* pAppLayer;
    
    Boolean wantMac;
    Boolean wantAuthenticate;
    Boolean wantMacIn;
    
    Unsigned8 keyId = 0;
    Unsigned8 key[24];
    Unsigned8 cipher;
    Unsigned8 iv[8];
    Unsigned8 ivLength;
    
    Unsigned16 userId;
    Unsigned8 password[20];    
    
    Boolean errorResponse;
    Boolean isNotification;
    Boolean isTest;
    Unsigned16 sequence;
    Unsigned8* buffer;
    Unsigned16 length;
    C1222MessageFailureCode reason;
    C1222StandardVersion requestStandard;
    
    
    C1219TableServer_GetClientUserAndPassword(pServer->pTableServer, &userId, password);
    
    pAppLayer = &(pServer->pStack->Xapplication);
    
    requestStandard = ACSE_Message_GetStandardVersion(pMsg);
    
    if ( !ACSE_Message_GetCallingApInvocationId(pMsg, &sequence) )
        sequence = 0;
        
    C1222AL_LogComEventAndBytes(pAppLayer, COMEVENT_C1222_RECEIVED_REQUEST, &sequence, sizeof(sequence));
    
    
    // if there are no keys in the meter, c12.22 gets client password
    
    #ifdef C1222_INCLUDE_DEVICE
    
    if ( C1222AL_GetNumberOfKeys(pAppLayer) > 0 )
    {
        // if there is no password in the meter, it should probably still work
    
        memset(password, 0, sizeof(password));
    }
    
    #endif // C1222_INCLUDE_DEVICE
    
    wantMac = wantMacIn = ACSE_Message_IsEncrypted(pMsg);
    wantAuthenticate = ACSE_Message_IsAuthenticated(pMsg);
    
    isNotification = ACSE_Message_IsNotification(pMsg);
    isTest = ACSE_Message_IsTest(pMsg);
    
    // if this message is a response, based on the presence of a called ap invocation id,
    // ignore it with no response to avoid getting into a ping-pong match with the 
    // comm module (actually happened).  
    
    // What happened before is that the optical port initiated a rflan table read which
    // timed out, but then the response finally came in.  Since the response had timed out
    // the client routine to handle responses did not take the message, so it got passed to
    // the server.  The response was ignored in the acse message, but that left an empty
    // message (no information content) that was sent to the rflan.  The rflan also processed
    // this and returned an empty response to it, etc.
    
    if ( ACSE_Message_GetCalledApInvocationId(pMsg, &sequence) )
    {
        pServer->processMessageResponse = C1222RESPONSE_OK;  // this prevents sending a message
        return FALSE;
    }
    
    // place the response epsem at the end of the message buffer
    
    pServer->responseEpsemBuffer = &pResponseMessage->buffer[MAX_ACSE_OVERHEAD_LENGTH];
    
    // to allow working with tables test bench, need an option to respond using the same
    // key as was used in the request
    // for now, just do that always

    // if encrypted or authenticated, get info needed to encrypt or authenticate, 
    // plus user info to establish access rights
    
    #ifdef C1222_INCLUDE_DEVICE
    
    if ( wantMac || wantAuthenticate )
    {
        if ( !ACSE_Message_GetEncryptionKeyId(pMsg, &keyId) )
            keyId = 0;
            
        if ( C1222AL_GetKey(pAppLayer, keyId, &cipher, key) )
        {
            
            // to decode the user info we need the iv in the sent message
            
            if ( !ACSE_Message_GetIV(pMsg, iv, &ivLength) )
            {
                ivLength = 4;
                memset(iv, 0, ivLength);  // if none sent assume zero iv
            }
            
            if ( !ACSE_Message_GetUserInfo(pMsg, key, cipher, iv, ivLength, &userId, password, pAppLayer->tempBuffer) )
            {
                // the request message has either passed decryption or authentication
                // if no user info is in the message, assume default values
                                
                C1219TableServer_GetClientUserAndPassword(pServer->pTableServer, &userId, password);
            }            
            
            // now set up to encrypt / authenticate the response message
            
            ivLength = 4;
            C1222Misc_RandomizeBuffer(iv, ivLength);            
        }
        else  // if we don't have the keyid they asked for, can't encrypt or authenticate
        {
            wantMac = wantAuthenticate = FALSE;
        }
    }  // end if want encryption or authentication

    #else
    // avoid some warnings in the comm module
    ivLength = 4;
    cipher = 0;
    #endif // C1222_INCLUDE_DEVICE
    
    // find the app-data in the request message
    
    if ( ACSE_Message_GetApData(pMsg, &buffer, &length) )
    {
        if ( pMsg->standardVersion == CSV_MARCH2006 && wantMacIn )
        {
            tagIndex = 2;  // skip the mac at the start of the buffer
            tagLength = length - 2;    
        }
        else
        {
            tagIndex = 0;
            tagLength = length;
            if ( wantMacIn )
                tagLength -= (Unsigned16)2;  // skip the crc at the end of the apdata
        }
        
        // if the message is encrypted, the user information includes a <mac> before the <epsem>
        
        Epsem_Init(&epsem, &buffer[tagIndex], tagLength);
        
        if ( Epsem_Validate(&epsem) )
        {
            responseMode = Epsem_GetResponseMode(&epsem); 
            
            errorResponse = ProcessEpsemRequests(pServer, &epsem, userId, password,
                                  isTest, isNotification, &responseEpsem, requestStandard);           
                        
            // now that the epsem response is set up, start the response message
            
            if ( (responseMode == C1222RESPOND_ALWAYS) ||
                 ((responseMode == C1222RESPOND_ONEXCEPTION) && errorResponse) )
            {
                return BuildResponse(pServer, pMsg, &responseEpsem, pResponseMessage, 
                                     pAppLayer, wantMac, wantAuthenticate,
                                     keyId, iv, ivLength, cipher, key);
            }
            
            // else no response needed
            
            pServer->processMessageResponse = C1222RESPONSE_OK;
            
        } // end if the epsem validated
        else
        {
            reason = MFC_RECEIVED_EPSEM_INVALID;
            C1222AL_LogComEventAndBytes(pAppLayer, COMEVENT_C1222_SEND_RESPONSE_FAILED, &reason, sizeof(reason)); // invalid epsem in request
            pServer->processMessageResponse = C1222RESPONSE_IAR; //this will cause an iar response to be sent
        }
              
    } // end if found the information element
    else
    {
        reason = MFC_RECEIVED_INFORMATION_ELEMENT_MISSING;
        C1222AL_LogComEventAndBytes(pAppLayer, COMEVENT_C1222_SEND_RESPONSE_FAILED, &reason, sizeof(reason)); // missing info element
        pServer->processMessageResponse = C1222RESPONSE_IAR; // this will cause an iar to be sent
    }

    return FALSE;
}







void C1222Server_ProcessMessage(C1222Server* p, ACSE_Message* pMsg, Boolean* pMessageWasSent, Boolean bDoNotNoteReceivedMessageHandlingIsComplete)
{
    ACSE_Message responseMessage;
    Unsigned8* nativeAddress;
    Unsigned8 addressLength;
    C1222AL* pAppLayer = (C1222AL*)(&p->pStack->Xapplication);
    C1222MessageFailureCode reason;
    C1222MessageLogEntry logEntry;
    
    *pMessageWasSent = FALSE;

    ACSE_Message_SetBuffer(&responseMessage, p->meterResponseBuffer, MAX_ACSE_MESSAGE_LENGTH);
    ACSE_Message_SetStandardVersion( &responseMessage, ACSE_Message_GetStandardVersion(pMsg) );
    
    C1222AL_MakeMessageLogEntry(pAppLayer, pMsg, &logEntry, FALSE, C1222_MSGTYPE_REQUEST);  // this is a receiving log entry
    C1222AL_LogComEventAndBytes(pAppLayer, COMEVENT_RECEIVED_MESSAGE_FROM, &logEntry, sizeof(logEntry));        
    

    if ( C1222Server_ProcessMeterMessage(p, pMsg, &responseMessage) )
    {
        pAppLayer->lastMessageLog.responseReadyToSend = C1222Misc_GetFreeRunningTimeInMS();
        
        if ( !bDoNotNoteReceivedMessageHandlingIsComplete && !p->sendToNonC1222Stack )
            C1222Stack_NoteReceivedMessageHandlingIsComplete(p->pStack);

        if ( C1222Stack_GetReceivedMessageNativeAddress(p->pStack, &addressLength, &nativeAddress) )
        {
#ifdef C1222_COMM_MODULE 
            if ( 0 == memcmp(nativeAddress,  p->pStack->Xapplication.transportConfig.broadcastAddress, addressLength) )
            {
                nativeAddress = p->pStack->Xapplication.transportConfig.nativeAddress;
                addressLength = p->pStack->Xapplication.transportConfig.nativeAddressLength;
            }
               
            if ( p->sendToNonC1222Stack )
                *pMessageWasSent = C1222Stack_SendMessageToNonC1222Stack(&responseMessage, nativeAddress, addressLength);
            else
#endif                
                *pMessageWasSent = C1222Stack_SendMessage(p->pStack, &responseMessage, nativeAddress, addressLength);
                
            C1222AL_MakeMessageLogEntry(pAppLayer, &responseMessage, &logEntry, TRUE, C1222_MSGTYPE_RESPONSE);  // this is a receiving log entry
            C1222AL_LogComEventAndBytes(pAppLayer, COMEVENT_SENT_MESSAGE_TO, &logEntry, sizeof(logEntry));        
            
            C1222AL_LogComEvent(pAppLayer, COMEVENT_C1222_SENT_RESPONSE);
                
            if ( *pMessageWasSent )
                C1222AL_LogComEvent(pAppLayer, COMEVENT_C1222_SEND_RESPONSE_SUCCESS);
            else
            {
                reason = MFC_SEND_MESSAGE_FAILED;
                C1222AL_LogComEventAndBytes(pAppLayer, COMEVENT_C1222_SEND_RESPONSE_FAILED, 
                                            &reason, sizeof(reason));
            }
        }        
    }
    else if ( !p->sendToNonC1222Stack )
    {
        if ( p->processMessageResponse != C1222RESPONSE_OK )
            C1222Server_SimpleResponse(p, pMsg, p->processMessageResponse);
        else
            C1222Stack_NoteReceivedMessageHandlingIsComplete(p->pStack);
    }
        
    // log message handling here to take care of the cases when there is no response
    
    C1222AL_LogComEventAndBytes(pAppLayer, COMEVENT_C1222_PROCESS_RCVD_MSG_TIMING, &pAppLayer->lastMessageLog,
                 sizeof(pAppLayer->lastMessageLog));
}





void C1222Server_SetDestinationToForeignStack(C1222Server* p, Boolean wantForeignStack)
{
    p->sendToNonC1222Stack = wantForeignStack;
}



void C1222Server_Init(C1222Server* p, C1222Stack* pStack, C1219TableServer* pTableServer,
                      Unsigned8* responseBuffer)
{
    p->pStack = pStack;
    p->sendToNonC1222Stack = FALSE;
    p->pTableServer = pTableServer;
    p->isCommModule = FALSE;
    p->isRelay = FALSE;
    p->meterResponseBuffer = responseBuffer;
}



void C1222Server_SetIsCommModule(C1222Server* p, Boolean isCommModule)
{
    p->isCommModule = isCommModule;
}


void C1222Server_SetIsRelay(C1222Server* p, Boolean isRelay)
{
    p->isRelay = isRelay;
}




void C1222Server_NotePowerUp(C1222Server* p, Unsigned32 outageDurationInSeconds)
{
    (void)p;
    (void)outageDurationInSeconds;
}


Unsigned16 C1222Server_encodeUIDTerm(Unsigned32 term, Unsigned8* uid, Unsigned16 length)
{
    // returns length appropriate to put in aptitle[1]
    Boolean forced = FALSE;
    
    uid += length + 2;
    
    if ( term & 0xF0000000 )
    {
        *uid++ = (Unsigned8)(0x80 | (term>>28));
        forced = TRUE;
        length++;
    }
    
    term <<= 4;
    term >>= 4;
    
    if ( forced || (term>>21) )
    {
        *uid++ = (Unsigned8)(0x80 | (term>>21));
        forced = TRUE;
        length++;
    }
    
    term <<= 11;
    term >>= 11;
    
    if ( forced || (term>>14) )
    {
        *uid++ = (Unsigned8)(0x80 | (term>>14));
        forced = TRUE;
        length++;
    }
    
    term <<= 18;
    term >>= 18;
    
    if ( forced || (term>>7) )
    {
        *uid++ = (Unsigned8)(0x80 | (term>>7));
        length++;
    }
    
    term <<= 25;
    term >>= 25;
    
    *uid = (Unsigned8)term;
    length++;
    return length;    
}


void C1222Server_MakeRelativeMacAddressFormApTitle(C1222Server* p, Unsigned8* apbuffer, Boolean wantCellID)
{
    Unsigned8 nativeAddress[C1222_NATIVE_ADDRESS_LENGTH];
    Unsigned8 nativeAddressLength;
    Unsigned32 macAddress = 0;
    Unsigned16 cellid;
    
    nativeAddressLength = C1222AL_GetMyNativeAddress(&p->pStack->Xapplication, nativeAddress, sizeof(nativeAddress));
    
    if ( nativeAddressLength > sizeof(macAddress) )
        nativeAddressLength = sizeof(macAddress);

    memcpy(&macAddress, nativeAddress, nativeAddressLength);
    
    cellid = p->pStack->Xapplication.status.currentRFLanCellId;
    
    if ( wantCellID )
    {
        apbuffer[0] = 0x0d;
        apbuffer[1] = C1222Server_encodeUIDTerm(cellid, apbuffer, 0);
        apbuffer[1] = C1222Server_encodeUIDTerm(macAddress, apbuffer, apbuffer[1]);
    }
    else
    {
        apbuffer[0] = 0x0d;
        apbuffer[1] = C1222Server_encodeUIDTerm(macAddress, apbuffer, 0);
    }
    
    if ( p->isCommModule )
        apbuffer[1] = C1222Server_encodeUIDTerm(1, apbuffer, apbuffer[1]);
}




Boolean C1222Server_IsMessageForMe(C1222Server* p, ACSE_Message* pMsg)
{
    // if there is no called aptitle, its for me
    // if the called aptitle is my aptitle, its for me
    
    C1222ApTitle calledApTitle;
    C1222ApTitle nodeApTitle;
    C1222ApTitle ap1;
    C1222ApTitle ap2;
    C1222ApTitle ap3;
    int difference;
    Unsigned8 ap1buffer[C1222_APTITLE_LENGTH];
    Unsigned8 ap2buffer[C1222_APTITLE_LENGTH];
    Unsigned8 ap3buffer[C1222_APTITLE_LENGTH];
#ifdef C1222_INCLUDE_DEVICE    
    Unsigned8 numberOfMulticasts;    
    Unsigned8 ii;
#endif  

    p->useCalledApTitleFromRequestForCallingInResponse = FALSE;
    p->useRelativeMacAddressFormForCallingApTitle = FALSE;
    
    if ( ! ACSE_Message_GetCalledApTitle(pMsg, &calledApTitle) )
    {
        if ( p->isRelay )
            return FALSE; // otherwise we consume registration requests
            
        if ( !p->isCommModule ) 
            return TRUE;   // no called aptitle, must be for the device
    } 
    
    // should I get the aptitle from the registration status or the configuration?
    
    C1222ApTitle_Construct(&nodeApTitle, p->pStack->Xapplication.registrationStatus.nodeApTitle, C1222_APTITLE_LENGTH);
    
    nodeApTitle.length = (Unsigned16)(nodeApTitle.buffer[1] + 2);
    
    C1222ApTitle_Construct(&ap1, ap1buffer, sizeof(ap1buffer));
    C1222ApTitle_Construct(&ap2, ap2buffer, sizeof(ap2buffer));
    
    if ( !C1222ApTitle_MakeRelativeFrom(&ap1, &nodeApTitle) ||
         !C1222ApTitle_MakeRelativeFrom(&ap2, &calledApTitle) )
        return FALSE; // can't make relative - fail
        
    
    if ( C1222ApTitle_Compare(&ap1, &ap2, &difference) )
    {
        if ( difference == 0 )
        {
            p->useCalledApTitleFromRequestForCallingInResponse = TRUE;  // why not
            return TRUE;  // called aptitle is same as my aptitle
        }
    }
    else
        return FALSE; // error in compare of called and my aptitle
        
    
    if ( C1222ApTitle_IsRelative(&calledApTitle) )
    {
        // check for .macAddress for the register and for .macAddress.1 for the comm module
        
        C1222Server_MakeRelativeMacAddressFormApTitle(p, ap3buffer, FALSE);
            
        C1222ApTitle_Construct(&ap3, ap3buffer, sizeof(ap3buffer));
        
        if ( C1222ApTitle_Compare(&ap3, &ap2, &difference) )
        {
            if ( difference == 0 )
            {
                p->useCalledApTitleFromRequestForCallingInResponse = TRUE;  // why not
                return TRUE;  // called aptitle is same as my aptitle
            }
            
            C1222Server_MakeRelativeMacAddressFormApTitle(p, ap3buffer, TRUE);
            C1222ApTitle_Construct(&ap3, ap3buffer, sizeof(ap3buffer));
            if ( C1222ApTitle_Compare(&ap3, &ap2, &difference) )
            {
                if ( difference == 0 )
                {
                    p->useCalledApTitleFromRequestForCallingInResponse = TRUE;  // why not
                    return TRUE;  // called aptitle is same as my aptitle
                }
            }
            else
                return FALSE;
            
        }
        else
            return FALSE; // error in compare of called and my aptitle
            
        // if this is the comm module, check for .0.1
#ifdef C1222_INCLUDE_COMMMODULE        
        if ( p->isCommModule )
        {
            // already know it is relative
            // since this is a broadcast better use our mac for the response
            
            if ( ap2buffer[1]==2 && ap2buffer[2]==0 && ap2buffer[3]==1 )
            {
                p->useRelativeMacAddressFormForCallingApTitle = TRUE;
                return TRUE;
            }
        }                
#endif // C1222_INCLUDE_COMMMODULE        
    }
    

#ifdef C1222_INCLUDE_DEVICE

    if ( p->isCommModule )
        return FALSE;
        
    if ( C1222ApTitle_IsBroadcastToMe(&ap2, &ap1) )  // both params must be relative
    {
        p->pStack->Xapplication.counters.receivedBroadCastForMeCount++;
        p->pStack->Xapplication.status.timeOfLastBroadCastForMe = C1222Misc_GetFreeRunningTimeInMS();
        return TRUE;
    }
     
    // don't really need to check here if its a multicast address since if not it should not be in table 122 -
    // just treat any address in table 122 as a multicast
       
    //if ( ! C1222ApTitle_IsAMulticastAddress(&ap2) )
    //    return FALSE;
    
        
    // check if the called aptitle is one of the multicast addresses
    
    numberOfMulticasts = Table122_GetNumberOfMulticastAddresses();
    
    for ( ii=0; ii<numberOfMulticasts; ii++ )
    {
        if ( Table122_GetMulticastAddress(ii, &ap1) )
        {
            if ( C1222ApTitle_Compare(&ap1, &ap2, &difference) )
            {
                if ( difference == 0 )
                {
                    p->pStack->Xapplication.counters.receivedMultiCastForMeCount++;
                    p->pStack->Xapplication.status.timeOfLastMultiCastForMe = C1222Misc_GetFreeRunningTimeInMS();
                    return TRUE;
                }
            }
        }
    }
#endif    
    
    // also check if this is a broadcast or multicast
    
	return FALSE;
}





#ifdef C1222_INCLUDE_COMMMODULE
void ConstructCommModuleApTitle(C1222AL* pAppLayer, C1222ApTitle * papTitle)
{
    C1222ApTitle apNodeTitle;
    //
    // Get the node ApTitle.  Our CommModule is at NodeApTitle.1
    //
    C1222AL_ConstructCallingApTitle(pAppLayer, &apNodeTitle);
    if ( !C1222ApTitle_MakeRelativeFrom(papTitle, &apNodeTitle) )
        return;
    papTitle->buffer[papTitle->buffer[1] + 2] = 1;
    papTitle->buffer[1]++;
    papTitle->length = C1222ApTitle_GetLength(papTitle);

}
#endif






Boolean BuildResponse(C1222Server* pServer, ACSE_Message* pMsg, Epsem* pResponseEpsem, ACSE_Message* pResponseMessage, 
            C1222AL* pAppLayer, Boolean wantMac, Boolean wantAuthenticate, Unsigned8 keyId, 
            Unsigned8* iv, Unsigned8 ivLength, Unsigned8 cipher, Unsigned8* key)
{
    // start the response message
    // copy calling to called aptitle
    // set calling aptitle to local aptitle
    // can skip the calling entity qualifier for the moment since
    //    assume this is not a test, it is not a notification, and not urgent
    // set called apinvocation id to calling apinvocation id
    // set calling apinvocation id to next sequence number
    // assume for the moment that it will not be segmented
    
    C1222ApTitle apTitle;
    Unsigned8 qualByte;
    Unsigned16 server_sequence;
    Unsigned16 client_sequence;
    Boolean ok = TRUE;
    Unsigned8 result;
    Unsigned8 CommModuleApTitleBuffer[C1222_APTITLE_LENGTH];
    Unsigned8 ApTitleBuffer2[C1222_APTITLE_LENGTH];
    C1222ApTitle apTitle2;
    Boolean want_absolute_aptitle = TRUE;
    Boolean respondIn2006Format = pMsg->standardVersion == CSV_MARCH2006 ? TRUE : FALSE;  // the request format should already be determined


    server_sequence = C1222AL_GetNextApInvocationId(pAppLayer);
    
    if ( ACSE_Message_HasContext(pMsg) )
    {
        ACSE_Message_SetContext( pResponseMessage );   
    }
    
    if ( ACSE_Message_GetCallingApTitle( pMsg, &apTitle) )
    {
        ACSE_Message_SetCalledApTitle( pResponseMessage, &apTitle );
        
        if ( C1222ApTitle_IsRelative(&apTitle) )
            want_absolute_aptitle = FALSE;
    }
    
    if ( pMsg->standardVersion == CSV_2008 && ACSE_Message_GetCallingApInvocationId( pMsg, &client_sequence ) )
    {
        ACSE_Message_SetCalledApInvocationId( pResponseMessage, client_sequence );
    }
    
    
    if ( pServer->useCalledApTitleFromRequestForCallingInResponse )
    {
        if ( ACSE_Message_GetCalledApTitle( pMsg, &apTitle) )
        {
            ACSE_Message_SetCallingApTitle( pResponseMessage, &apTitle );
        }
        // else - oh oh no called aptitle - should not happen
    }
    else if ( pServer->useRelativeMacAddressFormForCallingApTitle )
    {
        C1222Server_MakeRelativeMacAddressFormApTitle(pServer, CommModuleApTitleBuffer, FALSE);
        C1222ApTitle_Construct(&apTitle, CommModuleApTitleBuffer, sizeof(CommModuleApTitleBuffer));
    }
    else
    {    
        if ( pAppLayer->status.isCommModule )
        {
#ifdef C1222_INCLUDE_COMMMODULE            
            C1222ApTitle_Construct(&apTitle, CommModuleApTitleBuffer, sizeof(CommModuleApTitleBuffer));
            ConstructCommModuleApTitle(pAppLayer, &apTitle);
            //ACSE_Message_SetCallingApTitle( pResponseMessage, &apTitle );
#endif // C1222_INCLUDE_COMMMODULE            
        }
        else
            C1222AL_ConstructCallingApTitle(pAppLayer, &apTitle);
            
        C1222ApTitle_Construct(&apTitle2, ApTitleBuffer2, sizeof(ApTitleBuffer2));
            
        if ( want_absolute_aptitle )
        {
            // convert to absolute if possible
            
            if ( C1222ApTitle_MakeAbsoluteFrom(&apTitle2, &apTitle, respondIn2006Format) )
                ACSE_Message_SetCallingApTitle( pResponseMessage, &apTitle2); 
            else
                ACSE_Message_SetCallingApTitle( pResponseMessage, &apTitle); 
        }
        else
        {
            // convert to relative if possible
            
            if ( C1222ApTitle_MakeRelativeFrom(&apTitle2, &apTitle) )
                ACSE_Message_SetCallingApTitle( pResponseMessage, &apTitle2); 
            else
                ACSE_Message_SetCallingApTitle( pResponseMessage, &apTitle); 
        }
    }
    
    if ( ACSE_Message_GetCallingAeQualifier( pMsg, &qualByte) )
    {
        // we support test and notification, not urgent
        
        // TODO - is notification right here?
        
        qualByte &= C1222QUAL_TEST | C1222QUAL_NOTIFICATION;
        
        ACSE_Message_SetCallingAeQualifier( pResponseMessage, qualByte);
    }
    
    if ( pMsg->standardVersion == CSV_2008 )
    {
  #ifdef TTB_BUG_INVOKEIDONLYIFSEGMENTED
        // assume the message will be segmented if response epsem is more than 100 bytes
        if ( pResponseEpsem->length > 100 )
        {
            ACSE_Message_SetCallingApInvocationId( pResponseMessage, server_sequence );
        }
  #else
        // Tables test bench errors out when this element is included
        // but it is currently required by the standard
        // TODO: add this back in when ttb supports it
    
    #ifndef AVOID_TTB_BUG_SKIPCALLINGSEQONRESPONSE
        ACSE_Message_SetCallingApInvocationId( pResponseMessage, server_sequence );
    #endif

  #endif
    }
    
    #ifdef C1222_INCLUDE_DEVICE // no encryption or authentication yet in comm module
    if ( wantMac )
    {
        ACSE_Message_SetAuthValueForEncryption( pResponseMessage, keyId, iv, ivLength);
    }
    else if ( wantAuthenticate )
    {
        ACSE_Message_SetInitialAuthValueForAuthentication(pResponseMessage, keyId, iv, ivLength);
    }
    #else
    (void)keyId;
    (void)iv;
    (void)ivLength;
    (void)key;
    (void)wantAuthenticate;
    (void)wantMac;
    (void)cipher;
    #endif
    
    if ( pMsg->standardVersion == CSV_MARCH2006 && ACSE_Message_GetCallingApInvocationId( pMsg, &client_sequence ) )
    {
        ACSE_Message_SetCalledApInvocationId( pResponseMessage, client_sequence );
    }
    
    
    if ( pMsg->standardVersion == CSV_MARCH2006 )
    {
  #ifdef TTB_BUG_INVOKEIDONLYIFSEGMENTED
        // assume the message will be segmented if response epsem is more than 100 bytes
        if ( pResponseEpsem->length > 100 )
        {
            ACSE_Message_SetCallingApInvocationId( pResponseMessage, server_sequence );
        }
  #else
        // Tables test bench errors out when this element is included
        // but it is currently required by the standard
        // TODO: add this back in when ttb supports it
    
    #ifndef AVOID_TTB_BUG_SKIPCALLINGSEQONRESPONSE
        ACSE_Message_SetCallingApInvocationId( pResponseMessage, server_sequence );
    #endif

  #endif

    }
    result = ACSE_Message_SetUserInformation( pResponseMessage, pResponseEpsem, wantMac); 
    
    if ( result == C1222RESPONSE_OK )
    {
        pAppLayer->lastMessageLog.responseBuiltPreEncryption = C1222Misc_GetFreeRunningTimeInMS();
        
        #ifdef C1222_INCLUDE_DEVICE // no encryption or authentication yet in comm module
        if ( wantMac )
        {
            ok = ACSE_Message_Encrypt( pResponseMessage, cipher, key, iv, ivLength, pAppLayer->tempBuffer,
                            C1222AL_KeepAliveForEncryption, pAppLayer);
        }
        else if ( wantAuthenticate )
        {
            ok = ACSE_Message_UpdateAuthValueForAuthentication(pResponseMessage, cipher, key, iv, ivLength, pAppLayer->tempBuffer);
        }
        #endif
    }
    else
    {
        C1222AL_SimpleResponse(pAppLayer, pMsg, result, pResponseMessage->maxLength);
        ok = FALSE;
    }

    // ok determines if there will be a response

    return ok;
}






Boolean ProcessEpsemRequests(C1222Server* pServer, Epsem* pEpsem, Unsigned16 userId, 
            Unsigned8* password, Boolean isTest, Boolean isNotification, 
            Epsem* pResponseEpsem, C1222StandardVersion requestStandard)
{                       
    // still need to process the request message even if we do not want to respond
    
    Unsigned8* buffer;
    Unsigned16 length;
    Unsigned8* responseBuffer;
    Unsigned16 responseEpsemLength;
    Unsigned16 maxResponseLength;
    Unsigned16 responseLength;
    Boolean errorResponse = FALSE;
    Unsigned8 sizeofLength;
    Unsigned8 offset;
    
    Unsigned16 ii;
    
    pServer->responseEpsemBuffer[0] = 0x82;   // 0x80 = c12.22 requirement, 0x02 means respond never - appropriate for a response
    responseBuffer = &pServer->responseEpsemBuffer[1];
    responseEpsemLength = 0;
    
    C1219TableServer_StartUserAccess(pServer->pTableServer, userId, password);

    C1219TableServer_NoteTest(pServer->pTableServer, isTest);
    C1219TableServer_NoteNotification(pServer->pTableServer, isNotification);
    C1219TableServer_NoteRequestStandard(pServer->pTableServer, requestStandard);

    // process all of the epsem requests and respond to them
    
    while( Epsem_GetNextRequestOrResponse( pEpsem, &buffer, &length) )
    {
        // ignore responses
        
        if ( buffer[0] >= 0x20 )
        {
            // the 4 is for 0x80 and up to 3 length bytes at the start of the epsem message
            maxResponseLength = (Unsigned16)(MAX_EPSEM_MESSAGE_LENGTH - 4 - responseEpsemLength);
            
            // the 3 bytes are 0x80 one length byte, and the error code
            if ( responseEpsemLength >= (MAX_EPSEM_MESSAGE_LENGTH-3) )
            {
                errorResponse = TRUE;
                break; // no room for any response - should try to prevent this below
            }
            else if ( maxResponseLength < 5 )
            {
                // no room for RSTL response which requires the max response length to be returned in the response
                // as a UINT32 so takes 5 bytes
                responseBuffer[0] = 1;
                responseBuffer[1] = C1222RESPONSE_ERR;
                errorResponse = TRUE;
                //responseBuffer += 2;
                responseEpsemLength += (Unsigned16)2;
                
                break;
            }
        
            if ( !C1222Server_ProcessPsemRequest(pServer, buffer, length, &responseBuffer[1],
                            maxResponseLength, &responseLength) )
                errorResponse = TRUE;
            
            sizeofLength = C1222Misc_GetSizeOfLengthEncoding(responseLength);
        
        
            if ( sizeofLength == 1 )
            {
                responseBuffer[0] = responseLength;
                responseBuffer += 1 + responseLength;
                responseEpsemLength += (Unsigned16)(1+responseLength);
            }
            else
            {
                // need to shift up the response
            
                offset = (Unsigned8)(sizeofLength - 1); // how many bytes to shift up the message
            
                for ( ii=(Unsigned16)(responseLength+offset); ii>offset; ii-- )
                    responseBuffer[ii] = responseBuffer[ii-offset];
                
                C1222Misc_EncodeLength(responseLength, responseBuffer);
                responseBuffer += sizeofLength + responseLength;
                responseEpsemLength += (Unsigned16)(sizeofLength + responseLength); // fix for epsem length being wrong if need to shift response
            }
        }
        else
            C1222AL_LogEventAndBytes(&pServer->pStack->Xapplication, C1222EVENT_SERVER_IGNORINGRESPONSE, length, buffer);
        
    } // end while more requests or responses
    
    C1219TableServer_EndUserAccess(pServer->pTableServer);
    
    Epsem_Init( pResponseEpsem, pServer->responseEpsemBuffer, (Unsigned16)MAX_EPSEM_MESSAGE_LENGTH);
    
    pResponseEpsem->length = responseEpsemLength; 
    
    return errorResponse;           
}
