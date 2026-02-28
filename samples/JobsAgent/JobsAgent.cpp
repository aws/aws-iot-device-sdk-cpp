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
 * @file JobsAgent.cpp
 *
 * This is a full featured sample agent that interacts with the AWS IoT Jobs 
 * platform. For more information see the README.md file in the samples directory.
 *
 */

#include <chrono>
#include <cstring>
#include <sys/utsname.h>

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
#include "JobsAgent.hpp"

namespace awsiotsdk {
    namespace samples {
        ResponseCode JobsAgent::InitializeTLS() {
            ResponseCode rc = ResponseCode::SUCCESS;

#ifdef USE_WEBSOCKETS
            p_network_connection_ = std::make_shared<NetworkConnection>(ConfigCommon::endpoint_,
                                                                        ConfigCommon::endpoint_https_port_,
                                                                        ConfigCommon::root_ca_path_,
                                                                        ConfigCommon::aws_region_,
                                                                        ConfigCommon::aws_access_key_id_,
                                                                        ConfigCommon::aws_secret_access_key_,
                                                                        ConfigCommon::aws_session_token_,
                                                                        ConfigCommon::tls_handshake_timeout_,
                                                                        ConfigCommon::tls_read_timeout_,
                                                                        ConfigCommon::tls_write_timeout_, true);
            if (nullptr == p_network_connection_) {
                AWS_LOGSTREAM_ERROR(LOG_TAG_JOBS_AGENT, "Failed to initialize Network Connection. " << ResponseHelper::ToString(rc).c_str());
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
                AWS_LOGSTREAM_ERROR(LOG_TAG_JOBS_AGENT, "Failed to initialize Network Connection. " << ResponseHelper::ToString(rc).c_str());
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
                AWS_LOGSTREAM_ERROR(LOG_TAG_JOBS_AGENT, "Failed to initialize Network Connection. " << ResponseHelper::ToString(rc).c_str());
                rc = ResponseCode::FAILURE;
            } else {
                p_network_connection_ = std::dynamic_pointer_cast<NetworkConnection>(p_network_connection);
            }
#endif
            return rc;
        }

        ResponseCode JobsAgent::DisconnectCallback(util::String client_id,
                                                   std::shared_ptr<DisconnectCallbackContextData> p_app_handler_data) {
            std::cout << "*******************************************" << std::endl
                      << client_id << " Disconnected!" << std::endl
                      << "*******************************************" << std::endl;
            return ResponseCode::SUCCESS;
        }

        ResponseCode JobsAgent::ReconnectCallback(util::String client_id,
                                                  std::shared_ptr<ReconnectCallbackContextData> p_app_handler_data,
                                                  ResponseCode reconnect_result) {
            std::cout << "*******************************************" << std::endl
                      << client_id << " Reconnect Attempted. Result " << ResponseHelper::ToString(reconnect_result)
                      << std::endl
                      << "*******************************************" << std::endl;
            return ResponseCode::SUCCESS;
        }

        ResponseCode JobsAgent::ResubscribeCallback(util::String client_id,
                                                    std::shared_ptr<ResubscribeCallbackContextData> p_app_handler_data,
                                                    ResponseCode resubscribe_result) {
            std::cout << "*******************************************" << std::endl
                      << client_id << " Resubscribe Attempted. Result" << ResponseHelper::ToString(resubscribe_result)
                      << std::endl
                      << "*******************************************" << std::endl;
            return ResponseCode::SUCCESS;
        }

        ResponseCode JobsAgent::Subscribe() {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "Subscribe");

            auto p_next_handler = [this](util::String topic_name,
                                         util::String payload,
                                         std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
                return NextJobCallback(topic_name, payload, p_app_handler_data);
            };

            auto p_update_accepted_handler = [this](util::String topic_name,
                                                    util::String payload,
                                                    std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
                return UpdateAcceptedCallback(topic_name, payload, p_app_handler_data);
            };

            auto p_update_rejected_handler = [this](util::String topic_name,
                                                    util::String payload,
                                                    std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
                return UpdateRejectedCallback(topic_name, payload, p_app_handler_data);
            };

            util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;
            std::shared_ptr<mqtt::Subscription> p_subscription;

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

        void JobsAgent::StartInstalledPackages() {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "StartInstalledPackages");

            util::Map<util::String, util::String> statusDetailsMap;

            for (util::JsonValue::ConstMemberIterator package_itr = installed_packages_json_.MemberBegin();
                package_itr != installed_packages_json_.MemberEnd(); ++package_itr) {
                if (package_itr->value.IsObject()) {
                    util::String packageName = package_itr->name.GetString();
                    if (PackageIsAutoStart(packageName.c_str())) {
                        StartPackage(statusDetailsMap, packageName.c_str());
                    }
                }
            }
        }

