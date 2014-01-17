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


#include "c1222aptitle.h"

static Boolean ApTitleIsBranchOf(C1222ApTitle* p, const Unsigned8* rootEncoding );


// March 2006 C12.22 aptitle root = "2.16.840.1.114223" encoded as "60 86 48 01 86 FC 2F"

Unsigned8 C1222ApTitleRootMarch2006[] = { 0x06, 0x07, 0x60, 0x86, 0x48, 0x01, 0x86, 0xFC, 0x2F };

// January 2008 C12.22 aptitle root = "2.16.124.113620.1.22.0" encoded as "06 08 60 7c 86 f7 54 01 16 00"

Unsigned8 C1222ApTitleRoot2008[] = { 0x06, 0x08, 0x60, 0x7c, 0x86, 0xf7, 0x54, 0x01, 0x16, 0x00 };

// this allows table format aptitles
Boolean C1222ApTitle_TableValidate( C1222ApTitle* p)
{
    if ( p->length < 3 )
        return FALSE;
        
    if ( p->buffer[0] == 0x06 || p->buffer[0] == 0x0d )
    {
        if ( ((p->buffer[1]) & 0x80) != 0 ) // if the aptitle length is more than 127 bytes its not an aptitle
            return FALSE;
            
        if ( (p->buffer[1]+2) > C1222_APTITLE_LENGTH )
            return FALSE;
            
        // call it ok
        
        return TRUE;
    }
    
    return FALSE;
}



Boolean C1222ApTitle_Validate( C1222ApTitle* p)
{
    if ( p->isACSE2008Format )
        return C1222ApTitle_ValidateACSEFormat2008_AnyRoot(p);  // allow any root since aptitle might be 2006 root in 2008 message
        
    if ( p->length < 3 )
        return FALSE;
        
    if ( p->buffer[0] == 0x06 || p->buffer[0] == 0x0d || p->buffer[0] == 0x80 )
    {
        if ( ((p->buffer[1]) & 0x80) != 0 ) // if the aptitle length is more than 127 bytes its not an aptitle
            return FALSE;
            
        if ( (p->buffer[1]+2) > C1222_APTITLE_LENGTH )
            return FALSE;
            
        // call it ok
        
        return TRUE;
    }
    
    return FALSE;
}


static Boolean ValidateACSEApTitle( C1222ApTitle* p, Unsigned8 relativeTag)
{
    if ( p->length < 3 )
        return FALSE;
        
    if ( p->buffer[0] == 0x06 || p->buffer[0] == relativeTag )
    {
        if ( ((p->buffer[1]) & 0x80) != 0 ) // if the aptitle length is more than 127 bytes its not an aptitle
            return FALSE;
            
        if ( (p->buffer[1]+2) > C1222_APTITLE_LENGTH )
            return FALSE;
            
        // call it ok
        
        return TRUE;
    }
    
    return FALSE;
}


Boolean C1222ApTitle_ValidateACSEFormatMarch2006( C1222ApTitle* p)
{
    if ( ValidateACSEApTitle(p, 0x0d) )
    {
        if ( !C1222ApTitle_IsAbsolute(p) || ApTitleIsBranchOf(p, C1222ApTitleRootMarch2006)  )        
            return TRUE;
    }
    
    return FALSE;
}


Boolean C1222ApTitle_ValidateACSEFormat2008( C1222ApTitle* p)
{
    if ( ValidateACSEApTitle(p, 0x80) )
    {
        if ( !C1222ApTitle_IsAbsolute(p) || ApTitleIsBranchOf(p, C1222ApTitleRoot2008)  )        
            return TRUE;
    }
    
    return FALSE;
}


Boolean C1222ApTitle_ValidateACSEFormat2008_AnyRoot(C1222ApTitle* p)
{
    return ValidateACSEApTitle(p, 0x80);
}


Boolean C1222ApTitle_ValidateACSEFormatMarch2006_AnyRoot(C1222ApTitle* p)
{
    return ValidateACSEApTitle(p, 0x0d);
}



void C1222ApTitle_ConvertACSERelativeToTableRelative(Unsigned8* aptitleBuffer)
{
    if ( aptitleBuffer[0] == 0x80 )
        aptitleBuffer[0] = 0x0d;
}


