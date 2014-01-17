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



// Handles C12.22 segmentation in the application layer

#include "c1222.h"
#include "c1222environment.h"
#include "c1222al_segmenter.h"
#include "c1222misc.h"
#include "c1222al.h"
#include "c1222event.h"


static Boolean isSegmentAlreadyReceived(C1222ALSegmenter* p, Unsigned16 offset, Unsigned16 length);

#define ALSG_CLEAR    0
#define ALSG_OCCUPIED 1
#define ALSG_COMPLETE 2


void C1222ALSegmenter_Init(C1222ALSegmenter* p)
{
    memset(p, 0, sizeof(C1222ALSegmenter));
    C1222ApTitle_Construct(&p->callingApTitle, p->callingApTitleBuffer, C1222_APTITLE_LENGTH);
}


void C1222ALSegmenter_SetLinks(C1222ALSegmenter* p, void* pApplication)
{
    p->pApplication = pApplication;
}


void C1222ALSegmenter_LogEvent(C1222ALSegmenter* p, C1222EventId event)
{
    C1222AL_LogEvent((C1222AL*)(p->pApplication), event);
}


void C1222ALSegmenter_LogEventAndULongs(C1222ALSegmenter* p, C1222EventId event, 
                 Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4)
{
    C1222AL_LogEventAndULongs((C1222AL*)(p->pApplication), event, u1, u2, u3, u4);
}




Boolean C1222ALSegmenter_SetUnsegmentedRxMessage(C1222ALSegmenter* p, ACSE_Message* pMsg)
{
    Unsigned16 sizeofLength;
    Unsigned8* buffer;
    
    C1222ALSegmenter_Clear(p);  // if receiving an unsegmented message, clear segment receive status
    
    buffer = p->workingMessageBuffer;
    
    buffer[0] = 0x60;
    sizeofLength = C1222Misc_EncodeLength(pMsg->length, &buffer[1]);
    
    if ( (pMsg->length+1+sizeofLength) > sizeof(p->workingMessageBuffer) )
        return FALSE;
    
    memcpy(&buffer[1+sizeofLength], pMsg->buffer, pMsg->length);
    
    p->workingMessageLength = (Unsigned16)(1 + sizeofLength + pMsg->length);
    
    // parse the message and determine the number of segments that will be required
    
    p->workingMessage.buffer = &buffer[1+sizeofLength];
    p->workingMessage.length = pMsg->length;
    p->workingMessage.standardVersion = pMsg->standardVersion;
    
    p->numberOfSegments = 1;
    p->rxMessageStartTime = C1222Misc_GetFreeRunningTimeInMS();
    
    return TRUE;
    
}




