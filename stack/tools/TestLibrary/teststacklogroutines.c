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

#define C1222_WANT_EVENTID_TEXT

#include "c1222stack.h"
#include "c1222event.h"
#include "c1222misc.h"

#include "windows.h"



extern Boolean g_wantLogNetworkComm;



CRITICAL_SECTION g_log_critical_section;


void EnterLogCriticalSection(void)
{
    EnterCriticalSection(&g_log_critical_section);
}

void LeaveLogCriticalSection(void)
{
    LeaveCriticalSection(&g_log_critical_section);
}

void InitializeLogCriticalSection(void)
{
    InitializeCriticalSection(&g_log_critical_section);
}


// LOG FUNCTIONS

FILE* g_regLogFp = 0;
FILE* g_cmLogFp = 0;

FILE* g_relayLogFp = 0;
FILE* g_relayCMLogFp = 0;


void RegisterOpenLog(void)
{
    g_regLogFp = fopen("c1222reglog.txt", "wt");
}

void CommModuleOpenLog(void)
{
    g_cmLogFp = fopen("c1222cmlog.txt", "wt");
}

void RelayOpenLog(void)
{
    g_relayLogFp = fopen("c1222relaylog.txt", "wt");
}

void RelayCMOpenLog(void)
{
    g_relayCMLogFp = fopen("c1222relaycmlog.txt","wt");
}



char* LogTime(char* mybuffer)
{
  SYSTEMTIME systemTime;
  
  GetLocalTime( &systemTime );

  sprintf(mybuffer, "%2d/%02d/%02d %2d:%02d:%02d (+%3dms) ",
    systemTime.wMonth,
    systemTime.wDay,
    systemTime.wYear-2000,
    systemTime.wHour,
    systemTime.wMinute,
    systemTime.wSecond,
    systemTime.wMilliseconds); 
    
  return mybuffer;
}



static void LogEventDescription(FILE* fp, Unsigned16 event)
{
    if ( event < ELEMENTS(c1222_event_descriptions) )
        fprintf(fp, "\"%s\"", c1222_event_descriptions[event]); 
    else
        fprintf(fp, "<event description %u is missing>", event);  
}



static void LogText(FILE* fp, char* component, char* text)
{
    char mybuffer[30];
    
    if ( fp != 0 )
    {
        EnterLogCriticalSection();

        fprintf(fp, "%s: %s %s\n", component, LogTime(mybuffer), text);

        LeaveLogCriticalSection();
    }    
}



static void LogEvent(FILE* fp, char* component, C1222AL* p, Unsigned16 event)
{
    char mybuffer[30];
    
    if ( fp != 0 )
    {
        EnterLogCriticalSection();

        fprintf(fp, "%s: %s ", component, LogTime(mybuffer));
        LogEventDescription(fp, event);
        fprintf(fp,"\n");

        LeaveLogCriticalSection();
    }

    (void)p;
}




static void LogEventAndBytes(FILE* fp, char* component, C1222AL* p, Unsigned16 event, Unsigned8 n, Unsigned8* bytes)
{
    char myBuffer[4];
    char timebuffer[30];

    if ( fp != 0 )
    {
        EnterLogCriticalSection();

        fprintf(fp, "%s: %s ", component, LogTime(timebuffer));
        LogEventDescription(fp, event);
        fprintf(fp,"\n");

        while(n--)
        {
            sprintf(myBuffer," %02X", *bytes++ );
            fprintf(fp, myBuffer);
        }

        fprintf(fp, "\n");

        LeaveLogCriticalSection();
    }

    (void)p;
}



static void LogEventAndULongs(FILE* fp, char* component, C1222AL* p, Unsigned16 event, Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4)
{
    char timebuffer[30];
    
    if ( fp != 0 )
    {
        EnterLogCriticalSection();

        fprintf(fp, "%s: %s ", component, LogTime(timebuffer));
        LogEventDescription(fp, event);
        fprintf(fp,": %lu, %lu, %lu, %lu\n", u1, u2, u3, u4);   

        LeaveLogCriticalSection();
    }

    (void)p;
}





// Register logs

void RegisterLogText(char* text)
{
    LogText(g_regLogFp, "Reg", text);
}




void RegisterLogEvent(C1222AL* p, Unsigned16 event)
{
    LogEvent(g_regLogFp, "Reg", p, event);
}



void RegisterLogEventAndBytes(C1222AL* p, Unsigned16 event, Unsigned8 n, Unsigned8* bytes)
{
    LogEventAndBytes(g_regLogFp, "Reg", p, event, n, bytes);
}



void RegisterLogEventAndULongs(C1222AL* p, Unsigned16 event, Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4)
{
    LogEventAndULongs(g_regLogFp, "Reg", p, event, u1, u2, u3, u4);
}




void RegisterSetLogs(C1222Stack* pStack)
{
    C1222Stack_SetLog(pStack, RegisterLogEvent, RegisterLogEventAndBytes, RegisterLogEventAndULongs);
}





// comm module logs



void CommModuleLogEvent(C1222AL* p, Unsigned16 event)
{
    LogEvent(g_cmLogFp, "CM", p, event);
}



void CommModuleLogEventAndBytes(C1222AL* p, Unsigned16 event, Unsigned8 n, Unsigned8* bytes)
{
    LogEventAndBytes(g_cmLogFp, "CM", p, event, n, bytes);
}