Unsigned8 C1222ApTitle_GetLength( C1222ApTitle* p)
{
    return (Unsigned8)(p->buffer[1] + 2);    
}


// when aptitles are constructed they will default to table format
// then when get an aptitle from a message will be converted to acse format if 2008
// it would be nice to have this record also whether the aptitle is 2006 or 2008 format
// to allow checking the beginning of absolute aptitles, but lets delay that - it would require
// changes to lots of calls to identify the standard being used.

void C1222ApTitle_Construct( C1222ApTitle* p, Unsigned8* buffer, Unsigned8 length)
{
    p->buffer = buffer;
    p->maxLength = length;
    p->length = 0;
    p->isACSE2008Format = FALSE;
}


// this ignores the absolute/relative tag in the first byte
// should work with two absolute aptitles or two relative aptitles that are relative to the same
// root

Boolean ApTitleIsBranchOf(C1222ApTitle* p, const Unsigned8* rootEncoding )
{
    if ( p->length <= (rootEncoding[1]+2) )
        return FALSE;
                    
    if ( (memcmp(&p->buffer[2], &rootEncoding[2], rootEncoding[1]) != 0) )
        return FALSE;
        
    return TRUE;
}



Boolean C1222ApTitle_MakeAbsoluteFrom( C1222ApTitle* p, C1222ApTitle* pSource, Boolean want2006 ) 
{
    Unsigned8 index;
    const Unsigned8* proot;
    
    if ( pSource->buffer[0] == 0x06 )
    {
        if ( p->maxLength < pSource->length )
            return FALSE;
            
        // already absolute - just copy it
        // need to convert between standard roots?
        
        memcpy(p->buffer, pSource->buffer, pSource->length);
        p->length = pSource->length;
    }
    else
    {
        proot = want2006 ? C1222ApTitleRootMarch2006 : C1222ApTitleRoot2008;
        
        if ( p->maxLength < (proot[1]+2) )
            return FALSE;
            
        // this is a relative aptitle (should be relative to c12.22 root)
        p->buffer[0] = 0x06;
        memcpy(&p->buffer[2], (void*)(&proot[2]), proot[1]);
        p->buffer[1] = proot[1];
        
        // now add the source bytes
        
        index = (Unsigned8)(p->buffer[1] + 2);
        
        if ( (index + pSource->buffer[1]) > p->maxLength )
            return FALSE;
            
        memcpy(&p->buffer[index], &pSource->buffer[2], pSource->buffer[1] );
        
        p->buffer[1] += pSource->buffer[1];
        
        p->length = (Unsigned8)(p->buffer[1] + 2);
    }
    
    return TRUE;
}


// this returns true for both acse relative and table relative encodings - may need separate routines

Boolean C1222ApTitle_IsRelative( C1222ApTitle* p)
{
    if ( p->buffer[0] == 0x0d )
        return TRUE;
        
    if ( p->buffer[0] == 0x80 )
        return TRUE;
        
    return FALSE;
}


Boolean C1222ApTitle_IsAbsolute( C1222ApTitle* p)
{
    return (Boolean)((p->buffer[0] == 0x06) ? TRUE : FALSE);
}




