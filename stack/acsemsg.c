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

// a problem with the acse message is that the total message length is stored at the
// beginning of the message, right after the 0x60.  The entire message might be less than or
// more than 128 bytes, which means that we don't know how many bytes it will take to store
// the length at the beginning of the message.  To deal with this, the ACSE_Message will not
// include the constant 0x60 at the beginning of the message, or the total message length.
// this will need to be inserted before transmitting the message.

#include "c1222.h"
#include "c1222environment.h"
#include "acsemsg.h"
#include "c1222misc.h"
#include "c1222dl_crc.h"
#include "c1222encrypt.h"
#include "c1222al.h"
#include "c1222response.h"
#include "c1222aptitle.h"




static Boolean ValidAuthenticationValueFormatMarch2006(ACSE_Message* pMsg);
static Boolean ValidAuthenticationValueFormat2008(ACSE_Message* pMsg);
static Boolean ValidInformationValueFormat2008(ACSE_Message* pMsg);
static Boolean ValidInformationValueFormatMarch2006(ACSE_Message* pMsg);
static Boolean IsSegmentedFormat2008(ACSE_Message* pMsg);
static Boolean IsSegmentedFormat2006(ACSE_Message* pMsg);
static Boolean FindBufferTagListItem(Unsigned8* buffer, Unsigned16 length, Unsigned8* tagList, Unsigned8 tagListLength, 
                              Unsigned16* pOffset, Unsigned16* pLength);
#ifdef C1222_INCLUDE_DEVICE
static Unsigned16 AddCallingApTitleToCRC2008(ACSE_Message* pMsg, Unsigned16 crc);
#endif                             

Boolean ACSE_Message_IsValidFormat2008(ACSE_Message* pMsg); // should be in header!
static Boolean MessageHasTag(ACSE_Message* pMsg, Unsigned8 tag);




// there is a possibility that there may be more than one root for each of these - check before release

Unsigned8 C1222StandardEncryptionUniversalIDMarch2006[] = { 0x2A,0x86,0x48,0xCE,0x52,0x02,0x01 };
Unsigned8 C1222ContextMarch2006[] = { 0x06, 0x05, 0x2A, 0x86, 0x48, 0xCE, 0x52 };

Unsigned8 C1222StandardEncryptionUniversalID2008[] = { 0x60, 0x7c, 0x86, 0xf7, 0x54, 0x01, 0x16, 0x02, 0x01 };
Unsigned8 C1222Context2008[] = { 0x06, 0x07, 0x60, 0x7c, 0x86, 0xf7, 0x54, 0x01, 0x16 };



void ACSE_Message_SetBuffer( ACSE_Message* p, Unsigned8* buffer, Unsigned16 maxLength)
{
    p->buffer = buffer;
    p->length = 0;
    p->maxLength = maxLength;
    p->standardVersion = CSV_UNKNOWN;
}


void ACSE_Message_SetStandardVersion( ACSE_Message* pMsg, C1222StandardVersion std_version)
{
    pMsg->standardVersion = std_version;
}


C1222StandardVersion ACSE_Message_GetStandardVersion( ACSE_Message* pMsg )
{
    return pMsg->standardVersion;
}




static Unsigned16 DecodeLength(Unsigned8* buffer, Unsigned8* pSizeofLength)
{
    return C1222Misc_DecodeLength(buffer, pSizeofLength);
}





static Boolean FindACSEBufferTag(Unsigned8* buffer, Unsigned16 bufferLength, Unsigned8 tag, Unsigned16* pIndex, Unsigned16* pTagLength )
{
    Unsigned8 sizeofLength;
    Unsigned16 tagComponentLength;
    Unsigned16 index = 0;
    Unsigned16 dataIndex;

    while( index < bufferLength )
    {
        tagComponentLength = (Unsigned16)DecodeLength( &buffer[index+1], &sizeofLength);
        if ( sizeofLength == 0 )
            return FALSE;
            
        dataIndex = (Unsigned16)(index + 1 + sizeofLength);

        if ( buffer[index] == tag )
        {
            *pIndex = dataIndex; // return index of tag data
            *pTagLength = tagComponentLength;
            return TRUE;
        }

        index = (Unsigned16)(dataIndex + tagComponentLength);
    }

    return FALSE;  // not there
}

Boolean ACSE_Message_FindBufferTag(Unsigned8* buffer, Unsigned16 length, Unsigned8 tag, Unsigned16* pIndex, Unsigned16* pLength)
{
    return FindACSEBufferTag( buffer, length, tag, pIndex, pLength);
}



static Boolean FindACSETag(ACSE_Message* pMsg, Unsigned8 tag, Unsigned16* pIndex, Unsigned16* pTagLength )
{
    return FindACSEBufferTag( pMsg->buffer, pMsg->length, tag, pIndex, pTagLength );
}




Boolean ACSE_Message_FindACSETag(ACSE_Message* pMsg, Unsigned8 tag, Unsigned16* pIndex, Unsigned16* pTagLength )
{
    return FindACSEBufferTag( pMsg->buffer, pMsg->length, tag, pIndex, pTagLength );
}



// This routine calculates the size of the header information for a segmented message.
// This includes acse components that are to be copied from the full unsegmented message,
// context, called aptitle, calling aptitle, and calling entity qualifier.
// It does not count the initial 0x60 and length bytes (since that can be variable size and we
// aren't ready to calculate it.
// It also includes the calling ap invocation id which is sized based on the full apdu size
//
// the parameter is the full unsegmented message

Unsigned16 ACSE_Message_GetSegmentedMessageHeaderLength(ACSE_Message* pMsg)
{
    Unsigned16 index;
    Unsigned16 length;
    Unsigned16 headerLength = 0;
    Unsigned16 totalMessageLength;
    Unsigned8 ii;
    
    static Unsigned8 copiedTagList[] = {    0xA1,   // context
                                            0xA2,   // called aptitle
                                            0xA6,   // calling aptitle
                                            0xA7,   // calling entity qualifier
                                            0xA8    // calling ap invocation id (handled differently in 2006 version
                                        };
                                            
    
    
    // does not count the initial 0x60 and message length bytes
    
    // handle size of the elements copied from the full acse message
    
    for ( ii=0; ii<ELEMENTS(copiedTagList); ii++ )
    {
        if ( (copiedTagList[ii] == 0xA8) && (pMsg->standardVersion == CSV_MARCH2006) )  // A8 is handled differently in 2006 standard since it also has offset and full length
            continue;
            
        if ( FindACSETag(pMsg, copiedTagList[ii], &index, &length) )
        {
            headerLength += (Unsigned16)(1 + C1222Misc_GetSizeOfLengthEncoding(length) + length);
        }
    }
    
    totalMessageLength = (Unsigned16)(1 + C1222Misc_GetSizeOfLengthEncoding(pMsg->length) + pMsg->length);
    
    // handle size of the segment calling invocation id in 2006 standard
    
    if ( pMsg->standardVersion == CSV_MARCH2006 )
    {
        // A8H <seg-id-length> <seg-id>
        // <seg-id> ::= <apdu-seq-number> <segment-byte-offset> <apdu-size>
        // <apdu-seq-number> ::= <word16>
        // seg byte offset and apdu size are smallest of <byte> <word16> or <word24> able to hold
    
        headerLength += (Unsigned16)4; // a8 seg-id-length apdu-seq-number
    
        if ( totalMessageLength > 255 )
            headerLength += (Unsigned16)4;
        else
            headerLength += (Unsigned16)2; 
    }
    else
    {
        // in 2008 standard need an A3 tag
        
        // A3 <length> 02 <length2> <apdu-size>
     
        headerLength += (Unsigned16)5;
        
        if ( totalMessageLength > 255 )
            headerLength++;  
    }

    return headerLength;    
}



static Boolean GetApTitle(ACSE_Message* pMsg, C1222ApTitle* pApTitle, Unsigned8 tag)
{
    Unsigned16 index;
    Unsigned16 length;
    
    if ( FindACSETag(pMsg, tag, &index, &length ) )
    {
        pApTitle->buffer = &pMsg->buffer[index];
        pApTitle->length = length;
        pApTitle->isACSE2008Format = (Boolean)(( pMsg->standardVersion == CSV_2008 ) ? TRUE : FALSE);
        return TRUE;
    }
    else
        return FALSE;
}


Boolean ACSE_Message_GetCallingApTitle(ACSE_Message* pMsg, C1222ApTitle* pApTitle)
{
    return GetApTitle( pMsg, pApTitle, 0xA6);
}



static Boolean GetApInvocationId(ACSE_Message* pMsg, Unsigned16* pSequence, Unsigned8 tag)
{
    Unsigned16 index;
    Unsigned16 length;
    Unsigned8* buffer;
    
    if ( FindACSETag(pMsg, tag, &index, &length ) )
    {
        buffer = &pMsg->buffer[index];
        
        if ( pMsg->standardVersion == CSV_MARCH2006 )
        {
            // I'll special case lengths of 0 and 1 which are not per ballot 1 standard
            // but may make things work with the cell relay
        
            if ( length == 0 )
                *pSequence = 0; 
            else if ( length == 1 )
                *pSequence = (Unsigned16)(buffer[0]);
            else        
                *pSequence = (Unsigned16)((buffer[0]<<8) | buffer[1]);
        }
        else // length should be 3 or 4 - buffer[0] should be 2 with another length after that in buffer[1]
        {
            if ( length == 3 )
                *pSequence = (Unsigned16)(buffer[2]);
            else
                *pSequence = (Unsigned16)((buffer[2]<<8) | buffer[3]);
        }
            
        return TRUE;
    }
    else
        return FALSE;
}


Boolean ACSE_Message_GetCalledApInvocationId(ACSE_Message* pMsg, Unsigned16* pSequence)
{
    return GetApInvocationId(pMsg, pSequence, 0xA4);
}



Boolean ACSE_Message_GetCallingApInvocationId(ACSE_Message* pMsg, Unsigned16* pSequence)
{
    return GetApInvocationId(pMsg, pSequence, 0xA8);
}


Boolean getLengthInteger( Unsigned8* buffer, Unsigned16 length, Unsigned16* pInteger)
{
    if ( length == 1 )
    {
        *pInteger = (Unsigned16)(buffer[0]);
        return TRUE;
    }
    else if ( length == 2 )
    {
        *pInteger = (Unsigned16)((buffer[0]<<8) | buffer[1]);
        return TRUE;
    }
    
    // not handling length 3 since that is beyond the meter's capability
    return FALSE;
 }


Boolean ACSE_Message_GetInfomationOffset2008(ACSE_Message* pMsg, Unsigned16* pOffset )
{
    // used to get offset into the full message for a segment
    // this is now part of the information element (0xBE)
    
    Unsigned16 index;
    Unsigned16 length;
    Unsigned8* buffer;
    Unsigned8 tagList2008[] = { 0x28, 0x02 };

    if ( FindACSETag(pMsg, 0xBE, &index, &length ) )
    {
        buffer = &pMsg->buffer[index];
        
        if ( FindBufferTagListItem( buffer, length, tagList2008, sizeof(tagList2008), &index, &length) )
        {
            return getLengthInteger(&buffer[index], length, pOffset);
        }
    }
    
    return FALSE;   
}


Boolean ACSE_Message_GetCalledAEQualifier2008(ACSE_Message* pMsg, Unsigned16* pFullLength )
{
    // used to get full acse message length for a segmented message
    // this only handles 2008 standard since this tag did not exist in March 2006 version of the standard
    
    Unsigned16 index;
    Unsigned16 length;

    if ( FindACSETag(pMsg, 0xA3, &index, &length ) && (length==3 || length==4) )
    {  
        index += (Unsigned16)2;
        length -= (Unsigned16)2;
              
        return getLengthInteger( &pMsg->buffer[index], length, pFullLength);
    }
    
    return FALSE;   
}
    


Boolean ACSE_Message_GetCallingAeQualifier(ACSE_Message* pMsg, Unsigned8* pQualByte )
{
    Unsigned16 index;
    Unsigned16 length;
    Unsigned8* buffer;
    Unsigned8 subTag = (Unsigned8)(( pMsg->standardVersion == CSV_MARCH2006 ) ? 0x82 : 0x02);

    if ( FindACSETag(pMsg, 0xA7, &index, &length ) )
    {
        buffer = &pMsg->buffer[index];
        
        if ( (buffer[0] == subTag) && (buffer[1] == 0x01) )
        {
            *pQualByte = buffer[2];
            return TRUE;
        }
    }
    
    return FALSE;
}



