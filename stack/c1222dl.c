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


// C12.22 Datalink layer

// divides messages to send into packets and sends the packets.
// handles acknowledgements for packets
// receives packets and updates the working message (contained in transport layer).
// handles transparency if necessary.
// handles toggle bit.
// When a packet is received, if the packet is targeted to another port, provides 
// the packet to the local port router for disposition.
// Checks crc for received packets and generates crc etc for sent packets.
// if ack pending, sends bare ack (byte or message)


#include "c1222environment.h"
#include "c1222.h"
#include "c1222dl.h"
#include "c1222pl.h"
#include "c1222tl.h"
#include "c1222misc.h"
#include "c1222statistic.h"
#include "c1222al.h"


// static routines located in this module

//static void MakePacket(C1222DL* p, Unsigned8* message, Unsigned16 length, Unsigned8 sequence,
//                       Boolean multiple, Boolean first);

static void MakeAckPacket(C1222DL* p, Boolean ackNotNak);
static Boolean ProcessReceivedPacket(C1222DL* p); 
static Boolean ReceivePacket(C1222DL* p);
static void SendPacket(C1222DL* p);                      
static Boolean WaitForAck(C1222DL* p);
static void SendAcknowledge(C1222DL* p, Boolean wantAck, Boolean wantPacket);
static void SendAcknowledgeByte(C1222DL* p, Boolean wantAck);
static void SendAcknowledgePacket(C1222DL* p, Boolean wantAck);
static Unsigned16 MaxTxPacketPayloadLength(C1222DL* p);
static void StartPacket( C1222DL* p, Unsigned8 sequence, Boolean multiple, Boolean first);
static void FinishPacket( C1222DL* p);
static void AddToPacket( C1222DL* p, Unsigned8* message, Unsigned16 length);

#ifdef C1222_INCLUDE_COMMMODULE
static void SendEmptyPacket(C1222DL* p);
#endif // C1222_INCLUDE_COMMMODULE

static void ResetLink(C1222DL* p);
static void NoteAcknowledgedPacket(C1222DL* p);



// *************************************************************************************************
// public services for datalink layer
// *************************************************************************************************



Boolean C1222DL_IsInterfaceAvailable(C1222DL* p)
{
    return C1222PL_IsInterfaceAvailable((C1222PL*)(p->pPhysical));   
}




void CommonInit(C1222DL* p, Unsigned8 portID)
{   
    p->status.lastAcknowledgedMessageTime = C1222Misc_GetFreeRunningTimeInMS();
    
    p->status.ackPending = FALSE;
    p->status.rxToggle = p->status.txToggle = FALSE;
    p->status.rxToggleValid = FALSE;
    p->status.myTargetPortID = portID;


    p->status.oneTrySendInProgress = FALSE;
    
    p->possiblePacketStart = 0;
    
    p->status.lastPacketCRC = 0;

    p->status.responseTimeoutMs = C1222DL_RESPONSE_TIMEOUT_MS;
    p->status.trafficTimeoutMs = C1222DL_TRAFFIC_TIMEOUT_MS;
    p->status.intercharacterTimeoutMs = C1222DL_INTERCHARACTER_TIMEOUT_MS;
    p->status.retries = C1222DL_RETRIES;

    memset(&p->rxPacket, 0, sizeof(p->rxPacket));
    memset(&p->txPacket, 0, sizeof(p->txPacket));
    
    C1222DL_SetCRCByteOrderMode(p, FALSE, FALSE);
}


#ifdef C1222_INCLUDE_DEVICE
void C1222DL_ReInitAfterRFLANFwDownload(C1222DL* p, Unsigned8 portID)
{
    CommonInit(p, portID);
    p->status.maxTxPacketSize = 64;
    p->status.maxTxNumberOfPackets = 1;
    p->status.maxNumberOfChannels = 1;
    p->status.wantTransparency = FALSE;
}
#endif



void C1222DL_SetCRCByteOrderMode(C1222DL* p, Boolean detectOk, Boolean isReversed)
{
    // make sure the booleans are 0 or 1 since I will assume that
    p->status.crcDetectAllowed = detectOk   ? TRUE : FALSE;
    p->status.crcIsReversed    = isReversed ? TRUE : FALSE;
    
    C1222DL_ReenableCRCByteOrderDetect(p);
}



void C1222DL_ReenableCRCByteOrderDetect(C1222DL* p)
{
    p->counters.crcReverseCount = 0;
    p->counters.crcNormalCount = 0;
    
    p->status.crcDetectOn = p->status.crcDetectAllowed;
    // let isReversed stay the same
}




void C1222DL_Init(C1222DL* p, Boolean wantTransparency, Unsigned8 myTargetPortID, Boolean isCommModule)
{
    // can't zero structure because the links have already been set
    
    memset( &p->counters, 0, sizeof(p->counters));
    memset( &p->status, 0, sizeof(p->status));
    
    p->status.wantTransparency = wantTransparency;

    p->status.isCommModule = isCommModule;


    p->logWanted = FALSE;
    
    p->powerUpInProgress = FALSE;
    
    memset(&p->rxPacket.canary, 0xcd, sizeof(p->rxPacket.canary));
    memset(&p->txPacket.canary, 0xcd, sizeof(p->txPacket.canary));
    
    CommonInit(p, myTargetPortID);
}


static void SetDefaultParameters(C1222DL* p)
{
    C1222DL_SetParameters(p, 9600, 64, 1 ); 
}

void C1222DL_InitAtEndOfStackInit(C1222DL* p)
{
    // initialize to default params
    //   this sets physical baudRate, maxPacketSize, maxNumberOfPackets, and maxNumberOfChannels
    //   it also resets the intercharacter timeout
    
    SetDefaultParameters(p);    
}



// this will be called from a different task than the c12.22 task in the register
// namely from the event manager task - should abort any pending reads or maybe other requests to the rflan

void C1222DL_PowerUpInProgress(C1222DL* p)
{
    C1222PL* pPhysical = (C1222PL*)(p->pPhysical);
    
    // anything else needed?
    
    p->powerUpInProgress = TRUE;
    
    C1222PL_PowerUpInProgress(pPhysical);
}





void C1222DL_NotePowerUp(C1222DL* p, Unsigned32 outageDurationInSeconds)
{
    C1222PL* pPhysical = (C1222PL*)(p->pPhysical);
    
    p->powerUpInProgress = FALSE;
    
    p->status.rxToggleValid = FALSE;
    
    C1222PL_NotePowerUp(pPhysical, outageDurationInSeconds);
    SetDefaultParameters(p);
    
    NoteAcknowledgedPacket(p);
}




void C1222DL_LogEventAndBytes(C1222DL* p, C1222EventId event, Unsigned8 n, Unsigned8* bytes)
{
    C1222TL* pTL = (C1222TL*)(p->pTransport);
    
    if ( p->logWanted )
        C1222TL_LogEventAndBytes(pTL, event, n, bytes);
    else
        C1222AL_IncrementEventCount((C1222AL*)(pTL->pApplication), (Unsigned16)event);
        
}





