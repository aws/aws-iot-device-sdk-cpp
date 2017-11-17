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
 * @file JsonParser.cpp
 * @brief
 *
 */

#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <fstream>
#include <iostream>
#include <memory>

#include "util/JsonParser.hpp"

namespace awsiotsdk {
    namespace util {
        ResponseCode JsonParser::InitializeFromJsonFile(JsonDocument &json_document,
                                                        const util::String &input_file_path) {
            if (0 == input_file_path.length()) {
                return ResponseCode::FILE_NAME_INVALID;
            }

            std::unique_ptr<FILE, int (*)(FILE *)>
                input_file = std::unique_ptr<FILE, int (*)(FILE *)>(fopen(input_file_path.c_str(), "rb"), fclose);
            if (nullptr == input_file) {
                return ResponseCode::FILE_OPEN_ERROR;
            }

            std::streampos fsize = 0;
            std::ifstream config_file_stream(input_file_path, std::ios::binary);
            fsize = config_file_stream.tellg();
            config_file_stream.seekg(0, std::ios::end);
            fsize = config_file_stream.tellg() - fsize;
            config_file_stream.close();

            std::unique_ptr<char[]> read_buffer = std::unique_ptr<char[]>(new char[fsize]);
            rapidjson::FileReadStream fileReadStream(input_file.get(), read_buffer.get(), static_cast<size_t>(fsize));

            json_document.ParseStream(fileReadStream);

            if (json_document.HasParseError()) {
                return ResponseCode::JSON_PARSING_ERROR;
            }

            return ResponseCode::SUCCESS;
        }

        ResponseCode JsonParser::InitializeFromJsonString(JsonDocument &json_document,
                                                          const util::String &config_json_string) {
            if (0 == config_json_string.length()) {
                return ResponseCode::NULL_VALUE_ERROR;
            }

            json_document.Parse(config_json_string.c_str(), config_json_string.length());

            if (json_document.HasParseError()) {
                return ResponseCode::JSON_PARSING_ERROR;
            }

            return ResponseCode::SUCCESS;
        }

        ResponseCode JsonParser::GetBoolValue(const JsonDocument &json_document, const char *key, bool &value) {
            if (!json_document.HasMember(key)) {
                return ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR;
            }

            if (!json_document[key].IsBool()) {
                return ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR;
            }

            value = json_document[key].GetBool();

            return ResponseCode::SUCCESS;
        }

        ResponseCode JsonParser::GetIntValue(const JsonDocument &json_document, const char *key, int &value) {
            if (!json_document.HasMember(key)) {
                return ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR;
            }

            if (!json_document[key].IsInt()) {
                return ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR;
            }

            value = json_document[key].GetInt();

            return ResponseCode::SUCCESS;
        }

        ResponseCode JsonParser::GetUint16Value(const JsonDocument &json_document, const char *key, uint16_t &value) {
            if (!json_document.HasMember(key)) {
                return ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR;
            }

            if (!json_document[key].IsUint()) {
                return ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR;
            }

            value = static_cast<uint16_t>(json_document[key].GetUint());

            return ResponseCode::SUCCESS;
        }

        ResponseCode JsonParser::GetUint32Value(const JsonDocument &json_document, const char *key, uint32_t &value) {
            if (!json_document.HasMember(key)) {
                return ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR;
            }

            if (!json_document[key].IsUint()) {
                return ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR;
            }

            value = static_cast<uint32_t>(json_document[key].GetUint());

            return ResponseCode::SUCCESS;
        }

        ResponseCode JsonParser::GetSizeTValue(const JsonDocument &json_document, const char *key, size_t &value) {
            if (!json_document.HasMember(key)) {
                return ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR;
            }

            if (!json_document[key].IsUint()) {
                return ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR;
            }

            value = static_cast<size_t>(json_document[key].GetUint());

            return ResponseCode::SUCCESS;
        }

        ResponseCode JsonParser::GetCStringValue(const JsonDocument &json_document,
                                                 const char *key,
                                                 char *value,
                                                 uint16_t max_string_len) {
            if (nullptr == value) {
                return ResponseCode::NULL_VALUE_ERROR;
            }

            if (!json_document.HasMember(key)) {
                return ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR;
            }

            if (!json_document[key].IsString()) {
                return ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR;
            }

            snprintf(value, max_string_len, "%s", json_document[key].GetString());

            return ResponseCode::SUCCESS;
        }

        ResponseCode JsonParser::GetStringValue(const JsonDocument &json_document,
                                                const char *key,
                                                util::String &value) {
            if (!json_document.HasMember(key)) {
                return ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR;
            }

            if (!json_document[key].IsString()) {
                return ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR;
            }

            value = json_document[key].GetString();

            return ResponseCode::SUCCESS;
        }

        rapidjson::ParseErrorCode JsonParser::GetParseErrorCode(const JsonDocument &json_document) {
            return json_document.GetParseError();
        }

        size_t JsonParser::GetParseErrorOffset(const JsonDocument &json_document) {
            return json_document.GetErrorOffset();
        }

