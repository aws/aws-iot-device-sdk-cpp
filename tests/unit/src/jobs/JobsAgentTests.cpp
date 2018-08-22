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

#include "ConfigCommon.hpp"

#include "../../../../samples/JobsAgent/JobsAgent.hpp"

#define JOBS_AGENT_TEST_LOG_TAG "[Jobs Agent Unit Test]"

#define INSTALLED_PACKAGES_FILENAME_TEST "installedPackagesTest.json"

namespace awsiotsdk {
    util::String JobsMock::last_update_payload_;

    namespace samples {
        ResponseCode JobsAgent::InitializeTLS() {
            p_network_connection_ = std::make_shared<tests::mocks::MockNetworkConnection>();
            return ResponseCode::SUCCESS;
        }
    }

    namespace tests {
        namespace unit {
            class JobsAgentTestWrapper : public samples::JobsAgent {
            public:
                JobsAgentTestWrapper() {
                    p_jobs_ = std::make_shared<JobsMock>();
                    installed_packages_filename_ = INSTALLED_PACKAGES_FILENAME_TEST;

                    SetInstalledPackages("{}");
                }

                void SetInstalledPackages(util::String installedPackages) {
                    util::JsonParser::InitializeFromJsonString(installed_packages_json_, installedPackages);
                }

                ResponseCode NextJobCallback(util::String topic_name,
                                             util::String payload,
                                             std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
                    return JobsAgent::NextJobCallback(topic_name, payload, p_app_handler_data);
                }

                ResponseCode UpdateAcceptedCallback(util::String topic_name,
                                                    util::String payload,
                                                    std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
                    return JobsAgent::UpdateAcceptedCallback(topic_name, payload, p_app_handler_data);
                }

                ResponseCode UpdateRejectedCallback(util::String topic_name,
                                                    util::String payload,
                                                    std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
                    return JobsAgent::UpdateRejectedCallback(topic_name, payload, p_app_handler_data);
                }
            };

            class JobsAgentTester : public ::testing::Test {
            protected:
                std::shared_ptr<JobsAgentTestWrapper> p_jobs_agent_;

                JobsAgentTester() {
                    p_jobs_agent_ = std::shared_ptr<JobsAgentTestWrapper>(new JobsAgentTestWrapper());
                }
            };

            TEST_F(JobsAgentTester, UnhandledOperation) {
                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"unhandled\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"FAILED\",\"statusDetails\":{\"errorCode\":\"ERR_UNEXPECTED\",\"errorMessage\":\"unhandled operation\",\"operation\":\"unhandled\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());
            }

            TEST_F(JobsAgentTester, MissingJobDocument) {
                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\"}}", nullptr);
                EXPECT_EQ("{\"status\":\"FAILED\",\"statusDetails\":{\"errorCode\":\"ERR_UNEXPECTED\",\"errorMessage\":\"unable to process job document\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());
            }

            TEST_F(JobsAgentTester, ShutdownHandler) {
                p_jobs_agent_->UpdateAcceptedCallback("TestTopicName", "{\"executionState\":{\"statusDetails\":{\"step\":\"test\"}},\"jobDocument\":{\"operation\":\"reboot\"}}", nullptr);
                p_jobs_agent_->UpdateAcceptedCallback("TestTopicName", "{\"executionState\":{\"statusDetails\":{\"step\":\"test\"}},\"jobDocument\":{\"operation\":\"shutdown\"}}", nullptr);

                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"shutdown\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"SUCCEEDED\",\"statusDetails\":{\"operation\":\"shutdown\",\"step\":\"initiated\"},\"includeJobExecutionState\":\"true\",\"includeJobDocument\":\"true\",\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"reboot\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"IN_PROGRESS\",\"statusDetails\":{\"operation\":\"reboot\",\"step\":\"initiated\"},\"includeJobExecutionState\":\"true\",\"includeJobDocument\":\"true\",\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"reboot\"},\"statusDetails\":{\"step\":\"initiated\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"SUCCEEDED\",\"statusDetails\":{\"operation\":\"reboot\",\"step\":\"completed\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());
            }

