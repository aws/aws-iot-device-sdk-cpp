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
 *
 */

/**
 * @file Shadow.cpp
 * @brief
 *
 */

#include <chrono>

#include "shadow/Shadow.hpp"
#include "util/logging/LogMacros.hpp"

#define SUBSCRIPTION_SETTING_TIME_SECS 2

#define SHADOW_REQUEST_TYPE_GET_STRING "get"
#define SHADOW_REQUEST_TYPE_UPDATE_STRING "update"
#define SHADOW_REQUEST_TYPE_DELETE_STRING "delete"
#define SHADOW_REQUEST_TYPE_DELTA_STRING "delta"

#define SHADOW_RESPONSE_TYPE_ACCEPTED_STRING "accepted"
#define SHADOW_RESPONSE_TYPE_REJECTED_STRING "rejected"
#define SHADOW_RESPONSE_TYPE_DELTA_STRING "delta"

#define SHADOW_TOPIC_PREFIX "$aws/things/"
#define SHADOW_TOPIC_MIDDLE "/shadow/"

#define SHADOW_DOCUMENT_EMPTY_STRING "{" \
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

#define SHADOW_DOCUMENT_STATE_KEY "state"
#define SHADOW_DOCUMENT_REPORTED_KEY "reported"
#define SHADOW_DOCUMENT_DESIRED_KEY "desired"
#define SHADOW_DOCUMENT_CLIENT_TOKEN_KEY "clientToken"
#define SHADOW_DOCUMENT_VERSION_KEY "version"
#define SHADOW_DOCUMENT_TIMESTAMP_KEY "timestamp"

#define SHADOW_LOG_TAG "[Shadow]"

namespace awsiotsdk {
    Shadow::Shadow(std::shared_ptr<MqttClient> p_mqtt_client, std::chrono::milliseconds mqtt_command_timeout,
                   util::String &thing_name, util::String &client_token_prefix) {
        p_mqtt_client_ = p_mqtt_client;
        mqtt_command_timeout_ = mqtt_command_timeout;
        thing_name_ = thing_name;
        if (0 == client_token_prefix.length()) {
            client_token_prefix_ = thing_name;
        } else {
            client_token_prefix_ = client_token_prefix;
        }

        is_get_subscription_active_ = false;
        is_update_subscription_active_ = false;
        is_delete_subscription_active_ = false;
        is_delta_subscription_active_ = false;
        cur_shadow_version_ = 0; // Initial value, will be updated by get request
        shadow_topic_action_prefix_ = SHADOW_TOPIC_PREFIX;
        shadow_topic_action_prefix_.append(thing_name_);
        shadow_topic_action_prefix_.append(SHADOW_TOPIC_MIDDLE);
        shadow_topic_delete_ = shadow_topic_action_prefix_;
        shadow_topic_delete_.append(SHADOW_REQUEST_TYPE_DELETE_STRING);
        shadow_topic_get_ = shadow_topic_action_prefix_;
        shadow_topic_get_.append(SHADOW_REQUEST_TYPE_GET_STRING);
        shadow_topic_update_ = shadow_topic_action_prefix_;
        shadow_topic_update_.append(SHADOW_REQUEST_TYPE_UPDATE_STRING);
        shadow_topic_delta_ = shadow_topic_update_;
        shadow_topic_delta_.append("/");
        shadow_topic_delta_.append(SHADOW_REQUEST_TYPE_DELTA_STRING);
        response_type_delta_text_ = SHADOW_RESPONSE_TYPE_DELTA_STRING;
        response_type_rejected_text_ = SHADOW_RESPONSE_TYPE_REJECTED_STRING;
        response_type_accepted_text_ = SHADOW_RESPONSE_TYPE_ACCEPTED_STRING;

        util::JsonParser::InitializeFromJsonString(cur_device_state_document_, SHADOW_DOCUMENT_EMPTY_STRING);
        cur_server_state_document_.SetObject();
        client_token_ = client_token_prefix_ + "_"
            + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        cur_device_state_document_[SHADOW_DOCUMENT_CLIENT_TOKEN_KEY].SetString(client_token_.c_str(),
                                                                               cur_server_state_document_.GetAllocator());
        cur_device_state_document_[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY].SetObject();
        cur_device_state_document_[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_REPORTED_KEY].SetObject();
    }

