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
 * @file Subscribe.cpp
 * @brief MQTT Subscribe and Unsubscribe Actions and Action data definitions for IoT Client
 *
 * Defines classes for perform MQTT Subscribe and Unsubscribe Actions in Async mode for the IoT Client.
 * Also defines the packet types used by these actions as well as the related Ack packet types.
 */

#include "util/logging/LogMacros.hpp"
#include "util/memory/stl/Vector.hpp"

#include "mqtt/ClientState.hpp"
#include "mqtt/Subscribe.hpp"

#define SUBSCRIBE_ACTION_DESCRIPTION "MQTT Subscribe Action"
#define UNSUBSCRIBE_ACTION_DESCRIPTION "MQTT Unsubscribe Action"

#define SUBSCRIBE_ACTION_LOG_TAG "[Subscribe]"
#define UNSUBSCRIBE_ACTION_LOG_TAG "[Unsubscribe]"

namespace awsiotsdk {
    namespace mqtt {
        /**********************************************
         * SubscribePacket class function definitions *
         *********************************************/

        SubscribePacket::SubscribePacket(util::Vector<std::shared_ptr<Subscription>> subscription_list) {
            packet_size_ = 2; // Packet ID requires 2 bytes
            subscription_list_ = subscription_list;
            packet_id_ = 0; // Initialized by ClientCore

            util::Vector<std::shared_ptr<Subscription>>::iterator itr;
            for (itr = subscription_list.begin(); itr < subscription_list.end(); ++itr) {
                // 2 bytes for topic length, 1 for QoS
                packet_size_ = packet_size_ + (*itr)->GetTopicNameLength() + 2 + 1;
            }

            fixed_header_.Initialize(MessageTypes::SUBSCRIBE, false, QoS::QOS0, false, packet_size_);

            serialized_packet_length_ = packet_size_ + fixed_header_.Length();
        }

        std::shared_ptr<SubscribePacket> SubscribePacket::Create(util::Vector<std::shared_ptr<Subscription>> subscription_list) {
            if (subscription_list.empty() || MAX_TOPICS_IN_ONE_SUBSCRIBE_PACKET < subscription_list.size()) {
                return nullptr;
            }

            return std::make_shared<SubscribePacket>(subscription_list);
        }

        util::String SubscribePacket::ToString() {
            util::String buf;
            buf.reserve(serialized_packet_length_);
            char temp_qos_byte;

            fixed_header_.AppendToBuffer(buf);
            AppendUInt16ToBuffer(buf, static_cast<uint16_t>(packet_id_));

            uint8_t itr_index;
            util::Vector<std::shared_ptr<Subscription>>::iterator itr;
            for (itr = subscription_list_.begin(), itr_index = 1; itr < subscription_list_.end(); ++itr, ++itr_index) {
                std::shared_ptr<Utf8String> utf8_str = (*itr)->GetTopicName();
                AppendUtf8StringToBuffer(buf, utf8_str);
                switch ((*itr)->GetMaxQos()) {
                    case QoS::QOS0:
                        temp_qos_byte = 0x00;
                        break;
                    case QoS::QOS1:
                        temp_qos_byte = 0x01;
                        break;
                }
                buf.append(&temp_qos_byte, 1);
                (*itr)->SetAckIndex(static_cast<uint16_t>(packet_id_), itr_index);
            }

            return buf;
        }

        /*******************************************
         * SubackPacket class function definitions *
         ******************************************/
        SubackPacket::SubackPacket(const util::Vector<unsigned char> &buf) {
            size_t extract_index = 0;

            packet_size_ = buf.size();
            fixed_header_.Initialize(MessageTypes::SUBACK, false, QoS::QOS0, false, packet_size_);
            serialized_packet_length_ = packet_size_ + fixed_header_.Length();
            packet_id_ = ReadUInt16FromBuffer(buf, extract_index);

            for (; extract_index < packet_size_; extract_index++) {
                suback_list_.push_back(static_cast<uint8_t>(buf.at(extract_index)));
            }
        }

        std::shared_ptr<SubackPacket> SubackPacket::Create(const util::Vector<unsigned char> &buf) {
            if (0 == buf.size()) {
                return nullptr;
            }

            return std::make_shared<SubackPacket>(buf);
        }

        util::String SubackPacket::ToString() {
            util::String buf;
            buf.reserve(serialized_packet_length_);

            fixed_header_.AppendToBuffer(buf);
            AppendUInt16ToBuffer(buf, static_cast<uint16_t>(packet_id_));

            char temp;
            for (uint8_t suback_info : suback_list_) {
                temp = static_cast<char>(suback_info);
                buf.append(&temp, 1);
            }

            return buf;
        }

