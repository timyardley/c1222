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


// NOTICE:
// The DES algorithm and implementation source used in this file is based on the c12.22 standard 03/02/2006 version.
// The source has been modified from the encryption related source in that standard in three ways:
//     1) to allow it to compile under the various build environments used
//     2) to reduce the ram requirements
//     3) to speed execution (it is still not very fast)
//
// the C12.22 standard 3/02/2006 version contains the following paragraph concerning legal matters:
//
// "Cryptographic devices implementing this standard may be covered by U.S. and foreign patents issued 
// to the International Business Machines Corporation. However(IBM). However, IBM has granted nonexclusive, 
// royalty-free licenses under the patents to make, use and sell apparatus, which complies with the 
// standard. The terms, conditions, and scope of the license are set out in notices published in the 
// May 13, 1975 and August 31, 1976 issues of the Official Gazette of the United States Patent and 
// Trademark Office (9434 O"G" 452 and 949 O.G. 1717)."



#include "c1222.h"
#ifndef WANT_MAIN
#include "c1222environment.h"
#include <limits.h>
#else
#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#endif

#include "cbitarray.h"

//#include <stdio.h>
//#include <stdlib.h>

#ifdef __ICCAVR__
#define MYCONSTPOINTERDECLARATION   uint8 __flash *
#define MYCONSTPOINTERCAST          (uint8 __flash *)
#else
#define MYCONSTPOINTERDECLARATION   C1222CONST uint8 *
#define MYCONSTPOINTERCAST          
#endif


#ifdef DISABLE_STANDARD_TESTPINS
#define TEST_ENCRYPTION_TIMING  1
#endif

// scb - test code
#ifdef TEST_ENCRYPTION_TIMING
#include "bits.h"

static void EncSetupTestPins(void)
{
    // tp95
    PD5_bit.Factory_Signal  = 1;
    P5_bit.Factory_Signal  = 0;
    // tp94
    PD5_bit.Unused_5 = 1;
    P5_bit.Unused_5 = 0;    
    // tp96
    PD6_bit.Unused_5 = 1;
    P6_bit.Unused_5 = 0;    
}

#define SetTP94(x)       P5_bit.Unused_5 = (x)
#define SetTP95(x)       P5_bit.Factory_Signal = (x)
#define SetTP96(x)       P6_bit.Unused_5 = (x) 

#else

#define SetTP94(x)
#define SetTP95(x)
#define SetTP96(x) 

#endif


typedef unsigned char uint8;
typedef unsigned short uint16;

//static uint8    keys[16][6];
typedef struct KeyArrayTag
{
    uint8 keys[16][6];
    
} KeyArrayStruct;


C1222CONST uint8 perm1[56] = {
    57, 49, 41, 33, 25, 17,  9,  1, 58, 50, 42, 34, 26, 18,
    10,  2, 59, 51, 43, 35, 27, 19, 11,  3, 60, 52, 44, 36,
    63, 55, 47, 39, 31, 23, 15,  7, 62, 54, 46, 38, 30, 22,
    14,  6, 61, 53, 45, 37, 29, 21, 13,  5, 28, 20, 12, 4
};

C1222CONST uint8 perm2[56] = {
     2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,  1,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
    44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 29
};

C1222CONST uint8 perm3[48] = {
    14, 17, 11, 24,  1,  5,  3, 28, 15,  6, 21, 10, 23, 19, 12,  4,
    26,  8, 16,  7, 27, 20, 13,  2, 41, 52, 31, 37, 47, 55, 30, 40,
    51, 45, 33, 48, 44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32
};

C1222CONST uint8 perm4[64] = {
    58, 50, 42, 34, 26, 18, 10,  2, 60, 52, 44, 36, 28, 20, 12,  4,
    62, 54, 46, 38, 30, 22, 14,  6, 64, 56, 48, 40, 32, 24, 16,  8,
    57, 49, 41, 33, 25, 17,  9,  1, 59, 51, 43, 35, 27, 19, 11,  3,
    61, 53, 45, 37, 29, 21, 13,  5, 63, 55, 47, 39, 31, 23, 15,  7,
};

