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
 * @file SubscribeTests.cpp
 * @brief
 *
 */

#include <atomic>
#include <gtest/gtest.h>

#include "MockNetworkConnection.hpp"
#include "TestHelper.hpp"

#include "mqtt/Subscribe.hpp"
#include "mqtt/NetworkRead.hpp"

#define K 1024
#define LARGE_PAYLOAD_SIZE 127 * K

namespace awsiotsdk {
	namespace tests {
		namespace unit {
			class SubUnsubActionTester : public ::testing::Test {
			protected:
				std::shared_ptr<mqtt::ClientState> p_core_state_;
				std::shared_ptr<tests::mocks::MockNetworkConnection> p_network_connection_;
				tests::mocks::MockNetworkConnection *p_network_mock_;
				std::unique_ptr<Action> p_subscribe_action_;
				std::unique_ptr<Action> p_unsubscribe_action_;
				std::shared_ptr<std::atomic_bool> p_thread_continue_;

				static const uint16_t test_packet_id_;
				static const util::String test_payload_;
				util::String large_test_payload_;
				static const util::String test_topic_base_;

				std::atomic_bool callback_received_;
				util::String cur_expected_topic_name_;


				SubUnsubActionTester() {
					p_core_state_ = mqtt::ClientState::Create(std::chrono::milliseconds(200));
					p_network_connection_ = std::make_shared<tests::mocks::MockNetworkConnection>();
					p_network_mock_ = p_network_connection_.get();
					p_subscribe_action_ = mqtt::SubscribeActionAsync::Create(p_core_state_);
					p_unsubscribe_action_ = mqtt::UnsubscribeActionAsync::Create(p_core_state_);

					EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));
				}

				ResponseCode Subscribe(uint16_t packet_id, util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector);

				ResponseCode Unsubscribe(uint16_t packet_id, util::Vector<std::unique_ptr<Utf8String>> topic_vector);

			public:
				ResponseCode SubscribeCallback(util::String p_topic_name_, util::String payload_, std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data);
				ResponseCode SubscribeCallbackLargePayload(util::String p_topic_name_, util::String payload_, std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data);
			};

			const uint16_t SubUnsubActionTester::test_packet_id_ = 1234;
			const util::String SubUnsubActionTester::test_payload_ = "Hello From C++ SDK Tester";
			const util::String SubUnsubActionTester::test_topic_base_ = "testTopic";

			ResponseCode SubUnsubActionTester::SubscribeCallback(util::String topic_name, util::String payload, std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
				EXPECT_EQ(cur_expected_topic_name_, topic_name);
				EXPECT_EQ(test_payload_, payload);
				callback_received_ = true;

				return ResponseCode::SUCCESS;
			}

			ResponseCode SubUnsubActionTester::SubscribeCallbackLargePayload(util::String topic_name, util::String payload, std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
				EXPECT_EQ(cur_expected_topic_name_, topic_name);
				EXPECT_EQ(large_test_payload_, payload);
				callback_received_ = true;

				return ResponseCode::SUCCESS;
			}

			ResponseCode SubUnsubActionTester::Subscribe(uint16_t packet_id, util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector) {
				std::shared_ptr<mqtt::SubscribePacket> p_sub_packet = mqtt::SubscribePacket::Create(topic_vector);
				EXPECT_NE(nullptr, p_sub_packet);
				p_sub_packet->SetActionId(packet_id);

				EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(
						::testing::DoAll(::testing::SetArgReferee<1>(p_sub_packet->Size()),
										 ::testing::Return(ResponseCode::SUCCESS)));

				ResponseCode rc = p_subscribe_action_->PerformAction(p_network_connection_, p_sub_packet);
				EXPECT_EQ(ResponseCode::SUCCESS, rc);
				EXPECT_TRUE(p_network_connection_->was_write_called_);

				unsigned char *p_last_msg = (unsigned char *)(p_network_connection_->last_write_buf_.c_str());
				// Check header byte
				EXPECT_EQ(SUBSCRIBE_PACKET_FIXED_HEADER_VAL, (uint8_t)(*p_last_msg));
				p_last_msg++;

				// packet ID + 2 byte topic length + topic + 1 byte QoS for each topic
				size_t expected_rem_len = 2;
				for(std::shared_ptr<mqtt::Subscription> p_sub : topic_vector) {
					expected_rem_len += p_sub->GetTopicNameLength() + 2 + 1;
				}

				size_t calculated_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
				EXPECT_EQ(expected_rem_len, calculated_rem_len);

				uint16_t written_packet_id = TestHelper::ReadUint16FromBuffer(&p_last_msg);
				EXPECT_EQ(written_packet_id, packet_id);

				for(std::shared_ptr<mqtt::Subscription> p_sub : topic_vector) {
					util::String expected_topic_name = p_sub->GetTopicName()->ToStdString();
					std::unique_ptr<Utf8String> written_topic_name = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
					EXPECT_EQ(expected_topic_name, written_topic_name->ToStdString());
					// Check QoS was set correctly
					EXPECT_EQ(static_cast<uint8_t>(p_sub->GetMaxQos()), (uint8_t) (*p_last_msg));
					p_last_msg++;

					// Check subscription was added to client state
					std::shared_ptr<mqtt::Subscription> p_subscription_temp = p_core_state_->GetSubscription(
							expected_topic_name);
					EXPECT_NE(nullptr, p_subscription_temp);
					EXPECT_FALSE(p_subscription_temp->IsActive());
				}

				return rc;
			}