void C1222DL_LogEvent(C1222DL* p, C1222EventId event)
{
    C1222TL* pTL = (C1222TL*)(p->pTransport);
    
    if ( p->logWanted )
        C1222TL_LogEvent(pTL, event);
    else
        C1222AL_IncrementEventCount((C1222AL*)(pTL->pApplication), (Unsigned16)event);
}



void C1222DL_LogEventAndULongs(C1222DL* p, C1222EventId event, 
        Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4)
{
    C1222TL* pTL = (C1222TL*)(p->pTransport);
    
    if ( p->logWanted )
        C1222TL_LogEventAndULongs(pTL, event, u1, u2, u3, u4);
    else
        C1222AL_IncrementEventCount((C1222AL*)(pTL->pApplication), (Unsigned16)event);
}


#ifdef C1222_INCLUDE_COMMMODULE
Boolean C1222DL_IsCommModule(C1222DL* p)
{
    return p->status.isCommModule;
}
#endif



void C1222DL_SetLinks(C1222DL* p, void* physical, void* transport, void* localPortRouter)
{
    // this must be called before _Init because the physical layer is used in there!
    
    p->pPhysical = physical;
    p->pTransport = transport;
    p->pLocalPortRouter = localPortRouter;
}



// void C1222DL_SetParameters(C1222DL* p, Unsigned32 baud, Unsigned16 packetSize, Unsigned8 numberOfPackets)
//
// notifies the datalink of new negotiated parameters
// datalink implements the changes immediately (changes baud, packet size, and number of packets).

#ifdef METER_CODE
extern void C1222HistLog(Unsigned32 value);
#endif

void C1222DL_SetParameters(C1222DL* p, Unsigned32 baud, Unsigned16 packetSize, Unsigned8 numberOfPackets)
{
    C1222DL_LogEvent(p, C1222EVENT_DL_SETPARAMETERSSTART);
    
#ifdef METER_CODE    
    C1222HistLog(10);
#endif    
    
    C1222PL_ConfigurePort( (C1222PL*) p->pPhysical, baud);
    
    // also reset the intercharacter timeout - which fixes a problem that the rflan resets the icto to a default
    // value in configure port
    
    C1222PL_SetICTO( (C1222PL*)(p->pPhysical), p->status.intercharacterTimeoutMs );

    
    // if the packet size is too big it will overflow our buffer - but if the peer
    // negotiated a larger packet and smaller number of packets we may no longer be
    // able to send an entire transport message!!
    
    if ( packetSize > C1222DL_MaxSupportedPacketSize(p) )
        packetSize = C1222DL_MaxSupportedPacketSize(p);
    
    p->status.maxTxPacketSize = packetSize;
    p->status.maxTxNumberOfPackets = numberOfPackets;
    p->status.maxNumberOfChannels = 1;
    
    C1222DL_LogEventAndULongs(p, C1222EVENT_DL_SETPARAMETERS, baud, packetSize, numberOfPackets, 0);
}


void C1222DL_ReconfigurePort(C1222DL* p)
{
    C1222PL_ConfigurePort( (C1222PL*) p->pPhysical, C1222DL_GetCurrentBaud(p) );   
}



Unsigned32 C1222DL_GetCurrentBaud(C1222DL* p)
{
    C1222PL* pPhysical = (C1222PL*)(p->pPhysical);
    
    return pPhysical->status.baudRate;
}



// need to handle timeouts in this routine

Boolean IsPossiblePacketAvailable(C1222DL* p)
{
    Unsigned16 length;
    C1222PL* pPhysical = (C1222PL*)(p->pPhysical);
    Unsigned32 now;
    
    if ( C1222PL_IsByteAvailable(pPhysical) )
    {
        now = C1222Misc_GetFreeRunningTimeInMS();
        
        if ( p->possiblePacketStart == 0 )
            p->possiblePacketStart = now;
            
        if ( (now-p->possiblePacketStart) > 500 )
            return TRUE;

        // need to do this before checking for 8 bytes available to avoid trashing
        // 8 bytes due to a bad byte at the beginning!
        
        if ( C1222PL_PeekByteAtIndex(pPhysical, 0) != C1222DL_STP )
            return TRUE;
            
        if ( C1222PL_Are8BytesAvailable(pPhysical) )
        {                
            if ( C1222PL_PeekByteAtIndex(pPhysical, 2) & 0x10 )
                return TRUE;  // not handling transparency in the lookahead buffer at the moment
                
            length = (Unsigned16)((C1222PL_PeekByteAtIndex(pPhysical, 4) << 8) | C1222PL_PeekByteAtIndex(pPhysical, 5));
            
            if ( length <= (MAX_C1222_PACKET_DATA - 8) )
            {
                if ( C1222PL_NumberOfBytesAvailable(pPhysical) >= (8+length) )
                    return TRUE;
                else
                    return FALSE;
            }
            else
                return TRUE;
        }
        else
        {
            if ( C1222PL_PeekByte(pPhysical) != C1222DL_STP )
                return TRUE;  // this allows the garbage to be cleared
        }
             
    }
    
    return FALSE;    
}



void HandleCRCDetectDecision(C1222DL* p)
{
    Unsigned8 total;
    
    if ( p->status.crcDetectOn )
    {        
        total    = p->counters.crcNormalCount + p->counters.crcReverseCount;
        
        if ( total > 31 )
        {
            if ( p->counters.crcNormalCount > 28 )
            {
                p->status.crcIsReversed = FALSE;
                p->status.crcDetectOn = FALSE;
            }
            else if ( p->counters.crcReverseCount > 28 )
            {
                p->status.crcIsReversed = TRUE;
                p->status.crcDetectOn = FALSE;
            }
            else
            {
                C1222DL_ReenableCRCByteOrderDetect(p);
            }
        }
    }    
}




// there are several possible ways we might get a packet
//   1) there might be a packet already received as a result of a prior sendpacket
//   2) there could be a packet available from the local port router
//   3) we could get a packet from the physical layer
//
// we might need to send an acknowledgement also

