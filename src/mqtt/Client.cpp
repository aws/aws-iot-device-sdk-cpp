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
 * @file Client.cpp
 * @brief
 *
 */

#include "util/logging/LogMacros.hpp"

#include "ResponseCode.hpp"
#include "mqtt/Client.hpp"
#include "mqtt/NetworkRead.hpp"

#define MQTT_ACTION_TIMEOUT_MS 2000

#define MQTT_CLIENT_LOG_TAG "[MQTT Client]"

namespace awsiotsdk {
    std::unique_ptr<MqttClient> MqttClient::Create(std::shared_ptr<NetworkConnection> p_network_connection,
                                                   std::chrono::milliseconds mqtt_command_timeout) {
        if (nullptr == p_network_connection) {
            return nullptr;
        }

        return std::unique_ptr<MqttClient>(new MqttClient(p_network_connection,
                                                          mqtt_command_timeout,
                                                          nullptr, nullptr, nullptr,
                                                          nullptr, nullptr, nullptr));
    }

    std::unique_ptr<MqttClient> MqttClient::Create(std::shared_ptr<NetworkConnection> p_network_connection,
                                                   std::chrono::milliseconds mqtt_command_timeout,
                                                   ClientCoreState::ApplicationDisconnectCallbackPtr disconnect_callback_ptr,
                                                   std::shared_ptr<DisconnectCallbackContextData> p_disconnect_app_handler_data) {
        if (nullptr == p_network_connection) {
            return nullptr;
        }

        return std::unique_ptr<MqttClient>(new MqttClient(p_network_connection,
                                                          mqtt_command_timeout,
                                                          disconnect_callback_ptr,
                                                          p_disconnect_app_handler_data,
                                                          nullptr, nullptr, nullptr, nullptr));
    }

    std::unique_ptr<MqttClient> MqttClient::Create(std::shared_ptr<NetworkConnection> p_network_connection,
                                                   std::chrono::milliseconds mqtt_command_timeout,
                                                   ClientCoreState::ApplicationDisconnectCallbackPtr disconnect_callback_ptr,
                                                   std::shared_ptr<DisconnectCallbackContextData> p_disconnect_app_handler_data,
                                                   ClientCoreState::ApplicationReconnectCallbackPtr reconnect_callback_ptr,
                                                   std::shared_ptr<ReconnectCallbackContextData> p_reconnect_app_handler_data,
                                                   ClientCoreState::ApplicationResubscribeCallbackPtr resubscribe_callback_ptr,
                                                   std::shared_ptr<ResubscribeCallbackContextData> p_resubscribe_app_handler_data) {
        if (nullptr == p_network_connection) {
            return nullptr;
        }

        return std::unique_ptr<MqttClient>(new MqttClient(p_network_connection,
                                                          mqtt_command_timeout,
                                                          disconnect_callback_ptr,
                                                          p_disconnect_app_handler_data,
                                                          reconnect_callback_ptr,
                                                          p_reconnect_app_handler_data,
                                                          resubscribe_callback_ptr,
                                                          p_resubscribe_app_handler_data));
    }

    MqttClient::MqttClient(std::shared_ptr<NetworkConnection> p_network_connection,
                           std::chrono::milliseconds mqtt_command_timeout,
                           ClientCoreState::ApplicationDisconnectCallbackPtr disconnect_callback_ptr,
                           std::shared_ptr<DisconnectCallbackContextData> p_disconnect_app_handler_data,
                           ClientCoreState::ApplicationReconnectCallbackPtr reconnect_callback_ptr,
                           std::shared_ptr<ReconnectCallbackContextData> p_reconnect_app_handler_data,
                           ClientCoreState::ApplicationResubscribeCallbackPtr resubscribe_callback_ptr,
                           std::shared_ptr<ResubscribeCallbackContextData> p_resubscribe_app_handler_data) {
        p_client_state_ = mqtt::ClientState::Create(mqtt_command_timeout);
        p_client_state_->disconnect_handler_ptr_ = disconnect_callback_ptr;
        p_client_state_->p_disconnect_app_handler_data_ = p_disconnect_app_handler_data;
        p_client_state_->reconnect_handler_ptr_ = reconnect_callback_ptr;
        p_client_state_->p_reconnect_app_handler_data_ = p_reconnect_app_handler_data;
        p_client_state_->resubscribe_handler_ptr_ = resubscribe_callback_ptr;
        p_client_state_->p_resubscribe_app_handler_data_ = p_resubscribe_app_handler_data;

        // Construct Full MQTT Client
        p_client_core_ = std::unique_ptr<ClientCore>(ClientCore::Create(p_network_connection, p_client_state_));
        p_client_core_->RegisterAction(ActionType::CONNECT, mqtt::ConnectActionAsync::Create);
        p_client_core_->RegisterAction(ActionType::PUBLISH, mqtt::PublishActionAsync::Create);
        p_client_core_->RegisterAction(ActionType::PUBACK, mqtt::PubackActionAsync::Create);
        p_client_core_->RegisterAction(ActionType::SUBSCRIBE, mqtt::SubscribeActionAsync::Create);
        p_client_core_->RegisterAction(ActionType::UNSUBSCRIBE, mqtt::UnsubscribeActionAsync::Create);
        p_client_core_->RegisterAction(ActionType::DISCONNECT, mqtt::DisconnectActionAsync::Create);
        p_client_core_->RegisterAction(ActionType::READ_INCOMING, mqtt::NetworkReadActionRunner::Create);
        p_client_core_->RegisterAction(ActionType::KEEP_ALIVE, mqtt::KeepaliveActionRunner::Create);
    }

