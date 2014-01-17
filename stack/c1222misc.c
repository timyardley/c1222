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



#include "c1222environment.h"
#include "c1222misc.h"

volatile Unsigned32 freeRunningTimeInMs = 0;



Unsigned8 C1222Misc_GetUIDLength(Unsigned8* uid)
{
    if ( uid[0] == 0 )
        return 0;
    else if ( uid[0] == 0x06 ) // absolute uid
        return (Unsigned8)(uid[1] + 2);
    else if ( uid[0] == 0x0d ) // relative uid
        return (Unsigned8)(uid[1] + 2);
    else
        return 0;  // invalid uid
}



// although theoretically the length could be just about anything, I don't think we will
// be dealing with lengths more than 65535 bytes in the meter

Unsigned16 C1222Misc_DecodeLength(Unsigned8* buffer, Unsigned8* pSizeofLength)
{
    Unsigned16 length = 0;
    Unsigned8 sizeofLength;
    Unsigned8 ii;
    Unsigned8 numberOfBytesInLength;
    
    if ( *buffer < 128 )
    {
        length = *buffer;
        sizeofLength = 1;
    }
    else
    {
        numberOfBytesInLength = (Unsigned8)((*buffer) & 0x7F);
        
        if ( numberOfBytesInLength > 2 )
        {
            *pSizeofLength = 0;
            return 0;
        }
        
        for ( ii=0; ii<numberOfBytesInLength; ii++ )
        {
            length <<= 8;
            length |= buffer[ii+1];
        }
        
        sizeofLength = (Unsigned8)(numberOfBytesInLength + 1);
    }
     
    *pSizeofLength = sizeofLength;
    return length;   
}



Unsigned8 C1222Misc_GetSizeOfLengthEncoding(Unsigned16 length)
{
    if ( length < 128 )
        return 1;
    else if ( length < 256 )
        return 2;
    else
        return 3;
}



// returns number of bytes in encoded length
Unsigned8 C1222Misc_EncodeLength( Unsigned16 length, Unsigned8* buffer )
{
    if ( length < 128 )
    {
        buffer[0] = length;
        return 1;
    }
    else if ( length < 256 )
    {
        buffer[0] = 0x81;
        buffer[1] = length;
        return 2;
    }
    else
    {
        buffer[0] = 0x82;
        buffer[1] = (Unsigned8)(length >> 8);
        buffer[2] = (Unsigned8)(length & 0xFF);
        return 3;
    }
    
}



Boolean C1222Misc_DelayExpired(Unsigned32 start, Unsigned32 delay)
{
    if ( (C1222Misc_GetFreeRunningTimeInMS() - start) >= delay )
        return TRUE;
    else
        return FALSE;
}


void C1222Misc_ResetFreeRunningTime(void)
{
    freeRunningTimeInMs = 0;
}



// this is just a stab at it.
Unsigned32 C1222Misc_GetFreeRunningTimeInMS(void)
{
    Unsigned32 snap1;
    Unsigned32 snap2;
    
    // the assumption here is that if a timer interrupt happens before both snaps are taken, it 
    // should not happen again before the routine is complete
    
    snap1 = freeRunningTimeInMs;
    snap2 = freeRunningTimeInMs;
    
    if ( snap1 == snap2 )
        return snap1;
    else
        return freeRunningTimeInMs;
}



// this should be called from a timer isr running at least every ? ms (maybe 200?)
void C1222Misc_IsrIncrementFreeRunningTime(Unsigned16 ms)
{
    freeRunningTimeInMs += ms;
}



static void DisableTimerInterrupt(void)
{
    // how to do it?    TODO
}


static void EnableTimerInterrupt(void)
{
    // how to do it???   TODO
}



// need to protect against an interrupt firing in this routine!

void C1222Misc_AdjustFreeRunningTime(Unsigned32 msToAdd )
{
    DisableTimerInterrupt();
    freeRunningTimeInMs += msToAdd;
    EnableTimerInterrupt();
}




void C1222Misc_ReverseBytes(void* pVoid, Unsigned8 length)
{
    Unsigned8 ii;
    Unsigned8 loopIterations = (Unsigned8)(length/2);
    Unsigned8 last = (Unsigned8)(length-1);
    Unsigned8* p = (Unsigned8*)pVoid;
    Unsigned8 temp;
    
    for ( ii=0; ii<loopIterations; ii++ )
    {
        temp = p[ii];
        p[ii] = p[last];
        p[last] = temp;
        last--;
    }
}





void C1222Misc_RandomizeBuffer(Unsigned8* buffer, Unsigned16 length)
{
    Unsigned16 ii;
    
    // srand is called in main - it does not need to be called here.
    //srand((Unsigned16)C1222Misc_GetFreeRunningTimeInMS());
    
    for ( ii=0; ii<length; ii++ )
        buffer[ii] = (Unsigned8)(rand()&0xFF);
}



Unsigned32 C1222Misc_GetRandomDelayTime(Unsigned32 minDelay, Unsigned32 maxDelay)
{
    Unsigned32 delta = maxDelay - minDelay;
    
    // the following srand may have been causing rand errors - rand values tended to be 
    // bunched at the top and bottom of the range.
    
    // srand is called in main - it does not need to be called here
    
    //srand((Unsigned16)C1222Misc_GetFreeRunningTimeInMS());
    
    if ( delta < RAND_MAX )
    {
        return minDelay + (rand() % delta);
    }
    else
    {
        return minDelay + (((float)rand() * (float)delta) / ((float)RAND_MAX));
    }
}

