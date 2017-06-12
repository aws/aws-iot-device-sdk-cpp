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
 * @file switch.cpp
 * @brief Sample demonstrating connecting to a Greengrass core using discovery, interacts with RobotArm.cpp
 *
 */

#include <chrono>
#include <cstring>
#include <fstream>
#include <algorithm>

#include "OpenSSLConnection.hpp"

#include "util/logging/Logging.hpp"
#include "util/logging/LogMacros.hpp"
#include "util/logging/ConsoleLogSystem.hpp"

#include "ConfigCommon.hpp"
#include "switch.hpp"

#define DISCOVER_ACTION_RETRY_COUNT 10

#define THING_NAME_TO_UPDATE "RobotArm_Thing"

#define LOG_TAG_SWITCH_SAMPLE "[Sample - Switch]"

#define SHADOW_DOCUMENT_STATE_KEY "state"
#define SHADOW_DOCUMENT_DESIRED_KEY "desired"
#define STATE_KEY "myState"

#define SHADOW_TOPIC_PREFIX "$aws/things/"
#define SHADOW_TOPIC_MIDDLE "/shadow/"
#define SHADOW_REQUEST_TYPE_UPDATE_STRING "update"

#define SHADOW_MYSTATE_VALUE_ON "on"
#define SHADOW_MYSTATE_VALUE_OFF "off"

#define SHADOW_DOCUMENT_EMPTY_STRING "{" \
"    \"state\" : {" \
"        \"desired\" : {" \
"        	\"myState\" : \"off\"" \
"        }" \
"    }" \
"}"

namespace awsiotsdk {
    namespace samples {
        bool SwitchThing::ConnectivitySortFunction(ConnectivityInfo info1, ConnectivityInfo info2) {
            if (0 > info1.id_.compare(info2.id_)) {
                return true;
            }
            return false;
        }

        ResponseCode SwitchThing::RunSample() {
            ResponseCode rc = ResponseCode::SUCCESS;

            std::shared_ptr <network::OpenSSLConnection> p_openssl_connection =
                std::make_shared<network::OpenSSLConnection>(ConfigCommon::endpoint_,
                                                             ConfigCommon::endpoint_greengrass_discovery_port_,
                                                             ConfigCommon::root_ca_path_,
                                                             ConfigCommon::client_cert_path_,
                                                             ConfigCommon::client_key_path_,
                                                             ConfigCommon::tls_handshake_timeout_,
                                                             ConfigCommon::tls_read_timeout_,
                                                             ConfigCommon::tls_write_timeout_, true);
            rc = p_openssl_connection->Initialize();

            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(LOG_TAG_SWITCH_SAMPLE,
                              "Failed to initialize Network Connection with rc : %d",
                              static_cast<int>(rc));
                rc = ResponseCode::FAILURE;
            } else {
                p_network_connection_ = std::dynamic_pointer_cast<NetworkConnection>(p_openssl_connection);
            }

            // Run discovery to find Greengrass core endpoint to connect to
            p_iot_client_ = std::shared_ptr<GreengrassMqttClient>(GreengrassMqttClient::Create(p_network_connection_,
                                                                                               ConfigCommon::mqtt_command_timeout_));
            if (nullptr == p_iot_client_) {
                return ResponseCode::FAILURE;
            }

            DiscoveryResponse discovery_response;
            int max_retries = 0;

            do {
                std::unique_ptr <Utf8String> p_thing_name = Utf8String::Create(ConfigCommon::thing_name_);
                rc = p_iot_client_->Discover(std::chrono::milliseconds(ConfigCommon::discover_action_timeout_),
                                             std::move(p_thing_name), discovery_response);
                if (rc != ResponseCode::DISCOVER_ACTION_SUCCESS) {
                    max_retries++;
                    if (ResponseCode::DISCOVER_ACTION_NO_INFORMATION_PRESENT != rc) {
                        AWS_LOG_INFO(LOG_TAG_SWITCH_SAMPLE,
                                     "Discover Request failed with response code : %d.  Trying again...",
                                     static_cast<int>(rc));
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                    } else {
                        AWS_LOG_INFO(LOG_TAG_SWITCH_SAMPLE, "No GGC connectivity information present for this Device");
                        return rc;
                    }
                }
            } while (max_retries != DISCOVER_ACTION_RETRY_COUNT && rc != ResponseCode::DISCOVER_ACTION_SUCCESS);

