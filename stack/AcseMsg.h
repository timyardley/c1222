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




#ifndef ACSE_MSG_H
#define ACSE_MSG_H

#include "epsem.h"
#include "c1222aptitle.h"


#define MAX_ACSE_OVERHEAD_LENGTH 128    // just a stab at it - figure it out later

#define MAX_ACSE_MESSAGE_LENGTH     (MAX_EPSEM_MESSAGE_LENGTH+MAX_ACSE_OVERHEAD_LENGTH)



typedef enum C1222ResponseModeTag
{
    C1222RESPOND_ALWAYS, C1222RESPOND_ONEXCEPTION, C1222RESPOND_NEVER
} C1222ResponseMode;


// bit masks for the calling ae qualifier
#define C1222QUAL_TEST         0x01
#define C1222QUAL_URGENT       0x02
#define C1222QUAL_NOTIFICATION 0x04


typedef struct BufferItemTag
{
    Unsigned8* buffer;
    Unsigned16 length;
} BufferItem;





typedef enum C1222StandardVersionTag { CSV_UNKNOWN, CSV_MARCH2006, CSV_2008 } C1222StandardVersion;

typedef enum C1222MessageValidationResultTag {
    CMVR_OK,
    CMVR_MISSING_A8_TAG,
    CMVR_MISSING_BE_TAG,
    CMVR_TAGS_OUT_OF_ORDER,
    CMVR_WRONG_CONTEXT,
    CMVR_CALLED_APTITLE_INVALID,
    CMVR_CALLING_APTITLE_INVALID,
    CMVR_CALLING_AE_QUALIFIER_INVALID,
    CMVR_CALLED_AE_QUALIFIER_INVALID,
    CMVR_TAG_INVALID_IN_SEGMENTED_MSG,
    CMVR_CALLED_AP_INVOCATION_ID_INVALID,
    CMVR_CALLING_AP_INVOCATION_ID_INVALID,
    CMVR_AUTHENTICATION_VALUE_INVALID,
    CMVR_INFORMATION_VALUE_INVALID,
    CMVR_ENCRYPTION_KEYID_MISSING,
    CMVR_INVALID_MESSAGE_STRUCTURE_FOR_DECRYPTION,
    CMVR_INFORMATION_ELEMENT_DECRYPTION_FAILED,
    CMVR_AUTHENTICATION_FAILED,
    CMVR_INVALID_MESSAGE_STRUCTURE_FOR_AUTHENTICATION,
    CMVR_MESSAGE_MUST_NOT_BE_PLAIN_TEXT,
    CMVR_USER_ELEMENT_DECRYPTION_FAILED
    
    } C1222MessageValidationResult;
    

#ifdef C1222_WANT_CMVR_DESCRIPTIONS
const char* g_C1222MessageValidationResult_Descriptions[] = {
    "CMVR_OK",
    "CMVR_MISSING_A8_TAG",
    "CMVR_MISSING_BE_TAG",
    "CMVR_TAGS_OUT_OF_ORDER",
    "CMVR_WRONG_CONTEXT",
    "CMVR_CALLED_APTITLE_INVALID",
    "CMVR_CALLING_APTITLE_INVALID",
    "CMVR_CALLING_AE_QUALIFIER_INVALID",
    "CMVR_CALLED_AE_QUALIFIER_INVALID",
    "CMVR_TAG_INVALID_IN_SEGMENTED_MSG",
    "CMVR_CALLED_AP_INVOCATION_ID_INVALID",
    "CMVR_CALLING_AP_INVOCATION_ID_INVALID",
    "CMVR_AUTHENTICATION_VALUE_INVALID",
    "CMVR_INFORMATION_VALUE_INVALID",
    "CMVR_ENCRYPTION_KEYID_MISSING",
    "CMVR_INVALID_MESSAGE_STRUCTURE_FOR_DECRYPTION",
    "CMVR_INFORMATION_ELEMENT_DECRYPTION_FAILED",
    "CMVR_AUTHENTICATION_FAILED",
    "CMVR_INVALID_MESSAGE_STRUCTURE_FOR_AUTHENTICATION",
    "CMVR_MESSAGE_MUST_NOT_BE_PLAIN_TEXT",
    "CMVR_USER_ELEMENT_DECRYPTION_FAILED"
    };

