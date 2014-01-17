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



// C12.22 Application layer - device support

#include "c1222environment.h"
#include "c1222al.h"
#include "c1222tl.h"
#include "c1222response.h"
#include "c1222misc.h"
#include "c1222statistic.h"

//static void DeviceWaitForRegistrationStatus(C1222AL* p);
static void DeviceIdleState(C1222AL* p);
static Unsigned8 DeviceSendLinkControlNow(C1222AL* p, Unsigned8 linkControlByte);
static void DeviceHandleRegistration(C1222AL* p);
static void DeviceHandleDeRegistration(C1222AL* p);
static Boolean DeviceSendRegistrationRequestToNetwork(C1222AL* p);
static Boolean DeviceSendDeRegistrationRequestToNetwork(C1222AL* p);
static void DeviceWaitForRegistrationResponse(C1222AL* p);
static void DeviceWaitForDeRegistrationResponse(C1222AL* p);
static void DeviceHandleRegistrationScheduling(C1222AL* p);
static void DeviceHandleTimeToRegister(C1222AL* p);
static void DeviceHandleTimeToDeRegister(C1222AL* p);

extern Unsigned8 Table46_GetNumberOfKeys(void);
extern Boolean Table46_GetKey(Unsigned8 keyId, Unsigned8* pCipher, Unsigned8* key);



static void DeviceSendingMessage(C1222AL* p)
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



void RunDevice(C1222AL* p)
{       
    switch( p->status.state )
    {
        case ALS_INIT:
        	p->status.state = ALS_IDLE;
            // flow through to allow faster processing
            
        case ALS_IDLE:
            DeviceIdleState(p);
            break;
            
        case ALS_SENDINGMESSAGE:
            DeviceSendingMessage(p);
            break;
                        
        case ALS_REGISTERING:
            DeviceWaitForRegistrationResponse(p);
            break;

        case ALS_DEREGISTERING:
            DeviceWaitForDeRegistrationResponse(p);
            break;
    }
}






static Boolean ConfigurationIsRegisterable(C1222AL* p)
{
    Unsigned8 apLength;
    Unsigned8 esnLength;
    
    apLength = C1222Misc_GetUIDLength(p->transportConfig.nodeApTitle);
    esnLength = C1222Misc_GetUIDLength(p->transportConfig.esn);
    
    if ( (apLength > C1222_APTITLE_LENGTH) || (esnLength > C1222_APTITLE_LENGTH) )
        return FALSE;
    
    // check for static node aptitle
    
    if ( apLength > 2 )
        return TRUE;
        
    
    // check for esn valid
    
    if ( esnLength > 2 )
        return TRUE;
        
    return FALSE;   
}



static void SetIdleState(C1222AL* p, C1222AL_IdleState state)
{
    p->status.deviceIdleState = state;
}


static void DeviceTurnOffAutoRegistration(C1222AL* p)
{
    Unsigned32 now;
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    now = C1222Misc_GetFreeRunningTimeInMS();
    
    // wait for configuration to change
    if ( p->status.deviceIdleState != ALIS_TURNINGOFFAUTOREGISTRATION )
    {
        C1222AL_LogEvent(p, C1222EVENT_AL_SEND_LINK_CTRL_DISABLE_AUTO_REG);
        C1222TL_SendLinkControlRequest(pTransport, C1222TL_AUTOREG_CTRL_DISABLE_AUTO_REGISTRATION);
        p->status.lastRequestTime = now;
        SetIdleState( p,  ALIS_TURNINGOFFAUTOREGISTRATION);
        p->status.haveInterfaceInfo = FALSE;
    }
    else
    {
        if ( (now-p->status.lastRequestTime) > 5000 )
        {
            C1222AL_LogEvent(p, C1222EVENT_AL_SEND_LINK_CTRL_DISABLE_AUTO_REG);
            C1222TL_SendLinkControlRequest(pTransport, C1222TL_AUTOREG_CTRL_DISABLE_AUTO_REGISTRATION);
            p->status.lastRequestTime = now;
        }
    }             
}


