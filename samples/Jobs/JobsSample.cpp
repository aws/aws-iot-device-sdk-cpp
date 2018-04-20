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
 * @file JobsSample.cpp
 *
 * This example takes the parameters from the config/SampleConfig.json file and establishes
 * a connection to the AWS IoT MQTT Platform. It performs several operations to
 * demonstrate the basic capabilities of the AWS IoT Jobs platform.
 *
 * If all the certs are correct, you should see the list of pending Job Executions
 * printed out by the GetPendingCallback callback. If there are any existing pending
 * job executions each will be processed one at a time in the NextJobCallback callback.
 * After all of the pending jobs have been processed the program will wait for
 * notifications for new pending jobs and process them one at a time as they come in.
 *
 * In the Subscribe function you can see how each callback is registered for each corresponding
 * Jobs topic.
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
#include "jobs/Jobs.hpp"
#include "JobsSample.hpp"

#define LOG_TAG_JOBS "[Sample - Jobs]"

namespace awsiotsdk {
    namespace samples {
        ResponseCode JobsSample::GetPendingCallback(util::String topic_name,
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
                AWS_LOG_ERROR(LOG_TAG_JOBS, "Json Parse for GetPendingCallback failed. %s",
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
            return ResponseCode::SUCCESS;
        }

        ResponseCode JobsSample::NextJobCallback(util::String topic_name,
                                                 util::String payload,
                                                 std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
            std::cout << std::endl << "************" << std::endl;
            std::cout << "NextJobCallback called" << std::endl;
            std::cout << "Received message on topic : " << topic_name << std::endl;
            std::cout << "Payload Length : " << payload.length() << std::endl;
            std::cout << "Payload : " << payload << std::endl;

            ResponseCode rc;
            util::JsonDocument doc;

            done_ = false;

            rc = util::JsonParser::InitializeFromJsonString(doc, payload);
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(LOG_TAG_JOBS, "Json Parse for NextJobCallback failed. %s",
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
                        if (ResponseCode::SUCCESS != rc) {
                            AWS_LOG_ERROR(LOG_TAG_JOBS, "SendJobsUpdate failed. %s", ResponseHelper::ToString(rc).c_str());
                            return rc;
                        }
                    } else {
                        statusDetailsMap.insert(std::make_pair("failureDetail", "Unable to process job document"));
                        rc = p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
                    }
                }
            } else {
                std::cout << "No job execution description found, nothing to do." << std::endl;
                done_ = true;
            }

            std::cout << "************" << std::endl;
            return ResponseCode::SUCCESS;
        }

        ResponseCode JobsSample::UpdateAcceptedCallback(util::String topic_name,
                                                        util::String payload,
                                                        std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
            std::cout << std::endl << "************" << std::endl;
            std::cout << "Received message on topic : " << topic_name << std::endl;
            std::cout << "Payload Length : " << payload.length() << std::endl;
            std::cout << "Payload : " << payload << std::endl;
            std::cout << std::endl << "************" << std::endl;
            return ResponseCode::SUCCESS;
        }

        ResponseCode JobsSample::UpdateRejectedCallback(util::String topic_name,
                                                        util::String payload,
                                                        std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
            std::cout << std::endl << "************" << std::endl;
            std::cout << "Received message on topic : " << topic_name << std::endl;
            std::cout << "Payload Length : " << payload.length() << std::endl;
            std::cout << "Payload : " << payload << std::endl;
            std::cout << std::endl << "************" << std::endl;

            /* Do error handling here for when the update was rejected */

            return ResponseCode::SUCCESS;
        }

        ResponseCode JobsSample::DisconnectCallback(util::String client_id,
                                                    std::shared_ptr<DisconnectCallbackContextData> p_app_handler_data) {
            std::cout << "*******************************************" << std::endl
                      << client_id << " Disconnected!" << std::endl
                      << "*******************************************" << std::endl;
            return ResponseCode::SUCCESS;
        }

        ResponseCode JobsSample::ReconnectCallback(util::String client_id,
                                                   std::shared_ptr<ReconnectCallbackContextData> p_app_handler_data,
                                                   ResponseCode reconnect_result) {
            std::cout << "*******************************************" << std::endl
                      << client_id << " Reconnect Attempted. Result " << ResponseHelper::ToString(reconnect_result)
                      << std::endl
                      << "*******************************************" << std::endl;
            return ResponseCode::SUCCESS;
        }

        ResponseCode JobsSample::ResubscribeCallback(util::String client_id,
                                                     std::shared_ptr<ResubscribeCallbackContextData> p_app_handler_data,
                                                     ResponseCode resubscribe_result) {
            std::cout << "*******************************************" << std::endl
                      << client_id << " Resubscribe Attempted. Result" << ResponseHelper::ToString(resubscribe_result)
                      << std::endl
                      << "*******************************************" << std::endl;
            return ResponseCode::SUCCESS;
        }