    MqttClient::MqttClient(std::shared_ptr<NetworkConnection> p_network_connection,
                           std::chrono::milliseconds mqtt_command_timeout,
                           ClientCoreState::ApplicationDisconnectCallbackPtr disconnect_callback_ptr,
                           std::shared_ptr<DisconnectCallbackContextData> p_disconnect_app_handler_data)
        : MqttClient(p_network_connection, mqtt_command_timeout, disconnect_callback_ptr, p_disconnect_app_handler_data,
                     nullptr, nullptr, nullptr, nullptr) {

    }

    MqttClient::MqttClient(std::shared_ptr<NetworkConnection> p_network_connection,
                           std::chrono::milliseconds mqtt_command_timeout) : MqttClient(p_network_connection,
                                                                                        mqtt_command_timeout,
                                                                                        nullptr,
                                                                                        nullptr, nullptr, nullptr,
                                                                                        nullptr, nullptr) {

    }

    ResponseCode MqttClient::Connect(std::chrono::milliseconds action_response_timeout, bool is_clean_session,
                                     mqtt::Version mqtt_version, std::chrono::seconds keep_alive_timeout,
                                     std::unique_ptr<Utf8String> p_client_id, std::unique_ptr<Utf8String> p_username,
                                     std::unique_ptr<Utf8String> p_password,
                                     std::unique_ptr<mqtt::WillOptions> p_will_msg) {
        p_client_core_->CreateActionRunner(ActionType::READ_INCOMING, nullptr);
        p_client_core_->CreateActionRunner(ActionType::KEEP_ALIVE, nullptr);

        std::shared_ptr<mqtt::ConnectPacket> p_connect_packet
            = std::make_shared<mqtt::ConnectPacket>(is_clean_session, mqtt_version, keep_alive_timeout,
                                                    std::move(p_client_id), std::move(p_username),
                                                    std::move(p_password), std::move(p_will_msg));
        return p_client_core_->PerformAction(ActionType::CONNECT, p_connect_packet, action_response_timeout);
    }

    ResponseCode MqttClient::Connect(std::chrono::milliseconds action_response_timeout, bool is_clean_session,
                                     mqtt::Version mqtt_version, std::chrono::seconds keep_alive_timeout,
                                     std::unique_ptr<Utf8String> p_client_id, std::unique_ptr<Utf8String> p_username,
                                     std::unique_ptr<Utf8String> p_password,
                                     std::unique_ptr<mqtt::WillOptions> p_will_msg,
                                     bool is_metrics_enabled) {
        p_client_core_->CreateActionRunner(ActionType::READ_INCOMING, nullptr);
        p_client_core_->CreateActionRunner(ActionType::KEEP_ALIVE, nullptr);

        std::shared_ptr<mqtt::ConnectPacket> p_connect_packet
            = std::make_shared<mqtt::ConnectPacket>(is_clean_session, mqtt_version, keep_alive_timeout,
                                                    std::move(p_client_id), std::move(p_username),
                                                    std::move(p_password), std::move(p_will_msg), is_metrics_enabled);
        return p_client_core_->PerformAction(ActionType::CONNECT, p_connect_packet, action_response_timeout);
    }

