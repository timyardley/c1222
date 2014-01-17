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

#ifndef NETWORK_HPP
#define NETWORK_HPP

//#include "afx.h"
//#include "scb.hpp"

class s2NetworkError
{
    public:
    enum NetworkError
    {
        OK=0 ,
        CANTGETCOMMSTATE=1,
        CANTSETCOMMSTATE=2,
        NE_ERROR=3,
        CANTGETCOMMTIMEOUTS=4,
        CANTSETCOMMTIMEOUTS=5,
        INCOMPLETEWRITE=6,
        TIMEOUT=7,
        ACCESS_DENIED=8,
        FILE_NOT_FOUND=9,
    };
};

class s2NetworkServer
{
    public:
        s2NetworkServer()
          {
            m_isOpen=FALSE;
            m_dwLastError=0;
            m_handshakeRTS=FALSE;
            m_toggleRTS=FALSE;
            m_opticalProbe = 0;
            m_useCTS=FALSE;
            m_icto = 500;
            m_ipto = 2000;
          }
        void RxFlush();
        void ReleaseDispatch();
        BOOL CreateDispatch( const char* s );
 	    s2NetworkError::NetworkError OpenPort( int aport, int abaudrate, int azero1, int azero2);
        void ClosePort();
        s2NetworkError::NetworkError SendBuffer( const BYTE* c, WORD nbytes );
        s2NetworkError::NetworkError ReceiveBuffer( unsigned char* cReadAhead, int minus1, int buffer_size, WORD * pEndIndex );
        s2NetworkError::NetworkError ReceiveBufferPartialOK( unsigned char* cReadAhead, int minus1, int buffer_size, WORD * pEndIndex );
        BOOL ReadSomeBytes(unsigned char* buffer, unsigned short buffer_size, unsigned short* pEndIndex);

        BOOL isOpen() const { return (BOOL)m_isOpen; }
        BOOL PeekByte(unsigned char* pByte);
        BOOL PeekByteAtIndex(unsigned short index, unsigned char* pByte);
        BOOL isCarrierOn();
        BOOL resetTimeouts();
        BOOL FlushTransmitBuffers();
        void SetHandshakeRTS(BOOL b) { m_handshakeRTS=b; }
        void SetToggleRTS(BOOL b) { m_toggleRTS=b; }
        void SetUseCTS(BOOL b) { m_useCTS=b; }
        void SetOpticalProbe(int which);
        BOOL SetBreak(void);
        BOOL ClearBreak(void);
        BOOL DTRon(void);
        BOOL DTRoff(void);
        void SetInterCharacterTimeout(long icto) { m_icto = icto; }
        void SetInterPacketTimeout(long ipto) { m_ipto = ipto; }
        BOOL TimedOut(void) { return m_timedOut; }
        BOOL IsByteAvailable(void);
        unsigned long NumberOfBytesAvailable(void);
        unsigned long NumberOfUnReadBytesAvailable(void);
        BOOL IsBreakActive(void);

        ~s2NetworkServer();

    protected:
        s2NetworkError::NetworkError translateError(DWORD dwError);

        long 		MinimumTimeout( WORD wNumBytes );
        long    AdjustTimeOut( long ms);

    private:
        BOOL m_isOpen;
        DCB m_dcb;
        char reserved[100];
        HANDLE m_hCom;
        DWORD m_dwLastError;
        int m_nRetries;
        int m_nRetryDelay;
        int m_opticalProbe;
        long m_lBaudRate;
        DCB m_original_dcb;
        BOOL m_handshakeRTS;
        BOOL m_toggleRTS;
        BOOL m_useCTS;
        long m_icto;
        long m_ipto;
        BOOL m_timedOut;
};

#endif
