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

// this file defines the event ids that will be used in the lan communication 
// log

#ifndef COMEVENT_ID_H
#define COMEVENT_ID_H

#define COMEVENT_MAX_PARAMETER_SIZE                (C1222_APTITLE_LENGTH+2*sizeof(Unsigned16)+2*sizeof(Unsigned8))

#define COMEVENT_C1222_LINK_FAILURE	                1
#define COMEVENT_C1222_LINK_SWITCH	                2
#define COMEVENT_C1222_LINK_UP	                    3
#define COMEVENT_C1222_LINK_METRIC	                4

#define COMEVENT_C1222_SEND_PERIODIC_READ_FAILED	5
#define COMEVENT_C1222_SEND_PERIODIC_READ_SUCCESS   6

#define COMEVENT_C1222_SEND_EXCEPTION_FAILED        7        
#define COMEVENT_C1222_SEND_EXCEPTION_SUCCESS       8

#define COMEVENT_C1222_SEND_RESPONSE_FAILED         9
#define COMEVENT_C1222_SEND_RESPONSE_SUCCESS        10

#define COMEVENT_C1222_SENT_SEGMENT_BYTES           11
#define COMEVENT_C1222_RECEIVED_SEGMENT_BYTES       12      

#define COMEVENT_C1222_RECEIVED_REQUEST             13  
#define COMEVENT_C1222_SENT_RESPONSE                14      

#define COMEVENT_C1222_SEND_MESSAGE_SUCCEEDED       15
#define COMEVENT_C1222_SEND_MESSAGE_FAILED          16

#define COMEVENT_C1222_CONFIGURED_SEND_FAILURE_LIMIT_EXCEEDED	17

#define COMEVENT_C1222_REGISTRATION_RESULT	        18
#define COMEVENT_C1222_DEREGISTRATION_RESULT	    19
#define COMEVENT_C1222_COMM_MODULE_LINK_RESET	    20
#define COMEVENT_C1222_COMM_MODULE_LINK_FAILED	    21
#define COMEVENT_C1222_COMM_MODULE_LINK_UP	        22
#define COMEVENT_C1222_LEVEL_CHANGE	                23
#define COMEVENT_C1222_BEST_FATHER_CHANGE	        24
#define COMEVENT_C1222_SYNCH_FATHER_CHANGE	        25

#define COMEVENT_C1222_PROCESS_RCVD_MSG_TIMING      26

#define COMEVENT_C1222_REGISTRATION_ATTEMPT	        27
#define COMEVENT_C1222_REGISTERED	                28
#define COMEVENT_C1222_DEREGISTRATION_ATTEMPT	    29
#define COMEVENT_C1222_DEREGISTERED	                30
#define COMEVENT_C1222_RFLAN_CELL_CHANGE	        31

#define COMEVENT_C1222_RECEIVED_INVALID_MESSAGE     32

#define COMEVENT_RECEIVED_MESSAGE_FROM              33
#define COMEVENT_SENT_MESSAGE_TO                    34
#define COMEVENT_PR_SEND_TABLE                      35
#define COMEVENT_PR_SEND_TABLE_FAILED               36
#define COMEVENT_C1222_RESET                        37
#define COMEVENT_C1222_SENT_SIMPLE_ERROR_RESPONSE   38
#define COMEVENT_C1222_RFLAN_COLD_START             39

#endif

