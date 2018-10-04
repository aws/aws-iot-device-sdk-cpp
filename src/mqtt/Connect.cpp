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
 * @file Connect.cpp
 * @brief
 *
 */

#include "util/logging/LogMacros.hpp"

#include "mqtt/ClientState.hpp"
#include "mqtt/NetworkRead.hpp"
#include "mqtt/Connect.hpp"

#define CONNECT_ACTION_DESCRIPTION "MQTT Connect Action"
#define DISCONNECT_ACTION_DESCRIPTION "MQTT Disconnect Action"
#define KEEPALIVE_ACTION_DESCRIPTION "MQTT Keep alive Action"

#define CONNECT_LOG_TAG "[Connect]"
#define DISCONNECT_LOG_TAG "[Disconnect]"
#define KEEPALIVE_LOG_TAG "[KeepAlive]"

// Must be MQTT as per MQTT spec v3.1.1
#define MQTT_CONNECT_PROTOCOL_ID "MQTT"

#define CONNACK_RESERVED_PACKET_ID 0

#define SDK_USAGE_METRICS_STRING "?SDK=CPP&Version="

namespace awsiotsdk {
    namespace mqtt {
        /********************************************
         * ConnectPacket class function definitions *
         *******************************************/
        ConnectPacket::ConnectPacket(bool is_clean_session,
                                     mqtt::Version mqtt_version,
                                     std::chrono::seconds keep_alive_timeout,
                                     std::unique_ptr<Utf8String> p_client_id,
                                     std::unique_ptr<Utf8String> p_username,
                                     std::unique_ptr<Utf8String> p_password,
                                     std::unique_ptr<mqtt::WillOptions> p_will_msg,
                                     bool is_metrics_enabled) {
            connect_flags_ = 0;
            packet_size_ = 10; // Len = 10 for MQTT_3_1_1

            p_protocol_id_ = Utf8String::Create(MQTT_CONNECT_PROTOCOL_ID, 4);
            is_clean_session_ = is_clean_session;
            if (is_clean_session_) {
                connect_flags_ |= 0x02; // Cleansession is Left-to-Right bit 7 in the flag byte
            }

            mqtt_version_ = mqtt_version;
            keep_alive_timeout_ = keep_alive_timeout;

            p_client_id_ = nullptr;
            if (nullptr != p_client_id) {
                packet_size_ = packet_size_ + p_client_id->Length();
                p_client_id_ = std::move(p_client_id);
            } else {
                if (false == is_clean_session_) {
                    AWS_LOG_INFO(CONNECT_LOG_TAG,
                                 "Clean session value must be true when no client ID is provided. Forcing it to true");
                    is_clean_session_ = true;
                }
            }

            packet_size_ = packet_size_ + 2;  // +2 for writing length

            // username used for sending usage metrics
            if (is_metrics_enabled) {
                util::String username_string = "";
                // username is not supported by the service
                /*if (nullptr != p_username) {
                    username_string.append(p_username->ToStdString());
                }*/
                username_string.append(SDK_USAGE_METRICS_STRING);
                username_string.append(SDK_VERSION_STRING);

                p_username_ = Utf8String::Create(username_string);
                packet_size_ = packet_size_ + p_username_->Length() + 2;
                connect_flags_ |= 0x80;
            }

            // password is not supported by the service
            /*p_password_ = nullptr;
            if(nullptr != p_password) {
                packet_size_ = packet_size_ + p_password->Length() + 2; // +2 for writing length
                p_password_ = std::move(p_username);
                connect_flags_ |= 0x40; // Username is Left-to-Right bit 2 in the flag byte
            }*/


            p_will_msg_ = nullptr;
            if (nullptr != p_will_msg) {
                p_will_msg_ = std::move(p_will_msg);
                packet_size_ =
                    packet_size_ + p_will_msg_->Length() + 4; // +2 each for writing length of topic name and payload
                p_will_msg_->SetConnectFlags(connect_flags_);
            }

            fixed_header_.Initialize(MessageTypes::CONNECT, false, QoS::QOS0, false, packet_size_);

            serialized_packet_length_ = packet_size_ + fixed_header_.Length();
        }