Boolean C1222ALSegmenter_SetUnsegmentedMessage(C1222ALSegmenter* p, ACSE_Message* pMsg)
{
    Unsigned16 sizeofLength;
    Unsigned8* buffer;
    Unsigned16 overheadLength;
    Unsigned16 payloadLength;
    
    C1222ALSegmenter_Clear(p);  // if receiving an unsegmented message, clear segment receive status
    
    buffer = p->workingMessageBuffer;
    
    buffer[0] = 0x60;
    sizeofLength = C1222Misc_EncodeLength(pMsg->length, &buffer[1]);
    
    memcpy(&buffer[1+sizeofLength], pMsg->buffer, pMsg->length);
    
    p->workingMessageLength = (Unsigned16)(1 + sizeofLength + pMsg->length);
    
    // parse the message and determine the number of segments that will be required
    
    p->workingMessage.buffer = &buffer[1+sizeofLength];
    p->workingMessage.length = pMsg->length;
    p->workingMessage.standardVersion = pMsg->standardVersion;
    
    if ( p->workingMessageLength <= p->fragmentSize )
        p->numberOfSegments = 1;
    else
    {
        // first need to determine how big the message overhead will be
    
        overheadLength = ACSE_Message_GetSegmentedMessageHeaderLength(&p->workingMessage);
    
        // add into overhead the part of the BE tag before the payload
        
        if ( pMsg->standardVersion == CSV_MARCH2006 )
        {
            // BEh <segment-app-data-length> <segment-app-data>
            // assume 3 byte length (>255 bytes) 
    
            overheadLength += (Unsigned16)(1 + 3);
        
        }
        else // 2008 standard
        {
            // BEh <length1> 28h <length2> 02h <length3> <seg-byte-offset> 81h <length4> <segment-app-data>
            // assume 2 byte seg-byte-offset (max)
            // assume 2 byte <length4> (ie, more than 128 but less than 256 - max)
            // assume 2 byte length1 and length2
            
            overheadLength += 4 + 3*C1222Misc_GetSizeOfLengthEncoding(p->fragmentSize)+1+2;
            
            //if ( p->fragmentSize < 128 )
            //    overheadLength += 10;
            //else if ( p->fragmentSize < 256 )
            //    overheadLength += 13;
            //else
            //    overheadLength += 16;
        } 

        // add into overhead the 0x60 and length of message at the beginning of the segmented message
        
        // (this is not quite right since the 0x60 and initial length are part of the fragment limited by fragment size)
        
        overheadLength += (Unsigned16)(1 + C1222Misc_GetSizeOfLengthEncoding(p->fragmentSize)); 
    
        // calculate and store numberOfSegments
    
        if ( p->fragmentSize > overheadLength )
        {
            payloadLength = (Unsigned16)(p->fragmentSize - overheadLength);
    
            p->numberOfSegments = (Unsigned16)((p->workingMessageLength-1)/payloadLength + 1);
            p->segmentPayloadLength = payloadLength;
                        
            C1222ALSegmenter_LogEventAndULongs(p, C1222EVENT_ALSG_P_S, payloadLength, p->numberOfSegments, p->fragmentSize, 0);
        }
        else
        {
            C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_OVERHEAD_TOO_LARGE);
            return FALSE;
        }
    }
    
    return TRUE;
}


void C1222ALSegmenter_SetFragmentSize(C1222ALSegmenter* p, Unsigned16 bytes)
{
    p->fragmentSize = bytes;
}



Unsigned16 C1222ALSegmenter_GetNumberOfSegments(C1222ALSegmenter* p)
{
    return p->numberOfSegments;
}





static Boolean copyTagFromFullMessage(C1222ALSegmenter* p, Unsigned8 tag, Unsigned16* pIndex, Unsigned16 maxLength, Unsigned8* buffer)
{
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned16 index = *pIndex;
    Unsigned8 sizeofLength;
    
    if ( ACSE_Message_FindACSETag(&p->workingMessage, tag, &tagIndex, &tagLength) )
    {
    	if ( maxLength < (Unsigned16)(index+ 6 + tagLength) )
    	{
    	    C1222ALSegmenter_LogEventAndULongs(p, C1222EVENT_ALSG_MAX_TOO_SMALL,2,maxLength,index+ 6 + tagLength,0);
        	return FALSE;
        }

        buffer[index] = tag;
        sizeofLength = C1222Misc_EncodeLength( tagLength, &buffer[index+1] );
        memcpy(&buffer[index+1+sizeofLength], &p->workingMessage.buffer[tagIndex], tagLength);
        
        index += (Unsigned16)(1 + sizeofLength + tagLength);
    }
    
    *pIndex = index;
    
    return TRUE;  // no error
}





// this gets the indicated fragment of a full message - it will not correctly fragment messages
// that have already been fragmented!