        /************************************************
         * UnsubscribePacket class function definitions *
         ***********************************************/
        UnsubscribePacket::UnsubscribePacket(util::Vector<std::unique_ptr<Utf8String>> topic_list) {
            packet_size_ = 2; // Packet ID requires 2 bytes
            packet_id_ = 0; // Initialized by ClientCore
            topic_list_ = std::move(topic_list);

            util::Vector<std::unique_ptr<Utf8String>>::iterator itr;
            for (itr = topic_list_.begin(); itr < topic_list_.end(); ++itr) {
                packet_size_ = packet_size_ + (*itr)->Length() + 2; // 2 bytes for topic length, 1 for QoS
            }

            fixed_header_.Initialize(MessageTypes::UNSUBSCRIBE, false, QoS::QOS0, false, packet_size_);
            serialized_packet_length_ = packet_size_ + fixed_header_.Length();
        }

        std::shared_ptr<UnsubscribePacket> UnsubscribePacket::Create(util::Vector<std::unique_ptr<Utf8String>> topic_list) {
            if (topic_list.empty() || MAX_TOPICS_IN_ONE_SUBSCRIBE_PACKET < topic_list.size()) {
                return nullptr;
            }

            return std::make_shared<UnsubscribePacket>(std::move(topic_list));
        }

        util::String UnsubscribePacket::ToString() {
            util::String buf;
            buf.reserve(serialized_packet_length_);

            fixed_header_.AppendToBuffer(buf);
            AppendUInt16ToBuffer(buf, static_cast<uint16_t>(packet_id_));

            util::Vector<std::unique_ptr<Utf8String>>::iterator itr;
            for (itr = topic_list_.begin(); itr < topic_list_.end(); ++itr) {
                AppendUtf8StringToBuffer(buf, (*itr));
            }

            return buf;
        }

        /*********************************************
         * UnsubackPacket class function definitions *
         ********************************************/
        UnsubackPacket::UnsubackPacket(const util::Vector<unsigned char> &buf) {
            size_t extract_index = 0;

            packet_size_ = buf.size();
            fixed_header_.Initialize(MessageTypes::UNSUBACK, false, QoS::QOS0, false, packet_size_);
            serialized_packet_length_ = packet_size_ + fixed_header_.Length();
            packet_id_ = ReadUInt16FromBuffer(buf, extract_index);
        }

        std::shared_ptr<UnsubackPacket> UnsubackPacket::Create(const util::Vector<unsigned char> &buf) {
            if (0 == buf.size()) {
                return nullptr;
            }

            return std::make_shared<UnsubackPacket>(buf);
        }

        util::String UnsubackPacket::ToString() {
            util::String buf;
            buf.reserve(serialized_packet_length_);

            fixed_header_.AppendToBuffer(buf);
            AppendUInt16ToBuffer(buf, static_cast<uint16_t>(packet_id_));

            return buf;
        }

        /***************************************************
         * SubscribeActionAsync class function definitions *
         **************************************************/
        SubscribeActionAsync::SubscribeActionAsync(std::shared_ptr<ClientState> p_client_state)
            : Action(ActionType::SUBSCRIBE, SUBSCRIBE_ACTION_DESCRIPTION) {
            p_client_state_ = p_client_state;
        }

        std::unique_ptr<Action> SubscribeActionAsync::Create(std::shared_ptr<ActionState> p_action_state) {
            std::shared_ptr<mqtt::ClientState>
                p_client_state = std::dynamic_pointer_cast<mqtt::ClientState>(p_action_state);
            if (nullptr == p_client_state) {
                return nullptr;
            }

            return std::unique_ptr<SubscribeActionAsync>(new SubscribeActionAsync(p_client_state));
        }

        ResponseCode SubscribeActionAsync::PerformAction(std::shared_ptr<NetworkConnection> p_network_connection,
                                                         std::shared_ptr<ActionData> p_action_data) {
            if (nullptr == p_network_connection) {
                return ResponseCode::NULL_VALUE_ERROR;
            }

            ResponseCode rc = ResponseCode::SUCCESS;
            bool is_ack_registered = false;
            std::shared_ptr<SubscribePacket>
                p_subscribe_packet = std::dynamic_pointer_cast<SubscribePacket>(p_action_data);
            if (nullptr == p_subscribe_packet) {
                return ResponseCode::NULL_VALUE_ERROR;
            }

            uint16_t packet_id = p_subscribe_packet->GetPacketId();
            if (nullptr != p_subscribe_packet->p_async_ack_handler_) {
                rc = p_client_state_->RegisterPendingAck(packet_id, p_subscribe_packet->p_async_ack_handler_);
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(SUBSCRIBE_ACTION_LOG_TAG,
                                  "Registering Ack Handler for Connect Action failed. %s",
                                  ResponseHelper::ToString(rc).c_str());
                } else {
                    is_ack_registered = true;
                }
            }

