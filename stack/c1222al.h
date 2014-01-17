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

// note: sizes and offsets below are for the Renesas processor (and probably PC)


#ifndef C1222AL_H
#define C1222AL_H

#include "c1222.h"
#include "acsemsg.h"
#include "c1222aptitle.h"
#include "c1222event.h"
#include "comevent_id.h"

//#ifdef ISAC1222DEVICE
#include "c1222al_segmenter.h"
//#endif


#define MAX_ACSE_SEGMENTED_MESSAGE_LENGTH  158 //458 //158

#define C1222_NATIVE_ADDRESS_LENGTH   6

#define C1222_ESN_LENGTH              20
#define C1222_DEVICE_CLASS_LENGTH      4

#define C1222_LANWAN_INTERFACE_NUMBER  0


#define C1222_NODETYPE_END_DEVICE     0x20
#define C1222_NODETYPE_RELAY          0x01

#define C1222_APPLICATION_RETRIES        1

#define DEFAULT_MAX_REGISTRATION_RETRY_FACTOR  4    // will double registration randomization time until successful or retry factor exceeded

#define DEFAULT_SEND_FAILURE_LIMIT  5

#define C1222_MSGTYPE_UNKNOWN                   0
#define C1222_MSGTYPE_RECEIVED_INVALID_MSG      1
#define C1222_MSGTYPE_EXCEPTION                 2
#define C1222_MSGTYPE_PERIODIC_READ_RESPONSE    3
#define C1222_MSGTYPE_REGISTER_REQUEST          4
#define C1222_MSGTYPE_DEREGISTER_REQUEST        5
#define C1222_MSGTYPE_REGISTER_RESPONSE         6
#define C1222_MSGTYPE_DEREGISTER_RESPONSE       7
#define C1222_MSGTYPE_REQUEST                   8
#define C1222_MSGTYPE_RESPONSE                  9
#define C1222_MSGTYPE_SCHEDULE_PERIODIC_READ    10
#define C1222_MSGTYPE_OK_RESPONSES              11
#define C1222_MSGTYPE_ERROR_RESPONSES           12
#define C1222_MSGTYPE_TABLE_READS               13
#define C1222_MSGTYPE_TABLE_WRITES              14
#define C1222_MSGTYPE_TABLE_READS_AND_WRITES    15
#define C1222_MSGTYPE_OK_AND_ERROR_RESPONSES    16


typedef enum C1222MessageFailureCodeTag
{
    MFC_OK=0,
    MFC_SEND_MESSAGE_FAILED,
    MFC_RECEIVED_EPSEM_INVALID,
    MFC_RECEIVED_INFORMATION_ELEMENT_MISSING,
    MFC_SEND_TRANSPORT_BUSY,
    MFC_SEND_INVALID_NATIVE_ADDRESS,
    MFC_SEND_SET_UNSEGMENTED_MESSAGE_FAILED,
    MFC_SEND_ALL_RETRIES_FAILED,
    MFC_SEND_GET_SEGMENT_FAILED,
    MFC_NO_REQUEST_OR_RESPONSE_IN_EPSEM,
    MFC_REG_DEREG_APTITLE_INVALID,
    MFC_DEREG_APTITLE_MISMATCH,
    MFC_REG_DEREG_RESPONSE_LENGTH_INVALID,
    MFC_REG_DEREG_RESPONSE_NOT_OK,
    MFC_REG_DEREG_EPSEM_INVALID,
    MFC_REG_DEREG_INFORMATION_ELEMENT_MISSING,
    MFC_REG_DEREG_APTITLE_COMPARE_FAILED,
    MFC_REG_NOT_SYNCHRONIZED,
    MFG_REG_DEREG_PENDING_OR_TIMEOUT,
    MFC_REG_ESN_INVALID,
    MFC_REG_ESN_OR_APTITLE_TOO_LONG
} C1222MessageFailureCode;

#define C1222_VALIDATION_RESULT_OFFSET   80
// validation results are also message failure codes and start with 80


typedef struct C1222MessageLogEntryTag
{
    Unsigned8 aptitle[C1222_APTITLE_LENGTH];
    Unsigned16 callingSequence;
    Unsigned16 calledSequence;
    Unsigned8  messageType;
    Unsigned8  requestOrResponseMessageType;
} C1222MessageLogEntry;


