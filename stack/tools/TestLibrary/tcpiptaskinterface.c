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

#define C1222_WANT_CMVR_DESCRIPTIONS  // for c1222 message validation result descriptions


#include "c1222stack.h"

#ifndef ELEMENTS
#define ELEMENTS(x) (sizeof(x)/sizeof((x)[0]))
#endif


#include "windows.h"


extern void LogAndPrintf(char* buffer);
extern void NetworkCommLogRx(Unsigned8* buffer, Unsigned16 length);

#ifdef WANT_RELAY
extern Boolean findNativeAddress(C1222ApTitle* pApTitle, Unsigned8** pNativeAddress, Unsigned8* pNativeAddressLength);
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Implementation of message passing to/from the tcp ip thread
////////////////////////////////////////////////////////////////////////////////////////

Boolean notSoNice = TRUE;


void SystemIdleNice(void)
{
	MSG msg;
	
	if ( notSoNice )
	    return;

	while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
	{
		if (msg.message != WM_TIMER)
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}
}

void DoSleep(Unsigned32 delay)
{
	SystemIdleNice();
	Sleep(delay);
    SystemIdleNice();
}

#ifdef WANT_TCPIP

Unsigned8 g_receiveMessageBuffer[1000];
Unsigned16 g_receiveIndex = 0;
unsigned char g_receiveReady = FALSE;




Boolean CMProcessReceivedStreamBytes(Unsigned8* buffer, Unsigned16 length)
{
    static unsigned char state = 0;
    Unsigned8 c;
    static Unsigned16 dataLength = 0;
    Unsigned8 sizeofLength;
    
    while ( length )
    {
        c = *buffer++;
        length--;
        
        switch(state)
        {
        case 0: // initial state
            dataLength = 0;
            sizeofLength = 0;
            g_receiveIndex = 0;
            if ( c == 0x60 )
                state = 1;
            break;
        case 1: // parsing length bytes
            if ( sizeofLength == 0 )
            {
                if ( c < 128 )
                {
                    dataLength = c;
                    state = 2;
                }
                else
                {
                    sizeofLength = (Unsigned8)(c & 0x7F);
                }
            }
            else 
            {
                dataLength *= (Unsigned16)256;
                dataLength += c;
                
                sizeofLength--;
                
                if ( sizeofLength == 0 )
                {
                    state = 2;
                    
                    if ( dataLength >= sizeof(g_receiveMessageBuffer) )
                    {
                        LogAndPrintf("Network message too long\n");
                        g_receiveReady = FALSE;
                        state = 0;
                        return FALSE;
                    }
                }
            }
            break;
        case 2:
            g_receiveMessageBuffer[g_receiveIndex++] = c;
            dataLength--;
            
            if ( dataLength == 0 )
            {
                g_receiveReady = TRUE;
                state = 0; 
                return TRUE;
            }
            break;
            
        }
    }
    
    return FALSE;
}



Boolean doCMProcessReceivedStreamBytes(char* recvbuf, int bytesRecv)
{
    return CMProcessReceivedStreamBytes((Unsigned8*)recvbuf, (Unsigned16)bytesRecv);
}




unsigned char g_sendBuffer[1100];
Unsigned16 g_sendLength = 0;
unsigned char g_sendReady = FALSE;

int StartSendToClient(Unsigned8* buffer, Unsigned16 length)
{
    int totalLength = length;
    
    while(length > sizeof(g_sendBuffer) )
    {
        memcpy(g_sendBuffer, buffer, sizeof(g_sendBuffer));
        g_sendLength = sizeof(g_sendBuffer);
        g_sendReady = TRUE;
        // wait for send ready to go false again
        while( g_sendReady )
        {
            DoSleep(10);
        }
        buffer += sizeof(g_sendBuffer);
        length -= (Unsigned16)sizeof(g_sendBuffer);
    }
    
    memcpy(g_sendBuffer, buffer, length);
    g_sendLength = length;
    return totalLength;
}

int SendToClient(Unsigned8* buffer, Unsigned16 length)
{
    Unsigned16 addedLength;
    int totalLength = length;
    
    while( (g_sendLength+length) > sizeof(g_sendBuffer) )
    {
        addedLength = (Unsigned16)(sizeof(g_sendBuffer) - g_sendLength);
        memcpy(&g_sendBuffer[g_sendLength], buffer, addedLength);
        buffer += addedLength;
        length -= addedLength;
        g_sendLength += addedLength;
        g_sendReady = TRUE;
        
        while( g_sendReady )
        {
            DoSleep(10);
        }
        
        g_sendLength = 0;
    }
    
    memcpy(&g_sendBuffer[g_sendLength], buffer, length);
    g_sendLength += length;
    g_sendReady = TRUE;
    return totalLength;
}




Unsigned8 address[] = {0,0,0,0,0,0};

Boolean ImaServer = FALSE;
Unsigned32 lastHostRequest = 0;

