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
 * @file JobsTests.cpp
 * @brief
 *
 */

#include <chrono>

#include <gtest/gtest.h>

#include "util/logging/LogMacros.hpp"

#include "TestHelper.hpp"
#include "MockNetworkConnection.hpp"

#include "jobs/Jobs.hpp"
#include "mqtt/ClientState.hpp"

#define JOBS_TEST_LOG_TAG "[Jobs Unit Test]"

namespace awsiotsdk {
    namespace tests {
        namespace unit {
            class JobsTestWrapper : public Jobs {
            protected:
                static const util::String test_thing_name_;
                static const util::String client_token_;

            public:
                JobsTestWrapper(bool empty_thing_name, bool empty_client_token):
                    Jobs(nullptr, mqtt::QoS::QOS0,
                         empty_thing_name ? "" : test_thing_name_,
                         empty_client_token ? "" : client_token_) {}

                util::String SerializeStatusDetails(const util::Map<util::String, util::String> &statusDetailsMap) {
                    return Jobs::SerializeStatusDetails(statusDetailsMap);
                }

                util::String SerializeJobExecutionUpdatePayload(JobExecutionStatus status,
                                                                const util::Map<util::String, util::String> &statusDetailsMap = util::Map<util::String, util::String>(),
                                                                int64_t expectedVersion = 0,
                                                                int64_t executionNumber = 0,
                                                                bool includeJobExecutionState = false,
                                                                bool includeJobDocument = false) {
                    return Jobs::SerializeJobExecutionUpdatePayload(status, statusDetailsMap, expectedVersion, executionNumber, includeJobExecutionState, includeJobDocument);
                }

                util::String SerializeDescribeJobExecutionPayload(int64_t executionNumber = 0,
                                                                  bool includeJobDocument = true) {
                    return Jobs::SerializeDescribeJobExecutionPayload(executionNumber, includeJobDocument);
                }

                util::String SerializeStartNextPendingJobExecutionPayload(const util::Map<util::String, util::String> &statusDetailsMap = util::Map<util::String, util::String>()) {
                    return Jobs::SerializeStartNextPendingJobExecutionPayload(statusDetailsMap);
                }

                util::String SerializeClientTokenPayload() {
                    return Jobs::SerializeClientTokenPayload();
                }

                util::String Escape(const util::String &value) {
                    return Jobs::Escape(value);
                }
            };

            const util::String JobsTestWrapper::test_thing_name_ = "CppSdkTestClient";
            const util::String JobsTestWrapper::client_token_ = "CppSdkTestClientToken";

            class JobsTester : public ::testing::Test {
            protected:
                static const util::String job_id_;

                std::shared_ptr<JobsTestWrapper> p_jobs_;
                std::shared_ptr<JobsTestWrapper> p_jobs_empty_client_token_;
                std::shared_ptr<JobsTestWrapper> p_jobs_empty_thing_name_;

                JobsTester() {
                    p_jobs_ = std::shared_ptr<JobsTestWrapper>(new JobsTestWrapper(false, false));
                    p_jobs_empty_client_token_ = std::shared_ptr<JobsTestWrapper>(new JobsTestWrapper(false, true));
                    p_jobs_empty_thing_name_ = std::shared_ptr<JobsTestWrapper>(new JobsTestWrapper(true, false));
                }
            };

            const util::String JobsTester::job_id_ = "TestJobId";