void ACSE_Message_SetCallingAeQualifier(ACSE_Message* pMsg, Unsigned8 qualByte )
{
    Unsigned8* buffer = &pMsg->buffer[pMsg->length];
    
    buffer[0] = 0xA7;
    buffer[1] = 3;
    buffer[2] = (Unsigned8)((pMsg->standardVersion == CSV_MARCH2006) ? 0x82 : 0x02);
    buffer[3] = 1;
    buffer[4] = qualByte;
    
    pMsg->length += (Unsigned16)5;        
}




Boolean ACSE_Message_GetCalledApTitle(ACSE_Message* pMsg, C1222ApTitle* pApTitle)
{
    return GetApTitle( pMsg, pApTitle, 0xA2);
}



Boolean ACSE_Message_IsResponse(ACSE_Message* pMsg)
{
    if ( MessageHasTag(pMsg, 0xA4) )
        return TRUE;
        
    return FALSE;
}




Boolean ACSE_Message_GetApData(ACSE_Message* pMsg, Unsigned8** pData, Unsigned16* pLength)
{
    Unsigned16 index;
    Unsigned16 length;
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned8* buffer;
    
    if ( FindACSETag(pMsg, 0xBE, &index, &length ) )
    {
        if ( pMsg->standardVersion == CSV_MARCH2006 )
        {
            *pData = &pMsg->buffer[index];
            *pLength = length;
            return TRUE;
        }
        else // 2008 standard
        {
            if ( FindACSEBufferTag(&pMsg->buffer[index], length, 0x28, &tagIndex, &tagLength) )
            {
                buffer = &pMsg->buffer[index+tagIndex];
                
                if ( FindACSEBufferTag(buffer, tagLength, 0x81, &index, &length) )
                {
                    *pData = &buffer[index];
                    *pLength = length;
                    return TRUE;
                }  
            } 
        }
    }
    
    return FALSE;
}




// this routine gets segmentation info from a segmented message, including
// offset, full length, and sequence

Boolean ACSE_Message_GetSegmentOffset(ACSE_Message* pMsg, Unsigned16* pOffset, Unsigned16* pFullLength, Unsigned16* pSequence)
{
    Unsigned16 index;
    Unsigned16 length;
    Unsigned8* buffer;
    Unsigned16 offset;
    Unsigned16 fullLength;
    Unsigned16 sequence;
    
    if ( pMsg->standardVersion == CSV_MARCH2006 )
    {
        if ( FindACSETag(pMsg, 0xA8, &index, &length ) )
        {
            buffer = &pMsg->buffer[index];
        
            sequence = (Unsigned16)((buffer[0]<<8) | buffer[1]);
        
            switch( length-2 )
            {
                case 2:
                    offset = buffer[2];
                    fullLength = buffer[3];
                    break;
                case 4:
                    offset     = (Unsigned16)((buffer[2]<<8) | buffer[3]);
                    fullLength = (Unsigned16)((buffer[4]<<8) | buffer[5]);
                    break;
                case 6:
                default:
                    return FALSE;
            }        
        }
        else
            return FALSE;
    }
    else // 2008 standard
    {
        // unfortunately these are now stored in different places
        // sequence is in A8, full length is in A3, offset is part of BE tag
        
        if ( ! ACSE_Message_GetCallingApInvocationId(pMsg, &sequence) )
            return FALSE;
            
        if ( ! ACSE_Message_GetCalledAEQualifier2008( pMsg, &fullLength) )
            return FALSE;
            
        if ( ! ACSE_Message_GetInfomationOffset2008( pMsg, &offset) )
            return FALSE;   
    }
    
    *pOffset = offset;
    *pFullLength = fullLength;
    *pSequence = sequence;
        
    return TRUE;    
}




static Boolean MessageHasTag(ACSE_Message* pMsg, Unsigned8 tag)
{
    Unsigned16 index;
    Unsigned16 length;
    
    if ( FindACSETag(pMsg, tag, &index, &length ) )
        return TRUE;
    else
        return FALSE;
}


Boolean ACSE_Message_HasTag(ACSE_Message* pMsg, Unsigned8 tag)
{
    return MessageHasTag(pMsg, tag);
}


static Unsigned8 FindByteInList(Unsigned8* list, Unsigned8 length, Unsigned8 startIndex, Unsigned8 tag)
{
    Unsigned8 ii;
    
    for ( ii=startIndex; ii<length; ii++ )
        if ( list[ii] == tag )
            return ii;
            
    return 255;
}


static Boolean BufferTagOrderOK(Unsigned8* buffer, Unsigned16 bufferLength, Unsigned8* tagList, Unsigned16 numberOfTags)
{
    // checks if all tags in the msg are in tagList order and no other tags exist in the msg
    
    Unsigned8 sizeofLength;
    Unsigned16 tagComponentLength;
    Unsigned16 index = 0;
    Unsigned16 dataIndex;
    Unsigned16 tagListIndex = 0;

    while( index < bufferLength )
    {
        tagComponentLength = (Unsigned16)DecodeLength( &buffer[index+1], &sizeofLength);
        if ( sizeofLength == 0 )
            return FALSE;
            
        dataIndex = (Unsigned16)(index + 1 + sizeofLength);

        tagListIndex = FindByteInList(tagList, numberOfTags, tagListIndex, buffer[index]);
        
        if ( tagListIndex == 255 )
            return FALSE;  // this tag is undefined or out of order
            
        tagListIndex++; // this tag is used - increment for next time
        
        index = (Unsigned16)(dataIndex + tagComponentLength);
    }

    return TRUE;  // all tags in the buffer are present in the tag list in the right order    
}


static Boolean MessageTagOrderOK(ACSE_Message* pMsg, Unsigned8* tagList, Unsigned16 numberOfTags)
{
    return BufferTagOrderOK(pMsg->buffer, pMsg->length, tagList, numberOfTags);
}


Boolean IsSegmentedFormat2006(ACSE_Message* pMsg)
{
    Unsigned16 index;
    Unsigned16 length;
    
    // this should not be called unless it is known that the message is 2006 format (or it is assumed to be)
        
    if ( FindACSETag(pMsg, 0xA8, &index, &length ) )
    {        
        switch( length-2 )
        {
            case 2:
            case 4:
                return TRUE;

            case 6:
            default:
                break;
        }
    }

    return FALSE;
}


Boolean IsSegmentedFormat2008(ACSE_Message* pMsg)
{
    return MessageHasTag(pMsg, 0xA3);
}



#define ACSEMSG_ATTRIBUTE_SEGMENTED      0x0001
#define ACSEMSG_ATTRIBUTE_ENCRYPTED      0x0002
#define ACSEMSG_ATTRIBUTE_AUTHENTICATED  0x0004

Boolean ACSE_Message_IsValidFormat2006(ACSE_Message* pMsg)
{
    return (Boolean)((ACSE_Message_CheckFormat2006(pMsg) == CMVR_OK) ? TRUE : FALSE);    
}

C1222MessageValidationResult ACSE_Message_CheckFormat2006(ACSE_Message* pMsg)
{
    // checks if this msg is valid per march 2006 standard
    // this does not decrypt anything and so does not check encrypted fields
    
    // has at least BE and A8 tags
    // tags (if present) are in order A1 A2 A6 A7 AB AC A4 A8 BE, and there are no other tags
    // if segmented (based on A8 length), does not have A4 AB AC
    // tags that are present are valid for march 2006 format
    // A1 is of form "A1 07 06 05 2A 86 48 CE 52"
    // A2 is a valid aptitle of type 06 (absolute) or 0d (relative)
    // A6 is a valid aptitle of type 06 (absolute) or 0d (relative)
    // A7 is of form "A7 03 82 01 XX"
    // A4 is of form "A4 01 XX" or "A4 02 XX XX"
    // A8 is of form "A8 01 XX" or "A8 02 XX XX" if unsegmented
    // A8 is of form "A8 03 XX OO SS" or "A8 04 XX XX OO SS" or "A8 06 XX XX OO OO SS SS" if segmented
    // A8 is of form "A8 05 XX OO OO SS SS" if segmented
    // AB is of form "AB ll XX ... XX"
    // AC is of form "AC LL ..." (valid authentication tag mar 2006)
    // BE is of form "BE LL epsem" if unsegmented unencrypted
    // BE is of form "BE LL <mac> epsem <padding>" if unsegmented encrypted
    // BE is of form "BE LL ..." if segmented
    
    static Unsigned8 tagOrder[]    = { 0xA1, 0xA2, 0xA6, 0xA7, 0xAB, 0xAC, 0xA4, 0xA8, 0xBE };
    static Unsigned8 tagOrderAlt[] = { 0xA1, 0xA2, 0xA6, 0xA7, 0xAB, 0xA4, 0xA8, 0xAC, 0xBE };    // for testbench
    Unsigned16 index;
    Unsigned16 length;
    C1222ApTitle aptitle;
    C1222StandardVersion saveVersion = pMsg->standardVersion;
    Unsigned8* buffer;
    C1222MessageValidationResult result = CMVR_OK;
    
    pMsg->standardVersion = CSV_MARCH2006;  // pretend it is 2006 format while checking
    
    if ( !MessageHasTag(pMsg,0xA8) )
        result = CMVR_MISSING_A8_TAG;
        
    if ( !MessageHasTag(pMsg,0xBE) )
        result = CMVR_MISSING_BE_TAG;

    if ( !MessageTagOrderOK(pMsg, tagOrder, sizeof(tagOrder)) )
    {
    	if ( !MessageTagOrderOK(pMsg, tagOrderAlt, sizeof(tagOrderAlt)) )   // allow tag order that ttb wants
        	result = CMVR_TAGS_OUT_OF_ORDER;
    }
        
    if ( FindACSETag(pMsg, 0xA1, &index, &length) )
    {
        buffer = &pMsg->buffer[index];
        
        if ( length != sizeof(C1222ContextMarch2006) || memcmp(buffer,C1222ContextMarch2006,sizeof(C1222ContextMarch2006)) != 0 )
            result = CMVR_WRONG_CONTEXT;
    }

    if ( ACSE_Message_GetCalledApTitle(pMsg, &aptitle) )
    {
        if ( !C1222ApTitle_ValidateACSEFormatMarch2006_AnyRoot(&aptitle) )
            result = CMVR_CALLED_APTITLE_INVALID;
    }

    if ( ACSE_Message_GetCallingApTitle(pMsg, &aptitle) )
    {
        if ( !C1222ApTitle_ValidateACSEFormatMarch2006_AnyRoot(&aptitle) )
            result = CMVR_CALLING_APTITLE_INVALID;
    }

    if ( FindACSETag(pMsg, 0xA7, &index, &length) )
    {
        buffer = &pMsg->buffer[index];
        
        if ( length != 3 || buffer[0] != 0x82 || buffer[1] != 1 )
            result = CMVR_CALLING_AE_QUALIFIER_INVALID;
    }
        
    if ( IsSegmentedFormat2006(pMsg) )
    {
        if ( MessageHasTag(pMsg,0xA4) || MessageHasTag(pMsg,0xAB) || MessageHasTag(pMsg,0xAC) )
            result = CMVR_TAG_INVALID_IN_SEGMENTED_MSG;
    }
    else // not segmented message
    {
        if ( FindACSETag(pMsg, 0xA4, &index, &length) )
        {
            if ( length < 1 || length > 2 )
                result = CMVR_CALLED_AP_INVOCATION_ID_INVALID;
        }

        if ( FindACSETag(pMsg, 0xA8, &index, &length) )
        {
            if ( length < 1 || length > 2 )
                result = CMVR_CALLING_AP_INVOCATION_ID_INVALID;
        }        
        
        if ( FindACSETag(pMsg, 0xAC, &index, &length) )
        {
            if ( !ValidAuthenticationValueFormatMarch2006(pMsg) )
                result = CMVR_AUTHENTICATION_VALUE_INVALID;
        }
        
        if ( FindACSETag(pMsg, 0xBE, &index, &length) )
        {
            if ( !ValidInformationValueFormatMarch2006(pMsg) )
                result = CMVR_INFORMATION_VALUE_INVALID;       
        }        
    }
    
    pMsg->standardVersion = saveVersion;
    
    return result;
}


