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
 * @file Discovery.cpp
 * @brief Sample demonstrating MQTT operations
 *
 */

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>

#include "OpenSSLConnection.hpp"

#include "util/logging/Logging.hpp"
#include "util/logging/LogMacros.hpp"
#include "util/logging/ConsoleLogSystem.hpp"

#include "ConfigCommon.hpp"
#include "Discovery.hpp"

#define LOG_TAG_DISCOVERY_SAMPLE "[Sample - Discovery]"
#define MESSAGE_COUNT 5
#define SDK_SAMPLE_TOPIC "sdk/test/cpp"

namespace awsiotsdk {
    namespace samples {
        ResponseCode Discovery::RunPublish(int msg_count) {
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
                rc = p_iot_client_->PublishAsync(std::move(p_topic_name), false, false, mqtt::QoS::QOS0,
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

        ResponseCode Discovery::SubscribeCallback(util::String topic_name,
                                                  util::String payload,
                                                  std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
            std::cout << std::endl << "************" << std::endl;
            std::cout << "Received message on topic : " << topic_name << std::endl;
            std::cout << "Payload Length : " << payload.length() << std::endl;
            std::cout << std::endl << "************" << std::endl;
            cur_pending_messages_--;
            return ResponseCode::SUCCESS;
        }

        ResponseCode Discovery::Subscribe() {
            util::String p_topic_name_str = SDK_SAMPLE_TOPIC;
            std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
            mqtt::Subscription::ApplicationCallbackHandlerPtr p_sub_handler = std::bind(&Discovery::SubscribeCallback,
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

        ResponseCode Discovery::Unsubscribe() {
            util::String p_topic_name_str = SDK_SAMPLE_TOPIC;
            std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
            util::Vector<std::unique_ptr<Utf8String>> topic_vector;
            topic_vector.push_back(std::move(p_topic_name));

            ResponseCode rc = p_iot_client_->Unsubscribe(std::move(topic_vector), ConfigCommon::mqtt_command_timeout_);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return rc;
        }

        // Example of a comparison function that sorts connectivity information in ascending order of ID
        bool Discovery::ConnectivitySortFunction(ConnectivityInfo info1, ConnectivityInfo info2) {
            if (0 > info1.id_.compare(info2.id_)) {
                return true;
            }
            return false;
        }

        ResponseCode Discovery::RunSample() {
            total_published_messages_ = 0;
            cur_pending_messages_ = 0;

            // Creating OpenSSL connection to perform Discovery Operation
            std::shared_ptr<network::OpenSSLConnection> p_openssl_connection =
                std::make_shared<network::OpenSSLConnection>(ConfigCommon::endpoint_,
                                                             ConfigCommon::endpoint_greengrass_discovery_port_,
                                                             ConfigCommon::root_ca_path_,
                                                             ConfigCommon::client_cert_path_,
                                                             ConfigCommon::client_key_path_,
                                                             ConfigCommon::tls_handshake_timeout_,
                                                             ConfigCommon::tls_read_timeout_,
                                                             ConfigCommon::tls_write_timeout_, true);
            ResponseCode rc = p_openssl_connection->Initialize();

            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(LOG_TAG_DISCOVERY_SAMPLE,
                              "Failed to initialize Network Connection. %s",
                              ResponseHelper::ToString(rc).c_str());
                rc = ResponseCode::FAILURE;
                return rc;
            } else {
                p_network_connection_ = std::dynamic_pointer_cast<NetworkConnection>(p_openssl_connection);
            }

            p_iot_client_ = std::shared_ptr<GreengrassMqttClient>(GreengrassMqttClient::Create(p_network_connection_,
                                                                                               ConfigCommon::mqtt_command_timeout_));
            if (nullptr == p_iot_client_) {
                return ResponseCode::FAILURE;
            }

            std::unique_ptr<Utf8String> p_thing_name = Utf8String::Create(ConfigCommon::thing_name_);

            DiscoveryResponse discovery_response;

            // Perform discovery operation for thing name.
            rc = p_iot_client_->Discover(std::chrono::milliseconds(ConfigCommon::discover_action_timeout_),
                                         std::move(p_thing_name), discovery_response);
            if (ResponseCode::DISCOVER_ACTION_SUCCESS != rc) {
                if (ResponseCode::DISCOVER_ACTION_NO_INFORMATION_PRESENT != rc) {
                    AWS_LOG_INFO(LOG_TAG_DISCOVERY_SAMPLE,
                                 "Discover Request failed. %s",
                                 ResponseHelper::ToString(rc).c_str());
                    return rc;
                }
                AWS_LOG_INFO(LOG_TAG_DISCOVERY_SAMPLE, "No GGC connectivity information present for this Device");
                return rc;
            } else {
                AWS_LOG_INFO(LOG_TAG_DISCOVERY_SAMPLE, "GGC connectivity information found for this Device!!\n");

                util::String current_working_directory = ConfigCommon::GetCurrentPath();
#ifdef WIN32
                current_working_directory.append("\\");
#else
                current_working_directory.append("/");
#endif

                // Write complete Discovery Response JSON out to a file
                util::String discovery_response_output_path = current_working_directory;
                discovery_response_output_path.append("discovery_output.json");
                rc = discovery_response.WriteToPath(discovery_response_output_path);

                // Get vector of connectivity information and map of CA certificates
                util::Vector<ConnectivityInfo> parsed_response;
                util::Map<util::String, util::Vector<util::String>> ca_map;
                rc = discovery_response.GetParsedResponse(parsed_response, ca_map);

                // Sort all the connectivity information in ascending order of ID
                std::sort(parsed_response.begin(), parsed_response.end(), std::bind(Discovery::ConnectivitySortFunction,
                                                                                    std::placeholders::_1,
                                                                                    std::placeholders::_2));

                // Store all the certificates using group names for the certificate names
                for (auto ca_map_itr: ca_map) {
                    util::String ca_output_path_base = current_working_directory;
                    ca_output_path_base.append(ca_map_itr.first);
                    ca_output_path_base.append("_root_ca");
                    int suffix_itr = 1;
                    for (auto ca_list_itr: ca_map_itr.second) {
                        util::String ca_output_path = ca_output_path_base;
                        ca_output_path.append(std::to_string(suffix_itr));
                        ca_output_path.append(".pem");
                        std::ofstream ca_output_stream(ca_output_path, std::ios::out | std::ios::trunc);
                        ca_output_stream << ca_list_itr;
                        suffix_itr++;
                    }
                }

                // Attempt connecting to any of the endpoint in the vector
                for (auto connectivity_info_itr : parsed_response) {

                    // Update the host address and endpoint of the OpenSSL connection instance.
                    p_openssl_connection->SetEndpointAndPort(connectivity_info_itr.host_address_,
                                                             connectivity_info_itr.port_);

                    auto ca_map_itr = ca_map.find(connectivity_info_itr.group_name_);

                    util::String ca_output_path_base = current_working_directory;
                    ca_output_path_base.append(connectivity_info_itr.group_name_);
                    ca_output_path_base.append("_root_ca");
                    int suffix_itr = 1;

                    AWS_LOG_INFO(LOG_TAG_DISCOVERY_SAMPLE,
                                 "Attempting Connect with:\nGGC Endpoint ID: %s\nGGC Endpoint : %s\nGGC Endpoint Port : %u\n",
                                 connectivity_info_itr.id_.c_str(),
                                 connectivity_info_itr.host_address_.c_str(),
                                 connectivity_info_itr.port_);

                    // Iterate through list of root CAs for that group and attempt connecting to the GGC
                    // using any one of them
                    for (auto ca_list_itr: ca_map_itr->second) {
                        util::String core_ca_file_path = ca_output_path_base;
                        core_ca_file_path.append(std::to_string(suffix_itr));
                        core_ca_file_path.append(".pem");

                        // Update the path of the root CA used by the OpenSSL connection
                        p_openssl_connection->SetRootCAPath(core_ca_file_path);

                        AWS_LOG_INFO(LOG_TAG_DISCOVERY_SAMPLE, "Using CA at : %s\n", core_ca_file_path.c_str());

                        std::unique_ptr<Utf8String> p_client_id = Utf8String::Create(ConfigCommon::base_client_id_);

                        rc = p_iot_client_->Connect(ConfigCommon::mqtt_command_timeout_,
                                                    ConfigCommon::is_clean_session_, mqtt::Version::MQTT_3_1_1,
                                                    ConfigCommon::keep_alive_timeout_secs_, std::move(p_client_id),
                                                    nullptr, nullptr, nullptr);
                        if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED == rc) {
                            break;
                        }

                        // If connection is not successful, attempt connection with the next root CA in the list
                        AWS_LOG_INFO(LOG_TAG_DISCOVERY_SAMPLE, "Connect attempt failed with this CA!!");
                        suffix_itr++;
                    }

                    // If connection is successful, break and continue with the rest of the application
                    if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED == rc) {
                        AWS_LOG_INFO(LOG_TAG_DISCOVERY_SAMPLE, "Connected to GGC %s in Group %s!!",
                                     connectivity_info_itr.ggc_name_.c_str(),
                                     connectivity_info_itr.group_name_.c_str());
                        break;
                    }

                    // If connection is not successful, attempt connecting with the next endpoint and port in the vector
                    AWS_LOG_INFO(LOG_TAG_DISCOVERY_SAMPLE, "Connect attempt failed for GGC %s in Group %s!!",
                                 connectivity_info_itr.ggc_name_.c_str(),
                                 connectivity_info_itr.group_name_.c_str());
                }
            }

            // If unable to connect to any of the endpoints, then exit
            if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED != rc) {
                return rc;
            }

            rc = Subscribe();
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(LOG_TAG_DISCOVERY_SAMPLE, "Subscribe failed. %s",
                              ResponseHelper::ToString(rc).c_str());
            } else {
                // Test with delay between each action being queued up
                rc = RunPublish(MESSAGE_COUNT);
                if (ResponseCode::SUCCESS != rc) {
                    std::cout << std::endl << "Publish runner failed. " << ResponseHelper::ToString(rc).c_str()
                              << std::endl;
                    AWS_LOG_ERROR(LOG_TAG_DISCOVERY_SAMPLE, "Publish runner failed. %s",
                                  ResponseHelper::ToString(rc).c_str());
                    p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                }

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
                    AWS_LOG_ERROR(LOG_TAG_DISCOVERY_SAMPLE, "Unsubscribe failed. %s",
                                  ResponseHelper::ToString(rc).c_str());
                }
            }

            rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(LOG_TAG_DISCOVERY_SAMPLE, "Disconnect failed. %s", ResponseHelper::ToString(rc).c_str());
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

    std::unique_ptr<awsiotsdk::samples::Discovery>
        pub_sub = std::unique_ptr<awsiotsdk::samples::Discovery>(new awsiotsdk::samples::Discovery());

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

