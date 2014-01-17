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
#include "c1222stack.h"
#include "c1222misc.h"
#include "c1219table.h"
#include "c1222response.h"

#include "windows.h"



extern void LogAndPrintf(char* text);

#ifdef C1222_TABLESERVER_BIGENDIAN
Boolean g_littleEndian = FALSE;
#else
Boolean g_littleEndian = TRUE;
#endif


void C1219TableServerTest_SetLittleEndian(Boolean b)
{
	g_littleEndian = b;
}




///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The following are test implementation of the c12.19 table server
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

// this should be a static routine in the table server

Unsigned8 getchecksum(Unsigned8* buffer, Unsigned16 length)
{
    Unsigned16 ii;
    Unsigned8 sum = 0;
    
    for ( ii=0; ii<length; ii++ )
        sum += buffer[ii];
        
    return (Unsigned8)(~sum + 1);
}


Unsigned8 g_table2048[960];

void initTables(void)
{
    Unsigned16 ii;
    
    for ( ii=0; ii<sizeof(g_table2048); ii++ )
        g_table2048[ii] = (Unsigned8)(ii & 0xFF);
}


Unsigned8 g_myId[20] = "Test5678901234567890";

void C1219TableServer_StartUserAccess(C1219TableServer* p, Unsigned16 userId, Unsigned8* password)
{
    // nothing to do in this test
    
    (void)p;
    (void)userId;
    (void)password;
}


void C1219TableServer_NoteTest(C1219TableServer* pTableServer, Boolean isTest)
{
    pTableServer->isTest = isTest;
}


void C1219TableServer_NoteNotification(C1219TableServer* pTableServer, Boolean isNotification)
{
    pTableServer->isNotification = isNotification;
}


void C1219TableServer_NoteRequestStandard(C1219TableServer* p, C1222StandardVersion version)
{
	(void)version;
    (void)p;    
}




typedef struct CalTimeTag     // calendar time
{
    unsigned year : 8;      //Year relative to 2000
    unsigned month : 4;     // 1 = January
    unsigned day : 5;       // 1 = First day of the month
    unsigned hour : 5;      // 0 = Midnight
    unsigned min : 6;       // 0 based
    unsigned sec : 6;       // 0 based
    unsigned msec : 10;     // 0 based
    unsigned reserved : 4;  // 48 bits total
} CalTime;    
    


typedef struct TimeTag
{
    Signed16 mSecs;           //Milli-seconds
    Signed32 Secs;            //Seconds since 0000 hours, 01/01/00
} Time; 



    const Unsigned16 MonthDays[2][12] = 
    {
        {0,31,59,90,120,151,181,212,243,273,304,334},
        {0,31,60,91,121,152,182,213,244,274,305,335},
    };

    //   J  F  M  A  My  Ju  JL  AU  Se  Oc  No  De
    //  31 28 31 30  31  30  31  31  30  31  30  31
    //   0 31 59 90 120 151 181 212 243 273 304 334 365


#define SECONDSPERMINUTE (60L)
#define SECONDSPERHOUR   (60L*60L)
#define SECONDSPERDAY    (24L*SECONDSPERHOUR)
#define SECONDSPERYEAR   (365L*SECONDSPERDAY)

#define JULIAN19010101    (2415386L)




//extern unsigned long g_base_year;

#define BASE_YEAR   1970



//=======================================================================
//  Class: Date
//
//  Method: jday(dayTy day, monthTy month, yearTy year)
//
//  Description: Convert Gregorian calendar date to the corresponding
//  Julian day number j. Algorithm 199 from Communications of the ACM,
//  Volume 6, No. 8, (Aug. 1963), p. 444.  Gregorian calendar started
//  on Sep. 14, 1752. This function not valid before that.
//  This function was copied from the NIH class library.
//
// this was ported from the Q1000
//=======================================================================