    std::unique_ptr<Shadow> Shadow::Create(std::shared_ptr<MqttClient> p_mqtt_client,
                                           std::chrono::milliseconds mqtt_command_timeout,
                                           util::String &thing_name, util::String &client_token_prefix) {
        if (nullptr == p_mqtt_client || 0 == thing_name.length()) {
            return nullptr;
        }

        return std::unique_ptr<Shadow>(new Shadow(p_mqtt_client,
                                                  mqtt_command_timeout,
                                                  thing_name,
                                                  client_token_prefix));
    }

    Shadow::~Shadow() {
        if (nullptr == p_mqtt_client_) {
            return;
        }
        if (p_mqtt_client_->IsConnected()) {
            util::Vector<std::unique_ptr<Utf8String>> topic_list;
            if (is_get_subscription_active_) {
                util::String topic_name_accepted = shadow_topic_get_;
                topic_name_accepted.append("/");
                topic_name_accepted.append(SHADOW_RESPONSE_TYPE_ACCEPTED_STRING);
                util::String topic_name_rejected = shadow_topic_get_;
                topic_name_rejected.append("/");
                topic_name_rejected.append(SHADOW_RESPONSE_TYPE_REJECTED_STRING);
                topic_list.push_back(Utf8String::Create(topic_name_accepted));
                topic_list.push_back(Utf8String::Create(topic_name_rejected));
            }

            if (is_update_subscription_active_) {
                util::String topic_name_accepted = shadow_topic_update_;
                topic_name_accepted.append("/");
                topic_name_accepted.append(SHADOW_RESPONSE_TYPE_ACCEPTED_STRING);
                util::String topic_name_rejected = shadow_topic_update_;
                topic_name_rejected.append("/");
                topic_name_rejected.append(SHADOW_RESPONSE_TYPE_REJECTED_STRING);
                topic_list.push_back(Utf8String::Create(topic_name_accepted));
                topic_list.push_back(Utf8String::Create(topic_name_rejected));
            }

            if (is_delete_subscription_active_) {
                util::String topic_name_accepted = shadow_topic_delete_;
                topic_name_accepted.append("/");
                topic_name_accepted.append(SHADOW_RESPONSE_TYPE_ACCEPTED_STRING);
                util::String topic_name_rejected = shadow_topic_delete_;
                topic_name_rejected.append("/");
                topic_name_rejected.append(SHADOW_RESPONSE_TYPE_REJECTED_STRING);
                topic_list.push_back(Utf8String::Create(topic_name_accepted));
                topic_list.push_back(Utf8String::Create(topic_name_rejected));
            }

            if (is_delta_subscription_active_) {
                topic_list.push_back(Utf8String::Create(shadow_topic_delta_));
            }
            ResponseCode rc = p_mqtt_client_->Unsubscribe(std::move(topic_list), mqtt_command_timeout_);
            IOT_UNUSED(rc);
        }
    }

    util::JsonDocument Shadow::GetEmptyShadowDocument() {
        util::JsonDocument empty_shadow_json_document;
        util::JsonParser::InitializeFromJsonString(empty_shadow_json_document, SHADOW_DOCUMENT_EMPTY_STRING);
        return std::move(empty_shadow_json_document);
    }

    ResponseCode Shadow::HandleGetResponse(ShadowResponseType response_type, util::JsonDocument &payload) {
        if (ShadowResponseType::Delta == response_type) {
            AWS_LOG_WARN(SHADOW_LOG_TAG, "Unexpected response type for shadow : %s", thing_name_.c_str());
            return ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TYPE;
        }

        ResponseCode rc = ResponseCode::SHADOW_REQUEST_ACCEPTED;
        if (ShadowResponseType::Rejected == response_type) {
            AWS_LOG_WARN(SHADOW_LOG_TAG, "Get request rejected for shadow : %s", thing_name_.c_str());
            rc = ResponseCode::SHADOW_REQUEST_REJECTED;
        } else  if (!payload.IsObject()
            || !payload.HasMember(SHADOW_DOCUMENT_STATE_KEY)) {
            // Invalid payload
            rc = ResponseCode::SHADOW_UNEXPECTED_RESPONSE_PAYLOAD;
        } else {
            AWS_LOG_DEBUG(SHADOW_LOG_TAG, "Get request accepted for shadow : %s", thing_name_.c_str());
            cur_server_state_document_.RemoveAllMembers();
            cur_server_state_document_.CopyFrom(payload, cur_server_state_document_.GetAllocator());

            ResponseCode rc_parser = util::JsonParser::GetUint32Value(cur_server_state_document_,
                                                                      SHADOW_DOCUMENT_VERSION_KEY,
                                                                      cur_shadow_version_);
            if (ResponseCode::SUCCESS != rc_parser) {
                rc = rc_parser;
            }
        }

        util::Map<ShadowRequestType, RequestHandlerPtr>::iterator request_itr
            = request_mapping_.find(ShadowRequestType::Get);
        if (request_itr != request_mapping_.end() && nullptr != request_itr->second) {
            RequestHandlerPtr ptr = request_itr->second;
            ResponseCode rc_handler = ptr(thing_name_, ShadowRequestType::Get, response_type, payload);
            IOT_UNUSED(rc_handler);
        }

        return rc;
    }