void C1222DL_Run(C1222DL* p)
{
    C1222PL* pPhysical = (C1222PL*)(p->pPhysical);
    Unsigned8 c;
    
    // see if we should be sending an ack to the peer process
    // we only delay positive acknowledgements, so this must be an ack not nak
    // we only delay acknowledgements returned inside packets, so transparency must be on
    
    p->counters.runCount++;
    
    if ( p->powerUpInProgress )
        return;
    
    if ( p->status.ackPending )
    {
        p->status.ackPending = FALSE;
        
        SendAcknowledge(p, TRUE, TRUE);   // last 2 params are ack not nak and want packet
    }
        
    // check for packet from local port router

#ifdef ENABLE_LOCAL_PORT_ROUTER
    // if packet available for this port, send it 
    
    if ( C1222LocalPortRouter_IsPacketAvailableForPort(p->pLocalPortRouter, p->myTargetPortID) )
    {
        memcpy(&(p->txPacket), 
               C1222LocalPortRouter_GetPacketForPort(p->pLocalPortRouter, p->myTargetPortID), 
               sizeof(C1222DLPacket));
               
        // free the buffer in the local port router
        
        C1222LocalPortRouter_AcknowledgePacketForPort(p->pLocalPortRouter, p->myTargetPortID);

        SendPacket(p);
        (void)WaitForAck(p);
            
        // if there is an error we should do something with it
    }
#endif    

        
    // check for the peer process across link (comm module or device) trying to send a packet
    
    if ( IsPossiblePacketAvailable(p) )
    {
        p->possiblePacketStart = 0;  // ready for next time
        
        c = C1222PL_GetByteNoLog(pPhysical);
        
        if ( c == C1222DL_STP )
        {
            if ( ReceivePacket(p) )
            {
                // send the packet on to the transport layer or to some other port
                
                C1222DL_LogEvent(p, C1222EVENT_DL_PROCESS_RECEIVED_PACKET);
                
                ProcessReceivedPacket(p);
            }
            // if there is an interchar timeout or crc error, we don't know if the sender wanted 
            // transparency or not, since any part of the message may be garbage
            else
            {
                SendAcknowledgeByte(p, FALSE);   // sends a nak
                C1222DL_LogEvent(p, C1222EVENT_DL_NAK_ICTO_OR_CRC);
            }
        }
        else
            C1222PL_HandleNonC1222Packet(p->pPhysical, c);
        
        // lets not try to log these bytes - it may cause me to time out        
    }
    else if ( C1222PL_IsByteAvailable(pPhysical) )
    {
        if ( C1222PL_PeekByte(pPhysical) != C1222DL_STP )
        {
            c = C1222PL_GetByteNoLog(pPhysical);
            C1222PL_HandleNonC1222Packet(pPhysical, c);
            p->possiblePacketStart = 0;  // ready for next time
        }
    }
    
    // comm module sends empty packets for line supervision
#ifdef C1222_INCLUDE_COMMMODULE    
    
    if ( C1222DL_IsCommModule(p) )
    {
        if ( C1222TL_DLIsKeepAliveNeeded((C1222TL*)(p->pTransport)) )
        {
            if ( (C1222Misc_GetFreeRunningTimeInMS()-p->status.lastAcknowledgedMessageTime) > C1222_DL_KEEPALIVE_MS )
            {
                SendEmptyPacket(p);
            }
        }
    }
#endif // C1222_INCLUDE_COMMMODULE
    
    // if detecting crc, make a decision if time
    
    HandleCRCDetectDecision(p);

    // check for traffic timeout
        
    if ( (C1222Misc_GetFreeRunningTimeInMS()-p->status.lastAcknowledgedMessageTime) > (Unsigned32)p->status.trafficTimeoutMs )
    {
        // the link is down - TL can fix it I hope
            
        p->status.lastAcknowledgedMessageTime = C1222Misc_GetFreeRunningTimeInMS();  // pretend we got an ack to avoid resetting too often
            
        C1222DL_LogEvent(p, C1222EVENT_DL_LINK_TRAFFIC_TIMEOUT);
        
        p->counters.trafficTimeoutLinkFailure++;
            
        ResetLink(p);
    }
}


#ifdef C1222_INCLUDE_COMMMODULE
static void SendEmptyPacket(C1222DL* p)
{
    (void)C1222DL_SendOneTryMessage(p, NULL, 0);
}
#endif // C1222_INCLUDE_COMMMODULE



// this returns the max packet payload size
static Unsigned16 MaxTxPacketPayloadLength(C1222DL* p)
{
    return (Unsigned16)(p->status.maxTxPacketSize - 8);
}








Boolean C1222DL_SendMessage(C1222DL* p, Unsigned8* message, Unsigned16 length)
{
    Boolean ok;
    
    ok = C1222DL_StartMessageBuild(p, length);
    
    if ( ok )
        ok = C1222DL_AddMessagePart(p, message, length);
        
    if ( ok )
        ok = C1222DL_SendBuiltMessage(p);
        
    return ok;
}




Boolean C1222DL_SendOneTryMessage(C1222DL* p, Unsigned8* message, Unsigned16 length)
{
    Boolean ok;
    
    p->status.oneTrySendInProgress = TRUE;
    
    ok = C1222DL_SendMessage(p, message, length);
    
    p->status.oneTrySendInProgress = FALSE;
    
    return ok;
}





Unsigned16 C1222DL_GetMaxNegotiatedTxPayload(C1222DL* p)
{
    return MaxTxPacketPayloadLength(p) * p->status.maxTxNumberOfPackets;
}



// the next 3 routines are used to send a message that is fed in parts to the datalink

Boolean C1222DL_StartMessageBuild(C1222DL* p, Unsigned16 totalLength)
{
    // this is used by the transport layer to send a message to the peer (cm or device)
    
    // if ack pending, set the acknowledge state in the first packet
    // if too long, break into packets and send the packets
    // handle toggle bit
    // handle sequence numbers
    
    Unsigned16 payloadLength;
    Unsigned8 sequence;
    Boolean ok = TRUE;
    
    payloadLength = MaxTxPacketPayloadLength(p);
    
    if ( totalLength <= payloadLength )
    {
        StartPacket(p, 0, FALSE, FALSE); // uses the working packet - sequence 0, not multiple, not first
    }
    else if ( totalLength > (p->status.maxTxNumberOfPackets*payloadLength) )
    {
        // this message is too long to send
        ok = FALSE;
        C1222DL_LogEvent(p, C1222EVENT_DL_MESSAGETOOLONGTOSEND);
        p->counters.messageTooLongToSendCount++;
    }
    else // need to split this message into more than one packet
    {
        sequence = (Unsigned8)((totalLength-1)/payloadLength);
        
        StartPacket(p, sequence, TRUE, TRUE);
    }
            
    return ok;
}



Boolean C1222DL_AddMessagePart(C1222DL* p, Unsigned8* message, Unsigned16 length)
{
    // this is used by the transport layer to send a message to the peer (cm or device)
    
    // if ack pending, set the acknowledge state in the first packet
    // if too long, break into packets and send the packets
    // handle toggle bit
    // handle sequence numbers
    
    Unsigned16 payloadLength;
    Unsigned16 addedLength;
    Unsigned16 workingLength;
    Unsigned8 sequence;
    Boolean ok = TRUE;
    
    payloadLength = MaxTxPacketPayloadLength(p);
    
    workingLength = (Unsigned16)(p->txPacket.length - 8 + length);
    
    sequence = C1222DLPacket_GetSequence(&(p->txPacket));
    
    // send any complete packets
        
    while( (workingLength > payloadLength) && ok )
    {
        addedLength = (Unsigned16)(payloadLength - (p->txPacket.length-8) );
        AddToPacket(p, message, addedLength);
        
        ok = C1222DL_SendBuiltMessage(p);
        
        message += addedLength;
        workingLength -= payloadLength; //addedLength;
        sequence--;

        StartPacket(p, sequence, TRUE, FALSE);
    }
    
    // add partial packet to the remainder packet    

    addedLength = (Unsigned16)(workingLength - (p->txPacket.length - 8) );
    AddToPacket(p, message, addedLength);
        
    return ok;
}