unsigned long julianday(int day, int month, int year)
{
	unsigned long c, ya=year, mm=month, dd=day;

	if (mm > 2)
		mm -= 3;
	else
    {
      	mm += 9;
      	ya--;
    }

  	c = ya / 100;
  	ya = ya - 100*c;

    // comments on the magic numbers (scb)
    // 400*365 + 100 - 4 + 1 = 146097   ie, the number of days in 400 years
    // 365*4 + 1             = 1461     ie, the number of days in 4 years
    // check the above reference for more info

  	return ((146097L*c)>>2) + ((1461L*ya)>>2) + (153L*mm + 2L)/5L + dd + 1721119L;
}


// converts calendar time to absolute time

Time *TM_Cal2Abs(const CalTime *DateTime, Time *AbsoluteTime)
{
    Unsigned32 Secs;
    Unsigned8 LeapYear;
    Signed8 LeapDays;
    
    //Is this a leap year?
    LeapYear = (Unsigned8)(((DateTime->year % 4) == 0) ? 1 : 0);    // this will be wrong in the year 2100

    if ( DateTime->year > 0 )
        LeapDays = (Signed8)(( DateTime->year-1)/4 + 1);
    else
        LeapDays = 0;

    //Secs = 0L;
    
    //Add 365 days of seconds for each year
    Secs = DateTime->year * SECONDSPERYEAR;
    
    //Add in all the days this year plus the past leap year days.            
    Secs += ((MonthDays[LeapYear][DateTime->month-1] + DateTime->day-1 + LeapDays) * SECONDSPERDAY);
    Secs += (DateTime->hour * SECONDSPERHOUR);
    Secs += (DateTime->min * 60);
    Secs += DateTime->sec;
    
    AbsoluteTime->Secs = Secs;
    AbsoluteTime->mSecs = (Unsigned16)(DateTime->msec);
    
    // need to add in seconds from base_year to 2000
    
    AbsoluteTime->Secs += ((Signed32)(julianday( 1, 1, 2000) - julianday( 1, 1, BASE_YEAR)))*SECONDSPERDAY;

    
    return(AbsoluteTime);
}
         