    ResponseCode Shadow::HandleUpdateResponse(ShadowResponseType response_type, util::JsonDocument &payload) {
        ResponseCode rc = ResponseCode::SHADOW_REQUEST_ACCEPTED;
        if (ShadowResponseType::Rejected == response_type) {
            AWS_LOG_WARN(SHADOW_LOG_TAG, "Update request rejected for shadow : %s", thing_name_.c_str());
            rc = ResponseCode::SHADOW_REQUEST_REJECTED;
        } else if (!payload.IsObject() || !payload.HasMember(SHADOW_DOCUMENT_STATE_KEY)) {
            // Validate payload
            rc = ResponseCode::SHADOW_UNEXPECTED_RESPONSE_PAYLOAD;
        } else {
            uint32_t payload_version;
            ResponseCode rc_parser = util::JsonParser::GetUint32Value(payload, SHADOW_DOCUMENT_VERSION_KEY,
                                                                      payload_version);
            // Either key should exist or it shouldn't but no other errors should occur
            if (ResponseCode::SUCCESS != rc_parser && ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR != rc_parser) {
                rc = rc_parser;
            } else if (payload_version <= cur_shadow_version_) {
                rc = ResponseCode::SHADOW_RECEIVED_OLD_VERSION_UPDATE;
            } else {
                bool own_request = false;
                if (payload.HasMember(SHADOW_DOCUMENT_CLIENT_TOKEN_KEY)) {
                    util::String received_client_token;
                    util::JsonParser::GetStringValue((const util::JsonDocument &)payload, SHADOW_DOCUMENT_CLIENT_TOKEN_KEY, received_client_token);
                    own_request = (0 == client_token_.compare(received_client_token));
                }
                if (ShadowResponseType::Delta == response_type) {
                    if (!own_request) {
                        AWS_LOG_DEBUG(SHADOW_LOG_TAG, "Delta received for shadow : %s", thing_name_.c_str());
                        rc = ResponseCode::SHADOW_RECEIVED_DELTA;
                        if (!cur_server_state_document_.HasMember(SHADOW_DOCUMENT_STATE_KEY)) {
                            util::JsonParser::InitializeFromJsonString(cur_server_state_document_,
                                                                       SHADOW_DOCUMENT_EMPTY_STRING);
                        }

                        if (!cur_server_state_document_[SHADOW_DOCUMENT_STATE_KEY].HasMember(SHADOW_DOCUMENT_DESIRED_KEY)) {
                            util::JsonDocument empty_doc;
                            util::JsonParser::InitializeFromJsonString(empty_doc, SHADOW_DOCUMENT_EMPTY_STRING);
                            util::JsonParser::MergeValues(cur_server_state_document_[SHADOW_DOCUMENT_STATE_KEY],
                                                          empty_doc[SHADOW_DOCUMENT_STATE_KEY],
                                                          cur_server_state_document_.GetAllocator());
                        }
                        util::JsonParser::MergeValues(cur_server_state_document_[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY],
                                                      payload[SHADOW_DOCUMENT_STATE_KEY],
                                                      cur_server_state_document_.GetAllocator());
                        cur_shadow_version_ = payload_version;
                    } else {
                        AWS_LOG_DEBUG(SHADOW_LOG_TAG,
                                      "Delta received for own update request for shadow %s, ignoring in favor of processing in accepted",
                                      thing_name_.c_str());
                        rc = ResponseCode::SHADOW_RECEIVED_DELTA;
                    }
                } else {
                    AWS_LOG_DEBUG(SHADOW_LOG_TAG, "Update Accepted for shadow %s!!", thing_name_.c_str());
                    util::JsonParser::MergeValues(cur_server_state_document_, payload,
                                                  cur_server_state_document_.GetAllocator());
                    cur_shadow_version_ = payload_version;
                }
            }
        }

        ShadowRequestType shadow_req_type = ShadowRequestType::Update;
        if (ShadowResponseType::Delta == response_type) {
            shadow_req_type = ShadowRequestType::Delta;
        }

        util::Map<ShadowRequestType, RequestHandlerPtr>::iterator request_itr
            = request_mapping_.find(shadow_req_type);
        if (request_itr != request_mapping_.end() && nullptr != request_itr->second) {
            RequestHandlerPtr ptr = request_itr->second;
            ResponseCode rc_handler = ptr(thing_name_, shadow_req_type, response_type, payload);
            IOT_UNUSED(rc_handler);
        }

        return rc;
    }

