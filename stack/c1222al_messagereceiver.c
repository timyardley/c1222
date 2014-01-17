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
// Message Receiver
//**************************************************************************************
//
// the message receiver is used in the comm module to receive a message from the meter, or in
// the meter, to receive a message from the comm module
// it is also currently used by the application layer of the comm module to store a message that the
// comm module application layer generated (like register request) until the stack retrieves it


Boolean C1222MessageReceiver_HandleReceiveSegmentTimeout(C1222MessageReceiver* p)
{
    return C1222ALSegmenter_HandleSegmentTimeout(&p->receiveSegmenter);
}


void C1222MessageReceiver_CancelPartialMessage(C1222MessageReceiver* p)
{
    C1222ALSegmenter_CancelPartialMessage(&p->receiveSegmenter);
}



void C1222MessageReceiver_Init(C1222MessageReceiver* p)
{
    memset(p, 0, sizeof(C1222MessageReceiver));
    C1222ALSegmenter_Init(&p->receiveSegmenter);
}


void C1222MessageReceiver_SetLinks(C1222MessageReceiver* p, void* pApplication)
{
    p->pApplication = pApplication;
}


void C1222MessageReceiver_LogEvent(C1222MessageReceiver* p, C1222EventId event)
{
    C1222AL_LogEvent((C1222AL*)(p->pApplication), event);
}


void C1222MessageReceiver_NoteMessageHandlingComplete(C1222MessageReceiver* p)
{
    C1222ALSegmenter_Clear(&p->receiveSegmenter);
}


// this stores a message into the message receiver - segmentation is ignored
// so a segmented message will be stored at the beginning of the buffer
// the destination native address is also stored
Unsigned8 C1222MessageReceiver_SetMessage(C1222MessageReceiver* p, 
                                          Unsigned8 nativeAddressLength, Unsigned8* nativeAddress, 
                                          ACSE_Message* pMsg)
{    
    // need to check for request too large
    
    if ( pMsg->length > MAX_ACSE_MESSAGE_LENGTH )
        return C1222RESPONSE_RQTL;
        
    // save the native address
    
    if ( nativeAddressLength > C1222_NATIVE_ADDRESS_LENGTH )
        nativeAddressLength = C1222_NATIVE_ADDRESS_LENGTH;    
    
    p->nativeAddressLength = nativeAddressLength;
    memcpy(p->nativeAddress, nativeAddress, nativeAddressLength);
    
    if ( !C1222ALSegmenter_SetUnsegmentedRxMessage(&p->receiveSegmenter, pMsg) )
    {
        C1222MessageReceiver_LogEvent(p, C1222EVENT_ALR_CANT_SET_UNSEG_MSG1);
        return C1222RESPONSE_SGERR;
    }
    
    return C1222RESPONSE_OK;
}



#if 0
// current versions of the rflan do not update the meter display, so there are no low priority messages from
// the rflan (scb 3/17/08)
Boolean IsLowPriorityMessageFromRFLAN(C1222MessageReceiver* p, ACSE_Message* pMsg)
{
    // first check if from rflan
    C1222ApTitle apTitle;
    Unsigned8 rflanApTitleBuffer[3];
    C1222ApTitle rflanApTitle;
    C1222ApTitle myApTitle;
    int difference;
    Unsigned8* buffer;
    Unsigned16 length;
    Epsem epsem;
    Unsigned8* epsemBuffer;
    Unsigned16 epsemLength;
    
    if ( ACSE_Message_GetCallingApTitle(pMsg, &apTitle) )
    {
        // check if this is the rflan default aptitle
        
        C1222ApTitle_Construct(&rflanApTitle, rflanApTitleBuffer, sizeof(rflanApTitleBuffer));
        
        // special aptitle (".1") since we are not registered
        
        rflanApTitleBuffer[0] = 0x0d;
        rflanApTitleBuffer[1] = 1;
        rflanApTitleBuffer[2] = 1;
        rflanApTitle.length = C1222ApTitle_GetLength(&rflanApTitle);
        
        if ( !C1222ApTitle_Compare(&apTitle, &rflanApTitle, &difference) )
            return FALSE;  // if I can't compare the aptitles, assume it is not the rflan
            
        // check if this is a branch of my aptitle
        
        if ( difference != 0 )
        {
            if ( C1222AL_GetMyApTitle((C1222AL*)(p->pApplication), &myApTitle) )
            {
                if ( !C1222ApTitle_IsApTitleMyCommModule(&apTitle, &myApTitle) )
                    return FALSE;
            }
            else
                return FALSE;  // if I can't get my aptitle, assume it is not the rflan
        }
        // else if difference is 0, it is the rflan
    }
    
    // if no calling aptitle, assume it is from the rflan
    
    // so this is a message from the rflan - now see if it is low priority - for now, any write will be considered low priority
    
    if ( ACSE_Message_IsEncrypted(pMsg) )
        return FALSE;  // wont be able to parse the message - rflan doesn't have encryption anyway
    
    if ( ACSE_Message_GetApData(pMsg, &buffer, &length) )
    {
        Epsem_Init(&epsem, buffer, length);
        
        if ( Epsem_GetNextRequestOrResponse(&epsem, &epsemBuffer, &epsemLength) )
        {
            if ( epsemBuffer[0] == 0x40 || epsemBuffer[0] == 0x4F )
                return TRUE;
        }
    }
    
    return FALSE;
}
#endif