#endif

typedef struct ACSE_MessageTag
{
    Unsigned8* buffer;
    Unsigned16 length;
    Unsigned16 maxLength;
    C1222StandardVersion standardVersion;
} ACSE_Message;

EXTERN_C void ACSE_Message_SetBuffer( ACSE_Message* p, Unsigned8* buffer, Unsigned16 maxLength);
EXTERN_C Boolean ACSE_Message_GetCallingApTitle(ACSE_Message* pMsg, C1222ApTitle* pApTitle);
EXTERN_C Boolean ACSE_Message_GetCalledApTitle(ACSE_Message* pMsg, C1222ApTitle* pApTitle);
EXTERN_C Boolean ACSE_Message_GetCallingApInvocationId(ACSE_Message* pMsg, Unsigned16* pSequence);
EXTERN_C Boolean ACSE_Message_IsContextC1222(ACSE_Message* pMsg);
EXTERN_C Boolean ACSE_Message_GetSegmentOffset(ACSE_Message* pMsg, Unsigned16* pOffset, Unsigned16* pFullLength, Unsigned16* pSequence);
EXTERN_C Boolean ACSE_Message_GetApData(ACSE_Message* pMsg, Unsigned8** pData, Unsigned16* pLength);

EXTERN_C void ACSE_Message_SetCalledApTitle(ACSE_Message* pMsg, C1222ApTitle* pApTitle);
EXTERN_C void ACSE_Message_SetCallingApTitle(ACSE_Message* pMsg, C1222ApTitle* pApTitle);
EXTERN_C void ACSE_Message_SetCallingApInvocationId(ACSE_Message* pMsg, Unsigned16 id);
EXTERN_C Boolean ACSE_Message_GetCalledApInvocationId(ACSE_Message* pMsg, Unsigned16* pId);
EXTERN_C void ACSE_Message_SetCalledApInvocationId(ACSE_Message* pMsg, Unsigned16 id);
EXTERN_C Boolean ACSE_Message_Encrypt(ACSE_Message* pMsg, Unsigned8 cipher, Unsigned8* key, 
        Unsigned8* iv, Unsigned8 ivLength, Unsigned8* keybuffer, 
        void (*keepAliveFunctionPtr)(void* p), void* keepAliveParam);
EXTERN_C Boolean ACSE_Message_UpdateAuthValueForAuthentication(ACSE_Message* pMsg, Unsigned8 cipher, 
        Unsigned8* key, Unsigned8* iv, Unsigned8 ivLength, Unsigned8* keybuffer);

EXTERN_C Boolean ACSE_Message_Decrypt(ACSE_Message* pMsg, Unsigned8 cipher, Unsigned8* key, 
        Unsigned8* iv, Unsigned8 ivLength, Unsigned8* keybuffer, 
        void (*keepAliveFunctionPtr)(void* p), void* keepAliveParam);
EXTERN_C Boolean ACSE_Message_Authenticate(ACSE_Message* pMsg, Unsigned8 cipher, Unsigned8* key, Unsigned8* iv, Unsigned8 ivLength, Unsigned8* keybuffer);
EXTERN_C Unsigned8 ACSE_Message_SetUserInformation(ACSE_Message* pMsg, Epsem* epsemMessage, Boolean wantMac);

EXTERN_C Boolean ACSE_Message_IsEncrypted(ACSE_Message* pMsg);
EXTERN_C Boolean ACSE_Message_IsAuthenticated(ACSE_Message* pMsg);
EXTERN_C Boolean ACSE_Message_GetEncryptionKeyId(ACSE_Message* pMsg, Unsigned8* pKeyId);
EXTERN_C Boolean ACSE_Message_GetUserInfo(ACSE_Message* pMsg, Unsigned8* key, Unsigned8 cipher, 
            Unsigned8* iv, Unsigned8 ivLength, Unsigned16* pUserId, Unsigned8* password20, Unsigned8* keybuffer );
EXTERN_C Boolean ACSE_Message_GetIV(ACSE_Message* pMsg, Unsigned8* iv4or8byte, Unsigned8* pIvLength);            

