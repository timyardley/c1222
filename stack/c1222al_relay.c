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



#include "c1222environment.h"
#include "c1222al.h"
#include "c1222tl.h"
#include "c1222response.h"
#include "c1222misc.h"


#ifdef C1222_INCLUDE_RELAY

// this is the c1222 application code for the relay

static void RelayIdleState(C1222AL* p);




static void RelaySendingMessage(C1222AL* p)
{
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    if ( (C1222Misc_GetFreeRunningTimeInMS()-p->status.sendStartTime) > (p->status.sendTimeOutSeconds*1000L) )
    {
        p->counters.sendTimedOut++;
        
        C1222AL_LogEvent(p, C1222EVENT_AL_SENDTIMEDOUT);
        
        C1222AL_TL_NoteFailedSendMessage(p);
    }
    else
        C1222TL_Run( pTransport );
}




void RunRelay(C1222AL* p)
{   
    switch( p->status.state )
    {
        case ALS_INIT:
        	p->status.state = ALS_IDLE;
            // flow through to allow faster processing
            
        case ALS_IDLE:
            RelayIdleState(p);
            break;
            
        case ALS_SENDINGMESSAGE:
            RelaySendingMessage( p );
            break;
            
        default:
            p->status.state = ALS_INIT;
            break;            
    }
}







static void RelayIdleState(C1222AL* p)
{
    Unsigned32 now;
    
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    now = C1222Misc_GetFreeRunningTimeInMS();
    
        
    // wait until registered (poll transport status until interface is enabled)
    // then get registration status
    
    // assume automatic registration for now !
    
    if ( C1222TL_IsReady(pTransport) )
    {
            if ( !p->status.haveInterfaceInfo )
            {
                if ( p->status.deviceIdleState != ALIS_GETTINGINTERFACESTATUS )
                {
                    p->status.deviceIdleState = ALIS_GETTINGINTERFACESTATUS;
                    C1222AL_LogEvent(p, C1222EVENT_AL_SENT_GET_INTF_STATUS);
                    C1222TL_SendGetStatusRequest(pTransport, 1); // interface info only
                    p->status.lastRequestTime = now;
                }
                else
                {
                    if ( (now-p->status.lastRequestTime) > 2000 )
                    {
                        C1222AL_LogEvent(p, C1222EVENT_AL_SENT_GET_INTF_STATUS);
                        C1222TL_SendGetStatusRequest(pTransport, 1);    
                        p->status.lastRequestTime = now;
                    }  
                }
            }
            else if ( p->status.configurationChanged )
            {
                if ( p->status.deviceIdleState != ALIS_SENDINGRELOADCONFIGREQUEST )
                {
                    p->status.deviceIdleState = ALIS_SENDINGRELOADCONFIGREQUEST;
                    C1222AL_LogEvent(p, C1222EVENT_AL_SENT_LINK_CTRL_RELOAD_CONFIG);
                    C1222TL_SendLinkControlRequest(pTransport, C1222TL_RELOAD_CONFIG_FLAG);
                    p->status.lastRequestTime = now;
                    //p->status.isRegistered = FALSE;
                }
                else
                {
                    if ( (now-p->status.lastRequestTime) > 2000 )
                    {
                        C1222AL_LogEvent(p, C1222EVENT_AL_SENT_LINK_CTRL_RELOAD_CONFIG);
                        C1222TL_SendLinkControlRequest(pTransport, C1222TL_RELOAD_CONFIG_FLAG);    
                        p->status.lastRequestTime = now;
                    }  
                }
            }
            else
                p->status.deviceIdleState = ALIS_IDLE;
                      
    }
    
    C1222TL_Run(pTransport);
}



#endif // C1222_INCLUDE_RELAY