            TEST_F(JobsAgentTester, InstallHandler) {
                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"install\",\"workingDirectory\":\".\",\"files\":[]}}}", nullptr);
                EXPECT_EQ("{\"status\":\"FAILED\",\"statusDetails\":{\"errorCode\":\"ERR_UNNAMED_PACKAGE\",\"errorMessage\":\"installed packages must have packageName string property\",\"operation\":\"install\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"install\",\"packageName\":\"uniquePackageName\",\"workingDirectory\":\".\",\"files\":[]}}}", nullptr);
                EXPECT_EQ("{\"status\":\"FAILED\",\"statusDetails\":{\"errorCode\":\"ERR_FILE_COPY_FAILED\",\"errorMessage\":\"files property missing or invalid\",\"operation\":\"install\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"install\",\"packageName\":\"uniquePackageName\",\"workingDirectory\":\".\",\"files\":[{\"fileName\":\"test1.txt\",\"fileSource\":{\"url\":\"https://invalid-url/test1.txt\"},\"checksum\":{\"inline\":{\"value\":\"12345\"},\"hashAlgorithm\":\"test\"}},{\"fileName\":\"test2.txt\",\"fileSource\":{\"url\":\"https://invalid-url/test2.txt\"}}]}}}", nullptr);
                EXPECT_EQ("{\"status\":\"FAILED\",\"statusDetails\":{\"curlError\":\"Couldn't resolve host name\",\"errorCode\":\"ERR_DOWNLOAD_FAILED\",\"errorMessage\":\"curl error encountered\",\"fileSourceUrl\":\"https://invalid-url/test1.txt\",\"operation\":\"install\",\"packageName\":\"uniquePackageName\",\"step\":\"rollback files\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"install\",\"packageName\":\"uniquePackageName\",\"workingDirectory\":\"/\",\"files\":[{\"fileName\":\"test1.txt\",\"fileSource\":{\"url\":\"https://invalid-url/test1.txt\"},\"checksum\":{\"inline\":{\"value\":\"12345\"},\"hashAlgorithm\":\"test\"}},{\"fileName\":\"test2.txt\",\"fileSource\":{\"url\":\"https://invalid-url/test2.txt\"}}]}}}", nullptr);
                EXPECT_EQ("{\"status\":\"FAILED\",\"statusDetails\":{\"errorCode\":\"ERR_FILE_COPY_FAILED\",\"errorMessage\":\"unable to backup file\",\"fileName\":\"/test1.txt\",\"operation\":\"install\",\"packageName\":\"uniquePackageName\",\"step\":\"backup files\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->SetInstalledPackages("{\"testPackage1\":{\"packageName\":\"testPackage1\"},\"testPackage2\":{\"packageName\":\"testPackage2\",\"launchCommand\":\"nop\"}}");
                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"install\",\"packageName\":\"uniquePackageName\",\"workingDirectory\":\"/tmp\",\"launchCommand\":\"nop\",\"autoStart\":true,\"files\":[{\"fileName\":\"test2.txt\",\"fileSource\":{\"url\":\"https://www.amazon.com\"}}]}}}", nullptr);
                EXPECT_EQ("{\"status\":\"SUCCEEDED\",\"statusDetails\":{\"operation\":\"install\",\"packageName\":\"uniquePackageName\",\"step\":\"completed\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->SetInstalledPackages("{\"testPackage1\":{\"packageName\":\"testPackage1\"},\"testPackage2\":{\"packageName\":\"testPackage2\",\"launchCommand\":\"nop\"}}");
                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"install\",\"packageName\":\"uniquePackageName\",\"launchCommand\":\"nop\",\"autoStart\":true,\"files\":[{\"fileName\":\"/test2.txt\",\"fileSource\":{\"url\":\"https://www.amazon.com\"}}]}}}", nullptr);
                EXPECT_EQ("{\"status\":\"FAILED\",\"statusDetails\":{\"errorCode\":\"ERR_DOWNLOAD_FAILED\",\"errorMessage\":\"unable to open file for writing\",\"operation\":\"install\",\"packageName\":\"uniquePackageName\",\"step\":\"rollback files\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());
            }

            TEST_F(JobsAgentTester, UninstallHandler) {
                p_jobs_agent_->SetInstalledPackages("{\"testPackage1\":{\"packageName\":\"testPackage1\"},\"testPackage2\":{\"packageName\":\"testPackage2\",\"launchCommand\":\"nop\"}}");
                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"uninstall\",\"packageName\":\"invalidPackageName\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"FAILED\",\"statusDetails\":{\"errorCode\":\"ERR_INVALID_PACKAGE_NAME\",\"errorMessage\":\"no package found with name invalidPackageName\",\"operation\":\"uninstall\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"uninstall\",\"packageName\":\"testPackage1\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"SUCCEEDED\",\"statusDetails\":{\"operation\":\"uninstall\"},\"clientToken\":\"testClientToken\"}", JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"start\",\"packageName\":\"testPackage2\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"SUCCEEDED\",\"statusDetails\":{\"operation\":\"start\",\"step\":\"completed\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"uninstall\",\"packageName\":\"testPackage2\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"SUCCEEDED\",\"statusDetails\":{\"operation\":\"uninstall\",\"step\":\"stop package\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"uninstall\",\"packageName\":\"testPackage2\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"FAILED\",\"statusDetails\":{\"errorCode\":\"ERR_INVALID_PACKAGE_NAME\",\"errorMessage\":\"no package found with name testPackage2\",\"operation\":\"uninstall\"},\"clientToken\":\"testClientToken\"}", JobsMock::GetLastUpdatePayload());
            }

