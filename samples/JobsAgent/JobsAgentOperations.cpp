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
 * @file JobsAgentOperations.cpp
 *
 */

#include <chrono>
#include <cstring>
#include <sys/utsname.h>
#include <fstream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util/logging/Logging.hpp"
#include "util/logging/LogMacros.hpp"
#include "util/logging/ConsoleLogSystem.hpp"

#include "ConfigCommon.hpp"
#include "JobsAgent.hpp"

/*
jobs agent error values:

ERR_DOWNLOAD_FAILED
ERR_FILE_COPY_FAILED
ERR_UNNAMED_PACKAGE
ERR_INVALID_PACKAGE_NAME
ERR_SYSTEM_CALL_FAILED
ERR_UNEXPECTED_PACKAGE_EXIT
ERR_UNABLE_TO_START_PACKAGE
ERR_UNABLE_TO_STOP_PACKAGE
ERR_UNSUPPORTED_CHECKSUM_ALGORITHM
ERR_CHECKSUM_FAILED
ERR_UNEXPECTED
*/

namespace awsiotsdk {
    namespace samples {
        void JobsAgent::ShowJobsError(util::String operation, ResponseCode rc) {
            if (ResponseCode::SUCCESS != rc) {
                util::String fullMessage = "Error in %s operation. %s";
                AWS_LOG_ERROR(LOG_TAG_JOBS_AGENT, fullMessage.c_str(), 
                              operation.c_str(), ResponseHelper::ToString(rc).c_str());
            }            
        }

        util::String JobsAgent::GetShutdownSystemCommand(bool dryRun, bool reboot) {
            util::String result;
#ifdef WIN32
            result = "shutdown ";
            if (!dryRun) {
                result += (reboot ? "/r" : "/s");
                result += " /t:0";
            }
#else
            result = "sudo /sbin/shutdown ";
            if (dryRun || reboot) {
                result += "-";
                result += (reboot ? "r" : "");
                result += (dryRun ? "k" : "");
                result += " ";
            }
            result += "+0";
#endif
            return result;
        }

        util::String JobsAgent::GetFullPath(util::String workingDirectory, util::String fileName) {
            if (workingDirectory.size() > 0 && workingDirectory[workingDirectory.size() - 1] != '/') {
                return workingDirectory + "/" + fileName;
            } else {
                return workingDirectory + fileName;
            }
        }

        ResponseCode JobsAgent::ShutdownHandler(util::String jobId, util::String step, bool reboot) {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "ShutdownHandler");

            ResponseCode rc = ResponseCode::SUCCESS;
            util::Map<util::String, util::String> statusDetailsMap;
            statusDetailsMap["operation"] = reboot ? "reboot" : "shutdown";

            util::String systemCommand;
            int systemRC;

            if (step.empty()) {
                // User account running agent must have passwordless sudo access on /sbin/shutdown
                // Recommended online search for permissions setup instructions https://www.google.com/search?q=passwordless+sudo+access+instructions

                // Dry run to check permissions
                systemCommand = GetShutdownSystemCommand(true, reboot);
                systemRC = system(systemCommand.c_str());

                if (systemRC != 0) {
                    util::String errorMessage = "System command (" + systemCommand + ") returned error code: " + std::to_string(systemRC);
                    AWS_LOG_ERROR(LOG_TAG_JOBS_AGENT, errorMessage.c_str());
                    statusDetailsMap["errorCode"] = "ERR_SYSTEM_CALL_FAILED";
                    statusDetailsMap["errorMessage"] = "unable to execute shutdown, check passwordless sudo permissions on agent";
                    statusDetailsMap["error"] = errorMessage;
                    rc = p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap, 0, 0, true);
                } else {
                    statusDetailsMap["step"] = "initiated";
                    rc = p_jobs_->SendJobsUpdate(jobId, reboot ? Jobs::JOB_EXECUTION_IN_PROGRESS : Jobs::JOB_EXECUTION_SUCCEEDED, statusDetailsMap, 0, 0, true, true);
                }

