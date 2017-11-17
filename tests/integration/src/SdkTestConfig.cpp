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
 * @file SdkConfig.cpp
 * @brief
 *
 */

#include <rapidjson/filereadstream.h>

#include "util/JsonParser.hpp"
#include "SdkTestConfig.hpp"

namespace awsiotsdk {
    namespace util {
        rapidjson::Document SdkTestConfig::sdkConfigJson;

        ResponseCode SdkTestConfig::InitializeFromJsonFile(const util::String &configFilePath) {
            return JsonParser::InitializeFromJsonFile(sdkConfigJson, configFilePath);
        }

        ResponseCode SdkTestConfig::InitializeFromJsonString(const util::String &configJsonString) {
            return JsonParser::InitializeFromJsonString(sdkConfigJson, configJsonString);
        }

        ResponseCode SdkTestConfig::GetBoolValue(const char *key, bool &value) {
            return JsonParser::GetBoolValue(sdkConfigJson, key, value);
        }

        ResponseCode SdkTestConfig::GetIntValue(const char *key, int &value) {
            return JsonParser::GetIntValue(sdkConfigJson, key, value);
        }

        ResponseCode SdkTestConfig::GetUint16Value(const char *key, uint16_t &value) {
            return JsonParser::GetUint16Value(sdkConfigJson, key, value);
        }

        ResponseCode SdkTestConfig::GetUint32Value(const char *key, uint32_t &value) {
            return JsonParser::GetUint32Value(sdkConfigJson, key, value);
        }

        ResponseCode SdkTestConfig::GetSizeTValue(const char *key, size_t &value) {
            return JsonParser::GetSizeTValue(sdkConfigJson, key, value);
        }

        ResponseCode SdkTestConfig::GetCStringValue(const char *key, char *value, uint16_t max_string_len) {
            return JsonParser::GetCStringValue(sdkConfigJson, key, value, max_string_len);
        }

        ResponseCode SdkTestConfig::GetStringValue(const char *key, util::String &value) {
            return JsonParser::GetStringValue(sdkConfigJson, key, value);
        }

        rapidjson::ParseErrorCode SdkTestConfig::GetParseErrorCode() {
            return JsonParser::GetParseErrorCode(sdkConfigJson);
        }

        size_t SdkTestConfig::GetParseErrorOffset() {
            return JsonParser::GetParseErrorOffset(sdkConfigJson);
        }
    }
}
 
