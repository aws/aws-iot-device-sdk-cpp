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
 * @file PublishTests.cpp
 * @brief
 *
 */

#include <gtest/gtest.h>

#include "MockNetworkConnection.hpp"
#include "TestHelper.hpp"

#include "mqtt/Publish.hpp"
#include "mqtt/ClientState.hpp"
#include "mqtt/NetworkRead.hpp"
#include "mqtt/GreengrassMqttClient.hpp"

#define PUBLISH_QOS0_FIXED_HEADER_RETAINED_FALSE_VAL 0x30
#define PUBLISH_QOS0_FIXED_HEADER_RETAINED_TRUE_VAL 0x31

#define PUBLISH_QOS1_FIXED_HEADER_DUP_FALSE_RETAINED_FALSE_VAL 0x32
#define PUBLISH_QOS1_FIXED_HEADER_DUP_FALSE_RETAINED_TRUE_VAL 0x33
#define PUBLISH_QOS1_FIXED_HEADER_DUP_TRUE_RETAINED_FALSE_VAL 0x3A
#define PUBLISH_QOS1_FIXED_HEADER_DUP_TRUE_RETAINED_TRUE_VAL 0x3B

namespace awsiotsdk {
    namespace tests {
        namespace unit {
            class PublishActionTester : public ::testing::Test {
            protected:
                std::shared_ptr<mqtt::ClientState> p_core_state_;
                std::shared_ptr<tests::mocks::MockNetworkConnection> p_network_connection_;
                tests::mocks::MockNetworkConnection *p_network_mock_;

                std::atomic_bool callback_received_;

                static const uint16_t test_packet_id_;
                static const util::String test_payload_;
                static const util::String test_topic_;

                PublishActionTester() {
                    p_core_state_ = mqtt::ClientState::Create(std::chrono::milliseconds(200));
                    p_network_connection_ = std::make_shared<tests::mocks::MockNetworkConnection>();
                    p_network_mock_ = p_network_connection_.get();

                    EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));
                }
            public:
                void AsyncAckHandler(uint16_t action_id, ResponseCode rc);
            };

            const uint16_t PublishActionTester::test_packet_id_ = 1234;
            const util::String PublishActionTester::test_payload_ = "Hello From C++ SDK Tester";
            const util::String PublishActionTester::test_topic_ = "testtopic";

            void PublishActionTester::AsyncAckHandler(uint16_t action_id, ResponseCode rc) {
                EXPECT_EQ(test_packet_id_, action_id);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                callback_received_ = true;
            }

            TEST_F(PublishActionTester, PublishNullValueChecks) {
                std::shared_ptr<mqtt::PublishPacket> p_publish_packet = mqtt::PublishPacket::Create(
                    nullptr, false, false, mqtt::QoS::QOS1, test_payload_);
                EXPECT_EQ(p_publish_packet, nullptr);

                util::Vector<unsigned char> buf;
                p_publish_packet = mqtt::PublishPacket::Create(buf, false, false, mqtt::QoS::QOS1);
                EXPECT_EQ(p_publish_packet, nullptr);

                std::unique_ptr<Action> publish_action = mqtt::PublishActionAsync::Create(nullptr);
                EXPECT_EQ(nullptr, publish_action);

                std::unique_ptr<Action> puback_action = mqtt::PubackActionAsync::Create(nullptr);
                EXPECT_EQ(nullptr, puback_action);
            }

            TEST_F(PublishActionTester, PubackActiontest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                std::unique_ptr<Action> p_puback_action = mqtt::PubackActionAsync::Create(p_core_state_);
                std::shared_ptr<mqtt::PubackPacket> p_puback_packet = mqtt::PubackPacket::Create(test_packet_id_);

                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(::testing::DoAll(
                    ::testing::SetArgReferee<1>(p_puback_packet->Size()),
                    ::testing::Return(ResponseCode::SUCCESS)));
                ResponseCode rc = p_puback_action->PerformAction(p_network_connection_, p_puback_packet);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(true, p_network_connection_->was_write_called_);
                const char *p_last_msg = p_network_connection_->last_write_buf_.c_str();

                EXPECT_EQ(PUBACK_PACKET_FIXED_HEADER_VAL, (int) p_last_msg[0]);
                EXPECT_EQ(PUBACK_PACKET_REM_LEN_VAL, (int) p_last_msg[1]);

                uint8_t first_byte = (uint8_t) (p_last_msg[2]);
                uint8_t second_byte = (uint8_t) (p_last_msg[3]);
                uint16_t written_packet_id = (uint16_t) (second_byte + (256 * first_byte));
                EXPECT_EQ(test_packet_id_, written_packet_id);
            }

