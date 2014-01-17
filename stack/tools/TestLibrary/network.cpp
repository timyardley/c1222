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

#include "c1222.h"
#include "c1222environment.h"
#include "winsock2.h" // maybe not a good idea
#include "network.hpp"
//#include "scb.hpp"
#include <dos.h>
#include <time.h>

//extern QtalkInfo g_params;

// scb versions of network routines

//
// public routines:


typedef unsigned char Boolean;

extern Boolean g_inhibit_log;

extern void Report(char* text);

FILE* g_logfp = 0;

void errorMessage(char* text);


Unsigned8 networkLookaheadBuffer[500];
Unsigned16 networkLookaheadCount = 0;


int writeToLog(const char* text)
{
    int c;
    
    if ( g_logfp == (FILE*)0 )
    {
        g_logfp = fopen("c1222networklog.txt","wt");
    }

    if ( g_logfp == (FILE*)0 || g_inhibit_log )
      return 0;

    while( TRUE )
    {
        c=(int)(*text++);

        if ( c == '\0' )
           break;

        if ( fputc(c,g_logfp) != c )
        {
            errorMessage("Can't write to log");
            return 0;
        }
    }
    return 1;
}



HANDLE  g_hCom = INVALID_HANDLE_VALUE;


void reportBuffer(const unsigned char* buffer, unsigned long length)
{
    char mybuffer[400];
    char* p = mybuffer;
    unsigned long ii;

    *p++ = ' ';
    *p++ = ' ';
    *p++ = '>';

    for ( ii=0; ii<20; ii++ )
    {
        if ( ii < length )
        {
            sprintf(p," %02x", buffer[ii] );
        }
        else
          sprintf(p, "   ");

      p += 3;
    }

    *p++ = ' ';
    *p++ = '[';


    for ( ii=0; ii<20; ii++ )
    {
        int c = (int)(buffer[ii]&0xff);

        if ( ii<length )
        {
            if ( c == '\r' || c == '\n' )
                *p++ = '*';
            else if ( isascii(c) && isprint(c) )
                *p++ = (char)c;
            else
                *p++ = '.';
        }
    }

    *p++ = ']';
    *p++ = '\n';
    *p = '\0';

    writeToLog(mybuffer);
}


void logTextAndTime(const char* text)
{
  char mybuffer[100];
  struct time now;
  time_t atime;
  time(&atime);
  tm* ptm = localtime(&atime);

  gettime(&now);

  if ( g_logfp == (FILE*)0 )
  {
    //errorMessage("Log not open");
    return;
  }


  writeToLog(text);

  DWORD modemStatus;

  if ( g_hCom == INVALID_HANDLE_VALUE )
  {
      writeToLog(" invalid com handle");
  }
  else if ( GetCommModemStatus( g_hCom, &modemStatus) )
  {
      sprintf(mybuffer," [%s %s %s]",
          (modemStatus & MS_RLSD_ON) ? "CD " : "   ",
          (modemStatus & MS_CTS_ON)  ? "CTS" : "   ",
          (modemStatus & MS_DSR_ON)  ? "DSR" : "   ");

      writeToLog(mybuffer);
  }
  else
  {
      writeToLog(" can't get modem status ");
  }

  sprintf(mybuffer, " %2d:%2d:%2d.%02d\n",
     ptm->tm_hour,now.ti_min, now.ti_sec, now.ti_hund);
  writeToLog(mybuffer);
}


