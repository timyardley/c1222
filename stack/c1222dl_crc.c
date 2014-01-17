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


// the crc16 and crc routines in this module were copied from ansiC1222-wg1final.doc annex I.2, 
// version March 7, 2006. (and lightly modified to avoid warnings etc)

// The following is an example of C code which calculates the <crc> field in a manner compliant 
// with C12.22 layer 2. This code is provided as an example only.

#ifdef WANT_MAIN
#include <stdio.h>
#include <string.h>
#endif

#include "c1222.h"
#ifndef WANT_MAIN
#include "c1222environment.h"
#endif

#include "c1222dl_crc.h"




static unsigned short crc16(unsigned char octet, unsigned short crc);
static unsigned short crc(int size, unsigned char *packet);




static unsigned short crc16(unsigned char octet, unsigned short crc)
{
	int i;

	for (i = 8; i; i--)
	{
		if (crc & 0x0001)
		{
			crc >>= 1;
			if (octet & 0x01)
				crc |= 0x8000;
			crc = (unsigned short)(crc ^ 0x8408);   /* 0x1021 inverted = 1000 0100 0000 1000 */
			octet >>= 1;
		}
		else
		{
			crc >>= 1;
			if (octet & 0x01)
				crc |= 0x8000;
			octet >>= 1;
		}
	}

	return crc;
}




static unsigned short crc(int size, unsigned char *packet)
{
	int i;
	unsigned short crc;

	crc = (unsigned short)((~packet[1] << 8) | (~packet[0] & 0xFF));

	for (i=2 ; i<size; i++)
		crc = crc16(packet[i], crc);

	crc = crc16(0x00, crc);
	crc = crc16(0x00, crc);
	crc = (unsigned short)(~crc);
	//crc = (unsigned short)((crc >> 8) | (crc << 8));  // this line was in the standard but I am reversing the bytes externally so it is not needed

	return crc;
}



// this routine is used by the datalink of c12.22 support to calculate crc's

Unsigned16 C1222DL_CalcCRC(Unsigned8* buffer, Unsigned16 length)
{
    return (Unsigned16)crc((int)length, buffer);
}



// the next 3 routines are used by the application layer of c12.22 support to calculate
// crc's.  In that case the crc's generally are on non-contiguous areas in a message

// the length for the first call must be >= 2

Unsigned16 C1222AL_StartCRC(Unsigned8* packet, Unsigned16 length)
{
	unsigned short crc;

	crc = (unsigned short)((~packet[1] << 8) | (~packet[0] & 0xFF));

    if ( length > 2 )
        crc = C1222AL_AddBufferToCRC(crc, &packet[2], (Unsigned16)(length-2));
   
    return crc;
}



Unsigned16 C1222AL_AddBufferToCRC(Unsigned16 crc, Unsigned8* packet, Unsigned16 length)
{
	int i;

	for (i=0 ; i<length; i++)
		crc = crc16(packet[i], crc);

    return crc;
}



Unsigned16 C1222AL_FinishCRC(Unsigned16 crc)
{
    crc = crc16(0x00, crc);
    crc = crc16(0x00, crc);
    
	crc = (unsigned short)(~crc);
	//crc = (unsigned short)((crc >> 8) | (crc << 8));  // return the actual crc and handle byte order in calling code

	return crc;    
}



// test routines...

#ifdef WANT_MAIN1
int main()
{
	unsigned char packet[] =  { 0xEE, 0x00, 0x00, 0x00, 0x00, 0x01, 0x20 };

	printf("Crc   = %04x \n", crc(sizeof(packet), packet));
	return 0;
}
#endif


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

Unsigned8 myBuffer[1000];
Unsigned16 totalLength;

int main(int argc, char** argv)
{
    int ii;
    Unsigned16 crc1;
    Unsigned16 crc2;
    Unsigned16 length;
    
    totalLength = 0;

    if ( argc < 2 )
    {
        printf("Usage: testcrc hexString+\n");
        return 0;
    }
    
    for ( ii=1; ii<argc; ii++ )
    {
        length = (Unsigned16)(strlen(argv[ii])/2);
        ConvertHexToBytes( argv[ii], length, &myBuffer[totalLength]);
        if ( ii==1 )
            crc1 = C1222AL_StartCRC(&myBuffer[totalLength], length);
        else
            crc1 = C1222AL_AddBufferToCRC(crc1, &myBuffer[totalLength], length);
            
        totalLength += length;
    }
    
    crc1 = C1222AL_FinishCRC(crc1);
    crc2 = C1222DL_CalcCRC(myBuffer, totalLength);

	printf("AL Crc   = %04X \n", crc1);
	printf("DL Crc   = %04X \n", crc2);
	return 0;
}
#endif