typedef struct C1222TransportConfigTag
{
    Boolean passthroughMode;
    Unsigned8 linkControlByte;
    Unsigned8 nodeType;
    Unsigned8 deviceClass[C1222_DEVICE_CLASS_LENGTH];
    // offset 7:
    Unsigned8 esn[C1222_ESN_LENGTH];
    // offset 27:
    Unsigned8 nativeAddressLength;
    Unsigned8 nativeAddress[C1222_NATIVE_ADDRESS_LENGTH];
    // offset 34:
    Unsigned8 broadcastAddressLength;
    Unsigned8 broadcastAddress[C1222_NATIVE_ADDRESS_LENGTH];
    // offset 41:
    Unsigned8 relayAddressLength;
    Unsigned8 relayNativeAddress[C1222_NATIVE_ADDRESS_LENGTH];
    // offset 47:
    Unsigned8 nodeApTitle[C1222_APTITLE_LENGTH];
    // offset 67:
    Unsigned8 masterRelayApTitle[C1222_APTITLE_LENGTH];
    // offset 87:
    Boolean wantClientResponseControl;
    Unsigned8 numberOfRetries;
    Unsigned16 responseTimeout;
} C1222TransportConfig;  // size 91




#define C1222_INTERFACE_NAME_LENGTH    20
#define C1222_NUMBER_OF_STATISTICS     35



typedef struct C1222Signed48Tag
{ 
    Unsigned32 lower32Bits;
    Unsigned16 upper16Bits;
} C1222Signed48;


typedef struct C1222StatisticTag
{
    Unsigned16      statisticCode;
    C1222Signed48   statisticValue;    
} C1222Statistic;


typedef struct C1222TransportStatusTag
{
    Unsigned8 interfaceName[C1222_INTERFACE_NAME_LENGTH];
    // offset 20:
    Unsigned8 interfaceStatus; // as defined by link control service
    
    // offset 21:
    C1222Statistic statistics[C1222_NUMBER_OF_STATISTICS]; // size 6 * 35 = 210
    // offset 231:
    Boolean changedStatistics[C1222_NUMBER_OF_STATISTICS]; // size 35
    
} C1222TransportStatus; // size 266


typedef struct C1222InterfaceInfoTag
{
    Unsigned8 interfaceName[C1222_INTERFACE_NAME_LENGTH];
    Unsigned8 interfaceStatus;
    Unsigned8 nodeType;
} C1222InterfaceInfo;



typedef struct C1222RegistrationStatusTag
{
    Unsigned8 relayAddressLength;
    Unsigned8 relayNativeAddress[C1222_NATIVE_ADDRESS_LENGTH];
    // offset 7:
    Unsigned8 masterRelayApTitle[C1222_APTITLE_LENGTH];
    // offset 27:
    Unsigned8 nodeApTitle[C1222_APTITLE_LENGTH];
    // offset 47:
    Unsigned16 regPowerUpDelaySeconds;
    Unsigned32 regPeriodMinutes;
    Unsigned32 regCountDownMinutes;
} C1222RegistrationStatus; // size 53




// the message receiver is used by the comm module to receive a message from the meter
// and by the meter to receive a message from the comm module


typedef struct C1222MessageReceiverTag
{
    //void* pTransport;
    void* pApplication;
    
    // offset 4:
    
    //Unsigned16 workingSegmentIndex;
    C1222ALSegmenter receiveSegmenter; // size 743
    
    // offset 747:
    Unsigned8 nativeAddress[C1222_NATIVE_ADDRESS_LENGTH];
    
    // offset 753:
    Unsigned8 nativeAddressCanary[10];
    Unsigned16 nativeAddressLength;
    
} C1222MessageReceiver;  // size 765

Unsigned8 C1222MessageReceiver_AddSegment(C1222MessageReceiver* p, Unsigned8 nativeAddressLength, Unsigned8* nativeAddress, ACSE_Message* pMsg);
Boolean C1222MessageReceiver_IsMessageComplete(C1222MessageReceiver* p);
ACSE_Message* C1222MessageReceiver_GetUnsegmentedMessage(C1222MessageReceiver* p);
Unsigned8 C1222MessageReceiver_SetMessage(C1222MessageReceiver* p, Unsigned8 nativeAddressLength, Unsigned8* nativeAddress, ACSE_Message* pMsg);
void C1222MessageReceiver_SetLinks(C1222MessageReceiver* p, void* pApplication);
void C1222MessageReceiver_Init(C1222MessageReceiver* p);
void C1222MessageReceiver_NoteMessageHandlingComplete(C1222MessageReceiver* p);
Boolean C1222MessageReceiver_HandleReceiveSegmentTimeout(C1222MessageReceiver* p);
void C1222MessageReceiver_CancelPartialMessage(C1222MessageReceiver* p);





