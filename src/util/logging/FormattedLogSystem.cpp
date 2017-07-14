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
 * @file FormattedLogSystem.cpp
 * @brief
 *
 */

#include "util/logging/FormattedLogSystem.hpp"

#include <chrono>
#include <fstream>
#include <cstdarg>
#include <ctime>
#include <stdio.h>
#include <thread>

using namespace awsiotsdk;
using namespace awsiotsdk::util::Logging;

static util::String CreateLogPrefixLine(LogLevel logLevel, const char *tag) {
    util::StringStream ss;

    ss << "[" << GetLogLevelName(logLevel) << "] ";

    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    std::time_t time_now = std::chrono::duration_cast<std::chrono::seconds>(now_ms).count();

    char *time = std::ctime(&time_now);
    if (nullptr != time) {
        ss << time << ":" << now_ms.count() % 1000 << " ";
    }
    ss << tag << " [" << std::this_thread::get_id() << "] ";

    return ss.str();
}

FormattedLogSystem::FormattedLogSystem(LogLevel logLevel) :
    m_logLevel(logLevel) {
}

void FormattedLogSystem::Log(LogLevel logLevel, const char *tag, const char *formatStr, ...) {
    util::StringStream ss;
    ss << CreateLogPrefixLine(logLevel, tag);

    std::va_list args;
    va_start(args, formatStr);

    va_list tmp_args; //unfortunately you cannot consume a va_list twice
    va_copy(tmp_args, args); //so we have to copy it
#ifdef WIN32
    const int requiredLength = _vscprintf(formatStr, tmp_args) + 1;
#else
    const int requiredLength = vsnprintf(nullptr, 0, formatStr, tmp_args) + 1;
#endif
    va_end(tmp_args);

    std::unique_ptr<char[]> outputBuff_uptr = std::unique_ptr<char[]>(new char[requiredLength]);
    char *outputBuff = outputBuff_uptr.get();
#ifdef WIN32
    vsnprintf_s(outputBuff, requiredLength, _TRUNCATE, formatStr, args);
#else
    vsnprintf(outputBuff, static_cast<size_t>(requiredLength), formatStr, args);
#endif // WIN32

    ss << outputBuff << std::endl;

    ProcessFormattedStatement(ss.str());

    va_end(args);
}

void FormattedLogSystem::LogStream(LogLevel logLevel, const char *tag, const util::OStringStream &message_stream) {
    util::StringStream ss;
    ss << CreateLogPrefixLine(logLevel, tag) << message_stream.rdbuf()->str() << std::endl;

    ProcessFormattedStatement(ss.str());
}
