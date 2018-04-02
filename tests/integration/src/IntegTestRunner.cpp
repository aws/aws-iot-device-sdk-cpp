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
 * @file IntegTestRunner.cpp
 * @brief
 *
 */

#include "util/memory/stl/String.hpp"

#include "util/logging/Logging.hpp"
#include "util/logging/LogMacros.hpp"
#include "util/logging/ConsoleLogSystem.hpp"

#include "ConfigCommon.hpp"
#include "IntegTestRunner.hpp"
#include "SdkTestConfig.hpp"
#include "JobsTest.hpp"
#include "PubSub.hpp"
#include "AutoReconnect.hpp"
#include "MultipleClients.hpp"
#include "MultipleSubAutoReconnect.hpp"

#define INTEG_TEST_RUNNER_LOG_TAG "[Integration Test Runner]"

#define MAX_ALLOWED_SUBSCRIPTIONS 50

namespace awsiotsdk {
    namespace tests {
        namespace integration {
            ResponseCode IntegTestRunner::Initialize() {
                ResponseCode rc = awsiotsdk::ConfigCommon::InitializeCommon("config/IntegrationTestConfig.json");
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_INFO(INTEG_TEST_RUNNER_LOG_TAG,
                                 "Initialize Test Config Failed with rc : %d",
                                 static_cast<int>(rc));
                }
                return rc;
            }

            ResponseCode IntegTestRunner::RunAllTests() {
                ResponseCode rc = ResponseCode::SUCCESS;
                // Each test runs in its own scope to ensure complete cleanup
                /**
                 * Run Jobs Tests
                 */
                {
                    JobsTest jobs_test_runner;
                    rc = jobs_test_runner.RunTest();
                    if (ResponseCode::SUCCESS != rc) {
                        return rc;
                    }
                }

                /**
                 * Run Subscribe Publish Tests
                 */
                {
                    PubSub pub_sub_runner;
                    rc = pub_sub_runner.RunTest();
                    if (ResponseCode::SUCCESS != rc) {
                        return rc;
                    }
                }

                /**
                 * Run Autoreconnect test
                 */
                {
                    AutoReconnect auto_reconnect_runner;
                    rc = auto_reconnect_runner.RunTest();
                    if (ResponseCode::SUCCESS != rc) {
                        return rc;
                    }
                }

                /**
                 * Run Multiple Clients test
                 */
                {
                    MultipleClients multiple_client_runner;
                    rc = multiple_client_runner.RunTest();
                    if (ResponseCode::SUCCESS != rc) {
                        return rc;
                    }
                }

                /**
                 * Run Multiple Subscription auto reconnect test with 0 topics
                 */
                {
                    MultipleSubAutoReconnect multiple_sub_runner(0);
                    rc = multiple_sub_runner.RunTest();
                    if (ResponseCode::SUCCESS != rc) {
                        return rc;
                    }
                }

                /**
                 * Run Multiple Subscription auto reconnect test with 8 topics
                 */
                {
                    MultipleSubAutoReconnect multiple_sub_runner(8);
                    rc = multiple_sub_runner.RunTest();
                    if (ResponseCode::SUCCESS != rc) {
                        return rc;
                    }
                }

                /**
                 * Run Multiple Subscription auto reconnect test with 50 topics
                 */
                {
                    MultipleSubAutoReconnect multiple_sub_runner(MAX_ALLOWED_SUBSCRIPTIONS);
                    rc = multiple_sub_runner.RunTest();
                    if (ResponseCode::SUCCESS != rc) {
                        return rc;
                    }
                }

                return rc;
            }
        }
    }
}

int main(int argc, char **argv) {
    std::shared_ptr<awsiotsdk::util::Logging::ConsoleLogSystem> p_log_system =
        std::make_shared<awsiotsdk::util::Logging::ConsoleLogSystem>(awsiotsdk::util::Logging::LogLevel::Info);
    awsiotsdk::util::Logging::InitializeAWSLogging(p_log_system);

    std::unique_ptr<awsiotsdk::tests::integration::IntegTestRunner> test_runner =
        std::unique_ptr<awsiotsdk::tests::integration::IntegTestRunner>(new awsiotsdk::tests::integration::IntegTestRunner());

    awsiotsdk::ResponseCode rc = test_runner->Initialize();
    if (awsiotsdk::ResponseCode::SUCCESS == rc) {
        rc = test_runner->RunAllTests();
    }
#ifdef WIN32
    getchar();
#endif

    awsiotsdk::util::Logging::ShutdownAWSLogging();
    return static_cast<int>(rc);
}
