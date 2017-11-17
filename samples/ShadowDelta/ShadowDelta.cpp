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
 * @file ShadowDelta.cpp
 * @brief Sample demonstrating Shadow operations
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
#include "ShadowDelta.hpp"

#define LOG_TAG_SHADOW_DELTA "[Sample - ShadowDelta]"
#define SDK_SAMPLE_TOPIC "Pub_Sub_Sample_Topic"

#define MESSAGE_COUNT 10

#define SHADOW_DOCUMENT_STATE_KEY "state"
#define SHADOW_DOCUMENT_REPORTED_KEY "reported"
#define SHADOW_DOCUMENT_DESIRED_KEY "desired"
#define SHADOW_DOCUMENT_VERSION_KEY "version"
#define SHADOW_DOCUMENT_TIMESTAMP_KEY "timestamp"
#define MSG_COUNT_KEY "cur_msg_count"

#define SHADOW_TOPIC_PREFIX "$aws/things/"
#define SHADOW_TOPIC_MIDDLE "/shadow/"
#define SHADOW_REQUEST_TYPE_UPDATE_STRING "update"

#define SHADOW_DOCUMENT_EMPTY_STRING "{" \
"    \"state\" : {" \
"        \"desired\" : {" \
"        	\"cur_msg_count\" : 0" \
"        }," \
"        \"reported\" : {" \
"        	\"cur_msg_count\" : 0" \
"        }" \
"    }" \
"}"