C1222CONST uint8 perm5[48] = {
    32,  1,  2,  3,  4,  5,  4,  5,  6,  7,  8,  9,  8,  9, 10, 11,
    12, 13, 12, 13, 14, 15, 16, 17, 16, 17, 18, 19, 20, 21, 20, 21,
    22, 23, 24, 25, 24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32,  1,
};

C1222CONST uint8 perm6[32] = {
    16,  7, 20, 21, 29, 12, 28, 17,  1, 15, 23, 26,  5, 18, 31, 10,
     2,  8, 24, 14, 32, 27,  3,  9, 19, 13, 30,  6, 22, 11,  4, 25,
};

C1222CONST uint8 perm7[64] = {
    40,  8, 48, 16, 56, 24, 64, 32, 39,  7, 47, 15, 55, 23, 63, 31,
    38,  6, 46, 14, 54, 22, 62, 30, 37,  5, 45, 13, 53, 21, 61, 29,
    36,  4, 44, 12, 52, 20, 60, 28, 35,  3, 43, 11, 51, 19, 59, 27,
    34,  2, 42, 10, 50, 18, 58, 26, 33,  1, 41,  9, 49, 17, 57, 25,
};

C1222CONST uint8 sboxes[8][64] = {
{
    14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7,0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8,
    4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0,15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13,
},{
    15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10,3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5,
    0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15,13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9,
},{
    10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8,13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1,
    13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7,1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12,
},{
    7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15,13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9,
    10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4,3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14,
},{
    2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9,14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6,
    4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14,11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3,
},{
    12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11,10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8,
    9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6,4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13,
},{
    4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1,13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6,
    1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2,6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12,
},{
    13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7,1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2,
    7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8,2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11,
}};




static const unsigned char bitmasks[CHAR_BIT]   =
                       { 0x80, 0x40, 0x20, 0x10,
	                     0x08, 0x04, 0x02, 0x01 };

                       

#if CHAR_BIT != 8
#error Number of initializers wrong for bitmasks array
#endif





#if 0
static Boolean _isBitSet(const unsigned char* pArray, Unsigned16 n)
  //  this is here to enforce the same bit order as is used in a
  //  multibyte bit array
{
  return (Boolean)((pArray[n>>3] & bitmasks[n&0x07]) ? TRUE : FALSE);
}


static void _setBit(unsigned char* pArray, Unsigned16 n, Boolean value)
{
    if ( value )
      pArray[n>>3] |=  bitmasks[n & 0x7];     //  set it to 1
    else
      pArray[n>>3] &= (unsigned char)(~bitmasks[n & 0x7]);   //  set it to 0
}
#endif

#define _isBitSet(x,y)     (((x)[(myreg=(Unsigned16)(y))>>3] & bitmasks[(myreg)&0x7]) ? 1 : 0)
#define _isBitMyRegSet(x)     (((x)[(myreg)>>3] & bitmasks[(myreg)&0x7]) ? 1 : 0)

#define _setBit(x,y,z)     myreg=(Unsigned16)(y); ((z)? ((x)[(myreg)>>3] |= (Unsigned8)bitmasks[(myreg)&0x7]) : ((x)[(myreg)>>3] &= (Unsigned8)(~bitmasks[(myreg)&0x7])))

#define permBit(src,dst,srcBit,dstBit)     \
            (((src)[(myreg=(Unsigned16)(srcBit))>>3] & bitmasks[(myreg)&0x7]) ? \
                ( (dst)[(dstBit)>>3] |= (Unsigned8)bitmasks[(dstBit)&0x7] ) \
                :  \
                ( (dst)[(dstBit)>>3] &= (Unsigned8)(~bitmasks[(dstBit)&0x7] )) \
            )

