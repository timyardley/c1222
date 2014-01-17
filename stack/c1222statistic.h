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


#ifndef C1222_STATISTICS_H
#define C1222_STATISTICS_H

typedef enum
{
    // standard statistics ...
    C1222_STATISTIC_NO_STATISTIC_REPORTED = 0, //		0	No statistic reported
	C1222_STATISTIC_NUMBER_OF_OCTETS_SENT = 1, //       1   Number of octets sent
	C1222_STATISTIC_NUMBER_OF_OCTETS_RECEIVED = 2, //	2	Number of octets received
	C1222_STATISTIC_NUMBER_OF_ACSE_PDU_SENT = 3,    //	3	Number of <acse-pdu> sent
	C1222_STATISTIC_NUMBER_OF_ACSE_PDU_RECEIVED = 4, //	4	Number of <acse-pdu> received
	C1222_STATISTIC_NUMBER_OF_ACSE_PDU_FORWARDED = 5, //	5	Number of <acse-pdu> forwarded
	C1222_STATISTIC_NUMBER_OF_ACSE_PDU_DROPPED = 6, //	6	Number of <acse-pdu> dropped
	C1222_STATISTIC_NUMBER_OF_TRANSMISSION_ERRORS = 7, //	7	Number of transmission errors
	C1222_STATISTIC_NUMBER_OF_RECEPTION_ERRORS = 8, //	8	Number of reception errors
	C1222_STATISTIC_NUMBER_OF_COLLISIONS = 9, //    	9	Number of collisions
	C1222_STATISTIC_NUMBER_OF_MESSAGE_OVERRUNS = 10, //	10	Number of message overruns
	C1222_STATISTIC_NUMBER_OF_FRAMING_ERRORS = 11, //	11	Number of framing errors
	C1222_STATISTIC_NUMBER_OF_CHECKSUM_ERRORS = 12, //	12	Number of checksum errors
	C1222_STATISTIC_NUMBER_OF_ACTIVE_ASSOCIATIONS = 13, //	13	Number of active Associations
	C1222_STATISTIC_NUMBER_OF_ASSOCIATION_TIMEOUTS = 14, //	14	Number of Associations timeouts
	C1222_STATISTIC_NUMBER_OF_SIGNAL_CARRIERS_LOST = 15, //	15	Number of signal carriers lost
	C1222_STATISTIC_SIGNAL_STRENGTH_IN_PERCENT = 16, //	16	Signal strength (0-100%)
	C1222_STATISTIC_SIGNAL_STRENGTH_IN_DBM = 17, // 	17	Signal strength (dBm)
	C1222_STATISTIC_NUMBER_OF_REGISTRATIONS_SENT = 18, //	18	Number of registrations sent
	C1222_STATISTIC_NUMBER_OF_REGISTRATIONS_RECEIVED = 19, //	19	Number of registrations received
	C1222_STATISTIC_NUMBER_OF_REGISTRATIONS_DENIED = 20, //	20	Number of registration denials (received but rejected)
	C1222_STATISTIC_NUMBER_OF_REGISTRATIONS_FAILED = 21, //	21	Number of registrations failed (issued but rejected or lost)
	C1222_STATISTIC_NUMBER_OF_DEREGISTRATIONS_REQUESTED = 22, //	22	Number of de-registrations requested
	
	// manufacturers statistics ...
	C1222_STATISTIC_IS_SYNCHRONIZED_NOW = 2049,
	C1222_STATISTIC_RESYNCHRONIZED_COUNT = 2050,
	C1222_STATISTIC_REGISTRATION_SUCCEEDED = 2051,
	C1222_STATISTIC_RFLAN_CURRENT_CELL_ID = 2052,
	C1222_STATISTIC_NUMBER_OF_RFLAN_CELL_CHANGES = 2053,
	C1222_STATISTIC_DEREGISTRATION_SUCCEEDED = 2054,
	C1222_STATISTIC_RFLAN_CURRENT_LEVEL = 2055,
	C1222_STATISTIC_NUMBER_OF_RFLAN_LEVEL_CHANGES = 2056,
	C1222_STATISTIC_RFLAN_STATE_INFO = 2057,
    //
    // Additional RFLAN stats for Hartman's group...
    //
    C1222_STATISTIC_RFLAN_GPD = 2058,
    C1222_STATISTIC_RFLAN_FATHER_COUNT = 2059,
    C1222_STATISTIC_RFLAN_SYNCH_FATHER_MAC_ADDRESS = 2060,
    C1222_STATISTIC_RFLAN_SYNCH_FATHER_INFO = 2061,
    C1222_STATISTIC_RFLAN_BEST_FATHER_MAC_ADDRESS = 2062,
    C1222_STATISTIC_RFLAN_BEST_FATHER_INFO = 2063,
    C1222_STATISTIC_RFLAN_NET_DOWNLINK_PACKET_COUNT = 2064,
    C1222_STATISTIC_RFLAN_NET_UPLINK_PACKET_COUNT = 2065,
    C1222_STATISTIC_RFLAN_MAC_MONOCAST_RX_COUNT = 2066,
    C1222_STATISTIC_RFLAN_NET_ROUTING_COUNT = 2067

} C1222_STATISTIC_ID;

//#ifndef __BORLANDC__  // BORLAND complains about the bit fields


typedef struct tagRFLAN_STATE_INFO
{
    Unsigned32  m_CoreDumpState : 4;
    Unsigned32  m_NetworkState : 2;
    Unsigned32  m_BroadcastState : 1;
    Unsigned32  m_IsWarmStart : 1;
    Unsigned32  m_OutageID : 8;
    Unsigned32  m_Reserved : 16;
    Unsigned8   m_Version;
    Unsigned8   m_Revision;
} RFLAN_STATE_INFO;

typedef struct tagRFLANFATHERINFO
{

    Unsigned8   m_Level;
    Unsigned8   m_LPD;
    Unsigned16  m_GPD;
    Unsigned8   m_RXI;
    signed char m_AverageRSSI;
} RFLAN_FATHER_INFO;

//#endif

#endif