namespace awsiotsdk {
    namespace samples {
        ResponseCode ShadowDelta::InitializeTLS() {
            ResponseCode rc = ResponseCode::SUCCESS;

#ifdef USE_WEBSOCKETS
            p_network_connection_ = std::shared_ptr<NetworkConnection>(
                new network::WebSocketConnection(ConfigCommon::endpoint_,
                                                 ConfigCommon::endpoint_https_port_,
                                                 ConfigCommon::root_ca_path_,
                                                 ConfigCommon::aws_region_,
                                                 ConfigCommon::aws_access_key_id_,
                                                 ConfigCommon::aws_secret_access_key_,
                                                 ConfigCommon::aws_session_token_,
                                                 ConfigCommon::tls_handshake_timeout_,
                                                 ConfigCommon::tls_read_timeout_,
                                                 ConfigCommon::tls_write_timeout_, true));
            if (nullptr == p_network_connection_) {
                AWS_LOG_ERROR(LOG_TAG_SHADOW_DELTA,
                              "Failed to initialize Network Connection. %s",
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
                AWS_LOG_ERROR(LOG_TAG_SHADOW_DELTA,
                              "Failed to initialize Network Connection. %s",
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
                AWS_LOG_ERROR(LOG_TAG_SHADOW_DELTA,
                              "Failed to initialize Network Connection. %s",
                              ResponseHelper::ToString(rc).c_str());
                rc = ResponseCode::FAILURE;
            } else {
                p_network_connection_ = std::dynamic_pointer_cast<NetworkConnection>(p_network_connection);
            }
#endif
            return rc;
        }

        ResponseCode ShadowDelta::ActionResponseHandler(util::String thing_name, ShadowRequestType request_type,
                                                        ShadowResponseType response_type, util::JsonDocument &payload) {
            switch (response_type) {
                case ShadowResponseType::Accepted:
                    sync_action_response_ = ResponseCode::SHADOW_REQUEST_ACCEPTED;
                    break;
                case ShadowResponseType::Rejected:
                    sync_action_response_ = ResponseCode::SHADOW_REQUEST_REJECTED;
                    break;
                case ShadowResponseType::Delta:
                    sync_action_response_ = ResponseCode::SHADOW_RECEIVED_DELTA;
                    break;
            }

            sync_action_response_wait_.notify_all();
            return sync_action_response_;
        }

        ResponseCode ShadowDelta::RunSample() {
            total_published_messages_ = 0;
            cur_pending_messages_ = 0;

            ResponseCode rc = InitializeTLS();
            if (ResponseCode::SUCCESS != rc) {
                return rc;
            }

            p_iot_client_ = std::shared_ptr<MqttClient>(MqttClient::Create(p_network_connection_,
                                                                           ConfigCommon::mqtt_command_timeout_));
            if (nullptr == p_iot_client_) {
                return ResponseCode::FAILURE;
            }

            util::String client_id_tagged = ConfigCommon::base_client_id_;

            // Note: Comment out the below two lines when testing with a GGC as the routes are programmed
            // for the exact thing names
            client_id_tagged.append("_shadow_delta_tester_");
            client_id_tagged.append(std::to_string(rand()));
            std::unique_ptr<Utf8String> client_id = Utf8String::Create(client_id_tagged);

            rc = p_iot_client_->Connect(ConfigCommon::mqtt_command_timeout_, ConfigCommon::is_clean_session_,
                                        mqtt::Version::MQTT_3_1_1, ConfigCommon::keep_alive_timeout_secs_,
                                        std::move(client_id), nullptr, nullptr, nullptr);
            if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED != rc) {
                return rc;
            }

            // Get lock on shadow response handler execution
            std::unique_lock<std::mutex> block_handler_lock(sync_action_response_lock_);
            {
                // Using mqtt command timeout as shadow action timeout
                // Using Thing name as client token prefix
                std::chrono::milliseconds shadow_action_timeout = ConfigCommon::mqtt_command_timeout_;
                Shadow my_shadow(p_iot_client_, ConfigCommon::mqtt_command_timeout_, ConfigCommon::thing_name_,
                                 ConfigCommon::thing_name_);

                // Subscribe to shadow actions
                Shadow::RequestHandlerPtr p_action_handler =
                    std::bind(&ShadowDelta::ActionResponseHandler, this, std::placeholders::_1,
                              std::placeholders::_2,
                              std::placeholders::_3, std::placeholders::_4);
                util::Map<ShadowRequestType, Shadow::RequestHandlerPtr> request_mapping;
                request_mapping.insert(std::make_pair(ShadowRequestType::Get, p_action_handler));
                request_mapping.insert(std::make_pair(ShadowRequestType::Update, p_action_handler));
                request_mapping.insert(std::make_pair(ShadowRequestType::Delete, p_action_handler));
                request_mapping.insert(std::make_pair(ShadowRequestType::Delta, p_action_handler));
                my_shadow.AddShadowSubscription(request_mapping);

                // Start from a no shadow state
                // Attempt to get the current shadow
                rc = my_shadow.PerformGetAsync();
                if (ResponseCode::SUCCESS == rc) {
                    sync_action_response_wait_.wait_for(block_handler_lock, shadow_action_timeout);
                    rc = sync_action_response_;
                    if (ResponseCode::SHADOW_REQUEST_ACCEPTED == rc) {
                        // Shadow exists, delete it
                        rc = my_shadow.PerformDeleteAsync();
                        sync_action_response_wait_.wait_for(block_handler_lock, shadow_action_timeout);
                        rc = sync_action_response_;
                        if (ResponseCode::SHADOW_REQUEST_ACCEPTED != rc) {
                            AWS_LOG_ERROR(LOG_TAG_SHADOW_DELTA, "Shadow Delete request failed!!");
                            rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                            std::cout << "Exiting Sample!!!!" << std::endl;
                            return rc;
                        }
                    }
                }

                // Shadow delete, update with sample shadow
                util::JsonDocument doc;
                unsigned int request_itr = 0;

                util::String update_topic = SHADOW_TOPIC_PREFIX;
                update_topic.append(ConfigCommon::thing_name_);
                update_topic.append(SHADOW_TOPIC_MIDDLE);
                update_topic.append(SHADOW_REQUEST_TYPE_UPDATE_STRING);
                // Rapidjson uses move semantics, doc needs to be initialized again
                rc = util::JsonParser::InitializeFromJsonString(doc, SHADOW_DOCUMENT_EMPTY_STRING);
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(LOG_TAG_SHADOW_DELTA, "Json Parse for sample failed. %s",
                                  ResponseHelper::ToString(rc).c_str());
                    rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                    std::cout << "Exiting Sample!!!!" << std::endl;
                    return rc;
                }

                do {
                    if (0 != request_itr) {
                        doc = my_shadow.GetServerDocument();
                    }
                    // Checking if a member exists
                    if (doc[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY].HasMember(MSG_COUNT_KEY)) {
                        // Erasing a member
                        doc[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY].EraseMember(MSG_COUNT_KEY);
                    }

                    // Creating JsonValue for key
                    util::JsonValue key(MSG_COUNT_KEY, doc.GetAllocator());
                    // Creating JsonValue for the value of the above key
                    util::JsonValue val(request_itr + 1);

                    // Adding a member
                    doc[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY].AddMember(key.Move(), val.Move(),
                                                                                          doc.GetAllocator());

                    // Update current device shadow using the above doc
                    my_shadow.UpdateDeviceShadow(doc);

                    // Perform an Update operation
                    // This will generate a diff between the last received server state and the current device state
                    // and perform a shadow update operation
                    rc = my_shadow.PerformUpdateAsync();
                    sync_action_response_wait_.wait_for(block_handler_lock, shadow_action_timeout);
                    rc = sync_action_response_;
                    if (ResponseCode::SHADOW_REQUEST_REJECTED == rc) {
                        AWS_LOG_ERROR(LOG_TAG_SHADOW_DELTA, "Shadow update failed. %s",
                                      ResponseHelper::ToString(rc).c_str());
                        break;
                    }
                    //Sleep for 1 second and wait for all messages to be received
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                    if (my_shadow.IsInSync()) {
                        AWS_LOG_ERROR(LOG_TAG_SHADOW_DELTA, "Expected shadow to be out of sync!!");
                    } else {
                        AWS_LOG_INFO(LOG_TAG_SHADOW_DELTA, "Shadow out of sync!!");
                    }

                    // Get the current server document
                    doc = my_shadow.GetServerDocument();

                    util::String device = util::JsonParser::ToString(doc);
                    std::cout << std::endl << "Server Shadow State ------- " << std::endl << device << std::endl
                              << std::endl;
                    std::cout << "--------------------------- " << std::endl << std::endl;

                    // Checking if a member exists
                    if (doc[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_REPORTED_KEY].HasMember(MSG_COUNT_KEY)) {
                        // Erasing a member
                        doc[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_REPORTED_KEY].EraseMember(MSG_COUNT_KEY);
                    }

                    // Creating JsonValue for key
                    key = util::JsonValue(MSG_COUNT_KEY, doc.GetAllocator());
                    // Creating JsonValue for the value of the above key
                    val = util::JsonValue(request_itr + 1);

                    // Adding a member
                    doc[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_REPORTED_KEY].AddMember(key.Move(), val.Move(),
                                                                                           doc.GetAllocator());

                    // Below code demonstrates both performing an update using Shadow API as well as direct publish
                    if (0 == (request_itr % 2)) {
                        // Update current device shadow using the above doc
                        my_shadow.UpdateDeviceShadow(doc);

                        // Perform an Update operation
                        rc = my_shadow.PerformUpdateAsync();
                        sync_action_response_wait_.wait_for(block_handler_lock, shadow_action_timeout);
                        rc = sync_action_response_;
                        if (ResponseCode::SHADOW_REQUEST_REJECTED == rc) {
                            AWS_LOG_ERROR(LOG_TAG_SHADOW_DELTA, "2Shadow update failed. %s",
                                          ResponseHelper::ToString(rc).c_str());
                            break;
                        }
                    } else {
                        // Update device shadow using a publish to test Delta topic
                        util::JsonDocument diff;
                        util::JsonDocument cur_server_state_doc = my_shadow.GetServerDocument();
                        util::JsonParser::DiffValues(diff, cur_server_state_doc, doc, diff.GetAllocator());
                        if (diff.HasMember(SHADOW_DOCUMENT_TIMESTAMP_KEY)) {
                            diff.EraseMember(SHADOW_DOCUMENT_TIMESTAMP_KEY);
                        }

                        if (diff.HasMember(SHADOW_DOCUMENT_VERSION_KEY)) {
                            diff.EraseMember(SHADOW_DOCUMENT_VERSION_KEY);
                        }

                        util::String payload = util::JsonParser::ToString(diff);

                        // Note: For testing with a GGC, set QOS to 0 instead of 1.
                        rc = p_iot_client_->Publish(Utf8String::Create(update_topic), false, false,
                                                    awsiotsdk::mqtt::QoS::QOS1, payload,
                                                    ConfigCommon::mqtt_command_timeout_);
                        if (ResponseCode::SUCCESS != rc) {
                            AWS_LOG_ERROR(LOG_TAG_SHADOW_DELTA,
                                          "Shadow update using publish failed. %s",
                                          ResponseHelper::ToString(rc).c_str());
                            break;
                        }
                    }

                    //Sleep for 1 second and wait for all messages to be received
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                    // Get the current server document
                    doc = my_shadow.GetServerDocument();
                    device = util::JsonParser::ToString(doc);
                    std::cout << std::endl << "Server Shadow State ------- " << std::endl << device << std::endl
                              << std::endl;
                    if (my_shadow.IsInSync()) {
                        AWS_LOG_INFO(LOG_TAG_SHADOW_DELTA, "Shadow is in sync!!");
                    } else {
                        AWS_LOG_ERROR(LOG_TAG_SHADOW_DELTA, "Expected shadow to be in sync!!");
                    }

                    request_itr++;
                } while (request_itr < MESSAGE_COUNT);
            }
            rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(LOG_TAG_SHADOW_DELTA, "Disconnect failed. %s", ResponseHelper::ToString(rc).c_str());
            }

            std::cout << "Exiting Sample!!!!" << std::endl;
            return ResponseCode::SUCCESS;
        }
    }
}

int main(int argc, char **argv) {
    std::shared_ptr<awsiotsdk::util::Logging::ConsoleLogSystem> p_log_system =
        std::make_shared<awsiotsdk::util::Logging::ConsoleLogSystem>(awsiotsdk::util::Logging::LogLevel::Info);
    awsiotsdk::util::Logging::InitializeAWSLogging(p_log_system);

    std::unique_ptr<awsiotsdk::samples::ShadowDelta>
        shadow_delta = std::unique_ptr<awsiotsdk::samples::ShadowDelta>(new awsiotsdk::samples::ShadowDelta());

    awsiotsdk::ResponseCode rc = awsiotsdk::ConfigCommon::InitializeCommon("config/SampleConfig.json");
    if (awsiotsdk::ResponseCode::SUCCESS == rc) {
        rc = shadow_delta->RunSample();
    }
#ifdef WIN32
    std::cout<<"Press any key to continue!!!!"<<std::endl;
    getchar();
#endif

    awsiotsdk::util::Logging::ShutdownAWSLogging();
    return static_cast<int>(rc);
}