Boolean C1222ALSegmenter_GetSegment(C1222ALSegmenter* p, Unsigned16 segmentIndex, Unsigned8* messageBuffer,
                Unsigned16 maxLength, Unsigned16* pLength)
{
    Unsigned8* buffer;
    Unsigned8 sizeofLength;
    Unsigned16 tagIndex;
    Unsigned16 tagLength;
    Unsigned16 offset;
    Unsigned16 segmentLength;
    Unsigned16 index;
    Unsigned16 ii;
    Unsigned8 workingMessageStart;
    Unsigned16 totalLength;
    Boolean ok;
    Unsigned16 tempLength;
    Unsigned16 length28;
    
    static Unsigned8 copiedTagList2006[] = {    0xA1,   // context
                                                0xA2,   // called aptitle
                                                0xA6,   // calling aptitle
                                                0xA7 }; // calling entity qualifier
    
    if ( p->numberOfSegments == 1 )
    {
        if ( segmentIndex == 0 )
        {
            buffer = p->workingMessageBuffer;
            
            tagLength = (Unsigned16)(1 + C1222Misc_DecodeLength(&buffer[1], &sizeofLength));
            tagLength += sizeofLength;

            if ( tagLength > maxLength )
            	return FALSE;
            
            memcpy(messageBuffer, buffer, tagLength);
            
            *pLength = tagLength;
                        
            return TRUE;
        }
        else
        {
            C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_SEG_INDEX_INVALID);
            return FALSE;
        }
    }
    else
    {
        // acse message does not include the 0x60 and encoded total message length
        // need to copy the <application-context-element>, <called-aptitle-element>, 
        //   <calling-aptitle-element> and <calling-entity-qualifier-element>
        
        // the output of this needs to start with the 0x60 tag and then the encoded total length
        // at least now we need to know the size of the encoded total length in order to know
        // where to start with the rest of the message
        
        // we could calculate the total message length and put everything in the right place from
        // the start, but it seems easier to me to assume a size of length encoding, and
        // put the message according to that assumed location, then if the actual size of length is
        // different, relocate the message and finally encode the length
        
        // since this is a segmented message, the max size will be 150 bytes or so, so a 1 byte
        // length seems most likely

        if ( maxLength < 2 )
        {
            C1222ALSegmenter_LogEventAndULongs(p, C1222EVENT_ALSG_MAX_TOO_SMALL,1,maxLength,2,0);
        	return FALSE;
        }
            
        messageBuffer[0] = 0x60;
        messageBuffer[1] = 0;   // reserved length - will come back later and fix this
        
        workingMessageStart = (Unsigned8)(C1222Misc_GetSizeOfLengthEncoding(p->workingMessage.length) + 1);  // for the 0x60 and message length        
        
        offset = (Unsigned16)(segmentIndex * p->segmentPayloadLength);
            
        totalLength = (Unsigned16)(p->workingMessage.length + workingMessageStart);

        index = 0;
        buffer = &messageBuffer[2];        
        
        if ( p->workingMessage.standardVersion == CSV_MARCH2006 )
        {             
            for ( ii=0; ii<ELEMENTS(copiedTagList2006); ii++ )
            {
                if ( !copyTagFromFullMessage(p, copiedTagList2006[ii], &index, maxLength, buffer) )
                    return FALSE;
            }
    
            if ( maxLength < (Unsigned16)(index+8) )
            {
          	    C1222ALSegmenter_LogEventAndULongs(p, C1222EVENT_ALSG_MAX_TOO_SMALL,3,maxLength,index+8,0);
    
            	return FALSE;
            }
    
            // now create the <segment-calling-apinvocation-id-element>
    
            buffer[index] = 0xA8;
            buffer[index+1] = 0; // this is the length of the seg-id - 1 byte for the length should be adequate
    
            // encode the <seg-id> ::= <apdu-seq-number> <segment-byte-offset> <apdu-size>
            //     <apdu-seq-number> ::= <word16> and comes from the full message
            //     <segment-byte-offset> is 1..2 bytes and is based on segment number
            //     <apdu-size> is 1..2 bytes and is length of full apdu
    
            if ( !ACSE_Message_FindACSETag(&p->workingMessage, 0xA8, &tagIndex, &tagLength) )
            {
                C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_MISSING_A8_TAG);
                return FALSE;
            }
    
            // this is a required element
                
            // <calling-apinvocation-id> ::= <word16>
            buffer[index+2] = p->workingMessage.buffer[tagIndex];
            buffer[index+3] = p->workingMessage.buffer[tagIndex+1];    
    
            if ( totalLength > 255 )  // check this!!!!!
            {
                buffer[index+4] = (Unsigned8)(offset >> 8);
                buffer[index+5] = (Unsigned8)(offset & 0xff);
                buffer[index+6] = (Unsigned8)(totalLength >> 8);
                buffer[index+7] = (Unsigned8)(totalLength & 0xFF);
                buffer[index+1] = 6;
    
                index += (Unsigned16)8;
            }
            else
            {
                buffer[index+4] = offset;
                buffer[index+5] = totalLength;
                buffer[index+1] = 4;
    
                index += (Unsigned16)6;
            }
    
            // now copy the appropriate part of the full message
    
            segmentLength = (Unsigned16)((p->workingMessage.length+workingMessageStart) - offset);
            if ( segmentLength > p->segmentPayloadLength )
                segmentLength = p->segmentPayloadLength;
                            
            C1222ALSegmenter_LogEventAndULongs(p, C1222EVENT_ALSG_GET_SEG_NUM_OFF_T_L, segmentIndex, offset, totalLength, segmentLength);
    
            if ( maxLength < (Unsigned16)(index+4+segmentLength) )
            {
           	    C1222ALSegmenter_LogEventAndULongs(p, C1222EVENT_ALSG_MAX_TOO_SMALL,4,maxLength,index+4+segmentLength,0);
    
            	return FALSE;
            }
    
            buffer[index++] = 0xBE;
        }
        else // 2008 standard
        {
            // for 2008 standard we need A1, A2 copied from full message
            // then A3 which contains the length of the full message and indicates this is a segmented message
            // then A6, A7, A8 copied from the full message
            // then BE
            
            ok = copyTagFromFullMessage(p, 0xA1, &index, maxLength, buffer);
            
            ok = ok && copyTagFromFullMessage(p, 0xA2, &index, maxLength, buffer);
            
            if ( !ok )
                return FALSE;
            
            // add a3 tag: A3 <length> 02 <length2> <fullmessageLength>
            
            buffer[index++] = 0xA3;
            if ( totalLength < 256 )
            {
                buffer[index++] = 0x03;
                buffer[index++] = 0x02;
                buffer[index++] = 0x01;
                buffer[index++] = totalLength;
            }
            else
            {
                buffer[index++] = 0x04;
                buffer[index++] = 0x02;
                buffer[index++] = 0x02;
                buffer[index++] = totalLength>>8;
                buffer[index++] = totalLength & 0xFF;
            }
            
            // copy a6, a7, a8 tags
            
            ok = ok && copyTagFromFullMessage(p, 0xA6, &index, maxLength, buffer);

            ok = ok && copyTagFromFullMessage(p, 0xA7, &index, maxLength, buffer);
            
            ok = ok && copyTagFromFullMessage(p, 0xA8, &index, maxLength, buffer);
            
            if ( !ok )
                return FALSE;
                        
            // now copy the appropriate part of the full message
    
            segmentLength = (Unsigned16)((p->workingMessage.length+workingMessageStart) - offset);
            if ( segmentLength > p->segmentPayloadLength )
                segmentLength = p->segmentPayloadLength;
                            
            C1222ALSegmenter_LogEventAndULongs(p, C1222EVENT_ALSG_GET_SEG_NUM_OFF_T_L, segmentIndex, offset, totalLength, segmentLength);
    
            // figure out how long this message will be and fail if too long
            
            // BEh <length1> 28h <length2> 02h <length3> <seg-byte-offset> 81h <length4> <segment-app-data>
    
            tempLength = segmentLength + C1222Misc_GetSizeOfLengthEncoding(segmentLength) + 1;
            if ( offset < 256 )
                tempLength += 3;
            else
                tempLength += 4;
            length28 = tempLength;
            tempLength += C1222Misc_GetSizeOfLengthEncoding(tempLength) + 1; // now good starting at 0x28
    
            if ( maxLength < (Unsigned16)(index+1+C1222Misc_GetSizeOfLengthEncoding(tempLength)+tempLength) )
            {
           	    C1222ALSegmenter_LogEventAndULongs(p, C1222EVENT_ALSG_MAX_TOO_SMALL,4,maxLength,index+1+C1222Misc_GetSizeOfLengthEncoding(tempLength)+tempLength,0);
    
            	return FALSE;
            }
            
            // looks like it will fit - encode the be tag
            
            buffer[index++] = 0xBE;
            sizeofLength = C1222Misc_EncodeLength(tempLength, &buffer[index]);
            index += sizeofLength;
            
            buffer[index++] = 0x28;
            sizeofLength = C1222Misc_EncodeLength(length28, &buffer[index]);
            index += sizeofLength;
            
            buffer[index++] = 0x02;
            if ( offset < 256 )
            {
                buffer[index++] = 0x01;
                buffer[index++] = offset;
            }
            else
            {
                buffer[index++] = 0x02;
                buffer[index++] = offset >> 8;
                buffer[index++] = offset & 0xFF;
            }
            
            buffer[index++] = 0x81;
        }


        // encode length of the payload
        
        sizeofLength = C1222Misc_EncodeLength(segmentLength, &buffer[index]);
        index += sizeofLength;    

        if ( offset == 0 )
        {
            buffer[index++] = 0x60;
            sizeofLength = C1222Misc_EncodeLength(p->workingMessage.length, &buffer[index]);
            index += sizeofLength;
            memcpy(&buffer[index], p->workingMessage.buffer, segmentLength-workingMessageStart);
            index += (Unsigned16)(segmentLength - workingMessageStart);
        }
        else
        {
            if ( (offset-workingMessageStart+segmentLength) > p->workingMessage.length )
                segmentLength = (Unsigned16)(p->workingMessage.length - offset + workingMessageStart);
            
            memcpy( &buffer[index], &p->workingMessage.buffer[offset-workingMessageStart], segmentLength);
            index += segmentLength;
        }


        // index is now the length of the segmented message
        
        // encode the length and shift the message is necessary
        
        sizeofLength = (Unsigned8)(C1222Misc_GetSizeOfLengthEncoding(index));
        
        if ( sizeofLength > 1 ) // this is what we assumed above
        {
            C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_SHIFTING_MSG);
            // dang - need to fix the message
            offset = (Unsigned16)(sizeofLength - 1); // how many bytes to shift up the message
            
            for ( ii=(Unsigned16)(index+offset-1); ii>=offset; ii-- )
                buffer[ii] = buffer[ii-offset];
        }
                
        C1222Misc_EncodeLength(index, &messageBuffer[1]);
        *pLength = (Unsigned16)(1 + sizeofLength + index);
                
        return TRUE;
    }
}



