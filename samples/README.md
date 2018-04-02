## Samples
The SDK comes with various samples that demonstrate how various SDK features can be used. A list of the included samples is below. For all of the samples listed, a target is generated when the build process described in the README.md file is followed. Specific targets are mentioned along with the sample description. The "Basic Subscribe Publish" and "Shadow Delta" sample can also be used with any of the provided reference wrappers by specifying the corresponding argument in the CMake call:
 * OpenSSL (Default if no argument is provided)- `cmake <path_to_sdk>` OR `cmake <path_to_sdk> -DNETWORK_LIBRARY=OpenSSL`
 * MbedTLS - `cmake <path_to_sdk> -DNETWORK_LIBRARY=MbedTLS`
 * WebSocket - `cmake <path_to_sdk> -DNETWORK_LIBRARY=WebSocket`

Place the IoT certs for your thing in the `BASE_SDK_DIRECTORY/certs/` folder and modify the SampleConfig.json in the `BASE_SDK_DIRECTORY/common/` with your thing-specific values.

### Basic Subscribe Publish
This is a basic sample that demonstrates how MQTT Subscribe and Publish operations can be performed using the default MQTT Client.

 * Code for this sample is located [here](./PubSub)
 * Target for this sample is `pub-sub-sample`

### Shadow Delta
This sample demonstrates how various Shadow operations can be performed.

 * Code for this sample is located [here](./ShadowDelta)
 * Target for this sample is `shadow-delta-sample`

Note: The shadow client token is set as the thing name by default in the sample. The shadow client token is limited to 64 bytes and will return an error if a token longer than 64 bytes is used (`"code":400,"message":"invalid client token"`, although receiving a 400 does not necessarily mean that it is due to the length of the client token). Modify the code [here](../ShadowDelta/ShadowDelta.cpp#L184) if your thing name is longer than 64 bytes to prevent this error.

### Jobs Sample
This sample demonstrates how various Jobs API operations can be performed including subscribing to Jobs notifications and publishing Job execution updates.

 * Code for this sample is located [here](./Jobs)
 * Target for this sample is `jobs-sample`

### Discovery Sample
This sample demonstrates how the discovery operation can be performed to get the connectivity information to connect to a Greengrass Core (GGC).
The configuration for this example is slightly different as the Discovery operation is a HTTP call, and uses port 8443, instead of port 8883 which is used for MQTT operations. The endpoint is the same IoT host endpoint used to connect the IoT thing to the cloud.

More information about the Discovery process is located [here](../GreengrassDiscovery.md)

The sample uses the DiscoveryResponse parser class to convert the Discovery Response JSON into a vector of connectivity information. It then iterates through the vector and tries to connect to one of the GGCs using the port, endpoint and root CA for that group. On successfully connecting to a core, it proceeds to subscribe and publish messages to the GGC.

To ensure that the messages get sent back to the device, the GGC has to have this device authorized and the specific route for this device programmed into the GGC during deployment.

 * Code for this sample is located [here](./Discovery)
 * Target for this sample is `discovery-sample`

### StorySwitch
This sample demonstrates how the SDK can be used to create a device which uses Greengrass.  This sample should be used in conjunction with the StoryRobotArm sample and the Greengrass storyline.  In this sample the switch is a device in a simple storyline.  The storyline involves a switch (this device) controlling a robot arm through a Greengrass Core (GGC). The switch device will use the Discovery process to find the connectivity information of the GGC associated with its group and use the information to connect to the GGC.  Once connected to the GGC the switch will accept input from the user:  "1" (on), "0" (off) or "q" (quit).  Pressing "1" will publish a desired state of "on" to the robot arm shadows, thereby telling the robot arm to turn on.  Pressing "0" will publish a desired state of "off" to the robot arm shadows, thereby telling the robot arm to turn off.  Pressing "q" will quit the program.  Please see the documentation for Greengrass service in order to experience the full storyline.

More information about the Discovery process is located [here](../GreengrassDiscovery.md)

 * Code for this sample is located [here](./StorySwitch)
 * Target for this sample is `switch-sample`


### StoryRobotArm
This sample demonstrates how the SDK can be used to create a device which uses Greengrass.  This sample should be used in conjunction with the StorySwitch sample and the Greengrass storyline.  In this sample the robot arm is a device in a simple storyline.  The storyline involves a switch controlling a robot arm (this device) through a Greengrass Core (GGC).  The robot arm device will use the Discovery process to find the connectivity information of the GGC associated with its group and use the information to connect to the GGC.  Once connected the robot arm will wait for changes on its shadows to know wether to turn "on" or "off".  If the shadows reports a delta, indicating that there is a difference between reported and desired states, the robot arm will check the delta and make the appropriate action to turn "on" or "off".  Please see the documentation for Greengrass service in order the experience the full storyline.

More information about the Discovery process is located [here](../GreengrassDiscovery.md)

 * Code for this sample is located [here](./StoryRobotArm)
 * Target for this sample is `robot-arm-sample`
 
 
For further information about the provided MQTT and Shadow Classes, please refer to the [Development Guide](../DevGuide.md)
