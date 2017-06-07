## AWS Greengrass Discovery Support
## Overview
AWS Greengrass is software that lets you run local compute, messaging & data caching for connected devices in a secure way. For further information, please see AWS Greengrass documentation [here](https://aws.amazon.com/documentation/greengrass/). 

## What is the AWS Greengrass Discovery service?
The AWS Greengrass Discovery service is a new service that allows AWS IoT Devices to "Discover" which Greengrass Groups they are a part of. It also provides information about which Greengrass Cores (GGCs) exist in those groups and what is the "connectivity information" for each of the "Discovered" GGCs. More information about the discovery service can be found in the server side documentation located [here](https://aws.amazon.com/documentation/greengrass/).

## Does any existing code for AWS IoT need to change to work with Greengrass?
Any existing code that already works with AWS IoT will not need to change to work with Greengrass. Once the endpoint is changed to a properly configured Greengrass Core, the device should continue to operate normally. However, for Greengrass Discovery support, additional code will be required. Once a Greengrass group has been set up, the existing IoT Thing needs to be included in the group. It also needs to be updated with a version of the AWS IoT device SDK that support Greengrass Discovery. The only code changes that are required, are to call the new Discovery API that gets information about the Group this device is in. This API calls the discovery service and retrieves Connection Information for GGCs.

### Discovery Request
The Discovery request itself is a HTTPS GET request to the same endpoint you currently use to connect to the AWS IoT cloud.

The request is of the following form:
```
GET /greengrass/discover/thing/<thing name>
```

### Discovery Response format and description
The discovery service sends out a response in the form of a json payload. The contents of the payload are determined by the groups this device is in. The json payload has the following basic structure:

```
{
  "GGGroups": [
    {
      "GGGroupId": "<Your GG Group ID>",
      "Cores": [
        {
          "thingArn": "<Thing ARN for the GGC>",
          "Connectivity": [
            {
              "Id": "<id>",
              "HostAddress": "<endpoint>",
              "PortNumber": <port>,
              "Metadata": "<Some Description>"
            }
          ]
        }
      ],
      "CAs": [
        "-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n"
      ]
    }
  ]
}
```

The response consists of:
- the groups that this device belongs to (GGGroups)
  - the ID of the group (GGGroupId) 
  - the GGCs present in each of the groups (Cores)
    - the Amazon Resource Number (ARN) of each core (thingArn)
    - connectivity information for the endpoints available for each core (Connectivity)
      - Id - the id of the connectivity info
      - HostAddress - the host address of the GGC
      - PortNumber - the port number to which it can connect to
      - Metadata - this is a generic string for customer use
  - The list of CA certs belonging to each group (CAs)

Additional samples are provided in the Appendix section.

### Does the SDK provide APIs for parsing this response?
Yes it does. In addition, we also provide an API for extracting the raw Json response.

### How is this implemented in the C++ Device SDK?
The C++ Device SDK is based around the concept of Actions. A new Discover Action has been added to support the new Discovery service and can be called like any other action. The client has to be an instance of GreengrassMqttClient. The parameters passed to it are the timeout value for the action, the thing name of the device for which you want to perform discovery and the json document which receives the discovery response json. To perform discovery, the network instance being used has to be setup using the Greengrass discovery port setting from the config file.

```
rc = p_iot_client_->Discover(std::chrono::milliseconds(ConfigCommon::discover_action_timeout_), std::move(p_thing_name), discovery_response);
```

The discovery_response variable will be of Type DiscoveryResponse. This is a new class that provides the basic functionality needed to retrieve both the raw json file as well as a parsed response. The ResponseCodes indicate the status of the request.

#### Discovery Response class
The DiscoveryResponse class is used as the return type for Discovery Action response data. It contains the raw json that was received from the Discovery service as a JsonDocument instance. It also provides APIs to either retrieve the raw json document directly, or to retrieve a flattened version of the response that allows easier ordering of the data. The definition for this class is located [here](./include/discovery/DiscoveryResponse.hpp). The main APIs for this class are:
```
util::JsonDocument DiscoveryResponse::GetResponseDocument();
util::JsonDocument DiscoveryResponse::SetResponseDocument(util::JsonDocument response_document);
ResponseCode GetParsedResponse(util::Vector<ConnectivityInfo> &connectivity_info_list, util::Map<util::String, util::Vector<util::String>> &root_ca_map);
ResponseCode WriteToPath(util::String output_file_absolute_path);
```
#### GetParsedResponse API
The GetParsedResponse API returns a Vector containing a flattened list of Group->GGC->endpoint data in form of ConnectivityInfo instances. The vector makes it easy to use custom sorting algorithms and generate a priority order of GGCs to connect to. The API also returns a Group level root CA mapping. After sorting the response, the group name field can be used to retrieve the corresponding Root CA and make a successful connection.
##### ConnectivityInfo class
The ConnectivityInfo class represents an individual flattened row in the parsed output. The definition for this class is located [here](./include/discovery/DiscoveryResponse.hpp#L35). It contains the below parameters:
```
util::String group_name_;
util::String ggc_name_;
util::String id_
util::String host_address_;
util::String metadata_;
uint16_t port_;
```

#### How should the endpoints be prioritized?
Depending on how the groups have been designed, there are several options for prioritizing which GGC to connect to from each device. The simplest option is to sort alphabetically based on the various string fields in the ConnectivityInfo class. A better approach may be to use the ID field or metadata field for sorting. The main purpose of the metadata field is for storing any relevant metadata that is required for a successful connection to this host address. However, the field can also store additional information such as a priority level. This opens the door for complicated sorts based on the contents of the various fields.

#### How to use the Connectivity Information?
One of the main features of the C++ SDK is that the network layer is completely isolated from the MQTT SDK itself. Because of that, the initialization and setting of endpoint configuration for the Network instance, is something that we expect customers to take care of in their application code. You can read more about this in the SDK Readme located [here](./README.md). Because of this, the next step is simply to reconfigure the Network layer instance with the new endpoint, port and root CA. In the reference network layer for OpenSSL that we provide with the SDK, we have now added two new APIs that allow you to do this.
```
void SetRootCAPath(util::String root_ca_location);
void SetEndpointAndPort(util::String endpoint, uint16_t endpoint_port);
```

This is of course, specific to the provided reference implementation of the OpenSSL wrapper. For your custom implementations, you're free to update this information as you see fit. The only important point to remember here, the SDK will NOT be able to take care of this.

Once a connection has been established, the client can proceed as if it were trying to connect to AWS IoT cloud. GGCs, at the device SDK level, are indistinguishable from AWS IoT and the existing code should continue to operate as it did with the cloud.

#### What to do if the Discovery fails?
If Discovery fails, the Discover Action does provide you with several detailed response codes that can allow you to take appropriate action.

#### Response codes
The following are the response codes returned by the discovery action:
- DISCOVER_ACTION_NO_INFORMATION_PRESENT (401) - returned when there is no connectivity information present for the given thing name
- DISCOVER_ACTION_SUCCESS (400) - returned when discovery action is a success
- DISCOVER_ACTION_REQUEST_FAILED_ERROR (-1100) - the discovery request failed for an unknown reason
- DISCOVER_ACTION_REQUEST_TIMED_OUT_ERROR (-1101) - the discovery request timed out and did not reach the gateway
- DISCOVER_ACTION_UNAUTHORIZED (-1102) - this device does not have authorization to query the server
- DISCOVER_ACTION_SERVER_ERROR (-1103) - request failed due to service issues
- DISCOVER_ACTION_REQUEST_OVERLOAD (-1104) - the Discovery service is overloaded, please try again in some time

#### What to do if the Connect request to all discovered GGC endpoints fails?
The one caveat with the design of the Discovery service is that the data is eventually consistent. There is a small time delay between the connectivity information for the GGC being updated in the service, and new certs being generated for the GGC. This can unfortunately cause a small period where the discovery request returns valid data but the GGC is still updating certs. In cases like this, discovery can be performed again and the whole process can be repeated until the device successfully connects to a GGC.

However, there is only a finite amount of time before the service is fully consistent. Depending on the design of the network, it may be required to have a fallback option where the device continues to function with AWS IoT cloud and attempts discovery at fixed time intervals to check if any new information is available. This is however that is very specific to the design and goals of your network.

## Samples
We provide a discovery sample that demonstrates how the Discovery process can be implemented. Further details can be found [here](./samples/README.md).

## Appendices

### What to do if the Discovery request does not return any data?
You need to verify whether your GGC has any connectivity information set up for it or not. You can do this by going to the Greengrass console and checking the Connectivity Info tab when looking at GGC description. For additional information about how to set up the Group properly, please check the service side documentation.

### Discovery Response constants

The discovery response class provides the below constants that represent the keys in the discovery json response.
```
GROUP_ARRAY_KEY = "GGGroups";
GROUP_ID_KEY = "GGGroupId";
GGC_ARRAY_KEY = "Cores";
GGC_THING_ARN_KEY = "thingArn";
ROOT_CA_KEY = "CAs";
CONNECTIVITY_INFO_ARRAY_KEY = "Connectivity";
ID_KEY = "Id"
HOST_ADDRESS_KEY = "HostAddress";
PORT_KEY = "PortNumber";
METADATA_KEY = "Metadata";
DEFAULT_DISCOVERY_RESPONSE_FILE_NAME = "discovery_response.json";
```

### Sample Responses
#### One group, One core with one endpoint, One Root CA
```
{
  "GGGroups": [
    {
      "GGGroupId": "<Your GG Group ID>",
      "Cores": [
        {
          "thingArn": "<Thing ARN for the GGC>",
          "Connectivity": [
            {
              "Id": "<some id>",
              "HostAddress": "<some endpoint>",
              "PortNumber": <some port>,
              "Metadata": "<Some Description>"
            }
          ]
        }
      ],
      "CAs": [
        "-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n"
      ]
    }
  ]
}
```

#### Multiple groups, multiple cores each having multiple endpoints, multiple Root CAs per group
```
{
  "GGGroups": [
    {
      "GGGroupId": "<GG Group 1 ID>",
      "Cores": [
        {
          "thingArn": "<Thing ARN for the GGC 1>",
          "Connectivity": [
            {
              "Id": "<id 1>",
              "HostAddress": "<endpoint 1>",
              "PortNumber": <some port>,
              "Metadata": "<Some Description>"
            },
            {
              "Id": "<id 2>",
              "HostAddress": "<endpoint 2>",
              "PortNumber": <some port>,
              "Metadata": "<Some Description>"
            }
          ]
        },
        {
          "thingArn": "<Thing ARN for the GGC 1>",
          "Connectivity": [
            {
              "Id": "<id 3>",
              "HostAddress": "<endpoint 3>",
              "PortNumber": <some port>,
              "Metadata": "<Some Description>"
            },
            {
              "Id": "<id 4>",
              "HostAddress": "<endpoint 4>",
              "PortNumber": <some port>,
              "Metadata": "<Some Description>"
            }
          ]
        }
      ],
      "CAs": [
        "-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n",
        "-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n",
        "-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n"
      ]
    },
    {
      "GGGroupId": "<GG Group 2 ID>",
      "Cores": [
        {
          "thingArn": "<Thing ARN for the GGC 3>",
          "Connectivity": [
            {
              "Id": "<id 5>",
              "HostAddress": "<endpoint 5>",
              "PortNumber": <some port>,
              "Metadata": "<Some Description>"
            },
            {
              "Id": "<id 6>",
              "HostAddress": "<endpoint 6>",
              "PortNumber": <some port>,
              "Metadata": "<Some Description>"
            }
          ]
        }
      ],
      "CAs": [
        "-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n",
        "-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n",
        "-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n"
      ]
    }
  ]
}
```