typedef struct C1222MessageSenderTag
{
    void* pTransport;
    void* pApplication;
    
    // offset 8:
    Unsigned16 workingSegmentIndex;
    // offset 10:
    C1222ALSegmenter transmitSegmenter; // size 743
    // offset 753:
    Unsigned8 nativeAddress[C1222_NATIVE_ADDRESS_LENGTH];
    // offset 759
    Unsigned8 nativeAddressCanary[10];
    Unsigned16 nativeAddressLength;
    
    // offset 771:
    Unsigned8 messageBuffer[MAX_ACSE_SEGMENTED_MESSAGE_LENGTH]; // size 158
    Unsigned8 messageBufferCanary[10];
    // offset 939
    Unsigned16 messageLength;
    
    Unsigned8 triesLeft;
    
} C1222MessageSender; // size 942

void C1222MessageSender_SetLinks(C1222MessageSender* p, void* pTransport, void* pApplication);
Boolean C1222MessageSender_StartSend(C1222MessageSender* p, ACSE_Message* pMsg, Unsigned8* nativeAddress, Unsigned16 nativeAddressLength);
Boolean C1222MessageSender_ContinueSend(C1222MessageSender* p);
Boolean C1222MessageSender_IsMessageComplete(C1222MessageSender* p);
Boolean C1222MessageSender_SendShortMessage(C1222MessageSender* p, ACSE_Message* pMsg, Unsigned8* nativeAddress, Unsigned16 nativeAddressLength);
Boolean C1222MessageSender_RestartSend(C1222MessageSender* p);
void C1222MessageSender_Init(C1222MessageSender* p);
Boolean C1222MessageSender_TriesLeft(C1222MessageSender* p);
ACSE_Message* C1222MessageSender_GetWorkingMessage(C1222MessageSender* p);





typedef struct C1222ALCountersTag
{    
    Unsigned16 registrationSuccessCount;
    
    Unsigned32 messagesReceived;
    Unsigned32 messagesSent;
    
    // offset 10:    
    Unsigned16 runCount;
        
    Unsigned16 sendAbortedTransportBusy;
    Unsigned16 sendAbortedOtherReason;
    Unsigned16 sendContinuationFailed;
    Unsigned16 sendRetried;
     
    // offset 20:   
    Unsigned16 discardedCompleteMessages;
    Unsigned16 discardedPartialMessages;
    
    Unsigned16 sendTimedOut;
    
    Unsigned16 duplicateSegmentIgnored;
    Unsigned16 duplicateMessageIgnored;
    
    // offset 30:
    Unsigned16 sendAllRetriesFailed;
    Unsigned16 linkFailure;
    Unsigned16 partialMessageTimedoutAndDiscarded;
    
    Unsigned16 deRegistrationSuccessCount;
    
    Unsigned16 partialMessageCanceled;
    
    // offset 40:
    Unsigned16 registrationAttempts;
    Unsigned16 deRegistrationAttempts;
    
    Unsigned16 rflanMessageIgnored;
    
    Unsigned16 receivedBroadCastForMeCount;
    Unsigned16 receivedMultiCastForMeCount;
    
    // offset 50:
    Unsigned16 partialMessageOverLayedByLaterFullMessage;
    Unsigned16 partialMessageDiscardedBySegmenter;
    
    Unsigned16 received2006FormatMessages;
    Unsigned16 received2008FormatMessages;
    Unsigned16 receivedInvalidFormatMessages;
    
} C1222ALCounters; // length 54


typedef enum C1222AL_StateTag {ALS_INIT, ALS_SENDINGMESSAGE, ALS_IDLE, ALS_REGISTERING, ALS_DEREGISTERING} C1222AL_State;


typedef enum C1222ProcedureStateTag {C1222PS_IDLE, C1222PS_START, C1222PS_RUNNING, C1222PS_COMPLETE} C1222ProcedureState;