#define permBitMyReg(src,dst,dstBit)     \
            (((src)[(myreg)>>3] & bitmasks[(myreg)&0x7]) ? \
                ( (dst)[(dstBit)>>3] |= (Unsigned8)bitmasks[(dstBit)&0x7] ) \
                :  \
                ( (dst)[(dstBit)>>3] &= (Unsigned8)(~bitmasks[(dstBit)&0x7] )) \
            )


/**************************************************************************/
static void Permutation(uint8 *dst, uint8 *src, uint8 lgn, MYCONSTPOINTERDECLARATION perm_table)
{
    uint8           tmp[8];
    unsigned char  ii;
    register uint16 myreg;
    
    //SetTP96(1);  

    if (src == NULL)
    {
          src = tmp;
          memcpy(src, dst, 8);
    }
    

    for( ii=0; ii<lgn; ii++ )
    {
        SetTP96(1);
        //bit = _isBitSet(src, perm_table[ii]-1);
        //permBit(src, dst, perm_table[ii]-1, ii);
        myreg = (uint16)(perm_table[ii]-1);
        permBitMyReg(src, dst, ii);
        SetTP96(0);
        //_setBit(dst, ii, bit);
    }
    
    //SetTP96(0);
}

/**************************************************************************/
static void Xor(uint8 *dst, uint8 *src, uint8 lgn)
{
    for (; lgn > 0; lgn--, dst++, src++)
    *dst ^= *src;
}

/**************************************************************************/
static void Copy(uint8 *dst, uint8 *src, uint8 lgn)
{
    for (; lgn > 0; lgn--, dst++, src++)
    *dst = *src;
}

/**************************************************************************/
static void SBoxes(uint8 *dst, uint16 dst_offset, uint8 *src, uint16 src_offset, MYCONSTPOINTERDECLARATION sbox)
{
    int          i;
    register uint16 myreg;
    int   jj;
    
    //i = _isBitSet(src,src_offset+4);        //src[4];
    //i |= _isBitSet(src,src_offset+3) << 1; //src[3] << 1;
    //i |= _isBitSet(src,src_offset+2) << 2; //src[2] << 2;
    //i |= _isBitSet(src,src_offset+1) << 3; //src[1] << 3;
    //i |= _isBitSet(src,src_offset+5) << 4; //src[5] << 4;
    //i |= _isBitSet(src,src_offset+0) << 5; //src[0] << 5;

    // the new compiler included in workbench 4.0 gives warnings about undefined order of execution with myreg in the above
    // so lets force the right behavior.
    
    myreg = (uint16)(src_offset+4);
    i = _isBitMyRegSet(src);        //src[4];
    myreg = (uint16)(src_offset+3);
    i |= _isBitMyRegSet(src) << 1; //src[3] << 1;
    myreg = (uint16)(src_offset+2);
    i |= _isBitMyRegSet(src) << 2; //src[2] << 2;
    myreg = (uint16)(src_offset+1);
    i |= _isBitMyRegSet(src) << 3; //src[1] << 3;
    myreg = (uint16)(src_offset+5);
    i |= _isBitMyRegSet(src) << 4; //src[5] << 4;
    myreg = (uint16)(src_offset+0);
    i |= _isBitMyRegSet(src) << 5; //src[0] << 5;

    i = sbox[i];

    for ( jj=0; jj<4; jj++ )
    {
        _setBit(dst, dst_offset+3-jj, (uint8)((i>>jj) & 1)); //dst[3] = (uint8)(i & 1);         // scb: I added cast to uint8 in these 4 lines
    }

    //_setBit(dst, dst_offset+3, (uint8)(i & 1)); //dst[3] = (uint8)(i & 1);         // scb: I added cast to uint8 in these 4 lines
    //_setBit(dst, dst_offset+2, (uint8)((i >> 1) & 1)); //dst[2] = (uint8)((i >> 1) & 1);  // scb: I added parens in these 3 lines
    //_setBit(dst, dst_offset+1, (uint8)((i >> 2) & 1)); //dst[1] = (uint8)((i >> 2) & 1);
    //_setBit(dst, dst_offset+0, (uint8)((i >> 3) & 1)); //dst[0] = (uint8)((i >> 3) & 1);
}

