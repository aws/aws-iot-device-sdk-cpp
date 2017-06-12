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
 * @file NetworkRead.cpp
 * @brief
 *
 */

#include <iostream>
#include <chrono>
#include <thread>

#include "util/logging/LogMacros.hpp"

#include "mqtt/ClientState.hpp"
#include "mqtt/NetworkRead.hpp"

#define MAX_NO_OF_REMAINING_LENGTH_BYTES 4

#define NETWORK_READ_LOG_TAG "[Network Read]"

#define CONNACK_RESERVED_PACKET_ID 0

namespace awsiotsdk {
    namespace mqtt {

        NetworkReadActionRunner::NetworkReadActionRunner(std::shared_ptr<ClientState> p_client_state)
            : Action(ActionType::READ_INCOMING, "TLS Read Action Runner") {
            p_client_state_ = p_client_state;
            is_waiting_for_connack_ = true;
        }

        std::unique_ptr<Action> NetworkReadActionRunner::Create(std::shared_ptr<ActionState> p_action_state) {
            std::shared_ptr<mqtt::ClientState>
                p_client_state = std::dynamic_pointer_cast<mqtt::ClientState>(p_action_state);
            if (nullptr == p_client_state) {
                return nullptr;
            }

            return std::unique_ptr<NetworkReadActionRunner>(new NetworkReadActionRunner(p_client_state));
        }

        ResponseCode NetworkReadActionRunner::DecodeRemainingLength(size_t &rem_len) {
            util::String encoded_byte_str;
            encoded_byte_str.reserve(1);
            size_t multiplier = 1;
            size_t len = 0;
            ResponseCode rc;
            rem_len = 0;
            util::Vector<unsigned char> temp_buf;

            do {
                temp_buf.clear();
                if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES) {
                    /* bad data */
                    rc = ResponseCode::FAILURE;
                    break;
                }

                rc = ReadFromNetworkBuffer(p_network_connection_, temp_buf, 1);
                if (ResponseCode::SUCCESS != rc) {
                    break;
                }
                rem_len += (size_t) ((temp_buf[0] & 127) * multiplier);
                multiplier *= 128;
            } while (0 != (temp_buf[0] & 128));

            return rc;
        }

        ResponseCode NetworkReadActionRunner::ReadPacketFromNetwork(unsigned char &fixed_header_byte,
                                                                    util::Vector<unsigned char> &read_buf) {
            read_buf.clear();
            ResponseCode rc = ReadFromNetworkBuffer(p_network_connection_, read_buf, 1);
            if (ResponseCode::SUCCESS != rc) {
                return rc;
            }

            fixed_header_byte = read_buf[0];
            read_buf.clear();
            size_t rem_len = 0;
            rc = DecodeRemainingLength(rem_len);
            if (ResponseCode::SUCCESS != rc) {
                return rc;
            }

            if (0 < rem_len) {
                rc = ReadFromNetworkBuffer(p_network_connection_, read_buf, rem_len);
            }
            return rc;
        }