typedef enum C1222AL_IdleStateTag
{
    ALIS_IDLE, 
    ALIS_TURNINGOFFAUTOREGISTRATION, 
    ALIS_GETTINGSYNCHRONIZATIONSTATE,
    ALIS_HANDLINGREGISTRATION, 
    ALIS_GETTINGINTERFACESTATUS,
    ALIS_SENDINGRELOADCONFIGREQUEST,
    ALIS_HANDLINGDEREGISTRATION
} C1222AL_IdleState; 



typedef struct C1222ALStatusInfoTag
{
    C1222AL_State state;
    
    C1222AL_IdleState deviceIdleState;
    
    Boolean timeToRegister;
    Boolean autoRegistration;
    Boolean interfaceEnabled;
    Boolean useDirectMessaging;
    
    Boolean waitingToRegister;
    
    Boolean doNotRegister;
    
    Boolean isRegistered;
    Boolean isSynchronizedOrConnected;
    
    // offset 10:
    Boolean isCommModule;
    Boolean isRelay;
    Boolean isLocalPort;
    Boolean wantReassembly; // this is set to force reassembly of segmented messages in a comm module
    
    Boolean isReceivedMessageComplete;
    Boolean isReceivedMessageHandlingComplete;
    Boolean isReceivedMessageResponsePending;
    
    // offset 17:
    Unsigned32 lastSuccessfulRegistrationTime;
    Unsigned32 lastRegistrationAttemptTime;
    Unsigned32 lastRequestTime;
    
    Boolean directMessagingAvailable;
    
    // offset 30:
    Boolean haveConfiguration;
    Boolean haveRegistrationStatus;
    Boolean haveInterfaceInfo;
    
    Boolean configurationChanged;
    
    Unsigned16 nextApInvocationId;
    Unsigned16 registrationRequestApInvocationId;
    
    // offset 38:
    Unsigned8 inProcessProcedure;
    Boolean   linkControlRunning;
    Unsigned8 linkControlResponse;
        
    Unsigned32 lastPowerUp;
    Unsigned16 powerUpDelay;
    
    // offset 47:    
    Boolean sending;
    Boolean sendingRegistrationRequest;
    Boolean registrationInProgress;
    
    // offset 50:
    C1222MessageFailureCode registrationFailureReason;
    
    Unsigned32 lastCMSegmentedMessagePromotion;
    
    Unsigned32 sendStartTime;
    
    // offset 59:
    Unsigned16 registrationRetryMinutesMin;
    Unsigned16 registrationRetryMinutesMax;
    Unsigned32 registrationRetrySeconds;
    
    // offset 67:
    C1222ProcedureState inProcessProcedureState;
    Unsigned8 inProcessProcedureResponse;
    
    // offset 69:
    Unsigned32 lastCompleteMessageTime;
    Unsigned16 maxCompleteMessageProcessTime;
    Unsigned16 avgCompleteMessageProcessTime;
    Unsigned32 totalCompleteMessageProcessTime;
    
    // offset 81:
    Unsigned16 maxMessageSendTime;
    Unsigned16 avgMessageSendTime;
    Unsigned32 totalMessageSendTime;
    
    // offset 89:
    Unsigned16 currentRFLanCellId;
    
    Unsigned32 lastSuccessfulDeRegistrationTime;
    Unsigned32 lastDeRegistrationAttemptTime;
    
    // offset 99:
    Unsigned16 deRegistrationRequestApInvocationId;
    Boolean timeToDeRegister;
    Boolean deRegistrationInProgress;
    
    C1222MessageFailureCode deRegistrationFailureReason;
    
    Unsigned16 registrationRetryFactor;
    Unsigned16 registrationMaxRetryFactor;
    
    // offset 108:
    Unsigned32 lastUnSynchronizedTime;
    
    Unsigned16 currentRFLanLevel;
    
    Unsigned8 registrationResponse;
    
    Boolean tryingToGetRFLANRevisionInfo;
    
    // offset 116:
    Unsigned32 timeOfCellIDChange;
    
    Unsigned8 rflanStateInfo[6];
    
    Boolean haveRevInfo;
    
    // offset 127:
    Unsigned32 timeOfLastBroadCastForMe;
    Unsigned32 timeOfLastMultiCastForMe;
    Unsigned32 timeOfLastDiscardedSegmentedMessage;
    
    // offset 139:
    Unsigned32 timeLastLostRegistration;
    
    Unsigned8 cellSwitchRegistrationRetryTimeDivisor;
    Boolean tryingToRegisterAfterCellSwitch;
    
    // offset 145:
    Boolean isRFLAN_coldStart;
    
    Unsigned16 lastRegistrationRequestApInvocationId;
    
    Boolean disableRegisterOnCellChange;
    
    Unsigned16 sendTimeOutSeconds;
    
    C1222StandardVersion clientC1222StandardVersion;
    Boolean clientSendACSEContext;
    
    C1222MessageValidationResult lastInvalid2006Reason;
    C1222MessageValidationResult lastInvalid2008Reason;
    Unsigned32 lastInvalidMessageTime;
    
    Boolean requireEncryptionOrAuthenticationIfKeysExist;
    Unsigned32 bestFatherMacAddress;
    Unsigned32 syncFatherMacAddress;
    Unsigned16 rflan_gpd;
    signed char    rflan_syncFatherRssi;
    signed char    rflan_bestFatherRssi;
    Unsigned8  rflan_fatherCount;
    
    Unsigned16 consecutiveSendFailures;
    Unsigned16 consecutiveSendFailureLimit;

} C1222ALStatusInfo;  // size 148 - obsolete comment


