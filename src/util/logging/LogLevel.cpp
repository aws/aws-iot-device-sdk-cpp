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
 * @file LogLevel.cpp
 * @brief
 *
 */

#include "util/logging/LogLevel.hpp"

#include "util/memory/stl/String.hpp"
#include <cassert>

using namespace awsiotsdk::util::Logging;

namespace awsiotsdk {
    namespace util {
        namespace Logging {
            util::String GetLogLevelName(LogLevel logLevel) {
                switch (logLevel) {
                    case LogLevel::Off:
                        return "OFF";
                    case LogLevel::Fatal:
                        return "FATAL";
                    case LogLevel::Error:
                        return "ERROR";
                    case LogLevel::Warn:
                        return "WARN";
                    case LogLevel::Info:
                        return "INFO";
                    case LogLevel::Debug:
                        return "DEBUG";
                    case LogLevel::Trace:
                        return "TRACE";
                }
                // Can never happen but required for compilation
                return "UNKNOWN LOG LEVEL";
            }
        } // namespace Logging
    } // namespace util
} // namespace awsiotsdk
