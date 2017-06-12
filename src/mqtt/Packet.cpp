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
 * @file Packet.cpp
 * @brief
 *
 */

#include <algorithm>

#include "ResponseCode.hpp"
#include "mqtt/Packet.hpp"
#include <cstdio>

#define MAX_MQTT_PACKET_REM_LEN_BYTES 268435455

// Fixed header first bytes as per MQTT spec
// CONNECT - 0001 0000
#define MQTT_FIXED_HEADER_BYTE_CONNECT 0x10
// CONNACK - 0010 0000
#define MQTT_FIXED_HEADER_BYTE_CONNACK 0x20
// PUBLISH - 0011 <varies>
// <varies> = x00y
// <varies> = x10y
// <varies> = dxxx
// <varies> = xxxr
#define MQTT_FIXED_HEADER_BYTE_PUBLISH 0x30
// PUBACK - 0100 0000
#define MQTT_FIXED_HEADER_BYTE_PUBACK 0x40
// PUBREC - 0101 0000
#define MQTT_FIXED_HEADER_BYTE_PUBREC 0x50
// PUBREL 0110 0010
#define MQTT_FIXED_HEADER_BYTE_PUBREL 0x62
// PUBCOMP 0111 0000
#define MQTT_FIXED_HEADER_BYTE_PUBCOMP 0x70
// SUBSCRIBE 1000 0010
#define MQTT_FIXED_HEADER_BYTE_SUBSCRIBE 0x82
// SUBACK 1001 0000
#define MQTT_FIXED_HEADER_BYTE_SUBACK 0x90
// UNSUBSCRIBE 1010 0010
#define MQTT_FIXED_HEADER_BYTE_UNSUBSCRIBE 0xA2
// UNSUBACK 1011 0000
#define MQTT_FIXED_HEADER_BYTE_UNSUBACK 0xB0
// PINGREQ 1100 0000
#define MQTT_FIXED_HEADER_BYTE_PINGREQ 0xC0
// PINGRESP 1101 0000
#define MQTT_FIXED_HEADER_BYTE_PINGRESP 0xD0
// DISCONNECT 1110 0000
#define MQTT_FIXED_HEADER_BYTE_DISCONNECT 0xE0

namespace awsiotsdk {
    namespace mqtt {
        PacketFixedHeader::PacketFixedHeader() {
            remaining_length_ = 0;
            fixed_header_byte_ = 0;
            is_valid_ = false;
        }

        ResponseCode PacketFixedHeader::Initialize(MessageTypes message_type, bool is_duplicate, QoS qos,
                                                   bool is_retained, size_t rem_len) {
            if (MAX_MQTT_PACKET_REM_LEN_BYTES < rem_len) {
                return ResponseCode::FAILURE;
            }

            this->is_valid_ = true;
            this->remaining_length_ = rem_len;
            this->message_type_ = message_type;

            // Set all bits to zero
            fixed_header_byte_ = 0;
            switch (message_type) {
                case MessageTypes::CONNECT:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_CONNECT;
                    break;
                case MessageTypes::CONNACK:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_CONNACK;
                    break;
                case MessageTypes::PUBLISH:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_PUBLISH;
                    switch (qos) {
                        case QoS::QOS0:
                            fixed_header_byte_ |= 0x00;
                            break;
                        case QoS::QOS1:
                            fixed_header_byte_ |= 0x02;
                            break;
                            // Strongly typed enum, no default required
                    }

                    fixed_header_byte_ |= (is_duplicate ? 0x08 : 0x00);
                    fixed_header_byte_ |= (is_retained ? 0x01 : 0x00);

                    break;
                case MessageTypes::PUBACK:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_PUBACK;
                    break;
                case MessageTypes::PUBREC:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_PUBREC;
                    break;
                case MessageTypes::PUBREL:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_PUBREL;
                    break;
                case MessageTypes::PUBCOMP:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_PUBCOMP;
                    break;
                case MessageTypes::SUBSCRIBE:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_SUBSCRIBE;
                    break;
                case MessageTypes::SUBACK:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_SUBACK;
                    break;
                case MessageTypes::UNSUBSCRIBE:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_UNSUBSCRIBE;
                    break;
                case MessageTypes::UNSUBACK:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_UNSUBACK;
                    break;
                case MessageTypes::PINGREQ:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_PINGREQ;
                    break;
                case MessageTypes::PINGRESP:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_PINGRESP;
                    break;
                case MessageTypes::DISCONNECT:
                    fixed_header_byte_ = MQTT_FIXED_HEADER_BYTE_DISCONNECT;
                    break;
                default:
                    // Possible invalid packet type values while packet serialization/deserialization are 0000 and 1111
                    is_valid_ = false;
            }

            if (!is_valid_) {
                return ResponseCode::FAILURE;
            }

            return ResponseCode::SUCCESS;
        }

