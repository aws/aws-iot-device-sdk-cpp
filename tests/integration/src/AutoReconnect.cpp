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
 * @file AutoReconnect.cpp
 * @brief
 *
 */

#include "AutoReconnect.hpp"
#include "util/logging/LogMacros.hpp"

#include <cstring>
#include <chrono>

#ifdef USE_WEBSOCKETS
#include "WebSocketConnection.hpp"
#elif defined USE_MBEDTLS
#include "MbedTLSConnection.hpp"
#else
#include "OpenSSLConnection.hpp"
#endif

#ifdef WIN32
#define MAX_PATH_LENGTH_ 260
#include <direct.h>
#define getcwd _getcwd // avoid MSFT "deprecation" warning
#else
#include <limits>
#define MAX_PATH_LENGTH_ PATH_MAX
#endif

#include "ConfigCommon.hpp"

#define ARC_INTEGRATION_TEST_TAG "[Integration Test - Autoreconnect]"
#define SDK_SAMPLE_TOPIC "SdkTest/TestTopic"
#define AWS_LIFECYCLE_EVENT_TOPIC_BASE "$aws/events/subscriptions/subscribed/"
#define SDK_ACR_TEST_MSG_COUNT 5
#define NETWORK_RECONNECT_BACKOFF_TIMER_MIN 1
#define NETWORK_RECONNECT_BACKOFF_TIMER_MAX 64

namespace awsiotsdk {
    namespace tests {
        namespace integration {
            ResponseCode AutoReconnect::RunPublish(int msg_count) {
                std::cout << std::endl << "******************************Entering Publish!!**************************"
                          << std::endl;
                ResponseCode rc;
                uint16_t packet_id = 0;
                int itr = 1;

                util::String p_topic_name_str = SDK_SAMPLE_TOPIC;

                do {
                    util::String payload = "Hello from SDK : ";
                    payload.append(std::to_string(itr));
                    std::cout << "Publish Payload : " << payload << std::endl;

                    std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
                    rc = p_iot_client_->PublishAsync(std::move(p_topic_name), false, false, mqtt::QoS::QOS1,
                                                     payload, nullptr, packet_id);
                    if (ResponseCode::SUCCESS == rc) {
                        cur_pending_messages_++;
                        total_published_messages_++;
                        std::cout << "Publish Packet Id : " << packet_id << std::endl;
                    } else if (ResponseCode::ACTION_QUEUE_FULL == rc) {
                        itr--;
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    }
                } while (++itr <= msg_count && (ResponseCode::SUCCESS == rc || ResponseCode::ACTION_QUEUE_FULL == rc));

                return rc;
            }

            ResponseCode AutoReconnect::SubscribeCallback(util::String topic_name,
                                                          util::String payload,
                                                          std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
                std::cout << std::endl << "************" << std::endl;
                std::cout << "Received message on topic : " << topic_name << std::endl;
                std::cout << "Payload Length : " << payload.length() << std::endl;
                if (payload.length() < 50) {
                    std::cout << "Payload : " << payload << std::endl;
                }
                std::cout << std::endl << "************" << std::endl;
                cur_pending_messages_--;
                return ResponseCode::SUCCESS;
            }

            ResponseCode AutoReconnect::LifecycleSubscribeCallback(util::String topic_name,
                                                                   util::String payload,
                                                                   std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
                std::lock_guard<std::mutex> block_handler_lock(waiting_for_sub_lock_);
                std::cout << "Received Lifecycle event for topic " << topic_name << "!!" << std::endl;
                sub_lifecycle_wait_.notify_all();
                return ResponseCode::SUCCESS;
            }

