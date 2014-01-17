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


#ifndef C1222MISC_H
#define C1222MISC_H
#include "c1222.h"
#include "c1222environment.h"


// prototypes
EXTERN_C  Unsigned32 C1222Misc_GetFreeRunningTimeInMS(void);

EXTERN_C  Boolean C1222Misc_DelayExpired(Unsigned32 start, Unsigned32 delay);

EXTERN_C  void C1222Misc_ReverseBytes(void* p, Unsigned8 length);

EXTERN_C  Unsigned8 C1222Misc_EncodeLength( Unsigned16 length, Unsigned8* buffer);
EXTERN_C  Unsigned16 C1222Misc_DecodeLength(Unsigned8* buffer, Unsigned8* pSizeofLength);

EXTERN_C  Unsigned8 C1222Misc_GetSizeOfLengthEncoding(Unsigned16 length);

EXTERN_C  void C1222Misc_IsrIncrementFreeRunningTime(Unsigned16 ms);

EXTERN_C  Unsigned8 C1222Misc_GetUIDLength(Unsigned8* uid);

EXTERN_C  void C1222Misc_AdjustFreeRunningTime(Unsigned32 msToAdd );

EXTERN_C void C1222Misc_RandomizeBuffer(Unsigned8* buffer, Unsigned16 length);

EXTERN_C Unsigned32 C1222Misc_GetRandomDelayTime(Unsigned32 minDelay, Unsigned32 maxDelay);

EXTERN_C void C1222Misc_ResetFreeRunningTime(void);


// useful macro
#define ELEMENTS(x)    (sizeof(x)/sizeof((x)[0]))

#endif
