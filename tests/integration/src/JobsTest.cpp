/*
 * Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
 * @file JobsTest.cpp
 * @brief
 *
 */

#include "JobsTest.hpp"
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

#include "ConfigCommon.hpp"

#define JOBS_INTEGRATION_TEST_TAG "[Integration Test - Jobs]"

namespace awsiotsdk {
    namespace tests {
        namespace integration {
            ResponseCode JobsTest::GetPendingCallback(util::String topic_name,
                                                      util::String payload,
                                                      std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
                std::cout << std::endl << "************" << std::endl;
                std::cout << "GetPendingCallback called" << std::endl;
                std::cout << "Received message on topic : " << topic_name << std::endl;
                std::cout << "Payload Length : " << payload.length() << std::endl;
                std::cout << "Payload : " << payload << std::endl;

                ResponseCode rc;
                util::JsonDocument doc;

                done_ = false;

                rc = util::JsonParser::InitializeFromJsonString(doc, payload);
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(JOBS_INTEGRATION_TEST_TAG, "Json Parse for GetPendingCallback failed. %s",
                                  ResponseHelper::ToString(rc).c_str());
                    return rc;
                }

                if (doc.HasMember("inProgressJobs")) {
                    std::cout << "inProgressJobs : " << util::JsonParser::ToString(doc["inProgressJobs"]) << std::endl;
                }

                if (doc.HasMember("queuedJobs")) {
                    std::cout << "queuedJobs : " << util::JsonParser::ToString(doc["queuedJobs"]) << std::endl;
                }

                std::cout << "************" << std::endl;

                rc = p_jobs_->SendJobsQuery(Jobs::JOB_DESCRIBE_TOPIC, "$next");

                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(JOBS_INTEGRATION_TEST_TAG, "SendJobsQuery failed. %s",
                                  ResponseHelper::ToString(rc).c_str());
                    p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);

                    return ResponseCode::FAILURE;
                }

