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


#ifndef EPSEM_H
#define EPSEM_H

// this is a container for psem requests or responses
// format is length1, request1, length2, request2, ..., 0

#include "c1222.h"
#include "c1222environment.h"

#define MAX_EPSEM_MESSAGE_LENGTH    515

typedef struct EpsemTag
{
    Unsigned8  epsemControl;
    Unsigned8  deviceClass[4];
    
    Unsigned8* buffer;
    Unsigned16 maxLength;
    
    Unsigned16 length;  // index of next byte to process
    
} Epsem;

EXTERN_C void Epsem_Init(Epsem* p, Unsigned8* buffer, Unsigned16 length);
EXTERN_C void Epsem_Rewind(Epsem* p);
EXTERN_C Boolean Epsem_GetNextRequestOrResponse(Epsem* pthis, Unsigned8** buffer, Unsigned16* pLength);
EXTERN_C Boolean Epsem_AddRequestOrResponse(Epsem* p, Unsigned8* requestOrResponse, Unsigned16 length);
EXTERN_C Unsigned8 Epsem_GetResponseMode(Epsem* p);
EXTERN_C Boolean Epsem_Validate(Epsem* p);
EXTERN_C void Epsem_PositionToEnd(Epsem* p);
EXTERN_C Unsigned16 Epsem_GetFullLength(Epsem* p);

#endif