        // TODO: This implementation takes O(n^2), only possible way to shorten the time would
        // be to sort the json and bring it down to O(nlog(n)). But changing the json structure
        // might not be desired. Provide a second implementation that does this with an option
        ResponseCode JsonParser::MergeValues(JsonValue &target,
                                             JsonValue &source,
                                             JsonValue::AllocatorType &allocator) {
            if (source.IsNull() || target.IsNull() || !source.IsObject() || !target.IsObject()) {
                return ResponseCode::JSON_MERGE_FAILED;
            }

            ResponseCode rc = ResponseCode::SUCCESS;
            bool inner_merge_successful = true;
            bool target_has_key = false;

            util::JsonValue::MemberIterator target_itr;
            util::JsonValue::MemberIterator source_itr = source.MemberBegin();
            while (source_itr != source.MemberEnd()) {
                target_itr = target.MemberBegin();
                target_has_key = false;
                inner_merge_successful = true;

                while (target_itr != target.MemberEnd()) {
                    if (target_itr->name == source_itr->name) {
                        if (target_itr->value.IsObject() && source_itr->value.IsObject()) {
                            rc = MergeValues(target_itr->value, source_itr->value, allocator);
                            if (ResponseCode::SUCCESS != rc) {
                                inner_merge_successful = false;
                                break;
                            }
                        } else {
                            target_itr = target.EraseMember(target_itr);
                            JsonValue name;
                            JsonValue value;
                            name.CopyFrom(source_itr->name, allocator);
                            value.CopyFrom(source_itr->value, allocator);
                            target.AddMember(name.Move(), value.Move(), allocator);
                        }
                        target_has_key = true;
                        break;
                    }
                    target_itr++;
                }

                if (!inner_merge_successful) {
                    break;
                }

                if (!target_has_key) {
                    JsonValue name;
                    JsonValue value;
                    name.CopyFrom(source_itr->name, allocator);
                    value.CopyFrom(source_itr->value, allocator);
                    target.AddMember(name.Move(), value.Move(), allocator);
                }
                source_itr++;
            }

            return rc;
        }

        // TODO: This implementation takes O(n^2), only possible way to shorten the time would
        // be to sort the json and bring it down to O(nlog(n)). But changing the json structure
        // might not be desired. Provide a second implementation that does this with an option
        ResponseCode JsonParser::DiffValues(JsonValue &target_doc,
                                            JsonValue &old_doc,
                                            JsonValue &new_doc,
                                            JsonValue::AllocatorType &allocator) {
            if (old_doc.IsNull() || new_doc.IsNull() || !old_doc.IsObject() || !new_doc.IsObject()) {
                return ResponseCode::JSON_MERGE_FAILED;
            }

            ResponseCode rc = ResponseCode::SUCCESS;
            bool inner_merge_successful = true;
            bool old_doc_has_key = false;

            target_doc.SetObject();

            util::JsonValue::MemberIterator old_doc_itr;
            util::JsonValue::MemberIterator new_doc_itr = new_doc.MemberBegin();
            while (new_doc_itr != new_doc.MemberEnd()) {
                old_doc_itr = old_doc.MemberBegin();
                old_doc_has_key = false;
                inner_merge_successful = true;

                while (old_doc_itr != old_doc.MemberEnd()) {
                    if (old_doc_itr->name == new_doc_itr->name) {
                        if (old_doc_itr->value != new_doc_itr->value) {
                            if (old_doc_itr->value.IsObject() && new_doc_itr->value.IsObject()) {
                                JsonValue diff_val;
                                diff_val.SetObject();
                                rc = DiffValues(diff_val, old_doc_itr->value, new_doc_itr->value, allocator);
                                if (ResponseCode::SUCCESS != rc) {
                                    inner_merge_successful = false;
                                    break;
                                } else {
                                    if (diff_val.MemberCount() > 0) {
                                        JsonValue name;
                                        name.CopyFrom(new_doc_itr->name, allocator);
                                        target_doc.AddMember(name.Move(), diff_val.Move(), allocator);
                                    }
                                }
                            } else {
                                JsonValue name;
                                JsonValue value;
                                name.CopyFrom(new_doc_itr->name, allocator);
                                value.CopyFrom(new_doc_itr->value, allocator);
                                target_doc.AddMember(name.Move(), value.Move(), allocator);
                            }
                        }
                        old_doc_has_key = true;
                        break;
                    }
                    old_doc_itr++;
                }

                if (!inner_merge_successful) {
                    break;
                }

                if (!old_doc_has_key) {
                    JsonValue name;
                    JsonValue value;
                    name.CopyFrom(new_doc_itr->name, allocator);
                    value.CopyFrom(new_doc_itr->value, allocator);
                    target_doc.AddMember(name.Move(), value.Move(), allocator);
                }
                new_doc_itr++;
            }

            return rc;
        }

        util::String JsonParser::ToString(JsonDocument &json_document) {
            rapidjson::StringBuffer buffer;
            buffer.Clear();
            rapidjson::Writer <rapidjson::StringBuffer> writer(buffer);
            json_document.Accept(writer);

            return util::String(buffer.GetString());
        }

        util::String JsonParser::ToString(JsonValue &json_value) {
            rapidjson::StringBuffer buffer;
            buffer.Clear();
            rapidjson::Writer <rapidjson::StringBuffer> writer(buffer);
            json_value.Accept(writer);

            return util::String(buffer.GetString());
        }

        ResponseCode JsonParser::WriteToFile(JsonDocument &json_document, const util::String &output_file_path) {
            if (0 == output_file_path.length()) {
                return ResponseCode::FILE_NAME_INVALID;
            }

            std::unique_ptr<FILE, int (*)(FILE *)>
                output_file = std::unique_ptr<FILE, int (*)(FILE *)>(fopen(output_file_path.c_str(), "wb"), fclose);
            if (nullptr == output_file) {
                return ResponseCode::FILE_OPEN_ERROR;
            }

            util::String json_document_string = ToString(json_document);
            size_t json_file_length = json_document_string.length();

            std::unique_ptr<char[]> write_buffer = std::unique_ptr<char[]>(new char[json_file_length]);
            rapidjson::FileWriteStream fileWriteStream(output_file.get(), write_buffer.get(), json_file_length);
            rapidjson::Writer <rapidjson::FileWriteStream> writer(fileWriteStream);
            json_document.Accept(writer);

            return ResponseCode::SUCCESS;
        }
    }
}
 