            ResponseCode AutoReconnect::Subscribe() {
                util::String p_topic_name_str = SDK_SAMPLE_TOPIC;
                util::String p_lifecycle_topic_name_str = AWS_LIFECYCLE_EVENT_TOPIC_BASE;
                p_lifecycle_topic_name_str.append(client_id_tagged_);
                util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;

                std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
                std::unique_ptr<Utf8String> p_lifecycle_topic_name = Utf8String::Create(p_lifecycle_topic_name_str);

                mqtt::Subscription::ApplicationCallbackHandlerPtr p_sub_handler =
                    std::bind(&AutoReconnect::SubscribeCallback,
                              this,
                              std::placeholders::_1,
                              std::placeholders::_2,
                              std::placeholders::_3);
                mqtt::Subscription::ApplicationCallbackHandlerPtr p_lifecycle_sub_handler =
                    std::bind(&AutoReconnect::LifecycleSubscribeCallback,
                              this,
                              std::placeholders::_1,
                              std::placeholders::_2,
                              std::placeholders::_3);

                std::shared_ptr<mqtt::Subscription> p_subscription =
                    mqtt::Subscription::Create(std::move(p_topic_name), mqtt::QoS::QOS0, p_sub_handler, nullptr);
                topic_vector.push_back(p_subscription);
                std::shared_ptr<mqtt::Subscription> p_lifecycle_subscription =
                    mqtt::Subscription::Create(std::move(p_lifecycle_topic_name),
                                               mqtt::QoS::QOS0,
                                               p_lifecycle_sub_handler,
                                               nullptr);
                topic_vector.push_back(p_lifecycle_subscription);

                ResponseCode rc;
                {
                    std::unique_lock<std::mutex> block_handler_lock(waiting_for_sub_lock_);
                    {
                        rc = p_iot_client_->Subscribe(topic_vector, ConfigCommon::mqtt_command_timeout_);
                    }

                    // Wait for 30 secs for subscribe to finish (shouldn't take longer on a good network connection)
                    sub_lifecycle_wait_.wait_for(block_handler_lock, std::chrono::seconds(30));
                }
                return rc;
            }

            ResponseCode AutoReconnect::Unsubscribe() {
                uint16_t packet_id = 0;

                util::String p_topic_name_str = SDK_SAMPLE_TOPIC;
                util::String p_lifecycle_topic_name_str = AWS_LIFECYCLE_EVENT_TOPIC_BASE;
                p_lifecycle_topic_name_str.append(client_id_tagged_);

                std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
                std::unique_ptr<Utf8String> p_lifecycle_topic_name = Utf8String::Create(p_lifecycle_topic_name_str);

                util::Vector<std::unique_ptr<Utf8String>> topic_vector;
                topic_vector.push_back(std::move(p_topic_name));
                topic_vector.push_back(std::move(p_lifecycle_topic_name));

                ResponseCode rc = p_iot_client_->UnsubscribeAsync(std::move(topic_vector), nullptr, packet_id);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                return rc;
            }
            ResponseCode AutoReconnect::InitializeTLS() {
                ResponseCode rc = ResponseCode::SUCCESS;

#ifdef USE_WEBSOCKETS
                p_network_connection_ = std::shared_ptr<NetworkConnection>(
                    new network::WebSocketConnection(ConfigCommon::endpoint_, ConfigCommon::endpoint_https_port_,
                                                     ConfigCommon::root_ca_path_, ConfigCommon::aws_region_,
                                                     ConfigCommon::aws_access_key_id_,
                                                     ConfigCommon::aws_secret_access_key_,
                                                     ConfigCommon::aws_session_token_,
                                                     ConfigCommon::tls_handshake_timeout_,
                                                     ConfigCommon::tls_read_timeout_,
                                                     ConfigCommon::tls_write_timeout_, true));
                if (nullptr == p_network_connection_) {
                    AWS_LOG_ERROR(ARC_INTEGRATION_TEST_TAG, "Failed to initialize Network Connection. %s",
                                  ResponseHelper::ToString(rc).c_str());
                    rc = ResponseCode::FAILURE;
                }
#elif defined USE_MBEDTLS
                p_network_connection_ = std::make_shared<network::MbedTLSConnection>(ConfigCommon::endpoint_,
                                                                                     ConfigCommon::endpoint_mqtt_port_,
                                                                                     ConfigCommon::root_ca_path_,
                                                                                     ConfigCommon::client_cert_path_,
                                                                                     ConfigCommon::client_key_path_,
                                                                                     ConfigCommon::tls_handshake_timeout_,
                                                                                     ConfigCommon::tls_read_timeout_,
                                                                                     ConfigCommon::tls_write_timeout_,
                                                                                     true);
                if (nullptr == p_network_connection_) {
                    AWS_LOG_ERROR(ARC_INTEGRATION_TEST_TAG, "Failed to initialize Network Connection. %s",
                                  ResponseHelper::ToString(rc).c_str());
                    rc = ResponseCode::FAILURE;
                }
#else
                std::shared_ptr<network::OpenSSLConnection> p_network_connection =
                    std::make_shared<network::OpenSSLConnection>(ConfigCommon::endpoint_,
                                                                 ConfigCommon::endpoint_mqtt_port_,
                                                                 ConfigCommon::root_ca_path_,
                                                                 ConfigCommon::client_cert_path_,
                                                                 ConfigCommon::client_key_path_,
                                                                 ConfigCommon::tls_handshake_timeout_,
                                                                 ConfigCommon::tls_read_timeout_,
                                                                 ConfigCommon::tls_write_timeout_, true);
                rc = p_network_connection->Initialize();

                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(ARC_INTEGRATION_TEST_TAG, "Failed to initialize Network Connection. %s",
                                  ResponseHelper::ToString(rc).c_str());
                    rc = ResponseCode::FAILURE;
                } else {
                    p_network_connection_ = std::dynamic_pointer_cast<NetworkConnection>(p_network_connection);
                }
#endif
                return rc;
            }