void logit(const char* text, const unsigned char* buffer, unsigned long length)
{
  char mybuffer[100];
  struct time now;
  time_t atime;
  time(&atime);
  tm* ptm = localtime(&atime);

  gettime(&now);
  
  if ( g_logfp == (FILE*)0 )
  {
      g_logfp = fopen("c1222networklog.txt","wt");
  }
  

  if ( g_logfp == (FILE*)0 )
  {
    //errorMessage("Log not open");
    return;
  }
 

  writeToLog(text);

  if ( length > 0 )
  {
    DWORD modemStatus;

    if ( g_hCom == INVALID_HANDLE_VALUE )
    {
        writeToLog(" invalid com handle");
    }
    else if ( GetCommModemStatus( g_hCom, &modemStatus) )
    {
        sprintf(mybuffer," [%s %s %s]",
            (modemStatus & MS_RLSD_ON) ? "CD " : "   ",
            (modemStatus & MS_CTS_ON)  ? "CTS" : "   ",
            (modemStatus & MS_DSR_ON)  ? "DSR" : "   ");

        writeToLog(mybuffer);
    }
    else
    {
        writeToLog(" can't get modem status ");
    }

    sprintf(mybuffer, " %2d:%2d:%2d.%02d(%3ld): \n",
       ptm->tm_hour,now.ti_min, now.ti_sec, now.ti_hund, length);
    writeToLog(mybuffer);

    unsigned long ii = 0;

    while ( ii<length )
    {
        reportBuffer(&buffer[ii],(length-ii) > 20 ? 20:(length-ii));
        ii += 20;
    }

  }

  writeToLog("\n");
}


void logit(char* message)
{
  logit(message, 0, 0 );
}

void errorMessage(char* message)
{
	logit("ERROR: ");
	logit(message);
}

void reportSystemError()
{
	LPVOID lpMsgBuf;

	FormatMessage(
    	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    	NULL,
    	GetLastError(),
    	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    	(LPTSTR) &lpMsgBuf,
    	0,
    	NULL
	);

	// Display the string.
	//MessageBox( NULL, (const char*)lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );
	writeToLog((char*)lpMsgBuf);

	// Free the buffer.
	LocalFree( lpMsgBuf );
 
}


Unsigned8 rxflushbuffer[1000];
Unsigned16 flusherror = 0;

void s2NetworkServer::RxFlush()
{
    unsigned short n;
    unsigned short endIndex;
    
    PurgeComm(m_hCom,PURGE_RXCLEAR);
    
    while( (n=(unsigned short)NumberOfUnReadBytesAvailable()) > 0 )
    {
        if ( n > sizeof(rxflushbuffer) )
            n = sizeof(rxflushbuffer);
            
        if ( !ReadSomeBytes(rxflushbuffer, n, &endIndex) )
        {
            flusherror++;
            break;
        }
    }
    
    networkLookaheadCount = 0;
    
    if ( !PurgeComm(m_hCom,PURGE_RXCLEAR) )
      m_dwLastError = GetLastError();
}

s2NetworkServer::~s2NetworkServer()
{
  ClosePort();
}

BOOL s2NetworkServer::FlushTransmitBuffers()
{
  return isOpen() ? FlushFileBuffers(   m_hCom ) : FALSE;
}


void s2NetworkServer::ReleaseDispatch()
{
    // no problem
}


BOOL s2NetworkServer::CreateDispatch( const char* s )
{
    // whatever
    (void)s;
    return TRUE;
}


BOOL s2NetworkServer::SetBreak(void)
{
    return SetCommBreak(m_hCom);
}

BOOL s2NetworkServer::ClearBreak(void)
{
     return ClearCommBreak(m_hCom);
}

BOOL s2NetworkServer::IsBreakActive(void)
{
    Unsigned32 mask;
    
    if ( ClearCommError(m_hCom, &mask, NULL) )
    {
        if ( mask & CE_BREAK )
            return TRUE;
    }
    
    return FALSE;
}

BOOL s2NetworkServer::DTRon(void)
{
    return EscapeCommFunction( m_hCom, SETDTR);
}

BOOL s2NetworkServer::DTRoff(void)
{
    return EscapeCommFunction( m_hCom, CLRDTR);
}


void showSystemError(DWORD dwError)
{
    LPVOID lpMsgBuf;
 
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dwError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL
    );

    // Display the string.
    MessageBox( NULL, (char*)lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

    // Free the buffer.
    LocalFree( lpMsgBuf );
}



BOOL s2NetworkServer::ReadSomeBytes(
	unsigned char* buffer, unsigned short buffer_size, unsigned short* pEndIndex)
{
    DWORD nBytesRead;
    
    if ( !ReadFile(
                    m_hCom,	// handle of file to read
                    buffer,	// address of buffer that receives data
                    (DWORD) buffer_size,	// number of bytes to read
                    &nBytesRead,	// address of number of bytes read
                    NULL 	// address of structure for data
                    )
       )
    {
		logTextAndTime("Cannot read file");
       	reportSystemError();
       	return FALSE;
    }

    if ( (int)nBytesRead == 0 )
    {
        //logTextAndTime("Timed out");
      m_timedOut = TRUE;
      return FALSE;
    }
    
    *pEndIndex = (unsigned short)nBytesRead;
    
    return TRUE;
}






