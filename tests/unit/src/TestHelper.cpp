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
 * @file TestHelper.cpp
 * @brief
 *
 */

#include "TestHelper.hpp"

namespace awsiotsdk {
    namespace tests {
        void TestHelper::WriteCharToBuffer(unsigned char **p_buf, unsigned char value) {
            **p_buf = value;
            (*p_buf)++;
        }

        void TestHelper::WriteUint16ToBuffer(unsigned char **p_buf, uint16_t value) {
            **p_buf = (unsigned char) (value / 256);
            (*p_buf)++;
            **p_buf = (unsigned char) (value % 256);
            (*p_buf)++;
        }

        unsigned char TestHelper::ReadCharFromBuffer(unsigned char **p_buf) {
            unsigned char c = **p_buf;
            (*p_buf)++;
            return c;
        }

        uint16_t TestHelper::ReadUint16FromBuffer(unsigned char **p_buf) {
            unsigned char *ptr = *p_buf;
            uint16_t len = 0;
            uint8_t first_byte = (uint8_t) (*ptr);
            uint8_t second_byte = (uint8_t) (*(ptr + 1));
            len = (uint16_t) (second_byte + (256 * first_byte));

            *p_buf += 2;
            return len;
        }

        util::String TestHelper::GetEncodedRemLen(size_t rem_len) {
            char rem_len_buf[MAX_NO_OF_REMAINING_LENGTH_BYTES];
            char *p_rem_len = rem_len_buf;
            size_t encoded_byte_count = 0;
            char encoded_byte;

            if (0 < rem_len) {
                do {
                    encoded_byte = (unsigned char) (rem_len % 128);
                    rem_len /= 128;
                    /* if there are more digits to encode, set the top bit of this digit */
                    if (rem_len > 0) {
                        encoded_byte |= 0x80;
                    }
                    //std::cout << "encoded_byte : " << (int) encoded_byte << std::endl;
                    *p_rem_len = encoded_byte;
                    p_rem_len++;
                    encoded_byte_count++;
                } while (rem_len > 0);
            } else {
                *p_rem_len = 0;
                encoded_byte_count++;
            }

            return util::String(rem_len_buf, encoded_byte_count);
        }

        size_t TestHelper::ParseRemLenFromBuffer(unsigned char **p_buf) {
            size_t len = 1;
            size_t multiplier = 1;
            unsigned char *ptr = *p_buf;
            size_t calculated_rem_len = (size_t) ((*ptr & 127) * multiplier);

            while (0 != (*ptr & 128)) {
                ptr++;
                if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES) {
                    /* bad data */
                    calculated_rem_len = 0;
                    break;
                }

                calculated_rem_len += (size_t) ((*ptr & 127) * multiplier);
                multiplier *= 128;
            }
            *p_buf = ptr + 1;
            return calculated_rem_len;
        }

        std::unique_ptr<Utf8String> TestHelper::ReadUtf8StringFromBuffer(unsigned char **p_buf) {
            unsigned char *ptr = *p_buf;
            std::unique_ptr<Utf8String> utf8String = nullptr;
            uint16_t len = 0;

            uint8_t firstByte = (uint8_t) (*ptr);
            uint8_t secondByte = (uint8_t) (*(ptr + 1));
            len = (uint16_t) (secondByte + (256 * firstByte));

            *p_buf += 2;
            ptr += 2;

            if (len >= 1) {
                std::unique_ptr<char[]> read_string = std::unique_ptr<char[]>(new char[len]);
                std::copy(ptr, ptr + len, read_string.get());
                *p_buf += len;
                utf8String = Utf8String::Create(read_string.get(), len);
            }

            return utf8String;
        }

