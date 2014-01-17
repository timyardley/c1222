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

//#define C1222_WANT_EVENTID_TEXT

#include "c1222.h"
#include "c1222stack.h"
#include "c1222misc.h"
#include "c1222response.h"
#include "uart.h"
#include "c1222al_receivehistory.h"


#include "windows.h"
//#include <winsock2.h>
#include <process.h>



void SystemIdleNice(void);



void InitializeLogCriticalSection(void);


extern void CommModuleSetLogs(C1222Stack* pStack);


extern void CommModuleOpenLog(void);

extern int SendToClient(Unsigned8* buffer, Unsigned16 length);
extern Boolean HandleMessageFromClient(C1222Stack* pStack);
extern int StartSendToClient(Unsigned8* buffer, Unsigned16 length);
extern void DoSleep(Unsigned32 delay);

Boolean CMsimulateHost(ACSE_Message* pMsg, C1222Stack* pStack, Boolean * pMessageSent, Boolean sendRegistrationToNetwork);



extern void Set_ServerState(Boolean b);
extern Boolean Get_ServerState(void);
extern Unsigned32 GetLastHostRequestTime(void);

extern void NetworkCommOpenLog(void);
extern void NetworkCommLogTx(Unsigned8* part1, Unsigned16 part1Length, Unsigned8* part2, Unsigned16 part2Length);




C1222Stack commModuleStack;


#ifdef ENABLE_LOCAL_PORT_ROUTER
C1222LocalPortRouter commModuleLocalPortRouter;
#endif

C1222ReceiveHistory receiveHistory;

Boolean g_sendRegistrationToNetwork = FALSE;
Boolean g_wantLogNetworkComm = FALSE;
Boolean g_wantReassembly = TRUE;

Boolean g_wantDifferentSocketForServerResponse = FALSE;

int g_opticalProbe = 0;

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













// register thread








// comm module thread

Unsigned32 commModuleCounter = 0;

Unsigned8 g_myPort = 1;

char* hostipaddress = 0;
unsigned short hostipport = 0;


extern int TcpIpSend(char* sendbuf, int bytesToSend );
extern void TcpIpCom_DisconnectFromHost(void);
extern Bool TcpIpCom_Init(char* ipaddress, unsigned short ip_portid);


void TraceBuffer(char* buffer, unsigned short length)
{
    char traceBuffer[200];
    unsigned short ii;
    char myBuffer[20];
    
            	strcpy(traceBuffer,"Tx->\n");
            	for ( ii=0; ii<length; ii++ )
            	{
            		sprintf(myBuffer, " %02X", buffer[ii] & 0xFF);
                	strcat(traceBuffer, myBuffer);

            	    if ( ii%20 == 19 )
            	    {
            	        printf("%s\n", traceBuffer);
            	        strcpy(traceBuffer,"");
            	    }
            	}
            	
            	printf("%s\n",traceBuffer);
    
}

//extern long _threadid;


char myBigBuffer[10000];

extern void NoteSeparateReply(void);