Unsigned8 C1222MessageReceiver_AddSegment(C1222MessageReceiver* p, 
                                          Unsigned8 nativeAddressLength, Unsigned8* nativeAddress, 
                                          ACSE_Message* pMsg)
{
    Unsigned16 offset;
    Unsigned16 fullLength;
    Unsigned16 sequence;
    Boolean isSegmented;
    C1222AL* pApplication = (C1222AL*)(p->pApplication);
    
    // need to check for request too large
    
    if ( pMsg->length > MAX_ACSE_MESSAGE_LENGTH )
    {
        C1222MessageReceiver_LogEvent(p, C1222EVENT_ALR_ADDSEG_MSG_TOO_LONG);
        return C1222RESPONSE_RQTL;
    }
        
    if ( ACSE_Message_GetSegmentOffset(pMsg, &offset, &fullLength, &sequence) )
    {
        // this is a segmented message
        
        if ( fullLength > MAX_ACSE_MESSAGE_LENGTH )
        {
            C1222MessageReceiver_LogEvent(p, C1222EVENT_ALR_ADDSEG_FULLMSG_TOO_LONG);
            return C1222RESPONSE_RQTL;
        }
    }
    
    // save the native address
    
    if ( nativeAddressLength > C1222_NATIVE_ADDRESS_LENGTH )
        nativeAddressLength = C1222_NATIVE_ADDRESS_LENGTH;    
    
    p->nativeAddressLength = nativeAddressLength;
    memset(p->nativeAddress, 0, C1222_NATIVE_ADDRESS_LENGTH);
    memcpy(p->nativeAddress, nativeAddress, nativeAddressLength);
    
    // get segmentation info
    
    if ( !ACSE_Message_IsSegmented(pMsg, &isSegmented) )
    {
        C1222MessageReceiver_LogEvent(p, C1222EVENT_ALR_ADDSEG_GET_SEG_STATUS_ERR);
        return C1222RESPONSE_SGERR;
    }
        
    if ( isSegmented )
    {  
        if ( C1222ALSegmenter_IsSegmentAlreadyReceived(&p->receiveSegmenter, pMsg) )
        {
            pApplication->counters.duplicateSegmentIgnored++;
            
            C1222MessageReceiver_LogEvent(p, C1222EVENT_ALR_DUPLICATE_SEGMENT);
            // will return ok - message will not be complete if it was not complete before
        }  
        else if ( !C1222ALSegmenter_SetSegment(&p->receiveSegmenter, pMsg) )
        {
            C1222MessageReceiver_LogEvent(p, C1222EVENT_ALR_CANT_SET_SEGMENT);
            return C1222RESPONSE_SGERR;
        }
    }
    else
    {
        // if we are in the process of receiving a segmented message, and this is a display update from the rflan
        // fail the message to avoid trashing the segmented message
        
        if ( C1222ALSegmenter_IsBusy(&p->receiveSegmenter) )
        {
#if 0      // this is no longer needed since there are no low priority messages from the rflan            
            if ( IsLowPriorityMessageFromRFLAN(p, pMsg) )
            {
                ((C1222AL*)(p->pApplication))->counters.rflanMessageIgnored++;
                C1222MessageReceiver_LogEvent(p, C1222EVENT_ALR_IGNOREDRFLANMESSAGE);
                return C1222RESPONSE_BSY;
            }
#endif            
            
            ((C1222AL*)(p->pApplication))->counters.partialMessageOverLayedByLaterFullMessage++;
            ((C1222AL*)(p->pApplication))->status.timeOfLastDiscardedSegmentedMessage = C1222Misc_GetFreeRunningTimeInMS();
        }
        
        // the message is copied to the receive segmenter just to have someplace to store it
        // it currently resides in the transport layer buffer which is apparently big enough
        // so this should not fail
        
        if ( !C1222ALSegmenter_SetUnsegmentedRxMessage(&p->receiveSegmenter, pMsg) )
        {
            C1222MessageReceiver_LogEvent(p, C1222EVENT_ALR_CANT_SET_UNSEG_MSG2);
            return C1222RESPONSE_SGERR;
        }
            
        C1222ALSegmenter_EndNewUnsegmentedMessage(&p->receiveSegmenter);
    }
    
    return C1222RESPONSE_OK;
}


Boolean C1222MessageReceiver_IsMessageComplete(C1222MessageReceiver* p)
{
    return C1222ALSegmenter_IsMessageComplete(&p->receiveSegmenter);
}


ACSE_Message* C1222MessageReceiver_GetUnsegmentedMessage(C1222MessageReceiver* p)
{
    return C1222ALSegmenter_GetUnsegmentedMessage(&p->receiveSegmenter);
}