Boolean C1222DL_SendBuiltMessage(C1222DL* p)
{
    // there is part of the packet left to send
    FinishPacket(p);
    SendPacket(p);
    
    return WaitForAck(p);
}




Unsigned16 C1222DL_MaxSupportedPacketSize(C1222DL* p)
{
	(void)p;
    return MAX_C1222_PACKET_DATA; // entire packet size
}



Unsigned8 C1222DL_PacketOverheadSize(C1222DL* p)
{
    (void)p;
    return C1222_DL_PACKET_OVERHEAD;
}



Unsigned8 C1222DL_MaxSupportedNumberOfPackets(C1222DL* p)
{
	(void)p;
    return 255;
}


Unsigned32 g_myForceBaud = 0;

Boolean C1222DL_IsBaudRateSupported(C1222DL* p, Unsigned32 baud)
{
//#if 0
	(void)p;
	#ifdef FORCE_BAUD 
	return (Boolean)(baud==FORCE_BAUD);
	#else
	
	if ( g_myForceBaud != 0 )
	    return (Boolean)(baud==g_myForceBaud);
	else
        return (Boolean)( (baud == 9600) || (baud == 19200) || (baud==38400) );
    
    #endif
//#endif
//    return (Boolean)( (baud == 9600) );
}



void C1222DL_IncrementStatistic(C1222DL* p, Unsigned16 id)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    C1222TL_IncrementStatistic(pTransport, id);
}


void C1222DL_AddToStatistic(C1222DL* p, Unsigned16 id, Unsigned32 count)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    C1222TL_AddToStatistic(pTransport, id, count);
}




// *************************************************************************************************
// private routines for this module
// *************************************************************************************************



static void NoteAcknowledgedPacket(C1222DL* p)
{
    p->status.lastAcknowledgedMessageTime = C1222Misc_GetFreeRunningTimeInMS();
    
    if ( p->txPacket.crcIsReversed )
        p->counters.crcReverseCount++;
    else
        p->counters.crcNormalCount++;
}



static void ResetLink(C1222DL* p)
{
    p->status.rxToggleValid = FALSE;
    p->status.ackPending = FALSE;

    p->status.rxToggle = FALSE;
    
    // leave tx toggle bit alone on reset to get out of repeated failures with cell relay

    p->status.lastPacketCRC = 0;
    p->status.multiPacketOffset = 0;
    p->status.lastSequence = 0;
    
    C1222DL_ReenableCRCByteOrderDetect(p);
        
    // initialize to default params 
    //   this sets physical baudRate, maxPacketSize, maxNumberOfPackets, and maxNumberOfChannels
    //   it also resets the intercharacter timeout
    
    SetDefaultParameters(p); 
    
    C1222TL_DLNoteLinkFailure((C1222TL*)(p->pTransport));   
}




static void StartPacket(C1222DL* p, Unsigned8 sequence, Boolean multiple, Boolean first)
{
    // updates the working tx packet
    // local address and channel numbers are 0
    // adds crc and header
    // if ack pending, sets ack bits in header
    
    // caution - if length is 0, message pointer might be invalid
    
    Unsigned8* buffer;
    Unsigned8 byte;
    
    memset(&(p->txPacket), 0, sizeof(p->txPacket)); // start with fresh slate
    memset(&(p->txPacket.canary), 0xcd, sizeof(p->txPacket.canary));
    
    buffer = p->txPacket.buffer;
    
    buffer[0] = C1222DL_STP;
    buffer[1] = 0;     // both local address and channel number are always zero
    
    byte = 1;     // C12.22 packet format
    
    if ( multiple )
    {
        byte |= 0x80;
        if ( first )
            byte |= 0x40;
    }
    
    p->status.txToggle ^= 1;

    if ( p->status.txToggle )
        byte |= 0x20;

    if ( p->status.wantTransparency )
        byte |= 0x10;

    if ( p->status.ackPending )
    {
        byte |= 0x04;
        p->status.ackPending = FALSE;
    }
        
    buffer[2] = byte;
    buffer[3] = sequence;
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;
        
    p->txPacket.length = 8;
}




static void AddToPacket(C1222DL* p, Unsigned8* message, Unsigned16 length )
{
    // updates the working tx packet
    
    Unsigned8* buffer;
    Unsigned16 dataLength;
        
    buffer = p->txPacket.buffer;
    
    dataLength = (Unsigned16)(p->txPacket.length - 8);  // subtract header and trailer length
        
    if ( length > 0 )
    {
        memcpy( &buffer[6+dataLength], message, length);
    }
                
    p->txPacket.length += length;
}


static void FinishPacket(C1222DL* p)
{
    // updates the working tx packet
    // local address and channel numbers are 0
    // adds crc and header
    // if ack pending, sets ack bits in header
    
    // caution - if length is 0, message pointer might be invalid
    
    Unsigned8* buffer;
    Unsigned16 crc;
    Unsigned16 dataLength;
        
    buffer = p->txPacket.buffer;
    
    dataLength = (Unsigned16)(p->txPacket.length - 8);
    
    buffer[4] = (Unsigned8)(dataLength >> 8);
    buffer[5] = (Unsigned8)(dataLength & 0xFF);
                
    crc = C1222DL_CalcCRC(buffer,(Unsigned16)(dataLength+6));
    
    p->txPacket.crcIsReversed = FALSE;
    
    if ( p->status.crcIsReversed )
    {
        crc = (crc>>8) | (crc<<8);
        p->txPacket.crcIsReversed = TRUE;
    }
        
    buffer[6+dataLength] = (Unsigned8)(crc & 0xff); // crc is supposed to be transmitted least significant byte first
    buffer[7+dataLength] = (Unsigned8)(crc >> 8);        
}






static void MakeAckPacket(C1222DL* p, Boolean ackNotNak)
{
    // updates the working tx packet
    // local address and channel numbers are 0
    // adds crc and header
    // if ack pending, sets ack bits in header
    
    Unsigned8* buffer;
    Unsigned8 byte;

    StartPacket(p, 0, FALSE, FALSE);  // sequence 0, not multiple, not first
    
    // fix the acknowledge byte
    
    buffer = p->txPacket.buffer;

    byte = (Unsigned8)(buffer[2] & 0xF3);    
        
    if ( ackNotNak )
        byte |= 0x04;
    else
        byte |= 0x08;
        
    buffer[2] = byte;
    
    FinishPacket(p);
}



static void HandleAcknowledgementAccounting(C1222DL* p, Boolean wantAck)
{
    if ( wantAck )
    {
        NoteAcknowledgedPacket(p);
        p->counters.numberAcksSent++;
    }
    else
        p->counters.numberNaksSent++;
}


static void SendAcknowledgeByte(C1222DL* p, Boolean wantAck)
{
    Unsigned8 c;
    
    c = (Unsigned8)((wantAck) ? C1222DL_ACK : C1222DL_NAK);
    
    C1222PL_StartMessage( (C1222PL*)p->pPhysical );
    
    // since the cell relay sometimes sends packets without waiting for an ack, it works better to
    // not flush here - since flushing can trash the next packet.
    
    //C1222PL_PutBuffer((C1222PL*)(p->pPhysical), &c, 1);
    
    C1222PL_PutByte_Without_Flush((C1222PL*)(p->pPhysical), c);
    
    C1222PL_FinishMessage( (C1222PL*)p->pPhysical );
    
    HandleAcknowledgementAccounting(p, wantAck);
}


