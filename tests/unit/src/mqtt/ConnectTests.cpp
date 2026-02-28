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

#include "mqtt/Connect.hpp"
#include "mqtt/ClientState.hpp"
#include "mqtt/GreengrassMqttClient.hpp"
#include "mqtt/NetworkRead.hpp"

#define CONNECT_FIXED_HEADER_VAL 0x10
#define DISCONNECT_FIXED_HEADER_VAL 0xE0

#define KEEP_ALIVE_TIMEOUT_SECS 30
#define MQTT_COMMAND_TIMEOUT_MSECS 20000
#define MQTT_FIXED_HEADER_BYTE_PINGREQ 0xC0

#define SDK_USAGE_METRICS_STRING "?SDK=CPP&Version="

namespace awsiotsdk {
    namespace tests {
        namespace unit {
            class ConnectDisconnectActionTester : public ::testing::Test {
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

                ConnectDisconnectActionTester() {
                    p_core_state_ = mqtt::ClientState::Create(std::chrono::milliseconds(200));
                    p_network_connection_ = std::make_shared<tests::mocks::MockNetworkConnection>();
                    p_network_mock_ = p_network_connection_.get();
                }
            };

            const uint16_t ConnectDisconnectActionTester::test_packet_id_ = 1234;
            const util::String ConnectDisconnectActionTester::test_payload_ = "Test Payload";
            const util::String ConnectDisconnectActionTester::test_client_id_ = "CppSdkTestClient";
            const util::String ConnectDisconnectActionTester::test_topic_name_ = "SdkTest";
            const util::String ConnectDisconnectActionTester::test_user_name_ = SDK_USAGE_METRICS_STRING;
            const std::chrono::seconds
                ConnectDisconnectActionTester::keep_alive_timeout_ = std::chrono::seconds(KEEP_ALIVE_TIMEOUT_SECS);
            const std::chrono::milliseconds ConnectDisconnectActionTester::mqtt_command_timeout_ =
                std::chrono::milliseconds(MQTT_COMMAND_TIMEOUT_MSECS);

            TEST_F(ConnectDisconnectActionTester, ConnectWithNullValues) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                std::unique_ptr<Action> p_null_connect_action = mqtt::ConnectActionAsync::Create(nullptr);
                EXPECT_EQ(nullptr, p_null_connect_action);

                std::unique_ptr<Action> p_connect_action = mqtt::ConnectActionAsync::Create(p_core_state_);
                ResponseCode rc = p_connect_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::NULL_VALUE_ERROR, rc);
            }

            TEST_F(ConnectDisconnectActionTester, ConnectPacketWithNullClientID) {
                std::shared_ptr<mqtt::ConnectPacket> p_created_connect_packet = mqtt::ConnectPacket::Create(false,
                                                                                                            mqtt::Version::MQTT_3_1_1,
                                                                                                            keep_alive_timeout_,
                                                                                                            nullptr,
                                                                                                            nullptr,
                                                                                                            nullptr,
                                                                                                            nullptr);
                EXPECT_EQ(nullptr, p_created_connect_packet);

                mqtt::ConnectPacket p_constructed_connect_packet(false,
                                                                 mqtt::Version::MQTT_3_1_1,
                                                                 keep_alive_timeout_,
                                                                 nullptr,
                                                                 nullptr,
                                                                 nullptr,
                                                                 nullptr);
                EXPECT_NE((unsigned int) 0, p_constructed_connect_packet.ToString().length());
            }

