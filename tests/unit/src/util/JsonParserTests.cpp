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
 * @file JsonParserTests.cpp
 * @brief
 *
 */

#include <gtest/gtest.h>

#include "TestHelper.hpp"

#include "util/JsonParser.hpp"
#include <fstream>
#include <iostream>

#define KEY_INVALID "test_invalid_key"

#define KEY_STRING "test_string"
#define EXPECTED_VALUE_STRING "json_parser_test_string"

#define KEY_CSTRING "test_cstr"
#define EXPECTED_VALUE_CSTRING "json_parser_test_cstring"

#define KEY_UINT16 "test_uint16_t"
#define EXPECTED_VALUE_UINT16 16

#define KEY_UINT32 "test_uin32_t"
#define EXPECTED_VALUE_UINT32 32

#define KEY_SIZET "test_size_t"
#define EXPECTED_VALUE_SIZET 64

#define KEY_INT "test_int"
#define EXPECTED_VALUE_INT -128

#define KEY_BOOL_TRUE "test_bool_true"
#define EXPECTED_VALUE_BOOL_TRUE true

#define KEY_BOOL_FALSE "test_bool_false"
#define EXPECTED_VALUE_BOOL_FALSE false

#define JSON_TEST_FILE_PATH "./TestParser.json"

#define JSON_MERGE_TEST_SOURCE_DOCUMENT_STRING "{" \
"    \"level1\" : {" \
"        \"level2\" : {" \
"        	\"level3_key\" : \"level3_source_value\"," \
"        	\"level3\" : {" \
"	        	\"level4_key\" : \"level4_source_value\"" \
"        	}" \
"        }," \
"        \"level2_key\" : \"level2_source_value\"" \
"    }," \
"    \"level1_key\": \"level1_source_value\"" \
"}"

#define JSON_MERGE_TEST_TARGET_DOCUMENT_STRING "{" \
"    \"level1\" : {" \
"        \"level2\" : {" \
"        	\"level3_key\" : \"level3_target_value\"," \
"        	\"level3_key_2\" : \"level3_target_value\"" \
"        }," \
"        \"level2_key\" : \"level2_target_value\"" \
"    }," \
"    \"level1_key\": \"level1_target_value\"" \
"}"

#define JSON_MERGE_TEST_MERGED_DOCUMENT_STRING "{" \
"    \"level1\" : {" \
"        \"level2\" : {" \
"        	\"level3_key\" : \"level3_source_value\"," \
"        	\"level3_key_2\" : \"level3_target_value\"," \
"        	\"level3\" : {" \
"	        	\"level4_key\" : \"level4_source_value\"" \
"        	}" \
"        }," \
"        \"level2_key\" : \"level2_source_value\"" \
"    }," \
"    \"level1_key\": \"level1_source_value\"" \
"}"

#define JSON_MERGE_TEST_DIFF_DOCUMENT_STRING "{" \
"    \"level1\" : {" \
"        \"level2\" : {" \
"        	\"level3_key\" : \"level3_target_value\"," \
"        	\"level3_key_2\" : \"level3_target_value\"" \
"        }," \
"        \"level2_key\" : \"level2_target_value\"" \
"    }," \
"    \"level1_key\": \"level1_target_value\"" \
"}"

#define BROKEN_JSON_STRING "{" \
"    \"level1\" : {" \
"        \"level2\" : {" \
"        	\"level3_key\" : \"level3_target_value\"," \
"        	\"level3_key_2\" : \"level3_target_value\"" \
"        }," \
"        \"level2_key\" : \"level2_target_value\"" \
"    },"

#define SINGLE_VALUE_JSON_KEY "Key"
#define JSON_STRING_OUTPUT "\"Key\""


namespace awsiotsdk {
    namespace tests {
        namespace unit {
            class JsonParserTester : public ::testing::Test {
            protected:
                util::JsonDocument json_doc_file_;
                util::JsonDocument json_doc_str_;
                static const util::String test_json_;

                JsonParserTester() {}
            };

            const util::String JsonParserTester::test_json_ = "{"
                "  \"test_string\":\"json_parser_test_string\","
                "  \"test_cstr\":\"json_parser_test_cstring\","
                "  \"test_uint16_t\":16,"
                "  \"test_uin32_t\":32,"
                "  \"test_size_t\":64,"
                "  \"test_int\":-128,"
                "  \"test_bool_true\":true,"
                "  \"test_bool_false\":false"
                "}";