static void SendAcknowledgePacket(C1222DL* p, Boolean wantAck)
{
    MakeAckPacket(p, wantAck);
    SendPacket(p);
    
    HandleAcknowledgementAccounting(p, wantAck);
}


static void SendAcknowledge(C1222DL* p, Boolean wantAck, Boolean wantPacket)
{
    if ( wantPacket )
        SendAcknowledgePacket(p, wantAck);
    else
        SendAcknowledgeByte(p, wantAck);
}




static void NoteValidPacketForDuplicateHandling(C1222DL* p, C1222DLPacket* pPkt, Unsigned16 crc)
{
    p->status.rxToggleValid = TRUE;
    p->status.rxToggle = C1222DLPacket_ToggleState(pPkt);
    p->status.lastPacketCRC = crc;
}




#if 0
// this is called in 2 places -
//  1) before sending a packet when there is already a byte available
//  2) while waiting for an ack if a packet is received instead of an ack

static void ProcessReceivedPacketDuringTransmit(C1222DL* p)
{
    // the packet is in the rxPacket member
    // it is from the target port for this stack
    // it could be for this device (address 0) or for some other port (address <> 0)
    
    // mainly need to pass up to transport layer or forward to destination port
    // need to send ack to my target port in some cases
    
    // since we are in the process of sending a packet, we can't send acknowledge in a packet
    // so send acknowledge byte in this routine
    
    C1222DLPacket* pPkt;
    
    Unsigned8 destinationLocalAddressNumber;
    //Unsigned8* payload;
    Unsigned16 payloadLength;
    Unsigned16 crc;
    Unsigned16 offset;
    Boolean multiple;
    Boolean first;
    //Boolean wantTransparency;
    Unsigned8 sequence;
    
    // get some info from the packet

    pPkt = &p->rxPacket;
    
    destinationLocalAddressNumber = C1222DLPacket_GetLocalAddress(pPkt);
    //payload = C1222DLPacket_GetPacketData(pPkt);
    payloadLength = C1222DLPacket_GetPacketLength(pPkt);
    crc = C1222DLPacket_GetCRC(pPkt);
    
    multiple = C1222DLPacket_IsMultiPacket(pPkt);
    first = C1222DLPacket_IsFirstPacket(pPkt);
    //wantTransparency = C1222DLPacket_IsTransparencyEnabled(pPkt);
    sequence = C1222DLPacket_GetSequence(pPkt);
    
    // handle duplicates
    // we already know that the packet crc is ok
        
    if ( p->rxToggleValid )
    {
        if ( (C1222DLPacket_ToggleState(pPkt) == p->rxToggle) &&
             (C1222DLPacket_GetCRC(pPkt) == p->lastPacketCRC) )
        {
            C1222DL_LogEvent(p, C1222EVENT_DL_DUPLICATE_PACKET_IN_SEND);
            
            p->DuplicatePacketCount++;
            
            // this is a duplicate, ack it and discard
            SendAcknowledgeByte(p, TRUE);
            return;
        }
    }
    
    
    // lets check some other things about the packet
    // if multiple, and not first, sequence should be 1 less than last time

    if ( multiple && !first )
    {
        if ( sequence != (p->lastSequence - 1) )
        {
            C1222DL_LogEvent(p, C1222EVENT_DL_WRONG_SEQUENCE_IN_SEND);
            
            SendAcknowledgeByte(p, FALSE);
            return;
        }
    }
    
    // we only support channel 0
    // we only support C12.22 packet data format
    // the destination local address can't be mine because that is where it came from
    
    if ( (C1222DLPacket_GetChannelNumber(pPkt) != 0) ||
         (!C1222DLPacket_IsC1222Format(pPkt)) ||
         (destinationLocalAddressNumber==p->myTargetPortID)
       )
    {
        C1222DL_LogEvent(p, C1222EVENT_DL_INVALID_PACKET_IN_SEND);
        
        SendAcknowledgeByte(p, FALSE);
        return;
    }
    
    // if not multiple or first, and last sequence was not zero, we missed something, but
    // there seems no point in naking this packet, which seems to be ok    
    
    p->lastSequence = sequence;
    
    // figure offset for use in notifying transport layer
    
    if ( !multiple || first )
        offset = 0;
    else
        offset = p->multiPacketOffset;

    p->multiPacketOffset = (Unsigned16)(offset + payloadLength);
        
    // send the packet to its destination (transport layer or another local port)
    
    switch(destinationLocalAddressNumber)
    {
        case 0:
            // since we are trying to send a message here, lets try naking this packet
            // unless the payload length is 0 in which case we can ack it
            
            if ( payloadLength == 0 )
            {
                SendAcknowledgeByte(p, TRUE);
                
                NoteValidPacketForDuplicateHandling(p, pPkt, crc);
            }
            else
            {
                C1222DL_LogEvent(p, C1222EVENT_DL_NAK_COLLISION_IN_SEND);
                C1222DL_IncrementStatistic(p, C1222_STATISTIC_NUMBER_OF_COLLISIONS);
                SendAcknowledgeByte(p, FALSE);
            }
            break;
        
        case 1: // local port 0 = optical port
        #ifndef ENABLE_C1222_OPTICAL_PORT
            // if no c1222 optical port, handle as an undefined port
            SendAcknowledgeByte(p, TRUE);
            
            NoteValidPacketForDuplicateHandling(p, pPkt, crc);
            
            break;
        #endif   
        case 3: // default wan = comm module to rf lan or to ip wan module - should always be there
            // pass to the local port router
            #ifdef ENABLE_LOCAL_PORT_ROUTER
            if ( !C1222LocalPortRouter_IsDestinationPortBusy(p->pLocalPortRouter, destinationLocalAddressNumber) )
            {
                SendAcknowledgeByte(p, TRUE);
                
                NoteValidPacketForDuplicateHandling(p, pPkt, crc);                
                
                if ( payloadLength != 0 )
                {
                    // provide to local port router
                    C1222LocalPortRouter_SendPacket( p->pLocalPortRouter, &(p->rxPacket),  p->myTargetPortID); // last param is the source port
                }
            }
            #endif // ENABLE_LOCAL_PORT_ROUTER
            // if destination port is busy, just silently discard the packet
            break;
        default:
            // not a recognised port - ack and discard packet
            SendAcknowledgeByte(p, TRUE);
            
            NoteValidPacketForDuplicateHandling(p, pPkt, crc);
            
            break;
    }
}

#endif

// if receive a packet, and is multiple, get the entire message.
// transport layer should know that it is sending and if it gets a message
// complete notification, delay doing anything with that until done sending or the
// next run