            TEST_F(JobsAgentTester, StartPackageHandler) {
                p_jobs_agent_->SetInstalledPackages("{\"testPackage1\":{\"packageName\":\"testPackage1\"},\"testPackage2\":{\"packageName\":\"testPackage2\",\"launchCommand\":\"nop\"}}");
                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"start\",\"packageName\":\"testPackage1\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"FAILED\",\"statusDetails\":{\"errorCode\":\"ERR_UNABLE_TO_START_PACKAGE\",\"errorMessage\":\"package is not executable\",\"operation\":\"start\",\"step\":\"start package\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"start\",\"packageName\":\"testPackage2\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"SUCCEEDED\",\"statusDetails\":{\"operation\":\"start\",\"step\":\"completed\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"start\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"FAILED\",\"statusDetails\":{\"errorCode\":\"ERR_UNNAMED_PACKAGE\",\"errorMessage\":\"must specify packageName\",\"operation\":\"start\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());
            }

            TEST_F(JobsAgentTester, StopPackageHandler) {
                p_jobs_agent_->SetInstalledPackages("{\"testPackage1\":{\"packageName\":\"testPackage1\"},\"testPackage2\":{\"packageName\":\"testPackage2\",\"launchCommand\":\"nop\"}}");
                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"stop\",\"packageName\":\"testPackage1\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"FAILED\",\"statusDetails\":{\"errorCode\":\"ERR_UNABLE_TO_STOP_PACKAGE\",\"errorMessage\":\"package is not running\",\"operation\":\"stop\",\"step\":\"stop package\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());
            }

            TEST_F(JobsAgentTester, RestartPackageHandler) {
                p_jobs_agent_->SetInstalledPackages("{\"testPackage1\":{\"packageName\":\"testPackage1\"},\"testPackage2\":{\"packageName\":\"testPackage2\",\"launchCommand\":\"nop\"}}");
                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"restart\",\"packageName\":\"testPackage1\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"FAILED\",\"statusDetails\":{\"errorCode\":\"ERR_UNABLE_TO_START_PACKAGE\",\"errorMessage\":\"package is not executable\",\"operation\":\"restart\",\"step\":\"start package\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"restart\",\"packageName\":\"testPackage2\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"SUCCEEDED\",\"statusDetails\":{\"operation\":\"restart\",\"step\":\"completed\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());
            }

            TEST_F(JobsAgentTester, SystemStatusHandler) {
                p_jobs_agent_->SetInstalledPackages("{\"testPackage1\":{\"packageName\":\"testPackage1\"},\"testPackage2\":{\"packageName\":\"testPackage2\",\"launchCommand\":\"nop\"}}");
                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"systemStatus\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"SUCCEEDED\",\"statusDetails\":{\"installedPackages\":\"[\\\"testPackage1\\\",\\\"testPackage2\\\"]\",\"operation\":\"systemStatus\",\"runningPackages\":\"[]\",\"title\":\"\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->SetInstalledPackages("{\"testPackage1\":{\"packageName\":\"testPackage1\"},\"testPackage2\":{\"packageName\":\"testPackage2\",\"launchCommand\":\"nop\"}}");
                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"systemStatus\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"SUCCEEDED\",\"statusDetails\":{\"installedPackages\":\"[\\\"testPackage1\\\",\\\"testPackage2\\\"]\",\"operation\":\"systemStatus\",\"runningPackages\":\"[]\",\"title\":\"\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());

                p_jobs_agent_->SetInstalledPackages("{}");
                p_jobs_agent_->NextJobCallback("TestTopicName", "{\"execution\":{\"jobId\":\"TestJobId\",\"jobDocument\":{\"operation\":\"systemStatus\"}}}", nullptr);
                EXPECT_EQ("{\"status\":\"SUCCEEDED\",\"statusDetails\":{\"installedPackages\":\"[]\",\"operation\":\"systemStatus\",\"runningPackages\":\"[]\",\"title\":\"\"},\"clientToken\":\"testClientToken\"}",
                          JobsMock::GetLastUpdatePayload());
            }

            TEST_F(JobsAgentTester, UpdateRejectedCallback) {
                EXPECT_EQ(ResponseCode::SUCCESS, p_jobs_agent_->UpdateRejectedCallback("TestTopicName", "payload", nullptr));
            }
        }
    }
}