            TEST_F(JsonParserTester, RunTests) {
                /* Test invalid file path */
                ResponseCode rc = util::JsonParser::InitializeFromJsonFile(json_doc_file_, "");
                EXPECT_EQ(ResponseCode::FILE_NAME_INVALID, rc);
                rc = util::JsonParser::InitializeFromJsonFile(json_doc_file_, KEY_INVALID);
                EXPECT_EQ(ResponseCode::FILE_OPEN_ERROR, rc);

                /* Test invalid json */
                rc = util::JsonParser::InitializeFromJsonString(json_doc_file_, "{,,,}");
                EXPECT_EQ(ResponseCode::JSON_PARSING_ERROR, rc);

                rc = util::JsonParser::InitializeFromJsonFile(json_doc_file_, JSON_TEST_FILE_PATH);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                rc = util::JsonParser::InitializeFromJsonString(json_doc_str_, test_json_);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                util::String expected_string = EXPECTED_VALUE_STRING;
                util::String parsed_string;
                rc = util::JsonParser::GetStringValue(json_doc_str_, KEY_STRING, parsed_string);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(expected_string, parsed_string);
                rc = util::JsonParser::GetStringValue(json_doc_file_, KEY_STRING, parsed_string);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(expected_string, parsed_string);
                rc = util::JsonParser::GetStringValue(json_doc_str_, KEY_INVALID, parsed_string);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR, rc);
                rc = util::JsonParser::GetStringValue(json_doc_str_, KEY_INT, parsed_string);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR, rc);

