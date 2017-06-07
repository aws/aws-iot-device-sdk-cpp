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
 * @file robotArm.cpp
 * @brief Sample demonstrating connecting to a Greengrass core using discovery, interacts with Switch.cpp
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
#include "robotArm.hpp"

#define LOG_TAG_ROBOT_ARM_SAMPLE "[Sample - RobotArm]"

#define METERING_TOPIC "/topic/state"

#define DISCOVER_ACTION_RETRY_COUNT 10

#define SHADOW_DOCUMENT_STATE_KEY "state"
#define SHADOW_DOCUMENT_REPORTED_KEY "reported"
#define SHADOW_DOCUMENT_DESIRED_KEY "desired"

#define SHADOW_DOCUMENT_VERSION_KEY "version"
#define SHADOW_DOCUMENT_TIMESTAMP_KEY "timestamp"
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
"        }," \
"        \"reported\" : {" \
"        	\"myState\" : \"off\"" \
"        }" \
"    }" \
"}"

#define SHADOW_DOCUMENT_EMPTY_STRING_SEND "{" \
"    \"state\" : {" \
"        \"reported\" : {" \
"        	\"myState\" : \"off\"" \
"        }" \
"    }" \
"}"

namespace awsiotsdk {
    namespace samples {
        bool RobotArmThing::ConnectivitySortFunction(ConnectivityInfo info1, ConnectivityInfo info2) {
            if (0 > info1.id_.compare(info2.id_)) {
                return true;
            }
            return false;
        }

        ResponseCode RobotArmThing::ActionResponseHandler(util::String thing_name, ShadowRequestType request_type,
                                                          ShadowResponseType response_type,
                                                          util::JsonDocument &payload) {
            switch (response_type) {
                case ShadowResponseType::Accepted:
                    std::cout << "Message was accepted\r";
                    sync_action_response_ = ResponseCode::SHADOW_REQUEST_ACCEPTED;
                    break;
                case ShadowResponseType::Rejected:
                    std::cout << "Message was rejected\r";
                    sync_action_response_ = ResponseCode::SHADOW_REQUEST_REJECTED;
                    break;
                case ShadowResponseType::Delta:
                    std::cout << "Received delta\r";
                    sync_action_response_ = ResponseCode::SHADOW_RECEIVED_DELTA;
                    break;
            }
            sync_action_response_wait_.notify_all();
            return sync_action_response_;
        }

        ResponseCode RobotArmThing::RunSample() {
            std::string currentState = SHADOW_MYSTATE_VALUE_OFF;
            ResponseCode rc = ResponseCode::SUCCESS;

            std::shared_ptr <network::OpenSSLConnection> p_openssl_connection =
                std::make_shared<network::OpenSSLConnection>(ConfigCommon::endpoint_,
                                                             ConfigCommon::endpoint_greengrass_discovery_port_,
                                                             ConfigCommon::root_ca_path_,
                                                             ConfigCommon::client_cert_path_,
                                                             ConfigCommon::client_key_path_,
                                                             ConfigCommon::tls_handshake_timeout_,
                                                             ConfigCommon::tls_read_timeout_,
                                                             ConfigCommon::tls_write_timeout_,
                                                             true);
            rc = p_openssl_connection->Initialize();

            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(LOG_TAG_ROBOT_ARM_SAMPLE,
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

            std::unique_ptr <Utf8String> p_thing_name = Utf8String::Create(ConfigCommon::thing_name_);

            DiscoveryResponse discovery_response;
            int max_retries = 0;

            do {
                std::unique_ptr <Utf8String> p_thing_name = Utf8String::Create(ConfigCommon::thing_name_);
                rc = p_iot_client_->Discover(std::chrono::milliseconds(ConfigCommon::discover_action_timeout_),
                                             std::move(p_thing_name), discovery_response);
                if (rc != ResponseCode::DISCOVER_ACTION_SUCCESS) {
                    max_retries++;
                    if (rc != ResponseCode::DISCOVER_ACTION_NO_INFORMATION_PRESENT) {
                        AWS_LOG_INFO(LOG_TAG_ROBOT_ARM_SAMPLE,
                                     "Discover Request failed with response code: %d.  Trying again...",
                                     static_cast<int>(rc));
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                    } else {
                        AWS_LOG_INFO(LOG_TAG_ROBOT_ARM_SAMPLE, "No GGC connectivity information present for this Device: %d",
                                     static_cast<int>(rc));
                        return rc;
                    }
                }
            } while (max_retries != DISCOVER_ACTION_RETRY_COUNT && rc != ResponseCode::DISCOVER_ACTION_SUCCESS);

            if (max_retries == DISCOVER_ACTION_RETRY_COUNT) {
                AWS_LOG_INFO(LOG_TAG_ROBOT_ARM_SAMPLE, "Discover failed after max retries, exiting");
                return rc;
            }

            AWS_LOG_INFO(LOG_TAG_ROBOT_ARM_SAMPLE, "GGC connectivity information found for this Device! %d\n",
                         static_cast<int>(rc));

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
            std::sort(parsed_response.begin(), parsed_response.end(), std::bind(RobotArmThing::ConnectivitySortFunction,
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

                AWS_LOG_INFO(LOG_TAG_ROBOT_ARM_SAMPLE,
                             "Attempting Connect with:\nGGC Endpoint : %s\nGGC Endpoint Port : %u\n",
                             connectivity_info_itr.host_address_.c_str(), connectivity_info_itr.port_);

                for (auto ca_list_itr: ca_map_itr->second) {
                    util::String core_ca_file_path = ca_output_path_base;
                    core_ca_file_path.append(std::to_string(suffix_itr));
                    core_ca_file_path.append(".pem");
                    p_openssl_connection->SetRootCAPath(core_ca_file_path);

                    AWS_LOG_INFO(LOG_TAG_ROBOT_ARM_SAMPLE, "Using CA at : %s\n", core_ca_file_path.c_str());

                    std::unique_ptr <Utf8String> p_client_id = Utf8String::Create(ConfigCommon::base_client_id_);

                    rc = p_iot_client_->Connect(ConfigCommon::mqtt_command_timeout_,
                                                ConfigCommon::is_clean_session_, mqtt::Version::MQTT_3_1_1,
                                                ConfigCommon::keep_alive_timeout_secs_, std::move(p_client_id),
                                                nullptr, nullptr, nullptr);
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED == rc) {
                        break;
                    }
                    AWS_LOG_INFO(LOG_TAG_ROBOT_ARM_SAMPLE, "Connect attempt failed with this CA!!");
                    suffix_itr++;
                }
                if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED == rc) {
                    AWS_LOG_INFO(LOG_TAG_ROBOT_ARM_SAMPLE, "Connected to GGC %s in Group %s!!",
                                 connectivity_info_itr.ggc_name_.c_str(),
                                 connectivity_info_itr.group_name_.c_str());
                    break;
                }
                AWS_LOG_INFO(LOG_TAG_ROBOT_ARM_SAMPLE, "Connect attempt failed for GGC %s in Group %s!!",
                             connectivity_info_itr.ggc_name_.c_str(),
                             connectivity_info_itr.group_name_.c_str());
            }

            if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED != rc) {
                return rc;
            }