static void HandleReceiveInProgress(C1222DL* p)
{
    Unsigned8 c;
    Unsigned32 start;
    
    C1222PL* pPhysical = (C1222PL*)(p->pPhysical);
    
    // check for the peer process across link (comm module or device) trying to send a packet
    
    if ( C1222PL_IsByteAvailable(pPhysical) )
    {
        c = C1222PL_GetByteNoLog(pPhysical);
        
        if ( c == C1222DL_STP )
        {
            for (;;)
            {
                if ( ReceivePacket(p) )
                {
                    // send the packet on to the transport layer or to some other port
                
                    C1222DL_LogEvent(p, C1222EVENT_DL_PROCESS_RECEIVED_PACKET_IN_SEND);
                
                    // process received packet will return true if the packet is for this stack
                    // and it is a multiple and not the last (otherwise we are done)
                    if ( ProcessReceivedPacket(p) )
                    {
                        start = C1222Misc_GetFreeRunningTimeInMS();
                        
                        for (;;)
                        {
                            c = C1222PL_GetByteNoLog(pPhysical);
                            
                            if ( C1222PL_DidLastActionTimeout(pPhysical) )
                            {
                                // might not want to wait the entire traffic timeout here -
                                // it may cause another stack to timeout
                                
                                if ( (C1222Misc_GetFreeRunningTimeInMS()-start) > C1222DL_TRAFFIC_TIMEOUT_MS )
                                    return;
                            }
                            else
                                if ( c == C1222DL_STP )
                                    break;
                        }
                    }
                    else
                        return;
                }
                // if there is an interchar timeout or crc error, we don't know if the sender wanted 
                // transparency or not, since any part of the message may be garbage
                else
                {
                    SendAcknowledgeByte(p, FALSE);   // sends a nak
                    C1222DL_LogEvent(p, C1222EVENT_DL_NAK_ICTO_OR_CRC_IN_SEND);
                    break; 
                }
            }
        }
        else
            C1222PL_HandleNonC1222Packet(pPhysical, c);
                
    }   
}



// void SendPacket(C1222DL* p)
//
// sends the tx packet, after handling transparency, to the physical layer of this stack

static void SendPacket(C1222DL* p)
{
    Unsigned16 length;
    Unsigned8* buffer;
    C1222PL* pPhysical;
    Unsigned16 ii;
    Unsigned8 c;
    static Unsigned8 transparentEscape[2] = { C1222DL_ESC, 0x3B };         // two byte sequence for natural escape in buffer
    static Unsigned8 transparentStartOfPacket[2] = { C1222DL_ESC, 0xCE };  // two byte sequence for natural stp in buffer

    // handle receive in progress
    
    HandleReceiveInProgress(p);

    // get a few items for easy reference later
        
    length = p->txPacket.length;
    buffer = p->txPacket.buffer;
    pPhysical = (C1222PL*)(p->pPhysical);
    
    // if no transparency, we can just send the bytes to the physical layer
    
    C1222PL_StartMessage( pPhysical );
    
    if ( !p->status.wantTransparency )
    {
        C1222PL_PutBuffer( pPhysical, buffer, length);
    }
    else
    {
        // there are no transparency issues in the first 3 bytes of the buffer
        // (guaranteed by the protocol)
        
        C1222PL_PutBuffer( pPhysical, buffer, 3);
        
        // handle transparency in the rest of the buffer
        //   if byte is 0xee or 0x1b, send the appropriate 2 byte sequence, else send the byte
        
        for ( ii=3; ii<length; ii++ )
        {
            c = buffer[ii];
            if ( c == C1222DL_STP )
                C1222PL_PutBuffer(pPhysical, transparentStartOfPacket, 2);
            else if ( c == C1222DL_ESC )
                C1222PL_PutBuffer(pPhysical, transparentEscape, 2);
            else
                C1222PL_PutBuffer(pPhysical, &c, 1);
        }
    }
    
    C1222PL_FinishMessage( pPhysical );
    
    p->counters.numberPacketsSent++;
}




Boolean CheckPacketCRC(C1222DL* p, Unsigned8* buffer, Unsigned16 length)
{
    Unsigned16 crc = C1222DL_CalcCRC( buffer, length );
    
    // if detect is enabled, check it both ways but give preference to the last way it worked
    
    Unsigned16 packetCRC = buffer[length] | buffer[length+1]<<8;
    
    if ( p->status.crcIsReversed )
        packetCRC = (packetCRC >> 8) | (packetCRC << 8);
        
    if ( crc == packetCRC )
    {
        p->rxPacket.crcIsReversed = p->status.crcIsReversed;
        return TRUE;
    }
    
    if ( p->status.crcDetectOn )
    {
        packetCRC = (packetCRC >> 8) | (packetCRC << 8);
        if ( crc == packetCRC )
        {
            p->status.crcIsReversed ^= 1; // toggle it
            p->rxPacket.crcIsReversed = p->status.crcIsReversed;
            return TRUE;
        }
    }
    
    return FALSE;    
}




static Boolean ReceivePacket(C1222DL* p)
{
    // already have the 0xEE char received
    // put the packet in the rx packet
    // return true if the packet is valid
    // does not ack the packet
    
    Unsigned8* buffer;
    Unsigned16 length = 0;
    Unsigned16 ii;
    Unsigned8 transparent;
    Unsigned8 c;
    
    C1222PL* pPhysical;
    
    pPhysical = p->pPhysical;
    
    buffer = p->rxPacket.buffer;
    
    buffer[0] = C1222DL_STP;
    
    if ( C1222PL_GetBufferNoLog(pPhysical, &buffer[1], 2 ) < 2 )
    {
        p->counters.packetTimeoutCount++;
        return FALSE;
    }

    transparent = (Unsigned8)(buffer[2] & 0x10);
    
    for ( ii=3; ii<(8+length); ii++ )
    {
        c = C1222PL_GetByteNoLog(pPhysical);
    
        if ( C1222PL_DidLastActionTimeout(pPhysical) )
        {
            C1222PL_LogHexReceive(pPhysical, buffer, ii);
            p->counters.packetTimeoutCount++;
            return FALSE;
        }
            
        if ( transparent && (c == C1222DL_ESC) )
        {
            c = (Unsigned8)(C1222PL_GetByteNoLog(pPhysical) ^ 0x20); // complement bit 5
            
            if ( C1222PL_DidLastActionTimeout(pPhysical) )
            {
                C1222PL_LogHexReceive(pPhysical, buffer, ii);
                p->counters.packetTimeoutCount++;
                return FALSE;              
            }
        }

        buffer[ii] = c;
        
        if ( ii == 5 )
        {
            length = (Unsigned16)((buffer[4] << 8) | (buffer[5] & 0xFF));
            
            if ( length > (MAX_C1222_PACKET_DATA - 8) )
            {
                C1222PL_LogHexReceive(pPhysical, buffer, ii);
                p->counters.packetTooLongCount++;
                return FALSE;
            }
        }
    }
    
    C1222PL_LogHexReceive(pPhysical, buffer, ii);
            
    if ( CheckPacketCRC(p, buffer, length+6 ) )
    {
        p->counters.numberPacketsReceived++;
        
        if ( p->rxPacket.crcIsReversed )
            p->counters.crcReverseCount++;
        else
            p->counters.crcNormalCount++;
            
        return TRUE;            
    }
    
    p->counters.packetCRCErrorCount++;
    C1222DL_IncrementStatistic(p, C1222_STATISTIC_NUMBER_OF_CHECKSUM_ERRORS);
    
    return FALSE;            
}