static void DeviceGetSynchronizationState(C1222AL* p)
{
    Unsigned32 now;
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    Unsigned32 delay = 10000L;
    
    if ( ! p->status.haveRevInfo )
        delay = 5000;
    
    now = C1222Misc_GetFreeRunningTimeInMS();
        
    if ( p->status.deviceIdleState != ALIS_GETTINGSYNCHRONIZATIONSTATE )
    {
        SetIdleState( p, ALIS_GETTINGSYNCHRONIZATIONSTATE);
        C1222AL_LogEvent(p, C1222EVENT_AL_SEND_GET_INTF_INFO_AND_STATS);
        C1222TL_SendGetStatusRequest(pTransport, 0); // interface info and statistics
        p->status.lastRequestTime = now;
    }
    else
    {
        if ( (now-p->status.lastRequestTime) > delay )  // the rflan now only sets synced when it is synced through to the cell master which can take up to an hour - so don't get too busy here
        {
            C1222AL_LogEvent(p, C1222EVENT_AL_SEND_GET_INTF_INFO_AND_STATS);
            C1222TL_SendGetStatusRequest(pTransport, 0); // interface info and statistics
            p->status.lastRequestTime = now;
        }
    }
}





// this routine handles waiting to register after a powerup

static void DeviceHandleWaitingToRegister(C1222AL* p)
{
    Unsigned32 now;
    Unsigned32 minutesSinceRegistration;
    
    now = C1222Misc_GetFreeRunningTimeInMS();
    
    minutesSinceRegistration = (now-p->status.lastSuccessfulRegistrationTime)/60000L + 1;
    
    if ( p->registrationStatus.regPeriodMinutes < 5 )
    {
        // might as well register now
        p->registrationStatus.regCountDownMinutes = 0;
        p->status.timeToRegister = TRUE;
    }
    
    else if ( minutesSinceRegistration >= (Unsigned32)(p->registrationStatus.regPeriodMinutes - 5) )
    {
        p->registrationStatus.regCountDownMinutes = 0;
        p->status.timeToRegister = TRUE;
    }
    else
    { 
        p->registrationStatus.regCountDownMinutes = 
                 (Unsigned16)(p->registrationStatus.regPeriodMinutes - minutesSinceRegistration);
    }
    
    if ( ((now-p->status.lastPowerUp) > (Unsigned32)(p->status.powerUpDelay*1000L)) &&
         ((now-(p->status.lastUnSynchronizedTime)) > (Unsigned32)(p->status.powerUpDelay*1000L)) )
    {
        if ( p->status.powerUpDelay > 0 )
        {
            p->status.timeToRegister = TRUE;
            p->status.powerUpDelay = 0;
        }
    }
}



static void DeviceGetInterfaceStatus(C1222AL* p)
{
    Unsigned32 now;
    
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    now = C1222Misc_GetFreeRunningTimeInMS();
    
    if ( p->status.deviceIdleState != ALIS_GETTINGINTERFACESTATUS )
    {
        SetIdleState( p, ALIS_GETTINGINTERFACESTATUS);
        C1222AL_LogEvent(p, C1222EVENT_AL_SENT_GET_INTF_STATUS);
        C1222TL_SendGetStatusRequest(pTransport, 1); // interface info only
        p->status.lastRequestTime = now;
    }
    else
    {
        if ( (now-p->status.lastRequestTime) > 5000 )
        {
            C1222AL_LogEvent(p, C1222EVENT_AL_SENT_GET_INTF_STATUS);
            C1222TL_SendGetStatusRequest(pTransport, 1);    
            p->status.lastRequestTime = now;
        }  
    }
}


