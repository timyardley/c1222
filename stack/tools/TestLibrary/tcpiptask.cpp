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

#include <winsock2.h>
#include <stdio.h>




extern "C" void DoSleep(unsigned long ms);

extern "C" unsigned char doCMProcessReceivedStreamBytes(char*, int);

extern "C" unsigned long C1222Misc_GetFreeRunningTimeInMS(void);



char sendbuf[1100];
char recvbuf[1000];

extern unsigned char g_receiveReady;
extern unsigned char g_sendReady;
extern unsigned char g_sendBuffer[1100];
extern unsigned short g_sendLength;

unsigned long receivedChunks = 0;
unsigned long totalReceivedBytes = 0;

unsigned char separateReply = 0; //SEPARATE_REPLY;

char traceBuffer[4000];
char myBuffer[100];

WSADATA wsaData;

unsigned long g_server_port = 27015;

extern "C" void SetServerPort(unsigned long server_port)
{
    g_server_port = server_port;   
}

extern "C" void NoteSeparateReply(void)
{
    separateReply = 1;
}

extern "C" void TcpIpServerThread(void* dummy)
{
    char* ipaddress;
    int bytesSent;
    int bytesRecv;
    
    SOCKET m_socket;
    SOCKET save_m_socket;
    sockaddr_in service;
    hostent* localHost;
    SOCKET AcceptSocket;
    int iResult;
    int ii;
    unsigned char waitingForResponseFromStack = FALSE;
    unsigned char socketReady = FALSE;
    unsigned long waitingForResponseFromStackStart = 0;
    
    (void)dummy;


    iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
    if ( iResult != NO_ERROR )
        printf("Error in WSAStartup() = %d\n", WSAGetLastError());

    // Create a socket.
    m_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

    if ( m_socket == INVALID_SOCKET ) {
        printf( "Error in socket(): %ld\n", WSAGetLastError() );
        WSACleanup();
        return;
    }
    

    // Bind the socket.

    service.sin_family = AF_INET;


    // Get the local host information
    localHost = gethostbyname("");
    ipaddress = inet_ntoa (*(struct in_addr *)*localHost->h_addr_list);

    printf("Using ip address = '%s'\n", ipaddress);


    service.sin_addr.s_addr = inet_addr( ipaddress );
    service.sin_port = htons( (unsigned short)g_server_port );

    if ( bind( m_socket, (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR ) {
        printf( "bind() failed error=%d.\n", WSAGetLastError() );
        closesocket(m_socket);
        return;
    }

    // Listen on the socket.
    if ( listen( m_socket, 1 ) == SOCKET_ERROR )
        printf( "Error listening on socket. Error=%d\n", WSAGetLastError());

    // Accept connections.

    printf( "Waiting for a client to connect...\n" );
    
    save_m_socket = m_socket;

    while(1)
    {
        if ( !socketReady )
        {
            AcceptSocket = accept(m_socket, NULL, NULL );

        	if ( AcceptSocket != SOCKET_ERROR )
        	{
        		m_socket = AcceptSocket;
                socketReady = TRUE;
            }
        }

        if ( g_sendReady && socketReady )
        {
        	memcpy(sendbuf, g_sendBuffer, g_sendLength);

            bytesSent = send( m_socket, sendbuf, g_sendLength, 0);
            
            if ( bytesSent == SOCKET_ERROR || bytesSent < 0 )
            {            	
            	printf("socket error in send = %d\n", WSAGetLastError());

                m_socket = save_m_socket;
                socketReady = FALSE;
            }
            else
            {
            	strcpy(traceBuffer,"Tx->\n");
            	for ( ii=0; ii<g_sendLength; ii++ )
            	{
            		sprintf(myBuffer, " %02X", g_sendBuffer[ii] & 0xFF);
                	strcat(traceBuffer, myBuffer);

            	    if ( ii%20 == 19 )
            	    {
            	        printf("%s\n", traceBuffer);
            	        strcpy(traceBuffer,"");
            	    }
            	}
            	
            	printf("%s\n",traceBuffer);

                if ( separateReply )
                {
                	m_socket = save_m_socket;
                	socketReady = FALSE;

                }
            }

            waitingForResponseFromStack = FALSE;
            g_sendReady = FALSE;
        }

        if ( socketReady && !waitingForResponseFromStack )
        {
            // lets see if there is a message for me

            bytesRecv = recv( m_socket, (char*)recvbuf, 1000, MSG_PEEK );

            if ( bytesRecv != SOCKET_ERROR )
            {
            	if ( bytesRecv > 0 )
                	bytesRecv = recv( m_socket, (char*)recvbuf, bytesRecv, 0);

            	if ( bytesRecv > 0 )
            	{            	    
                    strcpy(traceBuffer,"Rx<-\n");
            	    for ( ii=0; ii<bytesRecv; ii++ )
            	    {

            		    sprintf(myBuffer, " %02X", recvbuf[ii]&0xFF);
                	    strcat(traceBuffer, myBuffer);

            	        if ( ii%20 == 19 )
            	        {
            	            printf("%s\n", traceBuffer);
            	            strcpy(traceBuffer,"");
            	        }

            	    }
            	    
            	    printf("%s\n", traceBuffer);
            	    
            		waitingForResponseFromStack = doCMProcessReceivedStreamBytes(recvbuf, bytesRecv);
            		
            		if ( waitingForResponseFromStack )
            		    waitingForResponseFromStackStart = C1222Misc_GetFreeRunningTimeInMS();

                	if ( waitingForResponseFromStack && separateReply )
                    	waitingForResponseFromStack = FALSE;
            	}

            	receivedChunks++;
                totalReceivedBytes += bytesRecv;
                
                if ( bytesRecv == 0 )
                {
                    printf("socket closed\n");
                    m_socket = save_m_socket;
                    socketReady = FALSE;                    
                }
                else if ( bytesRecv < 0 )
                {
                    printf("socket error in recv1 = %d\n", WSAGetLastError());
                
                    m_socket = save_m_socket;
                    socketReady = FALSE;
                }
            }
            else
            {
                printf("socket error in recv = %d\n", WSAGetLastError());
                
                m_socket = save_m_socket;
                socketReady = FALSE;
            }
        }
        
        if ( waitingForResponseFromStack  )
        {
            if ( (C1222Misc_GetFreeRunningTimeInMS()-waitingForResponseFromStackStart) > 60000L )
                waitingForResponseFromStack = FALSE;
        }

        DoSleep(100);
    }
}


void Report(char* text)
{
    printf(text);
}



//static WSADATA wsaData;

static SOCKET my_socket;


extern "C" bool TcpIpCom_Init(char* ipaddress, unsigned short ip_portid)
{
    hostent* localHost;
    char myMessage[100];
    
    //int iResult;
    
    //iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
    //if ( iResult != NO_ERROR )
    //{
    //    Report("Error at WSAStartup()\n");
    //}

    // Create a socket.
    
    my_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

    if ( my_socket == INVALID_SOCKET ) {
        sprintf( myMessage, "Error at socket(): %ld\n", WSAGetLastError() );
        Report(myMessage);
        //WSACleanup();
        return false;
    }

    if ( ip_portid == 0 )
    {
        if ( ipaddress[0] == 0 )
        {
            // Get the local host information
            localHost = gethostbyname("");
            ipaddress = inet_ntoa (*(struct in_addr *)*localHost->h_addr_list);
            ip_portid = (unsigned short)g_server_port;

        }
    }

    // Connect to a server.
    sockaddr_in clientService;

    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr(ipaddress) ;
    clientService.sin_port = htons(ip_portid);

    if ( connect( my_socket, (SOCKADDR*) &clientService, sizeof(clientService) ) == SOCKET_ERROR) {
        Report( "Failed to connect.\n" );
        //WSACleanup();
        return false;
    }
    else
    {
        Report("Connected to device at address ");
        Report(ipaddress);
        sprintf(myMessage, " port %d\n", ip_portid);
        Report(myMessage);
        
        //logit("Connected to device at address ");
        //logit(ipaddress);
        
        //logTextAndTime(myMessage);
    }
    
    //g_params.isPortOpen = TRUE;
    
    //c1222Setup.timeout = 30000;
    
    //g_inhibit_option_board_lid_read = TRUE;
    
    return true;
    
}




extern "C" void TcpIpCom_DisconnectFromHost(void)
{
    //g_params.isPortOpen = FALSE;
    
    //g_inhibit_option_board_lid_read = FALSE;
    
    closesocket(my_socket);
    
    //WSACleanup();
    
    //logTextAndTime("Disconnected");
}






extern "C" int TcpIpSend(char* sendbuf, int bytesToSend )
{
    // Send and receive data.
    int bytesSent;
    //int bytesRecv = SOCKET_ERROR;
    //char sendbuf[32] = "Client: Sending data.";
    //char recvbuf[32] = "";
    
    //logit("Sending ", (unsigned char*)sendbuf, (unsigned long)bytesToSend);

    bytesSent = send( my_socket, sendbuf, bytesToSend, 0 );
    //printf( "Bytes Sent: %ld\n", bytesSent );
    

    return bytesSent;
}