BOOL s2NetworkServer::PeekByte(unsigned char* pByte)
{
    unsigned short endIndex;
    unsigned short n;
    
    if ( networkLookaheadCount > 0 )
    {
        *pByte = networkLookaheadBuffer[0];
        return TRUE;
    }
    else if ( (n=(unsigned short)NumberOfUnReadBytesAvailable()) > 0 )
    {
        if ( n > sizeof(networkLookaheadBuffer) )
            n = sizeof(networkLookaheadBuffer);
            
        if ( s2NetworkServer::ReadSomeBytes( networkLookaheadBuffer, n, &endIndex) )
        {
            networkLookaheadCount = endIndex;
            *pByte = networkLookaheadBuffer[0];
            return TRUE;
        }   
    }
    
    return FALSE;
}

BOOL s2NetworkServer::PeekByteAtIndex(unsigned short index, unsigned char* pByte)
{
	unsigned short endIndex;
	unsigned short n;

    if ( index >= sizeof(networkLookaheadBuffer) )
        return FALSE;
        
    if ( index < networkLookaheadCount )
    {
        *pByte = networkLookaheadBuffer[index];
        return TRUE;
    }
    else if ( index < (unsigned short)(networkLookaheadCount+(n=(unsigned short)NumberOfUnReadBytesAvailable())) )
    {
        if ( n > (unsigned short)(sizeof(networkLookaheadBuffer)-networkLookaheadCount) )
            n = (unsigned short)(sizeof(networkLookaheadBuffer) - networkLookaheadCount);
            
        if ( s2NetworkServer::ReadSomeBytes(
                &networkLookaheadBuffer[networkLookaheadCount], n, &endIndex) )
        {
            networkLookaheadCount += endIndex;
            *pByte = networkLookaheadBuffer[index];
            return TRUE;
        }   
    }
    
    return FALSE;
}
 
//BOOL s2NetworkServer::PeekByte(unsigned char* pByte)   
//    Unsigned32 bytesRead;
//    Unsigned32 bytesAvail;
//    Unsigned32 bytesLeftThisMessage;
//    
//    if ( PeekNamedPipe( m_hCom, pByte, 1, &bytesRead, &bytesAvail, &bytesLeftThisMessage) && (bytesRead == 1) )
//        return TRUE;
//        
//    return FALSE;
//}

//BOOL s2NetworkServer::PeekByteAtIndex(unsigned short index, unsigned char* pByte)
//{
//    if ( index < sizeof(networkPeekBuffer) )
//    {
//        Unsigned32 bytesRead;
//        Unsigned32 bytesAvail;
//        Unsigned32 bytesLeftThisMessage;
//        
//        // this is going to drag down the system big time
//    
//        if ( PeekNamedPipe( m_hCom, networkPeekBuffer, index+1, &bytesRead, &bytesAvail, &bytesLeftThisMessage) )
//        {
//            if ( bytesRead == (Unsigned32)(index+1) )
//            {
//                *pByte = networkPeekBuffer[index];
//                return TRUE;
//            }
//        }
//    }
//    
//    return FALSE;
//}




void s2NetworkServer::SetOpticalProbe(int which)
{
	m_opticalProbe = which;
}