            // Get lock on shadow response handler execution
            std::unique_lock <std::mutex> block_handler_lock(sync_action_response_lock_);
            {
                // Using mqtt command timeout as shadow action timeout
                // Using Thing name as client token prefix
                std::chrono::milliseconds shadow_action_timeout = ConfigCommon::mqtt_command_timeout_;
                Shadow my_shadow(p_iot_client_, ConfigCommon::mqtt_command_timeout_, ConfigCommon::thing_name_,
                                 ConfigCommon::thing_name_);

                // Subscribe to shadow actions
                Shadow::RequestHandlerPtr p_action_handler =
                    std::bind(&RobotArmThing::ActionResponseHandler, this, std::placeholders::_1,
                              std::placeholders::_2,
                              std::placeholders::_3, std::placeholders::_4);
                util::Map <ShadowRequestType, Shadow::RequestHandlerPtr> request_mapping;
                request_mapping.insert(std::make_pair(ShadowRequestType::Get, p_action_handler));
                request_mapping.insert(std::make_pair(ShadowRequestType::Update, p_action_handler));
                request_mapping.insert(std::make_pair(ShadowRequestType::Delete, p_action_handler));
                request_mapping.insert(std::make_pair(ShadowRequestType::Delta, p_action_handler));
                my_shadow.AddShadowSubscription(request_mapping);

                // Setup send and receive documents
                util::JsonDocument receivedMessage, sendMessage;

                rc = util::JsonParser::InitializeFromJsonString(receivedMessage, SHADOW_DOCUMENT_EMPTY_STRING);
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(LOG_TAG_ROBOT_ARM_SAMPLE, "Json Parse for template failed with return code : %d",
                                  static_cast<int>(rc));
                    rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                    std::cout << "Exiting Sample!!!!" << std::endl;
                    return rc;
                }

                rc = util::JsonParser::InitializeFromJsonString(sendMessage, SHADOW_DOCUMENT_EMPTY_STRING_SEND);
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(LOG_TAG_ROBOT_ARM_SAMPLE, "Json Parse for template failed with return code : %d",
                                  static_cast<int>(rc));
                    rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                    std::cout << "Exiting Sample!!!!" << std::endl;
                    return rc;
                }


                // Creating JsonValue for key
                util::JsonValue initialKey(STATE_KEY, sendMessage.GetAllocator());
                // Creating JsonValue for the value of the above key
                util::JsonValue initialVal;
                initialVal.SetString(SHADOW_MYSTATE_VALUE_OFF);