EXTERN_C C1222ResponseMode ACSE_Message_GetResponseMode(ACSE_Message* pMsg);
EXTERN_C Boolean ACSE_Message_IsTest(ACSE_Message* pMsg);
EXTERN_C Boolean ACSE_Message_IsNotification(ACSE_Message* pMsg);

EXTERN_C Unsigned16 ACSE_Message_GetSegmentedMessageHeaderLength(ACSE_Message* pMsg);
EXTERN_C Boolean ACSE_Message_FindACSETag(ACSE_Message* pMsg, Unsigned8 tag, Unsigned16* pIndex, Unsigned16* pTagLength );

EXTERN_C Boolean ACSE_Message_IsSegmented(ACSE_Message* pMsg, Boolean* pIsSegmented);
EXTERN_C void ACSE_Message_SetAuthValueForEncryption( ACSE_Message* pMsg, Unsigned8 keyId, Unsigned8* iv, Unsigned8 ivLength);
EXTERN_C void ACSE_Message_SetInitialAuthValueForAuthentication( ACSE_Message* pMsg, Unsigned8 keyId, Unsigned8* iv, Unsigned8 ivLength);

EXTERN_C void ACSE_Message_SetCallingAeQualifier(ACSE_Message* pMsg, Unsigned8 qualByte );
EXTERN_C Boolean ACSE_Message_GetCallingAeQualifier(ACSE_Message* pMsg, Unsigned8* pQualByte );

EXTERN_C Boolean ACSE_Message_UpdateUserInformationMac(ACSE_Message* pMsg);

EXTERN_C Boolean ACSE_Message_SetAuthValueForEncryptionWithUser( ACSE_Message* pMsg, Unsigned8 keyId, Unsigned8* iv, 
              Unsigned8 ivLength, Unsigned16 userId, Unsigned8* password20, Unsigned8* key, 
              Unsigned8 cipher, Unsigned8* keybuffer);

EXTERN_C Boolean ACSE_Message_SetInitialAuthValueForAuthenticationWithUser( ACSE_Message* pMsg, Unsigned8 keyId, Unsigned8* iv, 
              Unsigned8 ivLength, Unsigned16 userId, Unsigned8* password20);
              
EXTERN_C void ACSE_Message_SetStandardVersion( ACSE_Message* pMsg, C1222StandardVersion std_version); 
EXTERN_C C1222StandardVersion ACSE_Message_GetStandardVersion( ACSE_Message* pMsg );

EXTERN_C Boolean ACSE_Message_IsMarch2006FormatMessage(ACSE_Message* pMsg);
EXTERN_C Boolean ACSE_Message_IsValidFormat2006(ACSE_Message* pMsg);
EXTERN_C Boolean ACSE_Message_IsValidFormat2008(ACSE_Message* pMsg); 

EXTERN_C Boolean ACSE_Message_GetAuthValue(ACSE_Message* pMsg, Unsigned8* authvalue); 

EXTERN_C Boolean ACSE_Message_IsResponse(ACSE_Message* pMsg);

EXTERN_C Boolean ACSE_Message_FindBufferTag(Unsigned8* buffer, Unsigned16 length, Unsigned8 tag, Unsigned16* pIndex, Unsigned16* pLength);
EXTERN_C void ACSE_Message_CopyBytesUp(Unsigned8* dest, Unsigned8* source, Unsigned16 length);
EXTERN_C void ACSE_Message_CopyBytesDown(Unsigned8* dest, Unsigned8* source, Unsigned16 length); 

EXTERN_C void ACSE_Message_SetContext(ACSE_Message* pMsg );  
EXTERN_C Boolean ACSE_Message_HasContext(ACSE_Message* pMsg); 
EXTERN_C Boolean ACSE_Message_HasTag(ACSE_Message* pMsg, Unsigned8 tag);
EXTERN_C Boolean ACSE_Message_IsEncryptedOrAuthenticated(ACSE_Message* pMsg);

EXTERN_C C1222MessageValidationResult ACSE_Message_CheckFormat2006(ACSE_Message* pMsg); 
EXTERN_C C1222MessageValidationResult ACSE_Message_CheckFormat2008(ACSE_Message* pMsg);     


#endif