s2NetworkError::NetworkError s2NetworkServer::OpenPort(
    int aport,
    int abaudrate,
    int nretries,
    int nretrydelay)
{
    s2NetworkError::NetworkError result = s2NetworkError::OK;


    DWORD dwError;
    int fSuccess;
    //COMMTIMEOUTS comto;

    char buffer[200];

    m_nRetries = nretries;
    m_nRetryDelay = nretrydelay;
    m_lBaudRate = abaudrate;

    logTextAndTime("Open port start");

    ClosePort();

    sprintf(buffer,"COM%d",aport);

    logTextAndTime("Open port create file");

    m_hCom = CreateFile(buffer,
        GENERIC_READ | GENERIC_WRITE,
        0,    /* comm devices must be opened w/exclusive-access */
        NULL, /* no security attrs */
        OPEN_EXISTING, /* comm devices must use OPEN_EXISTING */
        0,    /* not overlapped I/O */
        NULL  /* hTemplate must be NULL for comm devices */
        );

    if (m_hCom == INVALID_HANDLE_VALUE)
    {
        m_dwLastError = dwError = GetLastError();

        showSystemError(dwError);

        //formatMessage( FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError, 0, buffer,
        //    sizeof(buffer),

        /* handle error */
        return translateError(dwError);
    }

    g_hCom = m_hCom;

    /*
     * Omit the call to SetupComm to use the default queue sizes.
     * Get the current configuration.
     * OMIT THE OMIT - SCB 10/28/99
     */

    logTextAndTime("open port setup comm");

    fSuccess = SetupComm(m_hCom,2000, 2000); //g_params.dwInqueue,g_params.dwOutqueue); // was 2000, 2000 just a guess
    if ( !fSuccess )
    {
        m_dwLastError = dwError = GetLastError();

        showSystemError(dwError);

        /* handle error */
        return translateError(dwError);
    }

    logTextAndTime("open port get comm state");

    fSuccess = GetCommState(m_hCom, &m_dcb);

    if (!fSuccess)
    {
        /* Handle error */
        return s2NetworkError::CANTGETCOMMSTATE;
    }

    m_original_dcb = m_dcb; // save for use later in restoring the port

    /* Fill in the DCB: baud=9600, 8 data bits, no parity, 1 stop bit. */

    m_dcb.DCBlength = sizeof(DCB);
    m_dcb.BaudRate = abaudrate;
    m_dcb.ByteSize = 8;
    m_dcb.Parity = NOPARITY;
    m_dcb.StopBits = ONESTOPBIT;

    m_dcb.fRtsControl = RTS_CONTROL_DISABLE;
    m_dcb.fDtrControl = DTR_CONTROL_ENABLE;
    m_dcb.fOutxCtsFlow = FALSE;
    m_dcb.fOutxDsrFlow = FALSE;
    m_dcb.fBinary = TRUE;
    m_dcb.fDsrSensitivity = FALSE;
    m_dcb.fTXContinueOnXoff = TRUE;
    m_dcb.fOutX = FALSE;
    m_dcb.fInX = FALSE;
    m_dcb.fNull = FALSE;

    if ( m_handshakeRTS )
      m_dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    else if ( m_toggleRTS )
      m_dcb.fRtsControl = RTS_CONTROL_TOGGLE;
    else if ( m_opticalProbe == 1 )
    {
      m_dcb.fRtsControl = RTS_CONTROL_DISABLE;
      m_dcb.fDtrControl = DTR_CONTROL_DISABLE;
    }
    else if ( m_opticalProbe == 2 )
    {
      m_dcb.fRtsControl = RTS_CONTROL_DISABLE;
      m_dcb.fDtrControl = DTR_CONTROL_ENABLE;
    }
    else if ( m_opticalProbe == 3 )
    {
      m_dcb.fRtsControl = RTS_CONTROL_ENABLE;
      m_dcb.fDtrControl = DTR_CONTROL_DISABLE;
    }
    else if ( m_opticalProbe == 4 )
    {
      m_dcb.fRtsControl = RTS_CONTROL_ENABLE;
      m_dcb.fDtrControl = DTR_CONTROL_ENABLE;
    }
      

    if ( m_useCTS )
      m_dcb.fOutxCtsFlow = TRUE;

    logTextAndTime("open port set comm state");

    fSuccess = SetCommState(m_hCom, &m_dcb);

    if (!fSuccess)
    {
        /* Handle error */
        return s2NetworkError::CANTSETCOMMSTATE;
    }

    m_isOpen = TRUE;
    
    networkLookaheadCount = 0;

    logTextAndTime("open port reset timeouts");

    //fSuccess = resetTimeouts();

   // if ( !fSuccess )
   //    return s2NetworkError::CANTSETCOMMTIMEOUTS;

    logTextAndTime("open port complete");
    
    return result;
}