Unsigned8 C1219TableServer_Read(C1219TableServer* p, Unsigned16 tableId, Unsigned8* response, Unsigned16 maxLength, Unsigned16* pLength)
{
	(void)p;
	
    if ( p->isNotification )
        return C1222RESPONSE_IAR;
	
    
    if ( tableId == 0 )
    {
    	if ( maxLength < 25 )
        	return C1222RESPONSE_ERR;
            
        response[0] = 0; // ok
        response[1] = 0;
        response[2] = 35;
        response[3] = 0x02;
        if ( !g_littleEndian )
        	response[3] += 0x01;

        response[4] = 0x08 + 0x03; // offset reads and tm format 4
        response[5] = 0x8b; // ni fmt 1 and 2

        memcpy(&response[6], "TEMP" /*"SCH\0"*/, 4); // device class
        response[10] = 2; // electric
        response[11] = 0; // no default
        response[12] = 0;
        response[13] = 0;
        response[14] = 1; // std version 1
        response[15] = 0; // std rev number
        response[16] = 7; // dim std tables
        response[17] = 1; // dim mfg tables
        response[18] = 0; // dim std proc
        response[19] = 0; // dim mfg proc
        response[20] = 0; // dim mfg status flags
        response[21] = 0; // number pending
        response[22] = 0x29; // std tables  0 -  7: 0, 3, 5
        response[23] = 0;    // std tables  8-15:
        response[24] = 0;    // std tables 16-23
        response[25] = 0;    // std tables 24-31
        response[26] = 0;    // std tables 32-39
        response[27] = 0;    // std tables 40-47
        response[28] = 0x18; // std tables 48-55: 51,52
        response[29] = 0x01; // mfg table 0 only (2048)
        response[30] = 0x20; // std tables write
        response[31] = 0;
        response[32] = 0;
        response[33] = 0;
        response[34] = 0;
        response[35] = 0;
        response[36] = 0;
        response[37] = 0x01; // mfg table 0 is writable
        response[3+35] = getchecksum(&response[3],35);
        *pLength = 4+35;
    }
    else if ( tableId == 3 )
    {
    	if ( maxLength < 8 )
        	return C1222RESPONSE_ERR;

        response[0] = 0;
        response[1] = 0;
        response[2] = 4; // length
        response[3] = 0x04; // meter shop mode
        response[4] = 0x41; // unprogrammed, clock error
        response[5] = 0x01; // low battery
        response[6] = 0x00;
        response[3+4] = getchecksum(&response[3],4);
        *pLength = 4+4;
    }
    else if ( tableId == 5 )
    {
    	if ( maxLength < 24 )
        	return C1222RESPONSE_ERR;

        response[0] = 0;
        response[1] = 0;
        response[2] = 20;
        memcpy(&response[3], g_myId, 20);
        response[3+20] = getchecksum(&response[3], 20);
        *pLength = 20+4;
    }
    else if ( tableId == 51 )
    {
        response[0] = 0;
        response[1] = 0;
        response[2] = 9;
        memset(&response[3], 0, 9);
        response[3] = 0x20;
        response[12] = getchecksum(&response[3], 9);
        *pLength = 9+4;
    }
    else if ( tableId == 52 )
    {
        SYSTEMTIME systemTime;
        CalTime now;
        Time abs;
        Unsigned32 minutes;
        Unsigned8 seconds;

        // get time and date from system

        GetLocalTime(&systemTime);

        now.year = systemTime.wYear - 2000;
        now.month = systemTime.wMonth;
        now.day =   systemTime.wDay;
        now.hour =  systemTime.wHour;
        now.min =   systemTime.wMinute;
        now.sec =   systemTime.wSecond;
        now.msec =  systemTime.wMilliseconds;
        now.reserved = 0;

        LogAndPrintf("Time returned to rflan\n");


        // convert to seconds since 1/1/2000

        (void)TM_Cal2Abs(&now, &abs);

        minutes = abs.Secs/60;
        seconds = abs.Secs % 60;

        response[0] = 0;
        response[1] = 0;
        response[2] = 6;
        if ( g_littleEndian )
        {
        	response[6] = minutes>>24;
        	response[5] = (minutes>>16) & 0xff;
        	response[4] = (minutes>>8) & 0xff;
        	response[3] = (minutes) & 0xff;
        }
        else
        {
        	response[3] = minutes>>24;
        	response[4] = (minutes>>16) & 0xff;
        	response[5] = (minutes>>8) & 0xff;
        	response[6] = (minutes) & 0xff;
        }
    	response[7] = seconds;
        response[8] = 0;
        response[9] = getchecksum(&response[3], 6);
        *pLength = 6+4;
    }
    else if ( tableId == 2048 )
    {
        response[0] = 0;
        response[1] = (Unsigned8)(sizeof(g_table2048)>>8);    // 400 byte table
        response[2] = (Unsigned8)(sizeof(g_table2048) & 0xFF);
        memcpy(&response[3], g_table2048, sizeof(g_table2048));
        response[3+sizeof(g_table2048)] = getchecksum(&response[3], sizeof(g_table2048));
        *pLength = 4+sizeof(g_table2048);
    }
    else
    {
    	return C1222RESPONSE_ERR;
    }

    return C1222RESPONSE_OK;
}


Unsigned8 C1219TableServer_ReadOffset(C1219TableServer* p, Unsigned16 tableID, Unsigned24 offset, Unsigned16 length,  Unsigned8* buffer, Unsigned16* pLength)
{    
    static Unsigned8 myBuffer[1000];
    Unsigned16 fullLength;
    Unsigned8 result;
    
    *pLength = 1;
    
    result = C1219TableServer_Read(p, tableID, myBuffer, 1000, &fullLength);
    
    if ( result == C1222RESPONSE_OK )
    {
        buffer[0] = 0;
        buffer[1] = (Unsigned8)(length>>8);
        buffer[2] = (Unsigned8)(length & 0xff);
        memcpy(&buffer[3], &myBuffer[offset+3], length);
        buffer[3+length] = getchecksum(&buffer[3], length);
        
        *pLength = (Unsigned16)(length + 4);
    }
    
    
    
    return result;
}


