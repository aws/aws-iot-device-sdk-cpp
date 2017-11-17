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
 * @file DiscoveryResponse.hpp
 * @brief Contains constant strings used as keys in the discovery response JSON
 *
 */


#pragma once

#include <rapidjson/document.h>

#include "util/memory/stl/String.hpp"
#include "util/memory/stl/Vector.hpp"
#include "util/memory/stl/Map.hpp"

#include "util/JsonParser.hpp"

namespace awsiotsdk {

    class ConnectivityInfo {
    public:
        util::String group_name_;    ///< Name of the group that the GGC belongs to
        util::String ggc_name_;      ///< Name of the GGC in the group
        util::String id_;            ///< ID of the connectivity info
        util::String host_address_;  ///< Host address of the GGC
        util::String metadata_;      ///< User defined metadata string
        uint16_t port_;              ///< port of the GGC to connect to

        // Rule of 5 stuff
        // Disable copying/moving because subscription handler callbacks will not carry over automatically
        // Default constructor
        ConnectivityInfo() = default;
        // Default Copy constructor
        ConnectivityInfo(const ConnectivityInfo &) = default;
        // Default Move constructor
        ConnectivityInfo(ConnectivityInfo &&) = default;
        // Default Copy assignment operator
        ConnectivityInfo &operator=(const ConnectivityInfo &) & = default;
        // Default Move assignment operator
        ConnectivityInfo &operator=(ConnectivityInfo &&) & = default;
        // Default destructor
        ~ConnectivityInfo() = default;

        /**
         * @brief Constructor
         *
         * @param group_name - name of the group that the GGC belongs to
         * @param ggc_name - name of the GGC in the above thing group
         * @param host_address - host address of the GGC
         * @param port - port number of the GGC
         * @param metadata - user defined metadata string
         */
        ConnectivityInfo(util::String group_name, util::String ggc_name, util::String id, util::String host_address,
                         uint16_t port, util::String metadata) {
            group_name_ = group_name;
            ggc_name_ = ggc_name;
            id_ = id;
            host_address_ = host_address;
            port_ = port;
            metadata_ = metadata;
        }
    };

    class DiscoveryResponse {
    protected:
        util::JsonDocument response_document_;  ///< Json document that contains the complete Discovery Response

    public:

        // Constant keys used for parsing the discovery response Json
        static const util::String GROUP_ARRAY_KEY;                       ///< Key for array of groups
        static const util::String GROUP_ID_KEY;                          ///< Key for group ID
        static const util::String GGC_ARRAY_KEY;                         ///< Key for array of GGCs in the group
        static const util::String GGC_THING_ARN_KEY;                     ///< Key for thing ARN of the GGC
        static const util::String ROOT_CA_KEY;                           ///< Key for the root CAs of the group
        static const util::String CONNECTIVITY_INFO_ARRAY_KEY;           ///< Key for the array of connectivity information
        static const util::String ID_KEY;                                ///< Key for the ID in the connectivity information
        static const util::String HOST_ADDRESS_KEY;                      ///< Key for host address in the connectivity information
        static const util::String PORT_KEY;                              ///< Key for port in the connectivity information
        static const util::String METADATA_KEY;                          ///< Key for metadata for the connectivity information

        static const util::String DEFAULT_DISCOVERY_RESPONSE_FILE_NAME;  ///< Default file into which the complete discovery response is stored

        // Rule of 5 stuff
        // Disable copying/moving because subscription handler callbacks will not carry over automatically
        // Default constructor
        DiscoveryResponse() = default;
        // Delete Copy constructor
        DiscoveryResponse(const DiscoveryResponse &) = delete;
        // Delete Move constructor
        DiscoveryResponse(DiscoveryResponse &&) = delete;
        // Delete Copy assignment operator
        DiscoveryResponse &operator=(const DiscoveryResponse &) & = delete;
        // Delete Move assignment operator
        DiscoveryResponse &operator=(DiscoveryResponse &&) & = delete;
        // Custom destructor
        virtual ~DiscoveryResponse();

        /**
         * @brief Constructor
         *
         * @param response_document - Json document containing full Discovery Response
         */
        DiscoveryResponse(util::JsonDocument response_document);

        /**
         * @brief Return the full Discovery Response Json
         * @return JsonDocument
         */
        util::JsonDocument GetResponseDocument();

        /**
         * @brief Set the Discovery Response Json Document in the Discovery Response Object
         *
         * @param response_document
         */
        void SetResponseDocument(util::JsonDocument response_document);

        /**
         * @brief Get the parsed discovery response
         *
         * Get the parse vector of all the connectivity information present in the Discovery Response Json along
         * with a map of all the root CAs that correspond the the groups present in the Discovery Response. Returns
         * SUCCESS if parsed successfully or DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR if the Json
         * structure cannot be parsed
         *
         * @param connectivity_info_list - vector in which the connectivity information is stored
         * @param root_ca_map - mapping between groups and filenames of the root CAs
         * @return ResponseCode
         */
        ResponseCode GetParsedResponse(util::Vector <ConnectivityInfo> &connectivity_info_list,
                                       util::Map <util::String, util::Vector<util::String>> &root_ca_map);

        /**
         * @brief Write the complete Discovery Response Json out to a file
       *
         * Function to write out the whole Discovery Response Json out to a file that can be consumed by other applications
         * directly. Returns SUCCESS if it was able to write to the file correctly. Otherwise throws FILE_OPEN_ERROR if the file
         * cannot be opened to write to it or FILE_NAME_INVALID if the file name passed in is invalid.
         *
         * @param output_file_absolute_path - absolute file path to which the Json will be written out to
         * @return ResponseCode
         */
        ResponseCode WriteToPath(util::String output_file_absolute_path);
    };
}