                // Checking if a member exists
                if (sendMessage[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_REPORTED_KEY].HasMember(STATE_KEY)) {
                    // Erasing a member
                    sendMessage[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_REPORTED_KEY].EraseMember(STATE_KEY);
                }

                // Adding a member
                sendMessage[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_REPORTED_KEY].AddMember(initialKey.Move(),
                                                                                               initialVal.Move(),
                                                                                               sendMessage.GetAllocator());

                // Update current device shadow using the above doc
                my_shadow.UpdateDeviceShadow(sendMessage);

                util::String device = util::JsonParser::ToString(sendMessage);
                std::cout << std::endl << "Sending Inital State ------- " << std::endl << device << std::endl
                          << std::endl;

                rc = my_shadow.PerformUpdateAsync();
                sync_action_response_wait_.wait_for(block_handler_lock, shadow_action_timeout);
                rc = sync_action_response_;
                if (ResponseCode::SHADOW_REQUEST_REJECTED == rc) {
                    AWS_LOG_ERROR(LOG_TAG_ROBOT_ARM_SAMPLE, "Shadow update failed with return code : %d",
                                  static_cast<int>(rc));
                    return rc;
                }

                //Sleep for 1 second and wait for all messages to be received
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                std::cout << "Waiting for an update!\n";

                while (true) {
                    // Wait for delta
                    sync_action_response_wait_.wait_for(block_handler_lock, shadow_action_timeout);
                    rc = sync_action_response_;

                    if (rc == ResponseCode::SHADOW_RECEIVED_DELTA) {
                        receivedMessage = my_shadow.GetServerDocument();
                        device = util::JsonParser::ToString(receivedMessage);

                        if (receivedMessage[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY].HasMember(STATE_KEY)) {
                            std::string receivedDeltaString =
                                receivedMessage[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY][STATE_KEY].GetString();
                            if (receivedDeltaString != currentState) {
                                currentState = receivedDeltaString;

                                // Checking if a member exists
                                if (sendMessage[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_REPORTED_KEY].HasMember(
                                    STATE_KEY)) {
                                    // Erasing a member
                                    sendMessage[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_REPORTED_KEY].EraseMember(
                                        STATE_KEY);
                                }

                                // Creating JsonValue for key
                                util::JsonValue key(STATE_KEY, sendMessage.GetAllocator());
                                // Creating JsonValue for the value of the above key
                                util::JsonValue val;

                                util::String p_topic_name_str = METERING_TOPIC;
                                util::String payload;
                                if (currentState.compare(SHADOW_MYSTATE_VALUE_ON) == 0) {
                                    val.SetString(SHADOW_MYSTATE_VALUE_ON);
                                    payload = "{\"state\": \"on\"}";
                                } else {
                                    val.SetString(SHADOW_MYSTATE_VALUE_OFF);
                                    payload = "{\"state\": \"off\"}";
                                }
                                uint16_t packet_id = 0;
                                std::unique_ptr <Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
                                rc = p_iot_client_->PublishAsync(std::move(p_topic_name), false, false, mqtt::QoS::QOS0,
                                                                 payload, nullptr, packet_id);
                                if (ResponseCode::SUCCESS == rc) {
                                    std::cout
                                        << "-- Published state to /topic/metering (Should be routed to uptimelambda!) --"
                                        << std::endl;
                                }

                                std::cout << "------- Robot Arm State --------" << std::endl << currentState
                                          << std::endl;

                                sendMessage[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_REPORTED_KEY].AddMember(key.Move(),
                                                                                                               val.Move(),
                                                                                                               sendMessage.GetAllocator());

                                // Update current device shadow using the above doc
                                my_shadow.UpdateDeviceShadow(sendMessage);
                                rc = my_shadow.PerformUpdateAsync();
                                sync_action_response_wait_.wait_for(block_handler_lock, shadow_action_timeout);
                                rc = sync_action_response_;
                                if (ResponseCode::SHADOW_REQUEST_REJECTED == rc) {
                                    AWS_LOG_ERROR(LOG_TAG_ROBOT_ARM_SAMPLE,
                                                  "Shadow update failed with return code : %d",
                                                  static_cast<int>(rc));
                                    return rc;
                                }
                                //Sleep for 1 second and wait for all messages to be received
                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                            }
                        }
                    }
                }
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
    std::unique_ptr <awsiotsdk::samples::RobotArmThing>
        robotarm = std::unique_ptr<awsiotsdk::samples::RobotArmThing>(new awsiotsdk::samples::RobotArmThing());
    awsiotsdk::util::Logging::InitializeAWSLogging(p_log_system);

    awsiotsdk::ResponseCode rc = awsiotsdk::ConfigCommon::InitializeCommon("config/RobotArmConfig.json");
    if (awsiotsdk::ResponseCode::SUCCESS == rc) {
        rc = robotarm->RunSample();
    }
    awsiotsdk::util::Logging::ShutdownAWSLogging();
    return static_cast<int>(rc);
}