Unsigned8 C1219TableServer_Write(C1219TableServer* p, Unsigned16 tableId, Unsigned16 length, Unsigned8* buffer)
{
    if ( p->isTest )
        return C1222RESPONSE_OK;
        
    if ( p->isNotification )
        return C1222RESPONSE_IAR;
        
    if ( tableId == 5 )
    {
        if ( length > 20 )
            length = 20;

        memcpy(g_myId, buffer, length);
        return C1222RESPONSE_OK;
    }
    else if ( tableId == 2048 )
    {
        if ( length > sizeof(g_table2048) )
            length = sizeof(g_table2048);

        memcpy(g_table2048, buffer, length);
        return C1222RESPONSE_OK;
    }

    return C1222RESPONSE_IAR;
}


Unsigned8 C1219TableServer_WriteOffset(C1219TableServer* p, Unsigned16 tableId, Unsigned24 offset, Unsigned16 length, Unsigned8* buffer)
{
	(void)p;
    
    if ( tableId == 2048 )
    {
        if ( (offset+length) <= sizeof(g_table2048) )
        {
            memcpy(&g_table2048[offset], buffer, length);
            return C1222RESPONSE_OK;
        }
    }
    
    return C1222RESPONSE_SNS;
}


void C1219TableServer_EndUserAccess(C1219TableServer* p)
{
    // nothing to do in this test
    (void)p;
}





///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// End of test implementation of the c12.19 table server
//
///////////////////////////////////////////////////////////////////////////////////////////////////////



void C1219TableServer_GetClientUserAndPassword(C1219TableServer* p, Unsigned16* pUserId, Unsigned8* password)
{
    (void)p;
    *pUserId = 100;
    
    memset(password,0,20);
}


void Table00_GetMeterDeviceClass(Unsigned8* deviceClass)
{
    memcpy(deviceClass, "ITRN", 4);
}




void Table06_GetMyIdentification(Unsigned8* pmyId)
{
    memcpy( pmyId, "Test5678901234567890", 20 );    
}



Unsigned8 Table122_GetNumberOfMulticastAddresses(void)
{
    return 1;   // TEMP - TODO
}


Boolean Table122_GetMulticastAddress(Unsigned8 index, C1222ApTitle* pApTitle)
{
    // TODO
    
    static Unsigned8 aMulticastAddress[] = { 0x0d, 0x04, 0x44, 0x21, 0x00, 0x10 };
    
    if ( index == 0 )
    {
       pApTitle->buffer = aMulticastAddress;
       pApTitle->maxLength = pApTitle->length = sizeof(aMulticastAddress);
       
       return TRUE;
    }
    
    return FALSE;
}

        






// TODO - modify the next 2 routines to return key information from table 46.


Unsigned8 Table46_GetNumberOfKeys(void)
{
    // TEMP - TODO
    
    return 2;
}



Boolean Table46_GetKey(Unsigned8 keyId, Unsigned8* pCipher, Unsigned8* key)
{
    // TEMP!!  TODO
    
    static Unsigned8 key0[] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 };
    static Unsigned8 key1[] = { 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12,
                              0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
                              0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12 };
    
    if ( keyId == 0 )
    {
        *pCipher = 0;
        memcpy(key, key0, sizeof(key0));
        return TRUE;
    }
    else if ( keyId == 1 )
    {
        *pCipher = 1;
        memcpy(key, key1, sizeof(key1));
        return TRUE;
    }
    
    return FALSE;
}


