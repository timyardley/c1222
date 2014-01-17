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
#include "c1222event.h"



//**************************************************************************************
// Message Sender
//**************************************************************************************
//
// The message sender is used to send a message from the comm module to the device, or
// from the device to the comm module

void C1222MessageSender_Init(C1222MessageSender* p)
{
    memset(p, 0, sizeof(C1222MessageSender));
    C1222ALSegmenter_Init(&p->transmitSegmenter);
}

void C1222MessageSender_SetLinks(C1222MessageSender* p, void* pTransport, void* pApplication)
{
    p->pTransport = (C1222TL*)pTransport;
    p->pApplication = (C1222AL*)pApplication;
}



void C1222MessageSender_LogEvent(C1222MessageSender* p, C1222EventId event)
{
    C1222AL_LogEvent(p->pApplication, event);
}



Boolean C1222MessageSender_StartSend(C1222MessageSender* p, ACSE_Message* pMsg, Unsigned8* nativeAddress, Unsigned16 nativeAddressLength)
{
    C1222ALSegmenter* pSegmenter = &p->transmitSegmenter;
    Unsigned16 fragmentSize;
    
    p->workingSegmentIndex = 0;
    
    if ( nativeAddressLength > C1222_NATIVE_ADDRESS_LENGTH )
    {
        C1222AL_LogFailedSendMessage((C1222AL*)(p->pApplication), MFC_SEND_INVALID_NATIVE_ADDRESS, pMsg);
        return FALSE;
    }
        
    // fragment size is limited to whatever we can get through the datalink
    fragmentSize = C1222TL_GetMaxSendMessagePayloadSize((C1222TL*)(p->pTransport));
    
    // and to the size of the buffer we use to store the fragments
    
    if ( fragmentSize > MAX_ACSE_SEGMENTED_MESSAGE_LENGTH )
        fragmentSize = MAX_ACSE_SEGMENTED_MESSAGE_LENGTH;
    
    C1222ALSegmenter_SetFragmentSize( pSegmenter, fragmentSize );
        
    memcpy(p->nativeAddress, nativeAddress, nativeAddressLength);
    p->nativeAddressLength = nativeAddressLength;
        
    if ( C1222ALSegmenter_SetUnsegmentedMessage( pSegmenter, pMsg) )    
    {
        p->triesLeft = C1222_APPLICATION_RETRIES;
        
        return C1222MessageSender_ContinueSend(p);
    }
    
    C1222MessageSender_LogEvent( p, C1222EVENT_ALS_SET_UNSEG_MSG_FAILED);
    
    C1222AL_LogFailedSendMessage((C1222AL*)(p->pApplication), MFC_SEND_SET_UNSEGMENTED_MESSAGE_FAILED, pMsg);
    
    return FALSE;
}



Boolean C1222MessageSender_TriesLeft(C1222MessageSender* p)
{
    return p->triesLeft;
}




Boolean C1222MessageSender_RestartSend(C1222MessageSender* p)
{
    C1222AL* pApplication = (C1222AL*)(p->pApplication);

    if ( p->triesLeft )
    {
        C1222MessageSender_LogEvent( p, C1222EVENT_ALS_RESTARTING_SEND);
        
        p->triesLeft--;
        pApplication->counters.sendRetried++;
        
        p->workingSegmentIndex = 0;
        
        // this can be a little bit recursive
        // the calls are limited by the tries left
    
        return C1222MessageSender_ContinueSend(p);
    }
    
    C1222MessageSender_LogEvent(p, C1222EVENT_ALS_ALL_RETRIES_FAILED);
    
    C1222AL_LogFailedSendMessage((C1222AL*)(p->pApplication), MFC_SEND_ALL_RETRIES_FAILED, 
                     C1222ALSegmenter_GetWorkingMessage(&p->transmitSegmenter));
    
    return FALSE;
}



ACSE_Message* C1222MessageSender_GetWorkingMessage(C1222MessageSender* p)
{
    return C1222ALSegmenter_GetWorkingMessage(&p->transmitSegmenter);
}






#ifdef C1222_WANT_SHORT_MESSAGES

