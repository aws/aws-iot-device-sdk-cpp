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
 * @file JobsAgent.hpp
 * @brief
 *
 */


#pragma once

#include "mqtt/Client.hpp"
#include "NetworkConnection.hpp"
#include "jobs/Jobs.hpp"
#include <curl/curl.h>
#ifdef UNIT_TESTS
#include "JobsMock.hpp"
#endif

#define LOG_TAG_JOBS_AGENT "[Sample - JobsAgent]"

#define DEFAULT_INSTALLED_PACKAGES_FILENAME "installedPackages.json"

namespace awsiotsdk {
    namespace samples {
        class JobsAgent {
        protected:
            std::mutex m_;
            std::condition_variable cv_done_;

            std::shared_ptr<NetworkConnection> p_network_connection_;
            std::shared_ptr<MqttClient> p_iot_client_;
#ifdef UNIT_TESTS
            std::shared_ptr<JobsMock> p_jobs_;
#else
            std::shared_ptr<Jobs> p_jobs_;
#endif

            util::String process_title_;
            util::String installed_packages_filename_;
            util::JsonDocument installed_packages_json_;
            util::Map<util::String, pid_t> package_runtimes_map_;

            static void ShowJobsError(util::String operation, ResponseCode rc);
            static util::String GetShutdownSystemCommand(bool dryRun, bool reboot);
            static util::String GetFullPath(util::String workingDirectory, util::String fileName);

            ResponseCode DisconnectCallback(util::String topic_name,
                                            std::shared_ptr<DisconnectCallbackContextData> p_app_handler_data);

            ResponseCode ReconnectCallback(util::String client_id,
                                           std::shared_ptr<ReconnectCallbackContextData> p_app_handler_data,
                                           ResponseCode reconnect_result);

            ResponseCode ResubscribeCallback(util::String client_id,
                                             std::shared_ptr<ResubscribeCallbackContextData> p_app_handler_data,
                                             ResponseCode resubscribe_result);

            ResponseCode BackupFiles(util::Map<util::String, util::String> & statusDetailsMap,
                                     util::String workingDirectory,
                                     util::JsonValue & files);
            ResponseCode RollbackFiles(util::Map<util::String, util::String> & statusDetailsMap,
                                       util::String workingDirectory,
                                       util::JsonValue & files);
            ResponseCode DownloadFiles(util::Map<util::String, util::String> & statusDetailsMap,
                                       util::String workingDirectory,
                                       util::JsonValue & file);

            ResponseCode UpdateInstalledPackage(util::JsonValue & packageJobDocument);
            bool PackageIsExecutable(util::String packageName);
            bool PackageIsRunning(util::String packageName);
            bool PackageIsAutoStart(util::String packageName);
            void StartInstalledPackages();

            ResponseCode StartPackage(util::Map<util::String, util::String> & statusDetailsMap,
                                      util::String packageName);
            ResponseCode StopPackage(util::Map<util::String, util::String> & statusDetailsMap,
                                     util::String packageName);

            ResponseCode StartPackageHandler(util::String jobId,
                                             util::String packageName);
            ResponseCode StopPackageHandler(util::String jobId,
                                            util::String packageName);
            ResponseCode RestartPackageHandler(util::String jobId,
                                               util::String packageName);
            ResponseCode InstallPackageHandler(util::String jobId,
                                               util::JsonValue &jobDocument);
            ResponseCode UninstallPackageHandler(util::String jobId,
                                                 util::String packageName);
            ResponseCode ShutdownHandler(util::String jobId,
                                         util::String step,
                                         bool reboot);
            ResponseCode SystemStatusHandler(util::String jobId);

            ResponseCode NextJobCallback(util::String topic_name,
                                         util::String payload,
                                         std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data);

            ResponseCode UpdateAcceptedCallback(util::String topic_name,
                                                util::String payload,
                                                std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data);

            ResponseCode UpdateRejectedCallback(util::String topic_name,
                                                util::String payload,
                                                std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data);

            ResponseCode Subscribe();
            ResponseCode InitializeTLS();

        public:
            ResponseCode RunAgent(const util::String &processTitle = util::String());
        };
    }
}
