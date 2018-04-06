/*
 * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
 * @file Common.cpp
 * @brief
 *
 */

#include "mqtt/Common.hpp"
#include <iostream>

#define SINGLE_LEVEL_WILDCARD '+'
#define MULTI_LEVEL_WILDCARD '#'
#define RESERVED_TOPIC '$'
#define SINGLE_LEVEL_REGEX_STRING "[^/]*"       // Single level regex to allow all UTF-8 character except '\'
#define MULTI_LEVEL_REGEX_STRING  u8"[^\uc1bf]*"  // Placeholder for the multilevel regex to allow all UTF-8 character

namespace awsiotsdk {
    namespace mqtt {
        WillOptions::WillOptions(bool is_retained, QoS qos, std::unique_ptr<Utf8String> p_topic_name,
                                 util::String &message) {
            is_retained_ = is_retained;
            qos_ = qos;
            p_topic_name_ = std::move(p_topic_name);
            message_ = message;
            p_struct_id_ = Utf8String::Create("MQTW", 4); // Must be MQTW as per MQTT spec v3.1.1
        }
        std::unique_ptr<WillOptions> WillOptions::Create(bool is_retained, QoS qos,
                                                         std::unique_ptr<Utf8String> p_topic_name,
                                                         util::String &message) {
            if (nullptr == p_topic_name) {
                return nullptr;
            }

            return std::unique_ptr<WillOptions>(new WillOptions(is_retained, qos, std::move(p_topic_name), message));
        }

        // Define Copy constructor
        WillOptions::WillOptions(const WillOptions &source) {
            is_retained_ = source.is_retained_;
            qos_ = source.qos_;
            p_struct_id_ = Utf8String::Create(source.p_struct_id_->ToStdString());
            p_topic_name_ = Utf8String::Create(source.p_topic_name_->ToStdString());
            message_ = source.message_;
        }

        size_t WillOptions::Length() {
            return (p_topic_name_->Length() + message_.length());
        }

        // Could have used helper functions defined in packet if they were placed in a common Helper class
        // Did not make sense to define such a class since outside of packets, this is the only usage
        void WillOptions::WriteToBuffer(util::String &buf) {
            size_t length = p_topic_name_->Length();

            if (length > 0) {
                char temp_byte = (char) (length / 256);
                buf.append(&temp_byte, 1);
                temp_byte = (char) (length % 256);
                buf.append(&temp_byte, 1);

                buf.append(p_topic_name_->ToStdString());
            }

            length = message_.length();

            if (length > 0) {
                char temp_byte = (char) (length / 256);
                buf.append(&temp_byte, 1);
                temp_byte = (char) (length % 256);
                buf.append(&temp_byte, 1);

                buf.append(message_);
            }
        }

        void WillOptions::SetConnectFlags(unsigned char &p_flag) {
            if (is_retained_) {
                p_flag |= 0x20;
            }
            switch (qos_) {
                case QoS::QOS0:
                    //Do Nothing, leave bits as is assuming all bits were set to zero in buffer beforehand
                    break;
                case QoS::QOS1:
                    p_flag |= 0x08;
                    break;
            }
            p_flag |= 0x04; // Always set this bit to one when will message is present
        }

        SubscriptionHandlerContextData::~SubscriptionHandlerContextData() {}

        bool Subscription::IsValidTopicName(util::String p_topic_name) {
            if (1 == p_topic_name.length()) {
                if (RESERVED_TOPIC == p_topic_name[0]) {
                    return false;
                }
                else {
                    return true;
                }
            }

            util::String::iterator it;
            for (it = p_topic_name.begin(); it < p_topic_name.end(); ++it) {
                if (*it == SINGLE_LEVEL_WILDCARD) {
                    if (it == p_topic_name.begin()) {
                        if (*(it + 1) != '/' && it + 1 != p_topic_name.end()) {
                            return false;
                        }
                    } else if (it + 1 == p_topic_name.end()) {
                        if (*(it - 1) != '/') {
                            return false;
                        }
                    } else if (*(it - 1) != '/' || *(it + 1) != '/') {
                        return false;
                    }
                } else if (*it == MULTI_LEVEL_WILDCARD) {
                    if (it + 1 != p_topic_name.end()) {
                        return false;
                    } else if (it != p_topic_name.begin() && *(it - 1) != '/') {
                        return false;
                    }
                }
            }
            return true;
        }

        std::shared_ptr<Subscription> Subscription::Create(std::unique_ptr<Utf8String> p_topic_name,
                                                           QoS max_qos,
                                                           ApplicationCallbackHandlerPtr p_app_handler,
                                                           std::shared_ptr<SubscriptionHandlerContextData> p_app_handler_data) {
            if (nullptr == p_topic_name || nullptr == p_app_handler) {
                return nullptr;
            }

            if (false == IsValidTopicName(p_topic_name->ToStdString())) {
                return nullptr;
            }

            return std::shared_ptr<Subscription>(new Subscription(std::move(p_topic_name),
                                                                  max_qos,
                                                                  p_app_handler,
                                                                  p_app_handler_data));
        }

        Subscription::Subscription(std::unique_ptr<Utf8String> p_topic_name,
                                   QoS max_qos,
                                   ApplicationCallbackHandlerPtr p_app_handler,
                                   std::shared_ptr<SubscriptionHandlerContextData> p_app_handler_data) {
            is_active_ = false;
            index_in_packet_ = 0;
            packet_id_ = 0;
            p_topic_name_ = std::shared_ptr<Utf8String>(std::move(p_topic_name));
            max_qos_ = max_qos;
            p_app_handler_ = p_app_handler;
            p_app_handler_data_ = p_app_handler_data;

            // Add regex for topic
            p_topic_regex_ = "";
            if (p_topic_name_->ToStdString().find("#") != util::String::npos
                || p_topic_name_->ToStdString().find("+") != util::String::npos) {
                for (auto it : p_topic_name_->ToStdString()) {
                    if (it == SINGLE_LEVEL_WILDCARD) {
                        p_topic_regex_.append(SINGLE_LEVEL_REGEX_STRING);
                    } else if (it == MULTI_LEVEL_WILDCARD) {
                        p_topic_regex_.append(MULTI_LEVEL_REGEX_STRING);
                    } else if (it == RESERVED_TOPIC) {
                        p_topic_regex_ += "\\";
                        p_topic_regex_ += RESERVED_TOPIC;
                    } else {
                        p_topic_regex_ += it;
                    }
                }
            }
        }
    }
}
