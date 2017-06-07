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
 * @file Logging.cpp
 * @brief
 *
 */

#include "util/logging/Logging.hpp"
#include "util/logging/LogSystemInterface.hpp"

#include <memory>

using namespace awsiotsdk::util;
using namespace awsiotsdk::util::Logging;

static std::shared_ptr<LogSystemInterface> AWSLogSystem(nullptr);
static std::shared_ptr<LogSystemInterface> OldLogger(nullptr);

namespace awsiotsdk {
    namespace util {
        namespace Logging {

            void InitializeAWSLogging(const std::shared_ptr<LogSystemInterface> &logSystem) {
                AWSLogSystem = logSystem;
            }

            void ShutdownAWSLogging(void) {
                InitializeAWSLogging(nullptr);
            }

            LogSystemInterface *GetLogSystem() {
                return AWSLogSystem.get();
            }

            void PushLogger(const std::shared_ptr<LogSystemInterface> &logSystem) {
                OldLogger = AWSLogSystem;
                AWSLogSystem = logSystem;
            }

            void PopLogger() {
                AWSLogSystem = OldLogger;
                OldLogger = nullptr;
            }

        } // namespace Logging
    } // namespace util
} // namespace awsiotsdk