static void DeviceNotifyCommModuleOfChangedConfig(C1222AL* p)
{
    Unsigned32 now;
    
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    now = C1222Misc_GetFreeRunningTimeInMS();

    if ( p->status.deviceIdleState != ALIS_SENDINGRELOADCONFIGREQUEST )
    {
        SetIdleState( p, ALIS_SENDINGRELOADCONFIGREQUEST);
        C1222AL_LogEvent(p, C1222EVENT_AL_SENT_LINK_CTRL_RELOAD_CONFIG);
        C1222TL_SendLinkControlRequest(pTransport, C1222TL_RELOAD_CONFIG_FLAG);
        p->status.lastRequestTime = now;
    }
    else
    {
        if ( (now-p->status.lastRequestTime) > 5000 )
        {
            C1222AL_LogEvent(p, C1222EVENT_AL_SENT_LINK_CTRL_RELOAD_CONFIG);
            C1222TL_SendLinkControlRequest(pTransport, C1222TL_RELOAD_CONFIG_FLAG);    
            p->status.lastRequestTime = now;
        }  
    }
}




static void DeviceHandleKeepAliveRegistration(C1222AL* p)
{
    Unsigned32 now;
    Unsigned16 minutes;
    
    now = C1222Misc_GetFreeRunningTimeInMS();
    
    if ( p->registrationStatus.regPeriodMinutes != 0  )
    {
        // currently registered - handle keep alive
        
        if ( p->registrationStatus.regCountDownMinutes == 0 )
        {
            if ( p->status.state != ALS_REGISTERING )
            {
                if ( ((now-(p->status.lastRegistrationAttemptTime)) > (p->status.registrationRetrySeconds*1000L)) &&
                     ((now-(p->status.lastUnSynchronizedTime)) > (p->status.registrationRetrySeconds*1000L)) )
                {
                    C1222AL_LogEvent(p, C1222EVENT_AL_TIME_TO_REFRESH_REGISTRATION);
                    p->status.timeToRegister = TRUE;
                }
            }
        }
        else
        {
            minutes = (Unsigned16)((now - p->status.lastSuccessfulRegistrationTime)/60000L);

            if ( minutes < p->registrationStatus.regPeriodMinutes )
                p->registrationStatus.regCountDownMinutes = (Unsigned16)(p->registrationStatus.regPeriodMinutes - minutes);
            else
                p->registrationStatus.regCountDownMinutes = 0;
        }
    }
}




Boolean RegistrationIsAllowed(C1222AL* p)
{
    return (Boolean)(ConfigurationIsRegisterable(p) && !p->status.doNotRegister);
}



static void DeviceRefreshStatistics(C1222AL* p)
{
    Unsigned32 now;
    
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    now = C1222Misc_GetFreeRunningTimeInMS();
                
    if ( (now-p->status.lastRequestTime) > 10000 )
    {
        C1222AL_LogEvent(p, C1222EVENT_AL_SEND_GET_INTF_INFO_AND_STATS);
        C1222TL_SendGetStatusRequest(pTransport, 0); // interface info and statistics
        p->status.lastRequestTime = now;
    }
}                