Boolean ACSE_Message_IsMarch2006FormatMessage(ACSE_Message* pMsg)
{
    // There are some cases where we may not be able to tell if the message is 2006 format or
    // 2008 format.  This routine attempts to determine which it is.
    
    Unsigned16 index;
    Unsigned16 length;
    Unsigned8* buffer;
    
    // the march 2006 standard did not have an A3 tag.  2008 standard does, and if present it 
    // indicates that this is a segmented message.
    
    if ( MessageHasTag(pMsg, 0xA3) )
        return FALSE;
        
    // if BE content first byte is not 0x28 it is 2006 standard
    
    if ( FindACSETag(pMsg, 0xBE, &index, &length) )
    {
        if ( pMsg->buffer[index] != 0x28 )
            return TRUE;
    }
    
    // A8 is a required tag.  If not present it is neither 2006 nor 2008.
    
    if ( FindACSETag(pMsg, 0xA8, &index, &length) )
    {
        buffer = &pMsg->buffer[index];
        
        if ( (length == 3) && (buffer[0] == 2) && (buffer[1] == 1) )
        {
            // this is almost certainly 2008 standard - only possibility if 2006 is that it
            // is sequence 2 offset 1 and full length buffer[2].  SR1.0 C12.22 code would not have
            // handled this correctly.  Offset 1 is a very strange place to start a segment.
            // Still if this is not a valid 2008 format message we will assume 2006 format,

            if ( ! ACSE_Message_IsValidFormat2008(pMsg) )
                return TRUE;            
        }        
        else if ( (length == 4) && (buffer[0] == 2) && (buffer[1] == 2) )
        {
            // this is probably 2008 standard.  But it might be 2006 sequence 0x0202, offset=buffer[2]
            // full length=buffer[3].  Lets assume 2008 unless that is invalid
            
            if ( ! ACSE_Message_IsValidFormat2008(pMsg) )
                return TRUE;
        }
        else
            return TRUE;  // A8 is not valid for 2008 standard - so must be 2006            
    }
    
    return FALSE;
}


Boolean ACSE_Message_IsValidFormat2008(ACSE_Message* pMsg)
{
    return (Boolean)((ACSE_Message_CheckFormat2008(pMsg) == CMVR_OK) ? TRUE : FALSE);
}

C1222MessageValidationResult ACSE_Message_CheckFormat2008(ACSE_Message* pMsg)
{
    // Checks if this msg is valid per January 2008 standard
    // returns 0 if ok, else an error code
    
    // has at least BE and A8 tags
    // tags (if present) are in order A1 A2 A3 A4 A6 A7 A8 8B AC BE, and there are no other tags
    // if segmented (ie, has A3), does not have A4 8B AC
    // tags that are present are valid for jan 2008 format
    // A1 is of form "A1 08 06 05 2A 86 48 CE 52"
    // A2 is a valid aptitle of type 06 (absolute) or 80 (relative)
    // A6 is a valid aptitle of type 06 (absolute) or 80 (relative)
    // A3 has a valid 1-3 byte total length
    // A4 is of form "A4 03 02 01 XX" or "A4 04 02 02 XX YY"
    // A8 is of form "A8 03 02 01 XX" or "A8 04 02 02 XX YY"
    // A7 is of form "A7 03 02 01 XX"
    // AC is of form "AC L1 A2 L2 [02 01 00] A0 L3 { A1 L4 xxxxxx } ...  (valid authentication tag july 2007)
    // BE is of form "BE ..." (valid information tag)
    // if segmented, offset in BE is less than total length in A3

    static Unsigned8 tagOrder[] = { 0xA1, 0xA2, 0xA3, 0xA4, 0xA6, 0xA7, 0xA8, 0x8B, 0xAC, 0xBE };
    Unsigned16 index;
    Unsigned16 length;
    C1222ApTitle aptitle;
    C1222StandardVersion saveVersion = pMsg->standardVersion;
    Unsigned8* buffer;
    C1222MessageValidationResult result = CMVR_OK;
    
    pMsg->standardVersion = CSV_2008;  // pretend it is 2008 format while checking
    
    
    if ( !MessageHasTag(pMsg,0xA8) )
        result = CMVR_MISSING_A8_TAG;
    
    if ( !MessageHasTag(pMsg,0xBE) )
        result = CMVR_MISSING_BE_TAG;
        
    if ( !MessageTagOrderOK(pMsg, tagOrder, sizeof(tagOrder)) )
        result = CMVR_TAGS_OUT_OF_ORDER;
        
    if ( FindACSETag(pMsg, 0xA1, &index, &length) )
    {
        buffer = &pMsg->buffer[index];
        
        if ( length != sizeof(C1222Context2008) || memcmp(buffer,C1222Context2008,sizeof(C1222Context2008)) != 0 )
            result = CMVR_WRONG_CONTEXT;
    }
    
    if ( ACSE_Message_GetCalledApTitle(pMsg, &aptitle) )
    {
        if ( !C1222ApTitle_ValidateACSEFormat2008_AnyRoot(&aptitle) )
            result = CMVR_CALLED_APTITLE_INVALID;
    }

    if ( ACSE_Message_GetCallingApTitle(pMsg, &aptitle) )
    {
        if ( !C1222ApTitle_ValidateACSEFormat2008_AnyRoot(&aptitle) )
            result = CMVR_CALLING_APTITLE_INVALID;
    }

    if ( FindACSETag(pMsg, 0xA7, &index, &length) )
    {
        buffer = &pMsg->buffer[index];
        
        if ( length != 3 || buffer[0] != 0x02 || buffer[1] != 1 )
            result = CMVR_CALLING_AE_QUALIFIER_INVALID;
    }
        
    if ( FindACSETag(pMsg, 0xA3, &index, &length) ) // segmented
    {
        if ( length < 3 || length > 4 )
            result = CMVR_CALLED_AE_QUALIFIER_INVALID;
            
        if ( MessageHasTag(pMsg,0xA4) || MessageHasTag(pMsg,0x8B) || MessageHasTag(pMsg,0xAC) )
            result = CMVR_TAG_INVALID_IN_SEGMENTED_MSG;
    }
    else // not segmented
    {
        if ( FindACSETag(pMsg, 0xA4, &index, &length) )
        {
            if ( length < 3 || length > 4 )
                result = CMVR_CALLED_AP_INVOCATION_ID_INVALID;
                
            buffer = &pMsg->buffer[index];
                
            if ( length == 3 && (buffer[0] != 2 || buffer[1] != 1) )
                result = CMVR_CALLED_AP_INVOCATION_ID_INVALID;
                
            if ( length == 4 && (buffer[0] != 2 || buffer[1] != 2) )
                result = CMVR_CALLED_AP_INVOCATION_ID_INVALID;
        }

        if ( FindACSETag(pMsg, 0xA8, &index, &length) )
        {
            if ( length < 3 || length > 4 )
                result = CMVR_CALLING_AP_INVOCATION_ID_INVALID;
                
            buffer = &pMsg->buffer[index];

            if ( length == 3 && (buffer[0] != 2 || buffer[1] != 1) )
                result = CMVR_CALLING_AP_INVOCATION_ID_INVALID;
                
            if ( length == 4 && (buffer[0] != 2 || buffer[1] != 2) )
                result = CMVR_CALLING_AP_INVOCATION_ID_INVALID;
        }        
        
        if ( FindACSETag(pMsg, 0xAC, &index, &length) )
        {
            if ( !ValidAuthenticationValueFormat2008(pMsg) )
                result = CMVR_AUTHENTICATION_VALUE_INVALID;
        }
        
        if ( FindACSETag(pMsg, 0xBE, &index, &length) )
        {
            if ( !ValidInformationValueFormat2008(pMsg) )
                result = CMVR_INFORMATION_VALUE_INVALID;        
        }
        
        // maybe later check the contents of the BE tag        
    }
    
    pMsg->standardVersion = saveVersion;
    
    return result;
}






Boolean ACSE_Message_IsSegmented(ACSE_Message* pMsg, Boolean* pIsSegmented)
{
    if ( pMsg->standardVersion == CSV_MARCH2006 )
        *pIsSegmented = IsSegmentedFormat2006(pMsg);
    else
        *pIsSegmented = IsSegmentedFormat2008(pMsg);
        
    return TRUE;
}




void ACSE_Message_SetContext(ACSE_Message* pMsg )
{
    Unsigned8* buffer = &pMsg->buffer[pMsg->length];
    
    buffer[0] = 0xA1;
    if ( pMsg->standardVersion == CSV_MARCH2006 )
    {
        buffer[1] = sizeof(C1222ContextMarch2006);
        memcpy(&buffer[2], C1222ContextMarch2006, sizeof(C1222ContextMarch2006));
    }
    else
    {
        buffer[1] = sizeof(C1222Context2008);
        memcpy(&buffer[2], C1222Context2008, sizeof(C1222Context2008));
    }
    
    pMsg->length += (Unsigned16)(2 + buffer[1]);
}



Boolean ACSE_Message_HasContext(ACSE_Message* pMsg)
{
    return MessageHasTag(pMsg, 0xA1);
}



Boolean ACSE_Message_IsContextC1222(ACSE_Message* pMsg)
{
    Unsigned16 index;
    Unsigned16 length;
    
    if ( FindACSETag(pMsg, 0xA1, &index, &length ) )
    {
        if ( (length == sizeof(C1222ContextMarch2006)) && (memcmp(&pMsg->buffer[index], (void*)C1222ContextMarch2006, length) == 0) )
        {
            if ( (pMsg->standardVersion == CSV_MARCH2006) || (pMsg->standardVersion == CSV_UNKNOWN) )
                return TRUE;
        }
        
        if ( (length == sizeof(C1222Context2008)) && (memcmp(&pMsg->buffer[index], (void*)C1222Context2008, length) == 0) )
        {
            if ( (pMsg->standardVersion == CSV_2008) || (pMsg->standardVersion == CSV_UNKNOWN) )
                return TRUE;
        }

        return FALSE;
    }

    // if no context, assume c12.22
    
    return TRUE;
}



static void SetApTitle(ACSE_Message* pMsg, C1222ApTitle* pApTitle, Unsigned8 tag)
{
    Unsigned8* buffer = &pMsg->buffer[pMsg->length];
    
    buffer[0] = tag;
    buffer[1] = pApTitle->length;  // should easily be less than 128 bytes
    memcpy(&buffer[2], pApTitle->buffer, pApTitle->length);
    pMsg->length += (Unsigned16)(2 + pApTitle->length);
    
    // the acse version of the relative aptitle is different from the table version in c12.22 2008
    if ( pMsg->standardVersion == CSV_2008 )
    {
        if ( buffer[2] == 0x0d )
            buffer[2] = 0x80;
    }
}



void ACSE_Message_SetCalledApTitle(ACSE_Message* pMsg, C1222ApTitle* pApTitle)
{
    SetApTitle(pMsg, pApTitle, 0xA2);
}


void ACSE_Message_SetCallingApTitle(ACSE_Message* pMsg, C1222ApTitle* pApTitle)
{
    SetApTitle(pMsg, pApTitle, 0xA6);
}


static void SetApInvocationId(ACSE_Message* pMsg, Unsigned16 id, Unsigned8 tag)
{
    Unsigned8* buffer = &pMsg->buffer[pMsg->length];
    
    if ( pMsg->standardVersion == CSV_2008 )
    {
        buffer[0] = tag;
        if ( id < 256 )
        {
            buffer[1] = 3;
            buffer[2] = 0x02;
            buffer[3] = 1;
            buffer[4] = id;
            pMsg->length += (Unsigned16)5;
        }
        else
        {
            buffer[1] = 4;
            buffer[2] = 0x02;
            buffer[3] = 2;
            buffer[4] = (Unsigned8)(id>>8);
            buffer[5] = (Unsigned8)(id & 0xFF);
            pMsg->length += (Unsigned16)6;
        }
    }
    else // march 2006 standard
    {
        buffer[0] = tag;
        buffer[1] = sizeof(Unsigned16);
        buffer[2] = (Unsigned8)(id>>8);
        buffer[3] = (Unsigned8)(id&0xFF);
        pMsg->length += (Unsigned16)4;    
    }
}


void ACSE_Message_SetCallingApInvocationId(ACSE_Message* pMsg, Unsigned16 id)
{
    SetApInvocationId(pMsg, id, 0xA8);
}


void ACSE_Message_SetCalledApInvocationId(ACSE_Message* pMsg, Unsigned16 id)
{
    SetApInvocationId(pMsg, id, 0xA4);
}



// this encrypts the app-data in an encrypted message
// the message should already be correctly formatted (have a mac and padding to 8 bytes)
// cipher should be 0 for des or 1 for desede, and keylength depends on the cipher
// iv length can be 4 or 8 bytes and is extendded to 8 bytes if only 4

#ifdef C1222_INCLUDE_DEVICE // no encryption or authentication yet in comm module