        ResponseCode JobsSample::Subscribe() {
            std::cout << "******** Subscribe ***************" << std::endl;

            mqtt::Subscription::ApplicationCallbackHandlerPtr p_pending_handler =
                std::bind(&JobsSample::GetPendingCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

            mqtt::Subscription::ApplicationCallbackHandlerPtr p_next_handler =
                std::bind(&JobsSample::NextJobCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

            mqtt::Subscription::ApplicationCallbackHandlerPtr p_update_accepted_handler =
                std::bind(&JobsSample::UpdateAcceptedCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

            mqtt::Subscription::ApplicationCallbackHandlerPtr p_update_rejected_handler =
                std::bind(&JobsSample::UpdateRejectedCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

            util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;
            std::shared_ptr<mqtt::Subscription> p_subscription;

            p_subscription = p_jobs_->CreateJobsSubscription(p_pending_handler, nullptr, Jobs::JOB_GET_PENDING_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE);
            topic_vector.push_back(p_subscription);

            p_subscription = p_jobs_->CreateJobsSubscription(p_next_handler, nullptr, Jobs::JOB_DESCRIBE_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE, "$next");
            topic_vector.push_back(p_subscription);

            p_subscription = p_jobs_->CreateJobsSubscription(p_next_handler, nullptr, Jobs::JOB_NOTIFY_NEXT_TOPIC);
            topic_vector.push_back(p_subscription);

            p_subscription = p_jobs_->CreateJobsSubscription(p_update_accepted_handler, nullptr, Jobs::JOB_UPDATE_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE, "+");
            topic_vector.push_back(p_subscription);

            p_subscription = p_jobs_->CreateJobsSubscription(p_update_rejected_handler, nullptr, Jobs::JOB_UPDATE_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE, "+");
            topic_vector.push_back(p_subscription);

            ResponseCode rc = p_iot_client_->Subscribe(topic_vector, ConfigCommon::mqtt_command_timeout_);
            return rc;
        }

        ResponseCode JobsSample::InitializeTLS() {
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
                AWS_LOG_ERROR(LOG_TAG_JOBS, "Failed to initialize Network Connection. %s",
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
                AWS_LOG_ERROR(LOG_TAG_JOBS, "Failed to initialize Network Connection. %s",
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
                AWS_LOG_ERROR(LOG_TAG_JOBS,
                              "Failed to initialize Network Connection. %s",
                              ResponseHelper::ToString(rc).c_str());
                rc = ResponseCode::FAILURE;
            } else {
                p_network_connection_ = std::dynamic_pointer_cast<NetworkConnection>(p_network_connection);
            }
#endif
            return rc;
        }

        ResponseCode JobsSample::RunSample() {
            done_ = false;

            ResponseCode rc = InitializeTLS();
            if (ResponseCode::SUCCESS != rc) {
                return rc;
            }

            ClientCoreState::ApplicationDisconnectCallbackPtr p_disconnect_handler =
                std::bind(&JobsSample::DisconnectCallback, this, std::placeholders::_1, std::placeholders::_2);

            ClientCoreState::ApplicationReconnectCallbackPtr p_reconnect_handler =
                std::bind(&JobsSample::ReconnectCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

            ClientCoreState::ApplicationResubscribeCallbackPtr p_resubscribe_handler =
                std::bind(&JobsSample::ResubscribeCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

            p_iot_client_ = std::shared_ptr<MqttClient>(MqttClient::Create(p_network_connection_,
                                                                           ConfigCommon::mqtt_command_timeout_,
                                                                           p_disconnect_handler, nullptr,
                                                                           p_reconnect_handler, nullptr,
                                                                           p_resubscribe_handler, nullptr));
            if (nullptr == p_iot_client_) {
                return ResponseCode::FAILURE;
            }

            util::String client_id_tagged = ConfigCommon::base_client_id_;
            client_id_tagged.append("_jobs_sample_");
            client_id_tagged.append(std::to_string(rand()));
            std::unique_ptr<Utf8String> client_id = Utf8String::Create(client_id_tagged);

            rc = p_iot_client_->Connect(ConfigCommon::mqtt_command_timeout_, ConfigCommon::is_clean_session_,
                                        mqtt::Version::MQTT_3_1_1, ConfigCommon::keep_alive_timeout_secs_,
                                        std::move(client_id), nullptr, nullptr, nullptr);
            if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED != rc) {
                return rc;
            }

            p_jobs_ = Jobs::Create(p_iot_client_, mqtt::QoS::QOS1, ConfigCommon::thing_name_, client_id_tagged);

            rc = Subscribe();
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(LOG_TAG_JOBS, "Subscribe failed. %s", ResponseHelper::ToString(rc).c_str());
            } else {
                rc = p_jobs_->SendJobsQuery(Jobs::JOB_GET_PENDING_TOPIC);

                if (ResponseCode::SUCCESS == rc) {
                    rc = p_jobs_->SendJobsQuery(Jobs::JOB_DESCRIBE_TOPIC, "$next");
                }

                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(LOG_TAG_JOBS, "SendJobsQuery failed. %s",
                                  ResponseHelper::ToString(rc).c_str());
                    p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                }
            }

            // Wait for job processing to complete
            while (!done_) {
                done_ = true;
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }

            rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(LOG_TAG_JOBS, "Disconnect failed. %s", ResponseHelper::ToString(rc).c_str());
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

    std::unique_ptr<awsiotsdk::samples::JobsSample>
        jobs_sample = std::unique_ptr<awsiotsdk::samples::JobsSample>(new awsiotsdk::samples::JobsSample());

    awsiotsdk::ResponseCode rc = awsiotsdk::ConfigCommon::InitializeCommon("config/SampleConfig.json");
    if (awsiotsdk::ResponseCode::SUCCESS == rc) {
        rc = jobs_sample->RunSample();
    }
#ifdef WIN32
    std::cout<<"Press any key to continue!!!!"<<std::endl;
    getchar();
#endif

    awsiotsdk::util::Logging::ShutdownAWSLogging();
    return static_cast<int>(rc);
}
