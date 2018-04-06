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
 * @file Publish.cpp
 * @brief MQTT Publish and Puback Actions and Action data definitions for IoT Client
 *
 * Defines classes for perform MQTT Publish and Puback Actions in Async mode for the IoT Client.
 * Also defines the packet types used by these actions.
 */

#include "util/logging/LogMacros.hpp"

#include "mqtt/ClientState.hpp"
#include "mqtt/Publish.hpp"

#define PUBLISH_ACTION_DESCRIPTION "MQTT Publish Action"
#define PUBACK_ACTION_DESCRIPTION "MQTT Puback Action"

#define PUBLISH_ACTION_LOG_TAG "[Publish]"
#define PUBACK_ACTION_LOG_TAG "[Puback]"

namespace awsiotsdk {
    namespace mqtt {

        /********************************************
         * PublishPacket class function definitions *
         *******************************************/
        PublishPacket::PublishPacket(std::unique_ptr<Utf8String> p_topic_name,
                                     bool is_retained,
                                     bool is_duplicate,
                                     QoS qos,
                                     const util::String &payload) {
            packet_size_ = p_topic_name->Length() + 2 + payload.length(); // length of topic name requires 2 bytes

            if (QoS::QOS0 != qos) {
                packet_size_ += 2; // Packet ID requires 2 bytes in case of QoS1 and QoS2
            }

            p_topic_name_ = std::move(p_topic_name);
            payload_.clear();

            // If payload is not zero length, add it to the packet
            if (0 != payload.length()) {
                payload_ = payload;
            }

            is_retained_ = is_retained;
            is_duplicate_ = is_duplicate;
            if (QoS::QOS0 == qos) {
                // Must be false for QoS0 messages
                is_duplicate_ = false;
            }
            packet_id_ = 0; // Initialized by ClientCore
            qos_ = qos;

            fixed_header_.Initialize(MessageTypes::PUBLISH, is_duplicate, qos, is_retained, packet_size_);

            serialized_packet_length_ = packet_size_ + fixed_header_.Length();
        }

        PublishPacket::PublishPacket(const util::Vector<unsigned char> &buf,
                                     bool is_retained,
                                     bool is_duplicate,
                                     QoS qos) {
            size_t extract_index = 0;

            is_retained_ = is_retained;
            is_duplicate_ = is_duplicate;
            qos_ = qos;

            p_topic_name_ = std::unique_ptr<Utf8String>(ReadUtf8StringFromBuffer(buf, extract_index));

            if (qos != QoS::QOS0) {
                SetPacketId(ReadUInt16FromBuffer(buf, extract_index));
            }

            if (extract_index == buf.size()) {
                // Zero length payload
                payload_.clear();
            } else {
                payload_ = util::String(buf.begin() + extract_index, buf.end());
            }

            packet_size_ = p_topic_name_->Length() + 2 + payload_.length(); // length of topic name requires 2 bytes

            fixed_header_.Initialize(MessageTypes::PUBLISH, is_duplicate, qos, is_retained, packet_size_);

            serialized_packet_length_ = packet_size_ + fixed_header_.Length();
        }

        std::shared_ptr<PublishPacket> PublishPacket::Create(std::unique_ptr<Utf8String> p_topic_name,
                                                             bool is_retained,
                                                             bool is_duplicate,
                                                             QoS qos,
                                                             const util::String &payload) {
            if (nullptr == p_topic_name) {
                return nullptr;
            }
            return std::make_shared<PublishPacket>(std::move(p_topic_name), is_retained, is_duplicate, qos, payload);
        }

        std::shared_ptr<PublishPacket> PublishPacket::Create(const util::Vector<unsigned char> &buf,
                                                             bool is_retained,
                                                             bool is_duplicate,
                                                             QoS qos) {
            if (3 > buf.size()) {
                // Must be at least length 3 to be contain a valid Utf8String
                return nullptr;
            }
            return std::make_shared<PublishPacket>(buf, is_retained, is_duplicate, qos);
        }

        util::String PublishPacket::ToString() {
            util::String buf;
            buf.reserve(serialized_packet_length_);

            fixed_header_.AppendToBuffer(buf);
            AppendUtf8StringToBuffer(buf, p_topic_name_);

            if (QoS::QOS0 != qos_) {
                AppendUInt16ToBuffer(buf, GetPacketId());
            }

            buf.append(payload_);
            return buf;
        }

