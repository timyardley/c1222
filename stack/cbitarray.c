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

//-----------------------------------------------------------------------
// Module: bitarray.cc
// Date  : April 12, 1995
//
// Description : MultiByteBitArray
// 
// A MultiByteBitArray is several bytes treated as an array of bits, with
// capability to set, clear, toggle, and check the contents of individual 
// bits.  The bits are addressed by bit number in the total array.  Bit 
// numbering is low to high for bytes and high to low for bits.  Here are
// a few examples of bit numbers:
//
//   bit# =  byte index    bit number   mask
//    0          0             7        0x80
//    1          0             6        0x40
//    7          0             0        0x01
//    8          1             7        0x80
//    9          1             6        0x40
//
//
//
//-----------------------------------------------------------------------



//#include "slbtypes.h"
#include "cbitarray.h"
#include <limits.h>



//  These are masks used to convert from a bit number to the bit contents

static const unsigned char bitmasks[CHAR_BIT]   =
                       { 0x80, 0x40, 0x20, 0x10,
	                     0x08, 0x04, 0x02, 0x01 };

                       

#if CHAR_BIT != 8
#error Number of initializers wrong for bitmasks array
#endif






Boolean CMultiByteBitArray__isBitSet(const unsigned char* pArray, unsigned long n)
  //  this is here to enforce the same bit order as is used in a
  //  multibyte bit array
{
#if CHAR_BIT == 8
  Unsigned16 byteIndex = (Unsigned16)(n>>3);
  Unsigned16 bitIndex = (Unsigned16)(n & 0x7);
#else
  Unsigned16 byteIndex = (Unsigned16)(n/CHAR_BIT);
  Unsigned16 bitIndex  = (Unsigned16)(n % CHAR_BIT);
#endif

  return (Boolean)((pArray[byteIndex] & bitmasks[bitIndex]) ? TRUE : FALSE);
}


void CMultiByteBitArray__setBit(unsigned char* pArray, unsigned long n, Boolean value)
{
#if CHAR_BIT == 8
  Unsigned16 byteIndex = (Unsigned16)(n>>3);
  Unsigned16 bitIndex = (Unsigned16)(n & 0x7);
#else
  Unsigned16 byteIndex = (Unsigned16)(n/CHAR_BIT);
  Unsigned16 bitIndex  = (Unsigned16)(n % CHAR_BIT);
#endif

    if ( value )
      pArray[byteIndex] |=  bitmasks[bitIndex];     //  set it to 1
    else
      pArray[byteIndex] &= (unsigned char)(~bitmasks[bitIndex]);   //  set it to 0
}


static const unsigned char reversebitmasks[CHAR_BIT]   =
                       { 0x01, 0x02, 0x04, 0x08,
	                     0x10, 0x20, 0x40, 0x80 };

Boolean CMultiByteReverseBitArray__isBitSet(const unsigned char* pArray, unsigned long n)
  //  this is here to enforce the same bit order as is used in a
  //  multibyte bit array
{
#if CHAR_BIT == 8
  Unsigned16 byteIndex = (Unsigned16)(n>>3);
  Unsigned16 bitIndex = (Unsigned16)(n & 0x7);
#else
  Unsigned16 byteIndex = (Unsigned16)(n/CHAR_BIT);
  Unsigned16 bitIndex  = (Unsigned16)(n % CHAR_BIT);
#endif

  return (Boolean)((pArray[byteIndex] & reversebitmasks[bitIndex]) ? TRUE : FALSE);
}



//-----------------------------------------------------------------------
// End of Module
//-----------------------------------------------------------------------