static Boolean ProcessReceivedPacket(C1222DL* p)
{
    // the packet is in the rxPacket member
    // it is from the target port for this stack
    // it could be for this device (address 0) or for some other port (address <> 0)
    
    // mainly need to pass up to transport layer or forward to destination port
    // need to send ack to my target port in some cases
    
    C1222DLPacket* pPkt;
    
    Unsigned8 destinationLocalAddressNumber;
    Unsigned8* payload;
    Unsigned16 payloadLength;
    Unsigned16 crc;
    Unsigned16 offset;
    Boolean multiple;
    Boolean first;
    Boolean wantTransparency;
    Unsigned8 sequence;

    Boolean morePacketsExpected = FALSE;
    
    // get some info from the packet

    pPkt = &p->rxPacket;
    
    destinationLocalAddressNumber = C1222DLPacket_GetLocalAddress(pPkt);
    payload = C1222DLPacket_GetPacketData(pPkt);
    payloadLength = C1222DLPacket_GetPacketLength(pPkt);
    crc = C1222DLPacket_GetCRC(pPkt);
    
    multiple = C1222DLPacket_IsMultiPacket(pPkt);
    first = C1222DLPacket_IsFirstPacket(pPkt);
    wantTransparency = C1222DLPacket_IsTransparencyEnabled(pPkt);
    sequence = C1222DLPacket_GetSequence(pPkt);
    
    // handle duplicates
    // we already know that the packet crc is ok
        
    if ( p->status.rxToggleValid )
    {
        if ( (C1222DLPacket_ToggleState(pPkt) == p->status.rxToggle) &&
             (C1222DLPacket_GetCRC(pPkt) == p->status.lastPacketCRC) )
        {
            C1222DL_LogEvent(p, C1222EVENT_DL_DUPLICATE_PACKET);
            
            p->counters.DuplicatePacketCount++;
            
            // this is a duplicate, ack it and discard
            SendAcknowledge(p, TRUE, wantTransparency);
            return FALSE;
        }
    }
    
    
    // lets check some other things about the packet
    // if multiple, and not first, sequence should be 1 less than last time

    if ( multiple && !first )
    {
        if ( sequence != (p->status.lastSequence - 1) )
        {
            C1222DL_LogEvent(p, C1222EVENT_DL_WRONG_SEQUENCE);
            
            SendAcknowledge(p, FALSE, wantTransparency);
            p->counters.packetInvalidCount++;
            return FALSE;
        }
    }
    
    // we only support channel 0
    // we only support C12.22 packet data format
    // the destination local address can't be mine because that is where it came from
    
    if ( (C1222DLPacket_GetChannelNumber(pPkt) != 0) ||
         (!C1222DLPacket_IsC1222Format(pPkt)) ||
         (destinationLocalAddressNumber==p->status.myTargetPortID)
       )
    {
        C1222DL_LogEvent(p, C1222EVENT_DL_INVALID_PACKET);
        
        SendAcknowledge(p, FALSE, wantTransparency);
        p->counters.packetInvalidCount++;
        return FALSE;
    }
    
    // if not multiple or first, and last sequence was not zero, we missed something, but
    // there seems no point in naking this packet, which seems to be ok    
    
    p->status.lastSequence = sequence;
    
    // figure offset for use in notifying transport layer
    
    if ( !multiple || first )
        offset = 0;
    else
        offset = p->status.multiPacketOffset;

    p->status.multiPacketOffset = (Unsigned16)(offset + payloadLength);
        
    // send the packet to its destination (transport layer or another local port)
    
    switch(destinationLocalAddressNumber)
    {
        case 0:
            if ( p->status.wantTransparency )
                p->status.ackPending = TRUE;
            else
                SendAcknowledgeByte(p, TRUE);
                
            NoteValidPacketForDuplicateHandling(p, pPkt,crc);                
                
            if ( payloadLength == 0 )
            {
                C1222TL_DLNoteKeepAliveReceived((C1222TL*)p->pTransport);
                p->counters.KeepAlivePacketCount++;
                return FALSE;  // consume empty packets sent for link supervision
            }
            
            // the packet is intended for this device, pass to transport layer
            // the transport layer should assume that if the offset is 0, we are starting a new
            // message and discard any previous packets sent
            
            C1222DL_LogEvent(p, C1222EVENT_DL_PACKET_SENT_TO_TL);
            
            C1222TL_DLNotePacketReceived((C1222TL*)p->pTransport, payload, payloadLength, offset);
            if ( !multiple || (sequence==0) )
            {
                C1222DL_LogEvent(p, C1222EVENT_DL_MESSAGE_COMPLETE);
                
                C1222TL_DLNoteMessageRxComplete((C1222TL*)(p->pTransport));
            }
            else
                morePacketsExpected = TRUE;
                
            break;
        
        case 1: // local port 0 = optical port
        #ifndef ENABLE_C1222_OPTICAL_PORT
            // if no c1222 optical port, handle as an undefined port
            SendAcknowledge(p, TRUE, wantTransparency);
            
            NoteValidPacketForDuplicateHandling(p, pPkt,crc);
            
            break;        
        #endif   
        case 3: // default wan = comm module to rf lan or to ip wan module - should always be there
            // pass to the local port router
            #ifdef ENABLE_LOCAL_PORT_ROUTER
            if ( !C1222LocalPortRouter_IsDestinationPortBusy(p->pLocalPortRouter, destinationLocalAddressNumber) )
            {
                SendAcknowledge(p, TRUE, wantTransparency);
                
                NoteValidPacketForDuplicateHandling(p, pPkt, crc);                
                
                if ( payloadLength != 0 )
                {
                    // provide to local port router
                    C1222LocalPortRouter_SendPacket( p->pLocalPortRouter, &(p->rxPacket),  p->status.myTargetPortID); // last param is the source port
                }
            }
            #endif // ENABLE_LOCAL_PORT_ROUTER
            // if destination port is busy, just silently discard the packet
            break;

        default:
            // not a recognised port - ack and discard packet
            SendAcknowledge(p, TRUE, wantTransparency);
            
            NoteValidPacketForDuplicateHandling(p, pPkt, crc);            
            break;
    }

    return morePacketsExpected;
}



static void NoteWaitForAckComplete(C1222DL* p)
{
    Unsigned32 elapsedTime = C1222Misc_GetFreeRunningTimeInMS() - p->status.lastWaitForAckStart;
    
    p->status.totalWaitForAckTime += elapsedTime;
    
    p->counters.numberAcksReceived++;
    
    if ( p->counters.numberAcksReceived > 0 )
    {
        p->status.avgWaitForAckTime = (Unsigned16)(p->status.totalWaitForAckTime/p->counters.numberAcksReceived);
    }
    else
        p->status.avgWaitForAckTime = 0;
           
    if ( elapsedTime > (Unsigned32)(p->status.maxWaitForAckTime) )
        p->status.maxWaitForAckTime = (Unsigned16)elapsedTime;
                
}