            TEST_F(ConnectDisconnectActionTester, ConnectPacketWithKeepAliveOverLimit) {
                std::shared_ptr<mqtt::ConnectPacket> p_connect_packet = mqtt::ConnectPacket::Create(true,
                                                                                                    mqtt::Version::MQTT_3_1_1,
                                                                                                    std::chrono::seconds(
                                                                                                        UINT16_MAX + 2),
                                                                                                    Utf8String::Create(
                                                                                                        test_client_id_),
                                                                                                    nullptr,
                                                                                                    nullptr,
                                                                                                    nullptr);

                EXPECT_EQ(nullptr, p_connect_packet);

                mqtt::ConnectPacket p_constructed_connect_packet(true,
                                                                 mqtt::Version::MQTT_3_1_1,
                                                                 std::chrono::seconds(
                                                                     UINT16_MAX + 1),
                                                                 Utf8String::Create(
                                                                     test_client_id_),
                                                                 nullptr,
                                                                 nullptr,
                                                                 nullptr);
                EXPECT_NE((unsigned int) 0, p_constructed_connect_packet.ToString().length());
            }

            TEST_F(ConnectDisconnectActionTester, DisconnectActionAsyncWithNullClientState) {
                std::unique_ptr<Action> p_disconnect_action_async = mqtt::DisconnectActionAsync::Create(nullptr);
                EXPECT_EQ(nullptr, p_disconnect_action_async);
            }

            TEST_F(ConnectDisconnectActionTester, DisconnectActionAsyncWithDisconnectedNetwork) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                std::unique_ptr<Action> p_disconnect_action_async = mqtt::DisconnectActionAsync::Create(p_core_state_);
                EXPECT_NE(nullptr, p_disconnect_action_async);