            if (max_retries == DISCOVER_ACTION_RETRY_COUNT) {
                AWS_LOG_INFO(LOG_TAG_SWITCH_SAMPLE, "Discover failed after max retries, exiting");
                return rc;
            }

            AWS_LOG_INFO(LOG_TAG_SWITCH_SAMPLE, "GGC connectivity information found for this Device!!\n");

            util::String current_working_directory = ConfigCommon::GetCurrentPath();

#ifdef WIN32
            current_working_directory.append("\\");
#else
            current_working_directory.append("/");
#endif

            util::String discovery_response_output_path = current_working_directory;
            discovery_response_output_path.append("discovery_output.json");
            rc = discovery_response.WriteToPath(discovery_response_output_path);

            util::Vector <ConnectivityInfo> parsed_response;
            util::Map <util::String, util::Vector<util::String>> ca_map;
            rc = discovery_response.GetParsedResponse(parsed_response, ca_map);

            // sorting in ascending order of endpoints wrt ID
            std::sort(parsed_response.begin(), parsed_response.end(), std::bind(SwitchThing::ConnectivitySortFunction,
                                                                                std::placeholders::_1,
                                                                                std::placeholders::_2));

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

            for (auto connectivity_info_itr : parsed_response) {
                p_openssl_connection->SetEndpointAndPort(connectivity_info_itr.host_address_,
                                                         connectivity_info_itr.port_);

                auto ca_map_itr = ca_map.find(connectivity_info_itr.group_name_);

                util::String ca_output_path_base = current_working_directory;
                ca_output_path_base.append(connectivity_info_itr.group_name_);
                ca_output_path_base.append("_root_ca");
                int suffix_itr = 1;

                AWS_LOG_INFO(LOG_TAG_SWITCH_SAMPLE,
                             "Attempting Connect with:\nGGC Endpoint : %s\nGGC Endpoint Port : %u\n",
                             connectivity_info_itr.host_address_.c_str(), connectivity_info_itr.port_);

                for (auto ca_list_itr: ca_map_itr->second) {
                    util::String core_ca_file_path = ca_output_path_base;
                    core_ca_file_path.append(std::to_string(suffix_itr));
                    core_ca_file_path.append(".pem");
                    p_openssl_connection->SetRootCAPath(core_ca_file_path);

                    AWS_LOG_INFO(LOG_TAG_SWITCH_SAMPLE, "Using CA at : %s\n", core_ca_file_path.c_str());

                    std::unique_ptr <Utf8String> p_client_id = Utf8String::Create(ConfigCommon::base_client_id_);

                    rc = p_iot_client_->Connect(ConfigCommon::mqtt_command_timeout_,
                                                ConfigCommon::is_clean_session_, mqtt::Version::MQTT_3_1_1,
                                                ConfigCommon::keep_alive_timeout_secs_, std::move(p_client_id),
                                                nullptr, nullptr, nullptr);
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED == rc) {
                        break;
                    }
                    AWS_LOG_INFO(LOG_TAG_SWITCH_SAMPLE, "Connect attempt failed with this CA!!");
                    suffix_itr++;
                }
                if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED == rc) {
                    AWS_LOG_INFO(LOG_TAG_SWITCH_SAMPLE, "Connected to GGC %s in Group %s!!",
                                 connectivity_info_itr.ggc_name_.c_str(),
                                 connectivity_info_itr.group_name_.c_str());
                    break;
                }
                AWS_LOG_INFO(LOG_TAG_SWITCH_SAMPLE, "Connect attempt failed for GGC %s in Group %s!!",
                             connectivity_info_itr.ggc_name_.c_str(),
                             connectivity_info_itr.group_name_.c_str());
            }

            if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED != rc) {
                return rc;
            }

            // Document for JSON messages
            util::JsonDocument doc;

            // Build topic for publishing
            util::String update_topic = SHADOW_TOPIC_PREFIX;
            update_topic.append(THING_NAME_TO_UPDATE);
            update_topic.append(SHADOW_TOPIC_MIDDLE);
            update_topic.append(SHADOW_REQUEST_TYPE_UPDATE_STRING);
            // Rapidjson uses move semantics, doc needs to be initialized again
            rc = util::JsonParser::InitializeFromJsonString(doc, SHADOW_DOCUMENT_EMPTY_STRING);
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(LOG_TAG_SWITCH_SAMPLE, "Json Parse for sample failed with return code : %d",
                              static_cast<int>(rc));
                rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                std::cout << "Exiting Sample!!!!" << std::endl;
                return rc;
            }

            std::string userInput;
            char userInputChar = {0};

            while (true) {
                while (true) {
                    std::cout
                        << "\nPlease enter 1 (turn on) or 0 (turn off) to control the robot arm, q to quit: ";
                    getline(std::cin, userInput);
                    if (userInput.length() == 1) {
                        userInputChar = userInput[0];
                        if (userInputChar != 'q' && userInputChar != '1' && userInputChar != '0')
                            std::cout << "Invalid command\n";
                        else
                            break;
                    } else {
                        std::cout << "Invalid command\n";
                    }
                }

                if (userInputChar == 'q') {
                    break;
                }

                // Checking if a member exists
                if (doc[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY].HasMember(STATE_KEY)) {
                    // Erasing a member
                    doc[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY].EraseMember(STATE_KEY);
                }

                // Creating JsonValue for key
                util::JsonValue key(STATE_KEY, doc.GetAllocator());

                // Adding a member
                if (userInputChar == '1')
                    doc[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY].AddMember(key.Move(),
                                                                                          SHADOW_MYSTATE_VALUE_ON,
                                                                                          doc.GetAllocator());
                else
                    doc[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY].AddMember(key.Move(),
                                                                                          SHADOW_MYSTATE_VALUE_OFF,
                                                                                          doc.GetAllocator());



                // Publish method using standard pubsub model
                util::String payload = util::JsonParser::ToString(doc);
                rc = p_iot_client_->Publish(Utf8String::Create(update_topic), false, false,
                                            awsiotsdk::mqtt::QoS::QOS0, payload,
                                            ConfigCommon::mqtt_command_timeout_);
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(LOG_TAG_SWITCH_SAMPLE,
                                  "Shadow update using publish failed with return code : %d",
                                  static_cast<int>(rc));
                    break;
                }

                std::cout << std::endl << "Publishing message to cloud\n";
                util::String device = util::JsonParser::ToString(doc);
                std::cout << device << std::endl;

                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
            if (ResponseCode::SUCCESS != rc) {
                std::cout << "Disconnect failed";
            }

            std::cout << "Exiting sample." << std::endl;
            return ResponseCode::SUCCESS;
        }
    }
}

int main(int argc, char **argv) {
    std::shared_ptr <awsiotsdk::util::Logging::ConsoleLogSystem> p_log_system =
        std::make_shared<awsiotsdk::util::Logging::ConsoleLogSystem>(awsiotsdk::util::Logging::LogLevel::Info);
    std::unique_ptr <awsiotsdk::samples::SwitchThing>
        switch_thing = std::unique_ptr<awsiotsdk::samples::SwitchThing>(new awsiotsdk::samples::SwitchThing());
    awsiotsdk::util::Logging::InitializeAWSLogging(p_log_system);

    awsiotsdk::ResponseCode rc = awsiotsdk::ConfigCommon::InitializeCommon("config/SwitchConfig.json");
    if (awsiotsdk::ResponseCode::SUCCESS == rc) {
        rc = switch_thing->RunSample();
    }
    awsiotsdk::util::Logging::ShutdownAWSLogging();
    return static_cast<int>(rc);
}
