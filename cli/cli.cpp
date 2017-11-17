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
 * @file cli.cpp
 * @brief
 *
 */

#include <getopt.h>
#include <cstring>
#include <chrono>

#ifdef USE_WEBSOCKETS
#include "WebSocketConnection.hpp"
#elif defined USE_MBEDTLS
#include "MbedTLSConnection.hpp"
#else
#include "OpenSSLConnection.hpp"
#endif

#include "util/logging/LogMacros.hpp"
#include "util/JsonParser.hpp"
#include "cli.hpp"
#include "ConfigCommon.hpp"

#define CLI_LOG_TAG "[CLI]"

using namespace awsiotsdk;

namespace cppsdkcli {
    ResponseCode CLI::InitializeTLS() {
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
            AWS_LOG_ERROR(CLI_LOG_TAG,
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
                                                                             ConfigCommon::tls_write_timeout_, true);
        if (nullptr == p_network_connection_) {
            AWS_LOG_ERROR(CLI_LOG_TAG,
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
            AWS_LOG_ERROR(CLI_LOG_TAG,
                          "Failed to initialize Network Connection. %s",
                          ResponseHelper::ToString(rc).c_str());
            rc = ResponseCode::FAILURE;
        } else {
            p_network_connection_ = std::dynamic_pointer_cast<NetworkConnection>(p_network_connection);
        }
#endif

        return rc;
    }

    ResponseCode CLI::InitializeCliConfig() {
        ResponseCode rc = awsiotsdk::ConfigCommon::InitializeCommon("CliConfig.json");
        if (ResponseCode::SUCCESS != rc) {
            AWS_LOG_ERROR(CLI_LOG_TAG, "Initialize Test Config Failed. %s", ResponseHelper::ToString(rc).c_str());
        }
        return rc;
    }

    ResponseCode CLI::InitializeCLI(int argc, char **argv) {
        int opt;
        int long_index = 0;
        static struct option long_options[] = {
            {"subscribe", no_argument, 0, 's'},
            {"publish", no_argument, 0, 'p'},
            {"topic", required_argument, 0, 't'},
            {"qos", required_argument, 0, 'q'},
            {"endpoint", required_argument, 0, 'e'},
            {"port", required_argument, 0, 'r'},
            {0, 0, 0, 0}
        };

        InitializeCliConfig();

        while (-1 != (opt = getopt_long(argc, argv, "spt:q:c:e:r:", long_options, &long_index))) {
            switch (opt) {
                case 's':
                    is_subscribe_ = true;
                    std::cout << "Subscribe" << std::endl;
                    break;
                case 'p':
                    is_publish_ = true;
                    std::cout << "Publish" << std::endl;
                    break;
                case 'e':
                    endpoint_ = util::String(optarg, MAX_PATH_LENGTH_);
                    std::cout << "Host : " << optarg << std::endl;
                    break;
                case 'r':
                    port_ = atoi(optarg);
                    std::cout << "Port : " << optarg << std::endl;
                    break;
                case 't':
                    snprintf(topic_, MAX_PATH_LENGTH_, "%s", optarg);
                    std::cout << "Topic : " << optarg << std::endl;
                    break;
                case 'q':
                    qos_ = atoi(optarg);
                    std::cout << "Publish Count :" << optarg << " times" << std::endl;
                    break;
                default:
                    std::cout << "Error in command line argument parsing" << std::endl;
                    break;
            }
        }

        // Atleast one option should be selected but not both
        if (!(is_publish_ ^ is_subscribe_)) {
            return ResponseCode::FAILURE;
        }

        InitializeTLS();

        p_iot_client_ = MqttClient::Create(p_network_connection_, ConfigCommon::mqtt_command_timeout_);

        return ResponseCode::SUCCESS;
    }

    ResponseCode CLI::Connect() {
        util::String client_id_cstr_tagged = ConfigCommon::base_client_id_;

        if (is_subscribe_) {
            client_id_cstr_tagged.append("_subscribe");
        } else if (is_publish_) {
            client_id_cstr_tagged.append("_publish");
        } else {
            return ResponseCode::FAILURE;
        }
        std::unique_ptr<Utf8String> client_id = Utf8String::Create(client_id_cstr_tagged);

        return p_iot_client_->Connect(ConfigCommon::mqtt_command_timeout_, ConfigCommon::is_clean_session_,
                                      mqtt::Version::MQTT_3_1_1, ConfigCommon::keep_alive_timeout_secs_,
                                      std::move(client_id), nullptr, nullptr, nullptr);
    }

    ResponseCode CLI::RunPublish() {
        std::cout << "Entering Publish!" << std::endl;
        ResponseCode rc;
        uint16_t packet_id = 0;
        bool run_thread = true;
        util::String p_topic_name_str;
        util::String str;
        util::String payload_str;

        do {
            if (!p_topic_name_str.empty()) {
                std::cout << "Publish to same topic (" << p_topic_name_str << ") <yes/no>?";
                std::getline(std::cin, str);
                if (str != "yes" && str != "y" && str != "Y") {
                    std::cout << std::endl << "Enter topic name : ";
                    std::getline(std::cin, p_topic_name_str);
                }
            } else {
                std::cout << std::endl << "Enter topic name : ";
                std::getline(std::cin, p_topic_name_str);
            }

            std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);

            if (!payload_str.empty()) {
                std::cout << "Use name payload (" << payload_str << ") <yes/no>?";
                std::getline(std::cin, str);
                if (str != "yes" && str != "y" && str != "Y") {
                    std::cout << std::endl << "Enter new payload : ";
                    std::getline(std::cin, payload_str);
                }
            } else {
                std::cout << std::endl << "Enter new payload : ";
                std::getline(std::cin, payload_str);
            }
            rc = p_iot_client_->PublishAsync(std::move(p_topic_name), false, false, mqtt::QoS::QOS1,
                                             payload_str, nullptr, packet_id);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Publish Packet Id : " << packet_id << std::endl;
            std::cout << "Publish more messages <yes/no>? " << std::flush;
            std::getline(std::cin, str);
            run_thread = (str == "yes" || str == "y" || str == "Y");
        } while (run_thread);

        return rc;
    }