#if 0
BOOL s2NetworkServer::resetTimeouts()
{
    COMMTIMEOUTS comto;

    BOOL fSuccess = (BOOL)GetCommTimeouts(m_hCom,&comto);

    if ( fSuccess )
    {
        comto.ReadIntervalTimeout=2000; // ms
        comto.ReadTotalTimeoutMultiplier=40;
        comto.ReadTotalTimeoutConstant=4000;
        comto.WriteTotalTimeoutMultiplier=0;
        comto.WriteTotalTimeoutConstant=0;

        fSuccess = (BOOL)SetCommTimeouts(m_hCom,&comto);
    }

    if ( !fSuccess )
      reportSystemError();

    return fSuccess;
}
#endif


BOOL s2NetworkServer::isCarrierOn()
{
  DWORD modemStatus;
  //COMMPROP props;

  if ( m_isOpen )
  {
#if 0
    if ( GetCommProperties( m_hCom, &props ) )
    {
      if ( (props.dwProvCapabilities & PCF_RLSD) == 0 )
        return FALSE;

      if ( props.dwProvSubType != PST_MODEM )
        return FALSE;
    }
#endif    

    if ( GetCommModemStatus( m_hCom, &modemStatus) )
    {
      return (BOOL)((modemStatus & MS_RLSD_ON) ? TRUE : FALSE);
    }
    else
      reportSystemError();
  }

  return FALSE;
}


void s2NetworkServer::ClosePort()
{
    if ( m_isOpen )
    {
      Sleep(50); // allow the last character to go out
      
      logTextAndTime("close port set comm state");

      FlushTransmitBuffers();

      (void)SetCommState(m_hCom, &m_original_dcb);  // restore dcb

      logTextAndTime("close port close handle");

      if ( !CloseHandle(m_hCom) )
        m_dwLastError = GetLastError();

      m_isOpen=FALSE;
    }

    logTextAndTime("close port complete");
}

extern "C" void SystemIdleNice(void);

s2NetworkError::NetworkError s2NetworkServer::SendBuffer(
    const BYTE* buffer, WORD nbytes )
{
    s2NetworkError::NetworkError result = s2NetworkError::OK;
    DWORD nBytesWritten;
    char mybuffer[100];

    logit("  Tx",buffer,nbytes);

    //Sleep(20L);

    if ( !WriteFile(
            m_hCom,	// handle of file to write to
            (LPCVOID)buffer,	// address of data to write to file
            (DWORD)nbytes,	// number of bytes to write
            (LPDWORD)&nBytesWritten,	// address of number of bytes written
            NULL 	// addr. of structure needed for overlapped I/O
            )
        )
    {
      reportSystemError();
      s2NetworkError::NetworkError err = translateError(m_dwLastError=GetLastError());
      sprintf(mybuffer,"  ..write file error %ld", m_dwLastError);
      logit(mybuffer);
        return err;
    }

    if ( nBytesWritten != (DWORD)nbytes )
    {
      sprintf(mybuffer,"  ..incomplete write %d of %d", nBytesWritten, nbytes);
      logit(mybuffer);
      return s2NetworkError::INCOMPLETEWRITE;
    }

    return result;
}