void C1222ALSegmenter_Clear(C1222ALSegmenter* p)
{
    memset(p->workingMessageBuffer, 0, sizeof(p->workingMessageBuffer));
    memset(p->callingApTitleBuffer, 0, sizeof(p->callingApTitleBuffer));
    p->workingMessageLength = 0;
    p->workingSequence = 0;
    memset(p->disjointSegments, 0, sizeof(p->disjointSegments));
    p->disjointSegmentCount = 0;
    p->state = ALSG_CLEAR;
}





Boolean C1222ALSegmenter_IsBusy(C1222ALSegmenter* p)
{
    return (Boolean)((p->state == ALSG_CLEAR) ? FALSE : TRUE);
}



static void deleteDisjointSegment(C1222ALSegmenter* p, Unsigned8 indexToDelete)
{
    Unsigned8 ii;
    
    for ( ii=(Unsigned8)(indexToDelete+1); ii<p->disjointSegmentCount; ii++ )
        p->disjointSegments[(Unsigned8)(ii-1)] = p->disjointSegments[ii];
        
    p->disjointSegmentCount--;    
}



static Boolean isSegmentAlreadyReceived(C1222ALSegmenter* p, Unsigned16 offset, Unsigned16 length)
{
    Unsigned16 nextOffset;
    Unsigned8 ii;
    
    // see if there is room for this segment in the segmentation info
    
    nextOffset = (Unsigned16)(offset + length);
        
    // see if this segment touches one already there
    for ( ii=0; ii<p->disjointSegmentCount; ii++ )
    {
        if ( (offset >= p->disjointSegments[ii].offset) && (nextOffset <= p->disjointSegments[ii].nextOffset) )
            return TRUE;
    }
    
    return FALSE;
}