        ConnectPacket::ConnectPacket(bool is_clean_session,
                                     mqtt::Version mqtt_version,
                                     std::chrono::seconds keep_alive_timeout,
                                     std::unique_ptr<Utf8String> p_client_id,
                                     std::unique_ptr<Utf8String> p_username,
                                     std::unique_ptr<Utf8String> p_password,
                                     std::unique_ptr<mqtt::WillOptions> p_will_msg) : ConnectPacket(is_clean_session,
                                                                                                    mqtt_version,
                                                                                                    keep_alive_timeout,
                                                                                                    std::move(p_client_id),
                                                                                                    std::move(p_username),
                                                                                                    std::move(p_password),
                                                                                                    std::move(p_will_msg),
                                                                                                    true) {
        }

        std::shared_ptr<ConnectPacket> ConnectPacket::Create(bool is_clean_session,
                                                             mqtt::Version mqtt_version,
                                                             std::chrono::seconds keep_alive_timeout,
                                                             std::unique_ptr<Utf8String> p_client_id,
                                                             std::unique_ptr<Utf8String> p_username,
                                                             std::unique_ptr<Utf8String> p_password,
                                                             std::unique_ptr<mqtt::WillOptions> p_will_msg,
                                                             bool is_metrics_enabled) {
            if (UINT16_MAX < keep_alive_timeout.count()) {
                return nullptr;
            }

            if (nullptr == p_client_id && false == is_clean_session) {
                AWS_LOG_ERROR(CONNECT_LOG_TAG, "Clean session value must be true when no client ID is provided");
                return nullptr;
            }

            return std::make_shared<ConnectPacket>(is_clean_session,
                                                   mqtt_version,
                                                   keep_alive_timeout,
                                                   std::move(p_client_id),
                                                   std::move(p_username),
                                                   std::move(p_password),
                                                   std::move(p_will_msg),
                                                   is_metrics_enabled);
        }

        std::shared_ptr<ConnectPacket> ConnectPacket::Create(bool is_clean_session,
                                                             mqtt::Version mqtt_version,
                                                             std::chrono::seconds keep_alive_timeout,
                                                             std::unique_ptr<Utf8String> p_client_id,
                                                             std::unique_ptr<Utf8String> p_username,
                                                             std::unique_ptr<Utf8String> p_password,
                                                             std::unique_ptr<mqtt::WillOptions> p_will_msg) {
            return Create(is_clean_session,
                          mqtt_version,
                          keep_alive_timeout,
                          std::move(p_client_id),
                          std::move(p_username),
                          std::move(p_password),
                          std::move(p_will_msg),
                          true);
        }

        util::String ConnectPacket::ToString() {
            util::String buf;
            buf.reserve(serialized_packet_length_);

            fixed_header_.AppendToBuffer(buf);

            AppendUtf8StringToBuffer(buf, p_protocol_id_);
            char temp = static_cast<char>(mqtt_version_);
            buf.append(&temp, 1);
            temp = static_cast<char>(connect_flags_);
            buf.append(&temp, 1);

            // Ensure the value provided for keep alive is not too large to fit
            // This can happen if the constructor was used directly instead of the Create Factory method
            // Also find out why someone would want to use such a large value
            if (UINT16_MAX < keep_alive_timeout_.count()) {
                AppendUInt16ToBuffer(buf, static_cast<uint16_t>(UINT16_MAX));
            } else {
                AppendUInt16ToBuffer(buf, static_cast<uint16_t>(keep_alive_timeout_.count()));
            }

            if (nullptr == p_client_id_) {
                // No client id provided, server should assign one
                AppendUInt16ToBuffer(buf, static_cast<uint16_t>(0));
            } else {
                AppendUtf8StringToBuffer(buf, p_client_id_);
            }

            if (nullptr != p_will_msg_) {
                p_will_msg_->WriteToBuffer(buf);
            }

            if (nullptr != p_username_) {
                AppendUtf8StringToBuffer(buf, p_username_);
            }

            /*if(nullptr != p_password_) {
                AppendUtf8StringToBuffer(buf, p_password_);;
            }*/

            return buf;
        }

        /***********************************************
         * DisconnectPacket class function definitions *
         **********************************************/
        DisconnectPacket::DisconnectPacket() {
            packet_size_ = 0; // Len = 0 for MQTT_3_1_1
            fixed_header_.Initialize(MessageTypes::DISCONNECT, false, QoS::QOS0, false, packet_size_);
            serialized_packet_length_ = fixed_header_.Length();
        }