typedef struct C1222MessageLogTag
{
    Unsigned16 sequence;
    Unsigned8 callingApTitleEnd[2];
} C1222MessageLog;




typedef struct C1222LastMessageLogTag
{
    Unsigned32 firstSegmentReceived;
    Unsigned32 lastSegmentReceived;
    Unsigned32 availableForServer;
    Unsigned32 responseBuiltPreEncryption;
    Unsigned32 responseReadyToSend;
    
} C1222LastMessageLog;



typedef struct C1222ALTag
{
    const char* pInterfaceName;
    
    // offset 4:
    C1222ALStatusInfo status;
    
    // offset 150:
    C1222MessageSender messageSender;       // sends message through comm module/device link (size 942)
    // offset 1092:
    C1222MessageReceiver messageReceiver;   // receives message through comm module/device link (size 765)
    
    // offset 1857:
    C1222TransportConfig transportConfig; // size 91
    // offset 1948:
    C1222TransportStatus transportStatus;  // size 266
    // offset 2214:
    C1222RegistrationStatus registrationStatus; // size 53
        
    // offset 2267
    void* pTransport;
    
    //FILE* logfp;
    void (*logEventFunction)(struct C1222ALTag* p, Unsigned16 event);
    void (*logEventAndBytesFunction)(struct C1222ALTag* p, Unsigned16 event, Unsigned8 n, Unsigned8* bytes);
    void (*logEventAndULongsFunction)(struct C1222ALTag* p, Unsigned16 event, Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4);
    
    // offset 2279    
    Boolean logWanted;
           
#ifdef C1222_WANT_EVENTSUMMARY            
    Unsigned8 eventCounts[C1222EVENT_NUMBEROF];  // currently size=177
    // offset 2457:
    Unsigned32 eventLastTime[C1222EVENT_NUMBEROF]; // currently size=177*4=708
    // offset 3165
    Unsigned8 eventCountCanary[10];
#endif    
    
    // offset 3175:    
    C1222ALCounters counters;  
    
    // offset 3229
    C1222MessageLog receivedMessageLog[10]; // size 4*10
    // offset 3269
    Unsigned8 receivedMessageIndex;
    // offset 3270
    C1222MessageLog sentMessageLog[10];
    // offset 3310
    Unsigned8 sentMessageIndex;
    
    Boolean powerUpInProgress;
    
    // offset 3312
    Unsigned8 tempBuffer[300];  // needs to be at least 3*96 currently
    
    void (*comEventLogFunction)(Unsigned16 eventID, void* argument, Unsigned16 argLength);
    
    C1222LastMessageLog lastMessageLog;
    
} C1222AL; // size 3412











typedef struct C1222ALEventCountsTag
{
    Unsigned8 eventCounts[C1222EVENT_NUMBEROF];
} C1222ALEventCounts;



// these routines are called by the C1222Stack