/**************************************************************************/
static void DesData( uint8 *_data, KeyArrayStruct* pKeyStruct)
{
    uint8           data[8], right[6];
    int             i, j;

    /* Removed by Nader 11/29/07
    
    Permutation(key, _key, 56, MYCONSTPOINTERCAST perm1);

    for (i = 1; i <= 16; i++)
    {
        Permutation(key, NULL, 56, MYCONSTPOINTERCAST perm2);

        if (i != 1 && i != 2 && i != 9 && i != 16)
            Permutation(key, NULL, 56, MYCONSTPOINTERCAST perm2);

        Permutation(pKeyStruct->keys[_encrypt ? i - 1 : 16 - i], key, 48, MYCONSTPOINTERCAST perm3);
    }
    
    End of removal */
    
    SetTP95(1);

    Permutation(data, _data, 64, MYCONSTPOINTERCAST perm4);

    for (i = 1; i <= 16; i++)
    {
        Permutation(right, data + 4, 48, MYCONSTPOINTERCAST perm5);
        Xor(right, pKeyStruct->keys[i - 1], 6);

        for (j = 0; j < 8; j++)
            SBoxes(right, (uint16)(4 * j), right, (uint16)(6 * j), MYCONSTPOINTERCAST sboxes[j]);

        Permutation(right, NULL, 32, MYCONSTPOINTERCAST perm6);
        Xor(right, data, 4);
        Copy(data, data + 4, 4);
        Copy(data + 4, right, 4);
    }
    
    SetTP95(0);

    Copy(_data, data + 4, 4);
    Copy(_data + 4, data, 4);
    Permutation(_data, NULL, 64, MYCONSTPOINTERCAST perm7);
}

/**************************************************************************/
/* Added by Nader 11/29/07                                                */
/* This function is used to initialize the keys                           */
/**************************************************************************/
static void InitPermutationKeys(uint8 *_key, int _encrypt, KeyArrayStruct* pKeyStruct)
{
    uint8           key[8];
    int             i;
    
    Permutation(key, _key, 56, MYCONSTPOINTERCAST perm1);

    for (i = 1; i <= 16; i++)
    {
        Permutation(key, NULL, 56, MYCONSTPOINTERCAST perm2);

        if (i != 1 && i != 2 && i != 9 && i != 16)
            Permutation(key, NULL, 56, MYCONSTPOINTERCAST perm2);

        Permutation(pKeyStruct->keys[_encrypt ? i - 1 : 16 - i], key, 48, MYCONSTPOINTERCAST perm3);
    }
}


// The following is an example of C code that encrypts and decrypts messages
// of variable length, however message length must be a multiple of 8 bytes.
// These procedures can be used for both DES/CBC or DESede/CBC depending of
// the key length provided (8 bytes for DES and 24 bytes for DESede). This
// code is provided as an example only, and is not required for compliance
// with the either DES/CBC or DESede/CBC.

#ifdef WANT_MAIN1
static uint8    des_iv[64] = {
    0,0,1,1,1,0,1,0,0,1,1,1,1,1,0,1,1,1,1,0,0,0,1,0,0,0,1,1,1,0,0,0,
    0,0,1,1,1,0,1,0,0,1,1,1,1,1,0,1,1,1,1,0,0,0,1,0,0,0,1,1,1,0,0,0
};

static uint8    des_key[64] = {
    0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,0,0,
    0,0,0,0,0,1,0,1,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1,0,0,0,0,1,0,0,0
};

