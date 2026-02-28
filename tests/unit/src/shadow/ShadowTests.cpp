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
 * @file ShadowTests.cpp
 * @brief
 *
 */

#include <chrono>

#include <gtest/gtest.h>

#include "TestHelper.hpp"
#include "MockNetworkConnection.hpp"

#include "mqtt/NetworkRead.hpp"

#include "mqtt/GreengrassMqttClient.hpp"

#include "shadow/Shadow.hpp"

#define CONNECT_FIXED_HEADER_VAL 0x10
#define DISCONNECT_FIXED_HEADER_VAL 0xE0

#define KEEP_ALIVE_TIMEOUT_SECS 30

#define SUBSCRIPTION_SETTING_TIME_SECS 2

#define KEEP_ALIVE_TIMEOUT_SECS 30
#define MQTT_COMMAND_TIMEOUT_MSECS 20000

#define SHADOW_REQUEST_TYPE_GET_STRING "get"
#define SHADOW_REQUEST_TYPE_UPDATE_STRING "update"
#define SHADOW_REQUEST_TYPE_DELETE_STRING "delete"
#define SHADOW_REQUEST_TYPE_DELTA_STRING "delta"

#define SHADOW_RESPONSE_TYPE_ACCEPTED_STRING "accepted"
#define SHADOW_RESPONSE_TYPE_REJECTED_STRING "rejected"
#define SHADOW_RESPONSE_TYPE_DELTA_STRING "delta"

#define SHADOW_TOPIC_PREFIX "$aws/things/"
#define SHADOW_TOPIC_MIDDLE "/shadow/"

#define SHADOW_DOCUMENT_EMPTY_TEMPLATE "{" \
"    \"state\" : {" \
"        \"desired\" : {" \
"        }," \
"        \"reported\" : {" \
"        }" \
"    }," \
"    \"version\" : 0," \
"    \"clientToken\" : \"empty\"," \
"    \"timestamp\": 0" \
"}"

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

#define SHADOW_DOCUMENT_MODIFIED_VALUE_STRING  "{" \
"    \"state\" : {" \
"        \"desired\" : {" \
"           \"cur_msg_count\" : 5" \
"        }," \
"        \"reported\" : {" \
"        	\"cur_msg_count\" : 10" \
"        }" \
"    }," \
"    \"version\" : 12," \
"    \"clientToken\" : \"shadow_test_client\"," \
"    \"timestamp\": 12345" \
"}"

#define MODIFIED_VALUE_VERSION 12

#define SHADOW_DOCUMENT_MODIFIED_VALUE_STRING_V2  "{" \
"    \"state\" : {" \
"        \"desired\" : {" \
"           \"cur_msg_count\" : 7" \
"        }," \
"        \"reported\" : {" \
"        	\"cur_msg_count\" : 11" \
"        }" \
"    }," \
"    \"version\" : 15," \
"    \"clientToken\" : \"shadow_test_client\"," \
"    \"timestamp\": 12345" \
"}"

#define SHADOW_DOCUMENT_MODIFIED_VALUE_STRING_V3  "{" \
"    \"state\" : {" \
"        \"desired\" : {" \
"           \"cur_msg_count\" : 10" \
"        }," \
"        \"reported\" : {" \
"        	\"cur_msg_count\" : 13" \
"        }" \
"    }," \
"    \"version\" : 18," \
"    \"clientToken\" : \"shadow_test_client\"," \
"    \"timestamp\": 12345" \
"}"

#define BROKEN_SHADOW_DOCUMENT_WITH_INVALID_VERSION_KEY "{" \
"    \"state\" : {" \
"        \"desired\" : {" \
"           \"cur_msg_count\" : 5" \
"        }," \
"        \"reported\" : {" \
"        	\"cur_msg_count\" : 10" \
"        }" \
"    }," \
"    \"version\" : \"weird_version\"," \
"    \"clientToken\" : \"shadow_test_client\"," \
"    \"timestamp\": 12345" \
"}"

#define BROKEN_SHADOW_DOCUMENT_WITH_NO_VERSION_KEY "{" \
"    \"state\" : {" \
"        \"desired\" : {" \
"           \"cur_msg_count\" : 5" \
"        }," \
"        \"reported\" : {" \
"        	\"cur_msg_count\" : 10" \
"        }" \
"    }," \
"    \"clientToken\" : \"shadow_test_client\"," \
"    \"timestamp\": 12345" \
"}"

