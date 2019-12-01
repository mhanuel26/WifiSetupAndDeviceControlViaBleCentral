/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "message_protocol_utilities.h"
#include "nrf_log.h"
#include <string.h>

bool MessageProtocol_IsMessageComplete(uint8_t *message, uint8_t messageLength)
{
//    NRF_LOG_INFO("ENTER MessageProtocol_IsMessageComplete, len: %d",  messageLength);
//    for(int j=0; j<messageLength; j++){
//        NRF_LOG_INFO("0x%02X ", message[j]);
//    }
    // Check the message has the minimum required length and starts with Preamble bytes
    if (messageLength > sizeof(MessageProtocol_MessageHeader) &&
        memcmp(MessageProtocol_MessagePreamble, message, sizeof(MessageProtocol_MessagePreamble)) ==
            0) {
        // Check whether the overall length is equal or greater than message header size + length
        MessageProtocol_MessageHeader *messageHeader = (MessageProtocol_MessageHeader *)message;
        return (messageLength >= messageHeader->length + sizeof(MessageProtocol_MessageHeader));
    }
    return false;
}