Boolean ACSE_Message_Encrypt(ACSE_Message* pMsg, Unsigned8 cipher, Unsigned8* key,
                 Unsigned8* iv, Unsigned8 ivLength, Unsigned8* keybuffer,
                 void (*keepAliveFunctionPtr)(void* p), void* keepAliveParam)
{
    // the caller had to determine the key index from the auth value and from that determine
    // the key and cipher to use (0= des, 1=desede) which determines the key length
    
    static Unsigned8 tagList2008[] = { 0x28, 0x81 };
    
    Boolean ok = FALSE;
    Unsigned8* buffer = pMsg->buffer;
    Unsigned8 keyLength = (Unsigned8)((cipher==0) ? 8 : 24);
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    //Unsigned8 keybuffer[96];
    
    if ( FindACSETag(pMsg, 0xBE, &tagIndex, &tagLength) )
    {
        if ( pMsg->standardVersion == CSV_2008 )
        {
            buffer = &buffer[tagIndex];
            
            if ( !FindBufferTagListItem( buffer, tagLength, tagList2008, sizeof(tagList2008), &tagIndex, &tagLength) )
                return FALSE;
        }
        
        ok = C1222CBCEncrypt( &buffer[tagIndex], tagLength, key, keyLength, iv, ivLength, keybuffer,
                              keepAliveFunctionPtr, keepAliveParam);
    }
    
    return ok;
}

        

// returns false if the decryption failed for some reason or if the mac is not correct
// this can only be done on a complete acse message (not segmented)

Boolean ACSE_Message_Decrypt(ACSE_Message* pMsg, Unsigned8 cipher, Unsigned8* key, 
        Unsigned8* iv, Unsigned8 ivLength, Unsigned8* keybuffer, 
        void (*keepAliveFunctionPtr)(void* p), void* keepAliveParam)
{
    // the caller had to determine the key index from the auth value and from that determine
    // the key and cipher to use (0= des, 1=desede) which determines the key length
    
    static Unsigned8 tagList2008[] = { 0x28, 0x81 };
    
    Unsigned8* buffer = pMsg->buffer;
    Unsigned8 keyLength = (Unsigned8)((cipher==0) ? 8 : 24);
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned16 mac;
    Unsigned16 crc;
    C1222ApTitle apTitle;
    Unsigned16 save_crc;
    //Unsigned8 keybuffer[96];
    
    if ( FindACSETag(pMsg, 0xBE, &tagIndex, &tagLength) )
    {   
        // 2006: <app-data>	::=	[<mac>] <epsem> [<padding>]
        // 2008: <app-data> ::= <epsem> [<padding>] [mac]
        
        if ( pMsg->standardVersion == CSV_2008 )
        {
            buffer = &pMsg->buffer[tagIndex];
            
            if ( ! FindBufferTagListItem( buffer, tagLength, tagList2008, sizeof(tagList2008), &tagIndex, &tagLength) )
                return FALSE;
        }
        
        if ( C1222CBCDecrypt( &buffer[tagIndex], tagLength, key, keyLength, iv, ivLength, keybuffer,
                   keepAliveFunctionPtr, keepAliveParam) )
        {
            if ( pMsg->standardVersion == CSV_MARCH2006 )
            {
                // mac should be lsb first and we return lsb first
                mac = (Unsigned16)((buffer[tagIndex]) | (buffer[tagIndex+1]<<8));
                
                // mac is crc of concatenation of <epsem>, <padding>, <calling-aptitle>, 
                // <calling-ae-qualifier> and <calling-apinvocation-id>
                
                crc = C1222AL_StartCRC(&buffer[tagIndex+2], (Unsigned16)(tagLength-2));  // epsem and padding
                
                if ( ACSE_Message_GetCallingApTitle(pMsg, &apTitle) ) // calling aptitle
                {
                    crc = C1222AL_AddBufferToCRC( crc, apTitle.buffer, apTitle.length);
                }
            
                if ( FindACSETag(pMsg, 0xA7, &tagIndex, &tagLength ) )  // <calling-ae-qualifier>
                {
                    crc = C1222AL_AddBufferToCRC( crc, &buffer[tagIndex], tagLength );
                }
            
                if ( FindACSETag(pMsg, 0xA8, &tagIndex, &tagLength ) ) //<calling-apinvocation-id>
                {
                    crc = C1222AL_AddBufferToCRC( crc, &buffer[tagIndex], tagLength );
                }
                
                save_crc = crc;
            }
            else // 2008 standard
            {
                buffer = &buffer[tagIndex];
                
                mac = (Unsigned16)((buffer[tagLength-2]) | (buffer[tagLength-1]<<8));
                
                crc = C1222AL_StartCRC( buffer, (Unsigned16)(tagLength-2) );
            
                if ( FindACSETag(pMsg, 0xA7, &tagIndex, &tagLength ) && (tagLength==3) )
                {
                    tagIndex += (Unsigned16)2;  // skip over the 0x02 0x01 to the <calling-ae-qualifier>
                    tagLength -= (Unsigned16)2;
                    
                    crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[tagIndex], tagLength );
                }
            
                
                if ( FindACSETag(pMsg, 0xA8, &tagIndex, &tagLength ) && (tagLength==3 || tagLength==4) )  // <calling-apinvocation-id>
                {
                    tagIndex += (Unsigned16)2; // skip over the 0x02 0x01/0x02
                    tagLength -= (Unsigned16)2;
                    
                    crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[tagIndex], tagLength );
                }
                
                save_crc = crc;
            
                crc = AddCallingApTitleToCRC2008(pMsg, crc);
            }
            
            crc = C1222AL_FinishCRC(crc);
                    
            // might need to swap bytes on the crc before comparing
                    
            if ( crc == mac )
                return TRUE;
                
            // check the crc that does not include the calling aptitle
                
            crc = C1222AL_FinishCRC(save_crc);
            
            if ( crc == mac )
                return TRUE;
        }
    }
    
    return FALSE;
}




Unsigned16 AddCallingApTitleToCRC2008(ACSE_Message* pMsg, Unsigned16 crc)
{
    C1222ApTitle apTitle;
    C1222ApTitle absApTitle;
    Unsigned8 aptitleBuffer[C1222_APTITLE_LENGTH];
    
    if ( ACSE_Message_GetCallingApTitle(pMsg, &apTitle) )
    {
        if ( C1222ApTitle_IsAbsolute(&apTitle) )
        {
            crc = C1222AL_AddBufferToCRC( crc, &apTitle.buffer[2], apTitle.buffer[1]);
        }
        else
        {
            // need to convert to absolute for use in the crc
            C1222ApTitle_Construct(&absApTitle, aptitleBuffer, sizeof(aptitleBuffer));
        
            C1222ApTitle_MakeAbsoluteFrom(&absApTitle, &apTitle, FALSE); // we want 2008 form here
        
            crc = C1222AL_AddBufferToCRC( crc, &absApTitle.buffer[2], absApTitle.buffer[1]);
        }
    }
                
    return crc;
}



        
Boolean ACSE_Message_Authenticate(ACSE_Message* pMsg, Unsigned8 cipher, Unsigned8* key, Unsigned8* iv, Unsigned8 ivLength, Unsigned8* keybuffer)
{
    // the caller had to determine the key index from the auth value and from that determine
    // the key and cipher to use (0= des, 1=desede) which determines the key length
    
    Unsigned8 keyLength = (Unsigned8)((cipher==0) ? 8 : 24);

    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned8 randomHeader[2];
    Unsigned8 messageAuthToken[8];
    C1222ApTitle apTitle;
    Unsigned16 callingInfoMac;
    Unsigned16 appDataMac;
    Unsigned16 crc;
    Unsigned16 save_crc;
    Unsigned8* data;
    
    if ( ACSE_Message_GetAuthValue(pMsg, messageAuthToken) )
    {
        // the buffer should be a <msg-auth-token>
        // <msg-auth-token> ::= <calling-info-mac> <app-data-mac> <random-header> 00H 00H
        // <calling-info-mac> ::= <word16>
        // <app-data-mac> ::= <word16>
        // <random-header>	::=	<word16>
                    
        if ( C1222CBCDecrypt( messageAuthToken, sizeof(messageAuthToken), key, keyLength, iv, ivLength, keybuffer, 0, 0) )
        {
            // mac should be lsb first and we return lsb first
            callingInfoMac = (Unsigned16)((messageAuthToken[0]) | (messageAuthToken[1]<<8));
            appDataMac     = (Unsigned16)((messageAuthToken[2]) | (messageAuthToken[3]<<8));
            
            memcpy(randomHeader, &messageAuthToken[4], 2);
            
            // authenticate the calling info
            // in march 2006:
            // CRC of the concatenation of <random-header>, <calling-aptitle>, 
            //     <calling-ae-qualifier> and the <calling-apinvocation-id>
            // in 2008:
            // CRC of the concatenation of <random-header>, <calling-AE-qualifier>,
            // <calling-AP-invocation-id> and the CallingApTitle <universal-id>.
            
            // TODO - needs more work here!!!!
            
    
            crc = C1222AL_StartCRC( randomHeader, 2 );
    
            if ( pMsg->standardVersion == CSV_MARCH2006 )
            {
                if ( ACSE_Message_GetCallingApTitle(pMsg, &apTitle) )
                {
                    crc = C1222AL_AddBufferToCRC( crc, apTitle.buffer, apTitle.length);
                }

                if ( FindACSETag(pMsg, 0xA7, &tagIndex, &tagLength ) )  // <calling-ae-qualifier>
                {                
                    crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[tagIndex], tagLength );
                }
    
                if ( FindACSETag(pMsg, 0xA8, &tagIndex, &tagLength ) )
                {                
                    crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[tagIndex], tagLength );
                }
                
                save_crc = crc;

            }
            else
            {    
                if ( FindACSETag(pMsg, 0xA7, &tagIndex, &tagLength ) && (tagLength==3) )  // <calling-ae-qualifier>
                {
                    tagIndex += (Unsigned16)2;
                    tagLength -= (Unsigned16)2;
                
                    crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[tagIndex], tagLength );
                }
    
                if ( FindACSETag(pMsg, 0xA8, &tagIndex, &tagLength ) && (tagLength==3 || tagLength==4) )
                {
                    tagIndex += (Unsigned16)2;
                    tagLength -= (Unsigned16)2;
                
                    crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[tagIndex], tagLength );
                }
            
                save_crc = crc;
                
                crc = AddCallingApTitleToCRC2008(pMsg, crc);                
            }
    
            crc = C1222AL_FinishCRC(crc);
            
            // might need to swap bytes on the crc before comparing
            
            if ( crc != callingInfoMac )
                crc = C1222AL_FinishCRC(save_crc);
            
            if ( crc != callingInfoMac )
                return FALSE;
            
            // authenticate the app data
            
            crc = C1222AL_StartCRC( randomHeader, 2 );
            
            if ( ACSE_Message_GetApData(pMsg, &data, &tagLength) )
            {
                crc = C1222AL_AddBufferToCRC( crc, data, tagLength);
            }
            
            crc = C1222AL_FinishCRC(crc);
            
            // might need to swap bytes on the crc before comparing
            
            if ( crc != appDataMac )
                return FALSE;
                
            return TRUE;                
            
        } // end if was able to decrypt the auth token
    } // end if found auth value element
    
    return FALSE;    
}
#endif // C1222_INCLUDE_DEVICE


// if wantMac, will be encrypted and padded at the end, but encryption is not done in this
// routine

// returns C1222RESPONSE_OK or an error response