static void DeviceIdleState(C1222AL* p)
{
    Unsigned32 now;
    
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    now = C1222Misc_GetFreeRunningTimeInMS();
    
    if ( ! p->status.isSynchronizedOrConnected )
    {
        p->status.lastUnSynchronizedTime = now;  // for use in registration algorithm
        p->status.timeToRegister = FALSE;
    }
    
    
    // wait 5 seconds after powerup to get the rflan time to read its configuration
    // from factory data
    
    if ( C1222TL_IsReady(pTransport) && 
         ((now-p->status.lastPowerUp) > 5000) && 
         !C1222TL_IsOptionalCommInhibited(pTransport) &&
         !p->status.tryingToGetRFLANRevisionInfo )
    {
        // turn off auto registration if on
        
        if ( p->transportStatus.interfaceStatus & C1222_TL_STATUS_AUTO_REGISTRATION_FLAG )
        {
            DeviceTurnOffAutoRegistration(p);
        }
        else if ( !p->status.isSynchronizedOrConnected )
        {
            DeviceGetSynchronizationState(p);
        }
        else if ( RegistrationIsAllowed(p) 
                  && (p->status.timeToRegister || !p->status.isRegistered) ) 
        {
            p->status.waitingToRegister = FALSE; // ??
            
            DeviceHandleRegistration(p); 
            
            DeviceRefreshStatistics(p);                           
        }
        else if ( !C1222AL_IsBusy(p) ) // yes I am registered and not in the middle of something
        {
            if ( !p->status.haveInterfaceInfo )
            {
                DeviceGetInterfaceStatus(p);
            }
            else if ( p->status.configurationChanged )
            {
                DeviceNotifyCommModuleOfChangedConfig(p);
            }
            else 
            {
                 SetIdleState(p, ALIS_IDLE);
                 
                if ( p->status.waitingToRegister && RegistrationIsAllowed(p) )
                {
                    // we are waiting to register after a powerup that was not too long
                    // wait until the registration period expires and then register
                
                    DeviceHandleWaitingToRegister(p);  
                }
                else if ( p->status.timeToDeRegister || p->status.deRegistrationInProgress )
                {
                    DeviceHandleDeRegistration(p);
                }
                else if ( p->status.isRegistered )
                {
                    DeviceHandleKeepAliveRegistration(p);
                }
                
                // should check status once in a while
                // should get statistics once in a while
                
                DeviceRefreshStatistics(p);                
            }
        } // end if not busy
    }
    else
        SetIdleState(p, ALIS_IDLE);
    
    C1222TL_Run(pTransport);
}



Unsigned8 C1222AL_DeviceRegisterNow(C1222AL* p)
{    
    // returns an immediate response
    // this is used to start a registration request (proc 23)
    // table 8 read following a proc 23 should check current status of the request
    
    // here are some variables that are involved
    //  waitingToRegister
    //  doNotRegister
    //  isSynchronizedOrConnected
    //  sendingRegistrationRequest (this is only in comm module)
    //  registrationInProgress
    //  state
    //  isRegistered
    //  timeToRegister
    
    if ( !p->status.isSynchronizedOrConnected )
    {
        return C1222RESPONSE_DNR;
    }
    else if ( p->status.registrationInProgress )
    {
        return C1222RESPONSE_BSY;
    }

    p->status.timeToRegister = TRUE;
    p->status.doNotRegister = FALSE;
    p->status.waitingToRegister = FALSE;
    p->status.inProcessProcedure = 23; 
    p->status.inProcessProcedureState = C1222PS_RUNNING;   
        
    return C1222RESPONSE_OK;    
}




Unsigned8 C1222AL_DeviceDeRegisterNow(C1222AL* p)
{    
    // returns an immediate response
    // this is used to start a deregistration request (proc 24)
    // table 8 read following a proc 24 should check current status of the request
        
    // I'm not sure this is the way that it should be done - an alternative would 
    // be to do a procedure write to the comm module, but how do I know that it
    // handles procedures - I guess I could read table 0 at startup? 
    
    if ( !p->status.isSynchronizedOrConnected )
    {
        return C1222RESPONSE_DNR;
    }
    else if ( p->status.registrationInProgress || p->status.deRegistrationInProgress )
    {
        return C1222RESPONSE_BSY;
    }
    else if ( !p->status.isRegistered && p->status.doNotRegister )
    {
        return C1222RESPONSE_ISSS;
    }

    p->status.timeToDeRegister = TRUE;
    p->status.inProcessProcedure = 24; 
    p->status.inProcessProcedureState = C1222PS_RUNNING;   
        
    return C1222RESPONSE_OK;        
}