        /*******************************************
         * PubackPacket class function definitions *
         ******************************************/
        PubackPacket::PubackPacket(uint16_t publish_packet_id) {
            packet_size_ = 2; // Packet ID requires 2 bytes in case of QoS1 and QoS2
            publish_packet_id_.store(publish_packet_id, std::memory_order_relaxed);
            fixed_header_.Initialize(MessageTypes::PUBACK, false, QoS::QOS0, false, packet_size_);
            serialized_packet_length_ = packet_size_ + fixed_header_.Length();
        }

        std::shared_ptr<PubackPacket> PubackPacket::Create(uint16_t packet_id) {
            return std::make_shared<PubackPacket>(packet_id);
        }

        util::String PubackPacket::ToString() {
            util::String buf;
            buf.reserve(serialized_packet_length_);
            fixed_header_.AppendToBuffer(buf);
            AppendUInt16ToBuffer(buf, publish_packet_id_.load(std::memory_order_relaxed));

            return buf;
        }

        /*************************************************
         * PublishActionAsync class function definitions *
         ************************************************/
        PublishActionAsync::PublishActionAsync(std::shared_ptr<ClientState> p_client_state)
            : Action(ActionType::PUBLISH, PUBLISH_ACTION_DESCRIPTION) {
            p_client_state_ = p_client_state;
        }

        std::unique_ptr<Action> PublishActionAsync::Create(std::shared_ptr<ActionState> p_action_state) {
            std::shared_ptr<mqtt::ClientState>
                p_client_state = std::dynamic_pointer_cast<mqtt::ClientState>(p_action_state);
            if (nullptr == p_client_state) {
                return nullptr;
            }

            return std::unique_ptr<PublishActionAsync>(new PublishActionAsync(p_client_state));
        }

        ResponseCode PublishActionAsync::PerformAction(std::shared_ptr<NetworkConnection> p_network_connection,
                                                       std::shared_ptr<ActionData> p_action_data) {
            std::shared_ptr<PublishPacket> p_publish_packet = std::dynamic_pointer_cast<PublishPacket>(p_action_data);
            if (nullptr == p_publish_packet) {
                return ResponseCode::NULL_VALUE_ERROR;
            }

            bool is_ack_registered = false;
            ResponseCode rc = ResponseCode::SUCCESS;
            uint16_t packet_id = p_publish_packet->GetPacketId();
            if (QoS::QOS0 != p_publish_packet->GetQoS() && nullptr != p_publish_packet->p_async_ack_handler_) {
                rc = p_client_state_->RegisterPendingAck(packet_id, p_publish_packet->p_async_ack_handler_);
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(PUBLISH_ACTION_LOG_TAG,
                                  "Registering Ack Handler for Connect Action failed. %s",
                                  ResponseHelper::ToString(rc).c_str());
                } else {
                    is_ack_registered = true;
                }
            }

            const util::String packet_data = p_publish_packet->ToString();
            rc = WriteToNetworkBuffer(p_network_connection, packet_data);
            if (ResponseCode::SUCCESS != rc) {
                if (is_ack_registered) {
                    p_client_state_->DeletePendingAck(packet_id);
                }
                AWS_LOG_ERROR(PUBLISH_ACTION_LOG_TAG, "Publish Write to Network Failed. %s",
                              ResponseHelper::ToString(rc).c_str());
            }
            return rc;
        }

        /************************************************
         * PubackActionAsync class function definitions *
         ***********************************************/
        PubackActionAsync::PubackActionAsync(std::shared_ptr<ClientState> p_client_state)
            : Action(ActionType::PUBACK, PUBACK_ACTION_DESCRIPTION) {
            p_client_state_ = p_client_state;
        }

        std::unique_ptr<Action> PubackActionAsync::Create(std::shared_ptr<ActionState> p_action_state) {
            std::shared_ptr<mqtt::ClientState>
                p_client_state = std::dynamic_pointer_cast<mqtt::ClientState>(p_action_state);
            if (nullptr == p_client_state) {
                return nullptr;
            }

            return std::unique_ptr<PubackActionAsync>(new PubackActionAsync(p_client_state));
        }

        ResponseCode PubackActionAsync::PerformAction(std::shared_ptr<NetworkConnection> p_network_connection,
                                                      std::shared_ptr<ActionData> p_action_data) {
            std::shared_ptr<PubackPacket> p_puback_packet = std::dynamic_pointer_cast<PubackPacket>(p_action_data);
            if (nullptr == p_puback_packet) {
                return ResponseCode::NULL_VALUE_ERROR;
            }

            const util::String packet_data = p_puback_packet->ToString();
            ResponseCode rc = WriteToNetworkBuffer(p_network_connection, packet_data);
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(PUBACK_ACTION_LOG_TAG, "Puback Write to Network Failed. %s",
                              ResponseHelper::ToString(rc).c_str());
            }

            return rc;
        }
    }
}
