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
 * @file ConfigCommonTests.cpp
 * @brief
 *
 */

#include <gtest/gtest.h>

#include "TestHelper.hpp"

#include "util/JsonParser.hpp"
#include "ConfigCommon.hpp"

#define NUMBER_OF_CONFIGURATION_FIELDS 25

namespace awsiotsdk {
    namespace tests {
        namespace unit {
            class ConfigCommonTester : public ::testing::Test {

            protected:
                static const util::String configuration_line_list[NUMBER_OF_CONFIGURATION_FIELDS];

            public:
                ConfigCommonTester() {}
            };

            const util::String ConfigCommonTester::configuration_line_list[NUMBER_OF_CONFIGURATION_FIELDS] = {
                "\n\"endpoint\": \"\"",
                "\n\"mqtt_port\": 8883",
                "\n\"https_port\": 443",
                "\n\"greengrass_discovery_port\": 8443",
                "\n\"root_ca_relative_path\": \"certs/rootCA.crt\"",
                "\n\"device_certificate_relative_path\": \"certs/cert.pem\"",
                "\n\"device_private_key_relative_path\": \"certs/privkey.pem\"",
                "\n\"tls_handshake_timeout_msecs\": 60000",
                "\n\"tls_read_timeout_msecs\": 2000",
                "\n\"tls_write_timeout_msecs\": 2000",
                "\n\"aws_region\": \"\"",
                "\n\"aws_access_key_id\": \"\"",
                "\n\"aws_secret_access_key\": \"\"",
                "\n\"aws_session_token\": \"\"",
                "\n\"client_id\": \"CppSDKTesting\"",
                "\n\"thing_name\": \"CppSDKTesting\"",
                "\n\"is_clean_session\": true",
                "\n\"mqtt_command_timeout_msecs\": 20000",
                "\n\"keepalive_interval_secs\": 30",
                "\n\"minimum_reconnect_interval_secs\": 1",
                "\n\"maximum_reconnect_interval_secs\": 128",
                "\n\"maximum_acks_to_wait_for\": 32",
                "\n\"action_processing_rate_hz\": 5",
                "\n\"maximum_outgoing_action_queue_length\": 32",
                "\n\"discover_action_timeout_msecs\": 300000"
            };

            TEST_F(ConfigCommonTester, ErrorTests) {
                util::String file_name = "config_common_test.json";
                for (int i = 0; i < NUMBER_OF_CONFIGURATION_FIELDS; ++i) {
                    util::String test_json_string = "{";
                    for (int j = 0; j < NUMBER_OF_CONFIGURATION_FIELDS; ++j) {
                        if (j != i) {
                            test_json_string.append(ConfigCommonTester::configuration_line_list[j]);
                            if (j != (NUMBER_OF_CONFIGURATION_FIELDS - 1)) {
                                if (!(j == (NUMBER_OF_CONFIGURATION_FIELDS - 2) && i == (NUMBER_OF_CONFIGURATION_FIELDS - 1))) {
                                    test_json_string.append(",");
                                }
                            }
                        }
                    }
                    test_json_string.append("\n}");

                    util::JsonDocument new_document;
                    ResponseCode rc = util::JsonParser::InitializeFromJsonString(new_document, test_json_string);

                    EXPECT_EQ(ResponseCode::SUCCESS, rc);
                    util::String current_working_directory = ConfigCommon::GetCurrentPath();
                    EXPECT_NE(0U, current_working_directory.length());
#ifdef WIN32
                    current_working_directory.append("\\");
#else
                    current_working_directory.append("/");
#endif
                    // Write complete Discovery Response JSON out to a file
                    util::String config_common_output_path = current_working_directory;
                    config_common_output_path.append(file_name);
                    rc = util::JsonParser::WriteToFile(new_document, config_common_output_path);

                    EXPECT_EQ(ResponseCode::SUCCESS, rc);

                    rc = ConfigCommon::InitializeCommon(file_name);

                    EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR, rc);
                }
                remove(file_name.c_str());
            }

            TEST_F(ConfigCommonTester, RunHappyPathTest) {
                const util::String file_name = "config_common_test.json";
                util::String test_json_string = "{";
                for (int i = 0; i < NUMBER_OF_CONFIGURATION_FIELDS; ++i) {
                    test_json_string.append(ConfigCommonTester::configuration_line_list[i]);
                    if ((NUMBER_OF_CONFIGURATION_FIELDS - 1) != i) {
                        test_json_string.append(",");
                    }
                }
                test_json_string.append("\n}");

                util::JsonDocument new_document;
                ResponseCode rc = util::JsonParser::InitializeFromJsonString(new_document, test_json_string);

                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                util::String current_working_directory = ConfigCommon::GetCurrentPath();
                EXPECT_NE(0U, current_working_directory.length());
#ifdef WIN32
                current_working_directory.append("\\");
#else
                current_working_directory.append("/");
#endif
                // Write complete Discovery Response JSON out to a file
                util::String config_common_output_path = current_working_directory;
                config_common_output_path.append(file_name);
                rc = util::JsonParser::WriteToFile(new_document, config_common_output_path);

                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                rc = ConfigCommon::InitializeCommon(file_name);

                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                remove(file_name.c_str());
            }

            TEST_F(ConfigCommonTester, InvalidConfigFileTest) {
                const util::String invalid_file_name = "empty_file.json";
                ResponseCode rc = ConfigCommon::InitializeCommon(invalid_file_name);
                EXPECT_NE(ResponseCode::SUCCESS, rc);
            }
        }
    }
}