static Boolean manageDisjointSegmentInfo(C1222ALSegmenter* p, Unsigned16 offset, Unsigned16 length)
{
    Unsigned16 nextOffset;
    Unsigned8 ii;
    Unsigned8 workingIndex;
    Boolean segmentsToDelete[MAX_C1222_DISJOINT_SEGMENTS];
    
    memset(segmentsToDelete, FALSE, sizeof(segmentsToDelete));  // relies on boolean being a byte or false being 0, both of which are true
    
    // see if there is room for this segment in the segmentation info
    // we will assume that any overlapping bytes do not change
    
    nextOffset = (Unsigned16)(offset + length);
        
    // see if this segment touches or overlaps one already there
    
    for ( ii=0; ii<p->disjointSegmentCount; ii++ )
    {
        // if the new segment touches or overlaps any part of the segment at 
        // this index, fix the segment at this index to include the new segment
        // and then break - other segment cleanup will be handled below.
        
        if ( nextOffset < p->disjointSegments[ii].offset )
        {
            // the new segment is entirely before this one and does not touch it
        }
        else if ( offset > p->disjointSegments[ii].nextOffset )
        {
            // the new segment is entirely after this one and does not touch it
        }
        else 
        {
            // the new segment either overlaps or touches this segment
            // fix the current segment to include the new one and break
            
            if ( offset < p->disjointSegments[ii].offset )
                p->disjointSegments[ii].offset = offset;
                
            if ( nextOffset > p->disjointSegments[ii].nextOffset )
                p->disjointSegments[ii].nextOffset = nextOffset;
                
            break;
        }        
    }
        
    if ( ii == p->disjointSegmentCount )
    {
        // the segment did not touch or overlap any of the existing segments
        // need to add a new one
        
        if ( p->disjointSegmentCount == MAX_C1222_DISJOINT_SEGMENTS )
        {
            // this is a new segment - and we were already full - segerr
            C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_TOO_MANY_DISJOINT_SEGMENTS);
            return FALSE;
        }
        
        // add a new segment
        
        p->disjointSegments[ii].offset = offset;
        p->disjointSegments[ii].nextOffset = nextOffset;
        
        p->disjointSegmentCount++;
    }
    else
    {
        // the segment fit against another segment, or extended a segment - check if it filled a gap
        // actually it could fill more than one gap, overwriting entire segments that have already been
        // received.
        
        // the segment that changed was at index ii which is being put into "workingIndex".
        // so we need to extend that segment by each segment that overlaps part of it, unless that
        // segment is completely overlapped by the growing/modified segment.  And delete the segment that
        // was merged into workingIndex.
        
        workingIndex = ii;
        
        for ( ii=0; ii<p->disjointSegmentCount; ii++ )
        {
            if ( ii != workingIndex )
            {
                if ( p->disjointSegments[ii].nextOffset < p->disjointSegments[workingIndex].offset )
                {
                    // the ii'th segment is entirely before the working segment and does not touch it
                }
                else if ( p->disjointSegments[ii].offset > p->disjointSegments[workingIndex].nextOffset )
                {
                    // the ii'th segment is entirely after the working segment and does not touch it
                }
                else 
                {
                    // the ii'th segment either overlaps or touches the working segment
                    // fix the current segment and break
            
                    if ( p->disjointSegments[ii].offset < p->disjointSegments[workingIndex].offset )
                        p->disjointSegments[workingIndex].offset = p->disjointSegments[ii].offset;
                
                    if ( p->disjointSegments[ii].nextOffset > p->disjointSegments[workingIndex].nextOffset )
                        p->disjointSegments[workingIndex].nextOffset = p->disjointSegments[ii].nextOffset;
                
                    segmentsToDelete[ii] = TRUE;
                }        
            }                        
        }
        
        // if there was maintenance on the segments we may need to delete some
        
        for ( ii=p->disjointSegmentCount; ii>0; ii-- )
        {
            if ( segmentsToDelete[ii-1] )
            {
                deleteDisjointSegment(p, ii-1);
            }
        }
    }
    
    return TRUE;
    
}