Unsigned8 ACSE_Message_SetUserInformation(ACSE_Message* pMsg, Epsem* epsemMessage, Boolean wantMac)
{
    Unsigned8* buffer = &pMsg->buffer[pMsg->length];
    Unsigned16 length;
    Unsigned8 padBytes = 0;
    Unsigned16 index;
    Unsigned16 crc;
    C1222ApTitle apTitle;
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned16 macStartIndex;
    Unsigned16 macLength;
    Unsigned16 ii;
    Unsigned16 user_information_length;
    Unsigned16 length28;
    
    if ( pMsg->standardVersion == CSV_MARCH2006 )
    {
        if ( (epsemMessage->length+20+pMsg->length) > pMsg->maxLength )
            return C1222RESPONSE_RSTL;
    
        buffer[0] = 0xBE;
    
        length = (Unsigned16)(epsemMessage->length + 1);  // 1 is for the epsem control
        if ( epsemMessage->epsemControl & 0x10 )
            length += (Unsigned16)sizeof(epsemMessage->deviceClass);
    
        if ( wantMac )
        {
            length += (Unsigned16)(sizeof(Unsigned16));
    
            padBytes = (Unsigned8)(8 - (length % 8));
    
            if ( padBytes == 8 )
                padBytes = 0;
    
            length += padBytes;
        }
    
        #ifdef WANT_TTB_BUG1
        index = (Unsigned16)(C1222Misc_EncodeLength(length+1, &buffer[1]) + 1);
        #else
        index = (Unsigned16)(C1222Misc_EncodeLength(length, &buffer[1]) + 1);
        #endif
    
        if ( wantMac )
        {
            *(Unsigned16*)(&buffer[index]) = 0;
    
            index += (Unsigned16)(sizeof(Unsigned16));
            
            macStartIndex = index;
        }
    
        buffer[index++] = epsemMessage->epsemControl;
        if ( epsemMessage->epsemControl & 0x10 )
        {
            memcpy(&buffer[index], epsemMessage->deviceClass, sizeof(epsemMessage->deviceClass));
            index += (Unsigned16)sizeof(epsemMessage->deviceClass);
        }
        
        //memcpy( &buffer[index], epsemMessage->buffer, epsemMessage->length);
        
        // this copies byte by byte so it will be possible for the epsem message to 
        // overlap the destination of the bytes
        
        for ( ii=0; ii<epsemMessage->length; ii++ )
        {
            buffer[index+ii] = epsemMessage->buffer[ii];
        }
        
        if ( padBytes > 0 )
        {   
            memset(&buffer[index+epsemMessage->length], 0, padBytes);
        }
        
        pMsg->length += (Unsigned16)(index+epsemMessage->length+padBytes);
        
        if ( wantMac )
        {
            // mac is crc of concatenation of <epsem>, <padding>, <calling-aptitle>, 
            // <calling-ae-qualifier>, and <calling-apinvocation-id>
            
            macLength = (Unsigned16)((index-macStartIndex) + epsemMessage->length + padBytes);
            
            crc = C1222AL_StartCRC( &buffer[macStartIndex], macLength );
            
            if ( ACSE_Message_GetCallingApTitle(pMsg, &apTitle) )
            {
                crc = C1222AL_AddBufferToCRC( crc, apTitle.buffer, apTitle.length);
            }
            
            if ( FindACSETag(pMsg, 0xA7, &tagIndex, &tagLength ) )  // <calling-ae-qualifier>
            {
                crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[tagIndex], tagLength );
            }
            
            if ( FindACSETag(pMsg, 0xA8, &tagIndex, &tagLength ) )  // <calling-apinvocation-id>
            {
                crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[tagIndex], tagLength );
            }
            
            crc = C1222AL_FinishCRC(crc);
    
            buffer[macStartIndex-2] = (Unsigned8)(crc & 0xFF);   // crc is supposed to be transmitted lsb first and that is the way C1222AL_FinishCRC returns it
            buffer[macStartIndex-1] = (Unsigned8)(crc >> 8);        
        }
    }
    else // 2008 standard
    {
        // the 26 bytes extra is for the following:
        //    1 control epsem byte 
        //    4 byte device class if needed
        //    up to 7 byte padding
        //    up to 2 byte crc
        //    up to 3 byte encoding of user-information length
        //    1 byte 0x81 tag
        //    up to 3 byte encoding of 0x28 tag length
        //    1 byte 0x28 tag
        //    up to 3 byte encoding of 0xbe tag length
        //    1 byte 0xbe tag
        // since I'm checking the max the message might actually have fit if we tried
        
        if ( (epsemMessage->length+26+pMsg->length) > pMsg->maxLength )
            return C1222RESPONSE_RSTL;
    
        buffer[0] = 0xBE;
    
        user_information_length = (Unsigned16)(epsemMessage->length + 1);  // 1 is for the epsem control
        if ( epsemMessage->epsemControl & 0x10 )
            user_information_length += (Unsigned16)sizeof(epsemMessage->deviceClass);
    
        if ( wantMac )
        {
            user_information_length += (Unsigned16)(sizeof(Unsigned16));  // for the crc
    
            padBytes = (Unsigned8)(8 - (user_information_length % 8));
    
            if ( padBytes == 8 )
                padBytes = 0;
    
            user_information_length += padBytes;
        }
        
        // each of the tags needs to account for the length of the sub tags
        
        length = user_information_length;
        length += C1222Misc_GetSizeOfLengthEncoding(user_information_length);  // add length of 81 tag length
        length ++; // 0x81 tag
        length28 = length;
        length += C1222Misc_GetSizeOfLengthEncoding(length28); // add size of 0x28 tag length 
        length ++; // 0x28 tag
    
        index = (Unsigned16)(C1222Misc_EncodeLength(length, &buffer[1]) + 1);
        buffer[index++] = 0x28;
        index += C1222Misc_EncodeLength(length28, &buffer[index]);
        buffer[index++] = 0x81;
        index += C1222Misc_EncodeLength(user_information_length, &buffer[index]);
        // index is now at the first byte of the user-information

        // in 2008 standard the mac if encrypted is the last 2 bytes of the user information        
    
        macStartIndex = index;
    
        buffer[index++] = epsemMessage->epsemControl;
        if ( epsemMessage->epsemControl & 0x10 )
        {
            memcpy(&buffer[index], epsemMessage->deviceClass, sizeof(epsemMessage->deviceClass));
            index += (Unsigned16)sizeof(epsemMessage->deviceClass);
        }
        
        //memcpy( &buffer[index], epsemMessage->buffer, epsemMessage->length);
        
        // this copies byte by byte so it will be possible for the epsem message to 
        // overlap the destination of the bytes
        
        for ( ii=0; ii<epsemMessage->length; ii++ )
        {
            buffer[index+ii] = epsemMessage->buffer[ii];
        }
        
        if ( padBytes > 0 )
        {   
            memset(&buffer[index+epsemMessage->length], 0, padBytes);
        }
        
        // set message length to just after the pad bytes (does not yet include the crc)
        
        pMsg->length += (Unsigned16)(index+epsemMessage->length+padBytes);
        
        if ( wantMac )
        {
            // mac is CRC of the concatenation of <epsem>, <padding>, <calling-AE-qualifier>,
            // <calling-AP-invocation-id> and the CallingApTitle <universal-id>.
            
            macLength = (Unsigned16)((index-macStartIndex) + epsemMessage->length + padBytes);
            
            crc = C1222AL_StartCRC( &buffer[macStartIndex], macLength );

            if ( FindACSETag(pMsg, 0xA7, &tagIndex, &tagLength ) && (tagLength == 3) )  // <calling-ae-qualifier>
            {
                tagIndex += (Unsigned16)2;
                tagLength -= (Unsigned16)2;
                
                crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[tagIndex], tagLength );
            }
            
            if ( FindACSETag(pMsg, 0xA8, &tagIndex, &tagLength ) && (tagLength==3 || tagLength==4) )  // <calling-apinvocation-id>
            {
                tagIndex += (Unsigned16)2;
                tagLength -= (Unsigned16)2;
                
                crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[tagIndex], tagLength );
            }

            #ifdef C1222_INCLUDE_DEVICE
            crc = AddCallingApTitleToCRC2008(pMsg, crc);
            #endif
                        
            crc = C1222AL_FinishCRC(crc);
            
            // in 2008 standard the crc goes after the epsem and padding
    
            pMsg->buffer[pMsg->length++] = (Unsigned8)(crc & 0xFF);   // crc is supposed to be transmitted lsb first and that is the way C1222AL_FinishCRC returns it
            pMsg->buffer[pMsg->length++] = (Unsigned8)(crc >> 8);        
        }        
    }
    
    // need to worry about buffer length
    
    return C1222RESPONSE_OK;
}


// this should be used in the previous routine eventually

Boolean ACSE_Message_UpdateUserInformationMac(ACSE_Message* pMsg)
{
    Unsigned16 tagLength;
    Unsigned16 tagIndex;
    Unsigned16 index;
    Unsigned16 length;
    Unsigned16 crc;
    C1222ApTitle apTitle;
    Unsigned8* buffer;
    Unsigned8 tagList2008[] = { 0x28, 0x81 };
    
    
    if ( pMsg->standardVersion == CSV_MARCH2006 )
    {
        if ( ! ACSE_Message_FindACSETag(pMsg, 0xBE, &index, &length ) )
            return FALSE;
            
        // mac is crc of concatenation of <epsem>, <padding>, <calling-aptitle>, 
        // <calling-ae-qualifier>, and <calling-apinvocation-id>
                
        crc = C1222AL_StartCRC( &pMsg->buffer[index+2], (Unsigned16)(length-2) );
        
        if ( ACSE_Message_GetCallingApTitle(pMsg, &apTitle) )
        {
            crc = C1222AL_AddBufferToCRC( crc, apTitle.buffer, apTitle.length);
        }
        
        if ( FindACSETag(pMsg, 0xA7, &tagIndex, &tagLength ) )  // <calling-ae-qualifier>
        {
            crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[tagIndex], tagLength );
        }
        
        if ( FindACSETag(pMsg, 0xA8, &tagIndex, &tagLength ) )  // <calling-apinvocation-id>
        {
            crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[tagIndex], tagLength );
        }
        
        crc = C1222AL_FinishCRC(crc);
        
        pMsg->buffer[index]   = (Unsigned8)(crc & 0xFF);   // crc is supposed to be transmitted lsb first
        pMsg->buffer[index+1] = (Unsigned8)(crc >> 8);
    }
    else // 2008 standard
    {
        if ( ! ACSE_Message_FindACSETag(pMsg, 0xBE, &index, &length ) )
            return FALSE;
        
        buffer = &pMsg->buffer[index];
        
        if ( ! FindBufferTagListItem( buffer, length, tagList2008, sizeof(tagList2008), &index, &length ) )
            return FALSE;
            
        // now positioned at the start of epsem

        // mac is CRC of the concatenation of <epsem>, <padding>, <calling-AE-qualifier>,
        // <calling-AP-invocation-id> and the CallingApTitle <universal-id>.
                
        crc = C1222AL_StartCRC( &buffer[index], (Unsigned16)(length-2) );

        if ( FindACSETag(pMsg, 0xA7, &tagIndex, &tagLength ) && (tagLength>2) )  // <calling-ae-qualifier>
        {
            crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[(Unsigned16)(tagIndex+2)], (Unsigned16)(tagLength-2) );
        }
        
        if ( FindACSETag(pMsg, 0xA8, &tagIndex, &tagLength ) && (tagLength>2) )  // <calling-apinvocation-id>
        {
            crc = C1222AL_AddBufferToCRC( crc, &pMsg->buffer[(Unsigned16)(tagIndex+2)], (Unsigned16)(tagLength-2) );
        }

        #ifdef C1222_INCLUDE_DEVICE
        crc = AddCallingApTitleToCRC2008(pMsg, crc);
        #endif
        
        crc = C1222AL_FinishCRC(crc);
        
        // in 2008 standard the crc goes after the epsem and padding

        buffer[index+length-2] = (Unsigned8)(crc & 0xFF);   // crc is supposed to be transmitted lsb first and that is the way C1222AL_FinishCRC returns it
        buffer[index+length-1] = (Unsigned8)(crc >> 8);        

    }
    
    return TRUE;
}




Boolean MessageUsesC1222Encryption(ACSE_Message* pMsg)
{
    Unsigned16 index;
    Unsigned16 length;
    Unsigned8* buffer;

    if ( FindACSETag(pMsg, 0xAB, &index, &length ) )  // 2006 encryption
    {
        buffer = &pMsg->buffer[index];

        if ( length != sizeof(C1222StandardEncryptionUniversalIDMarch2006) )
            return FALSE;
            
        if ( memcmp(buffer, (void*)C1222StandardEncryptionUniversalIDMarch2006, length) != 0 )
            return FALSE;
    }
    else if ( FindACSETag(pMsg, 0x8B, &index, &length ) ) // 2008 encryption
    {
        buffer = &pMsg->buffer[index];

        if ( length != sizeof(C1222StandardEncryptionUniversalID2008) )
            return FALSE;
            
        if ( memcmp(buffer, (void*)C1222StandardEncryptionUniversalID2008, length) != 0 )
            return FALSE;        
    }    

    return TRUE;  // default is C12.22 encryption
}