                return ResponseCode::SUCCESS;
            }

            ResponseCode JobsTest::NextJobCallback(util::String topic_name,
                                                   util::String payload,
                                                   std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
                std::cout << std::endl << "************" << std::endl;
                std::cout << "NextJobCallback called" << std::endl;
                std::cout << "Received message on topic : " << topic_name << std::endl;
                std::cout << "Payload Length : " << payload.length() << std::endl;
                std::cout << "Payload : " << payload << std::endl;

                ResponseCode rc;
                util::JsonDocument doc;

                rc = util::JsonParser::InitializeFromJsonString(doc, payload);
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(JOBS_INTEGRATION_TEST_TAG, "Json Parse for NextJobCallback failed. %s",
                                  ResponseHelper::ToString(rc).c_str());
                    return rc;
                }

                if (doc.HasMember("execution")) {
                    std::cout << "execution : " << util::JsonParser::ToString(doc["execution"]) << std::endl;

                    if (doc["execution"].HasMember("jobId")) {
                        util::Map<util::String, util::String> statusDetailsMap;

                        util::String jobId = doc["execution"]["jobId"].GetString();
                        std::cout << "jobId : " << jobId << std::endl;

                        if (doc["execution"].HasMember("jobDocument")) {
                            std::cout << "jobDocument : " << util::JsonParser::ToString(doc["execution"]["jobDocument"]) << std::endl;
                            statusDetailsMap.insert(std::make_pair("exampleDetail", "a value appropriate for your successful job"));
                            rc = p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_SUCCEEDED, statusDetailsMap);
                        } else {
                            statusDetailsMap.insert(std::make_pair("failureDetail", "Unable to process job document"));
                            rc = p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
                        }
                        if (ResponseCode::SUCCESS != rc) {
                            AWS_LOG_ERROR(JOBS_INTEGRATION_TEST_TAG, "SendJobsUpdate failed. %s", ResponseHelper::ToString(rc).c_str());
                            return rc;
                        }
                    }
                } else {
                    std::cout << "No job execution description found, nothing to do." << std::endl;
                    done_ = true;
                }

                std::cout << "************" << std::endl;
                return ResponseCode::SUCCESS;
            }

            ResponseCode JobsTest::Subscribe() {
                std::cout << "******** Subscribe ***************" << std::endl;

                mqtt::Subscription::ApplicationCallbackHandlerPtr p_pending_handler =
                    std::bind(&JobsTest::GetPendingCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

                mqtt::Subscription::ApplicationCallbackHandlerPtr p_next_handler =
                    std::bind(&JobsTest::NextJobCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

                util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;
                std::shared_ptr<mqtt::Subscription> p_subscription;

                p_subscription = p_jobs_->CreateJobsSubscription(p_pending_handler, nullptr, Jobs::JOB_GET_PENDING_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE);
                topic_vector.push_back(p_subscription);

                p_subscription = p_jobs_->CreateJobsSubscription(p_next_handler, nullptr, Jobs::JOB_DESCRIBE_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE, "$next");
                topic_vector.push_back(p_subscription);

                p_subscription = p_jobs_->CreateJobsSubscription(p_next_handler, nullptr, Jobs::JOB_NOTIFY_NEXT_TOPIC);
                topic_vector.push_back(p_subscription);

                ResponseCode rc = p_iot_client_->Subscribe(topic_vector, ConfigCommon::mqtt_command_timeout_);
                std::this_thread::sleep_for(std::chrono::seconds(3));
                return rc;
            }

            ResponseCode JobsTest::Unsubscribe() {
                uint16_t packet_id = 0;
                std::unique_ptr<Utf8String> p_topic_name;
                util::Vector<std::unique_ptr<Utf8String>> topic_vector;

                p_topic_name = p_jobs_->GetJobTopic(Jobs::JOB_GET_PENDING_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE);
                topic_vector.push_back(std::move(p_topic_name));

                p_topic_name = p_jobs_->GetJobTopic(Jobs::JOB_DESCRIBE_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE, "$next");
                topic_vector.push_back(std::move(p_topic_name));

                p_topic_name = p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_NEXT_TOPIC);
                topic_vector.push_back(std::move(p_topic_name));

                ResponseCode rc = p_iot_client_->UnsubscribeAsync(std::move(topic_vector), nullptr, packet_id);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                return rc;
            }

            ResponseCode JobsTest::InitializeTLS() {
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
#elif defined USE_MBEDTLS
                std::shared_ptr<network::MbedTLSConnection> p_network_connection =
                    std::make_shared<network::MbedTLSConnection>(ConfigCommon::endpoint_,
                                                                 ConfigCommon::endpoint_mqtt_port_,
                                                                 ConfigCommon::root_ca_path_,
                                                                 ConfigCommon::client_cert_path_,
                                                                 ConfigCommon::client_key_path_,
                                                                 ConfigCommon::tls_handshake_timeout_,
                                                                 ConfigCommon::tls_read_timeout_,
                                                                 ConfigCommon::tls_write_timeout_, true);

                if (ResponseCode::SUCCESS != rc) {
                    rc = ResponseCode::FAILURE;
                } else {
                    p_network_connection_ = std::dynamic_pointer_cast<NetworkConnection>(p_network_connection);
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
                    rc = ResponseCode::FAILURE;
                } else {
                    p_network_connection_ = std::dynamic_pointer_cast<NetworkConnection>(p_network_connection);
                }
#endif
                return rc;
            }

            ResponseCode JobsTest::RunTest() {
                done_ = false;
                ResponseCode rc = InitializeTLS();

                do {
                    if (ResponseCode::SUCCESS != rc) {
                        AWS_LOG_ERROR(JOBS_INTEGRATION_TEST_TAG, "Failed to initialize TLS layer. %s",
                                      ResponseHelper::ToString(rc).c_str());
                        break;
                    }

                    p_iot_client_ = std::shared_ptr<MqttClient>(
                        MqttClient::Create(p_network_connection_, ConfigCommon::mqtt_command_timeout_));
                    if (nullptr == p_iot_client_) {
                        AWS_LOG_ERROR(JOBS_INTEGRATION_TEST_TAG, "Failed to create MQTT Client Instance!!");
                        rc = ResponseCode::FAILURE;
                        break;
                    }

                    util::String client_id_tagged = ConfigCommon::base_client_id_;
                    client_id_tagged.append("_jobs_tester_");
                    client_id_tagged.append(std::to_string(rand()));
                    std::unique_ptr<Utf8String> client_id = Utf8String::Create(client_id_tagged);

                    rc = p_iot_client_->Connect(ConfigCommon::mqtt_command_timeout_, ConfigCommon::is_clean_session_,
                                                mqtt::Version::MQTT_3_1_1, ConfigCommon::keep_alive_timeout_secs_,
                                                std::move(client_id), nullptr, nullptr, nullptr);

                    p_jobs_ = Jobs::Create(p_iot_client_, mqtt::QoS::QOS1, ConfigCommon::thing_name_, client_id_tagged);

                    if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED != rc) {
                        AWS_LOG_ERROR(JOBS_INTEGRATION_TEST_TAG, "MQTT Connect failed. %s",
                                      ResponseHelper::ToString(rc).c_str());
                        return rc;
                    }

                    rc = Subscribe();
                    if (ResponseCode::SUCCESS != rc) {
                        AWS_LOG_ERROR(JOBS_INTEGRATION_TEST_TAG, "Subscribe failed. %s",
                                      ResponseHelper::ToString(rc).c_str());
                        p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                        break;
                    }

                    rc = p_jobs_->SendJobsQuery(Jobs::JOB_GET_PENDING_TOPIC);

                    if (ResponseCode::SUCCESS != rc) {
                        AWS_LOG_ERROR(JOBS_INTEGRATION_TEST_TAG, "SendJobsQuery failed. %s",
                                      ResponseHelper::ToString(rc).c_str());
                        p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                    }

                    int retries = 5;
                    while (!done_ && retries-- > 0) {
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                    }

                    if (!done_) {
                        AWS_LOG_ERROR(JOBS_INTEGRATION_TEST_TAG, "Not all jobs processed.");
                        p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                        rc = ResponseCode::FAILURE;
                        break;
                    }

                    rc = Unsubscribe();
                    if (ResponseCode::SUCCESS != rc) {
                        AWS_LOG_ERROR(JOBS_INTEGRATION_TEST_TAG, "Unsubscribe failed. %s",
                                      ResponseHelper::ToString(rc).c_str());
                        p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                        break;
                    }

                    rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                    if (ResponseCode::SUCCESS != rc) {
                        AWS_LOG_ERROR(JOBS_INTEGRATION_TEST_TAG, "Disconnect failed. %s",
                                      ResponseHelper::ToString(rc).c_str());
                        break;
                    }
                } while (false);

                std::cout << std::endl;
                if (ResponseCode::SUCCESS != rc) {
                    std::cout
                        << "Test Failed!!!! See above output for details!!"
                        << std::endl;
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
