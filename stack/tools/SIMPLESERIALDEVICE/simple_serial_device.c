// CDDL HEADER START
//*************************************************************************************************
//                                     Copyright © 2006-2008
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

#define C1222_WANT_EVENTID_TEXT

#include "c1222.h"
#include "c1222stack.h"
#include "c1222misc.h"
#include "c1219table.h"
#include "c1222response.h"
#include "c1222server.h"
#include "c1222event.h"
#include "c1222al_receivehistory.h"

//#include "windows.h"
#include <winsock2.h>
#include <process.h>

void logTxt(char* txt);
void logInt(char* txt, int ival);

void SystemIdleNice(void);



void InitializeLogCriticalSection(void);

extern void RegisterSetLogs(C1222Stack* pStack);


extern void RegisterOpenLog(void);


extern void DoSleep(Unsigned32 delay);

extern Boolean simulateHost(ACSE_Message* pMsg, C1222Stack* pStack, Boolean * pMessageSent);

extern void initTables(void);


C1222Stack registerStack;
C1222Stack commModuleStack;
C1219TableServer registerTableServer;

#ifdef ENABLE_LOCAL_PORT_ROUTER
C1222LocalPortRouter registerLocalPortRouter;
C1222LocalPortRouter commModuleLocalPortRouter;
#endif // ENABLE_LOCAL_PORT_ROUTER

C1222ReceiveHistory receiveHistory;

