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



#include "acsemsg.h"
#include "c1222environment.h"
#include "c1222aptitle.h"
#include "c1222al.h"
#include "c1222misc.h"
#include "c1222al_receiveHistory.h"



void C1222ALReceiveHistory_Init(C1222ReceiveHistory* p, C1222AL* pAL)
{
    p->addIndex = 0;
    p->pApplication = pAL;
    
    memset(p->history, 0, sizeof(p->history));
}


static Boolean MessageIsReadOfRFLANFactoryDataOrUpdateVendorField(ACSE_Message* pMsg)
{
    //Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned8* buffer;
    Unsigned16 length;
    Unsigned8* data;
    
    Epsem epsem;
    
    // find the app-data in the request message
    
    if ( ACSE_Message_GetApData( pMsg, &data, &tagLength) )
    {
        // rflan read request is not encrypted
        
        Epsem_Init(&epsem, data, tagLength);
        
        if ( Epsem_Validate(&epsem) )
        {
            if ( Epsem_GetNextRequestOrResponse( &epsem, &buffer, &length) )
            {
                // check if read request
                
                if ( buffer[0] == 0x3f || buffer[0] == 0x30 )
                {
                    // check if rflan factory data (2113 is 0x0841 or mfg 65)
                    
                    if ( buffer[1] == 0x08 && buffer[2] == 0x41 )
                        return TRUE;
                }
                
                // check for writes of vendor fields
                
                if ( (buffer[0] == 0x40) && (buffer[1] == 0x00) && (buffer[2] == 0x07) && 
                     (buffer[5] == 0x12) && (buffer[6] == 0x08) )
                    return TRUE;
                
                if ( (buffer[0] == 0x4F) && (buffer[1] == 0x00) && (buffer[2] == 0x07) &&
                     (buffer[8] == 0x12) && (buffer[9] == 0x08) )
                    return TRUE;
            }
        }
    }
    
    return FALSE;
}


Boolean C1222ALReceiveHistory_IsMessageAlreadyReceived(C1222ReceiveHistory* p, ACSE_Message* pMsg)
{
    // returns true if this is a duplicate of one in history
    // else adds in the oldest history position
    
    C1222ApTitle callingApTitle;
    Boolean haveCallingApTitle;
    Unsigned16 sequence;
    Unsigned8 ii;
    C1222ApTitle histApTitle;
    int difference;
    Unsigned32 now;
    C1222ReceiveHistoryElement* pElement;
    C1222AL* pApplication = p->pApplication;
    C1222ApTitle myApTitle;
    static Unsigned8 defaultRFLanApTitleBuffer[3] = { 0x0d, 0x01, 0x01 };
    
    haveCallingApTitle = (Boolean)(ACSE_Message_GetCallingApTitle( pMsg, &callingApTitle) &&
                                        C1222ApTitle_Validate(&callingApTitle));
                                        
    // will never consider rflan initiated message a duplicate
    
    // unfortunately rflan does not use a calling aptitle when it tries to read factory data and when it updates vendor fields
    
    if ( haveCallingApTitle )
    {
        // check for ".1"
        // check for <my aptitle>.1
        
        if ( C1222AL_GetMyApTitle(p->pApplication, &myApTitle) )
        {        
            if ( C1222ApTitle_IsApTitleMyCommModule(&callingApTitle, &myApTitle) )
            {
                C1222AL_LogEvent(pApplication, C1222EVENT_DEVICE_SKIPDUPCHECKONRFLANMSG);
                return FALSE;
            }
        }
        
        if ( memcmp(callingApTitle.buffer, defaultRFLanApTitleBuffer, sizeof(defaultRFLanApTitleBuffer)) == 0 )
        {
            C1222AL_LogEvent(pApplication, C1222EVENT_DEVICE_SKIPDUPCHECKONRFLANMSG);
            return FALSE;
        }
    }
    else
    {
        // check for a read of rflan factory data with no aptitle
        
        if ( MessageIsReadOfRFLANFactoryDataOrUpdateVendorField(pMsg) )
        {
            C1222AL_LogEvent(pApplication, C1222EVENT_DEVICE_SKIPDUPCHECKONRFLANFACTORYREADORUPDATEVENDORFIELD);
            return FALSE;
        }
    }
    
    now = C1222Misc_GetFreeRunningTimeInMS();
    
    if ( !ACSE_Message_GetCallingApInvocationId( pMsg, &sequence ) )
        return FALSE;  // it should have this
        
    for ( ii=0; ii<ELEMENTS(p->history); ii++ )
    {
        pElement = &p->history[ii];
        
        if ( pElement->receptionTime != 0 )
        {
            // maintain the history
            if ( (now-pElement->receptionTime) > 120000L )
                memset(pElement, 0, sizeof(C1222ReceiveHistoryElement));
        
            // check for duplicate
            if ( sequence == pElement->sequence )
            {
                C1222ApTitle_Construct(&histApTitle, pElement->callingApTitle, C1222_APTITLE_LENGTH);
                histApTitle.length = C1222ApTitle_GetLength(&histApTitle);
            
                if ( !haveCallingApTitle && (histApTitle.length <= 2) )
                {
                    C1222AL_LogEvent(pApplication, C1222EVENT_DEVICE_DUPLICATE_MESSAGE_DROPPED);
                    return TRUE;
                }
            
                if ( haveCallingApTitle && (histApTitle.length > 2) && 
                      C1222ApTitle_Compare(&callingApTitle, &histApTitle, &difference) &&
                      (difference==0) )
                {
                    C1222AL_LogEvent(pApplication, C1222EVENT_DEVICE_DUPLICATE_MESSAGE_DROPPED);
                    return TRUE;
                }
            }
        }        
    }
    
    // not in the index - add in the oldest slot
    
    if ( p->addIndex >= ELEMENTS(p->history) )
        p->addIndex = 0;
        
    if ( now == 0 )
        now++;
        
    pElement = &p->history[p->addIndex];
    
    pElement->sequence = sequence;
    pElement->receptionTime = now;
    if ( haveCallingApTitle )
        memcpy(pElement->callingApTitle, callingApTitle.buffer, callingApTitle.length);
    else
        memset(pElement->callingApTitle, 0, C1222_APTITLE_LENGTH);
        
    p->addIndex++;
    
    return FALSE;
}
