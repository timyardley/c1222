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


// C12.22 Local Port Router


//typedef struct C1222LocalPortRouterTag
//{
//    // position in array indicates destination of the packet
//    // ident field in array indicates source of packet, and crc is fixed
//    C1222DLPacket    XpacketsForPort[C1222_NUM_LOCAL_PORTS];
//    Boolean          XisBusy[C1222_NUM_LOCAL_PORTS];
//    
//} C1222LocalPortRouter;


#include "c1222environment.h"
#include "c1222dl.h"
#ifdef BORLAND_C
#include "mem.h"
#else
#include <string.h>
#endif

#define INVALID_INDEX   255

#ifdef ENABLE_LOCAL_PORT_ROUTER

Unsigned8 PortIDToIndex(Unsigned8 portID)
{
    if ( portID == 1 )
        return 0;
    else if ( portID == 3 )
        return 1;
    else
        return INVALID_INDEX;
}

// in the send packet routine, the packet local address defines the destination
// it changes the port number to the source and stores it in the array position for the 
// destination (and recalculates the crc)

void C1222LocalPortRouter_SendPacket(C1222LocalPortRouter* pLPR, C1222DLPacket* pPacket, Unsigned8 sourcePort)
{
    // get the destination address from the packet itself
    
    Unsigned8 index;
    C1222DLPacket* pSavedPacket;
    Unsigned16 crc;
    Unsigned16 crcPos;
    
    index = PortIDToIndex(C1222DLPacket_GetLocalAddress(pPacket));
    
    if ( index != INVALID_INDEX )
    {
        pSavedPacket = &(pLPR->XpacketsForPort[index]);
        
        memcpy(pSavedPacket, pPacket, sizeof(C1222DLPacket) );
        
        // update the local address in the packet with the source port
        
        pSavedPacket->buffer[1] &= 0x07; // strip off the old local address
        pSavedPacket->buffer[1] |= (Unsigned8)(sourcePort << 3);
        
        // recalculate the crc
        
        crcPos = (Unsigned16)(pSavedPacket->length-3);
        
        crc = C1222DL_CalcCRC(pSavedPacket->buffer, (Unsigned16)(crcPos+1));
        
        // put the updated crc in the packet (the LPR will use correct crc byte order only since the LPR was not used in release 1.0)
        
        pSavedPacket->buffer[crcPos] = (Unsigned8)(crc & 0xFF);
        pSavedPacket->buffer[crcPos+1] = (Unsigned8)(crc >> 8);
    }
}



Boolean C1222LocalPortRouter_IsPacketAvailableForPort(C1222LocalPortRouter* pLPR, Unsigned8 destPort)
{
    Unsigned8 index = PortIDToIndex(destPort);
    
    if ( index != INVALID_INDEX )
    {
        return pLPR->XisBusy[index];
    }
    
    return FALSE;
}



C1222DLPacket* C1222LocalPortRouter_GetPacketForPort(C1222LocalPortRouter* pLPR, Unsigned8 destPort)
{
    Unsigned8 index = PortIDToIndex(destPort);
    
    if ( index != INVALID_INDEX )
    {
        return &(pLPR->XpacketsForPort[index]);
    }
    
    return 0;    
}



void C1222LocalPortRouter_AcknowledgePacketForPort(C1222LocalPortRouter* pLPR, Unsigned8 destPort)
{
    Unsigned8 index = PortIDToIndex(destPort);
    
    if ( index != INVALID_INDEX )
    {
        pLPR->XisBusy[index] = FALSE;
    }    
}



Boolean C1222LocalPortRouter_IsDestinationPortBusy(C1222LocalPortRouter* pLPR, Unsigned8 destPort)
{
    return C1222LocalPortRouter_IsPacketAvailableForPort(pLPR, destPort);
}



void C1222LocalPortRouter_Init(C1222LocalPortRouter* pLPR)
{
    memset(pLPR, 0, sizeof(C1222LocalPortRouter));
}



void C1222LocalPortRouter_NotePowerUp(C1222LocalPortRouter* pLPR, Unsigned32 outageDurationInSeconds)
{
    // reinitialize local port router - discard any packets in transit
    
    C1222LocalPortRouter_Init(pLPR);
    
    (void)outageDurationInSeconds;
}


#endif // ENABLE_LOCAL_PORT_ROUTER