static Boolean GetAuthValueAddress(ACSE_Message* pMsg, Unsigned8** pauthvalue)
{
    Unsigned16 index;
    Unsigned16 length;
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned8* buffer;
    static Unsigned8 tagList2008[] = { 0xA2, 0xA0, 0xA1, 0x83 };
    static Unsigned8 tagList2006[] = { 0xA3 };
    Unsigned8* tagList;
    Unsigned8 tagListLength;

    if ( FindACSETag(pMsg, 0xAC, &index, &length ) )
    {
        buffer = &pMsg->buffer[index];

        if (  MessageUsesC1222Encryption(pMsg) )
        {
            if ( pMsg->standardVersion == CSV_2008 )
            {
                tagList = tagList2008;
                tagListLength = sizeof(tagList2008);
            }
            else
            {
                tagList = tagList2006;
                tagListLength = sizeof(tagList2006);
            } 

            if ( FindBufferTagListItem( buffer, length, tagList, tagListLength, &tagIndex, &tagLength) )
            {
                //memcpy(authvalue, &buffer[tagIndex], 8);
                *pauthvalue = &buffer[tagIndex];
                return TRUE;
            }
        }
	}
            

	// if not standard c12.22 encryption I have no idea, or can't find auth value, fail

    return FALSE;
}



Boolean ACSE_Message_GetAuthValue(ACSE_Message* pMsg, Unsigned8* authvalue)
{
    Unsigned8* authvalueaddress;
    
    if ( GetAuthValueAddress(pMsg, &authvalueaddress) )
    {
        memcpy(authvalue, authvalueaddress, 8);
        return TRUE;
    }
    
    return FALSE;
}






#ifdef C1222_INCLUDE_DEVICE

// this routine assumes that the user element in the auth value element includes a length
// byte (which is not as specified in the standard, but with is what is specified in 
// a couple of examples in the standard.  Also, tables test bench has the length byte here).

Boolean ACSE_Message_GetUserInfo(ACSE_Message* pMsg, Unsigned8* key, Unsigned8 cipher, 
            Unsigned8* iv, Unsigned8 ivLength, Unsigned16* pUserId, Unsigned8* password20, Unsigned8* keybuffer )
{
    Unsigned16 index;
    Unsigned16 length;
    Unsigned8* buffer;
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned8 userInfo[24];
    Unsigned16 mac;
    Unsigned16 crc;
    Unsigned8 keyLength;
    //Unsigned8 keybuffer[96];
    static Unsigned8 tagList2008[] = { 0xA2, 0xA0, 0xA1, 0x82 };
    static Unsigned8 tagList2006[] = { 0xA2 };
    Unsigned8* tagList;
    Unsigned8 tagListLength;
    

    if ( FindACSETag(pMsg, 0xAC, &index, &length ) )
    {
        buffer = &pMsg->buffer[index];
        
        if (  MessageUsesC1222Encryption(pMsg) )
        {
            if ( pMsg->standardVersion == CSV_2008 )
            {
                tagList = tagList2008;
                tagListLength = sizeof(tagList2008);
            }
            else
            {
                tagList = tagList2006;
                tagListLength = sizeof(tagList2006);
            } 

            if ( FindBufferTagListItem( buffer, length, tagList, tagListLength, &tagIndex, &tagLength) )
            {
                memset(userInfo, 0, sizeof(userInfo));
                
                if ( tagLength > 24 )
                    tagLength = 24;
                                        
                memcpy(userInfo, &buffer[tagIndex], tagLength);  // assume unsupplied bytes are 0
                
                // allow password lengths less than 20, but encrypted part must be a multiple of 8 bytes
                
                if ( tagLength <= 8 )
                    tagLength = 8;
                else if ( tagLength <= 16 )
                    tagLength = 16;
                else
                    tagLength = 24;
                
                keyLength = (Unsigned8)((cipher==0) ? 8 : 24);
                
                if ( (!ACSE_Message_IsEncrypted(pMsg) && !ACSE_Message_IsAuthenticated(pMsg)) ||
                      C1222CBCDecrypt( userInfo, tagLength , key, keyLength, iv, ivLength, keybuffer, 0, 0) )
                {
                    // check the user mac
                    mac = (Unsigned16)((userInfo[0]) | (userInfo[1]<<8));   // mac is lsb first
                        
                    crc = C1222AL_StartCRC(&userInfo[2], 22);  // user id and password
                    crc = C1222AL_FinishCRC(crc);
                    
                    if ( crc == mac )
                    {
                        *pUserId = (Unsigned16)((userInfo[2]<<8) | userInfo[3]);
                        memcpy(password20, &userInfo[4], 20);
                        return TRUE;
                    }
                
                    // else bad crc
                }
                
                // else encrypted and could not decrypt
            }
            
            // else no user element
        }
        
        // else not c12.22 encryption
	}

	// else can't find auth value

    return FALSE;
}




void ACSE_Message_SetAuthValueForEncryption( ACSE_Message* pMsg, Unsigned8 keyId, Unsigned8* iv, Unsigned8 ivLength)
{
    Unsigned8* buffer = &pMsg->buffer[pMsg->length];
    Unsigned16 length;
    Unsigned8 index;
    
    if ( pMsg->standardVersion == CSV_MARCH2006 )
    {
        length = (Unsigned16)(ivLength+2);  // iv tag
        if ( keyId != 0 )
            length += (Unsigned16)3;
        
        buffer[0] = 0xAC;
        buffer[1] = length;  // should easily be less than 128 bytes
        index = 2;
        
        if ( keyId != 0 )
        {
            buffer[2] = 0xA0;
            buffer[3] = 1;
            buffer[4] = keyId;
            index = 5;
        }
        
        buffer[index] = 0xA1;
        buffer[index+1] = ivLength;  // must be 4 or 8
        memcpy(&buffer[index+2], iv, ivLength);
        
        pMsg->length += (Unsigned16)(length+2);
    }
    else // 2008 standard
    {
        // AC <length1> A2 <length2> A0 <length3> A1 <length4> [80 L5 keyid] [81 L6 iv] [82 L7 user] [83 L8 auth]
        // tag length
        // 80  0 or 3 if keyid > 0
        // 81  6 or 10 depending on iv length
        // 82  26 or 0 - will be 0 since I won't send a user element from the meter
        // 83  10 or 0 - will be 0 since this is encryption not authentication
        // A1 length4 range 0+6 to 3+10   (6-13)
        // A0 length3 range 8-15
        // A2 length2 range 10-17
        // AC length1 range 12-19
        
        length = (Unsigned16)(ivLength + 8);
        if ( keyId != 0 )
            length += (Unsigned16)3;
            
        buffer[0] = 0xAC;
        buffer[1] = length;
        buffer[2] = 0xA2;
        buffer[3] = (Unsigned8)(length-2);
        buffer[4] = 0xA0;
        buffer[5] = (Unsigned8)(length-4);
        buffer[6] = 0xA1;
        buffer[7] = (Unsigned8)(length-6);
        
        if ( keyId != 0 )
        {
            buffer[8] = 0x80;
            buffer[9] = 1;
            buffer[10] = keyId;
            index = 11;
        }
        else
            index = 8;
            
        buffer[index] = 0x81;
        buffer[index+1] = ivLength;
        memcpy(&buffer[index+2], iv, ivLength);
        
        pMsg->length += (Unsigned16)(length+2);        
    }
}



#define C1222_USER_ELEMENT_SIZE   26

Boolean ACSE_Message_SetAuthValueForEncryptionWithUser( ACSE_Message* pMsg, Unsigned8 keyId, Unsigned8* iv, 
              Unsigned8 ivLength, Unsigned16 userId, Unsigned8* password20, Unsigned8* key, Unsigned8 cipher, Unsigned8* keybuffer)
{
    Unsigned8* buffer;
    Unsigned16 length;
    Unsigned16 index;
    Unsigned8 keyLength = (Unsigned8)((cipher==0) ? 8 : 24);

    Unsigned16 userMac;
    Boolean ok;
    
    ACSE_Message_SetAuthValueForEncryption( pMsg, keyId, iv, ivLength );
    
    buffer = &pMsg->buffer[pMsg->length];
    
    if ( ! FindACSETag(pMsg, 0xAC, &index, &length) )
        return FALSE;
    
    if ( pMsg->standardVersion == CSV_MARCH2006 )
    {
        // add 26 bytes for user-info to AC tag length
        pMsg->buffer[index-1] += (Unsigned8)C1222_USER_ELEMENT_SIZE;
        buffer[0] = 0xA2; 
    }
    else // 2008 standard
    {
        // AC L1 A2 L2 A0 L3 A1 L4 []
        // index points at the A2 tag
        pMsg->buffer[index-1] += (Unsigned8)C1222_USER_ELEMENT_SIZE; // L1
        pMsg->buffer[index+1] += (Unsigned8)C1222_USER_ELEMENT_SIZE; // L2
        pMsg->buffer[index+3] += (Unsigned8)C1222_USER_ELEMENT_SIZE; // L3
        pMsg->buffer[index+5] += (Unsigned8)C1222_USER_ELEMENT_SIZE; // L3
        buffer[0] = 0x82;
    }
    
    buffer[1] = 24;
     
    buffer[2] = 0; // user mac
    buffer[3] = 0;
    buffer[4] = (Unsigned8)(userId>>8);
    buffer[5] = (Unsigned8)(userId & 0xff);
    memcpy(&buffer[6], password20, 20);
    
    userMac = C1222AL_StartCRC(&buffer[4], C1222_USER_ELEMENT_SIZE-4);
    userMac = C1222AL_FinishCRC(userMac);
    
    buffer[2] = (Unsigned8)(userMac & 0xff);
    buffer[3] = (Unsigned8)(userMac >> 8);
    
    // encrypt the user info
    
    ok = C1222CBCEncrypt( &buffer[2], C1222_USER_ELEMENT_SIZE-2, key, keyLength, iv, ivLength, keybuffer, 0, 0);
    
    pMsg->length += (Unsigned16)C1222_USER_ELEMENT_SIZE;
    
    return ok;
}






#define C1222_AUTH_ELEMENT_SIZE   10

void ACSE_Message_SetInitialAuthValueForAuthentication( ACSE_Message* pMsg, Unsigned8 keyId, Unsigned8* iv, 
                                      Unsigned8 ivLength)
{
    Unsigned8* buffer = &pMsg->buffer[pMsg->length];
    Unsigned16 length;
    Unsigned8 index;
    
    
    if ( pMsg->standardVersion == CSV_MARCH2006 )
    {    
        length = (Unsigned16)(ivLength+2);  // iv tag
        if ( keyId != 0 )
            length += (Unsigned16)3;
        length += (Unsigned16)10; // auth token
        
        buffer[0] = 0xAC;
        buffer[1] = length;  // should easily be less than 128 bytes
        index = 2;
        
        if ( keyId != 0 )
        {
            buffer[2] = 0xA0;
            buffer[3] = 1;
            buffer[4] = keyId;
            index = 5;
        }
        
        buffer[index] = 0xA1;
        buffer[index+1] = ivLength;  // must be 4 or 8
        memcpy(&buffer[index+2], iv, ivLength);
        
        index += (Unsigned8)(2+ivLength);
        
        buffer[index++] = 0xA3;
        buffer[index++] = 8;
        buffer[index++] = 0; //calling info mac1 - will be filled in later
        buffer[index++] = 0; //                2 
        buffer[index++] = 0; //app data mac 1 - will be filled in later
        buffer[index++] = 0; //             2
        C1222Misc_RandomizeBuffer(&buffer[index],2);
        index += (Unsigned8)2;
        buffer[index++] = 0;
        buffer[index] = 0;
        
        pMsg->length += (Unsigned8)(length+2);
    }
    else // 2008 standard
    {
        // AC <length1> A2 <length2> A0 <length3> A1 <length4> [80 L5 keyid] [81 L6 iv] [82 L7 user] [83 L8 auth]
        // tag length
        // 80  0 or 3 if keyid > 0
        // 81  6 or 10 depending on iv length
        // 82  26 or 0 - will be 0 since I won't send a user element from the meter
        // 83  10 or 0 - will be 10 since this is authentication not encryption
        // A1 length4 range 0+6+10 to 3+10+10   (16-23)
        // A0 length3 range 18-25
        // A2 length2 range 20-27
        // AC length1 range 22-29
        
        length = (Unsigned16)(ivLength + 18);
        if ( keyId != 0 )
            length += (Unsigned16)3;
            
        buffer[0] = 0xAC;
        buffer[1] = length;
        buffer[2] = 0xA2;
        buffer[3] = (Unsigned8)(length-2);
        buffer[4] = 0xA0;
        buffer[5] = (Unsigned8)(length-4);
        buffer[6] = 0xA1;
        buffer[7] = (Unsigned8)(length-6);
        
        if ( keyId != 0 )
        {
            buffer[8] = 0x80;
            buffer[9] = 1;
            buffer[10] = keyId;
            index = 11;
        }
        else
            index = 8;
            
        buffer[index] = 0x81;
        buffer[index+1] = ivLength;
        memcpy(&buffer[index+2], iv, ivLength);
        
        index += (Unsigned8)(2 + ivLength);
        
        buffer[index++] = 0x83;
        buffer[index++] = 8;
        buffer[index++] = 0; //calling info mac1 - will be filled in later
        buffer[index++] = 0; //                2 
        buffer[index++] = 0; //app data mac 1 - will be filled in later
        buffer[index++] = 0; //             2
        C1222Misc_RandomizeBuffer(&buffer[index],2);
        index += (Unsigned8)2;
        buffer[index++] = 0;  // these bytes may need to be randomized
        buffer[index] = 0;
                
        pMsg->length += (Unsigned16)(length+2);                
    }
}