static uint8    des_ede_iv[64] = {
    0,1,1,1,0,0,0,1,0,1,1,1,1,1,0,1,1,1,1,0,0,0,1,0,0,0,1,1,1,0,0,0,
    0,1,1,1,0,0,0,1,0,1,1,1,1,1,0,1,1,1,1,0,0,0,1,0,0,0,1,1,1,0,0,0
};

static uint8    des_ede_key[192] = {
    0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,0,0,
    0,0,0,0,0,1,0,1,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1,0,0,0,0,1,0,0,0,
    0,0,0,0,1,0,0,1,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,1,1,0,0,
    0,0,0,0,1,1,0,1,0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,1,0,0,0,1,0,0,0,0,
    0,0,0,1,0,0,0,1,0,0,0,1,0,0,1,0,0,0,0,1,0,0,1,1,0,0,0,1,0,1,0,0,
    0,0,0,1,0,1,0,1,0,0,0,1,0,1,1,0,0,0,0,1,0,1,1,1,0,0,0,1,1,0,0,0
};

static uint8    data[192] = {
    0,1,1,1,1,1,0,1,0,1,0,1,1,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,1,1,1,
    0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,1,0,0,0,1,0,0,1,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,1,0,0,1,0,0,0,1,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,0,0
};
#endif

/***************************************************************************/
/* Modified by Nader 11/29/07. Adding to the function call:                */ 
/* KeyArrayStruct* keys2, KeyArrayStruct *keys3                            */
/***************************************************************************/

static void CbcEncrypt(uint8 *data, int data_length, uint8 *key, int key_length, uint8 *iv, 
                       KeyArrayStruct* keys, KeyArrayStruct* keys2, KeyArrayStruct *keys3, 
                       void (*keepAliveFunctionPtr)(void* p), void* keepAliveParam)
{
	int lgn = data_length;
	
#ifdef TEST_ENCRYPTION_TIMING	
	EncSetupTestPins();
#endif	

        /* Added by Nader 11/29/07 */
        InitPermutationKeys(key,1,keys);
        if (key_length == 24)
        {
            InitPermutationKeys(key+8,0,keys2);
            InitPermutationKeys(key+16,1,keys3);
        }
        /* End of addition by Nader */

        
	while (data_length > 0)
	{
	    if ( keepAliveFunctionPtr != NULL )
	        (*keepAliveFunctionPtr)(keepAliveParam);
	        
	        
		if (data_length == lgn)
			Xor(data, iv, 8);
		else
			Xor(data, data-8, 8);

        SetTP94(1);
		DesData( data, keys);
		if (key_length == 24)
		{
		    SetTP94(0);
			DesData( data, keys2); /* Nader 11/29/07: changed keys to keys2 */
			SetTP94(1);
			DesData( data, keys3); /* Nader 11/29/07: changed keys to keys3 */
		}
		SetTP94(0);
		data += 8;
		data_length -= 8;
	}
}


/***************************************************************************/
/* Modified by Nader 11/29/07. Adding to the function call:                */ 
/* KeyArrayStruct* keys2, KeyArrayStruct *keys3                            */
/***************************************************************************/

static void CbcDecrypt(uint8 *data, int data_length, uint8 *key, int key_length, uint8 *iv, 
                       KeyArrayStruct* keys, KeyArrayStruct* keys2, KeyArrayStruct *keys3,
                       void (*keepAliveFunctionPtr)(void* p), void* keepAliveParam )
{
#ifdef TEST_ENCRYPTION_TIMING	
	EncSetupTestPins();
#endif	

	/* Added by Nader 11/29/07 */
        if (key_length == 24)
        {
            InitPermutationKeys(key+16,0,keys3);
            InitPermutationKeys(key+8,1,keys2);
        }
        InitPermutationKeys(key,0,keys);

        /* End of addition by Nader */

  
        data += data_length - 8;

	while (data_length > 0)
	{
	    if ( keepAliveFunctionPtr != NULL )
	        (*keepAliveFunctionPtr)(keepAliveParam);

		if (key_length == 24)
		{
			DesData( data, keys3); /* Nader 11/29/07: changed keys to keys3 */
			DesData( data, keys2); /* Nader 11/29/07: changed keys to keys2 */
		}
		DesData( data, keys);

		if (data_length > 8)
			Xor(data, data-8, 8);
		else
			Xor(data, iv, 8);

		data_length -= 8;
		data -= 8;
	}
}

