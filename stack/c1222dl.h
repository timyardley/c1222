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


#ifndef C1222DATALINK_H
#define C1222DATALINK_H

#include "c1222.h"
#include "c1222dl_crc.h"
#include "c1222event.h"

#define C1222_NUM_LOCAL_PORTS 2   // optical port and comm module interface - if later zigbee, add 1
#define MAX_C1222_PACKET_DATA 87 //166 // entire packet size (to carry 150 byte acse message)

#define C1222_DL_PACKET_OVERHEAD  8  // <stp> <identity> <ctrl> <seq-nbr> <length> ... <crc>

// special c12.22 characters
#define C1222DL_ACK    0x06
#define C1222DL_NAK    0x15
#define C1222DL_STP    0xEE
#define C1222DL_ESC    0x1B

// fixed communication parameters
#define C1222DL_RESPONSE_TIMEOUT_MS        2000
#define C1222DL_TRAFFIC_TIMEOUT_MS        6000
#define C1222DL_INTERCHARACTER_TIMEOUT_MS   500
#define C1222DL_RETRIES                      3

// keepalive every 2.5 seconds to allow two tries before a traffic timeout 
#define C1222_DL_KEEPALIVE_MS             2500



typedef enum C1222DLAckTag { C2ACK_None, C2ACK_Ack, C2ACK_Nak, C2ACK_UnAck } C1222DLAck;




typedef struct C1222DLPacketTag
{
    Unsigned16 length;
	Unsigned8 buffer[MAX_C1222_PACKET_DATA];
	Unsigned8 canary[10];
	Boolean   crcIsReversed;  // false means per standard, true means like in ami centron for SR 1.0
} C1222DLPacket;

Unsigned8 C1222DLPacket_GetLocalAddress(C1222DLPacket* p);
Unsigned8 C1222DLPacket_GetChannelNumber(C1222DLPacket* p);
Unsigned8* C1222DLPacket_GetPacketData(C1222DLPacket* p);
Unsigned16 C1222DLPacket_GetPacketLength(C1222DLPacket* p);
Unsigned16 C1222DLPacket_GetCRC(C1222DLPacket* p);
Boolean C1222DLPacket_CheckCRC(C1222DLPacket* p);
Unsigned8 C1222DLPacket_GetSequence(C1222DLPacket* p);
Boolean C1222DLPacket_IsMultiPacket(C1222DLPacket* p);
Boolean C1222DLPacket_IsFirstPacket(C1222DLPacket* p);
Boolean C1222DLPacket_ToggleState(C1222DLPacket* p);
Boolean C1222DLPacket_IsTransparencyEnabled(C1222DLPacket* p);
C1222DLAck C1222DLPacket_GetAcknowledgement(C1222DLPacket* p);
Boolean C1222DLPacket_IsC1222Format(C1222DLPacket* p);




// the dl packet includes the ident field that indicates the source or target of the 
// packet.  
typedef struct C1222LocalPortRouterTag
{
    // position in array indicates destination of the packet
    // ident field in array indicates source of packet, and crc is fixed
    C1222DLPacket    XpacketsForPort[C1222_NUM_LOCAL_PORTS];
    Boolean          XisBusy[C1222_NUM_LOCAL_PORTS];
    
} C1222LocalPortRouter;

// in the send packet routine, the packet local address defines the destination
// it changes the port number to the source and stores it in the array position for the 
// destination (and recalculates the crc)
void C1222LocalPortRouter_SendPacket(C1222LocalPortRouter* pLPR, C1222DLPacket* pPacket, Unsigned8 sourcePort);
Boolean C1222LocalPortRouter_IsPacketAvailableForPort(C1222LocalPortRouter* pLPR, Unsigned8 destPort);
C1222DLPacket* C1222LocalPortRouter_GetPacketForPort(C1222LocalPortRouter* pLPR, Unsigned8 destPort);
void C1222LocalPortRouter_AcknowledgePacketForPort(C1222LocalPortRouter* pLPR, Unsigned8 destPort);
Boolean C1222LocalPortRouter_IsDestinationPortBusy(C1222LocalPortRouter* pLPR, Unsigned8 destPort);
void C1222LocalPortRouter_Init(C1222LocalPortRouter* pLPR);
void C1222LocalPortRouter_NotePowerUp(C1222LocalPortRouter* pLPR, Unsigned32 outageDurationInSeconds);


typedef struct C1222DLCountersTag
{
    Unsigned32 numberPacketsReceived;
    Unsigned32 numberPacketsSent;
    
    Unsigned32 WaitForAckNakCount;
    Unsigned32 WaitForAckTimeoutCount;
    Unsigned32 WaitForAckGarbageCount;
    Unsigned32 WaitForAckNotAckCount;
    Unsigned32 WaitForAckICTOCount;
    Unsigned32 DuplicatePacketCount;
    Unsigned32 KeepAlivePacketCount;
    
    Unsigned32 numberAcksReceived;
    Unsigned32 numberAcksSent;
    
    Unsigned32 numberNaksSent;
    
    Unsigned16 runCount; 
    Unsigned16 messageTooLongToSendCount;
    
    Unsigned16 packetCRCErrorCount;
    Unsigned16 packetTimeoutCount;
    Unsigned16 packetTooLongCount;
    Unsigned16 packetInvalidCount; 
    
    Unsigned16 waitForAckLinkFailure;
    Unsigned16 trafficTimeoutLinkFailure;
    
    Unsigned8 crcNormalCount;
    Unsigned8 crcReverseCount;
    
} C1222DLCounters;



