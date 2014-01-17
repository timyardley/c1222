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


#ifndef C1222EVENT_H
#define C1222EVENT_H

// these event ids must be kept synchronized with the descriptions below

typedef enum C1222EventIdTag
{
    C1222EVENT_NOEVENT,
    
    C1222EVENT_AL_MSG_NOT_SENT_TL_BUSY, //1
    C1222EVENT_AL_SENDING_MSG,
    C1222EVENT_AL_START_SEND_FAILED,
    C1222EVENT_AL_CONFIG_SENT,
    C1222EVENT_AL_SIMPLE_RESPONSE,
    
    C1222EVENT_AL_REGISTERED, // 6
    C1222EVENT_AL_REG_RESPONSE_INVALID,
    C1222EVENT_AL_OTHER_MESSAGE_WAITING_FOR_REG_RESPONSE,
    C1222EVENT_AL_TIME_TO_REGISTER,
    C1222EVENT_AL_TIME_TO_REFRESH_REGISTRATION,
    
    C1222EVENT_AL_WAIT_FOR_REG_STATUS_TIMEOUT, //11
    C1222EVENT_AL_SEND_GET_INTF_INFO_AND_STATS,
    C1222EVENT_AL_SEND_LINK_CTRL_ENABLE_AUTO_REG,
    C1222EVENT_AL_SENT_GET_INTF_STATUS,
    C1222EVENT_AL_SEND_GET_REG_STATUS,
    C1222EVENT_AL_SENT_LINK_CTRL_RELOAD_CONFIG,
    
    C1222EVENT_ALR_CANT_SET_UNSEG_MSG1, //17
    C1222EVENT_ALR_ADDSEG_MSG_TOO_LONG,
    C1222EVENT_ALR_ADDSEG_FULLMSG_TOO_LONG,
    C1222EVENT_ALR_ADDSEG_GET_SEG_STATUS_ERR,
    C1222EVENT_ALR_CANT_SET_SEGMENT,
    C1222EVENT_ALR_CANT_SET_UNSEG_MSG2,
    
    C1222EVENT_ALS_SET_UNSEG_MSG_FAILED, //23
    C1222EVENT_ALS_CANT_GET_NATIVE_ADDRESS,
    C1222EVENT_ALS_SET_UI_FAILED_IN_SIMPLE_RESPONSE,
    C1222EVENT_ALS_GET_SEG_FAILED,
    
    C1222EVENT_ALSG_P_S, //27
    C1222EVENT_ALSG_OVERHEAD_TOO_LARGE,
    C1222EVENT_ALSG_SEG_INDEX_INVALID,
    C1222EVENT_ALSG_MAX_TOO_SMALL,
    
    C1222EVENT_ALSG_MISSING_A8_TAG, //31
    C1222EVENT_ALSG_GET_SEG_NUM_OFF_T_L,
    C1222EVENT_ALSG_SHIFTING_MSG,
    C1222EVENT_ALSG_TOO_MANY_DISJOINT_SEGMENTS,
    C1222EVENT_ALSG_OFFSET_GE_FULLLENGTH,
    C1222EVENT_ALSG_LENGTH_GE_FULLLENGTH,
    C1222EVENT_ALSG_OFF_PLUS_LEN_GE_FULLLEN,
    C1222EVENT_ALSG_FULLLEN_TOO_BIG,
    C1222EVENT_ALSG_DISJOINT_SEG_ERR,
    C1222EVENT_ALSG_SET_SEG_SQ_OFF_FULL_L,
    C1222EVENT_ALSG_SET_SEG_CANT_GET_APP_DATA,
    C1222EVENT_ALSG_SET_SEG_CANT_GET_OFFSET,
    
    C1222EVENT_DL_PACKET_AVAILABLE_FROM_SEND, //43
    C1222EVENT_DL_PROCESS_RECEIVED_PACKET,
    C1222EVENT_DL_NAK_ICTO_OR_CRC,
    C1222EVENT_DL_LINK_TRAFFIC_TIMEOUT,
    C1222EVENT_DL_DUPLICATE_PACKET_IN_SEND,
    C1222EVENT_DL_WRONG_SEQUENCE_IN_SEND,
    C1222EVENT_DL_INVALID_PACKET_IN_SEND,
    C1222EVENT_DL_NAK_COLLISION_IN_SEND,
    C1222EVENT_DL_PROCESS_RECEIVED_PACKET_IN_SEND, //51
    C1222EVENT_DL_NAK_ICTO_OR_CRC_IN_SEND,
    C1222EVENT_DL_DUPLICATE_PACKET,
    C1222EVENT_DL_WRONG_SEQUENCE,
    C1222EVENT_DL_INVALID_PACKET,
    C1222EVENT_DL_PACKET_SENT_TO_TL, // 56
    C1222EVENT_DL_MESSAGE_COMPLETE,
    C1222EVENT_DL_NAK_ICTO,
    C1222EVENT_DL_WAIT_FOR_ACK_LINK_FAILURE,
    
    C1222EVENT_TL_SENDING_DELAYED_REQUEST, //60
    C1222EVENT_TL_NO_RESPONSE_TO_LAST_REQUEST,
    C1222EVENT_TL_DELAY,
    C1222EVENT_TL_SEND_ACSE_MSG,
    C1222EVENT_TL_SEND_FAILED,
    C1222EVENT_TL_SEND_NEGOTIATE_REQUEST,
    C1222EVENT_TL_SEND_LINK_CONTROL_REQUEST,
    C1222EVENT_TL_SEND_GET_CONFIG_REQUEST,
    C1222EVENT_TL_SEND_GET_STATUS_REQUEST,
    C1222EVENT_TL_SENT_GET_REG_STATUS_REQUEST,
    C1222EVENT_TL_DL_NOTE_LINK_FAILURE, //70
    C1222EVENT_TL_BAD_RESULT_CODE, 
    C1222EVENT_TL_PROCESS_OK_RESPONSE_FOR_PENDING_REQUEST,
    C1222EVENT_TL_RCV_RESPONSE_TO_NEGOTIATE,
    C1222EVENT_TL_RCV_RESPONSE_TO_GETCONFIG,   
    C1222EVENT_TL_RCV_RESPONSE_TO_LINKCONTROL,    
    C1222EVENT_TL_RCV_RESPONSE_TO_SEND_MESSAGE,    
    C1222EVENT_TL_RCV_RESPONSE_TO_GET_STATUS,    
    C1222EVENT_TL_RCV_RESPONSE_TO_GET_REG_STATUS,    
    C1222EVENT_TL_RCV_RESPONSE_TO_SEND_SHORT,    
    C1222EVENT_TL_RCV_RESPONSE_TO_UNKNOWN, //80
    C1222EVENT_TL_PROCESS_COMMAND, 
    C1222EVENT_TL_RCV_CMD_NEGOTIATE,
    C1222EVENT_TL_RCV_CMD_GET_CONFIG,   
    C1222EVENT_TL_RCV_CMD_LINK_CONTROL,    
    C1222EVENT_TL_RCV_CMD_SEND_MSG, // 85   
    C1222EVENT_TL_RCV_CMD_GET_STATUS,    
    C1222EVENT_TL_RCV_CMD_GET_LINK_STATUS,    
    C1222EVENT_TL_RCV_CMD_SEND_SHORT,    
    C1222EVENT_TL_RCV_CMD_UNDEFINED,
    C1222EVENT_TL_SEND_NEGOTIATE_RESPONSE, //90
    C1222EVENT_TL_CHANGED_LINK_PARAMETERS,
    C1222EVENT_TL_SEND_GET_CONFIG_RESPONSE_FAILED,
    C1222EVENT_TL_ESN_INVALID,
    C1222EVENT_TL_CONFIG_MESSAGE_TOO_SHORT,
    C1222EVENT_TL_BOTH_ESN_AND_APTITLE_ARE_MISSING, 
    C1222EVENT_TL_CONFIG_UPDATED, 
    C1222EVENT_TL_SENDING_LINK_CONTROL_RESPONSE, 
    C1222EVENT_TL_RECEIVED_LINK_CONTROL_RESPONSE,
    C1222EVENT_TL_SENDING_SEND_MSG_RESPONSE,
    C1222EVENT_TL_RECEIVED_SEND_MSG_RESPONSE_AL_NOTIFIED, //100
    C1222EVENT_TL_SEND_GET_STATUS_RESPONSE_FAILED,
    C1222EVENT_TL_INTERFACE_INFO_UPDATED,
    C1222EVENT_TL_SEND_GET_REG_STATUS_RESPONSE_FAILED,
    C1222EVENT_TL_REG_STATUS_UPDATED,
    C1222EVENT_TL_SEND_SHORT_MSG,
    C1222EVENT_TL_SEND_SHORT_FAILED,
    
    C1222EVENT_PL_PORT1_RX, //107
    C1222EVENT_PL_PORT2_RX,
    C1222EVENT_PL_PORT3_RX,
    C1222EVENT_PL_PORT4_RX,
    
    C1222EVENT_PL_PORT1_TX, //111
    C1222EVENT_PL_PORT2_TX,
    C1222EVENT_PL_PORT3_TX,
    C1222EVENT_PL_PORT4_TX,
    
    C1222EVENT_SYSTEMTIME, //115
    
    C1222EVENT_AL_NOTERECEIVEMESSAGEHANDLINGCOMPLETE,
    C1222EVENT_TLSENDPENDINGOKRESPONSE,
    C1222EVENT_DELAYINGSENDMESSAGERESPONSE,
    
    C1222EVENT_ALS_RESTARTING_SEND, // 119
    C1222EVENT_ALS_SENDNEXTSEGMENT_FAILED,
    C1222EVENT_ALS_ALL_RETRIES_FAILED,
    C1222EVENT_TL_SENDING_NETR_RESPONSE_HANDLING_NOT_COMPLETE,
    C1222EVENT_AL_RECEIVEMESSAGEHANDLINGCOMPLETEBUTNOTPENDING,
    C1222EVENT_AL_NOTERECEIVEMESSAGEHANDLINGFAILED,
    C1222EVENT_AL_RECEIVEMESSAGEHANDLINGFAILEDBUTNOTPENDING,
    
    C1222EVENT_TLSENDPENDINGFAILEDRESPONSE, // 126
    C1222EVENT_TLSENDPENDINGFAILEDRESPONSENOTPENDING,
    
    C1222EVENT_AL_COLLISION, // 128
    C1222EVENT_AL_SEND_LINK_CTRL_DISABLE_AUTO_REG,
    
    C1222EVENT_AL_NOTEREGISTERMESSAGEHANDLINGCOMPLETE, // 130
    C1222EVENT_AL_NOTEREGISTERMESSAGEHANDLINGFAILED,
    
    C1222EVENT_AL_REGISTEREDAPTITLEINVALID, // 132
    C1222EVENT_AL_ESN_INVALID,
    C1222EVENT_AL_ESN_OR_APTITLE_TOOLONG,
    
    C1222EVENT_DL_ACK_RECEIVED, // 135
    C1222EVENT_DL_NAK_RECEIVED,
    C1222EVENT_DL_ACK_TIMEOUT,
    C1222EVENT_DL_GARBAGE_RECEIVED,
    C1222EVENT_DL_NONACK_PACKET_RECEIVED,
    C1222EVENT_DL_NONC1222PACKET_RECEIVED,
    
    C1222EVENT_AL_RECEIVEDMESSAGECOMPLETE, // 141
    
    C1222EVENT_AL_COMPLETEMESSAGEDISCARDED,
    C1222EVENT_AL_PARTIALMESSAGEDISCARDED,
    
    C1222EVENT_DEVICE_PERIODICREADSENDFAILED,  // 144
    C1222EVENT_DEVICE_PERIODICREADINVALIDREQUEST,
    C1222EVENT_DEVICE_EXCEPTIONREPORTFAILEDSEND,
    C1222EVENT_DEVICE_PERIODICREADREADERROR,
    
    C1222EVENT_DEVICE_RESETRFLANSTACK, // 148
    
    C1222EVENT_AL_DISCARDEDEXTRAREGRESPONSE,
    C1222EVENT_SERVER_IGNORINGRESPONSE,
    C1222EVENT_DL_MESSAGETOOLONGTOSEND,
    
    C1222EVENT_DL_SETPARAMETERS, // 152
    
    C1222EVENT_AL_SENDTIMEDOUT,
    
    C1222EVENT_APPLICATION_ERROR,
    
    C1222EVENT_ALR_DUPLICATE_SEGMENT, // 155
    C1222EVENT_ALSG_DISCARDED_MESSAGE,
    
    C1222EVENT_DEVICE_DUPLICATE_MESSAGE_DROPPED,
    
    C1222EVENT_AL_SEND_REGISTRATION_FAILED, // 158
    
    C1222EVENT_ALSG_RX_SEGMENT_TIMEOUT,
    
    C1222EVENT_AL_TIME_TO_DEREGISTER, // 160
    C1222EVENT_AL_DEREGISTERED,
    C1222EVENT_AL_DEREG_RESPONSE_INVALID,
    C1222EVENT_AL_OTHER_MESSAGE_WAITING_FOR_DEREG_RESPONSE,
    C1222EVENT_AL_DEREGISTEREDAPTITLEINVALID,
    C1222EVENT_AL_SEND_DEREGISTRATION_FAILED, // 165
    C1222EVENT_DL_SETPARAMETERSSTART, // 166
    C1222EVENT_TL_LINK_RESET,
    
    C1222EVENT_ALSG_RX_MESSAGE_CANCELED, // 168
    C1222EVENT_TL_IGNORINGSENDMESSAGEBEFORECONFIG, // 169
    C1222EVENT_DEVICE_SKIPDUPCHECKONRFLANMSG,  // 170
    C1222EVENT_DEVICE_SKIPDUPCHECKONRFLANFACTORYREADORUPDATEVENDORFIELD, // 171
    C1222EVENT_TL_FAILINGSENDMESSAGEBEFORECONFIG, // 172
    C1222EVENT_DEVICE_RESETRFLANCHIP, // 173
    C1222EVENT_ALR_IGNOREDRFLANMESSAGE, // 174
    C1222EVENT_DEVICE_2108_UPDATED, // 175
    C1222EVENT_DEVICE_OPTIONAL_COMM_ALLOWED, // 176
    
    C1222EVENT_AL_INVALID_MESSAGE_RECEIVED, // 177
    C1222EVENT_AL_2006_FORMAT_MESSAGE_RECEIVED,
    C1222EVENT_AL_2008_FORMAT_MESSAGE_RECEIVED,
        
    C1222EVENT_NUMBEROF  // Keep this one last in the list
       
          
    
} C1222EventId;


