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



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "c1222aptitle.h"

#define TRUE 1
#define FALSE 0

typedef unsigned __int64 UINT64;


unsigned short encodeTerm(UINT64 term, unsigned char* apTitle, unsigned short length)
{
    // returns length appropriate to put in aptitle[1]
    unsigned char forced = FALSE;
    int ii;
    unsigned char byte;

    apTitle += length + 2;

    // ii    =  1,  8, 15, 22, 29, 36, 43, 50, 57
    // 64-ii = 63, 56, 49, 42, 35, 28, 21, 14,  7

    for ( ii=1; ii<64; ii+=7 )
    {
        byte = (unsigned char)(term >> (64-ii));

        if ( forced || byte )
        {
            *apTitle++ = (unsigned char)(0x80 | byte );
            forced = TRUE;
            length++;
        }

        // clear top bits that we just encoded
        term <<= ii;
        term >>= ii;
    }

    byte = (unsigned char) term;
    *apTitle = byte;

    length++;
    return length;
}

void TextToC1222ApTitle(char* text, unsigned char* apTitle)
{
    // assume the length will fit in 1 byte

    char c;
    UINT64 term = 0;
    unsigned short length = 0;
    unsigned short termIndex = 0;
    unsigned char isAbsolute = FALSE;
    UINT64 term0;
    
    if ( text[0] == 0 )
    {
        apTitle[0] = 0;
        return;
    }
    
    if ( text[0] == '.' )
    {
        apTitle[0] = 0x0d;
        text++;
    }
    else
    {
        apTitle[0] = 0x06;
        isAbsolute = TRUE;
    }
        
    apTitle[1] = 0x00;  // fix it later
    
    while( (c=*text++) != 0 )
    {
        if ( c == '.' )
        {
            if ( isAbsolute )
            {
                if ( termIndex == 0 )
                    term0 = term;
                else if ( termIndex == 1 )
                    term += 40*term0;
            }
            
            if ( (termIndex > 0) || (!isAbsolute) )
                length = encodeTerm(term,apTitle, length);
                
            term = 0;
            termIndex++;
        }
        else
        {
            term = term*10 + (c-'0');
        }
    }
    
    apTitle[1] = encodeTerm(term,apTitle, length);
}

void C1222ApTitleToText(unsigned char* apTitle, char* text)
{
    unsigned short length;
    UINT64 term = 0;
    char mybuffer[50];
    unsigned char inhibitdot = TRUE;
    unsigned char byteValue;
    
    text[0] = 0;
    
    if ( apTitle[0] == 0 )
        return;
    
    length = apTitle[1];
    if ( length & 0x80 )
        return;
        
    if ( apTitle[0] == 0x0d || apTitle[0] == 0x80 )
    {
        // relative - start with a dot
        inhibitdot = FALSE;
    }
    
    apTitle += 2;
    
    if ( inhibitdot ) // absolute
    {
        byteValue = *apTitle++;
        sprintf(mybuffer,"%lu.%lu.", byteValue/40, byteValue%40);
        strcat(text, mybuffer);
        length--;
    }
    
    while(length--)
    {
        // get the next term
        byteValue = *apTitle++;
        
        term *= 128;
        term += byteValue & 0x7F;
        
        if ( (byteValue & 0x80) == 0)
        {
            if ( !inhibitdot )
                strcat(text,".");


            if ( term >= 1000000000ui64 )
                sprintf(mybuffer,"%lu%0lu", (unsigned long)(term/(1000000000ui64)), (unsigned long)(term%1000000000ui64));
            else
                sprintf(mybuffer, "%lu", (unsigned long)term);
            strcat(text, mybuffer);
            term = 0;
            inhibitdot = FALSE;
        }
    }
}

void showHex(unsigned char* hexBytes, int length)
{
    
        for ( int ii=0; ii<length; ii++ )
        {
            printf(" %02X", hexBytes[ii]);
        }
        
        printf("\n");
}


unsigned char hexToBin(char c)
{
	unsigned char nibble = 0;

	if ( c >= '0' && c <= '9' )
    	nibble = (unsigned char)(c - '0');
    else if ( c >= 'a' && c <= 'f' )
    	nibble = (unsigned char)(c - 'a' + 10);
    else if ( c >= 'A' && c <= 'F' )
    	nibble = (unsigned char)(c - 'A' + 10);

	return nibble;
}


void ConvertHexToBytes( char* hex, unsigned short length, unsigned char* bytes)
{
	unsigned char byte;

	while(length>0)
    {
    	byte = (unsigned char)(hexToBin(*hex++)<<4);
        byte |= hexToBin(*hex++);

        *bytes++ = byte;
    	length--;
    }
}


void printhelp(void)
{
        printf("Usage: c1222aptitle 1 aptitletext         shows aptitle in hex\n");
        printf("       c1222aptitle 2 hexaptitlebytes     shows aptitle in text\n");
        printf("       c1222aptitle 3 hexaptitlebytes     converts to absolute aptitle (2006 format)\n");
        printf("       c1222aptitle 4 hexaptitlebytes     converts to relative aptitle\n");
        printf("       [ c1222aptitle 5 hexaptitlebytes     is aptitle multicast (no longer supported) ]\n");
        printf("       c1222aptitle 6 hexaptitlebytes myhexaptitlebytes is aptitle broadcast to me\n");
        printf("       c1222aptitle 7 hexaptitlebytes     converts to absolute aptitle (2008 format)\n");
        printf("\n");
        printf("Note: table aptitle form is used for output (relative 0x0d), not ACSE (relative 0x80).\n)");
}