    ResponseCode MqttClient::Disconnect(std::chrono::milliseconds action_response_timeout) {
        std::shared_ptr<mqtt::DisconnectPacket> p_disconnect_packet = std::make_shared<mqtt::DisconnectPacket>();
        return p_client_core_->PerformAction(ActionType::DISCONNECT, p_disconnect_packet, action_response_timeout);
    }

    ResponseCode MqttClient::Publish(std::unique_ptr<Utf8String> p_topic_name, bool is_retained, bool is_duplicate,
                                     mqtt::QoS qos, const util::String &payload,
                                     std::chrono::milliseconds action_response_timeout) {
        if (nullptr == p_topic_name) {
            return ResponseCode::MQTT_INVALID_DATA_ERROR;
        }
        std::shared_ptr<mqtt::PublishPacket> p_publish_packet
            = std::make_shared<mqtt::PublishPacket>(std::move(p_topic_name), is_retained, is_duplicate, qos,
                                                    payload);
        return p_client_core_->PerformAction(ActionType::PUBLISH, p_publish_packet, action_response_timeout);
    }

    ResponseCode MqttClient::Subscribe(util::Vector<std::shared_ptr<mqtt::Subscription>> subscription_list,
                                       std::chrono::milliseconds action_response_timeout) {
        if (subscription_list.empty()) {
            return ResponseCode::MQTT_INVALID_DATA_ERROR;
        } else if (MAX_TOPICS_IN_ONE_SUBSCRIBE_PACKET < subscription_list.size()) {
            return ResponseCode::MQTT_TOO_MANY_SUBSCRIPTIONS_IN_REQUEST;
        }

        std::shared_ptr<mqtt::SubscribePacket>
            p_subscribe_packet = std::make_shared<mqtt::SubscribePacket>(subscription_list);
        return p_client_core_->PerformAction(ActionType::SUBSCRIBE, p_subscribe_packet, action_response_timeout);
    }

    ResponseCode MqttClient::Unsubscribe(util::Vector<std::unique_ptr<Utf8String>> topic_list,
                                         std::chrono::milliseconds action_response_timeout) {
        if (topic_list.empty()) {
            return ResponseCode::MQTT_INVALID_DATA_ERROR;
        } else if (MAX_TOPICS_IN_ONE_SUBSCRIBE_PACKET < topic_list.size()) {
            return ResponseCode::MQTT_TOO_MANY_SUBSCRIPTIONS_IN_REQUEST;
        }

        std::shared_ptr<mqtt::UnsubscribePacket>
            p_unsubscribe_packet = std::make_shared<mqtt::UnsubscribePacket>(std::move(topic_list));
        return p_client_core_->PerformAction(ActionType::UNSUBSCRIBE, p_unsubscribe_packet, action_response_timeout);
    }

    ResponseCode MqttClient::PublishAsync(std::unique_ptr<Utf8String> p_topic_name,
                                          bool is_retained,
                                          bool is_duplicate,
                                          mqtt::QoS qos,
                                          const util::String &payload,
                                          ActionData::AsyncAckNotificationHandlerPtr p_async_ack_handler,
                                          uint16_t &packet_id_out) {
        if (nullptr == p_topic_name) {
            return ResponseCode::MQTT_INVALID_DATA_ERROR;
        }

        std::shared_ptr<mqtt::PublishPacket> p_publish_packet =
            std::make_shared<mqtt::PublishPacket>(std::move(p_topic_name), is_retained, is_duplicate, qos, payload);
        p_publish_packet->p_async_ack_handler_ = p_async_ack_handler;
        return p_client_core_->PerformActionAsync(ActionType::PUBLISH, p_publish_packet, packet_id_out);
    }

    ResponseCode MqttClient::SubscribeAsync(util::Vector<std::shared_ptr<mqtt::Subscription>> subscription_list,
                                            ActionData::AsyncAckNotificationHandlerPtr p_async_ack_handler,
                                            uint16_t &packet_id_out) {
        if (subscription_list.empty()) {
            return ResponseCode::MQTT_INVALID_DATA_ERROR;
        } else if (MAX_TOPICS_IN_ONE_SUBSCRIBE_PACKET < subscription_list.size()) {
            return ResponseCode::MQTT_TOO_MANY_SUBSCRIPTIONS_IN_REQUEST;
        }

        std::shared_ptr<mqtt::SubscribePacket>
            p_subscribe_packet = std::make_shared<mqtt::SubscribePacket>(subscription_list);
        p_subscribe_packet->p_async_ack_handler_ = p_async_ack_handler;
        return p_client_core_->PerformActionAsync(ActionType::SUBSCRIBE, p_subscribe_packet, packet_id_out);
    }