            // Check for reboot previously in progress and mark as JOB_EXECUTION_SUCCEEDED
            } else if (reboot && step == "initiated") {
                statusDetailsMap["step"] = "completed";
                rc = p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_SUCCEEDED, statusDetailsMap);
            }

            return rc;
        }

        ResponseCode JobsAgent::SystemStatusHandler(util::String jobId) {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "SystemStatusHandler");

            util::Map<util::String, util::String> statusDetailsMap;
            statusDetailsMap["operation"] = "systemStatus";

            bool firstPackage = true;
            utsname sysInfo;
            uname(&sysInfo);

            std::stringstream installedPackages;
            std::stringstream runningPackages;

            installedPackages << '[';
            runningPackages << '[';
            for (util::JsonValue::ConstMemberIterator package_itr = installed_packages_json_.MemberBegin();
                package_itr != installed_packages_json_.MemberEnd(); ++package_itr) {
                if (package_itr->value.IsObject()) {
                    util::String packageName = package_itr->name.GetString();
                    if (firstPackage) {
                        firstPackage = false;
                    } else {
                        installedPackages << ',';
                    }
                    installedPackages << '\"';
                    installedPackages << packageName;
                    installedPackages << '\"';

                    if (PackageIsRunning(packageName)) {
                        runningPackages << '\"';
                        runningPackages << packageName;
                        runningPackages << '\"';
                    }
                }
            }
            installedPackages << ']';
            runningPackages << ']';

            statusDetailsMap["installedPackages"] = installedPackages.str();
            statusDetailsMap["runningPackages"] = runningPackages.str();
            statusDetailsMap["arch"] = sysInfo.machine;
            statusDetailsMap["cwd"] = ConfigCommon::GetCurrentPath();
            statusDetailsMap["platform"] = sysInfo.sysname;
            statusDetailsMap["title"] = process_title_;

            return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_SUCCEEDED, statusDetailsMap);
        }

        ResponseCode JobsAgent::BackupFiles(util::Map<util::String, util::String> & statusDetailsMap,
                                            util::String workingDirectory,
                                            util::JsonValue & files) {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "BackupFiles");

            statusDetailsMap["step"] = "backup files";

            for (unsigned int i = 0; i < files.Size(); i++) {
                if (files[i].HasMember("fileName") && files[i]["fileName"].IsString()) {
                    util::String fileNameWithPath = GetFullPath(workingDirectory, files[i]["fileName"].GetString());

                    std::ifstream src(fileNameWithPath, std::ios::binary);
                    if (src.good()) {
                        std::ofstream dst(fileNameWithPath + ".old", std::ios::binary);
                        if (dst.fail()) {
                            statusDetailsMap["errorCode"] = "ERR_FILE_COPY_FAILED";
                            statusDetailsMap["errorMessage"] = "unable to backup file";
                            statusDetailsMap["fileName"] = fileNameWithPath;
                            return ResponseCode::FAILURE;
                        }
                        dst << src.rdbuf();
                    }
                }
            }

            return ResponseCode::SUCCESS;
        }

        ResponseCode JobsAgent::RollbackFiles(util::Map<util::String, util::String> & statusDetailsMap,
                                              util::String workingDirectory,
                                              util::JsonValue & files) {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "RollbackFiles");

            ResponseCode rc = ResponseCode::SUCCESS;

            statusDetailsMap["step"] = "rollback files";

            for (unsigned int i = 0; i < files.Size(); i++) {
                if (files[i].HasMember("fileName") && files[i]["fileName"].IsString()) {
                    util::String fileNameWithPath = GetFullPath(workingDirectory, files[i]["fileName"].GetString());

                    std::ifstream src(fileNameWithPath + ".old", std::ios::binary);
                    if (src.good()) {
                        std::ofstream dst(fileNameWithPath, std::ios::binary);
                        if (dst.fail()) {
                            statusDetailsMap["rollbackError"] = "not all files were successfully rolled back";
                            rc = ResponseCode::FAILURE;
                        } else {
                            dst << src.rdbuf();
                        }
                    }
                }
            }

            return rc;
        }

        ResponseCode JobsAgent::DownloadFiles(util::Map<util::String, util::String> & statusDetailsMap,
                                              util::String workingDirectory,
                                              util::JsonValue & files) {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "DownloadFiles");

            statusDetailsMap["step"] = "download files";

            CURLcode res;
            CURL *curl = curl_easy_init();
            if (!curl) {
                statusDetailsMap["errorCode"] = "ERR_DOWNLOAD_FAILED";
                statusDetailsMap["errorMessage"] = "unable to initialize curl";
                return ResponseCode::FAILURE;
            }

            for (unsigned int i = 0; i < files.Size(); i++) {
                util::JsonValue & file = files[i];

                if (file.HasMember("fileName") && file["fileName"].IsString() && file.HasMember("fileSource") &&
                    file["fileSource"].HasMember("url") && file["fileSource"]["url"].IsString()) {

                    util::String fileNameWithPath = GetFullPath(workingDirectory, file["fileName"].GetString());
                    util::String fileSourceUrl = file["fileSource"]["url"].GetString();

                    FILE* file = fopen(fileNameWithPath.c_str(), "wb");
                    if (!file) {
                        statusDetailsMap["errorCode"] = "ERR_DOWNLOAD_FAILED";
                        statusDetailsMap["errorMessage"] = "unable to open file for writing";
                        RollbackFiles(statusDetailsMap, workingDirectory, files);
                        curl_easy_cleanup(curl);
                        return ResponseCode::FAILURE;
                    }

                    curl_easy_setopt(curl, CURLOPT_URL, fileSourceUrl.c_str());
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
                    res = curl_easy_perform(curl);
                    fclose(file);

                    if (res != CURLE_OK) {
                        statusDetailsMap["errorCode"] = "ERR_DOWNLOAD_FAILED";
                        statusDetailsMap["errorMessage"] = "curl error encountered";
                        statusDetailsMap["curlError"] = curl_easy_strerror(res);
                        statusDetailsMap["fileSourceUrl"] = fileSourceUrl;
                        RollbackFiles(statusDetailsMap, workingDirectory, files);
                        curl_easy_cleanup(curl);
                        return ResponseCode::FAILURE;
                    }
                }
            }

            curl_easy_cleanup(curl);
            return ResponseCode::SUCCESS;
        }

        ResponseCode JobsAgent::UpdateInstalledPackage(util::JsonValue & packageJobDocument) {
            util::String packageName = packageJobDocument["packageName"].GetString();

            installed_packages_json_.RemoveMember(packageName.c_str());
            util::JsonValue packageJobDocumentCopy;
            packageJobDocumentCopy.CopyFrom(packageJobDocument, installed_packages_json_.GetAllocator());
            installed_packages_json_.AddMember(util::JsonValue(packageName.c_str(), installed_packages_json_.GetAllocator()), packageJobDocumentCopy, installed_packages_json_.GetAllocator());

            return util::JsonParser::WriteToFile(installed_packages_json_, installed_packages_filename_.c_str());
        }

        bool JobsAgent::PackageIsExecutable(util::String packageName) {
            return (installed_packages_json_.HasMember(packageName.c_str()) && installed_packages_json_[packageName.c_str()].HasMember("launchCommand"));
        }

        bool JobsAgent::PackageIsRunning(util::String packageName) {
            return (package_runtimes_map_.count(packageName.c_str()) > 0 &&
                    package_runtimes_map_[packageName.c_str()] > 0 &&
                    kill(package_runtimes_map_[packageName.c_str()], 0) == 0);
        }

        bool JobsAgent::PackageIsAutoStart(util::String packageName) {
            return (PackageIsExecutable(packageName.c_str()) && installed_packages_json_[packageName.c_str()].HasMember("autoStart") &&
                    installed_packages_json_[packageName.c_str()]["autoStart"].IsBool() &&
                    installed_packages_json_[packageName.c_str()]["autoStart"].GetBool());
        }

        ResponseCode JobsAgent::StartPackage(util::Map<util::String, util::String> & statusDetailsMap,
                                             util::String packageName) {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "StartPackage");

            statusDetailsMap["step"] = "start package";

            if (PackageIsExecutable(packageName) && !PackageIsRunning(packageName)) {
                pid_t pid = fork();

                switch (pid) {
                    case -1: {
                        AWS_LOG_ERROR(LOG_TAG_JOBS_AGENT, "error in call to fork");
                        statusDetailsMap["errorCode"] = "ERR_UNABLE_TO_START_PACKAGE";
                        statusDetailsMap["errorMessage"] = "error in call to fork";
                        return ResponseCode::FAILURE;
                    }

                    case 0: {
                        if (installed_packages_json_[packageName.c_str()].HasMember("workingDirectory") &&
                            installed_packages_json_[packageName.c_str()]["workingDirectory"].IsString()) {
                            
                            int res = chdir(installed_packages_json_[packageName.c_str()]["workingDirectory"].GetString());
                            if (res != 0) {
                                AWS_LOG_ERROR(LOG_TAG_JOBS_AGENT, "unable to change to working directory");
                                exit(EXIT_FAILURE);
                            }
                        }

                        util::String cmd = "exec ";
                        cmd += installed_packages_json_[packageName.c_str()]["launchCommand"].GetString();
                        execl("/bin/sh", "sh", "-c", cmd.c_str(), NULL);
                        exit(1);
                        return ResponseCode::SUCCESS;
                    }

                    default: {
                        package_runtimes_map_[packageName.c_str()] = pid;
                        return ResponseCode::SUCCESS;
                    }
                }
            } else {
                statusDetailsMap["errorCode"] = "ERR_UNABLE_TO_START_PACKAGE";
                if (!PackageIsExecutable(packageName)) {
                    statusDetailsMap["errorMessage"] = "package is not executable";
                } else {
                    statusDetailsMap["errorMessage"] = "package already running";
                }
            }
            return ResponseCode::FAILURE;
        }

        ResponseCode JobsAgent::StartPackageHandler(util::String jobId,
                                                    util::String packageName) {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "StartPackageHandler");

            util::Map<util::String, util::String> statusDetailsMap;
            statusDetailsMap["operation"] = "start";

            if (StartPackage(statusDetailsMap, packageName) == ResponseCode::SUCCESS) {
                statusDetailsMap["step"] = "completed";
                return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_SUCCEEDED, statusDetailsMap);
            }

            return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
        }

        ResponseCode JobsAgent::StopPackage(util::Map<util::String, util::String> & statusDetailsMap,
                                            util::String packageName) {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "StopPackage");

            statusDetailsMap["step"] = "stop package";

            if (PackageIsRunning(packageName)) {
                if (kill(package_runtimes_map_[packageName.c_str()], SIGTERM) == 0) {
                    waitpid(package_runtimes_map_[packageName.c_str()], 0, 0);
                    return ResponseCode::SUCCESS;
                } else {
                    statusDetailsMap["errorCode"] = "ERR_UNABLE_TO_STOP_PACKAGE";
                    statusDetailsMap["errorMessage"] = "error in call to kill";
                    return ResponseCode::FAILURE;
                }
            } else {
                statusDetailsMap["errorCode"] = "ERR_UNABLE_TO_STOP_PACKAGE";
                statusDetailsMap["errorMessage"] = "package is not running";
            }

            return ResponseCode::FAILURE;
        }

        ResponseCode JobsAgent::StopPackageHandler(util::String jobId,
                                                   util::String packageName) {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "StopPackageHandler");

            util::Map<util::String, util::String> statusDetailsMap;
            statusDetailsMap["operation"] = "stop";

            if (StopPackage(statusDetailsMap, packageName) == ResponseCode::SUCCESS) {
                statusDetailsMap["step"] = "completed";
                return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_SUCCEEDED, statusDetailsMap);
            }

            return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
        }

        ResponseCode JobsAgent::RestartPackageHandler(util::String jobId,
                                                      util::String packageName) {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "RestartPackageHandler");

            util::Map<util::String, util::String> statusDetailsMap;
            statusDetailsMap["operation"] = "restart";

            if (PackageIsRunning(packageName) && StopPackage(statusDetailsMap, packageName) != ResponseCode::SUCCESS) {
                return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
            }

            if (StartPackage(statusDetailsMap, packageName) == ResponseCode::SUCCESS) {
                statusDetailsMap["step"] = "completed";
                return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_SUCCEEDED, statusDetailsMap);
            }

            return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
        }

        ResponseCode JobsAgent::InstallPackageHandler(util::String jobId,
                                                      util::JsonValue & jobDocument) {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "InstallPackageHandler");

            ResponseCode rc = ResponseCode::FAILURE;
            util::String workingDirectory = "";
            util::Map<util::String, util::String> statusDetailsMap;
            statusDetailsMap["operation"] = "install";

            if (!jobDocument.HasMember("packageName") || !jobDocument["packageName"].IsString()) {
                statusDetailsMap["errorCode"] = "ERR_UNNAMED_PACKAGE";
                statusDetailsMap["errorMessage"] = "installed packages must have packageName string property";
                return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
            }

            if (!jobDocument.HasMember("files") || !jobDocument["files"].IsArray() || jobDocument["files"].Size() == 0) {
                statusDetailsMap["errorCode"] = "ERR_FILE_COPY_FAILED";
                statusDetailsMap["errorMessage"] = "files property missing or invalid";
                return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
            }

            statusDetailsMap["packageName"] = jobDocument["packageName"].GetString();
            util::JsonValue & files = jobDocument["files"];

            if (jobDocument.HasMember("workingDirectory") && jobDocument["workingDirectory"].IsString()) {
                workingDirectory = jobDocument["workingDirectory"].GetString();
            }

            rc = p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_IN_PROGRESS, statusDetailsMap);
            ShowJobsError(statusDetailsMap["operation"], rc);

            rc = BackupFiles(statusDetailsMap, workingDirectory, files);
            if (rc == ResponseCode::SUCCESS) {
                rc = p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_IN_PROGRESS, statusDetailsMap);
                ShowJobsError(statusDetailsMap["operation"], rc);

                rc = DownloadFiles(statusDetailsMap, workingDirectory, files);
                if (rc == ResponseCode::SUCCESS) {
                    rc = UpdateInstalledPackage(jobDocument);
                    if (rc == ResponseCode::SUCCESS) {
                        if (PackageIsAutoStart(jobDocument["packageName"].GetString())) {
                            statusDetailsMap["step"] = "auto start package";
                            rc = StartPackage(statusDetailsMap, jobDocument["packageName"].GetString());
                            if (rc != ResponseCode::SUCCESS) {
                                statusDetailsMap["warning"] = "package installed but unable to start";
                                rc = ResponseCode::SUCCESS;
                            }
                        }
                    } else {
                        statusDetailsMap["errorCode"] = "ERR_FILE_COPY_FAILED";
                        statusDetailsMap["errorMessage"] = "unable to install package";
                    }
                }
            }

            if (rc == ResponseCode::SUCCESS) {
                statusDetailsMap["step"] = "completed";
                rc = p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_SUCCEEDED, statusDetailsMap);
            } else {
                rc = p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
            }
            return rc;
        }

        ResponseCode JobsAgent::UninstallPackageHandler(util::String jobId,
                                                        util::String packageName) {
            AWS_LOG_INFO(LOG_TAG_JOBS_AGENT, "UninstallPackageHandler");

            util::Map<util::String, util::String> statusDetailsMap;
            statusDetailsMap["operation"] = "uninstall";

            if (!installed_packages_json_.HasMember(packageName.c_str())) {
                statusDetailsMap["errorCode"] = "ERR_INVALID_PACKAGE_NAME";
                statusDetailsMap["errorMessage"] = "no package found with name " + packageName;
                return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
            }

            if (PackageIsRunning(packageName)) {
                if (StopPackage(statusDetailsMap, packageName) != ResponseCode::SUCCESS) {
                    return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
                }
            }

            installed_packages_json_.RemoveMember(packageName.c_str());

            if (util::JsonParser::WriteToFile(installed_packages_json_, installed_packages_filename_.c_str()) != ResponseCode::SUCCESS) {
                statusDetailsMap["errorCode"] = "ERR_FILE_COPY_FAILED";
                statusDetailsMap["errorMessage"] = "uninstall package failed";
                return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
            }

            return p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_SUCCEEDED, statusDetailsMap);
        }

        ResponseCode JobsAgent::NextJobCallback(util::String topic_name,
                                                util::String payload,
                                                std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
            ResponseCode rc;
            util::JsonDocument doc;
            util::String operation = "NextJobCallback";
            util::Map<util::String, util::String> statusDetailsMap;

            rc = util::JsonParser::InitializeFromJsonString(doc, payload);

            if (ResponseCode::SUCCESS == rc && doc.HasMember("execution")) {
                util::JsonValue & execution = doc["execution"];

                if (execution.HasMember("jobId") && execution["jobId"].IsString()) {
                    util::String jobId = execution["jobId"].GetString();

                    if (execution.HasMember("jobDocument")) {
                        util::JsonValue & jobDocument = execution["jobDocument"];

                        if (jobDocument.HasMember("operation") && jobDocument["operation"].IsString()) {
                            operation = jobDocument["operation"].GetString();
                            statusDetailsMap["operation"] = operation;

                            if (operation == "systemStatus") {
                                rc = SystemStatusHandler(jobId);
                            } else if (operation == "reboot" || operation == "shutdown") {
                                util::String step;

                                if (execution.HasMember("statusDetails") && execution["statusDetails"].HasMember("step") &&
                                    execution["statusDetails"]["step"].IsString()) {

                                    step = execution["statusDetails"]["step"].GetString();
                                }

                                rc = ShutdownHandler(jobId, step, operation == "reboot");
                            } else if (operation == "install") {
                                rc = InstallPackageHandler(jobId, jobDocument);
                            } else if (operation == "start" || operation == "stop" || operation == "restart" || operation == "uninstall") {
                                if (!jobDocument.HasMember("packageName") || !jobDocument["packageName"].IsString()) {
                                    statusDetailsMap["errorCode"] = "ERR_UNNAMED_PACKAGE";
                                    statusDetailsMap["errorMessage"] = "must specify packageName";
                                    rc = p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
                                } else if (operation == "start") {
                                    rc = StartPackageHandler(jobId, jobDocument["packageName"].GetString());
                                } else if (operation == "stop") {
                                    rc = StopPackageHandler(jobId, jobDocument["packageName"].GetString());
                                } else if (operation == "restart") {
                                    rc = RestartPackageHandler(jobId, jobDocument["packageName"].GetString());
                                } else {
                                    rc = UninstallPackageHandler(jobId, jobDocument["packageName"].GetString());
                                }
                            } else {
                                statusDetailsMap["errorCode"] = "ERR_UNEXPECTED";
                                statusDetailsMap["errorMessage"] = "unhandled operation";
                                rc = p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
                            }
                        }
                    } else {
                        statusDetailsMap["errorCode"] = "ERR_UNEXPECTED";
                        statusDetailsMap["errorMessage"] = "unable to process job document";
                        rc = p_jobs_->SendJobsUpdate(jobId, Jobs::JOB_EXECUTION_FAILED, statusDetailsMap);
                    }
                }
            }
            // Only shows error message when rc != ResponseCode::SUCCESS
            ShowJobsError(operation, rc);

            return rc;
        }

        ResponseCode JobsAgent::UpdateAcceptedCallback(util::String topic_name,
                                                       util::String payload,
                                                       std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
            ResponseCode rc;
            util::JsonDocument doc;
            util::Map<util::String, util::String> statusDetailsMap;
            util::String operation = "NextJobCallback";

            std::cout << std::endl << "************" << std::endl;
            std::cout << "Received message on topic : " << topic_name << std::endl;
            std::cout << "Payload Length : " << payload.length() << std::endl;
            std::cout << "Payload : " << payload << std::endl;
            std::cout << std::endl << "************" << std::endl;

            rc = util::JsonParser::InitializeFromJsonString(doc, payload);

            if (ResponseCode::SUCCESS == rc && doc.HasMember("executionState")) {
                util::JsonValue & executionState = doc["executionState"];

                if (doc.HasMember("jobDocument") && executionState.HasMember("statusDetails")) {
                    util::JsonValue & jobDocument = doc["jobDocument"];
                    util::JsonValue & statusDetails = executionState["statusDetails"];

                    if (jobDocument.HasMember("operation") && jobDocument["operation"].IsString()) {
                        util::String operation = jobDocument["operation"].GetString();

                        if (operation == "reboot" || operation == "shutdown") {
                            if (statusDetails.HasMember("step") && statusDetails["step"].IsString() &&
                                util::String(statusDetails["step"].GetString()) == "initiated") {

                                util::String systemCommand;
                                util::String outputMessage = (operation == "reboot" ? "rebooting..." : "shutting down...");

                                // User account running agent must have passwordless sudo access on /sbin/shutdown
                                // Recommended online search for permissions setup instructions https://www.google.com/search?q=passwordless+sudo+access+instructions
                                systemCommand = GetShutdownSystemCommand(false, operation == "reboot");
                                int rc = system(systemCommand.c_str());
                                IOT_UNUSED(rc);
                                std::cout << std::endl << outputMessage << std::endl;
                            }
                        }
                    } 
                }
            }
            return rc;
        }

        ResponseCode JobsAgent::UpdateRejectedCallback(util::String topic_name,
                                                       util::String payload,
                                                       std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data) {
            std::cout << std::endl << "************" << std::endl;
            std::cout << "Received message on topic : " << topic_name << std::endl;
            std::cout << "Payload Length : " << payload.length() << std::endl;
            std::cout << "Payload : " << payload << std::endl;
            std::cout << std::endl << "************" << std::endl;

            /* Do error handling here for when the update was rejected */

            return ResponseCode::SUCCESS;
        }
    }
}