Boolean C1222ApTitle_MakeRelativeFrom( C1222ApTitle* p, C1222ApTitle* pSource)
{
    Unsigned16 absLength;
    
    if ( !C1222ApTitle_IsAbsolute(pSource) )
    {
        if ( p->maxLength < pSource->length )
            return FALSE;
            
        // already relative - just copy it
        memcpy(p->buffer, pSource->buffer, pSource->length);
        
        // in case source was acse relative, make it table relative form - this allows
        // c1222aptitle to treat a table and acse version of the same aptitle as equivalent
        
        C1222ApTitle_ConvertACSERelativeToTableRelative(p->buffer);
            
        p->isACSE2008Format = FALSE; 
            
        p->length = pSource->length;
    }
    else if ( ApTitleIsBranchOf(pSource, C1222ApTitleRootMarch2006) )
    {
        absLength = sizeof(C1222ApTitleRootMarch2006);  // 06 <length> <length-bytes>
        
        if ( pSource->length < absLength )
            return FALSE;
                    
        p->buffer[0] = 0x0d;
        memcpy(&p->buffer[2], &pSource->buffer[absLength], pSource->length - absLength);
        p->buffer[1] = (Unsigned8)(pSource->buffer[1] - absLength + 2);
        
        p->length = (Unsigned16)(2 + p->buffer[1]);  
        p->isACSE2008Format = FALSE;      
    }
    else if ( ApTitleIsBranchOf(pSource, C1222ApTitleRoot2008) )
    {
        absLength = sizeof(C1222ApTitleRoot2008);  // 06 <length> <length-bytes>
        
        if ( pSource->length < absLength )
            return FALSE;
                    
        p->buffer[0] = 0x0d;
        memcpy(&p->buffer[2], &pSource->buffer[absLength], pSource->length - absLength);
        p->buffer[1] = (Unsigned8)(pSource->buffer[1] - absLength + 2);
        
        p->isACSE2008Format = FALSE;
        
        p->length = (Unsigned16)(2 + p->buffer[1]);        
    }
    else
        return FALSE;  // it is in the wrong format
    
    return TRUE;
}


Boolean C1222ApTitle_Compare(C1222ApTitle* p1, C1222ApTitle* p2, int* pdifference)
{
    C1222ApTitle ap1;
    C1222ApTitle ap2;
    Unsigned8 ap1buffer[C1222_APTITLE_LENGTH];
    Unsigned8 ap2buffer[C1222_APTITLE_LENGTH];
    Unsigned16 ii;
    int diff = 0;
    
    C1222ApTitle_Construct(&ap1, ap1buffer, sizeof(ap1buffer));
    C1222ApTitle_Construct(&ap2, ap2buffer, sizeof(ap2buffer));
    
    if ( C1222ApTitle_MakeRelativeFrom(&ap1, p1) &&
         C1222ApTitle_MakeRelativeFrom(&ap2, p2) )
    {
        // compare byte for byte until the shorter of the lengths, then the 
        // longer one is greater or they are the same
        
        for ( ii=0; ii<ap1.length; ii++ )
        {
            if ( ii < ap2.length )
                diff = (int)(ap1.buffer[ii]) - (int)(ap2.buffer[ii]);
            else
                diff = 1;
                
            if ( diff != 0 )
            {
                *pdifference = diff;
                return TRUE;
            }
        }
        
        // same so far - check for length2 longer
        
        if ( ap2.length > ap1.length )
            diff = -1;
            
        *pdifference = diff;
        
        return TRUE;
    }
    
    return FALSE;
}





Boolean C1222ApTitle_Is2ndBranchOf1st( C1222ApTitle* p, C1222ApTitle* pBranch, Unsigned8* pBranchStartIndex)
{
    Unsigned8 abuffer1[C1222_APTITLE_LENGTH];
    Unsigned8 abuffer2[C1222_APTITLE_LENGTH];
    C1222ApTitle apTitle1;
    C1222ApTitle apTitle2;
    Unsigned8 branchStart;
    
    if ( C1222ApTitle_Validate(p) && C1222ApTitle_Validate(pBranch) )
    {
        // make both aptitles relative
        C1222ApTitle_Construct(&apTitle1, abuffer1, C1222_APTITLE_LENGTH);
        C1222ApTitle_Construct(&apTitle2, abuffer2, C1222_APTITLE_LENGTH);    
        
        if ( !C1222ApTitle_MakeRelativeFrom(&apTitle1, p) )
            return FALSE;
            
        if ( !C1222ApTitle_MakeRelativeFrom(&apTitle2, pBranch) )
            return FALSE;
        
        // if second param is a branch, it should be longer
        
        if ( ApTitleIsBranchOf(&apTitle2, apTitle1.buffer) )
        {
            branchStart = apTitle1.length;
            
            if ( C1222ApTitle_IsAbsolute(pBranch) ) // absolute aptitle, so add the c1222 root length
            {
                if ( memcmp(&pBranch->buffer[2], &C1222ApTitleRootMarch2006[2], C1222ApTitleRootMarch2006[1])==0 )
                    branchStart += C1222ApTitleRootMarch2006[1];
                else
                    branchStart += C1222ApTitleRoot2008[1];
            }
                
            *pBranchStartIndex = branchStart;
            
            return TRUE;
        }
    }
    
    return FALSE;
}