    ResponseCode Shadow::HandleDeleteResponse(ShadowResponseType response_type, util::JsonDocument &payload) {
        if (ShadowResponseType::Delta == response_type) {
            return ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TYPE;
        }

        ResponseCode rc = ResponseCode::SHADOW_REQUEST_ACCEPTED;
        if (ShadowResponseType::Rejected == response_type) {
            rc = ResponseCode::SHADOW_REQUEST_REJECTED;
        } else {
            cur_server_state_document_.SetObject();
            cur_shadow_version_ = 0;
        }

        util::Map<ShadowRequestType, RequestHandlerPtr>::iterator request_itr
            = request_mapping_.find(ShadowRequestType::Delete);
        if (request_itr != request_mapping_.end() && nullptr != request_itr->second) {
            RequestHandlerPtr ptr = request_itr->second;
            ResponseCode rc_handler = ptr(thing_name_, ShadowRequestType::Delete, response_type, payload);
            IOT_UNUSED(rc_handler);
        }

        return rc;
    }

    ResponseCode Shadow::SubscriptionHandler(util::String topic_name, util::String payload,
                                             std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
        util::JsonDocument json_payload;
        ShadowResponseType response_type;
        ResponseCode rc = util::JsonParser::InitializeFromJsonString(json_payload, payload);

        if (ResponseCode::SUCCESS == rc) {
            if (std::equal(response_type_delta_text_.rbegin(), response_type_delta_text_.rend(), topic_name.rbegin())) {
                response_type = ShadowResponseType::Delta;
            } else if (std::equal(response_type_rejected_text_.rbegin(), response_type_rejected_text_.rend(),
                                  topic_name.rbegin())) {
                response_type = ShadowResponseType::Rejected;
            } else if (std::equal(response_type_accepted_text_.rbegin(), response_type_accepted_text_.rend(),
                                  topic_name.rbegin())) {
                response_type = ShadowResponseType::Accepted;
            } else {
                rc = ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TYPE;
            }

            if (ResponseCode::SUCCESS == rc) {
                rc = ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TOPIC;
                if (std::equal(shadow_topic_get_.begin(), shadow_topic_get_.end(), topic_name.begin())) {
                    rc = HandleGetResponse(response_type, json_payload);
                } else if (std::equal(shadow_topic_update_.begin(), shadow_topic_update_.end(), topic_name.begin())) {
                    rc = HandleUpdateResponse(response_type, json_payload);
                } else if (std::equal(shadow_topic_delete_.begin(), shadow_topic_delete_.end(), topic_name.begin())) {
                    rc = HandleDeleteResponse(response_type, json_payload);
                }
            }
        } else {
            AWS_LOG_ERROR(SHADOW_LOG_TAG, "\"Error in Parsing. %s\n parse error code : %d, offset : %u",
                          ResponseHelper::ToString(rc).c_str(),
                          static_cast<int>(util::JsonParser::GetParseErrorCode(json_payload)),
                          static_cast<unsigned int>(util::JsonParser::GetParseErrorOffset(json_payload)));
        }
        return rc;
    }