Boolean C1222MessageSender_SendShortMessage(C1222MessageSender* p, ACSE_Message* pMsg, Unsigned8* nativeAddress, Unsigned16 nativeAddressLength)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    Boolean ok;
    
    if ( nativeAddressLength > C1222_NATIVE_ADDRESS_LENGTH )
        return FALSE;
        
    memcpy(p->nativeAddress, nativeAddress, nativeAddressLength);
    p->nativeAddressLength = nativeAddressLength;
    
    ok = C1222TL_SendShortMessage( pTransport, pMsg->buffer, pMsg->length, p->nativeAddress, 
                       p->nativeAddressLength);
       
    return ok;
}

#endif








// this is used to send an error response when the epsem could not be parsed for some reason
// like a segmentation error or an encryption or authentication error.  But the reference message in this case
// is a full message response already - so get called params from called and calling from calling


void C1222MessageSender_SimpleResponse(C1222MessageSender* p, ACSE_Message* pMsg, Unsigned8 response)
{
    Unsigned8 nativeAddressLength;
    Unsigned8* nativeAddress;
    
    // assume no encryption or authentication
    
    C1222ApTitle apTitle;
    ACSE_Message responseMessage;
    Epsem responseEpsem;
    Unsigned16 sequence;
    Unsigned8 responseEpsemBuffer[3];
    Unsigned8 qualByte;
    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    Unsigned8 responseBuffer[110];
    Unsigned8 result;
    
    ACSE_Message_SetBuffer( &responseMessage, responseBuffer, sizeof(responseBuffer) );
    ACSE_Message_SetStandardVersion( &responseMessage, ACSE_Message_GetStandardVersion(pMsg) );
    
    if ( ACSE_Message_HasContext(pMsg) )
    {
        ACSE_Message_SetContext( &responseMessage );   
    }
    
    if ( pMsg->standardVersion == CSV_MARCH2006 )
    {
        if ( ACSE_Message_GetCalledApTitle( pMsg, &apTitle) )
        {
            ACSE_Message_SetCalledApTitle( &responseMessage, &apTitle );
        }
        
        C1222AL_ConstructCallingApTitle(pApplication, &apTitle);
        
        
        ACSE_Message_SetCallingApTitle( &responseMessage, &apTitle);
        
        if ( ACSE_Message_GetCallingAeQualifier( pMsg, &qualByte) )
        {
            // we support test and notification, not urgent
                        
            qualByte &= C1222QUAL_TEST | C1222QUAL_NOTIFICATION;
                        
            ACSE_Message_SetCallingAeQualifier( &responseMessage, qualByte);
        }
        
            
        if ( ACSE_Message_GetCalledApInvocationId( pMsg, &sequence ) )
        {
            ACSE_Message_SetCalledApInvocationId( &responseMessage, sequence );
        }
        
        sequence = C1222AL_GetNextApInvocationId(pApplication);
        
        // Tables test bench errors out when this element is included
        // but it is currently required by the standard
        // the version of TTB that I have now has this bug fixed (scb 5/2006)
        
        #ifndef AVOID_TTB_BUG_SKIPCALLINGSEQONRESPONSE
        ACSE_Message_SetCallingApInvocationId( &responseMessage, sequence );
        #endif
    }
    else // 2008 standard
    {
        // different tag order unfortunately
        
        if ( ACSE_Message_GetCalledApTitle( pMsg, &apTitle) )
        {
            ACSE_Message_SetCalledApTitle( &responseMessage, &apTitle );
        }
        
        if ( ACSE_Message_GetCalledApInvocationId( pMsg, &sequence ) )
        {
            ACSE_Message_SetCalledApInvocationId( &responseMessage, sequence );
        }
        
        
        C1222AL_ConstructCallingApTitle(pApplication, &apTitle);
        
        
        ACSE_Message_SetCallingApTitle( &responseMessage, &apTitle);
        
        if ( ACSE_Message_GetCallingAeQualifier( pMsg, &qualByte) )
        {
            // we support test and notification, not urgent
                        
            qualByte &= C1222QUAL_TEST | C1222QUAL_NOTIFICATION;
                        
            ACSE_Message_SetCallingAeQualifier( &responseMessage, qualByte);
        }

        // Tables test bench errors out when this element is included
        // but it is currently required by the standard
        // the version of TTB that I have now has this bug fixed (scb 5/2006)
        
        #ifndef AVOID_TTB_BUG_SKIPCALLINGSEQONRESPONSE
        ACSE_Message_SetCallingApInvocationId( &responseMessage, sequence );
        #endif
                
        sequence = C1222AL_GetNextApInvocationId(pApplication);        
    }
    
    responseEpsemBuffer[0] = 0x82;
    responseEpsemBuffer[1] = 1;
    responseEpsemBuffer[2] = response; 
    
    Epsem_Init( &responseEpsem, responseEpsemBuffer, (Unsigned16)sizeof(responseEpsemBuffer));
            
    responseEpsem.length = 2;            
     
    result = ACSE_Message_SetUserInformation( &responseMessage, &responseEpsem, FALSE);
    
    C1222AL_LogComEventAndBytes(pApplication, COMEVENT_C1222_SENT_SIMPLE_ERROR_RESPONSE, &response, 1);
    
    pApplication->lastMessageLog.responseBuiltPreEncryption = C1222Misc_GetFreeRunningTimeInMS();
    
    if ( result == C1222RESPONSE_OK )
    {
        pApplication->lastMessageLog.responseReadyToSend = C1222Misc_GetFreeRunningTimeInMS();
        
        if ( C1222AL_GetReceivedMessageNativeAddress(pApplication, &nativeAddressLength, &nativeAddress) )  
            C1222AL_SendMessage(pApplication, &responseMessage, nativeAddress, nativeAddressLength); 
        else
            C1222MessageSender_LogEvent(p, C1222EVENT_ALS_CANT_GET_NATIVE_ADDRESS);
            
        // else need a diagnostic (TODO)
        
        // any received message has already been noted complete (I think)
    }
    else
        C1222MessageSender_LogEvent(p, C1222EVENT_ALS_SET_UI_FAILED_IN_SIMPLE_RESPONSE);
    
    // else need a diagnostic - can't set user information (TODO)
        
    // this might look recursive, but we got here because of a segmentation problem and this simple
    // response should not have a segmentation problem 
}