Boolean C1222AL_SendMessage(C1222AL* p, ACSE_Message* pMsg, Unsigned8* destinationNativeAddress, Unsigned16 destNativeAddressLength);
Boolean C1222AL_SendShortMessage(C1222AL* p, ACSE_Message* pMsg, Unsigned8* destinationNativeAddress, Unsigned16 destNativeAddressLength);
void C1222AL_Run(C1222AL* p);
Boolean C1222AL_IsSendMessageComplete(C1222AL* p);
void C1222AL_NoteReceivedMessageHandlingIsComplete(C1222AL* p);
void C1222AL_NoteReceivedMessageHandlingFailed(C1222AL* p, Unsigned8 result);
Boolean C1222AL_TLIsReceivedMessageHandlingComplete(C1222AL* p);
ACSE_Message* C1222AL_GetReceivedMessage(C1222AL* p);
Boolean C1222AL_GetReceivedMessageNativeAddress(C1222AL* p, Unsigned8* pNativeAddressLength, Unsigned8** pNativeAddress);
void C1222AL_SetConfiguration(C1222AL* p, C1222TransportConfig* pCfg);
Boolean C1222AL_IsMessageComplete(C1222AL* p);
void C1222AL_SetInterfaceName(C1222AL* p, const char* name);
void C1222AL_Init(C1222AL* p, void* pTransport, Boolean isCommModule, Boolean isLocalPort, Boolean isRelay);
void C1222AL_IncrementStatistic(C1222AL* p, Unsigned16 id);
void C1222AL_AddToStatistic(C1222AL* p, Unsigned16 id, Unsigned32 count);
void C1222AL_SetStatisticU32(C1222AL* p, Unsigned16 id, Unsigned32 value);
void C1222AL_SetStatisticValueBuffer(C1222AL* p, Unsigned16 id, Unsigned8* buffer); // buffer is 6 bytes
void C1222AL_HandleStatisticChange(C1222AL* p, C1222Statistic* pStat);
//void C1222AL_SimpleResponse(C1222AL* p, ACSE_Message* pMsg, Unsigned8 response);
void C1222AL_SimpleResponse(C1222AL* p, ACSE_Message* pMsg, Unsigned8 response, Unsigned32 maxRequestOrResponseLength);
void C1222AL_SimpleResponseWithoutReceivedMessageHandling(C1222AL* p, ACSE_Message* pMsg, Unsigned8 response, Unsigned32 maxRequestOrResponseLength);
void C1222AL_SetReassemblyOption(C1222AL* p, Boolean wantReassembly);
void C1222AL_SetLog(C1222AL* p,
	   void (*fp1)(C1222AL* p, Unsigned16 event),
       void (*fp2)(C1222AL* p, Unsigned16 event, Unsigned8 n, Unsigned8* bytes),
       void (*fp3)(C1222AL* p, Unsigned16 event, Unsigned32 u1, Unsigned32 u2, Unsigned32 u3, Unsigned32 u4));
Boolean C1222AL_IsBusy(C1222AL* p);
Boolean C1222AL_IsInterfaceAvailable(C1222AL* p);
Boolean C1222AL_IsSynchronized(C1222AL* p);
Boolean C1222AL_IsRegistered(C1222AL* p);
void C1222AL_SetRFLANisSynchronizedState(C1222AL* p, Boolean isSynchronized);
Boolean C1222AL_IsProcedureRunning(C1222AL* p);


