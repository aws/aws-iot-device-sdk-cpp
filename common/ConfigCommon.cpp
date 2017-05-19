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
 * @file ConfigCommon.cpp
 * @brief
 *
 */

#ifdef WIN32
#define MAX_PATH_LENGTH_ 260
#include <direct.h>
#define getcwd _getcwd // avoid MSFT "deprecation" warning
#else
#include <limits.h>
#include <unistd.h>
#define MAX_PATH_LENGTH_ PATH_MAX
#endif

#include "util/logging/LogMacros.hpp"
#include "ConfigCommon.hpp"

#define LOG_TAG_SAMPLE_CONFIG_COMMON "[Sample Config]"

// Macro definitions for Json parser
// Network settings
#define SDK_CONFIG_ENDPOINT_KEY "endpoint"
#define SDK_CONFIG_ENDPOINT_PORT_KEY "port"

#define SDK_CONFIG_PROXY_KEY "proxy"
#define SDK_CONFIG_PROXY_PORT_KEY "proxy_port"
#define SDK_CONFIG_PROXY_USER_NAME_KEY "proxy_user_name"
#define SDK_CONFIG_PROXY_PASSWORD_KEY "proxy_password"

// TLS Settings
#define SDK_CONFIG_ROOT_CA_RELATIVE_KEY "root_ca_relative_path"
#define SDK_CONFIG_DEVICE_CERT_RELATIVE_KEY "device_certificate_relative_path"
#define SDK_CONFIG_DEVICE_PRIVATE_KEY_RELATIVE_KEY "device_private_key_relative_path"
#define SDK_CONFIG_TLS_HANDSHAKE_TIMEOUT_MSECS_KEY "tls_handshake_timeout_msecs"
#define SDK_CONFIG_TLS_READ_TIMEOUT_MSECS_KEY "tls_read_timeout_msecs"
#define SDK_CONFIG_TLS_WRITE_TIMEOUT_MSECS_KEY "tls_write_timeout_msecs"

// Websocket settings
#define SDK_CONFIG_AWS_REGION_KEY "aws_region"
#define SDK_CONFIG_AWS_ACCESS_KEY_ID_KEY "aws_access_key_id"
#define SDK_CONFIG_AWS_SECRET_ACCESS_KEY "aws_secret_access_key"
#define SDK_CONFIG_AWS_SESSION_TOKEN_KEY "aws_session_token"

// MQTT Settings
#define SDK_CONFIG_CLIENT_ID_KEY "client_id"
#define SDK_CONFIG_THING_NAME_KEY "thing_name"
#define SDK_CONFIG_IS_CLEAN_SESSION_KEY "is_clean_session"
#define SDK_CONFIG_MQTT_COMMAND_TIMEOUT_MSECS_KEY "mqtt_command_timeout_msecs"
#define SDK_CONFIG_KEEPALIVE_INTERVAL_SECS_KEY "keepalive_interval_secs"
#define SDK_CONFIG_MIN_RECONNECT_INTERVAL_SECS_KEY "minimum_reconnect_interval_secs"
#define SDK_CONFIG_MAX_RECONNECT_INTERVAL_SECS_KEY "maximum_reconnect_interval_secs"
#define SDK_CONFIG_MAX_ACKS_TO_WAIT_FOR_KEY "maximum_acks_to_wait_for"

// Core settings
#define SDK_CONFIG_MAX_TX_ACTION_QUEUE_LENGTH_KEY "maximum_outgoing_action_queue_length"
#define SDK_CONFIG_ACTION_PROCESSING_RATE_KEY "action_processing_rate_hz"

namespace awsiotsdk {
	util::JsonDocument ConfigCommon::sdk_config_json_;

	uint16_t ConfigCommon::endpoint_port_;

	util::String ConfigCommon::endpoint_;

	uint16_t ConfigCommon::proxy_port_;
	util::String ConfigCommon::proxy_;
	util::String ConfigCommon::proxy_user_name_;
	util::String ConfigCommon::proxy_password_;

