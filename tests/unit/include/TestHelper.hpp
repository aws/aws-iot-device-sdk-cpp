/*
 * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file TestHelper.hpp
 * @brief
 *
 */


#pragma once

#include "util/Utf8String.hpp"

#include "mqtt/Client.hpp"

#define MAX_NO_OF_REMAINING_LENGTH_BYTES 4

#define MAX_MQTT_PACKET_REM_LEN_BYTES 268435455

// Fixed header first bytes as per MQTT spec
// CONNECT - 0001 0000
#define CONNECT_PACKET_FIXED_HEADER_VAL 0x10
// CONNACK - 0010 0000
#define CONNACK_PACKET_FIXED_HEADER_VAL 0x20
// PUBLISH - 0011 <varies>
// <varies> = x00y
// <varies> = x10y
// <varies> = dxxx
// <varies> = xxxr
#define PUBLISH_PACKET_FIXED_HEADER_VAL 0x30
// PUBACK - 0100 0000
#define PUBACK_PACKET_FIXED_HEADER_VAL 0x40
// PUBREC - 0101 0000
#define PUBREC_PACKET_FIXED_HEADER_VAL 0x50
// PUBREL 0110 0010
#define PUBREL_PACKET_FIXED_HEADER_VAL 0x62
// PUBCOMP 0111 0000
#define PUBCOMP_PACKET_FIXED_HEADER_VAL 0x70
// SUBSCRIBE 1000 0010
#define SUBSCRIBE_PACKET_FIXED_HEADER_VAL 0x82
// SUBACK 1001 0000
#define SUBACK_PACKET_FIXED_HEADER_VAL 0x90
// UNSUBSCRIBE 1010 0010
#define UNSUBSCRIBE_PACKET_FIXED_HEADER_VAL 0xA2
// UNSUBACK 1011 0000
#define UNSUBACK_PACKET_FIXED_HEADER_VAL 0xB0
// PINGREQ 1100 0000
#define PINGREQ_PACKET_FIXED_HEADER_VAL 0xC0
// PINGRESP 1101 0000
#define PINGRESP_PACKET_FIXED_HEADER_VAL 0xD0
// DISCONNECT 1110 0000
#define DISCONNECT_PACKET_FIXED_HEADER_VAL 0xE0

#define CONNACK_PACKET_REM_LEN_VAL 2
#define PUBACK_PACKET_REM_LEN_VAL 2

namespace awsiotsdk {
    namespace tests {
        enum class ConnackTestReturnCode {
            CONNECTION_ACCEPTED = 0,
            UNACCEPTABLE_PROTOCOL_VERSION_ERROR = 1,
            IDENTIFIER_REJECTED_ERROR = 2,
            SERVER_UNAVAILABLE_ERROR = 3,
            BAD_USERDATA_ERROR = 4,
            NOT_AUTHORIZED_ERROR = 5,
            INVALID_VALUE_ERROR = 6
        };

        class TestHelper {
        public:
            static void WriteCharToBuffer(unsigned char **p_buf, unsigned char value);
            static void WriteUint16ToBuffer(unsigned char **p_buf, uint16_t value);
            static unsigned char ReadCharFromBuffer(unsigned char **p_buf);
            static uint16_t ReadUint16FromBuffer(unsigned char **p_buf);
            static std::unique_ptr<Utf8String> ReadUtf8StringFromBuffer(unsigned char **p_buf);
            static size_t ParseRemLenFromBuffer(unsigned char **p_buf);
            static util::String GetEncodedRemLen(size_t rem_len);
            static util::String GetSerializedPublishMessage(util::String topic_name,
                                                            uint16_t packet_id,
                                                            mqtt::QoS qos,
                                                            bool is_duplicate,
                                                            bool is_retained,
                                                            util::String payload);
            static util::String GetSerializedSubAckMessage(uint16_t packet_id, std::vector<uint8_t> suback_list_);
            static util::String GetSerializedUnsubAckMessage(uint16_t packet_id);
            static util::String GetSerializedPubAckMessage(uint16_t packet_id);
            static util::String GetSerializedConnAckMessage(bool is_session_present, ConnackTestReturnCode connack_rc);
        };
    }
}


