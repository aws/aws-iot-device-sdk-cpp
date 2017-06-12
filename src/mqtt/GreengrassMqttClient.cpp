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
 * @file GreengrassClient.cpp
 * @brief
 *
 */

#include "util/logging/LogMacros.hpp"

#include "ResponseCode.hpp"
#include "mqtt/GreengrassMqttClient.hpp"
#include "mqtt/NetworkRead.hpp"

#include "discovery/Discovery.hpp"

#define MQTT_ACTION_TIMEOUT_MS 2000

#define MQTT_CLIENT_LOG_TAG "[MQTT Client]"

namespace awsiotsdk {
    std::unique_ptr<GreengrassMqttClient> GreengrassMqttClient::Create(std::shared_ptr<NetworkConnection> p_network_connection,
                                                                       std::chrono::milliseconds mqtt_command_timeout) {
        if (nullptr == p_network_connection) {
            return nullptr;
        }

        return std::unique_ptr<GreengrassMqttClient>(new GreengrassMqttClient(p_network_connection,
                                                                              mqtt_command_timeout,
                                                                              nullptr,
                                                                              nullptr));
    }

    std::unique_ptr<GreengrassMqttClient> GreengrassMqttClient::Create(std::shared_ptr<NetworkConnection> p_network_connection,
                                                                       std::chrono::milliseconds mqtt_command_timeout,
                                                                       ClientCoreState::ApplicationDisconnectCallbackPtr p_callback_ptr,
                                                                       std::shared_ptr<DisconnectCallbackContextData> p_app_handler_data) {
        if (nullptr == p_network_connection) {
            return nullptr;
        }

        return std::unique_ptr<GreengrassMqttClient>(new GreengrassMqttClient(p_network_connection,
                                                                              mqtt_command_timeout,
                                                                              p_callback_ptr,
                                                                              p_app_handler_data));
    }

    GreengrassMqttClient::GreengrassMqttClient(std::shared_ptr<NetworkConnection> p_network_connection,
                                               std::chrono::milliseconds mqtt_command_timeout,
                                               ClientCoreState::ApplicationDisconnectCallbackPtr p_callback_ptr,
                                               std::shared_ptr<DisconnectCallbackContextData> p_app_handler_data)
        : MqttClient::MqttClient(p_network_connection, mqtt_command_timeout, p_callback_ptr, p_app_handler_data) {
        // Register Greengrass Discover Action in addition to other MQTT actions
        p_client_core_->RegisterAction(ActionType::GREENGRASS_DISCOVER, discovery::DiscoverAction::Create);
    }

    GreengrassMqttClient::GreengrassMqttClient(std::shared_ptr<NetworkConnection> p_network_connection,
                                               std::chrono::milliseconds mqtt_command_timeout)
        : GreengrassMqttClient(p_network_connection, mqtt_command_timeout, nullptr, nullptr) {

    }

    ResponseCode GreengrassMqttClient::Discover(std::chrono::milliseconds action_response_timeout,
                                                std::unique_ptr<Utf8String> p_thing_name,
                                                DiscoveryResponse &discovery_response) {
        std::shared_ptr<discovery::DiscoverRequestData> p_discover_request_data
            = std::make_shared<discovery::DiscoverRequestData>(std::move(p_thing_name), action_response_timeout);
        ResponseCode rc = p_client_core_->PerformAction(ActionType::GREENGRASS_DISCOVER, p_discover_request_data,
                                                        action_response_timeout);
        if (ResponseCode::DISCOVER_ACTION_SUCCESS == rc) {
            discovery_response.SetResponseDocument(std::move(p_discover_request_data->discovery_response_.GetResponseDocument()));
        }

        return rc;
    }

    GreengrassMqttClient::~GreengrassMqttClient() {
    }
}