#if 0
s2NetworkError::NetworkError s2NetworkServer::ReceiveBuffer(
    unsigned char* buffer, int nTimeout, int buffer_size, WORD * pEndIndex )
{
    s2NetworkError::NetworkError result = s2NetworkError::OK;
    DWORD nBytesRead;
    COMMTIMEOUTS comto;

    //logit("  PreRx",(unsigned char*)"1",1);

    m_timedOut = FALSE;

    if ( nTimeout == -1 )    // return immediately with bytes read so far
    {
        //logit("  Rx no timeout\n");
        comto.ReadIntervalTimeout=MAXDWORD; // ms
        comto.ReadTotalTimeoutMultiplier=0;
        comto.ReadTotalTimeoutConstant=0;
        comto.WriteTotalTimeoutMultiplier=0;
        comto.WriteTotalTimeoutConstant=0;
    }
    else if ( nTimeout == -2 )
    {
        comto.ReadIntervalTimeout=0; // ms
        comto.ReadTotalTimeoutMultiplier=0;      // was m_icto
        comto.ReadTotalTimeoutConstant=  g_params.channelto;   // was m_icto
        comto.WriteTotalTimeoutMultiplier=0;
        comto.WriteTotalTimeoutConstant=0;
    }
    else  // this is not being used at the moment -
    {
        //logit("  Rx calc timeout\n");
        comto.ReadIntervalTimeout=m_icto; // ms
        comto.ReadTotalTimeoutMultiplier=nTimeout;
        long lTimeout = AdjustTimeOut(MinimumTimeout(2));
        // since this clause is not used and the linkTimeout param is not there anymore
        // comment out the lines - if change so this clause is used will need to fix this
        //if ( lTimeout < g_params.linkTimeout )
        //  lTimeout = g_params.linkTimeout;
        comto.ReadTotalTimeoutConstant=  lTimeout+500;
        comto.WriteTotalTimeoutMultiplier=0;
        comto.WriteTotalTimeoutConstant=0;

    }

    if ( !SetCommTimeouts(m_hCom,&comto) )
      {
        writeToLog("Can't set comm timeouts\n");
       reportSystemError();
       return translateError(m_dwLastError=GetLastError());
      }

    SystemIdleNice();
    //Sleep(20L);

    Sleep(10L);

    //logTextAndTime("ReadFile");

    if ( !ReadFile(
                    m_hCom,	// handle of file to read
                    buffer,	// address of buffer that receives data
                    (DWORD) buffer_size,	// number of bytes to read
                    &nBytesRead,	// address of number of bytes read
                    NULL 	// address of structure for data
                    )
       )
    {
        logTextAndTime("Can't read file");
       reportSystemError();
       return translateError(m_dwLastError=GetLastError());
    }

    if ( (int)nBytesRead != buffer_size )
    {
        if ( nBytesRead != 0 )
            logit("  Rx",buffer,nBytesRead);

        logTextAndTime("Timed out");
        m_timedOut = TRUE;
        return s2NetworkError::TIMEOUT;
    }

    logit("  Rx",buffer,nBytesRead);

    *pEndIndex = (WORD)nBytesRead; // was .. -1


    return result;
}
#endif