// following is code added by scb...

// for now I am going to use the bit method assumed in this file - later I will convert to a method that keeps the bits
// in their original storage - or at least in bytes
#if 0

void ConvertToBits(Unsigned8* data, Unsigned16 length, uint8* bits)
{
    Unsigned8 byte;

    while(length > 0)
    {
        byte = *data++;

        *bits++ = (uint8)((byte >> 7) & 0x01);
        *bits++ = (uint8)((byte >> 6) & 0x01);
        *bits++ = (uint8)((byte >> 5) & 0x01);
        *bits++ = (uint8)((byte >> 4) & 0x01);
        *bits++ = (uint8)((byte >> 3) & 0x01);
        *bits++ = (uint8)((byte >> 2) & 0x01);
        *bits++ = (uint8)((byte >> 1) & 0x01);
        *bits++ = (uint8)((byte >> 0) & 0x01);
        
        length--;
    }
}


void ConvertToBytes(uint8* bits, Unsigned8* data, Unsigned16 length)
{
    Unsigned8 byte;
    
    while(length > 0)
    {
        byte = 0;
        byte |= (Unsigned8)((*bits++) << 7);
        byte |= (Unsigned8)((*bits++) << 6);
        byte |= (Unsigned8)((*bits++) << 5);
        byte |= (Unsigned8)((*bits++) << 4);
        byte |= (Unsigned8)((*bits++) << 3);
        byte |= (Unsigned8)((*bits++) << 2);
        byte |= (Unsigned8)((*bits++) << 1);
        byte |= (Unsigned8)((*bits++) << 0);

        *data++ = byte;
        
        length--;
    }
}
#endif


#define MAX_ENCRYPTED_BYTES   1000 // should be a multiple of 8 bytes
#define MAX_KEY_BYTES           24
#define MAX_IV_BYTES             8

#if 0
uint8 data_bits[CHAR_BIT*MAX_ENCRYPTED_BYTES];   // 8000 bytes !!!!
uint8 key_bits[CHAR_BIT*MAX_KEY_BYTES];
uint8 iv_bits[CHAR_BIT*MAX_IV_BYTES];
#endif


// iv length must be 4 or 8
// key length must be 8 or 24
// dataLength must be a multiple of 8 bytes

/***************************************************************************/
/* Modified by Nader 11/29/07: Adding to the function call:                */
/* Unsigned8* keybuffer962, Unsigned8* keybuffer963                        */
/***************************************************************************/

Boolean C1222CBCEncrypt(Unsigned8* data, Unsigned16 dataLength, Unsigned8* key, 
                        Unsigned16 keyLength, Unsigned8* iv, Unsigned8 ivLength, 
                        Unsigned8* keybuffer, void (*keepAliveFunctionPtr)(void* p), 
                        void* keepAliveParam)
{
    Unsigned8 local_iv[8];
    KeyArrayStruct* pkeys = (KeyArrayStruct*)(keybuffer);

    KeyArrayStruct* pkeys2 = (KeyArrayStruct*)(&keybuffer[96]); /* Added by Nader 11/29/07 */
    KeyArrayStruct* pkeys3 = (KeyArrayStruct*)(&keybuffer[192]); /* Added by Nader 11/29/07 */

    if ( dataLength > MAX_ENCRYPTED_BYTES )
        return FALSE;

    if ( (dataLength % 8) != 0 )
        return FALSE;

    if ( (keyLength != 8) && (keyLength != 24) )
        return FALSE;

    // convert the data, key, and iv to arrays of bits

    //ConvertToBits(data, dataLength, data_bits);
    //ConvertToBits(key, keyLength, key_bits);
    
    if ( ivLength == 4 )
    {
        //ConvertToBits(iv, 4, iv_bits);
        //memcpy(&iv_bits[4*CHAR_BIT], iv_bits, 4*CHAR_BIT);
        memcpy(local_iv, iv, 4);
        memcpy(&local_iv[4], iv, 4);
    }
    else
    {
        //ConvertToBits(iv, MAX_IV_BYTES, iv_bits);
        memcpy(local_iv, iv, 8);
    }
    
    // Modified by Nader: Adding to the function call: pkeys2, pkeys3
    
    CbcEncrypt(data, dataLength, key, keyLength, local_iv, pkeys, pkeys2, pkeys3, keepAliveFunctionPtr, keepAliveParam);
    
    //ConvertToBytes(data_bits, data, dataLength);
    
    return TRUE;
}