        ResponseCode NetworkReadActionRunner::PerformAction(std::shared_ptr<NetworkConnection> p_network_connection,
                                                            std::shared_ptr<ActionData> p_action_data) {
            if (nullptr == p_network_connection) {
                return ResponseCode::NULL_VALUE_ERROR;
            }

            bool is_duplicate;
            bool is_retained;
            QoS qos;
            unsigned char fixed_header_byte;
            unsigned char message_type_byte;
            util::Vector<unsigned char> read_buf;
            ResponseCode rc = ResponseCode::SUCCESS;
            p_network_connection_ = p_network_connection;
            std::atomic_bool &_p_thread_continue_ = *p_thread_continue_;
            std::chrono::milliseconds thread_sleep_duration(DEFAULT_CORE_THREAD_SLEEP_DURATION_MS);

            is_waiting_for_connack_ = !(p_client_state_->IsConnected());

            do {
                AWS_LOG_TRACE(NETWORK_READ_LOG_TAG,
                              " Network Read Thread, TLS Status : %d",
                              p_network_connection->IsConnected());
                // Clear buffers
                fixed_header_byte = 0x00;
                read_buf.clear();
                rc = ReadPacketFromNetwork(fixed_header_byte, read_buf);
                if (ResponseCode::NETWORK_SSL_NOTHING_TO_READ == rc) {
                    std::this_thread::sleep_for(thread_sleep_duration);
                    continue;
                } else if (ResponseCode::SUCCESS == rc) {
                    message_type_byte = fixed_header_byte;
                    message_type_byte >>= 4; // Packet type is in first 4 bits
                    message_type_byte &= 0x0F; // Only keep the least significant 4 bits
                    MessageTypes messageType = (MessageTypes) message_type_byte;
                    switch (messageType) {
                        case MessageTypes::CONNACK:
                            rc = HandleConnack(read_buf);
                            break;
                        case MessageTypes::PUBLISH: {
                            is_retained = ((fixed_header_byte & 0x01) == 0x01);
                            is_duplicate = ((fixed_header_byte & 0x08) == 0x08);
                            qos = ((fixed_header_byte & 0x02) == 0x02) ? QoS::QOS1 : QoS::QOS0;
                            rc = HandlePublish(read_buf, is_duplicate, is_retained, qos);
                        }
                            break;
                        case MessageTypes::PUBACK:
                            rc = HandlePuback(read_buf);
                            break;
                        case MessageTypes::SUBACK:
                            rc = HandleSuback(read_buf);
                            break;
                        case MessageTypes::UNSUBACK:
                            rc = HandleUnsuback(read_buf);
                            break;
                        case MessageTypes::PINGRESP:
                            p_client_state_->SetPingreqPending(false);
                            rc = ResponseCode::SUCCESS;
                            break;
                        default:
                            // Any type values other than above are either unsupported or invalid
                            // Packet types used for QoS2 are currently unsupported
                            break;
                    }
                } else if (!is_waiting_for_connack_) {
                    is_waiting_for_connack_ = true;
                    if (_p_thread_continue_ && p_client_state_->IsConnected()) {
                        AWS_LOG_ERROR(NETWORK_READ_LOG_TAG,
                                      "Network Read attempt returned unhandled error. %s Requesting  Network Reconnect.",
                                      ResponseHelper::ToString(rc).c_str());
                        rc = p_client_state_->PerformAction(ActionType::DISCONNECT,
                                                            DisconnectPacket::Create(),
                                                            p_client_state_->GetMqttCommandTimeout());
                        if (ResponseCode::SUCCESS != rc) {
                            AWS_LOG_ERROR(NETWORK_READ_LOG_TAG,
                                          "Network Disconnect attempt returned unhandled error. %s",
                                          ResponseHelper::ToString(rc).c_str());
                            // No further action being taken. Assumption is that reconnect logic should bring SDK back to working state
                        }
                        p_client_state_->SetAutoReconnectRequired(true);
                    }
                }
            } while (_p_thread_continue_);
            return rc;
        }

        ResponseCode NetworkReadActionRunner::HandleConnack(const util::Vector<unsigned char> &read_buf) {
            ResponseCode rc = ResponseCode::SUCCESS;
            if (2 != read_buf.size()) {
                // CONNACK remaining length is always 2
                rc = ResponseCode::MQTT_DECODE_REMAINING_LENGTH_ERROR;
            } else {
                p_client_state_->SetSessionPresent(1 == static_cast<uint8_t>(read_buf.at(0)));
                uint8_t connack_rc_byte = static_cast<uint8_t>(read_buf.at(1));
                if (connack_rc_byte > static_cast<uint8_t>(ConnackReturnCode::NOT_AUTHORIZED_ERROR)) {
                    return ResponseCode::MQTT_UNEXPECTED_PACKET_FORMAT_ERROR;
                }
                ConnackReturnCode connack_rc = static_cast<ConnackReturnCode>(connack_rc_byte);
                switch (connack_rc) {
                    case ConnackReturnCode::CONNECTION_ACCEPTED:
                        p_client_state_->SetConnected(true);
                        p_client_state_->ForwardReceivedAck(CONNACK_RESERVED_PACKET_ID,
                                                            ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED);
                        break;
                    case ConnackReturnCode::UNACCEPTABLE_PROTOCOL_VERSION_ERROR:
                        rc = ResponseCode::MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR;
                        break;
                    case ConnackReturnCode::IDENTIFIER_REJECTED_ERROR:
                        rc = ResponseCode::MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR;
                        break;
                    case ConnackReturnCode::SERVER_UNAVAILABLE_ERROR:
                        rc = ResponseCode::MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR;
                        break;
                    case ConnackReturnCode::BAD_USERDATA_ERROR:
                        rc = ResponseCode::MQTT_CONNACK_BAD_USERDATA_ERROR;
                        break;
                    case ConnackReturnCode::NOT_AUTHORIZED_ERROR:
                        rc = ResponseCode::MQTT_CONNACK_NOT_AUTHORIZED_ERROR;
                        break;
                }
            }

            AWS_LOG_INFO(NETWORK_READ_LOG_TAG, "Network Connect Response. %s", ResponseHelper::ToString(rc).c_str());
            return rc;
        }