Boolean C1222MessageSender_ContinueSend(C1222MessageSender* p)
{
    C1222ALSegmenter* pSegmenter = &p->transmitSegmenter;
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    Unsigned16 length;
    Boolean ok;
        
    if ( C1222ALSegmenter_GetSegment( pSegmenter, p->workingSegmentIndex, p->messageBuffer, 
                          MAX_ACSE_SEGMENTED_MESSAGE_LENGTH, &length ) )
    {
        // we've decided not to log the segments
        // C1222AL_LogComEventAndBytes((C1222AL*)(p->pApplication), COMEVENT_C1222_SENT_SEGMENT_BYTES, 
        //                             p->messageBuffer, length);

        ok = C1222TL_SendACSEMessage( pTransport, p->messageBuffer, length, p->nativeAddress, 
                       p->nativeAddressLength);
                   
        if ( ok )
            p->workingSegmentIndex++;
        else
        {
            C1222MessageSender_LogEvent(p, C1222EVENT_ALS_SENDNEXTSEGMENT_FAILED);
            
            ok = C1222MessageSender_RestartSend(p);
        }

        return ok;
    }
    
    C1222MessageSender_LogEvent(p, C1222EVENT_ALS_GET_SEG_FAILED);
    
    C1222AL_LogFailedSendMessage((C1222AL*)(p->pApplication), MFC_SEND_GET_SEGMENT_FAILED,
                C1222MessageSender_GetWorkingMessage(p));
    
    // I would like to send an error response here - need to construct it from the existing
    // full message - can only do this if it is a response though -can't send a response when no request was received
    
    if ( ACSE_Message_IsResponse(&pSegmenter->workingMessage) )
        C1222MessageSender_SimpleResponse( p, &pSegmenter->workingMessage, C1222RESPONSE_SGERR);
    
    return FALSE;   
}



Boolean C1222MessageSender_IsMessageComplete(C1222MessageSender* p)
{
    C1222ALSegmenter* pSegmenter = &p->transmitSegmenter;
    
    if ( p->workingSegmentIndex >= C1222ALSegmenter_GetNumberOfSegments( pSegmenter ) )
        return TRUE;
    else
        return FALSE; 
}