Unsigned8 C1222AL_DeviceSendLinkControlNow(C1222AL* p, Unsigned8 linkControlByte)
{
    Unsigned8 result;
    
    linkControlByte &= 0x7F;  // turn off the all interfaces bit since it means something else downstream
    
    result = DeviceSendLinkControlNow(p, linkControlByte);
    
    if ( result == C1222RESPONSE_OK )
    {
        p->status.inProcessProcedure = 25;
        p->status.linkControlRunning = TRUE;
    }
        
    return result;    
}





static Unsigned8 DeviceSendLinkControlNow(C1222AL* p, Unsigned8 linkControlByte)
{
    // returns an immediate response
    // this is used to start a link control request (proc 25)
    // table 8 read following a proc 24 should check current status of the request
    
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    
    if ( C1222AL_IsBusy(p) )
    {
        return C1222RESPONSE_BSY;
    }
    
    if ( !p->status.isSynchronizedOrConnected )
    {
        return C1222RESPONSE_ONP; // correct result code?
    } 
    
    // I'm not sure this is the way that it should be done - an alternative would 
    // be to do a procedure write to the comm module, but how do I know that it
    // handles procedures - I guess I could read table 0 at startup?  
    
    C1222TL_SendLinkControlRequest( pTransport, linkControlByte);
    
    // maybe should wait for response before changing state?
    
    switch ( (linkControlByte>>C1222TL_INTERFACECONTROL_SHIFT) & 0x03 )
    {
    case 1:
        p->status.interfaceEnabled = TRUE;
        p->status.doNotRegister = FALSE;
        break;
    case 2:
        p->status.interfaceEnabled = FALSE;
        p->status.doNotRegister = TRUE;
        break;
    default:
        break;
    }
    
    switch( (linkControlByte>>C1222TL_AUTOREGISTRATIONCONTROL_SHIFT) & 0x03 )
    {
    case 1:
        p->status.autoRegistration = FALSE;
        break;
    case 2:
        //p->status.autoRegistration = TRUE;  // can't turn on auto registration
        break;
    default:
        break;
    }
    

    switch( (linkControlByte>>C1222TL_DIRECTMESSAGINGCONTROL_SHIFT) & 0x03 )
    {
    case 1:
        p->status.useDirectMessaging = TRUE;
        break;
    case 2:
        p->status.useDirectMessaging = FALSE;
        break;
    }
    
    if ( (linkControlByte>>C1222TL_RESETSTATISTIC_SHIFT) & 0x01 )
    {
        // reset all statistics
        C1222AL_ResetAllStatistics(p);
    }
    
    return C1222RESPONSE_OK;
}






Unsigned8 C1222AL_DeviceGetRegisterNowResult(C1222AL* p)
{
    // returns busy if still in progress, else the psem response code from the register procedure
    
    Unsigned8 result = C1222RESPONSE_IAR;
    
    if ( (p->status.inProcessProcedureState != C1222PS_IDLE) &&
         (p->status.inProcessProcedure == 23) )
    {
        if ( p->status.inProcessProcedureState != C1222PS_COMPLETE )
            result = C1222RESPONSE_BSY;
        else
        {
            p->status.inProcessProcedureState = C1222PS_IDLE;
            result = p->status.inProcessProcedureResponse;
        }
    }
    
    return result;
}


Unsigned8 C1222AL_DeviceGetDeRegisterNowResult(C1222AL* p)
{
    // returns busy if still in progress, else the psem response code from the register procedure
    
    Unsigned8 result = C1222RESPONSE_IAR;
    
    if ( (p->status.inProcessProcedureState != C1222PS_IDLE) &&
         (p->status.inProcessProcedure == 24) )
    {
        if ( p->status.inProcessProcedureState != C1222PS_COMPLETE )
            result = C1222RESPONSE_BSY;
        else
        {
            p->status.inProcessProcedureState = C1222PS_IDLE;
            result = p->status.inProcessProcedureResponse;
        }
    }
    
    return result;    
}


