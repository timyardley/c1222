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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



typedef unsigned char Unsigned8;
typedef unsigned short Unsigned16;






static Unsigned8 getchecksum(Unsigned8* buffer, Unsigned16 length)
{
    Unsigned16 ii;
    Unsigned8 sum = 0;
    
    for ( ii=0; ii<length; ii++ )
        sum += buffer[ii];
        
    return (Unsigned8)(~sum + 1);
}




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
    Unsigned8 checksum;
    Unsigned16 length;
    
    totalLength = 0;

    if ( argc < 2 )
    {
        printf("Usage: getchecksum hexString+\n");
        return 0;
    }
    
    for ( ii=1; ii<argc; ii++ )
    {
        length = (Unsigned16)(strlen(argv[ii])/2);
        ConvertHexToBytes( argv[ii], length, &myBuffer[totalLength]);

        totalLength += length;
    }

    checksum = getchecksum(myBuffer, totalLength);
    

	printf("checksum   = %02X hex or %u decimal\n", checksum, checksum);
	return 0;
}