    ResponseCode MqttClient::UnsubscribeAsync(util::Vector<std::unique_ptr<Utf8String>> topic_list,
                                              ActionData::AsyncAckNotificationHandlerPtr p_async_ack_handler,
                                              uint16_t &packet_id_out) {
        if (topic_list.empty()) {
            return ResponseCode::MQTT_INVALID_DATA_ERROR;
        } else if (MAX_TOPICS_IN_ONE_SUBSCRIBE_PACKET < topic_list.size()) {
            return ResponseCode::MQTT_TOO_MANY_SUBSCRIPTIONS_IN_REQUEST;
        }
        std::shared_ptr<mqtt::UnsubscribePacket>
            p_unsubscribe_packet = std::make_shared<mqtt::UnsubscribePacket>(std::move(topic_list));
        p_unsubscribe_packet->p_async_ack_handler_ = p_async_ack_handler;
        return p_client_core_->PerformActionAsync(ActionType::UNSUBSCRIBE, p_unsubscribe_packet, packet_id_out);
    }

    bool MqttClient::IsConnected() {
        return p_client_state_->IsConnected();
    }

    std::chrono::seconds MqttClient::GetMinReconnectBackoffTimeout() { return p_client_state_->GetMinReconnectBackoffTimeout(); }
    void MqttClient::SetMinReconnectBackoffTimeout(std::chrono::seconds min_reconnect_backoff_timeout) {
        p_client_state_->SetMinReconnectBackoffTimeout(min_reconnect_backoff_timeout);
    }

    std::chrono::seconds MqttClient::GetMaxReconnectBackoffTimeout() { return p_client_state_->GetMaxReconnectBackoffTimeout(); }
    void MqttClient::SetMaxReconnectBackoffTimeout(std::chrono::seconds max_reconnect_backoff_timeout) {
        p_client_state_->SetMaxReconnectBackoffTimeout(max_reconnect_backoff_timeout);
    }

    ResponseCode MqttClient::SetDisconnectCallbackPtr(ClientCoreState::ApplicationDisconnectCallbackPtr p_callback_ptr,
                                                      std::shared_ptr<DisconnectCallbackContextData> p_app_handler_data) {
        p_client_state_->disconnect_handler_ptr_ = p_callback_ptr;
        p_client_state_->p_disconnect_app_handler_data_ = p_app_handler_data;

        return ResponseCode::SUCCESS;
    }

    ResponseCode MqttClient::SetReconnectCallbackPtr(ClientCoreState::ApplicationReconnectCallbackPtr p_callback_ptr,
                                                      std::shared_ptr<ReconnectCallbackContextData> p_app_handler_data) {
        p_client_state_->reconnect_handler_ptr_ = p_callback_ptr;
        p_client_state_->p_reconnect_app_handler_data_ = p_app_handler_data;

        return ResponseCode::SUCCESS;
    }

    ResponseCode MqttClient::SetResubscribeCallbackPtr(ClientCoreState::ApplicationResubscribeCallbackPtr p_callback_ptr,
                                                      std::shared_ptr<ResubscribeCallbackContextData> p_app_handler_data) {
        p_client_state_->resubscribe_handler_ptr_ = p_callback_ptr;
        p_client_state_->p_resubscribe_app_handler_data_ = p_app_handler_data;

        return ResponseCode::SUCCESS;
    }

    MqttClient::~MqttClient() {
        if (IsConnected()) {
            ResponseCode rc = Disconnect(p_client_state_->GetMqttCommandTimeout());
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(MQTT_CLIENT_LOG_TAG, "Disconnect returned error while destroying MQTT Client. %s",
                              ResponseHelper::ToString(rc).c_str());
            }
        }

        // wait for all running threads to finish respective tasks
        p_client_core_->GracefulShutdownAllThreadTasks();

        // p_client_state_.action_map_ and p_client_state_.outbound_action_queue_ retains p_client_state_
        // hence, calling p_client_state_->RegisterAction() or p_client_state_->EnqueueOutboundAction() introduces cyclic references inside p_client_state_
        // make sure that p_client_state_.action_map_ and p_client_state_.outbound_action_queue_ are cleared prior to p_client_state_ destructor
        // to break the cyclic references.
        p_client_state_->ClearRegisteredActions();
        p_client_state_->ClearOutboundActionQueue();
    }
}