    ResponseCode Shadow::AddShadowSubscription(util::Map<ShadowRequestType, RequestHandlerPtr> &request_mapping) {
        if (nullptr == p_mqtt_client_) {
            return ResponseCode::SHADOW_MQTT_CLIENT_NOT_SET_ERROR;
        }

        ResponseCode rc = ResponseCode::SHADOW_REQUEST_MAP_EMPTY;
        if (!request_mapping.empty()) {
            if (p_mqtt_client_->IsConnected()) {
                bool has_get = false;
                bool has_update = false;
                bool has_delete = false;
                bool has_delta = false;
                util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;
                mqtt::Subscription::ApplicationCallbackHandlerPtr p_sub_handler =
                    std::bind(&Shadow::SubscriptionHandler, this, std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3);

                util::Map<ShadowRequestType, RequestHandlerPtr>::const_iterator request_itr = request_mapping.begin();
                while (request_itr != request_mapping.end()) {
                    util::String topic_name_str = "";
                    switch (request_itr->first) {
                        case ShadowRequestType::Get :
                            has_get = true;
                            topic_name_str.append(shadow_topic_get_);
                            break;
                        case ShadowRequestType::Update :
                            has_update = true;
                            topic_name_str.append(shadow_topic_update_);
                            break;
                        case ShadowRequestType::Delete :
                            has_delete = true;
                            topic_name_str.append(shadow_topic_delete_);
                            break;
                        case ShadowRequestType::Delta :
                            has_delta = true;
                            topic_name_str.append(shadow_topic_delta_);
                            break;
                    }

                    if (has_delta) {
                        std::shared_ptr<mqtt::Subscription> p_subscription_delta =
                            mqtt::Subscription::Create(Utf8String::Create(shadow_topic_delta_), mqtt::QoS::QOS0,
                                                       p_sub_handler, nullptr);

                        topic_vector.push_back(p_subscription_delta);
                    } else {
                        topic_name_str.append("/");

                        util::String topic_name_accepted = topic_name_str;
                        topic_name_accepted.append(SHADOW_RESPONSE_TYPE_ACCEPTED_STRING);
                        std::shared_ptr<mqtt::Subscription> p_subscription_accepted =
                            mqtt::Subscription::Create(Utf8String::Create(topic_name_accepted), mqtt::QoS::QOS0,
                                                       p_sub_handler, nullptr);

                        util::String topic_name_rejected = topic_name_str;
                        topic_name_rejected.append(SHADOW_RESPONSE_TYPE_REJECTED_STRING);
                        std::shared_ptr<mqtt::Subscription> p_subscription_rejected =
                            mqtt::Subscription::Create(Utf8String::Create(topic_name_rejected), mqtt::QoS::QOS0,
                                                       p_sub_handler, nullptr);

                        topic_vector.push_back(p_subscription_accepted);
                        topic_vector.push_back(p_subscription_rejected);
                    }
                    request_itr++;
                }

                rc = p_mqtt_client_->Subscribe(topic_vector, mqtt_command_timeout_);
                if (ResponseCode::SUCCESS == rc) {
                    if (has_get) {
                        is_get_subscription_active_ = true;
                    }
                    if (has_update) {
                        is_update_subscription_active_ = true;
                    }
                    if (has_delete) {
                        is_delete_subscription_active_ = true;
                    }
                    if (has_delta) {
                        is_delta_subscription_active_ = true;
                    }

                    request_itr = request_mapping.begin();
                    while (request_itr != request_mapping.end()) {
                        util::Map<ShadowRequestType, RequestHandlerPtr>::const_iterator request_itr_temp
                            = request_mapping_.find(request_itr->first);
                        if (request_mapping_.end() != request_itr_temp) {
                            request_mapping_.erase(request_itr_temp);
                        }
                        request_mapping_.insert(std::make_pair(request_itr->first, request_itr->second));
                        request_itr++;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(SUBSCRIPTION_SETTING_TIME_SECS));
                }
            } else {
                rc = ResponseCode::SHADOW_MQTT_DISCONNECTED_ERROR;
            }
        }

        return rc;
    }