	util::String ConfigCommon::root_ca_path_;
	util::String ConfigCommon::client_cert_path_;
	util::String ConfigCommon::client_key_path_;
	util::String ConfigCommon::base_client_id_;
	util::String ConfigCommon::thing_name_;
	util::String ConfigCommon::aws_region_;
	util::String ConfigCommon::aws_access_key_id_;
	util::String ConfigCommon::aws_secret_access_key_;
	util::String ConfigCommon::aws_session_token_;

	std::chrono::milliseconds ConfigCommon::mqtt_command_timeout_;
	std::chrono::milliseconds ConfigCommon::tls_handshake_timeout_;
	std::chrono::milliseconds ConfigCommon::tls_read_timeout_;
	std::chrono::milliseconds ConfigCommon::tls_write_timeout_;
	std::chrono::seconds ConfigCommon::keep_alive_timeout_secs_;

	bool ConfigCommon::is_clean_session_;
	std::chrono::seconds ConfigCommon::minimum_reconnect_interval_;
	std::chrono::seconds ConfigCommon::maximum_reconnect_interval_;
	size_t ConfigCommon::max_pending_acks_;
	size_t ConfigCommon::maximum_outgoing_action_queue_length_;
	uint32_t ConfigCommon::action_processing_rate_hz_;

	void ConfigCommon::logParseError(const ResponseCode& response_code, const util::JsonDocument& config) {
			AWS_LOG_ERROR(LOG_TAG_SAMPLE_CONFIG_COMMON,
				"\"Error in Parsing, rc : %d parse error code : %d, offset : %d",
				static_cast<int>(response_code),
				static_cast<int>(util::JsonParser::GetParseErrorCode(config)),
				static_cast<unsigned int>(util::JsonParser::GetParseErrorOffset(config)));
	}

	ResponseCode ConfigCommon::InitializeCommon(const util::String &config_file_relative_path) {
		util::String current_working_directory;
		{
			char CurrentWD[MAX_PATH_LENGTH_ + 1];
			getcwd(CurrentWD, sizeof(CurrentWD));
			current_working_directory.append(CurrentWD, strlen(CurrentWD));
		}

		util::String config_file_absolute_path = current_working_directory;
#ifdef WIN32
		config_file_absolute_path.append("\\");
#else
		config_file_absolute_path.append("/");
#endif
		config_file_absolute_path.append(config_file_relative_path);
		ResponseCode rc = util::JsonParser::InitializeFromJsonFile(sdk_config_json_, config_file_absolute_path);
		if(ResponseCode::SUCCESS != rc) {
			AWS_LOG_ERROR(LOG_TAG_SAMPLE_CONFIG_COMMON,
						  "\"Error in Parsing, rc : %d parse error code : %d, offset : %u",
						  static_cast<int>(rc),
						  static_cast<int>(util::JsonParser::GetParseErrorCode(sdk_config_json_)),
						  static_cast<unsigned int>(util::JsonParser::GetParseErrorOffset(sdk_config_json_)));
			return rc;
		}

		util::String temp_str;

		rc = util::JsonParser::GetStringValue(sdk_config_json_, SDK_CONFIG_ENDPOINT_KEY, endpoint_);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetUint16Value(sdk_config_json_, SDK_CONFIG_ENDPOINT_PORT_KEY, endpoint_port_);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetStringValue(sdk_config_json_, SDK_CONFIG_PROXY_KEY, proxy_);
		if (ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetUint16Value(sdk_config_json_, SDK_CONFIG_PROXY_PORT_KEY, proxy_port_);
		if (ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetStringValue(sdk_config_json_, SDK_CONFIG_PROXY_USER_NAME_KEY, proxy_user_name_);
		if (ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetStringValue(sdk_config_json_, SDK_CONFIG_PROXY_PASSWORD_KEY, proxy_password_);
		if (ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetStringValue(sdk_config_json_, SDK_CONFIG_ROOT_CA_RELATIVE_KEY, temp_str);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}
		root_ca_path_ = current_working_directory;
		root_ca_path_.append("/");
		root_ca_path_.append(temp_str);

		rc = util::JsonParser::GetStringValue(sdk_config_json_, SDK_CONFIG_DEVICE_CERT_RELATIVE_KEY, temp_str);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}
		client_cert_path_ = current_working_directory;
		client_cert_path_.append("/");
		client_cert_path_.append(temp_str);

		rc = util::JsonParser::GetStringValue(sdk_config_json_, SDK_CONFIG_DEVICE_PRIVATE_KEY_RELATIVE_KEY, temp_str);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}
		client_key_path_ = current_working_directory;
		client_key_path_.append("/");
		client_key_path_.append(temp_str);

		rc = util::JsonParser::GetStringValue(sdk_config_json_, SDK_CONFIG_CLIENT_ID_KEY, base_client_id_);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetStringValue(sdk_config_json_, SDK_CONFIG_THING_NAME_KEY, thing_name_);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetStringValue(sdk_config_json_, SDK_CONFIG_AWS_REGION_KEY, aws_region_);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetStringValue(sdk_config_json_, SDK_CONFIG_AWS_ACCESS_KEY_ID_KEY, aws_access_key_id_);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetStringValue(sdk_config_json_, SDK_CONFIG_AWS_SECRET_ACCESS_KEY, aws_secret_access_key_);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetStringValue(sdk_config_json_, SDK_CONFIG_AWS_SESSION_TOKEN_KEY, aws_session_token_);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		uint32_t temp;

		rc = util::JsonParser::GetUint32Value(sdk_config_json_, SDK_CONFIG_MQTT_COMMAND_TIMEOUT_MSECS_KEY, temp);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}
		mqtt_command_timeout_ = std::chrono::milliseconds(temp);

		rc = util::JsonParser::GetUint32Value(sdk_config_json_, SDK_CONFIG_TLS_HANDSHAKE_TIMEOUT_MSECS_KEY, temp);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}
		tls_handshake_timeout_ = std::chrono::milliseconds(temp);