#define BROKEN_SHADOW_DOCUMENT_WITH_NO_STATE_KEY  "{" \
"    \"version\" : 12," \
"    \"clientToken\" : \"shadow_test_client\"," \
"    \"timestamp\": 12345" \
"}"

#define SHADOW_REPORTED_DOC "{" \
"           \"cur_msg_count\" : 10" \
"}"

#define SHADOW_DESIRED_DOC "{" \
"           \"cur_msg_count\" : 5" \
"}"


#define SHADOW_DOCUMENT_STATE_KEY "state"
#define SHADOW_DOCUMENT_REPORTED_KEY "reported"
#define SHADOW_DOCUMENT_DESIRED_KEY "desired"
#define SHADOW_DOCUMENT_CLIENT_TOKEN_KEY "clientToken"
#define SHADOW_DOCUMENT_VERSION_KEY "version"
#define SHADOW_DOCUMENT_TIMESTAMP_KEY "timestamp"

#define SHADOW_REQUEST_TYPE_GET_STRING "get"
#define SHADOW_REQUEST_TYPE_UPDATE_STRING "update"
#define SHADOW_REQUEST_TYPE_DELETE_STRING "delete"
#define SHADOW_REQUEST_TYPE_DELTA_STRING "delta"

#define SHADOW_RESPONSE_TYPE_ACCEPTED_STRING "accepted"
#define SHADOW_RESPONSE_TYPE_REJECTED_STRING "rejected"
#define SHADOW_RESPONSE_TYPE_DELTA_STRING "delta"

#define SHADOW_LOG_TAG "[Shadow]"

namespace awsiotsdk {
    namespace tests {
        namespace unit {
            class ShadowTester : public ::testing::Test {
            protected:
                std::shared_ptr<mqtt::ClientState> p_core_state_;
                std::shared_ptr<tests::mocks::MockNetworkConnection> p_network_connection_;
                tests::mocks::MockNetworkConnection *p_network_mock_;

                static const util::String test_client_id_;
                static const std::chrono::seconds keep_alive_timeout_;
                std::shared_ptr<GreengrassMqttClient> p_iot_greengrass_client_;
                util::String p_thing_name_;
                static const std::chrono::milliseconds mqtt_command_timeout_;
                std::unique_ptr<Shadow> p_test_shadow;

                static const util::String test_topic_name_;

                ShadowTester() {
                    p_core_state_ = mqtt::ClientState::Create(std::chrono::milliseconds(200));
                    p_network_connection_ = std::make_shared<tests::mocks::MockNetworkConnection>();
                    p_network_mock_ = p_network_connection_.get();
                    p_iot_greengrass_client_ =
                        std::shared_ptr<GreengrassMqttClient>(GreengrassMqttClient::Create(p_network_connection_,
                                                                                           std::chrono::milliseconds(
                                                                                               2000)));
                    p_thing_name_ = "ShadowUnitTestThing";
                }
            };

            const util::String ShadowTester::test_client_id_ = "CppSdkTestClient";
            const util::String ShadowTester::test_topic_name_ = "SdkTest";
            const std::chrono::seconds
                ShadowTester::keep_alive_timeout_ = std::chrono::seconds(KEEP_ALIVE_TIMEOUT_SECS);
            const std::chrono::milliseconds ShadowTester::mqtt_command_timeout_ =
                std::chrono::milliseconds(MQTT_COMMAND_TIMEOUT_MSECS);

            TEST_F(ShadowTester, ShadowCreateTest) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(nullptr, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_EQ(nullptr, test_shadow);

                util::String empty_thing_name = "";
                test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                             empty_thing_name, empty_thing_name);
                EXPECT_EQ(nullptr, test_shadow);

                test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                             p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                             p_thing_name_, empty_thing_name);
                EXPECT_NE(nullptr, test_shadow);
            }

            TEST_F(ShadowTester, TestShadowHandleGetResponseErrors) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                util::JsonDocument test_empty_document;

                ResponseCode rc = test_shadow->HandleGetResponse(ShadowResponseType::Delta, test_empty_document);
                EXPECT_EQ(ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TYPE, rc);