    ResponseCode Shadow::UpdateDeviceShadow(util::JsonDocument &document) {
        if (document.IsNull() && !document.IsObject()) {
            return ResponseCode::SHADOW_JSON_EMPTY_ERROR;
        }

        return util::JsonParser::MergeValues(cur_device_state_document_,
                                             document,
                                             cur_device_state_document_.GetAllocator());
    }

    util::JsonDocument Shadow::GetDeviceReported() {
        util::JsonDocument reported;
        reported.CopyFrom(cur_device_state_document_[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_REPORTED_KEY],
                          reported.GetAllocator());
        return std::move(reported);
    }

    util::JsonDocument Shadow::GetDeviceDesired() {
        util::JsonDocument desired;
        desired.CopyFrom(cur_device_state_document_[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY],
                         desired.GetAllocator());
        return std::move(desired);
    }

    util::JsonDocument Shadow::GetDeviceDocument() {
        util::JsonDocument shadow_doc;
        shadow_doc.CopyFrom(cur_device_state_document_, shadow_doc.GetAllocator());
        return std::move(shadow_doc);
    }

    util::JsonDocument Shadow::GetServerReported() {
        if (!cur_server_state_document_.HasMember(SHADOW_DOCUMENT_STATE_KEY)) {
            return nullptr;
        }
        if (!cur_server_state_document_[SHADOW_DOCUMENT_STATE_KEY].HasMember(SHADOW_DOCUMENT_REPORTED_KEY)) {
            return nullptr;
        }
        util::JsonDocument reported;
        reported.CopyFrom(cur_server_state_document_[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_REPORTED_KEY],
                          reported.GetAllocator());
        return std::move(reported);
    }

    util::JsonDocument Shadow::GetServerDesired() {
        if (!cur_server_state_document_.HasMember(SHADOW_DOCUMENT_STATE_KEY)) {
            return nullptr;
        }
        if (!cur_server_state_document_[SHADOW_DOCUMENT_STATE_KEY].HasMember(SHADOW_DOCUMENT_DESIRED_KEY)) {
            return nullptr;
        }
        util::JsonDocument desired;
        desired.CopyFrom(cur_server_state_document_[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY],
                         desired.GetAllocator());
        return std::move(desired);
    }

    util::JsonDocument Shadow::GetServerDocument() {
        util::JsonDocument shadow_doc;
        shadow_doc.CopyFrom(cur_server_state_document_, shadow_doc.GetAllocator());
        return std::move(shadow_doc);
    }

    ResponseCode Shadow::PerformGetAsync() {
        if (nullptr == p_mqtt_client_) {
            return ResponseCode::SHADOW_MQTT_CLIENT_NOT_SET_ERROR;
        }

        if (!p_mqtt_client_->IsConnected()) {
            return ResponseCode::SHADOW_MQTT_DISCONNECTED_ERROR;
        }

        ResponseCode rc = ResponseCode::SUCCESS;
        if (!is_get_subscription_active_) {
            util::Map<ShadowRequestType, Shadow::RequestHandlerPtr> request_mapping;
            request_mapping.insert(std::make_pair(ShadowRequestType::Get, nullptr));
            rc = AddShadowSubscription(request_mapping);
            if (ResponseCode::SUCCESS != rc) {
                return rc;
            }
        }

        // Get request requires empty payload
        util::String payload = "";
        rc = p_mqtt_client_->Publish(Utf8String::Create(shadow_topic_get_), false, false, mqtt::QoS::QOS0, payload,
                                     mqtt_command_timeout_);

        return rc;
    }