void main(int argc, char** argv)
{
    unsigned char hexBytes[40];
    unsigned char hexBytes2[40];
    char text[200];
    unsigned short length;
    int mode;
    C1222ApTitle aptitle;
    C1222ApTitle aptitle2;
    unsigned char buffer2[40];
    
    if ( argc != 3 && argc != 4 )
    {
        printhelp();
    }

    else
    {
    	mode = atoi(argv[1]);

    	switch(mode)
    	{
        case 1:
            memset(hexBytes, 0, sizeof(hexBytes));
            TextToC1222ApTitle(argv[2], hexBytes);
            showHex(hexBytes,20);
            break;
        case 2:
            length = (unsigned short)(strlen(argv[2])/2);
            ConvertHexToBytes(argv[2], length, hexBytes);
            C1222ApTitleToText(hexBytes, text);
            printf("%s\n", text);
            break;
        case 3:
            length = (unsigned short)(strlen(argv[2])/2);
            ConvertHexToBytes(argv[2], length, hexBytes);
            C1222ApTitle_Construct(&aptitle, hexBytes, length);
            aptitle.length = C1222ApTitle_GetLength(&aptitle);
            memset(buffer2,0,sizeof(buffer2));
            C1222ApTitle_Construct(&aptitle2, buffer2, sizeof(buffer2));
            if ( C1222ApTitle_MakeAbsoluteFrom(&aptitle2, &aptitle, TRUE) )
            {
                showHex(buffer2,20);
                C1222ApTitleToText(buffer2, text);
                printf("%s\n", text);
            }
            else
                printf("can't make absolute aptitle\n");
            break;	
        case 4:
            length = (unsigned short)(strlen(argv[2])/2);
            ConvertHexToBytes(argv[2], length, hexBytes);
            C1222ApTitle_Construct(&aptitle, hexBytes, length);
            aptitle.length = C1222ApTitle_GetLength(&aptitle);
            memset(buffer2,0,sizeof(buffer2));
            C1222ApTitle_Construct(&aptitle2, buffer2, sizeof(buffer2));
            if ( C1222ApTitle_MakeRelativeFrom(&aptitle2, &aptitle) )
            {
                showHex(buffer2,20);
                C1222ApTitleToText(buffer2, text);
                printf("%s\n", text);

            }
            else
                printf("can't make absolute aptitle\n");
            break;
#if 0 // no longer supported
        case 5: // is this a multicast
            length = (unsigned short)(strlen(argv[2])/2);
            ConvertHexToBytes(argv[2], length, hexBytes);
            C1222ApTitleToText(hexBytes, text);

            C1222ApTitle_Construct(&aptitle, hexBytes, length);
            aptitle.length = C1222ApTitle_GetLength(&aptitle);

            if ( C1222ApTitle_IsAMulticastAddress(&aptitle) )
                printf("%s is a multicast address\n", text);
            else
                printf("%s is not a multicast address\n", text);
            break;
#endif

        case 6: // is a broadcast to me
            length = (unsigned short)(strlen(argv[2])/2);
            ConvertHexToBytes(argv[2], length, hexBytes);
            C1222ApTitleToText(hexBytes, text);
            printf("%s ", text);
            
            C1222ApTitle_Construct(&aptitle, hexBytes, length);
            aptitle.length = C1222ApTitle_GetLength(&aptitle);
            
            length = (unsigned short)(strlen(argv[3])/2);
            ConvertHexToBytes(argv[3], length, hexBytes2);
            C1222ApTitleToText(hexBytes2, text);

            C1222ApTitle_Construct(&aptitle2, hexBytes2, length);
            aptitle2.length = C1222ApTitle_GetLength(&aptitle2);
            
            if ( C1222ApTitle_IsRelative(&aptitle) && C1222ApTitle_IsRelative(&aptitle2) )
            {
                if ( C1222ApTitle_IsBroadcastToMe(&aptitle, &aptitle2) )
                    printf("is a broadcast to %s\n", text);
                else
                    printf("is not a broadcast to %s\n", text);
            }
            else if ( C1222ApTitle_IsRelative(&aptitle) )
                printf(" is ok but %s is not relative\n", text);
            else
                printf(" is not relative\n");
                 
            break;
            
        case 7:
            length = (unsigned short)(strlen(argv[2])/2);
            ConvertHexToBytes(argv[2], length, hexBytes);
            C1222ApTitle_Construct(&aptitle, hexBytes, length);
            aptitle.length = C1222ApTitle_GetLength(&aptitle);
            memset(buffer2,0,sizeof(buffer2));
            C1222ApTitle_Construct(&aptitle2, buffer2, sizeof(buffer2));
            if ( C1222ApTitle_MakeAbsoluteFrom(&aptitle2, &aptitle, FALSE) )
            {
                showHex(buffer2,20);
                C1222ApTitleToText(buffer2, text);
                printf("%s\n", text);
            }
            else
                printf("can't make absolute aptitle\n");
            break;	

        default:
            printhelp();
            break;
    	}
    }
}