/***************************************************************************/
/* Modified by Nader 11/29/07: Adding to the function call:                */
/* Unsigned8* keybuffer962, Unsigned8* keybuffer963                        */
/***************************************************************************/

Boolean C1222CBCDecrypt(Unsigned8* data, Unsigned16 dataLength, Unsigned8* key, Unsigned16 keyLength, 
             Unsigned8* iv, Unsigned8 ivLength, 
             Unsigned8* keybuffer, void (*keepAliveFunctionPtr)(void* p), void* keepAliveParam)
{
    Unsigned8 local_iv[8];
    KeyArrayStruct* pkeys = (KeyArrayStruct*)keybuffer;
    
    KeyArrayStruct* pkeys2 = (KeyArrayStruct*)(&keybuffer[96]); /* Added by Nader 11/29/07 */
    KeyArrayStruct* pkeys3 = (KeyArrayStruct*)(&keybuffer[192]); /* Added by Nader 11/29/07 */
    
    
    if ( dataLength > MAX_ENCRYPTED_BYTES )
        return FALSE;
        
    if ( (dataLength % 8) != 0 )
        return FALSE;

    if ( (keyLength != 8) && (keyLength != 24) )
        return FALSE;
            
    // convert the data, key, and iv to arrays of bits
    
    //ConvertToBits(data, dataLength, data_bits);
    //ConvertToBits(key, keyLength, key_bits);

    if ( ivLength == 4 )
    {
        //ConvertToBits(iv, 4, iv_bits);
        //memcpy(&iv_bits[4*CHAR_BIT], iv_bits, 4*CHAR_BIT);
        memcpy(local_iv, iv, 4);
        memcpy(&local_iv[4], iv, 4);
    }
    else
    {
        //ConvertToBits(iv, MAX_IV_BYTES, iv_bits);
        memcpy(local_iv, iv, 8);
    }
    
    // Modified by Nader: Adding to the function call: pkeys2, pkeys3
    CbcDecrypt(data, dataLength, key, keyLength, local_iv, pkeys, pkeys2, pkeys3, keepAliveFunctionPtr, keepAliveParam);

    //ConvertToBytes(data_bits, data, dataLength);

    return TRUE;
}

#ifdef WANT_MAIN

Unsigned8 hexToBin(char c)
{
	Unsigned8 nibble = 0;

	if ( c >= '0' && c <= '9' )
    	nibble = (Unsigned8)(c - '0');
    else if ( c >= 'a' && c <= 'f' )
    	nibble = (Unsigned8)(c - 'a' + 10);
    else if ( c >= 'A' && c <= 'F' )
    	nibble = (Unsigned8)(c - 'A' + 10);

	return nibble;
}

void ConvertHexToBytes( char* hex, Unsigned16 length, Unsigned8* bytes)
{
	Unsigned8 byte;

	while(length>0)
    {
    	byte = (Unsigned8)(hexToBin(*hex++)<<4);
        byte |= hexToBin(*hex++);

        *bytes++ = byte;
    	length--;
    }
}


