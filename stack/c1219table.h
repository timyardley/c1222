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


#ifndef C1219TABLESERVER_H
#define C1219TABLESERVER_H
#include "acsemsg.h"


    

typedef struct C1219TableServerTag
{
    Unsigned16 userID;
    
    Boolean isNotification;
    Boolean isTest;
    
#ifdef C1219INREGISTER
    Unsigned32	Interrogate_Time;
    Boolean     SecuritySuccessful;
    Boolean     IsLoggedOn;

    QUEUE			Output_Queue;
    QUEUE			Input_Queue;
    SEMA			No_Connect;
    SEMA			No_More_Data;
#endif

    C1222StandardVersion requestStandard;     
    
} C1219TableServer;

void C1219TableServer_StartUserAccess(C1219TableServer* p, Unsigned16 userId, Unsigned8* password);
Unsigned8 C1219TableServer_Read(C1219TableServer* p, Unsigned16 tableID, Unsigned8* buffer, Unsigned16 maxLength, Unsigned16* pLength);
Unsigned8 C1219TableServer_ReadOffset(C1219TableServer* p, Unsigned16 tableID, Unsigned24 offset, Unsigned16 length,  Unsigned8* buffer, Unsigned16* pLength);
Unsigned8 C1219TableServer_Write(C1219TableServer* p, Unsigned16 tableID, Unsigned16 length, Unsigned8* buffer);
Unsigned8 C1219TableServer_WriteOffset(C1219TableServer* p, Unsigned16 tableID, Unsigned24 offset, Unsigned16 length, Unsigned8* buffer);
void C1219TableServer_EndUserAccess(C1219TableServer* p);
void C1219TableServer_NoteTest(C1219TableServer* p, Boolean isTest);
void C1219TableServer_NoteNotification(C1219TableServer* p, Boolean isNotification);
Unsigned32 C1219TableServer_FullTableReadSize(C1219TableServer* p, Unsigned16 tableID);
void C1219TableServer_GetClientUserAndPassword(C1219TableServer* p, Unsigned16* pUserId, Unsigned8* password);

void C1219TableServer_NotePowerUp(C1219TableServer* p, Unsigned32 outageDurationInSeconds);

void C1219TableServer_NoteRequestStandard(C1219TableServer* p, C1222StandardVersion version);


#endif
