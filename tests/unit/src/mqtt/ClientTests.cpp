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
 * @file ConnectTests.cpp
 * @brief
 *
 */

#include <chrono>

#include <gtest/gtest.h>

#include "TestHelper.hpp"
#include "MockNetworkConnection.hpp"

#include "mqtt/NetworkRead.hpp"

#include "mqtt/GreengrassMqttClient.hpp"
#include "mqtt/ClientState.hpp"

#define CONNECT_FIXED_HEADER_VAL 0x10
#define DISCONNECT_FIXED_HEADER_VAL 0xE0

#define KEEP_ALIVE_TIMEOUT_SECS 30

#define MQTT_FIXED_HEADER_BYTE_PINGREQ 0xC0

#define MAX_RECONNECT_BACKOFF_TIME 128
#define MIN_RECONNECT_BACKOFF_TIME 2

#define MQTT_COMMAND_TIMEOUT 20000

#define SDK_USAGE_METRICS_STRING "?SDK=CPP&Version="

namespace awsiotsdk {
    namespace tests {
        namespace unit {
            class GreengrassClientTester : public ::testing::Test {
            protected:
                std::shared_ptr<mqtt::ClientState> p_core_state_;
                std::shared_ptr<tests::mocks::MockNetworkConnection> p_network_connection_;
                tests::mocks::MockNetworkConnection *p_network_mock_;

                static const uint16_t test_packet_id_;
                static const util::String test_payload_;
                static const util::String test_client_id_;
                static const util::String test_user_name_;
                static const util::String test_topic_name_;
                static const std::chrono::seconds keep_alive_timeout_;
                static const std::chrono::milliseconds mqtt_command_timeout_;
                std::shared_ptr<GreengrassMqttClient> p_iot_greengrass_client_;

                GreengrassClientTester() {
                    p_core_state_ = mqtt::ClientState::Create(std::chrono::milliseconds(200));
                    p_network_connection_ = std::make_shared<tests::mocks::MockNetworkConnection>();
                    p_network_mock_ = p_network_connection_.get();
                    p_iot_greengrass_client_ =
                        std::shared_ptr<GreengrassMqttClient>(GreengrassMqttClient::Create(p_network_connection_,
                                                                                           std::chrono::milliseconds(2000)));
                }
            };

            const uint16_t GreengrassClientTester::test_packet_id_ = 1234;
            const util::String GreengrassClientTester::test_payload_ = "Test Payload";
            const util::String GreengrassClientTester::test_client_id_ = "CppSdkTestClient";
            const util::String GreengrassClientTester::test_topic_name_ = "SdkTest";
            const util::String test_user_name_ = SDK_USAGE_METRICS_STRING;
            const std::chrono::seconds
                GreengrassClientTester::keep_alive_timeout_ = std::chrono::seconds(KEEP_ALIVE_TIMEOUT_SECS);
            const std::chrono::milliseconds
                GreengrassClientTester::mqtt_command_timeout_ = std::chrono::milliseconds(MQTT_COMMAND_TIMEOUT);

            TEST_F(GreengrassClientTester, TestConstructorErrorCases) {
                EXPECT_NE(nullptr, p_network_connection_);
                std::unique_ptr<GreengrassMqttClient> client_1 = GreengrassMqttClient::Create(nullptr,
                                                                                              mqtt_command_timeout_);
                EXPECT_EQ(nullptr, client_1);

                std::unique_ptr<GreengrassMqttClient> client_2 = GreengrassMqttClient::Create(nullptr,
                                                                                              mqtt_command_timeout_,
                                                                                              nullptr,
                                                                                              nullptr);
                EXPECT_EQ(nullptr, client_2);

                std::unique_ptr<MqttClient> client_3 = MqttClient::Create(nullptr, mqtt_command_timeout_);
                EXPECT_EQ(nullptr, client_3);

                std::unique_ptr<MqttClient> client_4 = MqttClient::Create(nullptr, mqtt_command_timeout_, nullptr,
                                                                          nullptr);
                EXPECT_EQ(nullptr, client_4);
            }

            TEST_F(GreengrassClientTester, TestClientStateFunctions) {
                std::shared_ptr<mqtt::ClientState> p_client_state = mqtt::ClientState::Create(std::chrono::milliseconds(MQTT_COMMAND_TIMEOUT));
                EXPECT_NE(nullptr, p_client_state);

                p_client_state->setDisconnectCallbackPending(true);
                EXPECT_TRUE(p_client_state->isDisconnectCallbackPending());

                p_client_state->SetMaxReconnectBackoffTimeout(std::chrono::seconds(MAX_RECONNECT_BACKOFF_TIME));
                EXPECT_EQ(std::chrono::seconds(MAX_RECONNECT_BACKOFF_TIME),
                          p_client_state->GetMaxReconnectBackoffTimeout());

                p_client_state->SetMinReconnectBackoffTimeout(std::chrono::seconds(MIN_RECONNECT_BACKOFF_TIME));
                EXPECT_EQ(std::chrono::seconds(MIN_RECONNECT_BACKOFF_TIME),
                          p_client_state->GetMinReconnectBackoffTimeout());
            }

            TEST_F(GreengrassClientTester, TestAutoReconnectSetAndGet) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                p_iot_greengrass_client_->SetAutoReconnectEnabled(true);
                EXPECT_TRUE(p_iot_greengrass_client_->IsAutoReconnectEnabled());
            }

            TEST_F(GreengrassClientTester, TestSetDisconnectCallbackPtr) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                ResponseCode rc = p_iot_greengrass_client_->SetDisconnectCallbackPtr(nullptr, nullptr);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
            }

            TEST_F(GreengrassClientTester, TestSetAndGetReconnectBackoffTimeouts) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::chrono::seconds min_reconnect_timeout(2);
                std::chrono::seconds max_reconnect_timeout(10);

                p_iot_greengrass_client_->SetMinReconnectBackoffTimeout(min_reconnect_timeout);
                EXPECT_EQ(min_reconnect_timeout, p_iot_greengrass_client_->GetMinReconnectBackoffTimeout());

                p_iot_greengrass_client_->SetMaxReconnectBackoffTimeout(max_reconnect_timeout);
                EXPECT_EQ(max_reconnect_timeout, p_iot_greengrass_client_->GetMaxReconnectBackoffTimeout());
            }
        }
    }
}
