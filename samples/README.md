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

### Jobs Agent Sample
This sample is a full featured example of a Jobs Agent that uses Jobs API operations to perform a variety of management tasks such as installing additional files/programs, rebooting a device, and collecting device status information. It can be run on a device as-is or it can be modified to suit specific use cases. Example job documents are provided below. For more information see the AWS IoT connected device management documentation [here](https://aws.amazon.com/iot-device-management/).

 * Code for this sample is located [here](./JobsAgent)
 * Target for this sample is `jobs-agent`

#### Using the jobs-agent
##### systemStatus operation
The jobs-agent will respond to the AWS IoT jobs management platform with system status information when it receives a job execution notification with a job document that looks like this:
```
 {
  "operation": "systemStatus"
 }
```
##### reboot operation
When the jobs-agent receives a reboot job document it will attempt to reboot the device it is running on while sending updates on its progress to the AWS IoT jobs management platform. After the reboot the job execution status will be marked as IN_PROGRESS until the jobs-agent is also restarted at which point the status will be updated to SUCCESS. To avoid manual steps during reboot it is suggested that device be configured to automatically start the jobs-agent at device startup time. Job document format:
```
 {
  "operation": "reboot"
 }
```
##### shutdown operation
When the jobs-agent receives a shutdown job document it will attempt to shutdown the device.
```
 {
  "operation": "shutdown"
 }
```
##### install operation
When the jobs-agent receives an install job document it will attempt to install the files specified in the job document. An install job document should follow this general format.
```
 {
  "operation": "install",
  "packageName": "uniquePackageName",
  "workingDirectory": ".",
  "launchCommand": "program-name program-arguments",
  "autoStart": "true",
  "files": [
    {
      "fileName": "program-name",
      "fileVersion": "1.0.2.10",
      "fileSource": {
        "url": "https://some-bucket.s3.amazonaws.com/program-name"
      },
      "checksum": {
        "inline": {
          "value": "9569257356cfc5c7b2b849e5f58b5d287f183e08627743498d9bd52801a2fbe4"
        },
        "hashAlgorithm": "SHA256"
      }
    },
    {
      "fileName": "config.json",
      "fileSource": {
        "url": "https://some-bucket.s3.amazonaws.com/config.json"
      }
    }
  ]
}
```
* `packageName`: Each install operation must have a unique package name. If the packageName matches a previous install operation then the new install operation overwrites the previous one.
* `workingDirectory`: Optional property for working directory
* `launchCommand`: Optional property for launching an application/package. If omitted copy files only.
* `autoStart`: If set to true then agent will execute launch command when agent starts up.
* `files`: Specifies files to be installed
  * `fileName`: Name of file as written to file system
  * `fileSource.url`: Location of file to be downloaded from
  * `checksum`: Optional file checksum (currently ignored)
    * `inline.value`: Checksum value
    * `hashAlgorithm`: Checksum hash algorithm used
##### start operation
When the jobs-agent receives a start job document it will attempt to startup the specified package.
```
 {
  "operation": "start",
  "packageName": "somePackageName"
 }
```
##### stop operation
When the jobs-agent receives a stop job document it will attempt to stop the specified package.
```
 {
  "operation": "stop",
  "packageName": "somePackageName"
 }
```
##### restart operation
When the jobs-agent receives a restart job document it will attempt to restart the specified package.
```
 {
  "operation": "restart",
  "packageName": "somePackageName"
 }
```

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