Unsigned32 GetLastHostRequestTime(void)
{
    return lastHostRequest;
}

Boolean Get_ServerState(void)
{
    return ImaServer;
}

void Set_ServerState(Boolean b)
{
    ImaServer = b;
}






// new versions of uid encode decode
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


            if ( term >= 1000000000 )
                sprintf(mybuffer,"%lu%0lu", term/1000000000, term%1000000000);
            else
                sprintf(mybuffer, "%lu", (unsigned long)term);
            strcat(text, mybuffer);
            term = 0;
            inhibitdot = FALSE;
        }
    }
}

// end of new versions of uid encode decode




char* reportApTitle(Unsigned8* aptitle)
{
    static char buffer[200]; // max length is 60 bytes currently
    
    C1222ApTitleToText(aptitle, buffer);
        
    return buffer;
}



char* reportAddress(Unsigned8* address, Unsigned8 length)
{
    static char buffer[100];
    char mybuffer[10];
    Unsigned8 ii;
    
    if ( length == 6 )
    {
        sprintf(buffer, "uid=%d mac=%9ld", address[4], *(Unsigned32*)address);
    }
    else
    {    
        strcpy(buffer, "");
    
        for ( ii=0; ii<length; ii++ )
        {
            sprintf(mybuffer, " %02X", address[ii]);
            strcat(buffer, mybuffer);
        }
    }
    
    return buffer;
}


void LogAndPrintfCMVR( char* text, C1222MessageValidationResult result)
{
    char message[100];
    
    LogAndPrintf(text);
    if ( result < ELEMENTS(g_C1222MessageValidationResult_Descriptions) )
        LogAndPrintf((char*)g_C1222MessageValidationResult_Descriptions[result]);
    else
    {
        sprintf(message, "Invalid result = %u\n", result);
        LogAndPrintf(message);
    }
    LogAndPrintf("\n");
}


Boolean HandleMessageFromClient(C1222Stack* pStack)
{
    ACSE_Message message;
    Boolean messageSent = FALSE;
    C1222MessageValidationResult result;
    
#ifdef WANT_RELAY    
    C1222ApTitle apTitle;
    Unsigned8* nativeAddress;
    Unsigned8 nativeAddressLength;
    char mymessage[500];
#endif    
    

    if ( g_receiveReady && C1222Stack_IsSendMessageComplete(pStack) )
    {        
        message.buffer = g_receiveMessageBuffer;
        message.length = g_receiveIndex;
        
        // need to set the message standardVersion
        
        if ( ACSE_Message_IsMarch2006FormatMessage(&message) && ACSE_Message_IsValidFormat2006(&message) )
        {
            ACSE_Message_SetStandardVersion(&message, CSV_MARCH2006);
            LogAndPrintf("Message is march 2006 version\n");
        }
        
        else if ( ACSE_Message_IsValidFormat2008(&message) )
        {
            ACSE_Message_SetStandardVersion(&message, CSV_2008);
            LogAndPrintf("Message is 2008 version\n");
        }
        
        else
        {
            // return an error
            // count invalid messages
            LogAndPrintf("Message is not a valid version\n");
            
            result = ACSE_Message_CheckFormat2006(&message);
            LogAndPrintfCMVR("  2006 result = ", result);

            result = ACSE_Message_CheckFormat2008(&message);
            LogAndPrintfCMVR("  2008 result = ", result);
              
        }
        
        NetworkCommLogRx((unsigned char*)g_receiveMessageBuffer, g_receiveIndex);
        
        ImaServer = TRUE;
        lastHostRequest = GetTickCount();
        
#ifdef WANT_RELAY
        // the source native address is zero at the moment
        
        if ( ACSE_Message_GetCalledApTitle(&message, &apTitle) )
        {
            if ( findNativeAddress(&apTitle, &nativeAddress, &nativeAddressLength) )
            {
                sprintf(mymessage,"Sending message to address (%s) for called aptitle (%s)\n", reportAddress(nativeAddress, nativeAddressLength), reportApTitle(apTitle.buffer));
                LogAndPrintf(mymessage);
                messageSent = C1222Stack_SendMessage(pStack, &message, nativeAddress, nativeAddressLength);
            }
            else
            {
                sprintf(mymessage,"Sending message to address 0 - called aptitle (%s) not found\n", reportApTitle(apTitle.buffer));
                LogAndPrintf(mymessage);
                
                messageSent = C1222Stack_SendMessage(pStack, &message, address, 6); // hope for the best
            }
        }
        else
            LogAndPrintf("no called aptitle in the message - can't send to stack\n");
            
#else        
        // the source native address is zero at the moment
        
        messageSent = C1222Stack_SendMessage(pStack, &message, address, 0);
#endif        
        
        g_receiveReady = FALSE;
    }

    return messageSent;
}

#endif  // WANT_TCPIP