// these routines are called by C1222TL (transport layer)
C1222TransportConfig* C1222AL_GetConfiguration(C1222AL* p);
void C1222AL_TLUpdateConfiguration(C1222AL* p, C1222TransportConfig* pConfig); // called by TL (in comm module) on get config response
void C1222AL_TLUpdateRegistrationStatus(C1222AL* p, C1222RegistrationStatus* pStatus); // called by TL on get reg status response
Unsigned8 C1222AL_TLMessageReceived(C1222AL* p, Unsigned8 nativeAddressLength, Unsigned8* nativeAddress, BufferItem* pBufferItem);
Unsigned8 C1222AL_TLShortMessageReceived(C1222AL* p, Unsigned8 nativeAddressLength, Unsigned8* nativeAddress, BufferItem* pBufferItem);
void C1222AL_TLInterfaceControlReceived(C1222AL* p, Unsigned8 interfaceControl);
Unsigned8* C1222AL_TLGetNodeApTitle(C1222AL* p);
Unsigned8 C1222AL_TLGetInterfaceStatus(C1222AL* p);
void C1222AL_TLLinkControlResponse(C1222AL* p, Unsigned8 interfaceStatus, Unsigned8* pApTitle);
void C1222AL_TLSendMessageResponse(C1222AL* p);
const char* C1222AL_TLGetInterfaceName(C1222AL* p);  // returns asciiz string
Unsigned16 C1222AL_TLGetNumberOfStatistics(C1222AL* p);
Unsigned16 C1222AL_TLGetNumberOfChangedStatistics(C1222AL* p);
void C1222AL_TLGetChangedStatistic(C1222AL* p, Unsigned16 ii, C1222Statistic* pStatistic);
void C1222AL_TLGetStatistic(C1222AL* p, Unsigned16 ii, C1222Statistic* pStatistic);
C1222RegistrationStatus* C1222AL_TLGetRegistrationStatus(C1222AL* p);
void C1222AL_TLUpdateInterfaceInfo(C1222AL* p, Unsigned8 interfaceNameLength, char* interfaceName, Unsigned8 interfaceStatus);
void C1222AL_TLUpdateStatistic(C1222AL* p, C1222Statistic* pStat);
void C1222AL_TLNoteConfigurationSent(C1222AL* p);
void C1222AL_ConstructCallingApTitle(C1222AL* pAppLayer, C1222ApTitle* pApTitle);
void C1222AL_TL_NoteLinkFailure(C1222AL* p);
void C1222AL_TL_NotifyFailedLinkControl(C1222AL* p, Unsigned8 response);
void C1222AL_TL_NoteFailedSendMessage(C1222AL* p);


ACSE_Message* StartNewUnsegmentedMessage(C1222AL* p);
void SetMessageDestination(C1222AL* p, Unsigned8 addressLength, Unsigned8* address);
void EndNewUnsegmentedMessage(C1222AL* p);

Unsigned16 C1222AL_GetNextApInvocationId(C1222AL* p);

Unsigned8 C1222AL_GetNumberOfKeys(C1222AL* p);
Boolean C1222AL_GetKey( C1222AL* p, Unsigned8 keyId, Unsigned8* pCipher, Unsigned8* key);


void C1222AL_LogEvent(C1222AL* p, C1222EventId event);
void C1222AL_LogEventAndBytes(C1222AL* p, C1222EventId event, Unsigned8 n, Unsigned8* bytes);  
void C1222AL_LogEventAndULongs(C1222AL* p, C1222EventId event, Unsigned32 ul1, Unsigned32 ul2, Unsigned32 ul3, Unsigned32 ul4);  


void C1222AL_NotePowerUp(C1222AL* p, Unsigned32 outageDurationInSeconds);
Unsigned8 C1222AL_DeviceRegisterNow(C1222AL* p);
Unsigned8 C1222AL_DeviceDeRegisterNow(C1222AL* p);
Unsigned8 C1222AL_DeviceSendLinkControlNow(C1222AL* p, Unsigned8 linkControlByte);
void C1222AL_ResetAllStatistics(C1222AL* p);

Unsigned8 C1222AL_DeviceGetRegisterNowResult(C1222AL* p);
Unsigned8 C1222AL_DeviceGetDeRegisterNowResult(C1222AL* p);
Unsigned8 C1222AL_DeviceGetSendLinkControlNowResult(C1222AL* p);

void C1222AL_NoteReceivedCompleteMessage(C1222AL* p);

void C1222AL_IncrementEventCount(C1222AL* p, Unsigned16 event);

void C1222AL_SetRegistrationRetryLimits(C1222AL* p, Unsigned16 minMinutes, Unsigned16 maxMinutes, Unsigned8 cellSwitchDivisor);
void C1222AL_SetRegistrationRetryTime(C1222AL* p);

Boolean C1222AL_NeedToReregisterAfterConfigChange(C1222AL* p, C1222TransportConfig* pCfg);

Boolean C1222AL_CheckForRegistrationResponseDuringRegistration(C1222AL* p, ACSE_Message* pMsg, 
                      Unsigned8 addressLength, Unsigned8* nativeAddress);
                      
Boolean C1222AL_HandleRegistrationResponse(C1222AL* p, ACSE_Message* pMsg, Unsigned8 addressLength, 
                       Unsigned8* nativeAddress);