            TEST_F(PublishActionTester, PublishQoS0ActionTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                std::unique_ptr<Action> p_publish_action = mqtt::PublishActionAsync::Create(p_core_state_);

                /*** Is Retained false   ***/
                std::shared_ptr<mqtt::PublishPacket> p_publish_packet = mqtt::PublishPacket::Create(
                    Utf8String::Create(test_topic_), false, false, mqtt::QoS::QOS0, test_payload_);

                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<1>(p_publish_packet->Size()),
                                     ::testing::Return(ResponseCode::SUCCESS)));
                ResponseCode rc = p_publish_action->PerformAction(p_network_connection_, p_publish_packet);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(true, p_network_connection_->was_write_called_);
                unsigned char *p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());
                // Check header byte
                EXPECT_EQ(PUBLISH_QOS0_FIXED_HEADER_RETAINED_FALSE_VAL, (int) p_last_msg[0]);
                p_last_msg++;

                // Utf8 String + packet id + payload
                size_t expected_rem_len = test_topic_.length() + 2 + test_payload_.length();
                size_t calculated_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
                EXPECT_EQ(expected_rem_len, calculated_rem_len);

                std::unique_ptr<Utf8String> written_topic_name = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ(test_topic_, written_topic_name->ToStdString());
                util::String
                    written_payload((char *) p_last_msg, calculated_rem_len - written_topic_name->Length() - 2);
                EXPECT_EQ(test_payload_, written_payload);


                /*** Is Retained true   ***/
                p_publish_packet = mqtt::PublishPacket::Create(Utf8String::Create(test_topic_), true, false,
                                                               mqtt::QoS::QOS0, test_payload_);

                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<1>(p_publish_packet->Size()),
                                     ::testing::Return(ResponseCode::SUCCESS)));
                rc = p_publish_action->PerformAction(p_network_connection_, p_publish_packet);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(true, p_network_connection_->was_write_called_);
                p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());
                // Check header byte
                EXPECT_EQ(PUBLISH_QOS0_FIXED_HEADER_RETAINED_TRUE_VAL, (int) p_last_msg[0]);
                p_last_msg++;

                // Utf8 String + packet id + payload
                expected_rem_len = test_topic_.length() + 2 + test_payload_.length();
                calculated_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
                EXPECT_EQ(expected_rem_len, calculated_rem_len);

                written_topic_name = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ(test_topic_, written_topic_name->ToStdString());
                written_payload =
                    util::String((char *) p_last_msg, calculated_rem_len - written_topic_name->Length() - 2);
                EXPECT_EQ(test_payload_, written_payload);
            }

            TEST_F(PublishActionTester, PublishQoS1ActionDupFalseTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                std::unique_ptr<Action> p_publish_action = mqtt::PublishActionAsync::Create(p_core_state_);
                std::unique_ptr<Action> p_network_read_action = mqtt::NetworkReadActionRunner::Create(p_core_state_);

                /*** Is Retained false   ***/
                std::shared_ptr<mqtt::PublishPacket> p_publish_packet = mqtt::PublishPacket::Create(
                    Utf8String::Create(test_topic_), false, false, mqtt::QoS::QOS1, test_payload_);
                ActionData::AsyncAckNotificationHandlerPtr p_async_ack_handler =
                    std::bind(&PublishActionTester::AsyncAckHandler,
                              this,
                              std::placeholders::_1,
                              std::placeholders::_2);
                p_publish_packet->p_async_ack_handler_ = p_async_ack_handler;
                p_publish_packet->SetPacketId(test_packet_id_);

                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<1>(p_publish_packet->Size()),
                                     ::testing::Return(ResponseCode::SUCCESS)));
                ResponseCode rc = p_publish_action->PerformAction(p_network_connection_, p_publish_packet);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(true, p_network_connection_->was_write_called_);
                unsigned char *p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());
                // Check header byte
                EXPECT_EQ(PUBLISH_QOS1_FIXED_HEADER_DUP_FALSE_RETAINED_FALSE_VAL, (int) p_last_msg[0]);
                p_last_msg++;

                // Utf8 String + packet id + payload
                size_t expected_rem_len = 2 + test_topic_.length() + 2 + test_payload_.length();
                size_t calculated_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
                EXPECT_EQ(expected_rem_len, calculated_rem_len);

                std::unique_ptr<Utf8String> written_topic_name = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ(test_topic_, written_topic_name->ToStdString());

                uint16_t written_packet_id = TestHelper::ReadUint16FromBuffer(&p_last_msg);
                EXPECT_EQ(test_packet_id_, written_packet_id);

                util::String
                    written_payload((char *) p_last_msg, calculated_rem_len - written_topic_name->Length() - 4);
                EXPECT_EQ(test_payload_, written_payload);


                /*** Check for Puback ***/
                callback_received_ = false;
                p_core_state_->RegisterPendingAck(test_packet_id_, p_async_ack_handler);

                p_network_connection_->ClearNextReadBuf();
                p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedPubAckMessage(test_packet_id_));

                rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
                EXPECT_TRUE(callback_received_);

                /*** Is Retained true   ***/
                p_publish_packet = mqtt::PublishPacket::Create(Utf8String::Create(test_topic_), true, false,
                                                               mqtt::QoS::QOS1, test_payload_);
                p_publish_packet->SetPacketId(test_packet_id_);

                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<1>(p_publish_packet->Size()),
                                     ::testing::Return(ResponseCode::SUCCESS)));
                rc = p_publish_action->PerformAction(p_network_connection_, p_publish_packet);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(true, p_network_connection_->was_write_called_);
                p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());
                // Check header byte
                EXPECT_EQ(PUBLISH_QOS1_FIXED_HEADER_DUP_FALSE_RETAINED_TRUE_VAL, (int) p_last_msg[0]);
                p_last_msg++;

                // Utf8 String + packet id + payload
                expected_rem_len = 2 + test_topic_.length() + 2 + test_payload_.length();
                calculated_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
                EXPECT_EQ(expected_rem_len, calculated_rem_len);

                written_topic_name = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ(test_topic_, written_topic_name->ToStdString());

                written_packet_id = TestHelper::ReadUint16FromBuffer(&p_last_msg);
                EXPECT_EQ(test_packet_id_, written_packet_id);

                written_payload =
                    util::String((char *) p_last_msg, calculated_rem_len - written_topic_name->Length() - 4);
                EXPECT_EQ(test_payload_, written_payload);


                /*** Check for Puback ***/
                callback_received_ = false;
                p_core_state_->RegisterPendingAck(test_packet_id_, p_async_ack_handler);

                p_network_connection_->ClearNextReadBuf();
                p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedPubAckMessage(test_packet_id_));

                rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
                EXPECT_TRUE(callback_received_);
            }

            TEST_F(PublishActionTester, PublishQoS1ActionDupTrueTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                std::unique_ptr<Action> p_publish_action = mqtt::PublishActionAsync::Create(p_core_state_);
                std::unique_ptr<Action> p_network_read_action = mqtt::NetworkReadActionRunner::Create(p_core_state_);

                /*** Is Retained false   ***/
                std::shared_ptr<mqtt::PublishPacket> p_publish_packet = mqtt::PublishPacket::Create(
                    Utf8String::Create(test_topic_), false, true, mqtt::QoS::QOS1, test_payload_);
                ActionData::AsyncAckNotificationHandlerPtr p_async_ack_handler =
                    std::bind(&PublishActionTester::AsyncAckHandler,
                              this,
                              std::placeholders::_1,
                              std::placeholders::_2);
                p_publish_packet->p_async_ack_handler_ = p_async_ack_handler;
                p_publish_packet->SetPacketId(test_packet_id_);

                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<1>(p_publish_packet->Size()),
                                     ::testing::Return(ResponseCode::SUCCESS)));
                ResponseCode rc = p_publish_action->PerformAction(p_network_connection_, p_publish_packet);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(true, p_network_connection_->was_write_called_);
                unsigned char *p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());
                // Check header byte
                EXPECT_EQ(PUBLISH_QOS1_FIXED_HEADER_DUP_TRUE_RETAINED_FALSE_VAL, (int) p_last_msg[0]);
                p_last_msg++;

                // Utf8 String + packet id + payload
                size_t expected_rem_len = 2 + test_topic_.length() + 2 + test_payload_.length();
                size_t calculated_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
                EXPECT_EQ(expected_rem_len, calculated_rem_len);

                std::unique_ptr<Utf8String> written_topic_name = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ(test_topic_, written_topic_name->ToStdString());

                uint16_t written_packet_id = TestHelper::ReadUint16FromBuffer(&p_last_msg);
                EXPECT_EQ(test_packet_id_, written_packet_id);

                util::String
                    written_payload((char *) p_last_msg, calculated_rem_len - written_topic_name->Length() - 4);
                EXPECT_EQ(test_payload_, written_payload);


                /*** Check for Puback ***/
                callback_received_ = false;
                p_core_state_->RegisterPendingAck(test_packet_id_, p_async_ack_handler);

                p_network_connection_->ClearNextReadBuf();
                p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedPubAckMessage(test_packet_id_));

                rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
                EXPECT_TRUE(callback_received_);


                /*** Is Retained true   ***/
                p_publish_packet = mqtt::PublishPacket::Create(Utf8String::Create(test_topic_), true, true,
                                                               mqtt::QoS::QOS1, test_payload_);
                p_publish_packet->SetPacketId(test_packet_id_);

                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<1>(p_publish_packet->Size()),
                                     ::testing::Return(ResponseCode::SUCCESS)));
                rc = p_publish_action->PerformAction(p_network_connection_, p_publish_packet);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(true, p_network_connection_->was_write_called_);
                p_last_msg = (unsigned char *) (p_network_connection_->last_write_buf_.c_str());
                // Check header byte
                EXPECT_EQ(PUBLISH_QOS1_FIXED_HEADER_DUP_TRUE_RETAINED_TRUE_VAL, (int) p_last_msg[0]);
                p_last_msg++;

                // Utf8 String + packet id + payload
                expected_rem_len = 2 + test_topic_.length() + 2 + test_payload_.length();
                calculated_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
                EXPECT_EQ(expected_rem_len, calculated_rem_len);

                written_topic_name = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
                EXPECT_EQ(test_topic_, written_topic_name->ToStdString());

                written_packet_id = TestHelper::ReadUint16FromBuffer(&p_last_msg);
                EXPECT_EQ(test_packet_id_, written_packet_id);

                written_payload =
                    util::String((char *) p_last_msg, calculated_rem_len - written_topic_name->Length() - 4);
                EXPECT_EQ(test_payload_, written_payload);


                /*** Check for Puback ***/
                callback_received_ = false;
                p_core_state_->RegisterPendingAck(test_packet_id_, p_async_ack_handler);

                p_network_connection_->ClearNextReadBuf();
                p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedPubAckMessage(test_packet_id_));

                rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
                EXPECT_TRUE(callback_received_);
            }

            TEST_F(PublishActionTester, ClientPublishErrorTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                std::shared_ptr<GreengrassMqttClient> p_iot_greengrass_client =
                    std::shared_ptr<GreengrassMqttClient>(GreengrassMqttClient::Create(p_network_connection_,
                                                                                       std::chrono::milliseconds(2000)));
                EXPECT_NE(nullptr, p_iot_greengrass_client);

                ResponseCode
                    rc = p_iot_greengrass_client->Publish(nullptr, false, false, mqtt::QoS::QOS0, test_payload_,
                                                          std::chrono::milliseconds(20000));
                EXPECT_EQ(ResponseCode::MQTT_INVALID_DATA_ERROR, rc);

                uint16_t packet_id_out = 10;
                rc = p_iot_greengrass_client->PublishAsync(nullptr, false, false, mqtt::QoS::QOS0, test_payload_,
                                                           nullptr, packet_id_out);
                EXPECT_EQ(ResponseCode::MQTT_INVALID_DATA_ERROR, rc);
            }
        }
    }
}