Boolean C1222ApTitle_IsApTitleMyCommModule(C1222ApTitle* pApTitle, C1222ApTitle* pMyApTitle)
{
    Unsigned8 branchStartIndex;
    
    if ( C1222ApTitle_Is2ndBranchOf1st(pMyApTitle, pApTitle, &branchStartIndex) )
    {
        if ( pApTitle->buffer[branchStartIndex] == 1 )
        {
            // check for nodeApTitle.1
            if ( pApTitle->length == (branchStartIndex+1) )
                return TRUE;
                
            // check for nodeApTitle.1.0
            if ( pApTitle->length == (branchStartIndex+2) )
            {
                if ( pApTitle->buffer[branchStartIndex+1] == 0 )
                    return TRUE;
                    
                 // TODO - need to handle nodeaptitle.1.interfaceNumber for compatibility with other comm modules besides rflan
                 // currently - ignore nodeApTitle.1.interfaceNumber for now since we don't use it from the rflan
            }
        }
    }    
    
    return FALSE;
}
       



// copies the aptitle and makes it table relative if acse relative

void C1222ApTitle_Copy(C1222ApTitle* dest, Unsigned8* destBuffer, C1222ApTitle* source)
{
    dest->buffer = destBuffer;
    dest->maxLength = source->maxLength;
    dest->length = source->length;
    memcpy(dest->buffer, source->buffer, source->length);  // there better be room !
    C1222ApTitle_ConvertACSERelativeToTableRelative(dest->buffer);
    dest->isACSE2008Format = FALSE;
}




Boolean C1222ApTitle_IsBroadcastToMe(C1222ApTitle* relativeApTitle, C1222ApTitle* myRelativeApTitle)
{
    // a broadcast ends in a .0 
    // if it is a broadcast to me, the part before the .0 will match the beginning of
    // my address
    
    // note - it is only a broadcast if the last byte is a 0 and this is a new term
    // otherwise it would interpret multiples of 128 as broadcasts!

    if ( (relativeApTitle->buffer[relativeApTitle->length-1] == 0) &&  ((relativeApTitle->buffer[relativeApTitle->length-2]&0x80) == 0) )
    {
        // yes, this is a broadcast
        
        if ( relativeApTitle->buffer[1] & 0x80 )
            return FALSE;  // I'm not going to handle aptitles that long (more than 127 bytes)
        
        if ( myRelativeApTitle->length >= relativeApTitle->length )
        {
            // the aptitle length includes the 0x0d and <length> at beginning, and .0 at end
            if ( memcmp(&relativeApTitle->buffer[2], &myRelativeApTitle->buffer[2], relativeApTitle->length-3) == 0 )
                return TRUE;
        }
    }
    
    return FALSE;    
}




#if 0
// this routine is more trouble than it is worth - maybe do later
Boolean C1222ApTitle_IsAMulticastAddress(C1222ApTitle* aptitle)
{
    // in 2006 standard its a multicast if the form is z.0.xxx where xxx is anything and zzz will match the beginning
    // of my address
    
    // in 2008 standard its a multicast if its relative form has an interior 0 term but the last term is not 0.  ie,
    // ignore the 0 in the root aptitle.
    
    Unsigned16 ii;
    Unsigned8 abuffer1[C1222_APTITLE_LENGTH];
    C1222ApTitle apTitle1;
    
    
    if ( aptitle->buffer[1] & 0x80 )
        return FALSE;  // don't support aptitles that long (> 127 bytes)
        
    if ( (aptitle->buffer[aptitle->length-1] == 0) && ((aptitle->buffer[aptitle->length-2]&0x80) == 0) )
        return FALSE;  // this appears to be a broadcast, not a multicast address
    
    for ( ii=2; ii<aptitle->length-1; ii++ )
        if ( (aptitle->buffer[ii] == 0) && ((aptitle->buffer[ii-1]&0x80) == 0) )  // this works even for the first term since the length is <128
            return TRUE;
            
    return FALSE;
}
#endif