Boolean ACSE_Message_UpdateAuthValueForAuthentication(ACSE_Message* pMsg, Unsigned8 cipher, 
                            Unsigned8* key, Unsigned8* iv, Unsigned8 ivLength, Unsigned8* keybuffer)
{
    static Unsigned8 tagList2006User[] = { 0xA2 };
    static Unsigned8 tagList2008User[] = { 0xA2, 0xA0, 0xA1, 0x82 };
    static Unsigned8 tagList2006Auth[] = { 0xA3 };
    static Unsigned8 tagList2008Auth[] = { 0xA2, 0xA0, 0xA1, 0x83 };
    Unsigned16 index;
    Unsigned16 length;
    Unsigned8* buffer;
    Unsigned16 bufferLength;
    Unsigned16 callingInfoMac;
    Unsigned16 appDataMac;
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned8* messageBuffer;
    Unsigned8 keyLength;
    Unsigned8* tagList;
    Unsigned8 tagListLength;
    Unsigned8* data;
    
    keyLength = (Unsigned8)((cipher==0) ? 8 : 24);
    
    messageBuffer = pMsg->buffer;

    if ( FindACSETag(pMsg, 0xAC, &index, &bufferLength ) )
    {
        buffer = &pMsg->buffer[index];

        if (  MessageUsesC1222Encryption(pMsg) )
        {
            // if the user info is present, encrypt it
            
            if ( pMsg->standardVersion == CSV_2008 )
            {
                tagList = tagList2008User;
                tagListLength = sizeof(tagList2008User);
            }
            else
            {
                tagList = tagList2006User;
                tagListLength = sizeof(tagList2006User);
            } 

            if ( FindBufferTagListItem( buffer, bufferLength, tagList, tagListLength, &index, &length) )            
            {
                // encrypt the user info
                if ( !C1222CBCEncrypt( &buffer[index], length, key, keyLength, iv, ivLength, keybuffer, 0, 0) )
                    return FALSE;
            }

            if ( pMsg->standardVersion == CSV_2008 )
            {
                tagList = tagList2008Auth;
                tagListLength = sizeof(tagList2008Auth);
            }
            else
            {
                tagList = tagList2006Auth;
                tagListLength = sizeof(tagList2006Auth);
            } 

            if ( FindBufferTagListItem( buffer, bufferLength, tagList, tagListLength, &index, &length) )            
            {
                // TODO - for 2008 standard the crcs are supposed to be done with and without including the aptitles
                //        and the aptitles used are supposed to be absolute aptitles!!!!
                
                // <calling-info-mac> <apdata-mac> <random-header> xx xx
                
                // calculate calling info mac
                
                callingInfoMac = C1222AL_StartCRC(&buffer[index+4], 2);  // random header
                
                if ( pMsg->standardVersion == CSV_MARCH2006 )
                {                
                    if ( FindACSETag(pMsg, 0xA6, &tagIndex, &tagLength) )  // calling aptitle
                        callingInfoMac = C1222AL_AddBufferToCRC(callingInfoMac, &messageBuffer[tagIndex], tagLength);
                    
                    if ( FindACSETag(pMsg, 0xA7, &tagIndex, &tagLength) )  // calling ae qualifier
                        callingInfoMac = C1222AL_AddBufferToCRC(callingInfoMac, &messageBuffer[tagIndex], tagLength);
                    
                    if ( FindACSETag(pMsg, 0xA8, &tagIndex, &tagLength) )  // calling ap invocation id
                        callingInfoMac = C1222AL_AddBufferToCRC(callingInfoMac, &messageBuffer[tagIndex], tagLength);
                }
                else
                {                    
                    if ( FindACSETag(pMsg, 0xA7, &tagIndex, &tagLength) && (tagLength==3) )  // calling ae qualifier
                    {
                        tagIndex += (Unsigned16)2;
                        tagLength -= (Unsigned16)2;
                        
                        callingInfoMac = C1222AL_AddBufferToCRC(callingInfoMac, &messageBuffer[tagIndex], tagLength);
                    }
                    
                    if ( FindACSETag(pMsg, 0xA8, &tagIndex, &tagLength) && (tagLength==3 || tagLength==4) )  // calling ap invocation id
                    {
                        tagIndex += (Unsigned16)2;
                        tagLength -= (Unsigned16)2;
                        callingInfoMac = C1222AL_AddBufferToCRC(callingInfoMac, &messageBuffer[tagIndex], tagLength);                    
                    }
                    
                    callingInfoMac = AddCallingApTitleToCRC2008(pMsg, callingInfoMac);
                }
                    
                callingInfoMac = C1222AL_FinishCRC(callingInfoMac);
                
                // update the calling info mac
                
                buffer[index] = (Unsigned8)(callingInfoMac & 0xFF);  // lsb first
                buffer[index+1] = (Unsigned8)(callingInfoMac >> 8);
                
                // calculate app data mac

                appDataMac = C1222AL_StartCRC(&buffer[index+4], 2);  // random header
                                
                if ( ACSE_Message_GetApData(pMsg, &data, &tagLength) )
                {
                    appDataMac = C1222AL_AddBufferToCRC(appDataMac, data, tagLength);    
                }
                                    
                appDataMac = C1222AL_FinishCRC(appDataMac);
                
                // update the app data mac
                
                buffer[index+2] = (Unsigned8)(appDataMac & 0xFF); // lsb first
                buffer[index+3] = (Unsigned8)(appDataMac >> 8);
                
                // now encrypt the auth token
                
                if ( C1222CBCEncrypt( &buffer[index], 8, key, keyLength, iv, ivLength, keybuffer, 0, 0) )
                    return TRUE;
            }
        }
    }
    
    return FALSE;
}




void copyBytesUp(Unsigned8* dest, Unsigned8* source, Unsigned16 length)
{
    // make sure we don't destroy source bytes before transfer
    // since we are copying up, this means copy starting with the last byte of the source
    
    Unsigned16 ii;
    
    for ( ii=length; ii>0; ii-- )
        dest[ii-1] = source[ii-1];
}


void ACSE_Message_CopyBytesUp(Unsigned8* dest, Unsigned8* source, Unsigned16 length)
{
    copyBytesUp(dest, source, length);
}

void ACSE_Message_CopyBytesDown(Unsigned8* dest, Unsigned8* source, Unsigned16 length)
{
    Unsigned16 ii;
    
    for ( ii=0; ii<length; ii++ )
        dest[ii] = source[ii];
}


Boolean ACSE_Message_SetInitialAuthValueForAuthenticationWithUser( ACSE_Message* pMsg, Unsigned8 keyId, Unsigned8* iv, 
                                           Unsigned8 ivLength, Unsigned16 userId, Unsigned8* password20)
{
    Unsigned8* buffer;
    Unsigned16 length;
    Unsigned16 index;
    Unsigned16 userMac;
    
    ACSE_Message_SetInitialAuthValueForAuthentication( pMsg, keyId, iv, ivLength);
    
    buffer = &pMsg->buffer[pMsg->length-C1222_AUTH_ELEMENT_SIZE];  // points at the current start of the auth element
    
    // need to insert the user item before the auth tag
    // user item will be 26 bytes: A2 (or 82) LL <user-mac> <user-id> <password>
    // the auth tag is a 10 byte thing: A3 (or 83) LL <calling-info-mac> <apdata-mac> <random-header> 00 00
    
    // move the last 10 bytes of the message up 26 bytes
    
    copyBytesUp(buffer+C1222_USER_ELEMENT_SIZE, buffer, C1222_AUTH_ELEMENT_SIZE);
    
    if ( ! FindACSETag(pMsg, 0xAC, &index, &length) )
        return FALSE;
    
    // index is of the start of the data for the AC tag - the length is 1 byte before that
    
    if ( pMsg->standardVersion == CSV_MARCH2006 )
    {
        // add 26 bytes for user-info to AC tag length
        pMsg->buffer[index-1] += (Unsigned8)C1222_USER_ELEMENT_SIZE;
        buffer[0] = 0xA2; 
    }
    else // 2008 standard
    {
        // AC L1 A2 L2 A0 L3 A1 L4 []
        pMsg->buffer[index-1] += (Unsigned8)C1222_USER_ELEMENT_SIZE; // L1
        pMsg->buffer[index+1] += (Unsigned8)C1222_USER_ELEMENT_SIZE; // L2
        pMsg->buffer[index+3] += (Unsigned8)C1222_USER_ELEMENT_SIZE; // L3
        pMsg->buffer[index+5] += (Unsigned8)C1222_USER_ELEMENT_SIZE; // L4
        buffer[0] = 0x82;
    }
    
    buffer[1] = 24;
    buffer[2] = 0; // user mac
    buffer[3] = 0;
    buffer[4] = (Unsigned8)(userId>>8);
    buffer[5] = (Unsigned8)(userId & 0xff);
    memcpy(&buffer[6], password20, 20);
    
    userMac = C1222AL_StartCRC(&buffer[4], C1222_USER_ELEMENT_SIZE-4);
    userMac = C1222AL_FinishCRC(userMac);
    
    buffer[2] = (Unsigned8)(userMac & 0xff);
    buffer[3] = (Unsigned8)(userMac >> 8);

    // will encrypt this later?
    
    pMsg->length += (Unsigned8)(C1222_USER_ELEMENT_SIZE);
    
    return TRUE;
}








Boolean ACSE_Message_GetIV(ACSE_Message* pMsg, Unsigned8* iv4or8byte, Unsigned8* pIvLength)
{
    Unsigned16 index;
    Unsigned16 length;
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned8* buffer;
    static Unsigned8 tagList2008[] = { 0xA2, 0xA0, 0xA1, 0x81 };
    static Unsigned8 tagList2006[] = { 0xA1 };
    Unsigned8* tagList;
    Unsigned8 tagListLength;

    if ( FindACSETag(pMsg, 0xAC, &index, &length ) )
    {
        buffer = &pMsg->buffer[index];

        if (  MessageUsesC1222Encryption(pMsg) )
        {
            if ( pMsg->standardVersion == CSV_2008 )
            {
                tagList = tagList2008;
                tagListLength = sizeof(tagList2008);
            }
            else
            {
                tagList = tagList2006;
                tagListLength = sizeof(tagList2006);
            } 

            if ( FindBufferTagListItem( buffer, length, tagList, tagListLength, &tagIndex, &tagLength) )
            {
                if ( (tagLength == 4) || (tagLength == 8) )
                {                    
                    *pIvLength = tagLength;
                    memcpy(iv4or8byte, &buffer[tagIndex], tagLength);
                    return TRUE;
                }
            }
        }
	}
            

	// if not standard c12.22 encryption I have no idea, or can't find auth value, fail

    return FALSE;
}

#endif // C1222_INCLUDE_DEVICE


Boolean FindBufferTagListItem(Unsigned8* buffer, Unsigned16 length, Unsigned8* tagList, Unsigned8 tagListLength, 
                              Unsigned16* pOffset, Unsigned16* pLength)
{
    Unsigned8 ii;
    Unsigned16 offset = 0;
    Unsigned16 tagOffset;
    Unsigned16 tagLength;
    
    for( ii=0; ii < tagListLength; ii++ )
    {
        if ( !FindACSEBufferTag( &buffer[offset], length, tagList[ii], &tagOffset, &tagLength ) )
            return FALSE;
            
        offset += tagOffset;
        length = tagLength;
    }
    
    *pOffset = offset;
    *pLength = length;
    return TRUE;
}



