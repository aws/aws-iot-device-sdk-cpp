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
 * @file PubSub.cpp
 * @brief Sample demonstrating MQTT operations
 *
 */

#include <chrono>
#include <cstring>

#ifdef USE_WEBSOCKETS
#include "WebSocketConnection.hpp"
#elif defined USE_MBEDTLS
#include "MbedTLSConnection.hpp"
#else
#include "OpenSSLConnection.hpp"
#endif

#include "util/logging/Logging.hpp"
#include "util/logging/LogMacros.hpp"
#include "util/logging/ConsoleLogSystem.hpp"

#include "ConfigCommon.hpp"
#include "PubSub.hpp"

#define LOG_TAG_PUBSUB "[Sample - PubSub]"
#define MESSAGE_COUNT 5
#define SDK_SAMPLE_TOPIC "sdk/test/cpp"

namespace awsiotsdk {
    namespace samples {
        ResponseCode PubSub::RunPublish(int msg_count) {
            std::cout << std::endl
                      << "******************************Entering Publish with no queuing delay unless queue is full!!**************************"
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

        ResponseCode PubSub::SubscribeCallback(util::String topic_name,
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

        ResponseCode PubSub::DisconnectCallback(util::String client_id,
                                                std::shared_ptr<DisconnectCallbackContextData> p_app_handler_data) {
            std::cout << "*******************************************" << std::endl
                      << client_id << " Disconnected!" << std::endl
                      << "*******************************************" << std::endl;
            return ResponseCode::SUCCESS;
        }

        ResponseCode PubSub::Subscribe() {
            util::String p_topic_name_str = SDK_SAMPLE_TOPIC;
            std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
            mqtt::Subscription::ApplicationCallbackHandlerPtr p_sub_handler = std::bind(&PubSub::SubscribeCallback,
                                                                                        this,
                                                                                        std::placeholders::_1,
                                                                                        std::placeholders::_2,
                                                                                        std::placeholders::_3);
            std::shared_ptr<mqtt::Subscription> p_subscription =
                mqtt::Subscription::Create(std::move(p_topic_name), mqtt::QoS::QOS0, p_sub_handler, nullptr);
            util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;
            topic_vector.push_back(p_subscription);

            ResponseCode rc = p_iot_client_->Subscribe(topic_vector, ConfigCommon::mqtt_command_timeout_);
            std::this_thread::sleep_for(std::chrono::seconds(3));
            return rc;
        }

        ResponseCode PubSub::Unsubscribe() {
            util::String p_topic_name_str = SDK_SAMPLE_TOPIC;
            std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
            util::Vector<std::unique_ptr<Utf8String>> topic_vector;
            topic_vector.push_back(std::move(p_topic_name));

            ResponseCode rc = p_iot_client_->Unsubscribe(std::move(topic_vector), ConfigCommon::mqtt_command_timeout_);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return rc;
        }

        ResponseCode PubSub::InitializeTLS() {
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
                AWS_LOG_ERROR(LOG_TAG_PUBSUB, "Failed to initialize Network Connection. %s",
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
                AWS_LOG_ERROR(LOG_TAG_PUBSUB, "Failed to initialize Network Connection. %s",
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
                AWS_LOG_ERROR(LOG_TAG_PUBSUB,
                              "Failed to initialize Network Connection. %s",
                              ResponseHelper::ToString(rc).c_str());
                rc = ResponseCode::FAILURE;
            } else {
                p_network_connection_ = std::dynamic_pointer_cast<NetworkConnection>(p_network_connection);
            }
#endif
            return rc;
        }

        ResponseCode PubSub::RunSample() {
            total_published_messages_ = 0;
            cur_pending_messages_ = 0;

            ResponseCode rc = InitializeTLS();
            if (ResponseCode::SUCCESS != rc) {
                return rc;
            }

            ClientCoreState::ApplicationDisconnectCallbackPtr p_disconnect_handler =
                std::bind(&PubSub::DisconnectCallback, this, std::placeholders::_1, std::placeholders::_2);

            p_iot_client_ = std::shared_ptr<MqttClient>(MqttClient::Create(p_network_connection_,
                                                                           ConfigCommon::mqtt_command_timeout_,
                                                                           p_disconnect_handler, nullptr));
            if (nullptr == p_iot_client_) {
                return ResponseCode::FAILURE;
            }

            util::String client_id_tagged = ConfigCommon::base_client_id_;
            client_id_tagged.append("_pub_sub_tester_");
            client_id_tagged.append(std::to_string(rand()));
            std::unique_ptr<Utf8String> client_id = Utf8String::Create(client_id_tagged);

            rc = p_iot_client_->Connect(ConfigCommon::mqtt_command_timeout_, ConfigCommon::is_clean_session_,
                                        mqtt::Version::MQTT_3_1_1, ConfigCommon::keep_alive_timeout_secs_,
                                        std::move(client_id), nullptr, nullptr, nullptr);
            if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED != rc) {
                return rc;
            }

            rc = Subscribe();
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(LOG_TAG_PUBSUB, "Subscribe failed. %s", ResponseHelper::ToString(rc).c_str());
            } else {
                // Test with delay between each action being queued up
                rc = RunPublish(MESSAGE_COUNT);
                if (ResponseCode::SUCCESS != rc) {
                    std::cout << std::endl << "Publish runner failed. " << ResponseHelper::ToString(rc) << std::endl;
                    AWS_LOG_ERROR(LOG_TAG_PUBSUB, "Publish runner failed. %s",
                                  ResponseHelper::ToString(rc).c_str());
                    p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                }

                std::cout << ResponseHelper::ToString(rc) << std::endl;
                if (ResponseCode::SUCCESS == rc) {
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
                }

                do {
                    rc = Unsubscribe();
                    if (ResponseCode::ACTION_QUEUE_FULL == rc) {
                        std::cout << "Message queue full on Unsub, waiting!!!" << std::endl;
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    }
                } while (ResponseCode::ACTION_QUEUE_FULL == rc);
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(LOG_TAG_PUBSUB, "Unsubscribe failed. %s",
                                  ResponseHelper::ToString(rc).c_str());
                }
            }

            rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(LOG_TAG_PUBSUB, "Disconnect failed. %s", ResponseHelper::ToString(rc).c_str());
            }

            std::cout << std::endl << "*************************Results**************************" << std::endl;
            std::cout << "Pending published messages : " << cur_pending_messages_ << std::endl;
            std::cout << "Total published messages : " << total_published_messages_ << std::endl;
            std::cout << "Exiting Sample!!!!" << std::endl;
            return ResponseCode::SUCCESS;
        }
    }
}

int main(int argc, char **argv) {
    std::shared_ptr<awsiotsdk::util::Logging::ConsoleLogSystem> p_log_system =
        std::make_shared<awsiotsdk::util::Logging::ConsoleLogSystem>(awsiotsdk::util::Logging::LogLevel::Info);
    awsiotsdk::util::Logging::InitializeAWSLogging(p_log_system);

    std::unique_ptr<awsiotsdk::samples::PubSub>
        pub_sub = std::unique_ptr<awsiotsdk::samples::PubSub>(new awsiotsdk::samples::PubSub());

    awsiotsdk::ResponseCode rc = awsiotsdk::ConfigCommon::InitializeCommon("config/SampleConfig.json");
    if (awsiotsdk::ResponseCode::SUCCESS == rc) {
        rc = pub_sub->RunSample();
    }
#ifdef WIN32
    std::cout<<"Press any key to continue!!!!"<<std::endl;
    getchar();
#endif

    awsiotsdk::util::Logging::ShutdownAWSLogging();
    return static_cast<int>(rc);
}