Unsigned8 C1222AL_DeviceGetSendLinkControlNowResult(C1222AL* p)
{
    // returns busy if still in progress, else the psem response code from the register procedure
    
    Unsigned8 result = C1222RESPONSE_IAR;
    
    if ( p->status.inProcessProcedure == 25 )
    {
        if ( p->status.linkControlRunning )
            result = C1222RESPONSE_BSY;
        else
            result = p->status.linkControlResponse;
    }
    
    return result;
}




Unsigned8 C1222AL_GetNumberOfKeys(C1222AL* p)
{
    (void)p;
    
    return Table46_GetNumberOfKeys();
}




Boolean C1222AL_GetKey(C1222AL* p, Unsigned8 keyId, Unsigned8* pCipher, Unsigned8* key)
{
    (void)p;
    
    return Table46_GetKey(keyId, pCipher, key);
}




void NoteRegistrationAttemptComplete(C1222AL* p, Unsigned8 result)
{    
    if ( p->status.inProcessProcedureState != C1222PS_IDLE )
    {
        p->status.inProcessProcedureResponse = result;
        p->status.inProcessProcedureState = C1222PS_COMPLETE;
    }
}



void C1222AL_DeviceNoteRegistrationAttemptComplete(C1222AL* p, Unsigned8 result)
{
    NoteRegistrationAttemptComplete(p, result);    
}




void NoteDeRegistrationAttemptComplete(C1222AL* p, Unsigned8 result)
{   
    if ( p->status.inProcessProcedureState != C1222PS_IDLE )
    {
        p->status.inProcessProcedureResponse = result;
        p->status.inProcessProcedureState = C1222PS_COMPLETE;
    }
}



void C1222AL_DeviceNoteDeRegistrationAttemptComplete(C1222AL* p, Unsigned8 result)
{
    NoteDeRegistrationAttemptComplete(p, result);    
}






// this uses the message receiver buffer to construct the registration message
// I am going to try to continue that to avoid another buffer allocation

static Boolean DeviceSendRegistrationRequestToNetwork(C1222AL* p)
{
    ACSE_Message* pMsg;
    C1222MessageLogEntry logEntry;
    
    p->counters.registrationAttempts++;
    
    pMsg = C1222AL_MakeRegistrationMessage(p);
    
    if ( pMsg != 0 )
    {
        // since we are in the device, we need to send the message not just say it is available
        
        C1222AL_MakeMessageLogEntry(p, pMsg, &logEntry, TRUE, C1222_MSGTYPE_REGISTER_REQUEST);  // this is a receiving log entry
        C1222AL_LogComEventAndBytes(p, COMEVENT_SENT_MESSAGE_TO, &logEntry, sizeof(logEntry));        
        
    
        if ( C1222AL_SendMessage( p, pMsg, p->transportConfig.relayNativeAddress, p->transportConfig.relayAddressLength) )
            return TRUE;
            
        NoteRegistrationAttemptComplete(p, C1222RESPONSE_NETR);

        C1222AL_LogEvent(p, C1222EVENT_AL_SEND_REGISTRATION_FAILED);
        
        return FALSE;
    }
    
    NoteRegistrationAttemptComplete(p, C1222RESPONSE_ERR);
    
    return FALSE;
}