        ResponseCode NetworkReadActionRunner::HandlePublish(const util::Vector<unsigned char> &read_buf,
                                                            bool is_retained,
                                                            bool is_duplicate,
                                                            QoS qos) {
            ResponseCode rc = ResponseCode::FAILURE;
            std::shared_ptr<mqtt::PublishPacket>
                p_publish_packet = PublishPacket::Create(read_buf, is_retained, is_duplicate, qos);

            util::String topic_name = p_publish_packet->GetTopicName();
            std::shared_ptr<Subscription> p_sub = p_client_state_->GetSubscription(topic_name);

            if (nullptr != p_sub) {
                if (p_sub->IsActive()) {
                    p_sub->p_app_handler_(topic_name, p_publish_packet->GetPayload(), p_sub->p_app_handler_data_);
                    rc = ResponseCode::SUCCESS;
                } else {
                    rc = ResponseCode::MQTT_SUBSCRIPTION_NOT_ACTIVE;
                }
            } else {
                rc = ResponseCode::MQTT_NO_SUBSCRIPTION_FOUND;
            }

            if (ResponseCode::SUCCESS == rc && QoS::QOS0 != qos) {
                std::shared_ptr<mqtt::PubackPacket>
                    p_puback_packet = PubackPacket::Create(p_publish_packet->GetPacketId());
                uint16_t action_id = 0;
                /* TODO: nullchecks */
                //Ignore action_id, we don't support QoS2 at the moment
                rc = p_client_state_->EnqueueOutboundAction(ActionType::PUBACK, p_puback_packet, action_id);
            }

            return rc;
        }

        ResponseCode NetworkReadActionRunner::HandlePuback(const util::Vector<unsigned char> &read_buf) {
            ResponseCode rc = ResponseCode::SUCCESS;
            size_t extract_index = 0;

            uint16_t packet_id = Packet::ReadUInt16FromBuffer(read_buf, extract_index);
            p_client_state_->ForwardReceivedAck(packet_id, rc);

            return rc;
        }

        ResponseCode NetworkReadActionRunner::HandleSuback(const util::Vector<unsigned char> &read_buf) {
            ResponseCode rc = ResponseCode::SUCCESS;
            uint8_t itr = 0;
            bool has_atleast_one_success = false;
            bool has_atleast_one_failure = false;

            std::shared_ptr<mqtt::SubackPacket> p_suback_packet = SubackPacket::Create(read_buf);
            uint16_t packet_id = p_suback_packet->GetPacketId();
            for (uint8_t qos : p_suback_packet->suback_list_) {
                if (128 == qos) { // MQTT spec specifies 128 is returned when subscribe fails
                    has_atleast_one_failure = true;
                    rc = p_client_state_->RemoveSubscription(packet_id, static_cast<uint8_t>(itr + 1));
                } else if (0 == qos) {
                    has_atleast_one_success = true;
                    rc = p_client_state_->SetSubscriptionActive(packet_id,
                                                                static_cast<uint8_t>(itr + 1),
                                                                mqtt::QoS::QOS0);
                } else if (1 == qos) {
                    has_atleast_one_success = true;
                    rc = p_client_state_->SetSubscriptionActive(packet_id,
                                                                static_cast<uint8_t>(itr + 1),
                                                                mqtt::QoS::QOS1);
                } // QoS2 is not supported
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(NETWORK_READ_LOG_TAG, "Subscription update attempt returned unhandled error. %s",
                                  ResponseHelper::ToString(rc).c_str());
                    // No further action being taken
                }
                itr++;
            }

            rc = ResponseCode::MQTT_SUBSCRIBE_FAILED;
            if (has_atleast_one_success && has_atleast_one_failure) {
                rc = ResponseCode::MQTT_SUBSCRIBE_PARTIALLY_FAILED;
            } else if (has_atleast_one_success) {
                rc = ResponseCode::SUCCESS;
            }
            p_client_state_->ForwardReceivedAck(packet_id, rc);

            return rc;
        }

        ResponseCode NetworkReadActionRunner::HandleUnsuback(const util::Vector<unsigned char> &read_buf) {
            ResponseCode rc = ResponseCode::SUCCESS;

            std::shared_ptr<mqtt::UnsubackPacket> p_unsuback_packet = UnsubackPacket::Create(read_buf);
            uint16_t packet_id = p_unsuback_packet->GetPacketId();
            p_client_state_->RemoveAllSubscriptionsForPacketId(packet_id);
            p_client_state_->ForwardReceivedAck(packet_id, ResponseCode::SUCCESS);

            return rc;
        }
    }
}