            // Read running in separate thread, Insert before sending request to avoid situations where response arrives early
            util::Vector<std::shared_ptr<Subscription>>::iterator itr = p_subscribe_packet->subscription_list_.begin();
            while (itr != p_subscribe_packet->subscription_list_.end()) {
                util::String topic_name = (*itr)->GetTopicName()->ToStdString();
                auto existing_itr = p_client_state_->subscription_map_.find(topic_name);
                if (p_client_state_->subscription_map_.end() != existing_itr) {
                    if (existing_itr->second->IsActive()) {
                        itr = p_subscribe_packet->subscription_list_.erase(itr);
                        // TODO: This needs to be reworked
                        continue;
                    } else {
                        p_client_state_->subscription_map_.erase(existing_itr);
                        p_client_state_->subscription_map_.insert(std::make_pair(topic_name, (*itr)));
                    }
                } else {
                    p_client_state_->subscription_map_.insert(std::make_pair(topic_name, (*itr)));
                }

                itr++;
            }

            const util::String packet_data = p_subscribe_packet->ToString();
            rc = WriteToNetworkBuffer(p_network_connection, packet_data);
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(SUBSCRIBE_ACTION_LOG_TAG, "Subscribe Write to Network Failed. %s",
                              ResponseHelper::ToString(rc).c_str());
                // Remove acks
                for (itr = p_subscribe_packet->subscription_list_.begin();
                     itr < p_subscribe_packet->subscription_list_.end(); ++itr) {
                    util::String topic_name = (*itr)->GetTopicName()->ToStdString();
                    p_client_state_->subscription_map_.erase(topic_name);
                }
                if (is_ack_registered) {
                    p_client_state_->DeletePendingAck(packet_id);
                }
                return rc;
            }

            return ResponseCode::SUCCESS;
        }

        /*****************************************************
         * UnsubscribeActionAsync class function definitions *
         ****************************************************/
        UnsubscribeActionAsync::UnsubscribeActionAsync(std::shared_ptr<ClientState> p_client_state)
            : Action(ActionType::UNSUBSCRIBE, UNSUBSCRIBE_ACTION_DESCRIPTION) {
            p_client_state_ = p_client_state;
        }

        std::unique_ptr<Action> UnsubscribeActionAsync::Create(std::shared_ptr<ActionState> p_action_state) {
            std::shared_ptr<mqtt::ClientState>
                p_client_state = std::dynamic_pointer_cast<mqtt::ClientState>(p_action_state);
            if (nullptr == p_client_state) {
                return nullptr;
            }

            return std::unique_ptr<UnsubscribeActionAsync>(new UnsubscribeActionAsync(p_client_state));
        }

        ResponseCode UnsubscribeActionAsync::PerformAction(std::shared_ptr<NetworkConnection> p_network_connection,
                                                           std::shared_ptr<ActionData> p_action_data) {
            std::shared_ptr<UnsubscribePacket>
                p_unsubscribe_packet = std::dynamic_pointer_cast<UnsubscribePacket>(p_action_data);
            if (nullptr == p_unsubscribe_packet) {
                return ResponseCode::NULL_VALUE_ERROR;
            }

            ResponseCode rc = ResponseCode::SUCCESS;
            bool is_ack_registered = false;

            if (nullptr != p_unsubscribe_packet->p_async_ack_handler_) {
                rc = p_client_state_->RegisterPendingAck(p_unsubscribe_packet->GetPacketId(),
                                                         p_unsubscribe_packet->p_async_ack_handler_);
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(UNSUBSCRIBE_ACTION_LOG_TAG,
                                  "Registering Ack Handler for Connect Action failed. %s",
                                  ResponseHelper::ToString(rc).c_str());
                } else {
                    is_ack_registered = true;
                }
            }

            uint16_t packet_id = p_unsubscribe_packet->GetPacketId();
            for (auto &&itr : p_unsubscribe_packet->topic_list_) {
                p_client_state_->SetSubscriptionPacketInfo(itr->ToStdString(), packet_id, 0);
            }

            const util::String packet_data = p_unsubscribe_packet->ToString();
            rc = WriteToNetworkBuffer(p_network_connection, packet_data);
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(UNSUBSCRIBE_ACTION_LOG_TAG, "Publish Write to Network Failed. %s",
                              ResponseHelper::ToString(rc).c_str());
                if (is_ack_registered) {
                    p_client_state_->DeletePendingAck(packet_id);
                }
            }

            return rc;
        }
    }
}
 
