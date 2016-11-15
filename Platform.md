## Overview
This file contains instructions for installing dependencies on different platforms. If you have all the prerequisites specified in the README file, you could simply skip following instructions. 


### Common Issues
  * Known Issues
    * If you run into error like `"Unknown extension ".c" for file /XXX/CheckIncludeFiles.c  try_compile() works only for enabled languages. Currently these are: CXX`,  you need to change `project(aws-iot-sdk-cpp CXX)` to `project(aws-iot-sdk-cpp C CXX)` and add `ENABLE_LANGUAGE(C)` below it. 


### Raspberry Pi

  * Make sure to have CMake installed [CMake Installation](https://cmake.org/install/)
  * There is a reference for installing gcc-6 and g++-6 on raspberry pi. It also works for gcc-5/g++5 by replacing version number to 5. [GCC-6 Installation](https://solarianprogrammer.com/2016/06/24/raspberry-pi-raspbian-install-gcc-compile-cpp-14-and-cpp-17-programs/)
  * You could create symlinks to choose which version of gcc/g++ to use [update-alternatives](https://linux.die.net/man/8/update-alternatives). Assuming both gcc-5 and gcc-6 installed, the following commands will maintain symbolic links of both versions. In this case, gcc-5/g++-5 has higher priority (20) than gcc-6/g++-6 (10), which means the alternatives will point to gcc-5/g++-5 in automatic mode. You could change the priority based on your needs.
  `sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 20`
  `sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-6 10`
  `sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 20`
  `sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-6 10`
 
  `sudo update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 30`
  `sudo update-alternatives --set cc /usr/bin/gcc`
  
  `sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 30`
 ` sudo update-alternatives --set c++ /usr/bin/g++`
    
 `sudo update-alternatives --config gcc`
 `sudo update-alternatives --config g++`
  * Install Openssl 1.0.2 [OpenSSL Installation](http://www.linuxfromscratch.org/blfs/view/svn/postlfs/openssl.html)
  * Build libssl-dev 1.0.2 or above.
    * Download libssl-dev_1.0.2 [libssl-dev_1.0.2](http://mirrordirector.raspbian.org/raspbian/pool/main/o/openssl/libssl-dev_1.0.2j-1_armhf.deb)
    * run `sudo dpkg -i libssl-dev_1.0.2j-1_armhf.deb`

### Mac OS

  * With Mac OS, the default version of OpenSSL is not 1.0.2 and cannot be used with the SDK. Instead, version 1.0.2 needs to be installed. After that follow the below steps.
  * Open network/CMakelists.txt.in
  * Comment out the below line in default OpenSSL section by adding # in front of it
  
    `#find_package(OpenSSL REQUIRED)`
  
  * Add following two lines below the line commented out in default OpenSSL section
  
    `set(OPENSSL_LIBRARIES "YOUR_OPENSSL_PATH/lib/libssl.a;YOUR_OPENSSL_PATH/lib/libcrypto.a")`
    `set(OPENSSL_INCLUDE_DIR "YOUR_OPENSSL_PATH/include")`    
    
### Windows

  * Both the Websocket and OpenSSL builds work on windows. OpenSSL 1.0.2 needs to be installed for them to work properly.
  * Download and install OpenSSL from [this link](https://slproweb.com/products/Win32OpenSSL.html)
  * Download and install CMake for Windows from [this link](https://cmake.org/download/)
  * Download and install git from [this link](https://git-scm.com/downloads). Please be sure to install git bash as well.
  * Open the Git Bash terminal and navigate to the folder where you want the SDK to be downloaded.
  * Clone the repository in this folder. Once finished, change directory to the downloaded repo.
  * Create a `build` folder. Change directory to the build folder.
  * Run the following command:
  
  	`<absolute path to cmake install directory>/bin/cmake -G "Visual Studio 14 2015 Win64" ../.`
  	
  * The above command will generate a Visual studio solution file called `aws-iot-sdk-cpp`. Open this file in VS2015.
  * The list of available targets will show up in the solution explorer.
  * Before running available targets, be sure to change the working directory of the project to Output directory.
  * There are known issues with the Windows Build currently. Please check the [Known Issues](https://github.com/aws/aws-iot-device-sdk-cpp/blob/master/KnownIssues.md) file for more information.