void CommModuleThread(void* param)
{
    ACSE_Message* pMsg;

    char myBuffer[100];
    Boolean messageSent = FALSE;
#ifdef WANT_TCPIP
	Unsigned8 localBuffer[10];
    Unsigned16 length;
    int bytesSent;
    int totalBytesSent;
#endif

	

    Boolean isRFLANsynchronized = FALSE;
    Unsigned32 startTickCount;

	(void)param;

    CommModuleOpenLog();
    NetworkCommOpenLog();


    UART_SetOpticalProbe(g_opticalProbe);

    #ifdef ENABLE_LOCAL_PORT_ROUTER
	C1222Stack_Init(&commModuleStack, &commModuleLocalPortRouter, g_myPort, TRUE, 0, FALSE, FALSE);
	#else
	C1222Stack_Init(&commModuleStack, 0, g_myPort, TRUE, 0, FALSE, FALSE);
	#endif

    CommModuleSetLogs(&commModuleStack);
    
    C1222Stack_EnableLog(&commModuleStack, TRUE);
    
    C1222Stack_SetRegistrationMaxRetryFactor(&commModuleStack, 0xFF);
    
    C1222Stack_SetRegistrationRetryLimitsFormat1(&commModuleStack, 1, 2);
	
	C1222AL_SetReassemblyOption(&commModuleStack.Xapplication, g_wantReassembly);

	C1222AL_SetInterfaceName(&commModuleStack.Xapplication,"PC Serial");

    C1222AL_SetRFLANisSynchronizedState(&commModuleStack.Xapplication, FALSE);
    
    C1222ALReceiveHistory_Init(&receiveHistory, &commModuleStack.Xapplication);
    
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);

    DoSleep(2000);

    startTickCount = GetTickCount();

    while(1)
    {
        commModuleCounter++;
        if ( (commModuleCounter % 77) == 0 )
        {
        	sprintf(myBuffer,"CM:  %ld, R=%d, TS=%d, AS=%d, RP=%ld, TP=%ld\n", commModuleCounter, commModuleStack.Xtransport.counters.numberOfResets,
        	                   commModuleStack.Xtransport.status.state,
        	                   commModuleStack.Xapplication.status.state,
                                       commModuleStack.Xdatalink.counters.numberPacketsReceived,
                                       commModuleStack.Xdatalink.counters.numberPacketsSent);
            printf("%s", myBuffer);
        }
        
        C1222Stack_Run(&commModuleStack);
        
    	if ( messageSent && C1222Stack_IsSendMessageComplete(&commModuleStack) )
    	{
    	    messageSent = FALSE;
    	}
        

        // check for a registration request and if so, send a registration response

        if ( !messageSent && C1222Stack_IsMessageAvailable(&commModuleStack) )
        {
            pMsg = C1222Stack_GetReceivedMessage(&commModuleStack);
            
            if ( C1222ALReceiveHistory_IsMessageAlreadyReceived(&receiveHistory, pMsg) )
            {
                C1222Stack_NoteReceivedMessageHandlingIsComplete(&commModuleStack);
            }

            // if this is a message to the host (registration for example) the 
            // simulateHost routine will consume the message, otherwise the
            // message should be sent to the network
            
            
            else if ( !CMsimulateHost(pMsg, &commModuleStack, &messageSent, g_sendRegistrationToNetwork) )
            {                
                // send the message to the client

#ifdef WANT_TCPIP

                localBuffer[0] = 0x60;
                length = C1222Misc_EncodeLength(pMsg->length, &localBuffer[1]);
                
                bytesSent = totalBytesSent = 0;

                if ( Get_ServerState() && !g_wantDifferentSocketForServerResponse )
                {
                    printf("Starting send of %d bytes\n", length+1+pMsg->length);
                    
                    bytesSent += StartSendToClient(localBuffer, (Unsigned16)(length+1));
                    if ( pMsg->length > 0 )
                        bytesSent += SendToClient(pMsg->buffer, pMsg->length);
                    
                    totalBytesSent += bytesSent;
                }
                else if ( hostipaddress != 0 )
                {
                    if ( TcpIpCom_Init(hostipaddress, hostipport) )
                    {
                    	// sw needs the message all in one shot - so copy it
                        // to another buffer and send

                        memcpy(myBigBuffer, localBuffer, length+1);
                        memcpy(&myBigBuffer[length+1], pMsg->buffer, pMsg->length);

                        bytesSent = TcpIpSend((char*)myBigBuffer, (int)(length+1) + pMsg->length);
                        if ( bytesSent > 0 )
                            TraceBuffer((char*)myBigBuffer, (unsigned short)bytesSent);
                        else
                            printf("write failed\n");

                            
                        totalBytesSent += bytesSent;
                        
                    }
                    else 
                        printf("can't initialize tcpip client com\n");
                        
                    TcpIpCom_DisconnectFromHost();
                }

                if ( totalBytesSent == 0 )
                	printf("No bytes sent of %d byte msg\n", pMsg->length);

                printf("Sent %d bytes of %d byte msg\n", totalBytesSent, pMsg->length);
                
                NetworkCommLogTx( localBuffer, (Unsigned16)(length+1), pMsg->buffer, pMsg->length);
#endif           

                C1222Stack_NoteReceivedMessageHandlingIsComplete(&commModuleStack);
            }
            
            
        }
        
        // check for and process messages from the client (eg, tables test bench)
#ifdef WANT_TCPIP 
        if ( !messageSent )
        {       
            messageSent = HandleMessageFromClient(&commModuleStack);
        }
#endif        

        if ( (GetTickCount()-GetLastHostRequestTime()) > 40000 )
        {
            Set_ServerState(FALSE);
        }
        // wait a bit

        DoSleep(10);

        if ( (GetTickCount() - startTickCount) > 10000 )
        {
        	if ( !isRFLANsynchronized )
            {
            	isRFLANsynchronized = TRUE;

                C1222AL_SetRFLANisSynchronizedState(&commModuleStack.Xapplication, TRUE);

            }
        }
    }
}