Boolean C1222ALSegmenter_IsSegmentAlreadyReceived(C1222ALSegmenter* p, ACSE_Message* pMsg)
{
    Unsigned16 offset;
    Unsigned8* data;
    Unsigned16 length;
    Unsigned16 fullLength;
    Unsigned16 sequence;
    C1222ApTitle segmentCallingApTitle;
    int difference;
    Boolean haveSegmentCallingApTitle = FALSE;
    
    if ( p->state == ALSG_CLEAR )
        return FALSE;
        
    // if there is a calling aptitle in the segment and the full message, it should be the same
    // else both should be missing the calling aptitle
        
    if ( ACSE_Message_GetCallingApTitle(pMsg, &segmentCallingApTitle) && C1222ApTitle_Validate(&segmentCallingApTitle) )
        haveSegmentCallingApTitle = TRUE;
                
    if ( haveSegmentCallingApTitle && (p->callingApTitle.length > 0) )
    {                
        if ( C1222ApTitle_Compare(&segmentCallingApTitle, &p->callingApTitle, &difference) )
        {
            if ( difference != 0 )
                return FALSE;    
        }
    }
    else if ( haveSegmentCallingApTitle || (p->callingApTitle.length > 0) )
    {
        return FALSE;
    }        
    
    if ( ACSE_Message_GetSegmentOffset(pMsg, &offset, &fullLength, &sequence) )
    {
        if ( sequence == p->workingSequence )
        {
            if ( ACSE_Message_GetApData(pMsg, &data, &length ) )            
                return isSegmentAlreadyReceived(p, offset, length);
        }
    }
    
    return FALSE;
}




