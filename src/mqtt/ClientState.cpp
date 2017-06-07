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
 * @file ClientState.cpp
 * @brief
 *
 */

#include <regex>

#include "mqtt/ClientState.hpp"

#define MIN_RECONNECT_BACKOFF_DEFAULT_SEC 1
#define MAX_RECONNECT_BACKOFF_DEFAULT_SEC 128

namespace awsiotsdk {
    namespace mqtt {
        ClientState::ClientState(std::chrono::milliseconds mqtt_command_timeout) {
            is_session_present_ = false;
            is_connected_ = false;
            is_pingreq_pending_ = false;
            is_auto_reconnect_required_ = false;
            is_auto_reconnect_enabled_ = true;
            last_sent_packet_id_ = 0;
            mqtt_command_timeout_ = mqtt_command_timeout;
            p_connect_data_ = nullptr;
            min_reconnect_backoff_timeout_ = std::chrono::seconds(MIN_RECONNECT_BACKOFF_DEFAULT_SEC);
            max_reconnect_backoff_timeout_ = std::chrono::seconds(MAX_RECONNECT_BACKOFF_DEFAULT_SEC);
        }
        std::shared_ptr<ClientState> ClientState::Create(std::chrono::milliseconds mqtt_command_timeout) {
            return std::make_shared<ClientState>(mqtt_command_timeout);
        }

        uint16_t ClientState::GetNextPacketId() {
            if (UINT16_MAX == last_sent_packet_id_) {
                // 0 is reserved for CONNACK
                last_sent_packet_id_ = 1;
            } else {
                ++last_sent_packet_id_;
            }
            return last_sent_packet_id_;
        }

        std::shared_ptr<Subscription> ClientState::GetSubscription(util::String p_topic_name) {
            std::shared_ptr<Subscription> p_sub = nullptr;

            util::Map<util::String, std::shared_ptr<Subscription >>::const_iterator find_itr =
                std::find_if(subscription_map_.begin(),
                             subscription_map_.end(),
                             [p_topic_name](const std::pair<util::String, std::shared_ptr<Subscription>> &s) -> bool {
                                 if (s.first == p_topic_name) {
                                     return true;
                                 }
                                 if (0 < s.second->p_topic_regex_.length()) {
                                     std::regex wildcard_regex(s.second->p_topic_regex_, std::regex::ECMAScript);
                                     return std::regex_match(p_topic_name.c_str(),
                                                             wildcard_regex);
                                 }
                                 return false;
                             });
            if (find_itr != subscription_map_.end()) {
                p_sub = find_itr->second;
            }

            return p_sub;
        }

        std::shared_ptr<Subscription> ClientState::SetSubscriptionPacketInfo(util::String p_topic_name,
                                                                             uint16_t packet_id,
                                                                             uint8_t index_in_packet) {
            std::shared_ptr<Subscription> p_sub = nullptr;
            util::Map<util::String, std::shared_ptr<Subscription >>::const_iterator
                itr = subscription_map_.find(p_topic_name);
            if (itr != subscription_map_.end()) {
                itr->second->SetAckIndex(packet_id, index_in_packet);
            }

            return p_sub;
        }

        ResponseCode ClientState::SetSubscriptionActive(uint16_t packet_id,
                                                        uint8_t index_in_sub_packet,
                                                        mqtt::QoS max_qos) {
            ResponseCode rc = ResponseCode::FAILURE;
            util::Map<util::String, std::shared_ptr<Subscription >>::const_iterator itr = subscription_map_.begin();
            while (itr != subscription_map_.end()) {
                if (itr->second->IsInSuback(packet_id, index_in_sub_packet)) {
                    itr->second->SetActive(true);
                    itr->second->SetMaxQos(max_qos);
                    itr->second->SetAckIndex(0, 0); // Reset Packet index to prevent corruptions when packetid cycles back
                    rc = ResponseCode::SUCCESS;
                    break;
                }
                itr++;
            }
            return rc;
        }

        ResponseCode ClientState::RemoveSubscription(util::String p_topic_name) {
            subscription_map_.erase(p_topic_name);
            return ResponseCode::SUCCESS;
        }

        ResponseCode ClientState::RemoveSubscription(uint16_t packet_id, uint8_t index_in_sub_packet) {
            ResponseCode rc = ResponseCode::FAILURE;
            util::Map<util::String, std::shared_ptr<Subscription >>::const_iterator itr = subscription_map_.begin();
            while (itr != subscription_map_.end()) {
                if (itr->second->IsInSuback(packet_id, index_in_sub_packet)) {
                    itr = subscription_map_.erase(itr);
                    break;
                }
                itr++;
            }
            return rc;
        }

        ResponseCode ClientState::RemoveAllSubscriptionsForPacketId(uint16_t packet_id) {
            ResponseCode rc = ResponseCode::FAILURE;
            util::Map<util::String, std::shared_ptr<Subscription >>::const_iterator itr = subscription_map_.begin();
            while (itr != subscription_map_.end()) {
                if (itr->second->GetPacketId() == packet_id) {
                    itr = subscription_map_.erase(itr);
                } else {
                    itr++;
                }
            }
            return rc;
        }
    }
}