static Boolean WaitForAck(C1222DL* p)
{
    // this needs to handle receiving a packet or an ack byte, especially on last packet
    
    // need to handle either single byte ack/nak or packet with included acknowledge
    // the problem is that if the target got a bad crc or had interchar timeout, it does not really
    // know anything about the packet, such as whether transparency was selected, so it will reply
    // with one of them, but not necessarily the one we requested.
    
    Unsigned32 startTick;
    Unsigned8 c;
    C1222PL* pPhysical;
    Unsigned8 retry = 0;
    C1222DLAck acknowledgement;
    Boolean keepGoing = TRUE;
    Unsigned32 garbageCount;
    
    pPhysical = (C1222PL*)p->pPhysical;
    
    p->status.lastWaitForAckStart = C1222Misc_GetFreeRunningTimeInMS();
    
    
    while(keepGoing)  // this is the retry loop
    {
        // we will wait the response timeout for an acknowledgement char
        
        startTick = C1222Misc_GetFreeRunningTimeInMS();
        
        while(keepGoing)  // this is the loop to continue waiting for an ack
        {
            if ( p->powerUpInProgress )
                return FALSE;
            
            c = C1222PL_GetByteNoLog(pPhysical);  // this will wait an intercharacter timeout for the byte
            
            if ( C1222PL_DidLastActionTimeout(pPhysical) )
            {
                if ( (C1222Misc_GetFreeRunningTimeInMS() - startTick) > (Unsigned32)p->status.responseTimeoutMs )
                {
                    // timeout - try again
                    p->counters.WaitForAckTimeoutCount++;
                    p->status.WaitForAckReason = 1;
                    C1222DL_LogEvent(p, C1222EVENT_DL_ACK_TIMEOUT);
                    break; // from the while loop
                }
                
                // else keep waiting
            }
            else // we got something
            {  
                // could be ack, nak, or a packet, or garbage
                
                // if the packet was successfully received, they should have the transparency request, so
                // should follow what we want.
                          
                if ( (c == C1222DL_ACK) && !p->status.wantTransparency )
                {
                    NoteAcknowledgedPacket(p);
                    
                    NoteWaitForAckComplete(p);
                    C1222DL_LogEvent(p, C1222EVENT_DL_ACK_RECEIVED);
                    return TRUE;
                }
                
                // here the receiver might not know if we wanted transparency or not
                
                else if ( c == C1222DL_NAK )
                {
                    // this might be too sensitive to failure
                    if ( p->status.crcDetectOn )
                    {
                        p->status.crcIsReversed ^= 1;
                    }
                                               
                    p->counters.WaitForAckNakCount++;
                    p->status.WaitForAckReason = 2;
                    C1222DL_LogEvent(p, C1222EVENT_DL_NAK_RECEIVED);
                    break; // from the while - go try sending the packet again
                }
                
                else if ( c == C1222DL_STP )
                {
                    // if there was a packet already received, we will trash it here, since I only
                    // have room for one received packet.  But the sender is obviously no longer waiting
                    // for a response to it, so just ignore it.
                    
                    // get the rest of the packet and check for acknowledgement inside the packet
                       
                    if ( ReceivePacket(p) )
                    {                            
                        acknowledgement = C1222DLPacket_GetAcknowledgement(&(p->rxPacket));
                        
                        ProcessReceivedPacket(p);
                        
                        if ( acknowledgement == C2ACK_Ack )
                        {
                            NoteAcknowledgedPacket(p);
                           
                            NoteWaitForAckComplete(p);
                            C1222DL_LogEvent(p, C1222EVENT_DL_ACK_RECEIVED);
                            return TRUE;
                        }
                        else if ( acknowledgement == C2ACK_Nak )
                        {
                            p->counters.WaitForAckNakCount++;
                            p->status.WaitForAckReason = 6;
                            C1222DL_LogEvent(p, C1222EVENT_DL_NAK_RECEIVED);
                        }
                        else
                        {
                            // otherwise we will break from the while and retry 
                                                
                            p->counters.WaitForAckNotAckCount++; 
                            p->status.WaitForAckReason = 5;
                            C1222DL_LogEvent(p, C1222EVENT_DL_NONACK_PACKET_RECEIVED);
                        }
                    }
                    else
                    {
                        C1222DL_LogEvent(p, C1222EVENT_DL_NAK_ICTO);
                        SendAcknowledgeByte(p, FALSE);
                        p->counters.WaitForAckICTOCount++;
                        p->status.WaitForAckReason = 3;
                    }
                } // end if 0xFF
                else if ( C1222PL_HandleNonC1222Packet(pPhysical, c) )
                {
                    C1222DL_LogEvent(p, C1222EVENT_DL_NONC1222PACKET_RECEIVED);
                    // log something
                }
                else // must be garbage char
                {
                    // gobble garbage until interchar timeout
                    
                    garbageCount = 0;
                    
                    while ( ! C1222PL_DidLastActionTimeout(pPhysical) )
                    {
                        garbageCount++;
                        
                        c = C1222PL_GetByteNoLog(pPhysical);
                        
                        (void)c;
                        
                        // if the rf lan is in the loader it will keep sending naks at 38400
                        // forever - we need to stop waiting for an ack sometime
                        
                        if ( (C1222Misc_GetFreeRunningTimeInMS() - startTick) > (Unsigned32)p->status.responseTimeoutMs )
                        {
                            // timeout - try again
                            p->counters.WaitForAckTimeoutCount++;
                            C1222DL_LogEvent(p, C1222EVENT_DL_ACK_TIMEOUT);
                            break; // from the while loop
                        }                        
                    }
                    
                    p->counters.WaitForAckGarbageCount++;
                    p->status.WaitForAckReason = 4;
                    
                    C1222DL_LogEventAndBytes(p, C1222EVENT_DL_GARBAGE_RECEIVED, sizeof(garbageCount), (Unsigned8*)(&garbageCount));
                }
                
                break; // from the while to retry
            } // end if got something
        
        } // end while waiting for ack
        
        if ( p->status.oneTrySendInProgress )
            return FALSE;
        
        retry++;
        if ( retry > p->status.retries )
            keepGoing = FALSE;
            
        // resend the same packet
        else
            SendPacket(p);
        
    } // end of retry loop
    
    // notify transport of link failure
    
    C1222DL_LogEvent(p, C1222EVENT_DL_WAIT_FOR_ACK_LINK_FAILURE);
    
    p->counters.waitForAckLinkFailure++;
    
    ResetLink(p);
    
    return FALSE;
}




void C1222DL_ClearCounters(C1222DL* p)
{
    memset(&p->counters, 0, sizeof(p->counters));
}


void C1222DL_ClearStatusDiagnostics(C1222DL* p)
{
    p->status.maxWaitForAckTime = 0;
    p->status.avgWaitForAckTime = 0;
    p->status.totalWaitForAckTime = 0;    
}