Unsigned8 my_data[1000];
Unsigned8 my_key[24];
Unsigned8 my_iv[8];

void main( int argc, char** argv)
{
    // params: data key iv, all in hex

    Unsigned16 dataLength, keyLength, ivLength, ii;
    Unsigned8 keybuffer[96*3];
    char eord;

    if ( argc != 5 )
    {
    	printf("Usage: c1222testcbc data key iv EorD\n  all params in hex\n");
    	printf("       last parameter is 'e' to encrypt or 'd' to decrypt\n");
        exit(0);
	}

    dataLength = (Unsigned16)(strlen(argv[1])/2);
    keyLength = (Unsigned16)(strlen(argv[2])/2);
    ivLength = (Unsigned16)(strlen(argv[3])/2);
    eord = *argv[4];

    ConvertHexToBytes(argv[1], dataLength, my_data);
    ConvertHexToBytes(argv[2], keyLength, my_key);
    ConvertHexToBytes(argv[3], ivLength, my_iv);

    printf("Data = %s\n", argv[1]);
    printf("Key  = %s\n", argv[2]);
    printf("IV   = %s\n", argv[3]);

    if ( eord == 'e' || eord == 'E' )
    {
        if ( C1222CBCEncrypt(my_data, dataLength, my_key, keyLength, my_iv, ivLength, keybuffer, 0, 0) )
        {
    	    printf("EData= ");
            for ( ii=0; ii<dataLength; ii++ )
        	    printf("%02X", my_data[ii]);
    	    printf("\n");

            if ( C1222CBCDecrypt(my_data, dataLength, my_key, keyLength, my_iv, ivLength, keybuffer, 0, 0) )
            {
    		    printf("Data = ");
        	    for ( ii=0; ii<dataLength; ii++ )
        		    printf("%02X", my_data[ii]);
    		    printf("\n");
            }
            else
        	    printf("Decrypt failed\n");
        }
        else
    	    printf("Encrypt failed\n");
    }
    else
    {
            if ( C1222CBCDecrypt(my_data, dataLength, my_key, keyLength, my_iv, ivLength, keybuffer, 0, 0) )
            {
    		    printf("DData= ");
        	    for ( ii=0; ii<dataLength; ii++ )
        		    printf("%02X", my_data[ii]);
    		    printf("\n");
            }
            else
        	    printf("Decrypt failed\n");
    }
}
#endif



#ifdef WANT_MAIN1
/**************************************************************************/
void Print(char *text, uint8 *data, int data_length)
{
   int i, byte;

   printf(text);

   for (i=0, byte=0; i<data_length; i++)
   {
      byte <<= 1;
      byte |= data[i];
      if ((i % 8) == 7)
      {
         printf("%02X", byte);
         byte = 0;
      }
   }

   printf("\n");
}

/**************************************************************************/
int main()
{
    Print("Key: ", des_key, sizeof(des_key));
    Print("IV: ", des_iv, sizeof(des_iv));
    Print("Plain text: ", data, sizeof(data));
    CbcEncrypt(data, sizeof(data), des_key, sizeof(des_key), des_iv, keybuffer96);
    Print("Encrypted  text: ", data, sizeof(data));
    CbcDecrypt(data, sizeof(data), des_key, sizeof(des_key), des_iv, keybuffer96);
    Print("Plain text: ", data, sizeof(data));

    Print("\nKey: ", des_ede_key, sizeof(des_ede_key));
    Print("IV: ", des_ede_iv, sizeof(des_ede_iv));
    Print("Plain text: ", data, sizeof(data));
    CbcEncrypt(data, sizeof(data), des_ede_key, sizeof(des_ede_key), des_ede_iv, keybuffer96);
    Print("Encrypted  text: ", data, sizeof(data));
    CbcDecrypt(data, sizeof(data), des_ede_key, sizeof(des_ede_key), des_ede_iv, keybuffer96);
    Print("Plain text: ", data, sizeof(data));
    return 0;
}
#endif