s2NetworkError::NetworkError s2NetworkServer::ReceiveBufferPartialOK(
    unsigned char* buffer, int nTimeout, int buffer_size, WORD * pEndIndex )
{
    s2NetworkError::NetworkError result = s2NetworkError::OK;
    DWORD nBytesRead;
    COMMTIMEOUTS comto;

    //logit("  PreRx",(unsigned char*)"1",1);

    m_timedOut = FALSE;
    
    if ( networkLookaheadCount > 0 )
    {
        if ( buffer_size > networkLookaheadCount )
            buffer_size = networkLookaheadCount;
            
        memcpy(buffer, networkLookaheadBuffer, buffer_size);
        
        if ( networkLookaheadCount > buffer_size )
        {
            memcpy(networkLookaheadBuffer, &networkLookaheadBuffer[buffer_size], networkLookaheadCount-buffer_size);
            networkLookaheadCount -= (unsigned short)buffer_size;
        }
        else
        {
            networkLookaheadCount = 0;
        }
        
        *pEndIndex = (WORD)buffer_size;
        
        return s2NetworkError::OK;
    }

    if ( nTimeout == -1 )
    {
        //logit("  Rx no timeout\n");
        comto.ReadIntervalTimeout=MAXDWORD; // ms
        comto.ReadTotalTimeoutMultiplier=0;
        comto.ReadTotalTimeoutConstant=0;
        comto.WriteTotalTimeoutMultiplier=0;
        comto.WriteTotalTimeoutConstant=0;
    }
    else if ( nTimeout == -2 )
    {
        comto.ReadIntervalTimeout=m_icto; // ms
        comto.ReadTotalTimeoutMultiplier=0;      // was m_icto
        comto.ReadTotalTimeoutConstant=  6000;   // was m_icto
        comto.WriteTotalTimeoutMultiplier=0;
        comto.WriteTotalTimeoutConstant=0;
    }
    else if ( nTimeout == -3 )
    {
        comto.ReadIntervalTimeout = 250;
        comto.ReadTotalTimeoutMultiplier=5;      // was m_icto
        comto.ReadTotalTimeoutConstant=  500;   // was m_icto
        comto.WriteTotalTimeoutMultiplier=0;
        comto.WriteTotalTimeoutConstant=0;
     }
    else  // this is not being used at the moment -
    {
        //logit("  Rx calc timeout\n");
        comto.ReadIntervalTimeout=nTimeout; // ms
        comto.ReadTotalTimeoutMultiplier=0;
        //long lTimeout = AdjustTimeOut(MinimumTimeout(2));
        // since this clause is not used and the linkTimeout param is not there anymore
        // comment out the lines - if change so this clause is used will need to fix this
        //if ( lTimeout < 500 )
        //    lTimeout = 500;
        comto.ReadTotalTimeoutConstant=  nTimeout;
        comto.WriteTotalTimeoutMultiplier=0;
        comto.WriteTotalTimeoutConstant=0;

    }

    if ( !SetCommTimeouts(m_hCom,&comto) )
      {
        writeToLog("Can't set comm timeouts\n");
       reportSystemError();
       return translateError(m_dwLastError=GetLastError());
      }

    SystemIdleNice();
    //Sleep(20L);

    //Sleep(10L);

    //logTextAndTime("ReadFile");

    if ( !ReadFile(
                    m_hCom,	// handle of file to read
                    buffer,	// address of buffer that receives data
                    (DWORD) buffer_size,	// number of bytes to read
                    &nBytesRead,	// address of number of bytes read
                    NULL 	// address of structure for data
                    )
       )
    {
        logTextAndTime("Can't read file");
       reportSystemError();
       return translateError(m_dwLastError=GetLastError());
    }

    if ( (int)nBytesRead == 0 )
    {
        //logTextAndTime("Timed out");
      m_timedOut = TRUE;
      return s2NetworkError::TIMEOUT;
    }

    //logit("  Rx",buffer,nBytesRead);

    *pEndIndex = (WORD)nBytesRead; // was .. -1


    return result;
}


unsigned long s2NetworkServer::NumberOfUnReadBytesAvailable(void)
{
    DWORD errors = 0;
    COMSTAT comStats;
    
    BOOL ok = ClearCommError(m_hCom, &errors, &comStats);
    
    if ( ok )
    {
        return comStats.cbInQue;
    }

    return 0;
}

unsigned long s2NetworkServer::NumberOfBytesAvailable(void)
{
    return NumberOfUnReadBytesAvailable() + networkLookaheadCount;
}


BOOL s2NetworkServer::IsByteAvailable(void)
{
    return NumberOfBytesAvailable() > 0;
//    DWORD errors = 0;
//    COMSTAT comStats;
//    
//    BOOL ok = ClearCommError(m_hCom, &errors, &comStats);
//    
//    if ( ok )
//    {
//        ok = comStats.cbInQue > 0;
//    }
//
//    return ok;
}





long s2NetworkServer::MinimumTimeout( WORD wNumBytes )
{
    WORD wBitsPerChar = (WORD) 10; // (m_usDataBits + m_usStopBits + 1);
    long lBits = (long)wNumBytes * (WORD)wBitsPerChar;

    return ( (lBits * 1000L) /m_lBaudRate) + 1;
}


long s2NetworkServer::AdjustTimeOut( long lMs )
{
  long lTimeoutAllowance = lMs / 10L;

  // Lengthen the minimum expected timeout by adding either 1/20 of a second or
  // 10% of the minimum timeout, which ever is larger

  if ( lTimeoutAllowance < 50L )
	  return( lMs + 50L );
  else
    return( lMs + lTimeoutAllowance );
}



s2NetworkError::NetworkError s2NetworkServer::translateError(DWORD dwError)
{
    s2NetworkError::NetworkError result;

    switch(dwError)
    {
      case ERROR_ACCESS_DENIED:
        result = s2NetworkError::ACCESS_DENIED;
        break;

      case ERROR_FILE_NOT_FOUND:
        result = s2NetworkError::FILE_NOT_FOUND;
        break;

      default:
        result = (s2NetworkError::NetworkError)(1000000+dwError);
        break;
    }

    return result;
}