static Boolean DeviceSendDeRegistrationRequestToNetwork(C1222AL* p)
{
    ACSE_Message* pMsg;
    C1222MessageLogEntry logEntry;
    
    p->counters.deRegistrationAttempts++;
    
    pMsg = C1222AL_MakeDeRegistrationMessage(p);
    
    if ( pMsg != 0 )
    {
        C1222AL_MakeMessageLogEntry(p, pMsg, &logEntry, TRUE, C1222_MSGTYPE_DEREGISTER_REQUEST);  // this is a receiving log entry
        C1222AL_LogComEventAndBytes(p, COMEVENT_SENT_MESSAGE_TO, &logEntry, sizeof(logEntry));        
        
        // since we are in the device, we need to send the message not just say it is available
    
        if ( C1222AL_SendMessage( p, pMsg, p->transportConfig.relayNativeAddress, p->transportConfig.relayAddressLength) )
            return TRUE;
            
        NoteDeRegistrationAttemptComplete(p, C1222RESPONSE_BSY);

        C1222AL_LogEvent(p, C1222EVENT_AL_SEND_DEREGISTRATION_FAILED);
    }
    
    NoteDeRegistrationAttemptComplete(p, C1222RESPONSE_ERR);
    
    return FALSE;
}






static void DeviceWaitForRegistrationResponse(C1222AL* p)
{
	Unsigned32 now;
	
	// need to check once in awhile if it has been too long and send the request again
	
	now = C1222Misc_GetFreeRunningTimeInMS();
	
	if ( ((now-p->status.lastRegistrationAttemptTime) > (p->status.registrationRetrySeconds*1000L)) &&
	     ((now-(p->status.lastUnSynchronizedTime)) > (p->status.registrationRetrySeconds*1000L)) )  // wait 20 secs //2 minutes for now
	{
	    p->status.timeToRegister = TRUE;
	    p->status.state = ALS_IDLE;
	    p->status.registrationInProgress = FALSE;
	    
	    //C1222AL_SetRegistrationRetryTime(p);
	}
	else if ( (now-p->status.lastRegistrationAttemptTime) > (60000L) )
	{
	    // go back to idle state while we are waiting to try again to allow sending messages to the
	    // comm module etc
	    p->status.state = ALS_IDLE;
	    p->status.registrationInProgress = FALSE;
	    
	    C1222AL_IncrementStatistic(p, C1222_STATISTIC_NUMBER_OF_REGISTRATIONS_FAILED);
	    
	    NoteRegistrationAttemptComplete(p, C1222RESPONSE_DNR);	    
	}
    else
	{
	    // let the transport layer run to keep the device happy
	    // will need to handle messages from the device now in this state!
	    
	    
	    C1222TL_Run( (C1222TL*)(p->pTransport) );
	}
}






static void DeviceWaitForDeRegistrationResponse(C1222AL* p)
{
	Unsigned32 now;
	
	// need to check once in awhile if it has been too long and send the request again
	
	now = C1222Misc_GetFreeRunningTimeInMS();
	
	if ( ((now-p->status.lastDeRegistrationAttemptTime) > (p->status.registrationRetrySeconds*1000L)) &&
	     ((now-p->status.lastUnSynchronizedTime) > (p->status.registrationRetrySeconds*1000L)) )  // wait 20 secs //2 minutes for now
	{
	    p->status.timeToDeRegister = TRUE;
	    p->status.state = ALS_IDLE;
	    p->status.deRegistrationInProgress = FALSE;
	    
	    //C1222AL_SetRegistrationRetryTime(p);
	}
	else if ( (now-p->status.lastDeRegistrationAttemptTime) > (60000L) )
	{
	    // go back to idle state while we are waiting to try again to allow sending messages to the
	    // comm module etc
	    p->status.state = ALS_IDLE;
	    p->status.deRegistrationInProgress = FALSE;
	    	    
	    NoteDeRegistrationAttemptComplete(p, C1222RESPONSE_DNR);	    
	}
    else
	{
	    // let the transport layer run to keep the device happy
	    // will need to handle messages from the device now in this state!
	    
	    
	    C1222TL_Run( (C1222TL*)(p->pTransport) );
	}
}