        size_t PacketFixedHeader::GetRemainingLengthByteCount() {
            size_t length = 0;

            if (remaining_length_ < 128) {
                length = 1;
            } else if (remaining_length_ < 16384) {
                length = 2;
            } else if (remaining_length_ < 2097152) {
                length = 3;
            } else {
                length = 4;
            }

            return length;
        }

        void PacketFixedHeader::AppendToBuffer(util::String &p_buf) {
            unsigned char encoded_byte = (unsigned char) 0;
            size_t length = remaining_length_;

            p_buf.append((char *) &fixed_header_byte_, 1);

            if (0 < remaining_length_) {
                do {
                    encoded_byte = (unsigned char) (length % 128);
                    length /= 128;
                    if (length > 0) {
                        encoded_byte |= 0x80;
                    }
                    p_buf.append((char *) &encoded_byte, 1);
                } while (length > 0);
            } else {
                p_buf.append((char *) &encoded_byte, 1);
            }
        }

        void Packet::AppendUInt16ToBuffer(util::String &buf, uint16_t value) {
            char first_byte = (char) (value / 256);
            char second_byte = (char) (value % 256);
            buf.append(&first_byte, 1);
            buf.append(&second_byte, 1);
        }

        uint16_t Packet::ReadUInt16FromBuffer(const util::Vector<unsigned char> &buf, size_t &extract_index) {
            uint8_t first_byte = (uint8_t) buf[extract_index++];
            uint8_t second_byte = (uint8_t) buf[extract_index++];
            uint16_t len = (uint16_t)(second_byte + (256 * first_byte));
            return len;
        }

        std::unique_ptr <Utf8String> Packet::ReadUtf8StringFromBuffer(const util::Vector<unsigned char> &buf,
                                                                      size_t &extract_index) {
            uint8_t first_byte = (uint8_t) buf.at(extract_index++);
            uint8_t second_byte = (uint8_t) buf.at(extract_index++);
            uint16_t len = (uint16_t)(second_byte + (256 * first_byte));

            if ((1 <= len) && (len <= (buf.size() - extract_index))) {
                util::String out_str(buf.begin() + extract_index, buf.begin() + extract_index + len);
                extract_index += len;
                return Utf8String::Create(out_str);
            }

            return nullptr;
        }

        void Packet::AppendUtf8StringToBuffer(util::String &buf, std::unique_ptr <Utf8String> &utf8_str) {
            size_t length = utf8_str->Length();

            if (length > 0) {
                char temp_byte = (char) (length / 256);
                buf.append(&temp_byte, 1);
                temp_byte = (char) (length % 256);
                buf.append(&temp_byte, 1);

                buf.append(utf8_str->ToStdString());
            }
        }

        void Packet::AppendUtf8StringToBuffer(util::String &buf, std::shared_ptr <Utf8String> &utf8_str) {
            size_t length = utf8_str->Length();

            if (length > 0) {
                char temp_byte = (char) (length / 256);
                buf.append(&temp_byte, 1);
                temp_byte = (char) (length % 256);
                buf.append(&temp_byte, 1);

                buf.append(utf8_str->ToStdString());
            }
        }
    }
}