                rc = test_shadow->HandleGetResponse(ShadowResponseType::Accepted, test_empty_document);
                EXPECT_EQ(ResponseCode::SHADOW_UNEXPECTED_RESPONSE_PAYLOAD, rc);
            }

            TEST_F(ShadowTester, TestShadowUpdateDeviceShadowErrorCases) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                util::JsonDocument test_empty_document;

                ResponseCode rc = test_shadow->UpdateDeviceShadow(test_empty_document);
                EXPECT_EQ(ResponseCode::SHADOW_JSON_EMPTY_ERROR, rc);
            }

            TEST_F(ShadowTester, TestShadowSubscriptionHandlerError) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                util::String non_json_payload = "Not a json";
                ResponseCode rc = test_shadow->SubscriptionHandler("Some random topic", non_json_payload, nullptr);
                EXPECT_NE(ResponseCode::SUCCESS, rc);

                util::String json_payload(SHADOW_DOCUMENT_EMPTY_STRING);
                util::String non_shadow_topic = "Non shadow topic";
                rc = test_shadow->SubscriptionHandler(non_shadow_topic, json_payload, nullptr);
                EXPECT_EQ(ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TYPE, rc);
            }


            TEST_F(ShadowTester, TestShadowSubscriptionHandlerForValidTopics) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                util::String get_base_topic = SHADOW_TOPIC_PREFIX + p_thing_name_ + SHADOW_TOPIC_MIDDLE  +
                    SHADOW_REQUEST_TYPE_GET_STRING + "/";

                util::String get_rejected_topic = get_base_topic + SHADOW_RESPONSE_TYPE_REJECTED_STRING;
                util::String get_accepted_topic = get_base_topic + SHADOW_RESPONSE_TYPE_ACCEPTED_STRING;
                util::String get_delta_topic = get_base_topic + SHADOW_RESPONSE_TYPE_DELTA_STRING;

                ResponseCode rc = test_shadow->SubscriptionHandler(get_accepted_topic, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING,
                                                                   nullptr);
                EXPECT_EQ(ResponseCode::SHADOW_REQUEST_ACCEPTED, rc);

                rc = test_shadow->SubscriptionHandler(get_rejected_topic, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING, nullptr);
                EXPECT_EQ(ResponseCode::SHADOW_REQUEST_REJECTED, rc);

                rc = test_shadow->SubscriptionHandler(get_delta_topic, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING, nullptr);
                EXPECT_EQ(ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TYPE, rc);

                util::String update_base_topic = SHADOW_TOPIC_PREFIX + p_thing_name_ + SHADOW_TOPIC_MIDDLE  +
                    SHADOW_REQUEST_TYPE_UPDATE_STRING + "/";

                util::String udpate_rejected_topic = update_base_topic + SHADOW_RESPONSE_TYPE_REJECTED_STRING;
                util::String update_accepted_topic = update_base_topic + SHADOW_RESPONSE_TYPE_ACCEPTED_STRING;
                util::String update_delta_topic = update_base_topic + SHADOW_RESPONSE_TYPE_DELTA_STRING;

                rc = test_shadow->SubscriptionHandler(update_accepted_topic, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING_V2,
                                                                   nullptr);
                EXPECT_EQ(ResponseCode::SHADOW_REQUEST_ACCEPTED, rc);

                rc = test_shadow->SubscriptionHandler(udpate_rejected_topic, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING, nullptr);
                EXPECT_EQ(ResponseCode::SHADOW_REQUEST_REJECTED, rc);

                rc = test_shadow->SubscriptionHandler(update_delta_topic, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING_V3, nullptr);
                EXPECT_EQ(ResponseCode::SHADOW_RECEIVED_DELTA, rc);

                util::String delete_base_topic = SHADOW_TOPIC_PREFIX + p_thing_name_ + SHADOW_TOPIC_MIDDLE  +
                    SHADOW_REQUEST_TYPE_DELETE_STRING + "/";

                util::String delete_rejected_topic = delete_base_topic + SHADOW_RESPONSE_TYPE_REJECTED_STRING;
                util::String delete_accepted_topic = delete_base_topic + SHADOW_RESPONSE_TYPE_ACCEPTED_STRING;
                util::String delete_delta_topic = delete_base_topic + SHADOW_RESPONSE_TYPE_DELTA_STRING;

                rc = test_shadow->SubscriptionHandler(delete_accepted_topic, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING_V2,
                                                      nullptr);
                EXPECT_EQ(ResponseCode::SHADOW_REQUEST_ACCEPTED, rc);

                rc = test_shadow->SubscriptionHandler(delete_rejected_topic, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING, nullptr);
                EXPECT_EQ(ResponseCode::SHADOW_REQUEST_REJECTED, rc);

                rc = test_shadow->SubscriptionHandler(delete_delta_topic, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING_V3, nullptr);
                EXPECT_EQ(ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TYPE, rc);
            }


            TEST_F(ShadowTester, TestShadowHandleGetResponseWithValidPayload) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                util::JsonDocument test_payload;
                ResponseCode rc =
                    util::JsonParser::InitializeFromJsonString(test_payload, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                rc = test_shadow->HandleGetResponse(ShadowResponseType::Accepted, test_payload);
                EXPECT_EQ(ResponseCode::SHADOW_REQUEST_ACCEPTED, rc);

                rc = util::JsonParser::InitializeFromJsonString(test_payload,
                                                                BROKEN_SHADOW_DOCUMENT_WITH_NO_VERSION_KEY);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                rc = test_shadow->HandleGetResponse(ShadowResponseType::Accepted, test_payload);
                EXPECT_NE(ResponseCode::SHADOW_REQUEST_ACCEPTED, rc);

                rc = test_shadow->HandleGetResponse(ShadowResponseType::Rejected, test_payload);
                EXPECT_EQ(ResponseCode::SHADOW_REQUEST_REJECTED, rc);
            }

            TEST_F(ShadowTester, TestShadowHandleUpdateResponse) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                util::JsonDocument test_payload;

                ResponseCode rc = test_shadow->HandleUpdateResponse(ShadowResponseType::Rejected, test_payload);
                EXPECT_EQ(ResponseCode::SHADOW_REQUEST_REJECTED, rc);

                util::JsonDocument invalid_payload;
                rc = test_shadow->HandleUpdateResponse(ShadowResponseType::Accepted, invalid_payload);
                EXPECT_EQ(ResponseCode::SHADOW_UNEXPECTED_RESPONSE_PAYLOAD, rc);

                rc = util::JsonParser::InitializeFromJsonString(invalid_payload,
                                                                BROKEN_SHADOW_DOCUMENT_WITH_NO_STATE_KEY);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                rc = test_shadow->HandleUpdateResponse(ShadowResponseType::Accepted, invalid_payload);
                EXPECT_EQ(ResponseCode::SHADOW_UNEXPECTED_RESPONSE_PAYLOAD, rc);

                rc = util::JsonParser::InitializeFromJsonString(test_payload,
                                                                BROKEN_SHADOW_DOCUMENT_WITH_INVALID_VERSION_KEY);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                rc = test_shadow->HandleUpdateResponse(ShadowResponseType::Accepted, test_payload);
                EXPECT_NE(ResponseCode::SHADOW_REQUEST_ACCEPTED, rc);

                rc = util::JsonParser::InitializeFromJsonString(test_payload, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                rc = test_shadow->HandleUpdateResponse(ShadowResponseType::Accepted, test_payload);
                EXPECT_EQ(ResponseCode::SHADOW_REQUEST_ACCEPTED, rc);

                rc = test_shadow->HandleUpdateResponse(ShadowResponseType::Accepted, test_payload);
                EXPECT_EQ(ResponseCode::SHADOW_RECEIVED_OLD_VERSION_UPDATE, rc);

                rc = test_shadow->HandleUpdateResponse(ShadowResponseType::Accepted, test_payload);
                EXPECT_NE(ResponseCode::SHADOW_REQUEST_ACCEPTED, rc);

                rc = util::JsonParser::InitializeFromJsonString(test_payload, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING_V2);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                rc = test_shadow->HandleUpdateResponse(ShadowResponseType::Delta, test_payload);
                EXPECT_EQ(ResponseCode::SHADOW_RECEIVED_DELTA, rc);
            }

            TEST_F(ShadowTester, TestShadowHandleDeleteResponse) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                util::JsonDocument test_payload;
                ResponseCode rc =
                    util::JsonParser::InitializeFromJsonString(test_payload, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                rc = test_shadow->HandleDeleteResponse(ShadowResponseType::Delta, test_payload);
                EXPECT_EQ(ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TYPE, rc);

                rc = test_shadow->HandleDeleteResponse(ShadowResponseType::Rejected, test_payload);
                EXPECT_EQ(ResponseCode::SHADOW_REQUEST_REJECTED, rc);

                rc = test_shadow->HandleDeleteResponse(ShadowResponseType::Accepted, test_payload);
                EXPECT_EQ(ResponseCode::SHADOW_REQUEST_ACCEPTED, rc);
            }

            TEST_F(ShadowTester, TestShadowUpdateDeviceShadow) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                util::JsonDocument test_payload;
                ResponseCode rc = test_shadow->UpdateDeviceShadow(test_payload);
                EXPECT_EQ(ResponseCode::SHADOW_JSON_EMPTY_ERROR, rc);

                rc = util::JsonParser::InitializeFromJsonString(test_payload, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                rc = test_shadow->UpdateDeviceShadow(test_payload);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
            }

            TEST_F(ShadowTester, TestShadowGetFunctions) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                util::JsonDocument test_payload;
                ResponseCode rc =
                    util::JsonParser::InitializeFromJsonString(test_payload, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                rc = test_shadow->UpdateDeviceShadow(test_payload);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                util::JsonDocument expected_doc;
                util::JsonDocument shadow_doc;

                rc = util::JsonParser::InitializeFromJsonString(expected_doc, SHADOW_DESIRED_DOC);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                shadow_doc = test_shadow->GetDeviceDesired();
                EXPECT_EQ(shadow_doc, expected_doc);

                rc = util::JsonParser::InitializeFromJsonString(expected_doc, SHADOW_REPORTED_DOC);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                shadow_doc = test_shadow->GetDeviceReported();
                EXPECT_EQ(shadow_doc, expected_doc);


                rc = util::JsonParser::InitializeFromJsonString(expected_doc, SHADOW_DOCUMENT_MODIFIED_VALUE_STRING);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                shadow_doc = test_shadow->GetDeviceDocument();
                EXPECT_EQ(shadow_doc, expected_doc);

                rc = util::JsonParser::InitializeFromJsonString(expected_doc, "{}");
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                shadow_doc = test_shadow->GetServerDocument();
                EXPECT_EQ(shadow_doc, expected_doc);

           }

            TEST_F(ShadowTester, TestShadowResetClientTokenSuffix) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                test_shadow->ResetClientTokenSuffix();
            }

            TEST_F(ShadowTester, TestShadowGetCurrrentVersionNumber) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                uint32_t shadow_version = test_shadow->GetCurrentVersionNumber();
                EXPECT_EQ(uint32_t(0), shadow_version);
            }

            TEST_F(ShadowTester, TestShadowIsInSyncFail) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                EXPECT_FALSE(test_shadow->IsInSync());
            }

            TEST_F(ShadowTester, TestShadowPerformAsyncOperationsDisconnected) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                ResponseCode rc = test_shadow->PerformDeleteAsync();
                EXPECT_EQ(ResponseCode::SHADOW_MQTT_DISCONNECTED_ERROR, rc);

                rc = test_shadow->PerformUpdateAsync();
                EXPECT_EQ(ResponseCode::SHADOW_MQTT_DISCONNECTED_ERROR, rc);

                rc = test_shadow->PerformGetAsync();
                EXPECT_EQ(ResponseCode::SHADOW_MQTT_DISCONNECTED_ERROR, rc);
            }

            TEST_F(ShadowTester, TestShadowConstructorAndNullClientCases) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                Shadow test_shadow(nullptr, mqtt_command_timeout_,
                                   p_thing_name_, p_thing_name_);

                util::Map<ShadowRequestType , Shadow::RequestHandlerPtr> mapping;
                ResponseCode rc = test_shadow.AddShadowSubscription(mapping);
                EXPECT_EQ(ResponseCode::SHADOW_MQTT_CLIENT_NOT_SET_ERROR, rc);

                rc = test_shadow.PerformGetAsync();
                EXPECT_EQ(ResponseCode::SHADOW_MQTT_CLIENT_NOT_SET_ERROR, rc);

                rc = test_shadow.PerformUpdateAsync();
                EXPECT_EQ(ResponseCode::SHADOW_MQTT_CLIENT_NOT_SET_ERROR, rc);

                rc = test_shadow.PerformDeleteAsync();
                EXPECT_EQ(ResponseCode::SHADOW_MQTT_CLIENT_NOT_SET_ERROR, rc);
            }

            TEST_F(ShadowTester, TestAddShadowSubscriptionWithDisconnectedClient) {
                EXPECT_NE(nullptr, p_iot_greengrass_client_);

                std::unique_ptr<Shadow> test_shadow = Shadow::Create(p_iot_greengrass_client_, mqtt_command_timeout_,
                                                                     p_thing_name_, p_thing_name_);
                EXPECT_NE(nullptr, test_shadow);

                util::Map<ShadowRequestType, Shadow::RequestHandlerPtr> request_mapping;
                request_mapping.insert(std::make_pair(ShadowRequestType::Get, nullptr));
                ResponseCode rc = test_shadow->AddShadowSubscription(request_mapping);
                EXPECT_EQ(ResponseCode::SHADOW_MQTT_DISCONNECTED_ERROR, rc);
            }
        }
    }
}