        util::String TestHelper::GetSerializedPublishMessage(util::String topic_name,
                                                             uint16_t packet_id,
                                                             mqtt::QoS qos,
                                                             bool is_duplicate,
                                                             bool is_retained,
                                                             util::String payload) {
            char fixed_header_byte = 0;

            size_t topic_name_len = topic_name.length();
            size_t rem_len = 2 + topic_name_len + payload.length();
            if (qos == mqtt::QoS::QOS1) {
                rem_len += 2; // packet ID
            }
            util::String encoded_rem_len = GetEncodedRemLen(rem_len);
            size_t serialized_len = 1 + encoded_rem_len.length() + rem_len;

            fixed_header_byte = PUBLISH_PACKET_FIXED_HEADER_VAL;
            switch (qos) {
                case mqtt::QoS::QOS0:
                    fixed_header_byte |= 0x00;
                    break;
                case mqtt::QoS::QOS1:
                    fixed_header_byte |= 0x02;
                    break;
                    // Strongly typed enum, no default required
            }

            fixed_header_byte |= (is_duplicate ? 0x08 : 0x00);
            fixed_header_byte |= (is_retained ? 0x01 : 0x00);

            util::String buf;
            buf.reserve(serialized_len);
            buf.append(&fixed_header_byte, 1);
            buf.append(encoded_rem_len);

            char temp_byte = (char) (topic_name_len / 256);
            buf.append(&temp_byte, 1);
            temp_byte = (char) (topic_name_len % 256);
            buf.append(&temp_byte, 1);
            buf.append(topic_name);

            if (qos == mqtt::QoS::QOS1) {
                temp_byte = (char) (packet_id / 256);
                buf.append(&temp_byte, 1);
                temp_byte = (char) (packet_id % 256);
                buf.append(&temp_byte, 1);
            }

            buf.append(payload);
            return buf;
        }

        util::String TestHelper::GetSerializedSubAckMessage(uint16_t packet_id, std::vector<uint8_t> suback_list_) {
            size_t rem_len = 2 + suback_list_.size();
            util::String encoded_rem_len = GetEncodedRemLen(rem_len);
            size_t serialized_len = 1 + encoded_rem_len.length() + rem_len;

            util::String buf;
            buf.reserve(serialized_len);

            char temp_byte = (char) SUBACK_PACKET_FIXED_HEADER_VAL;
            buf.append(&temp_byte, 1);
            buf.append(encoded_rem_len);

            temp_byte = (char) (packet_id / 256);
            buf.append(&temp_byte, 1);
            temp_byte = (char) (packet_id % 256);
            buf.append(&temp_byte, 1);
            for (uint8_t suback_val : suback_list_) {
                temp_byte = (char) suback_val;
                buf.append(&temp_byte, 1);
            }

            return buf;
        }

        util::String TestHelper::GetSerializedUnsubAckMessage(uint16_t packet_id) {
            size_t rem_len = PUBACK_PACKET_REM_LEN_VAL;
            util::String encoded_rem_len = GetEncodedRemLen(rem_len);
            size_t serialized_len = 1 + encoded_rem_len.length() + rem_len;

            util::String buf;
            buf.reserve(serialized_len);

            char temp_byte = (char) UNSUBACK_PACKET_FIXED_HEADER_VAL;
            buf.append(&temp_byte, 1);
            buf.append(encoded_rem_len);

            temp_byte = (char) (packet_id / 256);
            buf.append(&temp_byte, 1);
            temp_byte = (char) (packet_id % 256);
            buf.append(&temp_byte, 1);

            return buf;
        }

        util::String TestHelper::GetSerializedPubAckMessage(uint16_t packet_id) {
            size_t rem_len = PUBACK_PACKET_REM_LEN_VAL;
            util::String encoded_rem_len = GetEncodedRemLen(rem_len);
            size_t serialized_len = 1 + encoded_rem_len.length() + rem_len;

            util::String buf;
            buf.reserve(serialized_len);

            char temp_byte = (char) PUBACK_PACKET_FIXED_HEADER_VAL;
            buf.append(&temp_byte, 1);
            buf.append(encoded_rem_len);

            temp_byte = (char) (packet_id / 256);
            buf.append(&temp_byte, 1);
            temp_byte = (char) (packet_id % 256);
            buf.append(&temp_byte, 1);

            return buf;
        }

        util::String TestHelper::GetSerializedConnAckMessage(bool is_session_present,
                                                             ConnackTestReturnCode connack_rc) {
            size_t rem_len = CONNACK_PACKET_REM_LEN_VAL;
            util::String encoded_rem_len = GetEncodedRemLen(rem_len);
            size_t serialized_len = 1 + encoded_rem_len.length() + rem_len;

            util::String buf;
            buf.reserve(serialized_len);

            char temp_byte = (char) CONNACK_PACKET_FIXED_HEADER_VAL;
            buf.append(&temp_byte, 1);
            buf.append(encoded_rem_len);

            temp_byte = is_session_present ? (char) 0 : (char) 1;
            buf.append(&temp_byte, 1);

            temp_byte = static_cast<char>(connack_rc);
            buf.append(&temp_byte, 1);

            return buf;
        }
    }
}