    ResponseCode CLI::RunPublish(int msg_count) {
        std::cout << "Entering Publish!" << std::endl;
        ResponseCode rc;
        util::String p_topic_name_str;
        int itr = 0;

        std::cout << std::endl << "Enter topic name : ";
        std::getline(std::cin, p_topic_name_str);
        int error_count = 0;

        do {
            util::String payload = "Hello from SDK : ";
            payload.append(std::to_string(itr));
            std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
            rc = p_iot_client_->Publish(std::move(p_topic_name), false, false,
                                        mqtt::QoS::QOS1, payload, std::chrono::milliseconds(2000));
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Publish Payload : " << payload << std::endl;
            std::cout << "Publish Response : " << ResponseHelper::ToString(rc) << std::endl;
            if (ResponseCode::SUCCESS != rc && error_count > 5) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                rc = ResponseCode::SUCCESS;
                continue;
            }
            error_count = 0;
        } while (++itr < msg_count && ResponseCode::SUCCESS == rc);

        return rc;
    }

    ResponseCode CLI::SubscribeCallback(util::String topic_name,
                                        util::String payload,
                                        std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
        std::cout << std::endl << "Received message on topic : " << topic_name << std::endl;
        std::cout << "Message Payload : " << payload << std::endl;
        return ResponseCode::SUCCESS;
    }

    ResponseCode CLI::Subscribe(std::unique_ptr<Utf8String> p_topic_name, mqtt::QoS qos) {
        std::cout << "Entering Subscribe!" << std::endl;
        uint16_t packet_id = 0;

        std::shared_ptr<mqtt::Subscription>
            p_subscription = mqtt::Subscription::Create(std::move(p_topic_name), qos, &SubscribeCallback, nullptr);
        util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;

        topic_vector.push_back(p_subscription);

        ResponseCode rc = p_iot_client_->SubscribeAsync(topic_vector, nullptr, packet_id);
        std::cout << "Subscribe Packet Id : " << packet_id << std::endl;

        return rc;
    }

    ResponseCode CLI::RunSubscribe() {
        ResponseCode rc = ResponseCode::SUCCESS;
        bool run_thread = true;
        util::String str;
        util::String p_topic_name_str;
        mqtt::QoS qos = mqtt::QoS::QOS1;

        std::cout << std::endl << "Enter topic name : ";
        std::getline(std::cin, p_topic_name_str);

        rc = Subscribe(Utf8String::Create(p_topic_name_str), qos);
        do {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            std::cout << "Keep thread running <yes/no>? " << std::flush;
            std::getline(std::cin, str);
            run_thread = (str == "yes" || str == "y" || str == "Y");
        } while (run_thread);
        return rc;
    }

    ResponseCode CLI::RunCLI() {
        ResponseCode rc = Connect();

        if (awsiotsdk::ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED != rc) {
            AWS_LOG_ERROR(CLI_LOG_TAG, "Connect failed. %s", ResponseHelper::ToString(rc).c_str());
            return rc;
        }

        AWS_LOG_INFO(CLI_LOG_TAG, "Connected Successfully!!");
        if (is_publish_) {
            rc = RunPublish(100);
        } else if (is_subscribe_) {
            rc = RunSubscribe();
        }

        return rc;
    }
}

int main(int argc, char **argv) {
    std::unique_ptr<cppsdkcli::CLI> p_cli = std::unique_ptr<cppsdkcli::CLI>(new cppsdkcli::CLI());

    ResponseCode rc = p_cli->InitializeCLI(argc, argv);
    if (ResponseCode::SUCCESS != rc) {
        return -1;
    }

    rc = p_cli->RunCLI();
    if (ResponseCode::SUCCESS != rc) {
        return -1;
    }

    return 0;
}