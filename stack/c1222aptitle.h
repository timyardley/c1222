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


#ifndef C1222APTITLE_H
#define C1222APTITLE_H

#include "c1222.h"
#include "c1222environment.h"

#define C1222_APTITLE_LENGTH          20



typedef struct C1222ApTitleTag
{
	Unsigned8* buffer;
	Unsigned8 length;
	Unsigned8 maxLength;
	Boolean isACSE2008Format;
} C1222ApTitle;

EXTERN_C Boolean C1222ApTitle_TableValidate( C1222ApTitle* p);
EXTERN_C Boolean C1222ApTitle_Validate( C1222ApTitle* p);
EXTERN_C Unsigned8 C1222ApTitle_GetLength( C1222ApTitle* p);

EXTERN_C Boolean C1222ApTitle_Is2ndBranchOf1st( C1222ApTitle* p, C1222ApTitle* pBranch, Unsigned8* pBranchStartIndex);
EXTERN_C Boolean C1222ApTitle_MakeAbsoluteFrom( C1222ApTitle* p, C1222ApTitle* pSource, Boolean want2006Standard );
EXTERN_C void C1222ApTitle_Construct( C1222ApTitle* p, Unsigned8* buffer, Unsigned8 length);
EXTERN_C Boolean C1222ApTitle_MakeRelativeFrom( C1222ApTitle* p, C1222ApTitle* pSource);
EXTERN_C Boolean C1222ApTitle_Compare(C1222ApTitle* p1, C1222ApTitle* p2, int* pdiff);
EXTERN_C void C1222ApTitle_Copy(C1222ApTitle* dest, Unsigned8* destBuffer, C1222ApTitle* source);
EXTERN_C Boolean C1222ApTitle_IsBroadcastToMe(C1222ApTitle* aptitle, C1222ApTitle* myApTitle);
#if 0  // this routine is not really needed at the moment anyway
EXTERN_C Boolean C1222ApTitle_IsAMulticastAddress(C1222ApTitle* aptitle);
#endif
EXTERN_C Boolean C1222ApTitle_IsRelative( C1222ApTitle* p);
EXTERN_C Boolean C1222ApTitle_IsAbsolute( C1222ApTitle* p);
EXTERN_C Boolean C1222ApTitle_IsApTitleMyCommModule(C1222ApTitle* pApTitle, C1222ApTitle* pMyApTitle);

EXTERN_C Boolean C1222ApTitle_ValidateACSEFormatMarch2006( C1222ApTitle* p);
EXTERN_C Boolean C1222ApTitle_ValidateACSEFormat2008( C1222ApTitle* p);
EXTERN_C Boolean C1222ApTitle_ValidateACSEFormat2008_AnyRoot(C1222ApTitle* p);
EXTERN_C Boolean C1222ApTitle_ValidateACSEFormatMarch2006_AnyRoot(C1222ApTitle* p);
EXTERN_C void C1222ApTitle_ConvertACSERelativeToTableRelative(Unsigned8* aptitleBuffer);


#endif