    ResponseCode Shadow::PerformUpdateAsync() {
        if (nullptr == p_mqtt_client_) {
            return ResponseCode::SHADOW_MQTT_CLIENT_NOT_SET_ERROR;
        }

        if (!p_mqtt_client_->IsConnected()) {
            return ResponseCode::SHADOW_MQTT_DISCONNECTED_ERROR;
        }
        if (!cur_server_state_document_.IsObject()
            || !cur_device_state_document_.IsObject()) {
            // Should never happen
            AWS_LOG_ERROR(SHADOW_LOG_TAG, "Server/Device state no longer an object!! Should never Happen!!");
            return ResponseCode::FAILURE;
        }

        if (cur_server_state_document_.HasMember(SHADOW_DOCUMENT_STATE_KEY)
            && cur_device_state_document_.HasMember(SHADOW_DOCUMENT_STATE_KEY)
            && (cur_device_state_document_[SHADOW_DOCUMENT_STATE_KEY]
                == cur_server_state_document_[SHADOW_DOCUMENT_STATE_KEY])) {
            return ResponseCode::SHADOW_NOTHING_TO_UPDATE;
        }

        ResponseCode rc = ResponseCode::SUCCESS;
        if (!is_update_subscription_active_) {
            util::Map<ShadowRequestType, Shadow::RequestHandlerPtr> request_mapping;
            request_mapping.insert(std::make_pair(ShadowRequestType::Update, nullptr));
            rc = AddShadowSubscription(request_mapping);
            if (ResponseCode::SUCCESS != rc) {
                return rc;
            }
        }

        // TODO: Optimization needed here
        util::JsonDocument diff;
        util::JsonParser::DiffValues(diff, cur_server_state_document_, cur_device_state_document_, diff.GetAllocator());
        if (diff.HasMember(SHADOW_DOCUMENT_TIMESTAMP_KEY)) {
            diff.EraseMember(SHADOW_DOCUMENT_TIMESTAMP_KEY);
        }

        if (diff.HasMember(SHADOW_DOCUMENT_VERSION_KEY)) {
            diff.EraseMember(SHADOW_DOCUMENT_VERSION_KEY);
        }

        // make sure that payload is valid
        if (!diff.HasMember(SHADOW_DOCUMENT_STATE_KEY)) {
            return ResponseCode::SHADOW_NOTHING_TO_UPDATE;
        }

        util::String payload = util::JsonParser::ToString(diff);

        rc = p_mqtt_client_->Publish(Utf8String::Create(shadow_topic_update_), false, false, mqtt::QoS::QOS0,
                                     payload, mqtt_command_timeout_);

        return rc;
    }

    ResponseCode Shadow::PerformDeleteAsync() {
        if (nullptr == p_mqtt_client_) {
            return ResponseCode::SHADOW_MQTT_CLIENT_NOT_SET_ERROR;
        }

        if (!p_mqtt_client_->IsConnected()) {
            return ResponseCode::SHADOW_MQTT_DISCONNECTED_ERROR;
        }

        ResponseCode rc = ResponseCode::SUCCESS;
        if (!is_delete_subscription_active_) {
            util::Map<ShadowRequestType, Shadow::RequestHandlerPtr> request_mapping;
            request_mapping.insert(std::make_pair(ShadowRequestType::Delete, nullptr));
            rc = AddShadowSubscription(request_mapping);
            if (ResponseCode::SUCCESS != rc) {
                return rc;
            }
        }

        // Delete request requires empty payload
        util::String payload = "";
        rc = p_mqtt_client_->Publish(Utf8String::Create(shadow_topic_delete_), false, false, mqtt::QoS::QOS0, payload,
                                     mqtt_command_timeout_);
        return rc;
    }

    void Shadow::ResetClientTokenSuffix() {
        client_token_ = client_token_prefix_ + "_"
            + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        cur_device_state_document_[SHADOW_DOCUMENT_CLIENT_TOKEN_KEY].SetString(client_token_.c_str(),
                                                                               cur_server_state_document_.GetAllocator());
    }

    uint32_t Shadow::GetCurrentVersionNumber() {
        return cur_shadow_version_;
    }

    bool Shadow::IsInSync() {
        if (!cur_server_state_document_.IsObject()) {
            return false;
        }

        if (!cur_server_state_document_.HasMember(SHADOW_DOCUMENT_STATE_KEY)) {
            return false;
        }

        bool has_reported = cur_server_state_document_[SHADOW_DOCUMENT_STATE_KEY].HasMember(SHADOW_DOCUMENT_REPORTED_KEY);
        bool has_desired = cur_server_state_document_[SHADOW_DOCUMENT_STATE_KEY].HasMember(SHADOW_DOCUMENT_DESIRED_KEY);

        if (!has_reported && !has_desired) {
            // Empty document
            return false;
        }

        if (has_reported && !has_desired) {
            return false;
        }

        if (!has_reported && has_desired) {
            return false;
        }

        return (cur_server_state_document_[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_DESIRED_KEY]
            == cur_server_state_document_[SHADOW_DOCUMENT_STATE_KEY][SHADOW_DOCUMENT_REPORTED_KEY]);
    }
}