// this routine is used to add a received segment to the working message in the message receiver

Boolean C1222ALSegmenter_SetSegment(C1222ALSegmenter* p, ACSE_Message* pMsg)
{
    Unsigned16 offset;
    Unsigned8* data;
    Unsigned16 length;
    Unsigned16 fullLength;
    Unsigned16 sequence;
    Unsigned8 sizeofLength;
    C1222ApTitle segmentCallingApTitle;
    Boolean haveSegmentCallingApTitle = FALSE;
    int difference;
    Boolean needToClear = FALSE;
    
    
    if ( ACSE_Message_GetSegmentOffset(pMsg, &offset, &fullLength, &sequence) )
    {
        if ( ACSE_Message_GetApData(pMsg, &data, &length ) )
        {
            if ( offset >= fullLength )
            {
                C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_OFFSET_GE_FULLLENGTH);
                return FALSE;
            }
                
            if ( length >= fullLength )
            {
                C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_LENGTH_GE_FULLLENGTH);
                return FALSE;
            }
                
            if ( (offset+length) > fullLength )
            {
                C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_OFF_PLUS_LEN_GE_FULLLEN);
                return FALSE;
            }
                
            if ( fullLength > MAX_ACSE_MESSAGE_LENGTH )
            {
                C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_FULLLEN_TOO_BIG);
                return FALSE;
            }
                                
                
            if ( ACSE_Message_GetCallingApTitle(pMsg, &segmentCallingApTitle) && C1222ApTitle_Validate(&segmentCallingApTitle) )
                haveSegmentCallingApTitle = TRUE;
                
            // see if we need to clear the working message
                
            // both the full message and the segment should have the same calling aptitle
            // if one is missing the calling aptitle, both should be 
            // otherwise, clear the working message
                
            if ( p->state != ALSG_CLEAR )
            {    
                if ( sequence != p->workingSequence )
                    needToClear = TRUE;
                                
                if ( haveSegmentCallingApTitle && (p->callingApTitle.length > 2) )
                {                
                    if ( C1222ApTitle_Compare(&segmentCallingApTitle, &p->callingApTitle, &difference) )
                    {
                        if ( difference != 0 )
                            needToClear = TRUE;    
                    }
                    else
                        needToClear = TRUE;
                }
                else if ( haveSegmentCallingApTitle || (p->callingApTitle.length > 2) )
                {
                    needToClear = TRUE;    
                }
            
                if ( needToClear  )
                {
                    ((C1222AL*)(p->pApplication))->counters.partialMessageDiscardedBySegmenter++;
                    ((C1222AL*)(p->pApplication))->status.timeOfLastDiscardedSegmentedMessage = C1222Misc_GetFreeRunningTimeInMS();
                    C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_DISCARDED_MESSAGE);
                    C1222ALSegmenter_Clear(p);
                }
            }
                
                
            // I'm not sure why I did this - try it without for the moment
            //if ( (p->workingMessageLength != 0) &&
            //     (p->workingMessageLength != offset) )
            //    C1222ALSegmenter_Clear(p);
                
            if ( !manageDisjointSegmentInfo(p, offset, length ) )
            {
                C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_DISJOINT_SEG_ERR);
                return FALSE;
            }
                                
            C1222ALSegmenter_LogEventAndULongs(p, C1222EVENT_ALSG_SET_SEG_SQ_OFF_FULL_L, sequence, offset, fullLength, length);
                
            // update full message storage

            p->workingSequence = sequence;
            p->workingMessageLength = fullLength;
            memcpy(&p->workingMessageBuffer[offset], data, length);
            
            if ( haveSegmentCallingApTitle )
            {
                memcpy(p->callingApTitle.buffer, segmentCallingApTitle.buffer, segmentCallingApTitle.length);
                p->callingApTitle.length = segmentCallingApTitle.length;
                p->callingApTitle.isACSE2008Format = segmentCallingApTitle.isACSE2008Format;
            }
            else
                p->callingApTitle.length = 0;
                
            if ( p->state == ALSG_CLEAR )
                p->rxMessageStartTime = C1222Misc_GetFreeRunningTimeInMS();
            
            p->state = ALSG_OCCUPIED;
            
            if ( C1222ALSegmenter_IsMessageComplete(p) )
            {
                length = C1222Misc_DecodeLength(&p->workingMessageBuffer[1], &sizeofLength);
                
                p->workingMessage.buffer = &p->workingMessageBuffer[1+sizeofLength];
                p->workingMessage.length = length;
                p->workingMessage.standardVersion = pMsg->standardVersion;
                
                p->state = ALSG_COMPLETE;
            }
            
            return TRUE;
        }
        else
            C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_SET_SEG_CANT_GET_APP_DATA);
    }
    else
        C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_SET_SEG_CANT_GET_OFFSET);
    
    return FALSE;
}



