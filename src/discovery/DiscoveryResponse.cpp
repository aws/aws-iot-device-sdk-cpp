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
 * @file DiscoveryResponse.cpp
 * @brief
 *
 */

#include "util/logging/Logging.hpp"
#include "util/logging/LogMacros.hpp"
#include "util/logging/ConsoleLogSystem.hpp"

#include "discovery/DiscoveryResponse.hpp"

#define LOG_TAG_DISCOVERY_RESPONSE "[Discovery Response]"

namespace awsiotsdk {
	const util::String awsiotsdk::DiscoveryResponse::GROUP_ARRAY_KEY = "GGGroups";
	const util::String awsiotsdk::DiscoveryResponse::GROUP_ID_KEY = "GGGroupId";
	const util::String awsiotsdk::DiscoveryResponse::GGC_ARRAY_KEY = "Cores";
	const util::String awsiotsdk::DiscoveryResponse::GGC_THING_ARN_KEY = "thingArn";
	const util::String awsiotsdk::DiscoveryResponse::ROOT_CA_KEY = "CAs";
	const util::String awsiotsdk::DiscoveryResponse::CONNECTIVITY_INFO_ARRAY_KEY = "Connectivity";
	const util::String awsiotsdk::DiscoveryResponse::ID_KEY = "Id";
	const util::String awsiotsdk::DiscoveryResponse::HOST_ADDRESS_KEY = "HostAddress";
	const util::String awsiotsdk::DiscoveryResponse::PORT_KEY = "PortNumber";
	const util::String awsiotsdk::DiscoveryResponse::METADATA_KEY = "Metadata";
	const util::String awsiotsdk::DiscoveryResponse::DEFAULT_DISCOVERY_RESPONSE_FILE_NAME = "discovery_response.json";

	util::JsonDocument DiscoveryResponse::GetResponseDocument() {
		util::JsonDocument response;
		response.CopyFrom(response_document_, response.GetAllocator());
		return std::move(response);
	}

	void DiscoveryResponse::SetResponseDocument(util::JsonDocument response_document) {
		response_document_ = std::move(response_document);
	}

	DiscoveryResponse::DiscoveryResponse(util::JsonDocument response_document) {
		response_document_ = std::move(response_document);
	}

	DiscoveryResponse::~DiscoveryResponse() {
	}

	ResponseCode DiscoveryResponse::WriteToPath(util::String output_file_absolute_path) {
		return util::JsonParser::WriteToFile(response_document_, output_file_absolute_path);
	}

	// The runtime for this is O(n) where n is the total number of individual Connectivity Information sets in the
	// response json over ALL groups and GGCs
	ResponseCode DiscoveryResponse::GetParsedResponse(util::Vector<ConnectivityInfo> &connectivity_info_list,
													  util::Map<util::String,
																util::Vector<util::String>> &root_ca_map) {
		util::String group_name;
		util::String core_name;
		util::String ca_string;
		util::String id;
		util::String host_name;
		util::String metadata;
		uint16_t port;
		ResponseCode parsing_response = ResponseCode::SUCCESS;

		const char *group_array_key = GROUP_ARRAY_KEY.c_str();
		const char *group_id_key = GROUP_ID_KEY.c_str();
		const char *ggc_array_key = GGC_ARRAY_KEY.c_str();
		const char *root_ca_key = ROOT_CA_KEY.c_str();
		const char *connectivity_info_array_key = CONNECTIVITY_INFO_ARRAY_KEY.c_str();
		const char *id_key = ID_KEY.c_str();
		const char *host_address_key = HOST_ADDRESS_KEY.c_str();
		const char *port_key = PORT_KEY.c_str();
		const char *metadata_key = METADATA_KEY.c_str();
		const char *ggc_thing_arn_key = GGC_THING_ARN_KEY.c_str();

		for (auto &group_itr : response_document_[group_array_key].GetArray()) {
			if (!group_itr.HasMember(group_id_key)
				|| !group_itr.HasMember(ggc_array_key)
				|| !group_itr.HasMember(root_ca_key)) {
				parsing_response = ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR;
				break;
			}

			group_name = util::String(group_itr[group_id_key].GetString());

			for (auto &core_itr : group_itr[ggc_array_key].GetArray()) {
				if (!core_itr.HasMember(ggc_thing_arn_key)
					|| !core_itr.HasMember(connectivity_info_array_key)) {
					parsing_response = ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR;
					break;
				}
				core_name = util::String(core_itr[ggc_thing_arn_key].GetString());
				for (auto &connectivity_info_itr : core_itr[connectivity_info_array_key].GetArray()) {
					if (!connectivity_info_itr.HasMember(id_key)
						|| !connectivity_info_itr.HasMember(host_address_key)
						|| !connectivity_info_itr.HasMember(port_key)) {
						parsing_response = ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR;
						break;
					}

					id = util::String(connectivity_info_itr[id_key].GetString());

					host_name = util::String(connectivity_info_itr[host_address_key].GetString());

					if (connectivity_info_itr.HasMember(metadata_key)) {
						metadata = util::String(connectivity_info_itr[metadata_key].GetString());
					} else {
						metadata = util::String("");
					}

					port = static_cast<uint16_t>(connectivity_info_itr[port_key].GetInt());

					ConnectivityInfo connectivity_info(group_name, core_name, id, host_name, port, metadata);

					connectivity_info_list.push_back(connectivity_info);
					util::Vector<util::String> ca_list;
					for (auto &ca_itr : group_itr[root_ca_key].GetArray()) {
						ca_string = util::String(ca_itr.GetString());
						ca_list.push_back(ca_string);
					}
					root_ca_map.insert(std::pair<util::String, util::Vector<util::String>>(group_name, ca_list));
				}
			}
			if (ResponseCode::SUCCESS != parsing_response) {
				break;
			}
		}
		return parsing_response;
	}
}