void CommModuleLogEventAndULongs(C1222AL* p, Unsigned16 event, Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4)
{   
    LogEventAndULongs(g_cmLogFp, "CM", p, event, u1, u2, u3, u4); 
}



void CommModuleLogText(char* text)
{
    LogText(g_cmLogFp, "CM", text);
}



void CommModuleSetLogs(C1222Stack* pStack)
{
    C1222Stack_SetLog(pStack, CommModuleLogEvent, CommModuleLogEventAndBytes, CommModuleLogEventAndULongs);
}





// relay logs






void RelayLogText(char* text)
{
    LogText(g_relayLogFp, "Rly", text);
}


void RelayLogEvent(C1222AL* p, Unsigned16 event)
{
    LogEvent(g_relayLogFp, "Rly", p, event);
}



void RelayLogEventAndBytes(C1222AL* p, Unsigned16 event, Unsigned8 n, Unsigned8* bytes)
{
    LogEventAndBytes(g_relayLogFp, "Rly", p, event, n, bytes);
}



void RelayLogEventAndULongs(C1222AL* p, Unsigned16 event, Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4)
{
    LogEventAndULongs(g_relayLogFp, "Rly", p, event, u1, u2, u3, u4);
}



// relay cm logs


void RelayCMLogEvent(C1222AL* p, Unsigned16 event)
{
    LogEvent(g_relayCMLogFp, "RlyCm", p, event);
}


void RelayCMLogText(char* text)
{
    LogText(g_relayCMLogFp, "RlyCM", text);
}



void RelayCMLogEventAndBytes(C1222AL* p, Unsigned16 event, Unsigned8 n, Unsigned8* bytes)
{
    LogEventAndBytes(g_relayCMLogFp, "RlyCm", p, event, n, bytes);
}



void RelayCMLogEventAndULongs(C1222AL* p, Unsigned16 event, Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4)
{
    LogEventAndULongs(g_relayCMLogFp, "RlyCm", p, event, u1, u2, u3, u4);
}





void RelayCMSetLogs(C1222Stack* pStack)
{
	C1222Stack_SetLog(pStack, RelayCMLogEvent, RelayCMLogEventAndBytes, RelayCMLogEventAndULongs);
}




void RelaySetLogs(C1222Stack* pStack)
{
	C1222Stack_SetLog(pStack, RelayLogEvent, RelayLogEventAndBytes, RelayLogEventAndULongs );
}




// log and printf

void RegisterLogAndPrintf(char* text)
{
    RegisterLogText(text);
    printf(text);
}


void RelayLogAndPrintf(char* text)
{
    RelayLogText(text);
    printf(text);
}


void CMLogAndPrintf(char* text)
{
    CommModuleLogText(text);
    RelayCMLogText(text);
    printf(text);
}


void LogAndPrintf(char* text)
{
    if ( g_relayLogFp )
        RelayLogAndPrintf(text);
    else if ( g_relayCMLogFp )
        CMLogAndPrintf(text);
}


void LogAndPrintMessage(char* description, ACSE_Message* pMsg)
{
    char buffer[100];
    Unsigned16 ii;
    
    LogTime(buffer);
    LogAndPrintf(buffer);
    LogAndPrintf(description);
    sprintf(buffer, "(Length=%u):\n", pMsg->length);
    LogAndPrintf(buffer);
    
    for ( ii=0; ii<pMsg->length; ii++ )
    {
        sprintf(buffer, " %02X", pMsg->buffer[ii] );
        LogAndPrintf(buffer);
        if ( (ii%16) == 1 )
            LogAndPrintf("\n");
    }
    
    LogAndPrintf("\n");
    
}



void LogHex(FILE* fp, unsigned char* buffer, unsigned short length)
{
    char myBuffer[4];
    
    while(length--)
    {
        sprintf(myBuffer," %02X", *buffer++ );
        fprintf(fp, myBuffer);
    }
    
    fprintf(fp, "\n");
}


void LogTime1(FILE* fp)
{
  char mybuffer[100];
  SYSTEMTIME systemTime;

  GetLocalTime( &systemTime );

  sprintf(mybuffer, " %2d:%02d:%02d.%03d>  ",
     systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);

  fprintf(fp, mybuffer);
}



FILE* g_networkCommLogFp = 0;

void NetworkCommOpenLog(void)
{
    if ( g_wantLogNetworkComm )
    {
        g_networkCommLogFp = fopen("c1222netlog.txt", "wt");
    }
}


void NetworkCommLogRx(Unsigned8* buffer, Unsigned16 length)
{
    if ( g_networkCommLogFp )
    {
        EnterLogCriticalSection();
        
        fprintf(g_networkCommLogFp, "Rx <-\n");
        LogTime1(g_networkCommLogFp);
        LogHex(g_networkCommLogFp, buffer, length);
        fprintf(g_networkCommLogFp,"\n"); 
        
        LeaveLogCriticalSection();   
    }
}


void NetworkCommLogTx(Unsigned8* part1, Unsigned16 part1Length, Unsigned8* part2, Unsigned16 part2Length)
{
    if ( g_wantLogNetworkComm )
    {
        EnterLogCriticalSection();
        
        fprintf(g_networkCommLogFp, "Tx ->\n");
        LogTime1(g_networkCommLogFp);
        LogHex(g_networkCommLogFp, part1, part1Length);
        LogHex(g_networkCommLogFp, part2, part2Length);
        fprintf(g_networkCommLogFp,"\n");
        
        LeaveLogCriticalSection(); 
    }
}