ACSE_Message* C1222AL_MakeRegistrationMessage(C1222AL* p);

Boolean C1222AL_CheckForDeRegistrationResponseDuringDeRegistration(C1222AL* p, ACSE_Message* pMsg);
                      
Boolean C1222AL_HandleDeRegistrationResponse(C1222AL* p, ACSE_Message* pMsg);
ACSE_Message* C1222AL_MakeDeRegistrationMessage(C1222AL* p);


void C1222AL_SetRegisteredStatus(C1222AL* p, Boolean isRegistered); 

void C1222AL_DeviceNoteRegistrationAttemptComplete(C1222AL* p, Unsigned8 result);
void C1222AL_DeviceNoteDeRegistrationAttemptComplete(C1222AL* p, Unsigned8 result);
void C1222AL_NoteDeRegisterRequest(C1222AL* p);

Boolean C1222AL_GetMyApTitle(C1222AL* p, C1222ApTitle* pApTitle);
Boolean C1222AL_IsMyApTitleAvailable(C1222AL* p);  

void C1222AL_ClearCounters(C1222AL* p);
void C1222AL_ClearStatusDiagnostics(C1222AL* p);                   

void C1222AL_LogReceivedMessage(C1222AL* p, ACSE_Message* pMsg);
void C1222AL_LogSentMessage(C1222AL* p, ACSE_Message* pMsg);

void C1222AL_ResetRegistrationRetryFactor(C1222AL* p);

void C1222AL_SetRegistrationMaxRetryFactor(C1222AL* p, Unsigned8 factor);

void C1222AL_ReInitAfterRFLANFwDownload(C1222AL* p, void* pTransport);

Unsigned16 C1222AL_GetDebugEventLogLength(void);
Unsigned8* C1222AL_GetDebugEventLog(void);

void C1222AL_NoteTryingToGetRFLANRevisionInfo(C1222AL* p, Boolean b);

void C1222AL_PowerUpInProgress(C1222AL* p);

Unsigned16 C1222AL_GetMyNativeAddress(C1222AL* p, Unsigned8* myNativeAddress, Unsigned16 maxLength);

Boolean C1222AL_IsBusyForRFLANAccess(C1222AL* p);

Boolean C1222AL_IsStackWaitingOnReceivedMessageHandlingNotification(C1222AL* p);

void C1222AL_TLNoteStatisticsUpdated(C1222AL* p);

void C1222AL_KeepAliveForEncryption(void* pAL);

void C1222AL_SetSendTimeoutSeconds(C1222AL* p, Unsigned16 seconds);

void C1222AL_SetClientStandardVersion(C1222AL* p, C1222StandardVersion version);
void C1222AL_SetClientSendACSEContextOption(C1222AL* p, Boolean b);

void C1222AL_RequireEncryptionOrAuthenticationIfKeys(C1222AL* p, Boolean b);

void C1222AL_RestartNormalRegistrationProcess(C1222AL* p);

Unsigned8 C1222AL_NoteDisconnectRequest(C1222AL* p);

void C1222AL_SetComEventLog(C1222AL* p, void (*comEventLogFunction)(Unsigned16 eventID, void* argument, Unsigned16 argLength));
void C1222AL_LogComEvent(C1222AL* p, Unsigned16 event);
void C1222AL_LogComEventAndBytes(C1222AL* p, Unsigned16 event, void* argument, Unsigned16 length);

void C1222AL_LogDeRegistrationResult(C1222AL* p, C1222MessageFailureCode reason);
void C1222AL_LogRegistrationResult(C1222AL* p, C1222MessageFailureCode reason);
void C1222AL_LogInvalidMessage(C1222AL* p, C1222MessageValidationResult reason, 
            C1222MessageValidationResult reason2, ACSE_Message* pMsg);
void C1222AL_LogFailedSendMessage(C1222AL* p, C1222MessageFailureCode reason, ACSE_Message* pMsg);

void C1222AL_SetConsecutiveFailureLimit(C1222AL* p, Unsigned16 limit);
void C1222AL_MakeMessageLogEntry(C1222AL* p, ACSE_Message* pMsg, C1222MessageLogEntry* pEntry, Boolean sending, Unsigned8 messageType);

Boolean C1222AL_IsMsgForCommModule(C1222AL* p, ACSE_Message* pMsg);






#endif