extern void TcpIpServerThread(void*);






unsigned long mainCount = 0;


extern void SetServerPort(unsigned long server_port);

void main(int argc, char** argv)
{
	Unsigned32 next, start;

    g_myPort = 1;
    if ( argc > 1 )
    {
        if ( strcmp(argv[1],"?") == 0 )
        {
            printf("simple_tcpip_cm <port> <optical probe type> <options> <hostipaddress> <hosttcpipportid> <serveripport>\n");
            printf("    options = [RrLlDdAa] concatenated chars cap=Y lower=N\n");
            printf("               R=reg to network, L=log network comm, D= use diff socket for response\n");
            printf("               A=reassemble segments\n");
            printf("    default hostipaddress is your ip address, default hostportid=27016\n");
            printf("    note server ipaddress = your ip address and default ip port is 27015\n");
            printf("    opt probe type is 0 for none or 1-4\n");
            return;
        }
        
    	g_myPort = (Unsigned8)atoi(argv[1]);
        if ( g_myPort < 1 || g_myPort > 10 )
    	{
        	printf("Bad port id\n");
            return;
        }
    }
    
    if ( argc > 2 )
    {
       g_opticalProbe = atoi(argv[2]); 
    }
    
    if ( argc > 3 )
    {
       if ( strstr(argv[3],"R") )
           g_sendRegistrationToNetwork = TRUE;
       else if ( strstr(argv[3],"r") )
           g_sendRegistrationToNetwork = FALSE; // else default
           
       if ( strstr(argv[3],"L") )
           g_wantLogNetworkComm = TRUE;
       else if ( strstr(argv[3],"l") )
           g_wantLogNetworkComm = FALSE;  // else default
           
       if ( strstr(argv[3],"D") )
       {
           g_wantDifferentSocketForServerResponse = TRUE;
           NoteSeparateReply();
       }
       else if ( strstr(argv[3],"d") )
           g_wantDifferentSocketForServerResponse = FALSE;

       if ( strstr(argv[3],"A") )
           g_wantReassembly = TRUE;
       else if ( strstr(argv[3],"a") )
           g_wantReassembly = FALSE;
    }

    if ( argc > 4 )
    {
        hostipaddress = argv[4];
        hostipport = 27016;
    }
    
    if ( argc > 5 )
    {
        hostipport = (unsigned short)atoi(argv[5]);
    }
    
    if ( argc > 6 )
    {
        SetServerPort((unsigned long)atoi(argv[6]));
    }

    srand(GetTickCount()&0xFFFF);

    
    InitializeLogCriticalSection();

    #ifdef  ENABLE_LOCAL_PORT_ROUTER
    C1222LocalPortRouter_Init(&commModuleLocalPortRouter);
    #endif

    
    _beginthread(CommModuleThread, 0, NULL);

#ifdef WANT_TCPIP
    DoSleep(2000);
	
	_beginthread(TcpIpServerThread, 0, NULL);
#endif
    

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





// should not be called
Boolean C1222Stack_SendMessageToNonC1222Stack(ACSE_Message* pMsg, Unsigned8* destinationNativeAddress, Unsigned16 addressLength)
{
    (void)pMsg;
    (void)destinationNativeAddress;
    (void)addressLength;
    
    return FALSE;
}




void C1222EnterCriticalRegion(void)
{
}

void C1222LeaveCriticalRegion(void)
{
}
