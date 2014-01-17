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


// epsem message support

#include "epsem.h"
#include "c1222misc.h"


void Epsem_Init(Epsem* p, Unsigned8* buffer, Unsigned16 length)
{
    p->epsemControl = buffer[0];
    
    if ( p->epsemControl & 0x10 )
    {
        memcpy(p->deviceClass, &buffer[1], 4);
        p->buffer = &buffer[5];
        p->maxLength = (Unsigned16)(length - 5);
    }
    else
    {
        memset(p->deviceClass, 0, 4);
        p->buffer = &buffer[1];
        p->maxLength = (Unsigned16)(length - 1);
    }
    
    Epsem_Rewind(p);
}


Boolean Epsem_Validate(Epsem* p)
{
    Unsigned16 index;
    Unsigned16 length;
    Unsigned8 sizeofLength; 
    Boolean keepGoing = TRUE;   
    
    // check epsemControl
    
    if ( (p->epsemControl & 0xEC) != 0x80 )
        return FALSE;
        
    // check the epsem request/responses
        
    index = 0;
    
    while(keepGoing)
    {
        length = C1222Misc_DecodeLength(&p->buffer[index], &sizeofLength);
        
        if ( length == 0 )
            return TRUE;
            
        index += (Unsigned16)(sizeofLength + length);
        
        if ( index == p->maxLength )
            return TRUE;
            
        else if ( index > p->maxLength )
            keepGoing = FALSE;
    }
    
    return FALSE;
}



void Epsem_Rewind(Epsem* p)
{
    p->length = 0;
}



Boolean Epsem_GetNextRequestOrResponse(Epsem* p, Unsigned8** buffer, Unsigned16* pLength)
{
    Unsigned16 length;
    Unsigned8 sizeofLength;
    
    if ( p->length >= p->maxLength )
        return FALSE;
    
    length = C1222Misc_DecodeLength(&p->buffer[p->length], &sizeofLength);
    
    if ( length > 0 )
    {
        p->length += sizeofLength;
        *buffer = &p->buffer[p->length];
        *pLength = length;
        p->length += length;
        return TRUE;
    }
    
    return FALSE;
}


void Epsem_PositionToEnd(Epsem* p)
{
    Unsigned16 length;
    Unsigned8* buffer;
    
    while( Epsem_GetNextRequestOrResponse(p, &buffer, &length ) )
    {
        // nothing to do
        (void)buffer;
        (void)length;
    }
}



Boolean Epsem_AddRequestOrResponse(Epsem* p, Unsigned8* requestOrResponse, Unsigned16 length)
{
    Unsigned8 sizeofLength;
    
    // always terminate the chain
    
    if ( (p->length+C1222Misc_GetSizeOfLengthEncoding(length)+length) >= p->maxLength )
        return FALSE;
        
    sizeofLength = C1222Misc_EncodeLength( length, &p->buffer[p->length] );
    
    p->length += sizeofLength;
    
    memcpy(&p->buffer[p->length], requestOrResponse, length);
    
    p->length += length;
    
    //p->buffer[p->length] = 0;
    
    return TRUE;
}



Unsigned8 Epsem_GetResponseMode(Epsem* p)
{   
    return (Unsigned8)(p->epsemControl & 0x03);
}



Unsigned16 Epsem_GetFullLength(Epsem* p)
{
    Unsigned16 length = (Unsigned16)(1 + p->length);
    
    if ( p->epsemControl & 0x10 )
        length += (Unsigned16)4;
        
    return length;
}