                p_core_state_->SetConnected(false);
                ResponseCode rc = p_disconnect_action_async->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::NETWORK_DISCONNECTED_ERROR, rc);
            }

            TEST_F(ConnectDisconnectActionTester, KeepaliveActionRunnerWithNullClientState) {
                std::unique_ptr<Action> p_keepalive_action_runner = mqtt::KeepaliveActionRunner::Create(nullptr);
                EXPECT_EQ(nullptr, p_keepalive_action_runner);
            }

            TEST_F(ConnectDisconnectActionTester, ConnectPacketCreateWithWrongKeepalive) {
                std::shared_ptr<mqtt::ConnectPacket> p_connect_packet = mqtt::ConnectPacket::Create(true,
                                                                                                    mqtt::Version::MQTT_3_1_1,
                                                                                                    std::chrono::seconds(UINT16_MAX + 1),
                                                                                                    Utf8String::Create(
                                                                                                        test_client_id_),
                                                                                                    nullptr,
                                                                                                    nullptr,
                                                                                                    nullptr,
                                                                                                    true);
                EXPECT_EQ(nullptr, p_connect_packet);

                p_connect_packet = mqtt::ConnectPacket::Create(true,
                                                               mqtt::Version::MQTT_3_1_1,
                                                               std::chrono::seconds(UINT16_MAX + 1),
                                                               Utf8String::Create(
                                                                   test_client_id_),
                                                               nullptr,
                                                               nullptr,
                                                               nullptr);

                EXPECT_EQ(nullptr, p_connect_packet);

            }

            TEST_F(ConnectDisconnectActionTester, ConnectActionTestNoWillMessage) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                std::unique_ptr<Action> p_connect_action = mqtt::ConnectActionAsync::Create(p_core_state_);
                std::shared_ptr<mqtt::ConnectPacket> p_connect_packet = mqtt::ConnectPacket::Create(true,
                                                                                                    mqtt::Version::MQTT_3_1_1,
                                                                                                    keep_alive_timeout_,
                                                                                                    Utf8String::Create(
                                                                                                        test_client_id_),
                                                                                                    nullptr,
                                                                                                    nullptr,
                                                                                                    nullptr,
                                                                                                    true);

                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                EXPECT_CALL(*p_network_mock_, ConnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(::testing::DoAll(
                    ::testing::SetArgReferee<1>(p_connect_packet->Size()),
                    ::testing::Return(ResponseCode::SUCCESS)));
                ResponseCode rc = p_connect_action->PerformAction(p_network_connection_, p_connect_packet);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_TRUE(p_network_connection_->was_write_called_);

                unsigned char *p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());

                EXPECT_EQ(CONNECT_FIXED_HEADER_VAL, (int) p_last_msg[0]);
                p_last_msg++;

                //Rem len = Variable header (10 bytes) + 2 + test_client_id_ length
                size_t expected_rem_len = 10 + 2 + test_client_id_.length() + 2 + test_user_name_.length() + strlen(SDK_VERSION_STRING);
                size_t written_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
                EXPECT_EQ(expected_rem_len, written_rem_len);

                std::unique_ptr<Utf8String> protocol_id = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ("MQTT", protocol_id->ToStdString());

                // MQTT Version
                EXPECT_EQ(0x04, (int) p_last_msg[0]);
                p_last_msg++;

                // Connect flags
                EXPECT_EQ(0x82, (int) p_last_msg[0]);
                p_last_msg++;

                uint16_t keep_alive_timeout = TestHelper::ReadUint16FromBuffer(&p_last_msg);
                EXPECT_EQ(keep_alive_timeout_, std::chrono::seconds(keep_alive_timeout));
                EXPECT_EQ(KEEP_ALIVE_TIMEOUT_SECS, keep_alive_timeout);

                std::unique_ptr<Utf8String> client_id = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ(test_client_id_, client_id->ToStdString());

                EXPECT_CALL(*p_network_mock_, ConnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(::testing::DoAll(
                    ::testing::SetArgReferee<1>(p_connect_packet->Size()),
                    ::testing::Return(ResponseCode::SUCCESS)));
                rc = p_connect_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_TRUE(p_network_connection_->was_write_called_);

                // Repeat with nullptr to check auto-reconnect case
                p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());

                EXPECT_EQ(CONNECT_FIXED_HEADER_VAL, (int) p_last_msg[0]);
                p_last_msg++;

                //Rem len = Variable header (10 bytes) + 2 + test_client_id_ length
                expected_rem_len = 10 + 2 + test_client_id_.length() + 2 + test_user_name_.length() + strlen(SDK_VERSION_STRING);
                written_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
                EXPECT_EQ(expected_rem_len, written_rem_len);

                protocol_id = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ("MQTT", protocol_id->ToStdString());

                // MQTT Version
                EXPECT_EQ(0x04, (int) p_last_msg[0]);
                p_last_msg++;

                // Connect flags
                EXPECT_EQ(0x82, (int) p_last_msg[0]);
                p_last_msg++;

                keep_alive_timeout = TestHelper::ReadUint16FromBuffer(&p_last_msg);
                EXPECT_EQ(keep_alive_timeout_, std::chrono::seconds(keep_alive_timeout));
                EXPECT_EQ(KEEP_ALIVE_TIMEOUT_SECS, keep_alive_timeout);

                client_id = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ(test_client_id_, client_id->ToStdString());
            }

            TEST_F(ConnectDisconnectActionTester, ConnectActionTestWithWillMessage) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                util::String payload = test_payload_;
                std::unique_ptr<Action> p_connect_action = mqtt::ConnectActionAsync::Create(p_core_state_);
                std::unique_ptr<mqtt::WillOptions> p_will_options =
                    mqtt::WillOptions::Create(false, mqtt::QoS::QOS0, Utf8String::Create(test_topic_name_), payload);
                std::shared_ptr<mqtt::ConnectPacket> p_connect_packet = mqtt::ConnectPacket::Create(true,
                                                                                                    mqtt::Version::MQTT_3_1_1,
                                                                                                    keep_alive_timeout_,
                                                                                                    Utf8String::Create(
                                                                                                        test_client_id_),
                                                                                                    nullptr,
                                                                                                    nullptr,
                                                                                                    std::move(
                                                                                                        p_will_options),
                                                                                                    true);

                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                EXPECT_CALL(*p_network_mock_, ConnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(::testing::DoAll(
                    ::testing::SetArgReferee<1>(p_connect_packet->Size()),
                    ::testing::Return(ResponseCode::SUCCESS)));
                ResponseCode rc = p_connect_action->PerformAction(p_network_connection_, p_connect_packet);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_TRUE(p_network_connection_->was_write_called_);

                unsigned char *p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());

                EXPECT_EQ(CONNECT_FIXED_HEADER_VAL, (int) p_last_msg[0]);
                p_last_msg++;

                //Rem len = Variable header (10 bytes) + 2 + test_client_id_ length
                size_t expected_rem_len = 10 + 2 + test_client_id_.length() + 2 + test_topic_name_.length() + 2 +
                                          test_payload_.length() + 2 + test_user_name_.length() + strlen(SDK_VERSION_STRING);
                size_t written_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
                EXPECT_EQ(expected_rem_len, written_rem_len);

                std::unique_ptr<Utf8String> protocol_id = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ("MQTT", protocol_id->ToStdString());

                // MQTT Version
                EXPECT_EQ(0x04, (int) p_last_msg[0]);
                p_last_msg++;

                // Connect flags
                EXPECT_EQ(0x86, (int) p_last_msg[0]);
                p_last_msg++;

                uint16_t keep_alive_timeout = TestHelper::ReadUint16FromBuffer(&p_last_msg);
                EXPECT_EQ(keep_alive_timeout_, std::chrono::seconds(keep_alive_timeout));
                EXPECT_EQ(KEEP_ALIVE_TIMEOUT_SECS, keep_alive_timeout);

                std::unique_ptr<Utf8String> client_id = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ(test_client_id_, client_id->ToStdString());

                EXPECT_CALL(*p_network_mock_, ConnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(::testing::DoAll(
                    ::testing::SetArgReferee<1>(p_connect_packet->Size()),
                    ::testing::Return(ResponseCode::SUCCESS)));
                rc = p_connect_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_TRUE(p_network_connection_->was_write_called_);

                // Repeat with nullptr to check auto-reconnect case
                p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());

                EXPECT_EQ(CONNECT_FIXED_HEADER_VAL, (int) p_last_msg[0]);
                p_last_msg++;

                //Rem len = Variable header (10 bytes) + 2 + test_client_id_ length
                expected_rem_len = 10 + 2 + test_client_id_.length() + 2 + test_topic_name_.length() +
                                   2 + test_payload_.length() + 2 + test_user_name_.length() + strlen(SDK_VERSION_STRING);
                written_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
                EXPECT_EQ(expected_rem_len, written_rem_len);

                protocol_id = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ("MQTT", protocol_id->ToStdString());

                // MQTT Version
                EXPECT_EQ(0x04, (int) p_last_msg[0]);
                p_last_msg++;

                // Connect flags
                EXPECT_EQ(0x86, (int) p_last_msg[0]);
                p_last_msg++;

                keep_alive_timeout = TestHelper::ReadUint16FromBuffer(&p_last_msg);
                EXPECT_EQ(keep_alive_timeout_, std::chrono::seconds(keep_alive_timeout));
                EXPECT_EQ(KEEP_ALIVE_TIMEOUT_SECS, keep_alive_timeout);

                client_id = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ(test_client_id_, client_id->ToStdString());
            }

            TEST_F(ConnectDisconnectActionTester, DisconnectActionTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                std::unique_ptr<Action> p_disconnect_action = mqtt::DisconnectActionAsync::Create(p_core_state_);
                std::shared_ptr<mqtt::DisconnectPacket> p_disconnect_packet = mqtt::DisconnectPacket::Create();

                p_core_state_->SetConnected(true);

                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                EXPECT_CALL(*p_network_mock_, DisconnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(::testing::DoAll(
                    ::testing::SetArgReferee<1>(p_disconnect_packet->Size()),
                    ::testing::Return(ResponseCode::SUCCESS)));
                ResponseCode rc = p_disconnect_action->PerformAction(p_network_connection_, p_disconnect_packet);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_TRUE(p_network_connection_->was_write_called_);

                unsigned char *p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());

                EXPECT_EQ(DISCONNECT_FIXED_HEADER_VAL, (int) p_last_msg[0]);
                p_last_msg++;

                //Rem len =  0 for Disconnect
                size_t expected_rem_len = 0;
                size_t written_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
                EXPECT_EQ(expected_rem_len, written_rem_len);
            }

            TEST_F(ConnectDisconnectActionTester, HandleConnackAcceptedTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_core_state_->SetConnected(false);
                p_network_connection_->ClearNextReadBuf();
                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedConnAckMessage(false,
                                                                                              ConnackTestReturnCode::CONNECTION_ACCEPTED));

                std::unique_ptr<Action> p_network_read_action = mqtt::NetworkReadActionRunner::Create(p_core_state_);

                ResponseCode rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
                EXPECT_TRUE(p_core_state_->IsConnected());
            }

            TEST_F(ConnectDisconnectActionTester, HandleConnackRejectedUnacceptableProtocolTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_core_state_->SetConnected(false);
                p_network_connection_->ClearNextReadBuf();
                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedConnAckMessage(false,
                                                                                              ConnackTestReturnCode::UNACCEPTABLE_PROTOCOL_VERSION_ERROR));

                std::unique_ptr<Action> p_network_read_action = mqtt::NetworkReadActionRunner::Create(p_core_state_);

                ResponseCode rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR, rc);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
                EXPECT_FALSE(p_core_state_->IsConnected());
            }

            TEST_F(ConnectDisconnectActionTester, HandleConnackRejectedIdentifierRejectedTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_core_state_->SetConnected(false);
                p_network_connection_->ClearNextReadBuf();
                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedConnAckMessage(false,
                                                                                              ConnackTestReturnCode::IDENTIFIER_REJECTED_ERROR));

                std::unique_ptr<Action> p_network_read_action = mqtt::NetworkReadActionRunner::Create(p_core_state_);

                ResponseCode rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR, rc);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
                EXPECT_FALSE(p_core_state_->IsConnected());
            }

            TEST_F(ConnectDisconnectActionTester, HandleConnackRejectedServerUnavailableTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_core_state_->SetConnected(false);
                p_network_connection_->ClearNextReadBuf();
                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedConnAckMessage(false,
                                                                                              ConnackTestReturnCode::SERVER_UNAVAILABLE_ERROR));

                std::unique_ptr<Action> p_network_read_action = mqtt::NetworkReadActionRunner::Create(p_core_state_);

                ResponseCode rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR, rc);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
                EXPECT_FALSE(p_core_state_->IsConnected());
            }

            TEST_F(ConnectDisconnectActionTester, HandleConnackRejectedBadUserdataTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_core_state_->SetConnected(false);
                p_network_connection_->ClearNextReadBuf();
                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedConnAckMessage(false,
                                                                                              ConnackTestReturnCode::BAD_USERDATA_ERROR));

                std::unique_ptr<Action> p_network_read_action = mqtt::NetworkReadActionRunner::Create(p_core_state_);

                ResponseCode rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::MQTT_CONNACK_BAD_USERDATA_ERROR, rc);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
                EXPECT_FALSE(p_core_state_->IsConnected());
            }

            TEST_F(ConnectDisconnectActionTester, HandleConnackRejectedNotAuthorizedTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_core_state_->SetConnected(false);
                p_network_connection_->ClearNextReadBuf();
                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedConnAckMessage(false,
                                                                                              ConnackTestReturnCode::NOT_AUTHORIZED_ERROR));

                std::unique_ptr<Action> p_network_read_action = mqtt::NetworkReadActionRunner::Create(p_core_state_);

                ResponseCode rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::MQTT_CONNACK_NOT_AUTHORIZED_ERROR, rc);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
                EXPECT_FALSE(p_core_state_->IsConnected());
            }

            TEST_F(ConnectDisconnectActionTester, HandleConnackInvalidReturnCodeTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_core_state_->SetConnected(false);
                p_network_connection_->ClearNextReadBuf();
                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedConnAckMessage(false,
                                                                                              ConnackTestReturnCode::INVALID_VALUE_ERROR));

                std::unique_ptr<Action> p_network_read_action = mqtt::NetworkReadActionRunner::Create(p_core_state_);

                ResponseCode rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::MQTT_UNEXPECTED_PACKET_FORMAT_ERROR, rc);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
                EXPECT_FALSE(p_core_state_->IsConnected());
            }

            TEST_F(ConnectDisconnectActionTester, KeepAliveSendPingreqTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                std::chrono::seconds keepalive = std::chrono::seconds(1);
                p_core_state_->SetConnected(true);
                p_core_state_->SetAutoReconnectEnabled(true);
                p_core_state_->SetAutoReconnectRequired(false);
                p_core_state_->SetPingreqPending(false);
                p_core_state_->SetKeepAliveTimeout(keepalive);
                p_core_state_->p_network_connection_ = nullptr;

                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                std::unique_ptr<Action> p_keepalive_action = mqtt::KeepaliveActionRunner::Create(p_core_state_);

                std::shared_ptr<mqtt::PingreqPacket> p_pingreq_packet = mqtt::PingreqPacket::Create();
                EXPECT_NE(nullptr, p_pingreq_packet);

                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(::testing::DoAll(
                    ::testing::SetArgReferee<1>(p_pingreq_packet->Size()),
                    ::testing::Return(ResponseCode::SUCCESS)));

                std::shared_ptr<std::atomic_bool> thread_task_out_sync = std::make_shared<std::atomic_bool>(true);
                util::Threading::ThreadTask temp_task(util::Threading::DestructorAction::JOIN, thread_task_out_sync,
                                                      "TestKeepAlivePingReq");
                temp_task.Run(&Action::PerformAction, std::move(p_keepalive_action), p_network_connection_,
                              p_pingreq_packet);

                std::this_thread::sleep_for(keepalive * 2);
                EXPECT_TRUE(p_network_connection_->was_write_called_);
                EXPECT_TRUE(p_core_state_->IsPingreqPending());
            }

            TEST_F(ConnectDisconnectActionTester, KeepAlivePingrespNotReceivedTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                std::chrono::seconds keepalive = std::chrono::seconds(1);
                p_core_state_->SetConnected(true);
                p_core_state_->SetAutoReconnectEnabled(true);
                p_core_state_->SetAutoReconnectRequired(false);
                p_core_state_->SetPingreqPending(true);
                p_core_state_->SetKeepAliveTimeout(keepalive);
                p_core_state_->p_network_connection_ = p_network_connection_;
                p_core_state_->RegisterAction(ActionType::DISCONNECT,
                                              mqtt::DisconnectActionAsync::Create,
                                              p_core_state_);

                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                std::unique_ptr<Action> p_keepalive_action = mqtt::KeepaliveActionRunner::Create(p_core_state_);

                std::shared_ptr<mqtt::DisconnectPacket> p_disconnect_packet = mqtt::DisconnectPacket::Create();
                EXPECT_NE(nullptr, p_disconnect_packet);

                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(::testing::DoAll(
                    ::testing::SetArgReferee<1>(p_disconnect_packet->Size()),
                    ::testing::Return(ResponseCode::SUCCESS)));
                EXPECT_CALL(*p_network_mock_, DisconnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));

                std::shared_ptr<std::atomic_bool> thread_task_out_sync = std::make_shared<std::atomic_bool>(true);
                util::Threading::ThreadTask temp_task(util::Threading::DestructorAction::JOIN, thread_task_out_sync,
                                                      "TestKeepAlivePingReq");
                temp_task.Run(&Action::PerformAction, std::move(p_keepalive_action), p_network_connection_,
                              nullptr);

                std::this_thread::sleep_for(keepalive * 2);

                EXPECT_TRUE(p_network_connection_->was_write_called_);
                EXPECT_TRUE(p_core_state_->IsAutoReconnectRequired());
                EXPECT_FALSE(p_core_state_->IsConnected());
                p_core_state_->p_network_connection_ = nullptr;
            }

            TEST_F(ConnectDisconnectActionTester, KeepAliveSendPingreqFailedTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                std::chrono::seconds keepalive = std::chrono::seconds(1);
                p_core_state_->SetConnected(true);
                p_core_state_->SetAutoReconnectEnabled(true);
                p_core_state_->SetAutoReconnectRequired(false);
                p_core_state_->SetPingreqPending(false);
                p_core_state_->SetKeepAliveTimeout(keepalive);
                p_core_state_->p_network_connection_ = p_network_connection_;
                p_core_state_->RegisterAction(ActionType::DISCONNECT,
                                              mqtt::DisconnectActionAsync::Create,
                                              p_core_state_);

                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                std::unique_ptr<Action> p_keepalive_action = mqtt::KeepaliveActionRunner::Create(p_core_state_);

                std::shared_ptr<mqtt::DisconnectPacket> p_disconnect_packet = mqtt::DisconnectPacket::Create();
                EXPECT_NE(nullptr, p_disconnect_packet);

                {
                    ::testing::InSequence test_sequence;
                    EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(
                        ::testing::DoAll(::testing::SetArgReferee<1>(0), ::testing::Return(ResponseCode::FAILURE)));
                    EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(
                        ::testing::DoAll(::testing::SetArgReferee<1>(p_disconnect_packet->Size()),
                                         ::testing::Return(ResponseCode::SUCCESS)));
                }
                EXPECT_CALL(*p_network_mock_,
                            DisconnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));

                std::shared_ptr<std::atomic_bool> thread_task_out_sync = std::make_shared<std::atomic_bool>(true);
                util::Threading::ThreadTask temp_task(util::Threading::DestructorAction::JOIN, thread_task_out_sync,
                                                      "TestKeepAlivePingReq");
                temp_task.Run(&Action::PerformAction, std::move(p_keepalive_action), p_network_connection_,
                              nullptr);

                std::this_thread::sleep_for(keepalive * 2);

                EXPECT_TRUE(p_network_connection_->was_write_called_);
                EXPECT_TRUE(p_core_state_->IsAutoReconnectRequired());
                EXPECT_FALSE(p_core_state_->IsConnected());
                p_core_state_->p_network_connection_ = nullptr;
            }

            TEST_F(ConnectDisconnectActionTester, KeepAliveNoExistingSubscriptionTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                std::chrono::seconds keepalive = std::chrono::seconds(1);
                p_core_state_->SetConnected(true);
                p_core_state_->SetAutoReconnectEnabled(true);
                p_core_state_->SetAutoReconnectRequired(false);
                p_core_state_->SetPingreqPending(false);
                p_core_state_->SetKeepAliveTimeout(keepalive);
                p_core_state_->p_network_connection_ = nullptr;

                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                std::unique_ptr<Action> p_keepalive_action = mqtt::KeepaliveActionRunner::Create(p_core_state_);

                std::shared_ptr<mqtt::PingreqPacket> p_pingreq_packet = mqtt::PingreqPacket::Create();
                EXPECT_NE(nullptr, p_pingreq_packet);

                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(::testing::DoAll(
                    ::testing::SetArgReferee<1>(p_pingreq_packet->Size()),
                    ::testing::Return(ResponseCode::SUCCESS)));

                std::shared_ptr<std::atomic_bool> thread_task_out_sync = std::make_shared<std::atomic_bool>(true);
                util::Threading::ThreadTask temp_task(util::Threading::DestructorAction::JOIN, thread_task_out_sync,
                                                      "TestKeepAlivePingReq");
                temp_task.Run(&Action::PerformAction, std::move(p_keepalive_action), p_network_connection_,
                              p_pingreq_packet);

                std::this_thread::sleep_for(keepalive * 2);
                EXPECT_TRUE(p_network_connection_->was_write_called_);
                EXPECT_TRUE(p_core_state_->IsPingreqPending());

                unsigned char *p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());
                if (p_network_connection_->last_write_buf_.length() != 0) {
                    EXPECT_EQ(PINGREQ_PACKET_FIXED_HEADER_VAL, *(p_last_msg++));
                    EXPECT_EQ(*(p_last_msg), '\0');
                }
            }

            TEST_F(ConnectDisconnectActionTester, ConnectActionTestWithNoClientID) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                util::String payload = test_payload_;
                std::unique_ptr<Action> p_connect_action = mqtt::ConnectActionAsync::Create(p_core_state_);
                std::unique_ptr<mqtt::WillOptions> p_will_options =
                    mqtt::WillOptions::Create(false, mqtt::QoS::QOS0, Utf8String::Create(test_topic_name_), payload);
                std::shared_ptr<mqtt::ConnectPacket> p_connect_packet = mqtt::ConnectPacket::Create(true,
                                                                                                    mqtt::Version::MQTT_3_1_1,
                                                                                                    keep_alive_timeout_,
                                                                                                    nullptr,
                                                                                                    nullptr,
                                                                                                    nullptr,
                                                                                                    std::move(
                                                                                                        p_will_options),
                                                                                                    true);

                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));

                EXPECT_CALL(*p_network_mock_, ConnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(::testing::DoAll(
                    ::testing::SetArgReferee<1>(p_connect_packet->Size()),
                    ::testing::Return(ResponseCode::SUCCESS)));
                ResponseCode rc = p_connect_action->PerformAction(p_network_connection_, p_connect_packet);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_TRUE(p_network_connection_->was_write_called_);

                unsigned char *p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());

                EXPECT_EQ(CONNECT_FIXED_HEADER_VAL, (int) p_last_msg[0]);
                p_last_msg++;

                //Rem len = Variable header (10 bytes) + 2 + test_client_id_ length
                size_t expected_rem_len = 10 + 2 + 2 + test_topic_name_.length() + 2 + test_payload_.length()
                                          + 2 + test_user_name_.length() + strlen(SDK_VERSION_STRING);
                size_t written_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
                EXPECT_EQ(expected_rem_len, written_rem_len);

                std::unique_ptr<Utf8String> protocol_id = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ("MQTT", protocol_id->ToStdString());

                // MQTT Version
                EXPECT_EQ(0x04, (int) p_last_msg[0]);
                p_last_msg++;

                // Connect flags
                EXPECT_EQ(0x86, (int) p_last_msg[0]);
                p_last_msg++;

                uint16_t keep_alive_timeout = TestHelper::ReadUint16FromBuffer(&p_last_msg);
                EXPECT_EQ(keep_alive_timeout_, std::chrono::seconds(keep_alive_timeout));
                EXPECT_EQ(KEEP_ALIVE_TIMEOUT_SECS, keep_alive_timeout);

                EXPECT_CALL(*p_network_mock_, ConnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(::testing::DoAll(
                    ::testing::SetArgReferee<1>(p_connect_packet->Size()),
                    ::testing::Return(ResponseCode::SUCCESS)));
                rc = p_connect_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_TRUE(p_network_connection_->was_write_called_);

                // Repeat with nullptr to check auto-reconnect case
                p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());

                EXPECT_EQ(CONNECT_FIXED_HEADER_VAL, (int) p_last_msg[0]);
                p_last_msg++;

                //Rem len = Variable header (10 bytes) + 2 + test_client_id_ length
                expected_rem_len = 10 + 2 + 2 + test_topic_name_.length() + 2 + test_payload_.length() +
                                   2 + test_user_name_.length() + strlen(SDK_VERSION_STRING);
                written_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
                EXPECT_EQ(expected_rem_len, written_rem_len);

                protocol_id = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ("MQTT", protocol_id->ToStdString());

                // MQTT Version
                EXPECT_EQ(0x04, (int) p_last_msg[0]);
                p_last_msg++;

                // Connect flags
                EXPECT_EQ(0x86, (int) p_last_msg[0]);
                p_last_msg++;

                keep_alive_timeout = TestHelper::ReadUint16FromBuffer(&p_last_msg);
                EXPECT_EQ(keep_alive_timeout_, std::chrono::seconds(keep_alive_timeout));
                EXPECT_EQ(KEEP_ALIVE_TIMEOUT_SECS, keep_alive_timeout);
            }
        }
    }
}