        ResponseCode JobsAgent::RunAgent(const util::String &processTitle) {
            process_title_ = processTitle;

            installed_packages_filename_ = DEFAULT_INSTALLED_PACKAGES_FILENAME;
            ResponseCode rc = util::JsonParser::InitializeFromJsonFile(installed_packages_json_, installed_packages_filename_.c_str());
            if (ResponseCode::FILE_OPEN_ERROR == rc) {
                AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "Unable to open installed packages file %s, assuming no packages installed.", installed_packages_filename_.c_str());
                rc = util::JsonParser::InitializeFromJsonString(installed_packages_json_, "{}");
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOGSTREAM_ERROR(LOG_TAG_JOBS_AGENT, "Unexpected initialization error: " << ResponseHelper::ToString(rc).c_str());
                }
            } else if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(LOG_TAG_JOBS_AGENT,
                              "Error in Parsing. %s\n parse error code : %d, offset : %d",
                              ResponseHelper::ToString(rc).c_str(),
                              static_cast<int>(util::JsonParser::GetParseErrorCode(installed_packages_json_)),
                              static_cast<unsigned int>(util::JsonParser::GetParseErrorOffset(installed_packages_json_)));
                return rc;
            }

            rc = InitializeTLS();
            if (ResponseCode::SUCCESS != rc) {
                return rc;
            }

            auto p_disconnect_handler = [this](util::String topic_name,
                                               std::shared_ptr<DisconnectCallbackContextData> p_app_handler_data) {
                return DisconnectCallback(topic_name, p_app_handler_data);
            };

            auto p_reconnect_handler = [this](util::String client_id,
                                              std::shared_ptr<ReconnectCallbackContextData> p_app_handler_data,
                                              ResponseCode reconnect_result) {
                return ReconnectCallback(client_id, p_app_handler_data, reconnect_result);
            };

            auto p_resubscribe_handler = [this](util::String client_id,
                                                std::shared_ptr<ResubscribeCallbackContextData> p_app_handler_data,
                                                ResponseCode resubscribe_result) {
                return ResubscribeCallback(client_id, p_app_handler_data, resubscribe_result);
            };

            p_iot_client_ = std::shared_ptr<MqttClient>(MqttClient::Create(p_network_connection_,
                                                                           ConfigCommon::mqtt_command_timeout_,
                                                                           p_disconnect_handler, nullptr,
                                                                           p_reconnect_handler, nullptr,
                                                                           p_resubscribe_handler, nullptr));
            if (nullptr == p_iot_client_) {
                return ResponseCode::FAILURE;
            }

            std::stringstream client_id_tagged;
            client_id_tagged << ConfigCommon::base_client_id_ << "_jobs_agent_" << std::to_string(rand());
            std::unique_ptr<Utf8String> client_id = Utf8String::Create(client_id_tagged.str());

            rc = p_iot_client_->Connect(ConfigCommon::mqtt_command_timeout_, ConfigCommon::is_clean_session_,
                                        mqtt::Version::MQTT_3_1_1, ConfigCommon::keep_alive_timeout_secs_,
                                        std::move(client_id), nullptr, nullptr, nullptr);
            if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED != rc) {
                return rc;
            }

            StartInstalledPackages();

            p_jobs_ = Jobs::Create(p_iot_client_, mqtt::QoS::QOS1, ConfigCommon::thing_name_, client_id_tagged.str());

            rc = Subscribe();
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOGSTREAM_ERROR(LOG_TAG_JOBS_AGENT, "Subscribe failed. " << ResponseHelper::ToString(rc).c_str());
            } else {
                rc = p_jobs_->SendJobsQuery(Jobs::JOB_GET_PENDING_TOPIC);

                if (ResponseCode::SUCCESS == rc) {
                    rc = p_jobs_->SendJobsQuery(Jobs::JOB_DESCRIBE_TOPIC, "$next");
                }

                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOGSTREAM_ERROR(LOG_TAG_JOBS_AGENT, "SendJobsQuery failed. " << ResponseHelper::ToString(rc).c_str());
                    p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
                }
            }

            // Wait for job processing to complete. To cause agent to exit add call to cv_done_.notify_one() when
            // exit condition is satisfied.
            std::unique_lock<std::mutex> lk(m_);
            cv_done_.wait(lk);

            rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOGSTREAM_ERROR(LOG_TAG_JOBS_AGENT, "Disconnect failed. " << ResponseHelper::ToString(rc).c_str());
            }

            std::cout << "Exiting Sample!!!!" << std::endl;
            return ResponseCode::SUCCESS;
        }
    }
}

int main(int argc, char **argv) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::shared_ptr<awsiotsdk::util::Logging::ConsoleLogSystem> p_log_system =
        std::make_shared<awsiotsdk::util::Logging::ConsoleLogSystem>(awsiotsdk::util::Logging::LogLevel::Info);
    awsiotsdk::util::Logging::InitializeAWSLogging(p_log_system);

    std::unique_ptr<awsiotsdk::samples::JobsAgent>
        jobs_agent = std::unique_ptr<awsiotsdk::samples::JobsAgent>(new awsiotsdk::samples::JobsAgent());

    awsiotsdk::ResponseCode rc = awsiotsdk::ConfigCommon::InitializeCommon("config/SampleConfig.json");
    if (awsiotsdk::ResponseCode::SUCCESS == rc) {
        rc = jobs_agent->RunAgent(argv[0]);
    }
#ifdef WIN32
    std::cout<<"Press any key to continue!!!!"<<std::endl;
    getchar();
#endif

    awsiotsdk::util::Logging::ShutdownAWSLogging();
    return static_cast<int>(rc);
}