			ResponseCode SubUnsubActionTester::Unsubscribe(uint16_t packet_id, util::Vector<std::unique_ptr<Utf8String>> topic_vector) {
				// Create a copy
				util::Vector<std::unique_ptr<Utf8String>> topic_vector_temp;
				for(auto &&itr : topic_vector) {
					topic_vector_temp.push_back(Utf8String::Create(itr->ToStdString()));
				}
				std::shared_ptr<mqtt::UnsubscribePacket> p_unsub_packet = mqtt::UnsubscribePacket::Create(std::move(topic_vector));
				EXPECT_NE(nullptr, p_unsub_packet);
				p_unsub_packet->SetActionId(packet_id);

				EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_, ::testing::_)).WillOnce(
						::testing::DoAll(::testing::SetArgReferee<1>(p_unsub_packet->Size()),
										 ::testing::Return(ResponseCode::SUCCESS)));

				ResponseCode rc = p_unsubscribe_action_->PerformAction(p_network_connection_, p_unsub_packet);
				EXPECT_EQ(ResponseCode::SUCCESS, rc);
				EXPECT_TRUE(p_network_connection_->was_write_called_);

				unsigned char *p_last_msg = (unsigned char *)(p_network_connection_->last_write_buf_.c_str());
				// Check header byte
				EXPECT_EQ(UNSUBSCRIBE_PACKET_FIXED_HEADER_VAL, (uint8_t)(*p_last_msg));
				p_last_msg++;

				// packet ID + 2 byte topic length + topic for each topic
				size_t expected_rem_len = 2;
				for(auto &&itr : topic_vector_temp) {
					expected_rem_len += itr->Length() + 2;
				}

				size_t calculated_rem_len = TestHelper::ParseRemLenFromBuffer(&p_last_msg);
				EXPECT_EQ(expected_rem_len, calculated_rem_len);

				uint16_t written_packet_id = TestHelper::ReadUint16FromBuffer(&p_last_msg);
				EXPECT_EQ(written_packet_id, packet_id);

				for(auto &&itr : topic_vector_temp) {
					std::unique_ptr<Utf8String> written_topic_name = TestHelper::ReadUtf8StringFromBuffer(&p_last_msg);
					EXPECT_EQ(itr->ToStdString(), written_topic_name->ToStdString());
				}

				return rc;
			}

			TEST_F(SubUnsubActionTester, SubscribeActionTestWithOneTopicAndQoS0) {
				ASSERT_NE(nullptr, p_network_connection_);
				ASSERT_NE(nullptr, p_core_state_);
				ASSERT_NE(nullptr, p_subscribe_action_);

				p_network_connection_->last_write_buf_.clear();
				p_network_connection_->was_write_called_ = false;

				mqtt::Subscription::ApplicationCallbackHandlerPtr p_app_handler = std::bind(&SubUnsubActionTester::SubscribeCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

				std::shared_ptr<mqtt::Subscription> p_subscription = mqtt::Subscription::Create(Utf8String::Create(test_topic_base_), mqtt::QoS::QOS0, p_app_handler, nullptr);
				util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;

				topic_vector.push_back(p_subscription);
				ResponseCode rc = Subscribe(test_packet_id_, std::move(topic_vector));
				EXPECT_EQ(ResponseCode::SUCCESS, rc);
			}

			TEST_F(SubUnsubActionTester, SubscribeActionTestWithMaxAllowedTopicsAndQoS0) {
				ASSERT_NE(nullptr, p_network_connection_);
				ASSERT_NE(nullptr, p_core_state_);
				ASSERT_NE(nullptr, p_subscribe_action_);

				p_network_connection_->last_write_buf_.clear();
				p_network_connection_->was_write_called_ = false;

				util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;
				mqtt::Subscription::ApplicationCallbackHandlerPtr p_app_handler = std::bind(&SubUnsubActionTester::SubscribeCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);


				for(int itr = 1; itr <= MAX_TOPICS_IN_ONE_SUBSCRIBE_PACKET; itr++) {
					util::String sub_topic = test_topic_base_;
					sub_topic.append("_");
					sub_topic.append(std::to_string(itr));
					std::shared_ptr<mqtt::Subscription> p_subscription = mqtt::Subscription::Create(
							Utf8String::Create(sub_topic), mqtt::QoS::QOS0, p_app_handler, nullptr);
					topic_vector.push_back(p_subscription);
				}
				ResponseCode rc = Subscribe(test_packet_id_, std::move(topic_vector));
				EXPECT_EQ(ResponseCode::SUCCESS, rc);
			}

			TEST_F(SubUnsubActionTester, SubscribeActionTestWithOneTopicAndQoS1) {
				ASSERT_NE(nullptr, p_network_connection_);
				ASSERT_NE(nullptr, p_core_state_);
				ASSERT_NE(nullptr, p_subscribe_action_);

				p_network_connection_->last_write_buf_.clear();
				p_network_connection_->was_write_called_ = false;

				std::unique_ptr<Action> p_subscribe_action = mqtt::SubscribeActionAsync::Create(p_core_state_);
				mqtt::Subscription::ApplicationCallbackHandlerPtr p_app_handler = std::bind(&SubUnsubActionTester::SubscribeCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

				std::shared_ptr<mqtt::Subscription> p_subscription = mqtt::Subscription::Create(Utf8String::Create(test_topic_base_), mqtt::QoS::QOS1, p_app_handler, nullptr);
				util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;

				topic_vector.push_back(p_subscription);
				std::shared_ptr<mqtt::SubscribePacket> p_sub_packet = mqtt::SubscribePacket::Create(topic_vector);
				EXPECT_NE(nullptr, p_sub_packet);
				ResponseCode rc = Subscribe(test_packet_id_, std::move(topic_vector));
				EXPECT_EQ(ResponseCode::SUCCESS, rc);
			}

			TEST_F(SubUnsubActionTester, UnubscribeActionTestWithOneTopic) {
				ASSERT_NE(nullptr, p_network_connection_);
				ASSERT_NE(nullptr, p_core_state_);
				ASSERT_NE(nullptr, p_subscribe_action_);

				p_network_connection_->last_write_buf_.clear();
				p_network_connection_->was_write_called_ = false;

				std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(test_topic_base_);
				util::Vector<std::unique_ptr<Utf8String>> topic_vector;
				topic_vector.push_back(std::move(p_topic_name));

				ResponseCode rc = Unsubscribe(test_packet_id_, std::move(topic_vector));
				EXPECT_EQ(ResponseCode::SUCCESS, rc);
			}

			TEST_F(SubUnsubActionTester, IncomingPublishOnSubscribedTopicTest) {
				ASSERT_NE(nullptr, p_network_connection_);
				ASSERT_NE(nullptr, p_core_state_);
				ASSERT_NE(nullptr, p_subscribe_action_);

				p_network_connection_->ClearNextReadBuf();
				p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedPublishMessage(test_topic_base_, test_packet_id_, mqtt::QoS::QOS0, false, false, test_payload_));

				p_network_connection_->last_write_buf_.clear();
				p_network_connection_->was_write_called_ = false;

				callback_received_ = false;

				std::unique_ptr<Action> p_network_read_action = mqtt::NetworkReadActionRunner::Create(p_core_state_);

				std::unique_ptr<Action> p_subscribe_action = mqtt::SubscribeActionAsync::Create(p_core_state_);
				mqtt::Subscription::ApplicationCallbackHandlerPtr p_app_handler = std::bind(&SubUnsubActionTester::SubscribeCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

				std::shared_ptr<mqtt::Subscription> p_subscription = mqtt::Subscription::Create(Utf8String::Create(test_topic_base_), mqtt::QoS::QOS1, p_app_handler, nullptr);
				util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;

				topic_vector.push_back(p_subscription);
				ResponseCode rc = Subscribe(test_packet_id_, topic_vector);
				EXPECT_EQ(ResponseCode::SUCCESS, rc);

				std::vector<uint8_t> suback_list;
				suback_list.push_back(0);
				util::String suback_packet_str = TestHelper::GetSerializedSubAckMessage(test_packet_id_, suback_list);
				util::Vector<unsigned char> suback_packet;
				for(char c : suback_packet_str) {
					suback_packet.push_back(static_cast<unsigned char>(c));
				}

				p_network_connection_->SetNextReadBuf(suback_packet_str);
				std::shared_ptr<mqtt::SubackPacket> p_suback_packet = mqtt::SubackPacket::Create(suback_packet);
				util::String deserialized_suback_str = p_suback_packet->ToString();
				EXPECT_TRUE(suback_packet_str.compare(deserialized_suback_str));

				rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
				EXPECT_EQ(ResponseCode::SUCCESS, rc);
				EXPECT_EQ(true, p_network_connection_->was_read_called_);
				EXPECT_TRUE(p_subscription->IsActive());

				cur_expected_topic_name_ = test_topic_base_;
				p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedPublishMessage(test_topic_base_, test_packet_id_, mqtt::QoS::QOS0, false, false, test_payload_));

				rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
				EXPECT_EQ(ResponseCode::SUCCESS, rc);
				EXPECT_EQ(true, p_network_connection_->was_read_called_);
				EXPECT_TRUE(callback_received_);
			}

			TEST_F(SubUnsubActionTester, IncomingLargePublishOnSubscribedTopicTest) {
				ASSERT_NE(nullptr, p_network_connection_);
				ASSERT_NE(nullptr, p_core_state_);
				ASSERT_NE(nullptr, p_subscribe_action_);

				p_network_connection_->ClearNextReadBuf();
				p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedPublishMessage(test_topic_base_, test_packet_id_, mqtt::QoS::QOS0, false, false, test_payload_));

				p_network_connection_->last_write_buf_.clear();
				p_network_connection_->was_write_called_ = false;

				callback_received_ = false;

				std::unique_ptr<Action> p_network_read_action = mqtt::NetworkReadActionRunner::Create(p_core_state_);

				std::unique_ptr<Action> p_subscribe_action = mqtt::SubscribeActionAsync::Create(p_core_state_);
				mqtt::Subscription::ApplicationCallbackHandlerPtr p_app_handler = std::bind(&SubUnsubActionTester::SubscribeCallbackLargePayload, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

				std::shared_ptr<mqtt::Subscription> p_subscription = mqtt::Subscription::Create(Utf8String::Create(test_topic_base_), mqtt::QoS::QOS1, p_app_handler, nullptr);
				util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;

				topic_vector.push_back(p_subscription);
				ResponseCode rc = Subscribe(test_packet_id_, topic_vector);
				EXPECT_EQ(ResponseCode::SUCCESS, rc);

				std::vector<uint8_t> suback_list;
				suback_list.push_back(0);
				util::String suback_packet_str = TestHelper::GetSerializedSubAckMessage(test_packet_id_, suback_list);
				util::Vector<unsigned char> suback_packet;
				for(char c : suback_packet_str) {
					suback_packet.push_back(static_cast<unsigned char>(c));
				}

				p_network_connection_->SetNextReadBuf(suback_packet_str);
				std::shared_ptr<mqtt::SubackPacket> p_suback_packet = mqtt::SubackPacket::Create(suback_packet);
				util::String deserialized_suback_str = p_suback_packet->ToString();
				EXPECT_TRUE(suback_packet_str.compare(deserialized_suback_str));

				rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
				EXPECT_EQ(ResponseCode::SUCCESS, rc);
				EXPECT_EQ(true, p_network_connection_->was_read_called_);
				EXPECT_TRUE(p_subscription->IsActive());


				char c_payload[LARGE_PAYLOAD_SIZE];

				for(int i = 0; i < LARGE_PAYLOAD_SIZE; i++) {
					c_payload[i] = 'a';
				}

				c_payload[LARGE_PAYLOAD_SIZE - 1] = '\n';
				large_test_payload_ = "Large Test Payload : ";
				large_test_payload_.append(c_payload, LARGE_PAYLOAD_SIZE);
				size_t msg_count = 0;
				do {
					cur_expected_topic_name_ = test_topic_base_;
					p_network_connection_->SetNextReadBuf(
							TestHelper::GetSerializedPublishMessage(test_topic_base_, test_packet_id_, mqtt::QoS::QOS0,
																	false, false, large_test_payload_));

					rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
					EXPECT_EQ(ResponseCode::SUCCESS, rc);
					EXPECT_EQ(true, p_network_connection_->was_read_called_);
					EXPECT_TRUE(callback_received_);
					msg_count++;
				} while(msg_count < 50);
			}

			TEST_F(SubUnsubActionTester, IncomingUnsubackOnSubscribedTopicTest) {
				ASSERT_NE(nullptr, p_network_connection_);
				ASSERT_NE(nullptr, p_core_state_);
				ASSERT_NE(nullptr, p_subscribe_action_);

				p_network_connection_->ClearNextReadBuf();
				p_network_connection_->SetNextReadBuf(TestHelper::GetSerializedPublishMessage(test_topic_base_, test_packet_id_, mqtt::QoS::QOS0, false, false, test_payload_));

				p_network_connection_->last_write_buf_.clear();
				p_network_connection_->was_write_called_ = false;

				callback_received_ = false;

				std::unique_ptr<Action> p_network_read_action = mqtt::NetworkReadActionRunner::Create(p_core_state_);

				std::unique_ptr<Action> p_subscribe_action = mqtt::SubscribeActionAsync::Create(p_core_state_);
				mqtt::Subscription::ApplicationCallbackHandlerPtr p_app_handler = std::bind(&SubUnsubActionTester::SubscribeCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

				std::shared_ptr<mqtt::Subscription> p_subscription = mqtt::Subscription::Create(Utf8String::Create(test_topic_base_), mqtt::QoS::QOS1, p_app_handler, nullptr);
				util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;

				topic_vector.push_back(p_subscription);
				ResponseCode rc = Subscribe(test_packet_id_, topic_vector);
				EXPECT_EQ(ResponseCode::SUCCESS, rc);

				std::vector<uint8_t> suback_list;
				suback_list.push_back(0);
				util::String suback_packet = TestHelper::GetSerializedSubAckMessage(test_packet_id_, suback_list);
				p_network_connection_->SetNextReadBuf(suback_packet);

				rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
				EXPECT_EQ(ResponseCode::SUCCESS, rc);
				EXPECT_EQ(true, p_network_connection_->was_read_called_);
				EXPECT_TRUE(p_subscription->IsActive());

				p_network_connection_->last_write_buf_.clear();
				p_network_connection_->was_write_called_ = false;

				std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(test_topic_base_);
				util::Vector<std::unique_ptr<Utf8String>> unsub_topic_vector;
				unsub_topic_vector.push_back(std::move(p_topic_name));

				rc = Unsubscribe(test_packet_id_, std::move(unsub_topic_vector));
				EXPECT_EQ(ResponseCode::SUCCESS, rc);

				util::String unsuback_packet_str = TestHelper::GetSerializedUnsubAckMessage(test_packet_id_);
				util::Vector<unsigned char> unsuback_packet;
				for(char c : unsuback_packet_str) {
					unsuback_packet.push_back(static_cast<unsigned char>(c));
				}
				p_network_connection_->SetNextReadBuf(unsuback_packet_str);
				std::shared_ptr<mqtt::UnsubackPacket> p_unsuback_packet = mqtt::UnsubackPacket::Create(unsuback_packet);
				util::String deserialized_unsuback_str = p_unsuback_packet->ToString();
				EXPECT_TRUE(unsuback_packet_str.compare(deserialized_unsuback_str));

				rc = p_network_read_action->PerformAction(p_network_connection_, nullptr);
				EXPECT_EQ(ResponseCode::SUCCESS, rc);
				EXPECT_EQ(true, p_network_connection_->was_read_called_);

				EXPECT_EQ(nullptr, p_core_state_->GetSubscription(test_topic_base_));
			}
		}
	}
}