            ResponseCode AutoReconnect::RunTest() {
                bool ran_all_tests = false;
                total_published_messages_ = 0;
                cur_pending_messages_ = 0;
                ResponseCode rc = InitializeTLS();

                do {
                    if (ResponseCode::SUCCESS != rc) {
                        AWS_LOG_ERROR(ARC_INTEGRATION_TEST_TAG, "Failed to initialize TLS layer. %s",
                                      ResponseHelper::ToString(rc).c_str());
                        break;
                    }

                    p_iot_client_ = std::shared_ptr<MqttClient>(
                        MqttClient::Create(p_network_connection_, ConfigCommon::mqtt_command_timeout_));
                    if (nullptr == p_iot_client_) {
                        AWS_LOG_ERROR(ARC_INTEGRATION_TEST_TAG, "Failed to create MQTT Client Instance!!");
                        rc = ResponseCode::FAILURE;
                        break;
                    }

                    p_iot_client_->SetAutoReconnectEnabled(true);
                    client_id_tagged_ = ConfigCommon::base_client_id_;
                    client_id_tagged_.append("_autoreconnect_tester_");
                    client_id_tagged_.append(std::to_string(rand()));
                    std::unique_ptr<Utf8String> client_id = Utf8String::Create(client_id_tagged_);

                    rc = p_iot_client_->Connect(ConfigCommon::mqtt_command_timeout_, ConfigCommon::is_clean_session_,
                                                mqtt::Version::MQTT_3_1_1, ConfigCommon::keep_alive_timeout_secs_,
                                                std::move(client_id), nullptr, nullptr, nullptr);
                    if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED != rc) {
                        AWS_LOG_ERROR(ARC_INTEGRATION_TEST_TAG, "MQTT Connect failed. %s",
                                      ResponseHelper::ToString(rc).c_str());
                        return rc;
                    }

                    rc = Subscribe();
                    if (ResponseCode::SUCCESS != rc) {
                        AWS_LOG_ERROR(ARC_INTEGRATION_TEST_TAG, "Subscribe failed. %s",
                                      ResponseHelper::ToString(rc).c_str());
                        p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                        break;
                    }
                    // Test with delay between each action being queued up
                    rc = RunPublish(SDK_ACR_TEST_MSG_COUNT);
                    if (ResponseCode::SUCCESS != rc) {
                        AWS_LOG_ERROR(ARC_INTEGRATION_TEST_TAG, "Publish runner failed. %s",
                                      ResponseHelper::ToString(rc).c_str());
                        p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                        break;
                    }

                    //Sleep for 10 seconds and wait for all messages to be received
                    int cur_sleep_sec_count = 0;
                    do {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        if (0 == cur_pending_messages_) {
                            break;
                        }
                        cur_sleep_sec_count++;
                        std::cout << "Waiting!!! " << cur_sleep_sec_count << std::endl;
                    } while (cur_sleep_sec_count < 100);

                    {
                        std::unique_lock<std::mutex> block_handler_lock(waiting_for_sub_lock_);
                        {
                            p_iot_client_->SetMinReconnectBackoffTimeout(std::chrono::seconds(
                                NETWORK_RECONNECT_BACKOFF_TIMER_MIN));
                            p_iot_client_->SetMaxReconnectBackoffTimeout(std::chrono::seconds(
                                NETWORK_RECONNECT_BACKOFF_TIMER_MAX));
                            std::cout << "************************Simulating Disconnect*********************************"
                                      << std::endl << std::endl;
                            rc = p_network_connection_->Disconnect();
                            IOT_UNUSED(rc);
                        }

                        std::cout << "************************Wait for resubscribe!!*********************************"
                                  << std::endl;
                        // Wait for resubscribe to finish
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        sub_lifecycle_wait_.wait_for(block_handler_lock,
                                                     std::chrono::seconds(NETWORK_RECONNECT_BACKOFF_TIMER_MAX * 2));
                    }

                    if (p_iot_client_->IsConnected()) {
                        // Test with delay between each action being queued up
                        rc = RunPublish(SDK_ACR_TEST_MSG_COUNT);
                        if (ResponseCode::SUCCESS != rc) {
                            AWS_LOG_ERROR(ARC_INTEGRATION_TEST_TAG, "Publish runner failed. %s",
                                          ResponseHelper::ToString(rc).c_str());
                            p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                            break;
                        }

                        //Sleep for 10 seconds and wait for all messages to be received
                        cur_sleep_sec_count = 0;
                        do {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            if (0 == cur_pending_messages_) {
                                break;
                            }
                            cur_sleep_sec_count++;
                            std::cout << "Waiting!!! " << cur_sleep_sec_count << std::endl;
                        } while (cur_sleep_sec_count < 100);

                        do {
                            rc = Unsubscribe();
                            if (ResponseCode::ACTION_QUEUE_FULL == rc) {
                                std::cout << "Message queue full on Unsub, waiting!!!" << std::endl;
                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                            }
                        } while (ResponseCode::ACTION_QUEUE_FULL == rc);
                        if (ResponseCode::SUCCESS != rc) {
                            AWS_LOG_ERROR(ARC_INTEGRATION_TEST_TAG, "Unsubscribe failed. %s",
                                          ResponseHelper::ToString(rc).c_str());
                            p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                            break;
                        }
                    }

                    rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                    if (ResponseCode::SUCCESS != rc) {
                        AWS_LOG_ERROR(ARC_INTEGRATION_TEST_TAG, "Disconnect failed. %s",
                                      ResponseHelper::ToString(rc).c_str());
                        break;
                    }
                    ran_all_tests = true;
                } while (false);

                std::cout << std::endl << "*************************Results**************************" << std::endl;
                std::cout << "Pending published messages : " << cur_pending_messages_ << std::endl;
                std::cout << "Total published messages : " << total_published_messages_ << std::endl;
                if ((ResponseCode::FAILURE == rc) || !ran_all_tests
                    || (cur_pending_messages_ > 0 || total_published_messages_ == 0)) {
                    std::cout << "Test Failed!!!! See above output for details!!" << std::endl;
                    std::cout << "**********************************************************" << std::endl;
                    return ResponseCode::FAILURE;
                }

                std::cout << "Test Successful!!!!" << std::endl;
                std::cout << "**********************************************************" << std::endl;
                return ResponseCode::SUCCESS;
            }
        }
    }
}