typedef struct C1222DLStatusTag
{
    Unsigned8 myTargetPortID;
    
    Boolean isCommModule;
    
    Boolean    rxToggle;
    Boolean    txToggle;
    Boolean    rxToggleValid;
    
    Unsigned16 lastPacketCRC;
    
    Unsigned16 multiPacketOffset;
    Unsigned8 lastSequence;
    
    Boolean    ackPending; // this can only be true if the received packet selected transparency, and not nak
    
    Unsigned16 maxTxPacketSize;
    Unsigned8 maxTxNumberOfPackets;
    Unsigned8 maxNumberOfChannels;
    Boolean   wantTransparency;
    
    Unsigned32 lastAcknowledgedMessageTime;  // either received or sent

    Unsigned8 WaitForAckReason;  
        
    Boolean oneTrySendInProgress; 
        
    Unsigned16 responseTimeoutMs;
    Unsigned16 trafficTimeoutMs;
    Unsigned16 intercharacterTimeoutMs;
    Unsigned8 retries;
    
    Unsigned32 lastWaitForAckStart;
    Unsigned16 maxWaitForAckTime;
    Unsigned16 avgWaitForAckTime;
    Unsigned32 totalWaitForAckTime;

    Boolean crcDetectAllowed;
    Boolean crcIsReversed;
    Boolean crcDetectOn;
    
} C1222DLStatus;



typedef struct C1222DLTag
{
    C1222DLStatus status;
    
    void* pTransport;
    void* pPhysical;
        
    C1222DLPacket rxPacket;
        
    C1222DLPacket txPacket;
    
    C1222LocalPortRouter* pLocalPortRouter;
        
    Boolean logWanted;
        
    C1222DLCounters counters;
    
    Unsigned32 possiblePacketStart;
    Boolean powerUpInProgress;
    
} C1222DL;


// services for transport layer

void C1222DL_Init(C1222DL* p, Boolean wantTransparency, Unsigned8 myTargetPortID, Boolean isCommModule);
void C1222DL_SetLinks(C1222DL* p, void* physical, void* transport, void* localPortRouter);
void C1222DL_Run(C1222DL* p);
Boolean C1222DL_SendMessage(C1222DL* p, Unsigned8* message, Unsigned16 length);
Boolean C1222DL_StartMessageBuild(C1222DL* p, Unsigned16 totalLength);
Boolean C1222DL_AddMessagePart(C1222DL* p, Unsigned8* message, Unsigned16 length);
Boolean C1222DL_SendBuiltMessage(C1222DL* p);
void C1222DL_SetParameters(C1222DL* p, Unsigned32 baud, Unsigned16 packetSize, Unsigned8 numberOfPackets);
Boolean C1222DL_IsCommModule(C1222DL* p);
Boolean C1222DL_SendOneTryMessage(C1222DL* p, Unsigned8* message, Unsigned16 length);


Boolean C1222DL_IsInterfaceAvailable(C1222DL* p);

Unsigned16 C1222DL_MaxSupportedPacketSize(C1222DL* p);
Unsigned8 C1222DL_MaxSupportedNumberOfPackets(C1222DL* p);
Unsigned8 C1222DL_PacketOverheadSize(C1222DL* p);
Boolean C1222DL_IsBaudRateSupported(C1222DL* p, Unsigned32 baud);

void C1222DL_NotePowerUp(C1222DL* p, Unsigned32 outageDurationInSeconds);

void C1222DL_IncrementStatistic(C1222DL* p, Unsigned16 id);
void C1222DL_AddToStatistic(C1222DL* p, Unsigned16 id, Unsigned32 count);



void C1222DL_LogEvent(C1222DL* p, C1222EventId event);
void C1222DL_LogEventAndBytes(C1222DL* p, C1222EventId event, Unsigned8 n, Unsigned8* bytes);
void C1222DL_LogEventAndULongs(C1222DL* p, C1222EventId event, 
        Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4);
        
void C1222DL_InitAtEndOfStackInit(C1222DL* p); 

void C1222DL_ClearCounters(C1222DL* p);
void C1222DL_ClearStatusDiagnostics(C1222DL* p);  

Unsigned32 C1222DL_GetCurrentBaud(C1222DL* p);
void C1222DL_ReconfigurePort(C1222DL* p); 

void C1222DL_ReInitAfterRFLANFwDownload(C1222DL* p, Unsigned8 portID);

void C1222DL_PowerUpInProgress(C1222DL* p);                

void C1222DL_ReenableCRCByteOrderDetect(C1222DL* p); 
void C1222DL_SetCRCByteOrderMode(C1222DL* p, Boolean detectOk, Boolean isReversed);
Unsigned16 C1222DL_GetMaxNegotiatedTxPayload(C1222DL* p);
               
       


#endif