		rc = util::JsonParser::GetUint32Value(sdk_config_json_, SDK_CONFIG_TLS_READ_TIMEOUT_MSECS_KEY, temp);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}
		tls_read_timeout_ = std::chrono::milliseconds(temp);

		rc = util::JsonParser::GetUint32Value(sdk_config_json_, SDK_CONFIG_TLS_WRITE_TIMEOUT_MSECS_KEY, temp);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}
		tls_write_timeout_ = std::chrono::milliseconds(temp);

		rc = util::JsonParser::GetUint32Value(sdk_config_json_, SDK_CONFIG_KEEPALIVE_INTERVAL_SECS_KEY, temp);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}
		keep_alive_timeout_secs_ = std::chrono::seconds(temp);

		rc = util::JsonParser::GetUint32Value(sdk_config_json_, SDK_CONFIG_MIN_RECONNECT_INTERVAL_SECS_KEY, temp);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}
		minimum_reconnect_interval_ = std::chrono::seconds(temp);

		rc = util::JsonParser::GetUint32Value(sdk_config_json_, SDK_CONFIG_MAX_RECONNECT_INTERVAL_SECS_KEY, temp);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}
		maximum_reconnect_interval_ = std::chrono::seconds(temp);

		rc = util::JsonParser::GetSizeTValue(sdk_config_json_, SDK_CONFIG_MAX_TX_ACTION_QUEUE_LENGTH_KEY,
											 maximum_outgoing_action_queue_length_);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetSizeTValue(sdk_config_json_, SDK_CONFIG_MAX_ACKS_TO_WAIT_FOR_KEY,
											 max_pending_acks_);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetBoolValue(sdk_config_json_, SDK_CONFIG_IS_CLEAN_SESSION_KEY, is_clean_session_);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		rc = util::JsonParser::GetUint32Value(sdk_config_json_, SDK_CONFIG_ACTION_PROCESSING_RATE_KEY,
											  action_processing_rate_hz_);
		if(ResponseCode::SUCCESS != rc) {
			logParseError(rc, sdk_config_json_);
			return rc;
		}

		return rc;
	}
}