Boolean ACSE_Message_GetEncryptionKeyId(ACSE_Message* pMsg, Unsigned8* pKeyId)
{
    Unsigned16 index;
    Unsigned16 length;
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned8* buffer;
    static Unsigned8 tagList2008[] = { 0xA2, 0xA0, 0xA1, 0x80 };
    static Unsigned8 tagList2006[] = { 0xA0 };
    Unsigned8* tagList;
    Unsigned8 tagListLength;

    if ( FindACSETag(pMsg, 0xAC, &index, &length ) )
    {
        buffer = &pMsg->buffer[index];

        if (  MessageUsesC1222Encryption(pMsg) )
        {
            *pKeyId = 0;  // if the key id is not specified, id 0 is assumed
            
            if ( pMsg->standardVersion == CSV_2008 )
            {
                tagList = tagList2008;
                tagListLength = sizeof(tagList2008);
            }
            else
            {
                tagList = tagList2006;
                tagListLength = sizeof(tagList2006);
            } 

            if ( FindBufferTagListItem( buffer, length, tagList, tagListLength, &tagIndex, &tagLength) )
            {
                *pKeyId = buffer[tagIndex];
            }
                
            return TRUE;               
        }
	}

	// if not standard c12.22 encryption I have no idea, or can't find auth value, fail

    return FALSE;
}



static Boolean ValidInformationValueFormat2008(ACSE_Message* pMsg)
{
    static Unsigned8 tagOrder1[] = { 0x28 };
    static Unsigned8 tagOrder1_28[] = { 0x02, 0x81 };
    Unsigned16 index;
    Unsigned16 length;
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned8* buffer;
    Unsigned16 fullLength;
    Unsigned16 offset;
    Epsem epsem;
    
    if ( FindACSETag(pMsg, 0xBE, &index, &length ) )
    {
        buffer = &pMsg->buffer[index];
        
        // check that there is just an 28 subtag
        if ( !BufferTagOrderOK(buffer, length, tagOrder1, sizeof(tagOrder1)) )
            return FALSE;
        
        if ( FindACSEBufferTag(buffer, length, 0x28, &tagIndex, &tagLength) )
        {
            buffer = &buffer[tagIndex];
            
            if ( ! BufferTagOrderOK(buffer, tagLength, tagOrder1_28, sizeof(tagOrder1_28)) )
                return FALSE;
                
            if ( FindACSEBufferTag(buffer, tagLength, 0x02, &index, &length) )
            {
                // if segmented this is the offset of the segment - else it is optional and has value 0 if present
                if ( ! IsSegmentedFormat2008(pMsg) )
                {
                    if ( length != 1 || buffer[1] != 0 )
                        return FALSE;
                }
                else
                {
                    // length should be 1 or 2 and value of offset should be less than full length
                    
                    if ( ACSE_Message_GetCalledAEQualifier2008(pMsg, &fullLength) )
                    {
                        if ( ACSE_Message_GetInfomationOffset2008(pMsg, &offset) )
                        {
                            if ( offset >= fullLength )
                                return FALSE;
                        }
                        else
                            return FALSE;
                    }
                    else
                        return FALSE;
                }
            }
            else if ( IsSegmentedFormat2008(pMsg) )
                return FALSE;
            
            if ( FindACSEBufferTag(buffer, tagLength, 0x81, &index, &length) )
            {
                if ( ACSE_Message_IsEncrypted(pMsg) )
                {
                    // information is not yet decrypted, so lets not check this yet
        
                    //Epsem_Init(&epsem, &pMsg->buffer, length-2);
        
                    //if ( !Epsem_Validate(&epsem) )
                    //    return FALSE;                    
                }
                else
                {
                    Epsem_Init(&epsem, &buffer[index], length);
                    
                    if ( !Epsem_Validate(&epsem) )
                        return FALSE;
                }
            } 
            else
                return FALSE;  // there is no actual information in this message
        }
    }
    else
        return FALSE;
    
    return TRUE; // seems to be ok
}

static Boolean ValidInformationValueFormatMarch2006(ACSE_Message* pMsg)
{
    Unsigned16 index;
    Unsigned16 length;
    Epsem epsem;
    
    // about all you can check is that the epsem control has the c12.22 bit set
    
    if ( ! FindACSETag(pMsg, 0xBE, &index, &length ) )
        return FALSE;
    
    if ( ACSE_Message_IsEncrypted(pMsg) )
    {
        // epsem is after the mac in 2006
        
        // information is not yet decrypted, so lets not check this yet
        
        //Epsem_Init(&epsem, &pMsg->buffer[2], length-2);
        
        //if ( !Epsem_Validate(&epsem) )
        //    return FALSE;
    }
    else
    {
        Epsem_Init(&epsem, &pMsg->buffer[index], length);
        
        if ( !Epsem_Validate(&epsem) )
            return FALSE;
    }
    
    return TRUE;  // just about anything is ok in March 2006
}


static Boolean ValidAuthenticationValueFormatMarch2006(ACSE_Message* pMsg)
{
    static Unsigned8 tagOrder[] = { 0xA0, 0xA1, 0xA2, 0xA3 };
    Unsigned8* buffer;
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned16 index;
    Unsigned16 length;
    
    // this checks that the format is valid - it may still reference keys that the meter does not have (for example)
    
    if ( FindACSETag(pMsg, 0xAC, &index, &length) )
    {
        buffer = &pMsg->buffer[index];
        
        if ( !MessageUsesC1222Encryption(pMsg) )
            return FALSE;  // we only support C12.22 encryption at the moment - so if they have an AC tag but use some other auth method, fail
        
        if ( !BufferTagOrderOK(buffer, length, tagOrder, sizeof(tagOrder)) )
            return FALSE;
            
        if ( FindACSEBufferTag(buffer, length, 0xA0, &tagIndex, &tagLength) ) // key id
        {
            if ( tagLength != 1 )
                return FALSE;
        }        
        
        if ( FindACSEBufferTag(buffer, length, 0xA1, &tagIndex, &tagLength) ) // iv
        {
            if ( tagLength != 4 && tagLength != 8 )
                return FALSE;
        } 
        
        if ( FindACSEBufferTag(buffer, length, 0xA2, &tagIndex, &tagLength) ) // user element
        {
            if ( tagLength != 24 )
                return FALSE;
        }        
               
        if ( FindACSEBufferTag(buffer, length, 0xA3, &tagIndex, &tagLength) ) // auth token
        {
            if ( tagLength != 8 )
                return FALSE;
        }
    }
    
    return TRUE;  // it is valid to not have an AC tag
}




static Boolean ValidAuthenticationValueFormat2008(ACSE_Message* pMsg)
{
    static Unsigned8 tagOrder1[] = { 0xA2 };
    static Unsigned8 tagOrder1a2[] = { 0x02, 0xA0, 0x81 };
    static Unsigned8 tagOrder1a2a0[] = { 0xA1 };
    static Unsigned8 tagOrder1a2a0a1[] = { 0x80, 0x81, 0x82, 0x83 };
    Unsigned8* buffer;
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned16 tag2Index;
    Unsigned16 tag2Length;
    Unsigned16 index;
    Unsigned16 length;
    
    // this checks that the format is valid - it may still reference keys that the meter does not have (for example)
    
    if ( FindACSETag(pMsg, 0xAC, &index, &length) )
    {
        buffer = &pMsg->buffer[index];
        
        if ( !MessageUsesC1222Encryption(pMsg) )
            return FALSE;  // we only support C12.22 encryption at the moment - so if they have an AC tag but use some other auth method, fail
       
        // check that there is just an A2 subtag
        if ( !BufferTagOrderOK(buffer, length, tagOrder1, sizeof(tagOrder1)) )
            return FALSE;
            
        // find the A2 subtag
        if ( FindACSEBufferTag(buffer, length, 0xA2, &tagIndex, &tagLength) )
        {
            buffer += tagIndex;
            
            if ( !BufferTagOrderOK(buffer, tagLength, tagOrder1a2, sizeof(tagOrder1a2)) )
                return FALSE;
                
            if ( FindACSEBufferTag(buffer, tagLength, 0x02, &tag2Index, &tag2Length) )
            {
                if ( tag2Length != 1 || buffer[tag2Index] != 0 )
                    return FALSE;
            }
            
            if ( FindACSEBufferTag(buffer, tagLength, 0xA0, &tag2Index, &tag2Length) )
            {
                buffer += tag2Index;
                
                if ( !BufferTagOrderOK(buffer, tag2Length, tagOrder1a2a0, sizeof(tagOrder1a2a0)) )
                    return FALSE;
                        
                if ( FindACSEBufferTag(buffer, tag2Length, 0xA1, &tag2Index, &tag2Length) )
                {
                	buffer += tag2Index;

                    if ( !BufferTagOrderOK(buffer, tag2Length, tagOrder1a2a0a1, sizeof(tagOrder1a2a0a1)) )  // is this order required?
                        return FALSE;
                        
                    if ( FindACSEBufferTag(buffer, tag2Length, 0x80, &tagIndex, &tagLength) )  // key-id
                    {
                        if ( tagLength != 1 )
                            return FALSE;
                            
                        // should I check that the key-id is defined in table 46?
                    }
                    
                    if ( FindACSEBufferTag(buffer, tag2Length, 0x81, &tagIndex, &tagLength) )  // iv
                    {
                        if ( tagLength != 4 && tagLength != 8 )
                            return FALSE;
                    }
                    
                    if ( FindACSEBufferTag(buffer, tag2Length, 0x82, &tagIndex, &tagLength) )  // user-mac user-id password
                    {
                        if ( tagLength < 4 || tagLength > 24 )
                            return FALSE;
                    }
                    
                    if ( FindACSEBufferTag(buffer, tag2Length, 0x83, &tagIndex, &tagLength) )  // auth token
                    {
                        if ( tagLength != 8 )
                            return FALSE;
                    }
                    
                }
                else
                    return FALSE;    
            }
            else
                return FALSE;  // we don't yet support <calling-authentication-value-octet-aligned> but this may be used for itron security in SR2.0
        }
        else
            return FALSE;                
    }
    
    return TRUE;  // it is valid to not have an AC tag
}



Boolean ACSE_Message_IsEncryptedOrAuthenticated(ACSE_Message* pMsg)
{
    Unsigned16 index;
    Unsigned16 length;

    if ( FindACSETag(pMsg, 0xAC, &index, &length ) )
    {
        if (  MessageUsesC1222Encryption(pMsg) )
            return TRUE;
    }

    return FALSE;
}


Boolean ACSE_Message_IsEncrypted(ACSE_Message* pMsg)
{
    Unsigned16 index;
    Unsigned16 length;
    Unsigned8 authtoken[8];

    if ( FindACSETag(pMsg, 0xAC, &index, &length ) )
    {
        if (  MessageUsesC1222Encryption(pMsg) )
        {
            if ( ACSE_Message_GetAuthValue(pMsg, authtoken) )
                return FALSE;  // its authenticated
            else
                return TRUE;
        }
	}
	
	// if no auth value it is not encrypted
	// if not standard c12.22 encryption I have no idea, but assume it is not encrypted

    return FALSE;
}


Boolean ACSE_Message_IsAuthenticated(ACSE_Message* pMsg)
{
    Unsigned16 index;
    Unsigned16 length;
    Unsigned8 authtoken[8];

    if ( FindACSETag(pMsg, 0xAC, &index, &length ) )
    {
        if (  MessageUsesC1222Encryption(pMsg) )
        {
            if ( ACSE_Message_GetAuthValue(pMsg, authtoken) )
                return TRUE;  // its authenticated
        }
	}
	
	// if no auth value it is not authenticated
	// if not standard c12.22 encryption I have no idea, but assume it is not authenticated

    return FALSE;
}    
    
    
    
    


C1222ResponseMode ACSE_Message_GetResponseMode(ACSE_Message* pMsg)
{
    Unsigned16 length;
    Unsigned8* buffer;
    Boolean isEncrypted;
    Unsigned8 responseControl;

    isEncrypted = ACSE_Message_IsEncrypted(pMsg);
    
    if ( ACSE_Message_GetApData( pMsg, &buffer, &length) )
    {
        if ( pMsg->standardVersion == CSV_MARCH2006 && isEncrypted )
            buffer += 2;  // skip over the <mac>    

    	// buffer now points to the start of epsem message

        responseControl = (Unsigned8)(buffer[0] & 0x03);
        return (C1222ResponseMode)responseControl;
    }
    
    return C1222RESPOND_NEVER;
}





Boolean ACSE_Message_IsTest(ACSE_Message* pMsg)
{
    Unsigned8 byte;
    
    if ( ACSE_Message_GetCallingAeQualifier(pMsg, &byte) )
        return (Boolean)((byte & 0x01) ? TRUE : FALSE);
    else
        return FALSE;
}


Boolean ACSE_Message_IsNotification(ACSE_Message* pMsg)
{
    Unsigned8 byte;
    
    if ( ACSE_Message_GetCallingAeQualifier(pMsg, &byte) )
        return (Boolean)((byte & 0x04) ? TRUE : FALSE);
    else
        return FALSE;
}