static void DeviceHandleRegistrationScheduling(C1222AL* p)
{    
    C1222TL* pTransport = (C1222TL*)(p->pTransport);
    Unsigned32 now = C1222Misc_GetFreeRunningTimeInMS();
    
    if ( !C1222TL_IsReady(pTransport) )
        return;
        
    if ( C1222AL_IsBusy(p) )
        return; 
        
    if ( !p->status.isSynchronizedOrConnected )
    {
        p->status.registrationFailureReason = MFC_REG_NOT_SYNCHRONIZED;
        p->status.timeToRegister = FALSE;
        return;
    }       
    
    if ( p->status.timeToRegister || p->status.registrationInProgress )
    {
        return;
    }
    
    if ( !p->status.isRegistered )
    {
        // in order to register, need an esn or an aptitle
        
        if ( !RegistrationIsAllowed(p) )
        {
            // an event here would be too busy since this will be called lots
            
            return;
        }
        
        
        // if not registered, better register
        
        if ( ((now-(p->status.lastRegistrationAttemptTime)) > (p->status.registrationRetrySeconds*1000L)) &&
             ((now-(p->status.lastUnSynchronizedTime)) > (p->status.registrationRetrySeconds*1000L)) )
        {
            C1222AL_LogEvent(p, C1222EVENT_AL_TIME_TO_REGISTER);
            p->status.timeToRegister = TRUE;
            //C1222AL_SetRegistrationRetryTime(p);
        }
    }
}






static void DeviceHandleTimeToRegister(C1222AL* p)
{         
    if ( p->status.timeToRegister && p->status.haveConfiguration && (!C1222AL_IsBusy(p))
         && p->status.isReceivedMessageHandlingComplete && !p->status.isReceivedMessageComplete )
    {           
        if ( DeviceSendRegistrationRequestToNetwork(p) )
        {                    
            p->status.registrationInProgress = TRUE;
            p->status.registrationFailureReason = MFG_REG_DEREG_PENDING_OR_TIMEOUT;  // pending or timeout
        
            p->status.lastRegistrationAttemptTime = C1222Misc_GetFreeRunningTimeInMS();
            C1222AL_SetRegistrationRetryTime(p);
        }
        
        p->status.timeToRegister = FALSE;
    }
}



static void DeviceHandleTimeToDeRegister(C1222AL* p)
{             
    if ( p->status.timeToDeRegister && p->status.haveConfiguration && (!C1222AL_IsBusy(p))
         && p->status.isReceivedMessageHandlingComplete && !p->status.isReceivedMessageComplete )
    {           
        if ( DeviceSendDeRegistrationRequestToNetwork(p) )
        {        
            p->status.deRegistrationInProgress = TRUE;
            p->status.deRegistrationFailureReason = MFG_REG_DEREG_PENDING_OR_TIMEOUT;  // pending or timeout
        
            p->status.lastDeRegistrationAttemptTime = C1222Misc_GetFreeRunningTimeInMS();
            C1222AL_SetRegistrationRetryTime(p);
            
            //p->status.configurationChanged = TRUE; // to get the registered aptitle to the comm module
        }
        
        p->status.timeToDeRegister = FALSE;
    }
}






static void DeviceHandleRegistration(C1222AL* p)
{
    SetIdleState( p, ALIS_HANDLINGREGISTRATION);
    
    DeviceHandleRegistrationScheduling(p);
                
    if ( p->status.timeToRegister )
        DeviceHandleTimeToRegister(p);
}


static void DeviceHandleDeRegistration(C1222AL* p)
{
    Unsigned32 now = C1222Misc_GetFreeRunningTimeInMS();
  
    SetIdleState( p, ALIS_HANDLINGDEREGISTRATION);
                
    if ( p->status.timeToDeRegister )
        DeviceHandleTimeToDeRegister(p);
    else
    {
        if ( (now-(p->status.lastDeRegistrationAttemptTime)) > (p->status.registrationRetrySeconds*1000L) )
        {
            C1222AL_LogEvent(p, C1222EVENT_AL_TIME_TO_DEREGISTER);
            p->status.timeToDeRegister = TRUE;
            //C1222AL_SetRegistrationRetryTime(p);
        }
        
    }
}