        std::shared_ptr<DisconnectPacket> DisconnectPacket::Create() {
            return std::make_shared<DisconnectPacket>();
        }

        util::String DisconnectPacket::ToString() {
            util::String buf;
            buf.reserve(serialized_packet_length_);

            fixed_header_.AppendToBuffer(buf);

            return buf;
        }

        /********************************************
         * PingreqPacket class function definitions *
         *******************************************/
        PingreqPacket::PingreqPacket() {
            packet_size_ = 0; // Len = 0 for MQTT_3_1_1
            fixed_header_.Initialize(MessageTypes::PINGREQ, false, QoS::QOS0, false, packet_size_);
            serialized_packet_length_ = fixed_header_.Length();
        }

        std::shared_ptr<PingreqPacket> PingreqPacket::Create() {
            return std::make_shared<PingreqPacket>();
        }

        util::String PingreqPacket::ToString() {
            util::String buf;
            buf.reserve(serialized_packet_length_);

            fixed_header_.AppendToBuffer(buf);

            return buf;
        }

        /***************************************************
         * ConnectActionAsync class function definitions *
         **************************************************/
        ConnectActionAsync::ConnectActionAsync(std::shared_ptr<ClientState> p_client_state)
            : Action(ActionType::CONNECT, CONNECT_ACTION_DESCRIPTION) {
            p_client_state_ = p_client_state;
        }

        std::unique_ptr<Action> ConnectActionAsync::Create(std::shared_ptr<ActionState> p_action_state) {
            std::shared_ptr<mqtt::ClientState>
                p_client_state = std::dynamic_pointer_cast<mqtt::ClientState>(p_action_state);
            if (nullptr == p_client_state) {
                return nullptr;
            }

            return std::unique_ptr<ConnectActionAsync>(new ConnectActionAsync(p_client_state));
        }

        ResponseCode ConnectActionAsync::PerformAction(std::shared_ptr<NetworkConnection> p_network_connection,
                                                       std::shared_ptr<ActionData> p_action_data) {
            ResponseCode rc = ResponseCode::SUCCESS;
            bool is_ack_registered = false;

            std::shared_ptr<ConnectPacket> p_connect_packet = std::dynamic_pointer_cast<ConnectPacket>(p_action_data);
            if (nullptr == p_connect_packet) {
                // Check if Client state has any reconnect data available
                p_connect_packet = std::dynamic_pointer_cast<ConnectPacket>(p_client_state_->GetAutoReconnectData());
                if (nullptr == p_connect_packet) {
                    return ResponseCode::NULL_VALUE_ERROR;
                }
            } else {
                p_client_state_->SetAutoReconnectData(p_connect_packet);
            }

            p_connect_packet->SetPacketId(CONNACK_RESERVED_PACKET_ID);
            if (nullptr != p_connect_packet->p_async_ack_handler_) {
                rc = p_client_state_->RegisterPendingAck(CONNACK_RESERVED_PACKET_ID,
                                                         p_connect_packet->p_async_ack_handler_);
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(CONNECT_LOG_TAG,
                                  "Registering Ack Handler for Connect Action. %s",
                                  ResponseHelper::ToString(rc).c_str());
                } else {
                    is_ack_registered = true;
                }
            }

            p_client_state_->SetKeepAliveTimeout(p_connect_packet->GetKeepAliveTimeout());
            rc = p_network_connection->Connect();
            if (ResponseCode::SUCCESS != rc) {
                return rc;
            }

            const util::String packet_data = p_connect_packet->ToString();
            rc = WriteToNetworkBuffer(p_network_connection, packet_data);
            if (ResponseCode::SUCCESS != rc) {
                if (is_ack_registered) {
                    p_client_state_->DeletePendingAck(CONNACK_RESERVED_PACKET_ID);
                }
                AWS_LOG_ERROR(CONNECT_LOG_TAG, "Connect Write to Network Failed. %s",
                              ResponseHelper::ToString(rc).c_str());
                p_network_connection->Disconnect();
            } else {
                p_client_state_->setDisconnectCallbackPending(true);
            }

            return rc;
        }

        /****************************************************
         * DisconnectActionAsync class function definitions *
         ***************************************************/
        DisconnectActionAsync::DisconnectActionAsync(std::shared_ptr<ClientState> p_client_state)
            : Action(ActionType::DISCONNECT, DISCONNECT_ACTION_DESCRIPTION) {
            p_client_state_ = p_client_state;
        }