Boolean C1222ALSegmenter_HandleSegmentTimeout(C1222ALSegmenter* p)
{
    if ( p->state == ALSG_OCCUPIED ) // timeout changed to 1 hour from 1 minute (scb)
    {
        if ( C1222Misc_DelayExpired(p->rxMessageStartTime, 3600000L ) )
        {
            C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_RX_SEGMENT_TIMEOUT);
            C1222ALSegmenter_Clear(p);
            return TRUE;
        }
    }
    
    return FALSE;
}





void C1222ALSegmenter_CancelPartialMessage(C1222ALSegmenter* p)
{
    if ( p->state == ALSG_OCCUPIED )
    {
        C1222ALSegmenter_LogEvent(p, C1222EVENT_ALSG_RX_MESSAGE_CANCELED);
        C1222ALSegmenter_Clear(p);
    }
}






// disjoint segment count 0 implies there is no message - so it is complete
Boolean C1222ALSegmenter_IsMessageComplete(C1222ALSegmenter* p)
{
    return  (Boolean)( (p->disjointSegmentCount == 0) || 
                       (
                         (p->disjointSegmentCount == 1) && 
                         (p->disjointSegments[0].offset == 0) && 
                         (p->workingMessageLength == p->disjointSegments[0].nextOffset)
                       ) 
                     );
}



ACSE_Message* C1222ALSegmenter_GetUnsegmentedMessage(C1222ALSegmenter* p) // returns null on error
{
    if ( C1222ALSegmenter_IsMessageComplete(p) )
        return &p->workingMessage;

	return 0;
}

ACSE_Message* C1222ALSegmenter_GetWorkingMessage(C1222ALSegmenter* p)
{
    return &p->workingMessage;
}


ACSE_Message* C1222ALSegmenter_StartNewUnsegmentedMessage(C1222ALSegmenter* p)
{
    p->workingMessage.buffer = p->workingMessageBuffer;
    p->workingMessage.length = 0;
    p->workingMessage.maxLength = MAX_ACSE_MESSAGE_LENGTH;
    p->workingMessage.standardVersion = CSV_UNKNOWN;
        
    return &p->workingMessage;
}


void C1222ALSegmenter_EndNewUnsegmentedMessage(C1222ALSegmenter* p)
{
    p->workingMessageLength = p->workingMessage.length;
    p->disjointSegmentCount = 1;
    p->disjointSegments[0].nextOffset = p->workingMessageLength;
}