                util::String expected_cstring = EXPECTED_VALUE_CSTRING;
                char buf[100];
                rc = util::JsonParser::GetCStringValue(json_doc_str_, KEY_CSTRING, buf, 100);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(expected_cstring, buf);
                rc = util::JsonParser::GetCStringValue(json_doc_file_, KEY_CSTRING, buf, 100);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(expected_cstring, buf);
                rc = util::JsonParser::GetCStringValue(json_doc_str_, KEY_CSTRING, nullptr, 100);
                EXPECT_EQ(ResponseCode::NULL_VALUE_ERROR, rc);
                rc = util::JsonParser::GetCStringValue(json_doc_str_, KEY_INVALID, buf, 100);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR, rc);
                rc = util::JsonParser::GetCStringValue(json_doc_str_, KEY_INT, buf, 100);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR, rc);

                int parsed_int = 0;
                rc = util::JsonParser::GetIntValue(json_doc_str_, KEY_INT, parsed_int);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(EXPECTED_VALUE_INT, parsed_int);
                parsed_int = 0;
                rc = util::JsonParser::GetIntValue(json_doc_file_, KEY_INT, parsed_int);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(EXPECTED_VALUE_INT, parsed_int);
                rc = util::JsonParser::GetIntValue(json_doc_str_, KEY_INVALID, parsed_int);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR, rc);
                rc = util::JsonParser::GetIntValue(json_doc_str_, KEY_STRING, parsed_int);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR, rc);

                uint16_t parsed_uint16t = 0;
                rc = util::JsonParser::GetUint16Value(json_doc_str_, KEY_UINT16, parsed_uint16t);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(EXPECTED_VALUE_UINT16, parsed_uint16t);
                parsed_uint16t = 0;
                rc = util::JsonParser::GetUint16Value(json_doc_file_, KEY_UINT16, parsed_uint16t);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(EXPECTED_VALUE_UINT16, parsed_uint16t);
                rc = util::JsonParser::GetUint16Value(json_doc_str_, KEY_INVALID, parsed_uint16t);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR, rc);
                rc = util::JsonParser::GetUint16Value(json_doc_str_, KEY_STRING, parsed_uint16t);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR, rc);

                uint32_t parsed_uint32t = 0;
                rc = util::JsonParser::GetUint32Value(json_doc_str_, KEY_UINT32, parsed_uint32t);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(static_cast<uint32_t>(EXPECTED_VALUE_UINT32), parsed_uint32t);
                parsed_uint32t = 0;
                rc = util::JsonParser::GetUint32Value(json_doc_file_, KEY_UINT32, parsed_uint32t);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(static_cast<uint32_t>(EXPECTED_VALUE_UINT32), parsed_uint32t);
                rc = util::JsonParser::GetUint32Value(json_doc_str_, KEY_INVALID, parsed_uint32t);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR, rc);
                rc = util::JsonParser::GetUint32Value(json_doc_str_, KEY_STRING, parsed_uint32t);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR, rc);

                size_t parsed_sizet = 0;
                rc = util::JsonParser::GetSizeTValue(json_doc_str_, KEY_SIZET, parsed_sizet);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(static_cast<size_t>(EXPECTED_VALUE_SIZET), parsed_sizet);
                parsed_uint16t = 0;
                rc = util::JsonParser::GetSizeTValue(json_doc_file_, KEY_SIZET, parsed_sizet);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(static_cast<size_t>(EXPECTED_VALUE_SIZET), parsed_sizet);
                rc = util::JsonParser::GetSizeTValue(json_doc_str_, KEY_INVALID, parsed_sizet);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR, rc);
                rc = util::JsonParser::GetSizeTValue(json_doc_str_, KEY_STRING, parsed_sizet);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR, rc);

                bool parsed_bool = false;
                rc = util::JsonParser::GetBoolValue(json_doc_str_, KEY_BOOL_TRUE, parsed_bool);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_TRUE(parsed_bool);
                parsed_bool = true;
                rc = util::JsonParser::GetBoolValue(json_doc_file_, KEY_BOOL_FALSE, parsed_bool);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_FALSE(parsed_bool);
                rc = util::JsonParser::GetBoolValue(json_doc_str_, KEY_INVALID, parsed_bool);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR, rc);
                rc = util::JsonParser::GetBoolValue(json_doc_str_, KEY_STRING, parsed_bool);
                EXPECT_EQ(ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR, rc);
            }

            TEST_F(JsonParserTester, RunMergeTest) {
                util::JsonDocument source_doc;
                util::JsonDocument target_doc;
                util::JsonDocument expected_doc;
                util::JsonValue empty_json;

                ResponseCode rc = util::JsonParser::MergeValues(empty_json, empty_json, target_doc.GetAllocator());
                EXPECT_EQ(ResponseCode::JSON_MERGE_FAILED, rc);

                rc = util::JsonParser::InitializeFromJsonString(source_doc, JSON_MERGE_TEST_SOURCE_DOCUMENT_STRING);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                rc = util::JsonParser::InitializeFromJsonString(target_doc, JSON_MERGE_TEST_TARGET_DOCUMENT_STRING);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                rc = util::JsonParser::InitializeFromJsonString(expected_doc, JSON_MERGE_TEST_MERGED_DOCUMENT_STRING);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                rc = util::JsonParser::MergeValues(target_doc, source_doc, target_doc.GetAllocator());
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                // Json library has overloaded == operator
                EXPECT_TRUE(target_doc == expected_doc);
            }

            TEST_F(JsonParserTester, RunDiffTest) {
                util::JsonDocument old_doc;
                util::JsonDocument new_doc;
                util::JsonDocument target_doc;
                util::JsonDocument expected_doc;
                util::JsonValue  empty_json;

                ResponseCode rc = util::JsonParser::DiffValues(target_doc, empty_json, empty_json, target_doc.GetAllocator());
                EXPECT_EQ(ResponseCode::JSON_MERGE_FAILED, rc);

                rc = util::JsonParser::InitializeFromJsonString(old_doc, JSON_MERGE_TEST_SOURCE_DOCUMENT_STRING);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                rc = util::JsonParser::InitializeFromJsonString(new_doc, JSON_MERGE_TEST_TARGET_DOCUMENT_STRING);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                rc = util::JsonParser::InitializeFromJsonString(expected_doc, JSON_MERGE_TEST_DIFF_DOCUMENT_STRING);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                rc = util::JsonParser::DiffValues(target_doc, old_doc, new_doc, target_doc.GetAllocator());
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                // Json library has overloaded == operator
                EXPECT_TRUE(target_doc == expected_doc);
            }

            TEST_F(JsonParserTester, BrokenJsonTest) {
                util::JsonDocument broken_doc;
                util::String zero_length_string("");
                util::String broken_string(BROKEN_JSON_STRING);

                ResponseCode rc = util::JsonParser::InitializeFromJsonString(broken_doc, zero_length_string);
                EXPECT_EQ(ResponseCode::NULL_VALUE_ERROR, rc);

                std::ofstream out("broken_json_test.json");
                out << broken_string;
                out.close();

                rc = util::JsonParser::InitializeFromJsonFile(broken_doc, "broken_json_test.json");
                EXPECT_EQ(ResponseCode::JSON_PARSING_ERROR, rc);

                rapidjson::ParseErrorCode error_code = util::JsonParser::GetParseErrorCode(broken_doc);
                EXPECT_NE(0, error_code);

                size_t error_offset = util::JsonParser::GetParseErrorOffset(broken_doc);
                EXPECT_NE((size_t)0, error_offset);

                util::JsonDocument file_json;
                util::String document(JSON_MERGE_TEST_SOURCE_DOCUMENT_STRING);

                rc = util::JsonParser::InitializeFromJsonString(file_json, document);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
            }

            TEST_F(JsonParserTester, WriteToFileTest) {
                util::JsonDocument file_json;

                const util::String file_name = "js>.json";
                util::String null_file_name("");
                ResponseCode rc = util::JsonParser::WriteToFile(file_json, null_file_name);
                EXPECT_EQ(ResponseCode::FILE_NAME_INVALID, rc);
           }

            TEST_F(JsonParserTester, StringConversionTest) {
                util::JsonDocument string_test_json;

                util::JsonValue key(SINGLE_VALUE_JSON_KEY, string_test_json.GetAllocator());

                util::String json_value_string = util::JsonParser::ToString(key);

                EXPECT_EQ(JSON_STRING_OUTPUT, json_value_string);
            }
        }
    }
}