C1222TransportConfig config = 
             {
    TRUE, //Boolean passthroughMode;
    0x01, //Unsigned8 linkControlByte; (enabled but nothing else)
    0x20, //Unsigned8 nodeType;
    {0x01, 0x02, 0x03, 0x04}, //Unsigned8 deviceClass[C1222_DEVICE_CLASS_LENGTH];
    {0x06, 18, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12}, //Unsigned8 esn[C1222_ESN_LENGTH];
    4, //Unsigned8 nativeAddressLength;
    {0x12, 0x34, 0x56, 0x78, 0, 0}, //Unsigned8 nativeAddress[C1222_NATIVE_ADDRESS_LENGTH];
    0, //Unsigned8 broadcastAddressLength;
    {0, 0, 0, 0, 0, 0}, //Unsigned8 broadcastAddress[C1222_NATIVE_ADDRESS_LENGTH];
    4, //Unsigned8 relayAddressLength;
    {0, 0, 0, 1, 0, 0}, //Unsigned8 relayNativeAddress[C1222_NATIVE_ADDRESS_LENGTH];
    {0x0d, 0x00, 0x00, 0x00,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //Unsigned8 nodeApTitle[C1222_APTITLE_LENGTH];
    {0x0d, 0x00, 0x00, 0x00,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},//Unsigned8 masterRelayApTitle[C1222_APTITLE_LENGTH];
    FALSE, //Boolean wantClientResponseControl;
    0, //Unsigned8 numberOfRetries;
    0, //Unsigned16 responseTimeout;                
             };








Unsigned8 g_myPort = 6;


// register thread

#ifndef C1222_COMM_MODULE

Unsigned32 registerCounter = 0;


C1222Server registerServer;



static Unsigned8 c1222RegisterMessageBuffer[MAX_ACSE_MESSAGE_LENGTH];

Unsigned32 messageSentCount = 0;
Unsigned32 messageNotSentCount = 0;

void RegisterThread(void* param)
{
    ACSE_Message* pMsg;
    char myBuffer[100];
    Boolean messageSent = FALSE;

    (void)param;

    RegisterOpenLog();
    
    initTables();

    #ifdef ENABLE_LOCAL_PORT_ROUTER
	C1222Stack_Init(&registerStack, &registerLocalPortRouter, g_myPort, FALSE, 0, FALSE, FALSE);
    #else
    C1222Stack_Init(&registerStack, 0, g_myPort, FALSE, 0, FALSE, FALSE);
	#endif // ENABLE_LOCAL_PORT_ROUTER

    RegisterSetLogs(&registerStack);
    
    C1222Stack_EnableLog(&registerStack, TRUE);

    C1222Stack_SetConfiguration(&registerStack, &config);
    
    C1222Stack_SetRegistrationMaxRetryFactor(&registerStack, 0xFF);
    
    C1222Stack_SetRegistrationRetryLimitsFormat1(&registerStack, 0, 1);
    
    C1222Server_Init(&registerServer, &registerStack, &registerTableServer, c1222RegisterMessageBuffer);
    
    C1222ALReceiveHistory_Init(&receiveHistory, &registerStack.Xapplication);

    DoSleep(2000);

    while(1)
    {
    	registerCounter++;
    	if ( (registerCounter%79) == 0 )
        {
    	    sprintf(myBuffer,"Reg: %ld, R=%d, TS=%d, AS=%d, RP=%ld, TP=%ld\n", registerCounter, registerStack.Xtransport.counters.numberOfResets,
    	                               registerStack.Xtransport.status.state,
    	                               registerStack.Xapplication.status.state,
                                       registerStack.Xdatalink.counters.numberPacketsReceived,
                                       registerStack.Xdatalink.counters.numberPacketsSent);
            //logTxt(myBuffer);
            printf("%s", myBuffer);
        }
        
        
    	C1222Stack_Run(&registerStack);
    	
    	if ( messageSent && C1222Stack_IsSendMessageComplete(&registerStack) )
    	{
    	    messageSent = FALSE;
    	}
    	

        if ( !messageSent && C1222Stack_IsMessageAvailable(&registerStack) )
        {
            pMsg = C1222Stack_GetReceivedMessage(&registerStack);
            
            if ( C1222ALReceiveHistory_IsMessageAlreadyReceived(&receiveHistory, pMsg) )
            {
                C1222Stack_NoteReceivedMessageHandlingIsComplete(&registerStack);
            }
            else
            {   int ii;

                for ( ii=0; ii<pMsg->length; ii++ )
                {
                    if ( ii%16 == 0 )
                        printf("\n");
                    printf(" %02x", pMsg->buffer[ii]);
                }
                printf("\n");
                            
                C1222Server_ProcessMessage(&registerServer, pMsg, &messageSent, FALSE);
            
                if ( messageSent )
            	    messageSentCount++;
        	    else
            	    messageNotSentCount++;            
            }
        }
        
        if ( registerCounter == 500 )
            registerStack.Xapplication.status.timeToRegister = TRUE;

    	DoSleep(10);
    }
}

#endif


// comm module and host implementation


Unsigned32 receivedMessages = 0;
Unsigned32 receivedRegistrations = 0;








Boolean g_wantLogNetworkComm = FALSE;

unsigned long mainCount = 0;


void main(int argc, char** argv)
{
	//Unsigned32 params[3];
    //unsigned long threadp1;
 
	Unsigned32 start, next;

    g_myPort = 6;
    if ( argc > 1 )
    {
        g_myPort = (Unsigned8)atoi(argv[1]);
        if ( g_myPort < 1 || g_myPort > 10 )
        {
            printf("Bad port id\n");
            return;
        }
    }

    srand(GetTickCount()&0xFFFF);
    
    InitializeLogCriticalSection();


    #ifdef ENABLE_LOCAL_PORT_ROUTER
    C1222LocalPortRouter_Init(&registerLocalPortRouter);
    #endif

    //CreateThread( NULL, 20000, (LPTHREAD_START_ROUTINE)RegisterThread, &params[0], 0, &threadp1);
    
    _beginthread(RegisterThread, 0, NULL);
    

    start = GetTickCount();

    while(1)
    {
        mainCount++;
        
        
    	DoSleep(50);
        next = GetTickCount();

    	C1222Misc_IsrIncrementFreeRunningTime((Unsigned16)(next-start));
        start = next;
	}
}



void C1222EnterCriticalRegion(void)
{
}

void C1222LeaveCriticalRegion(void)
{
}