        std::unique_ptr<Action> DisconnectActionAsync::Create(std::shared_ptr<ActionState> p_action_state) {
            std::shared_ptr<mqtt::ClientState>
                p_client_state = std::dynamic_pointer_cast<mqtt::ClientState>(p_action_state);
            if (nullptr == p_client_state) {
                return nullptr;
            }

            return std::unique_ptr<DisconnectActionAsync>(new DisconnectActionAsync(p_client_state));
        }

        ResponseCode DisconnectActionAsync::PerformAction(std::shared_ptr<NetworkConnection> p_network_connection,
                                                          std::shared_ptr<ActionData> p_action_data) {
            if (!p_client_state_->IsConnected()) {
                return ResponseCode::NETWORK_DISCONNECTED_ERROR;
            }

            //Ignore error codes, always assume disconnect
            p_client_state_->SetConnected(false);

            ResponseCode rc = ResponseCode::SUCCESS;

            // Attempt to send MQTT Disconnect if Network is still connected
            if (p_network_connection->IsConnected()) {
                std::shared_ptr<DisconnectPacket> p_disconnect_packet = std::dynamic_pointer_cast<DisconnectPacket>(
                    p_action_data);
                if (nullptr != p_disconnect_packet) {
                    rc = WriteToNetworkBuffer(p_network_connection, p_disconnect_packet->ToString());
                    if (ResponseCode::SUCCESS != rc) {
                        AWS_LOG_WARN(DISCONNECT_LOG_TAG, "Received Response Code. %s",
                                     ResponseHelper::ToString(rc).c_str());
                    }
                } else {
                    AWS_LOG_WARN(DISCONNECT_LOG_TAG, "Error creating MQTT Disconnect packet!!");
                }
            }

            /* convert all subscriptions to inactive */
            for (auto itr = p_client_state_->subscription_map_.begin(); itr != p_client_state_->subscription_map_.end();
                 ++itr) {
                itr->second->SetActive(false);
            }

            rc = p_network_connection->Disconnect();
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_WARN(DISCONNECT_LOG_TAG, "Network disconnect. %s", ResponseHelper::ToString(rc).c_str());
            }
            return ResponseCode::SUCCESS;
        }

        /****************************************************
         * KeepaliveActionRunner class function definitions *
         ***************************************************/
        KeepaliveActionRunner::KeepaliveActionRunner(std::shared_ptr<ClientState> p_client_state)
            : Action(ActionType::KEEP_ALIVE, KEEPALIVE_ACTION_DESCRIPTION) {
            p_client_state_ = p_client_state;
        }

        std::unique_ptr<Action> KeepaliveActionRunner::Create(std::shared_ptr<ActionState> p_action_state) {
            std::shared_ptr<mqtt::ClientState>
                p_client_state = std::dynamic_pointer_cast<mqtt::ClientState>(p_action_state);
            if (nullptr == p_client_state) {
                return nullptr;
            }

            return std::unique_ptr<KeepaliveActionRunner>(new KeepaliveActionRunner(p_client_state));
        }

        ResponseCode KeepaliveActionRunner::PerformAction(std::shared_ptr<NetworkConnection> p_network_connection,
                                                          std::shared_ptr<ActionData> p_action_data) {
            // TODO : This action needs cleanup in the future
            std::atomic_bool &_p_thread_continue_ = *p_thread_continue_;
            std::chrono::milliseconds thread_sleep_duration(DEFAULT_CORE_THREAD_SLEEP_DURATION_MS);

            // Wait for first connect, keep alive data will not be available until then
            while (_p_thread_continue_ && !p_client_state_->IsConnected()) {
                std::this_thread::sleep_for(thread_sleep_duration);
            }

            std::shared_ptr<PingreqPacket> p_pingreq_packet = PingreqPacket::Create();
            if (nullptr == p_pingreq_packet) {
                return ResponseCode::NULL_VALUE_ERROR;
            }

            ResponseCode rc = ResponseCode::SUCCESS;
            p_client_state_->setDisconnectCallbackPending(true);

            std::chrono::seconds reconnect_backoff_timer = p_client_state_->GetMinReconnectBackoffTimeout();
            std::chrono::seconds max_backoff_value = p_client_state_->GetMaxReconnectBackoffTimeout();
            std::chrono::seconds keep_alive_interval = p_client_state_->GetKeepAliveTimeout() / 2;
            auto next = std::chrono::system_clock::now() + std::chrono::seconds(keep_alive_interval);

            do {
                if (p_client_state_->IsAutoReconnectEnabled() && p_client_state_->IsAutoReconnectRequired()) {
                    p_client_state_->SetPingreqPending(false);
                    if (p_client_state_->isDisconnectCallbackPending()) {

                        std::shared_ptr<ConnectPacket> p_connect_packet =
                            std::dynamic_pointer_cast<ConnectPacket>(p_client_state_->GetAutoReconnectData());

                        /**
                         * NOTE: All callbacks used by the keepalive should be non-blocking
                         */
                        if (nullptr != p_client_state_->disconnect_handler_ptr_ && nullptr != p_connect_packet) {
                            p_client_state_->disconnect_handler_ptr_(p_connect_packet->GetClientID(),
                                                                   p_client_state_->p_disconnect_app_handler_data_);
                        }

                        reconnect_backoff_timer = p_client_state_->GetMinReconnectBackoffTimeout();
                        max_backoff_value = p_client_state_->GetMaxReconnectBackoffTimeout();
                        AWS_LOG_INFO(KEEPALIVE_LOG_TAG,
                                     "Initial value of reconnect timer : %ld!!",
                                     reconnect_backoff_timer.count());
                        AWS_LOG_INFO(KEEPALIVE_LOG_TAG, "Max backoff value : %ld!!", max_backoff_value.count());
                    }
                    AWS_LOG_INFO(KEEPALIVE_LOG_TAG, "Attempting Reconnect");

                    std::shared_ptr<ConnectPacket> p_connect_packet =
                        std::dynamic_pointer_cast<ConnectPacket>(p_client_state_->GetAutoReconnectData());

                    rc = p_client_state_->PerformAction(ActionType::CONNECT,
                                                        p_connect_packet,
                                                        p_client_state_->GetMqttCommandTimeout());

                    if (nullptr != p_client_state_->reconnect_handler_ptr_) {
                        p_client_state_->reconnect_handler_ptr_(p_connect_packet->GetClientID(),
                                                              p_client_state_->p_reconnect_app_handler_data_,
                                                              rc);
                    }
                    if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED == rc) {
                        p_client_state_->SetAutoReconnectRequired(false);
                        // if no subscriptions, skip resubscribe
                        if (!p_client_state_->subscription_map_.empty()) {

                            util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;

                            util::Map<util::String, std::shared_ptr<Subscription>>::const_iterator
                                itr = p_client_state_->subscription_map_.begin();
                            while (itr != p_client_state_->subscription_map_.end()) {
                                topic_vector.push_back(itr->second);
                                itr++;
                                if (topic_vector.size() == MAX_TOPICS_IN_ONE_SUBSCRIBE_PACKET) {
                                    std::shared_ptr<mqtt::SubscribePacket>
                                        p_subscribe_packet = mqtt::SubscribePacket::Create(topic_vector);
                                    p_subscribe_packet->SetPacketId(p_client_state_->GetNextPacketId());

                                    rc = p_client_state_->PerformAction(ActionType::SUBSCRIBE,
                                                                        p_subscribe_packet,
                                                                        p_client_state_->GetMqttCommandTimeout());
                                    if (ResponseCode::SUCCESS != rc) {
                                        AWS_LOG_ERROR(KEEPALIVE_LOG_TAG,
                                                      "Resubscribe attempt returned unhandled error. \n%s",
                                                      ResponseHelper::ToString(rc).c_str());
                                        break;
                                    }
                                    topic_vector.clear();
                                }
                            }

                            if (ResponseCode::SUCCESS == rc || ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED == rc) {
                                if (!topic_vector.empty()) {
                                    std::shared_ptr<mqtt::SubscribePacket>
                                        p_subscribe_packet = mqtt::SubscribePacket::Create(topic_vector);
                                    p_subscribe_packet->SetPacketId(p_client_state_->GetNextPacketId());

                                    rc = p_client_state_->PerformAction(ActionType::SUBSCRIBE,
                                                                        p_subscribe_packet,
                                                                        p_client_state_->GetMqttCommandTimeout());
                                }
                            }

                            if (nullptr != p_client_state_->resubscribe_handler_ptr_) {
                                p_client_state_->resubscribe_handler_ptr_(p_connect_packet->GetClientID(),
                                                                        p_client_state_->p_resubscribe_app_handler_data_,
                                                                        rc);
                            }
                        }
                        /**
                         * NOTE :The resubscribe response can be NETWORK_DISCONNECTED_ERROR as the network might have
                         * disconnected again after the reconnect was successful.
                         */
                        if (ResponseCode::NETWORK_DISCONNECTED_ERROR != rc) {
                            p_client_state_->SetAutoReconnectRequired(false);
                        } else {
                            p_client_state_->PerformAction(ActionType::DISCONNECT,
                                                           DisconnectPacket::Create(),
                                                           p_client_state_->GetMqttCommandTimeout());
                            p_client_state_->SetAutoReconnectRequired(true);
                        }
                        continue;
                    }

                    p_client_state_->setDisconnectCallbackPending(false);
                    AWS_LOG_ERROR(KEEPALIVE_LOG_TAG, "Reconnect failed. %s", ResponseHelper::ToString(rc).c_str());

                    AWS_LOG_INFO(KEEPALIVE_LOG_TAG,
                                 "Current value of reconnect timer : %ld!!",
                                 reconnect_backoff_timer.count());
                    if (max_backoff_value > reconnect_backoff_timer) {
                        reconnect_backoff_timer += reconnect_backoff_timer;
                    }

                    AWS_LOG_INFO(KEEPALIVE_LOG_TAG,
                                 "Updated value of reconnect timer : %ld!!",
                                 reconnect_backoff_timer.count());
                    std::this_thread::sleep_for(reconnect_backoff_timer);
                    continue;
                } else if (p_client_state_->IsAutoReconnectRequired()) {
                    if (p_client_state_->isDisconnectCallbackPending()) {
                        std::shared_ptr<ConnectPacket> p_connect_packet =
                            std::dynamic_pointer_cast<ConnectPacket>(p_client_state_->GetAutoReconnectData());

                        if (nullptr != p_client_state_->disconnect_handler_ptr_ && nullptr != p_connect_packet) {
                            p_client_state_->disconnect_handler_ptr_(p_connect_packet->GetClientID(),
                                                                   p_client_state_->p_disconnect_app_handler_data_);
                        }

                        p_client_state_->setDisconnectCallbackPending(false);
                    }
                }

                if (std::chrono::system_clock::now() > next) {
                    if (p_client_state_->IsPingreqPending()) {
                        if (p_client_state_->IsConnected()) {
                            rc = p_client_state_->PerformAction(ActionType::DISCONNECT,
                                                                DisconnectPacket::Create(),
                                                                p_client_state_->GetMqttCommandTimeout());
                            if (ResponseCode::SUCCESS != rc && ResponseCode::NETWORK_DISCONNECTED_ERROR != rc) {
                                AWS_LOG_ERROR(KEEPALIVE_LOG_TAG,
                                              "Network Disconnect attempt returned unhandled error. \n%s",
                                              ResponseHelper::ToString(rc).c_str());
                            }
                        }
                        p_client_state_->SetAutoReconnectRequired(true);
                        continue;
                    } else if (p_client_state_->IsConnected()) {
                        rc = WriteToNetworkBuffer(p_network_connection, p_pingreq_packet->ToString());

                        if (ResponseCode::SUCCESS != rc) {
                            AWS_LOG_ERROR(KEEPALIVE_LOG_TAG,
                                          "Writing PingReq to Network Failed. \n%s. \nDisconnecting!",
                                          ResponseHelper::ToString(rc).c_str());
                            rc = p_client_state_->PerformAction(ActionType::DISCONNECT,
                                                                DisconnectPacket::Create(),
                                                                p_client_state_->GetMqttCommandTimeout());
                            if (ResponseCode::SUCCESS != rc) {
                                AWS_LOG_ERROR(KEEPALIVE_LOG_TAG,
                                              "Network Disconnect attempt returned unhandled error. \n%s",
                                              ResponseHelper::ToString(rc).c_str());
                            }
                            p_client_state_->SetAutoReconnectRequired(true);
                            continue;
                        }

                        p_client_state_->SetPingreqPending(true);
                        next = std::chrono::system_clock::now() + std::chrono::seconds(keep_alive_interval);
                    }
                }
                std::this_thread::sleep_for(thread_sleep_duration);
            } while (_p_thread_continue_);

            return rc;
        }
    }
}
 