            TEST_F(JobsTester, ValidTopicsTests) {
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/get", p_jobs_->GetJobTopic(Jobs::JOB_GET_PENDING_TOPIC)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/get/accepted", p_jobs_->GetJobTopic(Jobs::JOB_GET_PENDING_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/get/rejected", p_jobs_->GetJobTopic(Jobs::JOB_GET_PENDING_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/get/#", p_jobs_->GetJobTopic(Jobs::JOB_GET_PENDING_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE)->ToStdString());

                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/TestJobId/get", p_jobs_->GetJobTopic(Jobs::JOB_DESCRIBE_TOPIC, Jobs::JOB_REQUEST_TYPE, job_id_)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/TestJobId/get/accepted", p_jobs_->GetJobTopic(Jobs::JOB_DESCRIBE_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE, job_id_)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/TestJobId/get/rejected", p_jobs_->GetJobTopic(Jobs::JOB_DESCRIBE_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE, job_id_)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/TestJobId/get/#", p_jobs_->GetJobTopic(Jobs::JOB_DESCRIBE_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE, job_id_)->ToStdString());

                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/start-next", p_jobs_->GetJobTopic(Jobs::JOB_START_NEXT_TOPIC)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/start-next/accepted", p_jobs_->GetJobTopic(Jobs::JOB_START_NEXT_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/start-next/rejected", p_jobs_->GetJobTopic(Jobs::JOB_START_NEXT_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/start-next/#", p_jobs_->GetJobTopic(Jobs::JOB_START_NEXT_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE)->ToStdString());

                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/TestJobId/update", p_jobs_->GetJobTopic(Jobs::JOB_UPDATE_TOPIC, Jobs::JOB_REQUEST_TYPE, job_id_)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/TestJobId/update/accepted", p_jobs_->GetJobTopic(Jobs::JOB_UPDATE_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE, job_id_)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/TestJobId/update/rejected", p_jobs_->GetJobTopic(Jobs::JOB_UPDATE_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE, job_id_)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/TestJobId/update/#", p_jobs_->GetJobTopic(Jobs::JOB_UPDATE_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE, job_id_)->ToStdString());

                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/notify", p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_TOPIC)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/notify-next", p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_NEXT_TOPIC)->ToStdString());

                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/#", p_jobs_->GetJobTopic(Jobs::JOB_WILDCARD_TOPIC)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/#", p_jobs_->GetJobTopic(Jobs::JOB_WILDCARD_TOPIC, Jobs::JOB_REQUEST_TYPE, job_id_)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/#", p_jobs_->GetJobTopic(Jobs::JOB_WILDCARD_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/#", p_jobs_->GetJobTopic(Jobs::JOB_WILDCARD_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/#", p_jobs_->GetJobTopic(Jobs::JOB_WILDCARD_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/#", p_jobs_->GetJobTopic(Jobs::JOB_WILDCARD_TOPIC, Jobs::JOB_REQUEST_TYPE, job_id_)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/#", p_jobs_->GetJobTopic(Jobs::JOB_WILDCARD_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE, job_id_)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/#", p_jobs_->GetJobTopic(Jobs::JOB_WILDCARD_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE, job_id_)->ToStdString());
                EXPECT_EQ("$aws/things/CppSdkTestClient/jobs/#", p_jobs_->GetJobTopic(Jobs::JOB_WILDCARD_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE, job_id_)->ToStdString());
            }

            TEST_F(JobsTester, InvalidTopicsTests) {
                EXPECT_EQ(nullptr, p_jobs_empty_thing_name_->GetJobTopic(Jobs::JOB_WILDCARD_TOPIC));
                EXPECT_EQ(nullptr, p_jobs_empty_thing_name_->GetJobTopic(Jobs::JOB_GET_PENDING_TOPIC));
                EXPECT_EQ(nullptr, p_jobs_empty_thing_name_->GetJobTopic(Jobs::JOB_START_NEXT_TOPIC));
                EXPECT_EQ(nullptr, p_jobs_empty_thing_name_->GetJobTopic(Jobs::JOB_NOTIFY_TOPIC));
                EXPECT_EQ(nullptr, p_jobs_empty_thing_name_->GetJobTopic(Jobs::JOB_NOTIFY_NEXT_TOPIC));
                EXPECT_EQ(nullptr, p_jobs_empty_thing_name_->GetJobTopic(Jobs::JOB_UNRECOGNIZED_TOPIC));
                EXPECT_EQ(nullptr, p_jobs_empty_thing_name_->GetJobTopic(Jobs::JOB_DESCRIBE_TOPIC));
                EXPECT_EQ(nullptr, p_jobs_empty_thing_name_->GetJobTopic(Jobs::JOB_UPDATE_TOPIC));

                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_UNRECOGNIZED_TOPIC));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_UNRECOGNIZED_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_UNRECOGNIZED_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_UNRECOGNIZED_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_UNRECOGNIZED_TOPIC, Jobs::JOB_REQUEST_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_UNRECOGNIZED_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_UNRECOGNIZED_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_UNRECOGNIZED_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE, job_id_));

                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_GET_PENDING_TOPIC, Jobs::JOB_REQUEST_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_GET_PENDING_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_GET_PENDING_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_GET_PENDING_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE, job_id_));

                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_DESCRIBE_TOPIC));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_DESCRIBE_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_DESCRIBE_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_DESCRIBE_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE));

                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_START_NEXT_TOPIC, Jobs::JOB_REQUEST_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_START_NEXT_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_START_NEXT_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_START_NEXT_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE, job_id_));

                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_UPDATE_TOPIC));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_UPDATE_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_UPDATE_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_UPDATE_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE));

                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_TOPIC, Jobs::JOB_REQUEST_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE, job_id_));

                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_NEXT_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_NEXT_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_NEXT_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_NEXT_TOPIC, Jobs::JOB_REQUEST_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_NEXT_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_NEXT_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_NOTIFY_NEXT_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE, job_id_));

                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_START_NEXT_TOPIC, Jobs::JOB_ACCEPTED_REPLY_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_START_NEXT_TOPIC, Jobs::JOB_REJECTED_REPLY_TYPE, job_id_));
                EXPECT_EQ(nullptr, p_jobs_->GetJobTopic(Jobs::JOB_START_NEXT_TOPIC, Jobs::JOB_WILDCARD_REPLY_TYPE, job_id_));
            }


            TEST_F(JobsTester, PayloadSerializationTests) {
                util::Map<util::String, util::String> statusDetailsMap;
                statusDetailsMap.insert(std::make_pair("testKey", "testVal"));

                EXPECT_EQ("{}", p_jobs_empty_client_token_->SerializeClientTokenPayload());
                EXPECT_EQ("{\"clientToken\":\"CppSdkTestClientToken\"}", p_jobs_->SerializeClientTokenPayload());

                EXPECT_EQ("{}", p_jobs_empty_client_token_->SerializeStartNextPendingJobExecutionPayload());
                EXPECT_EQ("{\"statusDetails\":{\"testKey\":\"testVal\"}}", p_jobs_empty_client_token_->SerializeStartNextPendingJobExecutionPayload(statusDetailsMap));
                EXPECT_EQ("{\"clientToken\":\"CppSdkTestClientToken\"}", p_jobs_->SerializeStartNextPendingJobExecutionPayload());
                EXPECT_EQ("{\"statusDetails\":{\"testKey\":\"testVal\"},\"clientToken\":\"CppSdkTestClientToken\"}", p_jobs_->SerializeStartNextPendingJobExecutionPayload(statusDetailsMap));

                EXPECT_EQ("{\"includeJobDocument\":\"true\"}", p_jobs_empty_client_token_->SerializeDescribeJobExecutionPayload());
                EXPECT_EQ("{\"includeJobDocument\":\"true\",\"executionNumber\":\"1\"}", p_jobs_empty_client_token_->SerializeDescribeJobExecutionPayload(1));
                EXPECT_EQ("{\"includeJobDocument\":\"false\",\"executionNumber\":\"1\"}", p_jobs_empty_client_token_->SerializeDescribeJobExecutionPayload(1, false));

                EXPECT_EQ("{\"includeJobDocument\":\"true\"\"clientToken\":\"CppSdkTestClientToken\"}", p_jobs_->SerializeDescribeJobExecutionPayload());
                EXPECT_EQ("{\"includeJobDocument\":\"true\",\"executionNumber\":\"1\"\"clientToken\":\"CppSdkTestClientToken\"}", p_jobs_->SerializeDescribeJobExecutionPayload(1));
                EXPECT_EQ("{\"includeJobDocument\":\"false\",\"executionNumber\":\"1\"\"clientToken\":\"CppSdkTestClientToken\"}", p_jobs_->SerializeDescribeJobExecutionPayload(1, false));

                EXPECT_EQ("", p_jobs_empty_client_token_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_STATUS_NOT_SET));
                EXPECT_EQ("", p_jobs_empty_client_token_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_UNKNOWN_STATUS));
                EXPECT_EQ("", p_jobs_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_STATUS_NOT_SET));
                EXPECT_EQ("", p_jobs_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_UNKNOWN_STATUS));

                EXPECT_EQ("{\"status\":\"QUEUED\"}", p_jobs_empty_client_token_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_QUEUED));
                EXPECT_EQ("{\"status\":\"QUEUED\",\"statusDetails\":{\"testKey\":\"testVal\"}}",
                          p_jobs_empty_client_token_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_QUEUED, statusDetailsMap));
                EXPECT_EQ("{\"status\":\"QUEUED\",\"statusDetails\":{\"testKey\":\"testVal\"},\"expectedVersion\":\"1\"}",
                          p_jobs_empty_client_token_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_QUEUED, statusDetailsMap, 1));
                EXPECT_EQ("{\"status\":\"QUEUED\",\"statusDetails\":{\"testKey\":\"testVal\"},\"expectedVersion\":\"1\",\"executionNumber\":\"1\"}",
                          p_jobs_empty_client_token_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_QUEUED, statusDetailsMap, 1, 1));
                EXPECT_EQ("{\"status\":\"QUEUED\",\"statusDetails\":{\"testKey\":\"testVal\"},\"expectedVersion\":\"1\",\"executionNumber\":\"1\",\"includeJobExecutionState\":\"true\"}",
                          p_jobs_empty_client_token_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_QUEUED, statusDetailsMap, 1, 1, true));
                EXPECT_EQ("{\"status\":\"QUEUED\",\"statusDetails\":{\"testKey\":\"testVal\"},\"expectedVersion\":\"1\",\"executionNumber\":\"1\",\"includeJobExecutionState\":\"true\",\"includeJobDocument\":\"true\"}",
                          p_jobs_empty_client_token_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_QUEUED, statusDetailsMap, 1, 1, true, true));

                EXPECT_EQ("{\"status\":\"IN_PROGRESS\"}", p_jobs_empty_client_token_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_IN_PROGRESS));
                EXPECT_EQ("{\"status\":\"FAILED\"}", p_jobs_empty_client_token_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_FAILED));
                EXPECT_EQ("{\"status\":\"SUCCEEDED\"}", p_jobs_empty_client_token_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_SUCCEEDED));
                EXPECT_EQ("{\"status\":\"CANCELED\"}", p_jobs_empty_client_token_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_CANCELED));
                EXPECT_EQ("{\"status\":\"REJECTED\"}", p_jobs_empty_client_token_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_REJECTED));

                EXPECT_EQ("{\"status\":\"QUEUED\",\"clientToken\":\"CppSdkTestClientToken\"}",
                          p_jobs_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_QUEUED));
                EXPECT_EQ("{\"status\":\"QUEUED\",\"statusDetails\":{\"testKey\":\"testVal\"},\"clientToken\":\"CppSdkTestClientToken\"}",
                          p_jobs_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_QUEUED, statusDetailsMap));
                EXPECT_EQ("{\"status\":\"QUEUED\",\"statusDetails\":{\"testKey\":\"testVal\"},\"expectedVersion\":\"1\",\"clientToken\":\"CppSdkTestClientToken\"}",
                          p_jobs_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_QUEUED, statusDetailsMap, 1));
                EXPECT_EQ("{\"status\":\"QUEUED\",\"statusDetails\":{\"testKey\":\"testVal\"},\"expectedVersion\":\"1\",\"executionNumber\":\"1\",\"clientToken\":\"CppSdkTestClientToken\"}",
                          p_jobs_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_QUEUED, statusDetailsMap, 1, 1));
                EXPECT_EQ("{\"status\":\"QUEUED\",\"statusDetails\":{\"testKey\":\"testVal\"},\"expectedVersion\":\"1\",\"executionNumber\":\"1\",\"includeJobExecutionState\":\"true\",\"clientToken\":\"CppSdkTestClientToken\"}",
                          p_jobs_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_QUEUED, statusDetailsMap, 1, 1, true));
                EXPECT_EQ("{\"status\":\"QUEUED\",\"statusDetails\":{\"testKey\":\"testVal\"},\"expectedVersion\":\"1\",\"executionNumber\":\"1\",\"includeJobExecutionState\":\"true\",\"includeJobDocument\":\"true\",\"clientToken\":\"CppSdkTestClientToken\"}",
                          p_jobs_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_QUEUED, statusDetailsMap, 1, 1, true, true));

                EXPECT_EQ("{\"status\":\"IN_PROGRESS\",\"clientToken\":\"CppSdkTestClientToken\"}", p_jobs_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_IN_PROGRESS));
                EXPECT_EQ("{\"status\":\"FAILED\",\"clientToken\":\"CppSdkTestClientToken\"}", p_jobs_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_FAILED));
                EXPECT_EQ("{\"status\":\"SUCCEEDED\",\"clientToken\":\"CppSdkTestClientToken\"}", p_jobs_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_SUCCEEDED));
                EXPECT_EQ("{\"status\":\"CANCELED\",\"clientToken\":\"CppSdkTestClientToken\"}", p_jobs_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_CANCELED));
                EXPECT_EQ("{\"status\":\"REJECTED\",\"clientToken\":\"CppSdkTestClientToken\"}", p_jobs_->SerializeJobExecutionUpdatePayload(Jobs::JOB_EXECUTION_REJECTED));

                statusDetailsMap.insert(std::make_pair("testEscapeKey \" \t \r \n \\ '!", "testEscapeVal \" \t \r \n \\ '!"));
                EXPECT_EQ("{\"testEscapeKey \\\" \\t \\r \\n \\\\ '!\":\"testEscapeVal \\\" \\t \\r \\n \\\\ '!\",\"testKey\":\"testVal\"}", p_jobs_->SerializeStatusDetails(statusDetailsMap));
            }
        }
    }
}
