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


#ifndef C1222RESPONSE_H

#define C1222RESPONSE_OK        0x00
#define C1222RESPONSE_ERR       0x01
#define C1222RESPONSE_SNS       0x02
#define C1222RESPONSE_ISC       0x03
#define C1222RESPONSE_ONP       0x04
#define C1222RESPONSE_IAR       0x05
#define C1222RESPONSE_BSY       0x06
#define C1222RESPONSE_DNR       0x07
#define C1222RESPONSE_DLK       0x08
#define C1222RESPONSE_RNO       0x09
#define C1222RESPONSE_ISSS      0x0A
#define C1222RESPONSE_SME       0x0B
#define C1222RESPONSE_UAT       0x0C
#define C1222RESPONSE_NETT      0x0D
#define C1222RESPONSE_NETR      0x0E
#define C1222RESPONSE_RQTL      0x0F
#define C1222RESPONSE_RSTL      0x10
#define C1222RESPONSE_SGNP      0x11
#define C1222RESPONSE_SGERR     0x12

// the 2006 standard had a duplication of rstl and sgnp
// this was fixed in 2008 version

#define C1222RESPONSE_SGNP_2006      0x10
#define C1222RESPONSE_SGERR_2006     0x11


#endif