#ifdef C1222_WANT_EVENTID_TEXT

// these descriptions must be kept synchronized with the above event ids

const char * c1222_event_descriptions[] = 
    {
        "No event",
        
        "AL: msg not sent - TL busy",
        "AL: sending msg",
        "AL: start send failed",
        "AL: config sent",
        "AL: simple response",
        
        "AL: registered",
        "AL: reg response invalid",
        "AL: other message received waiting for reg response",
        "AL: time to register",
        "AL: time to refresh registration",
        
        "AL: wait for reg status timeout",
        "AL: send get intf info + stats",
        "AL: send link ctrl enable auto reg",
        "AL: send get intf status",
        "AL: send get reg status",
        "AL: send link ctrl reload config",
        
        "ALR: can't set unseg msg1",
        "ALR: addseg - msg too long",
        "ALR: addseg - fullmsg too long",
        "ALR: addseg - get seg status err",
        "ALR: addseg - can't set segment",
        "ALR: addseg - can't set unseg msg2",
        
        "ALS: set unseg msg failed",
        "ALS: can't get native address",
        "ALS: set ui failed in simple response",
        "ALS: get seg failed",
        
        "ALSG: P,S",
        "ALSG: overhead too large",
        "ALSG: seg index invalid",
        "ALSG: max too small",
        "ALSG: missing a8 tag",
        "ALSG: get seg #/off/t/l",
        "ALSG: shifting msg",
        "ALSG: too many disjoint segments",
        "ALSG: offset >= fulllength",
        "ALSG: length >= full length",
        "ALSG: off+len >= fulllen",
        "ALSG: fulllen too big",
        "ALSG: disjoint seg err",
        "ALSG: set seg SQ,Off,Full,L",
        "ALSG: set seg - can't get app data",
        "ALSG: set seg - can't get offset",
        
        "DL: Packet available from send",
        "DL: Process received packet",
        "DL: nak icto or crc",
        "DL: Link traffic timeout",
        "DL: Duplicate packet in send",
        "DL: Wrong sequence in send",
        "DL: Invalid packet in send",
        "DL: nak collision in send",
        "DL: Process received packet in send",
        "DL: nak icto or crc in send",
        "DL: Duplicate packet",
        "DL: Wrong sequence",
        "DL: Invalid packet",
        "DL: Packet sent to TL",
        "DL: Message complete",
        "DL: nak icto",
        "DL: Wait for ack link failure",
        
        "TL: sending delayed request",
        "TL: no response to last request",
        "TL: delay",
        "TL: send acse msg",
        "TL: send failed",
        "TL: send negotiate request",
        "TL: send link control request",
        "TL: send get config request",
        "TL: send get status request",
        "TL: send get reg status request",
        "TL: dl note link failure",
        "TL: Bad result code",
        "TL: Process ok response for pending request",
        "TL: rcv response=negotiate",
        "TL: rcv response=get config",
        "TL: rcv response=link control",
        "TL: rcv response=send msg",
        "TL: rcv response=get status",
        "TL: rcv response=get reg status",
        "TL: rcv response=send short",
        "TL: rcv response=UNKNOWN",
        "TL: process command",
        "TL: rcv cmd=negotiate",
        "TL: rcv cmd=get config",
        "TL: rcv cmd=link control",
        "TL: rcv cmd=send msg",
        "TL: rcv cmd=get status",
        "TL: rcv cmd=get link status",
        "TL: rcv cmd=send short",
        "TL: rcv cmd=undefined",
        "TL: send negotiate response",
        "TL: changed link parameters",
        "TL: send get config response failed",
        "TL: esn invalid",
        "TL: config message too short",
        "TL: both esn and aptitle missing",
        "TL: config updated",
        "TL: sending link control response",
        "TL: received link control response",
        "TL: sending send msg response",
        "TL: received send msg response - al notified",
        "TL: send get status response failed",
        "TL: interface info updated",
        "TL: send get reg status response failed",
        "TL: reg status updated",
        "TL: send short message",
        "TL: send short failed",
        
        "PL: port1 rx",
        "PL: port2 rx",
        "PL: port3 rx",
        "PL: port4 rx",
        "PL: port1 tx",
        "PL: port2 tx",
        "PL: port3 tx",
        "PL: port4 tx",
        
        "PL: system time h:m:s:msl:msh",
        "AL: NOTERECEIVEMESSAGEHANDLINGCOMPLETE",
        "TL: SENDPENDINGOKRESPONSE",
        "TL: DELAYINGSENDMESSAGERESPONSE",
        
        "ALS: restarting send",
        "ALS: send next segment failed",
        "ALS: all retries failed",
        
        "TL: sending netr - msg handling incomplete",
        "AL: received msg handling complete - NOT PENDING",
        "AL: received msg handling failed",
        "AL: received msg handling failed not pending",
    
        "TL: send pending failed response",
        "TL: send pending failed response not pending",
        
        "AL: collision",
        "AL: send link ctrl disable auto reg",
        
        "AL: note register msg handling complete",
        "AL: note register msg handling failed",
        
        "AL: registered aptitle invalid",
        "AL: ESN invalid",
        "AL: ESN or aptitle too long",
        
        "DL: ACK received",
        "DL: NAK received",
        "DL: ACK TIMEOUT",
        "DL: GARBAGE received",
        "DL: non-ack packet received",
        "DL: non-C1222 packet received",
        
        "AL: received message complete",
        
        "AL: complete message discarded",
        "AL: partial message discarded",
        
        "Device: Periodic read send failed",
        "Device: Periodic read invalid request",
        "Device: Exception report send failed",
        "Device: Periodic read read error",
        
        "Device: Reset rflan stack",
        
        "AL: discarded extra reg response",
        
        "Server: ignoring response",
        
        "DL: message too long to send",
        "DL: set parameters",
        
        "AL: send timed out",
        
        "Device: application error",
        
        "ALR: duplicate segment received",
        "ALSG: discarded message",
        
        "Device: duplicate msg dropped",
        
        "AL: send registration failed",
        
        "ALSG: rx segment timeout",
        
        "AL: time to deregister",
        "AL: deregisered",
        "AL: dereg response invalid",
        "AL: other message waiting for dereg response",
        "AL: deregistered aptitle invalid",
        "AL: send deregistration failed",
        "DL: SETPARAMETERSSTART",
        "TL: link reset by application",
        
        "ALSG: RX message CANCELED",
        
        "TL: ignoring send msg before config",
        "Device: skip dup check on rflan msg",  // 170
        "Device: skip dup chk on rflan fact rd or vendor update", // 171
        "TL: failing send msg before config", // 172
        
        "Device: Reset RFLAN Chip",
        "ALR: Ignored RFLAN Message",
        "Device: 2108 Updated", // 175
        "Device: Optional comm allowed", // 176
        
        "AL: invalid message received", // 177
        "AL: 2006 format message received", 
        "AL: _2008_ format message received",
        
        
    };

#endif



#endif
