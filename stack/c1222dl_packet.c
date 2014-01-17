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




//  C12.22 Datalink packet support


// From ansic1222-wg1final.doc modified 3/9/2006 version:

//
// <packet>	::= <stp> <identity> <ctrl> <seq-nbr> <length> <data> <crc>
// 
// <stp>	::=	EEH	{Start of packet character.}
// 
// <identity>	::=	<byte>	{Identity.
// Bits 3 to 7: LOCAL_ADDRESS_NUMBER
//			This field is used to facilitate routing of information between a Local Port and the local C12.22 Communication Modules. This field can be set to the following values:
//			0 = C12.22 Device
//			1 = Local Port 0 (Default)
//			2 = Local Port 1 (Alternate)
//			3 = Interface 0 (Default WAN)
//			4 = Interface 1 (Alternate WAN)
//			5 = Interface 2(Default POT modem)
//			6 = Interface 3 (Alternate POT modem)
//			7 to 12 = Reserved
//			13 to 28 = Manufacturer defined
//			29 = Invalid (to avoid <stp>)
//			30 to 31 = Reserved for future extension of this field
//
//			Bits 0 to 2: CHANNEL NUMBER
//			This field allows simultaneous transfer of multiple segmented <acse-pdu>. A different number is assigned dynamically by the C12.22 Device or by the C12.22 Communication module for each <acse-pdu> message transferred simultaneously.}
//
//<ctrl>	::=	<byte>	{Control field.
//			Bit 7: MULTI_PACKET_TRANSMISSION
//			If true (1H) then this packet is part of a multiple packet transmission.
//
//			Bit 6: FIRST_PACKET
//			If true (1H), then this packet is the first packet of a multiple packet transmission (MULTI_PACKET_TRANSMISSION, bit-7, is also set to 1), otherwise this bit shall be set to 0.
//
//			Bit 5: TOGGLE_BIT
//			Represents a toggle bit to reject duplicate packets. This bit is toggled for each new packet sent. Retransmitted packets keep the same state as the original packet sent. After a Traffic Time-out, the state of the toggle bit may be re-initialized; thus, it is assumed to be indeterminate.
//
//			Bit 4: TRANSPARENCY
//			If true, this packet uses the transparency mechanism as described in section “7.9.9. Transparency”. Also the target shall respond with an ACK or optional NAK inside the packet structure as described in section “7.9.4. Acknowledgment”.
//
//			Bits 2 to 3: ACKNOWLEGMENT
//			Used to acknowledge the reception of a valid or invalid packet. Acknowledgment and response data can be sent together in the same packet or as two consecutive packets.
//			0 = No acknowledgment included of the previous packet.
//			1 = Positive acknowledgment of the previous packet.
//			2 = Negative acknowledgment of the previous packet.
//			3 = Unacknowledged packet sent.
//
//			A possible use of option 3 is when a one-way device send blurts which do not require acknowledgments. In this context, the C12.22 Device shall be the originator of all messages. This is an indication to the C12.22 Communication Module to disable the ACK/NAK packet handshake for one-way devices.
//
//			Bit 0 to 1: DATA_FORMAT
//			0 = C12.18 or C12.21 Device
//			1 = C12.22 Device
//			2 = Invalid (to avoid <stp>)
//			3 = Reserved}
//
//<seq-nbr>	::=	<byte>	{Number that is decremented by one for each new packet sent. The first packet in a multiple packet transmission shall have a <seq-nbr> equal to the total number of packets minus one. A value of zero in this field indicates that this packet is the last packet of a multiple packet transmission. If only one packet is in a transmission, this field shall be set to zero.}
//
//<length>	::=	<word16>	{Number of bytes of <data> in packet.}
//
//<data>	::=	<byte>*	{<length> number of bytes of actual packet data. This value is limited by the maximum packet size minus the packet overhead. This value can never exceed 8183.}
//
//<crc>	::=	<word16>	{HDLC implementation of the polynomial X16 + X12 + X5 + 1}
//

#include "c1222environment.h"
#include "c1222dl.h"



#define packet_identity(p)        ((p)->buffer[1])
#define packet_ctrl(p)            ((p)->buffer[2])


Unsigned8 C1222DLPacket_GetLocalAddress(C1222DLPacket* p)
{
    return (Unsigned8)(packet_identity(p) >> 3);
}


Unsigned8 C1222DLPacket_GetChannelNumber(C1222DLPacket* p)
{
    return (Unsigned8)(packet_identity(p) & 0x07);
}



Unsigned8* C1222DLPacket_GetPacketData(C1222DLPacket* p)
{
    return &(p->buffer[6]);
}



Unsigned16 C1222DLPacket_GetPacketLength(C1222DLPacket* p)
{
    return (Unsigned16)((p->buffer[4]<<8) | (p->buffer[5]));
}



Unsigned16 C1222DLPacket_GetCRC(C1222DLPacket* p)
{
    Unsigned16 pos = (Unsigned16)(6 + C1222DLPacket_GetPacketLength(p));
    
    if ( p->crcIsReversed )
        return (Unsigned16)((p->buffer[pos+1]) | ((p->buffer[pos])<<8));
    else
        return (Unsigned16)((p->buffer[pos]) | ((p->buffer[pos+1])<<8));    
}


#if 0 // I don't think this routine is used
Boolean C1222DLPacket_CheckCRC(C1222DLPacket* p)
{
    return (Boolean)(C1222DLPacket_GetCRC(p) == C1222DL_CalcCRC(p->buffer, (Unsigned16)(p->length-2)));
}
#endif



Unsigned8 C1222DLPacket_GetSequence(C1222DLPacket* p)
{
    return p->buffer[3];
}



Boolean C1222DLPacket_IsMultiPacket(C1222DLPacket* p)
{
    return (Boolean)((packet_ctrl(p) & 0x80) ? TRUE : FALSE);
}



Boolean C1222DLPacket_IsFirstPacket(C1222DLPacket* p)
{
    return (Boolean)((packet_ctrl(p) & 0x40) ? TRUE : FALSE);
}



Boolean C1222DLPacket_ToggleState(C1222DLPacket* p)
{
    return (Boolean)((packet_ctrl(p) & 0x20) ? TRUE : FALSE);
}



Boolean C1222DLPacket_IsTransparencyEnabled(C1222DLPacket* p)
{
    return (Boolean)((packet_ctrl(p) & 0x10) ? TRUE : FALSE);
}



C1222DLAck C1222DLPacket_GetAcknowledgement(C1222DLPacket* p)
{
    return (C1222DLAck)((packet_ctrl(p) & 0x0C) >> 2);
}



Boolean C1222DLPacket_IsC1222Format(C1222DLPacket* p)
{
    return (Boolean)(((packet_ctrl(p) & 0x03) == 0x01) ? TRUE : FALSE);
}


