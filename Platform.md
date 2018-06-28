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
  * Install Openssl 1.0.2. Linux From Scratch has good guides on installing versions of OpenSSL from source. 
  * Build libssl-dev 1.0.2 or above.
    * Download libssl-dev_1.0.2 [libssl-dev_1.0.2](http://mirrors.manchester.m247.com/raspbian/pool/main/o/openssl/libssl-dev_1.0.2j-1_armhf.deb)
    * run `sudo dpkg -i libssl-dev_1.0.2j-1_armhf.deb`

### Mac OS
  * Install Homebrew if it's not installed `$ /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"`
  * Install CMake 
    * You can build from source [CMake Installation](https://cmake.org/install/) or
    * Install directly using `brew install cmake`
    * If you use the .dmg installer, the following instructions are used to add the CMake path into $PATH after installation:
      * `sudo vim /etc/paths`
      * Add the following line to the paths already present `/Applications/CMake.app/Contents/bin`
      * save and quit
      * In a new terminal window, run `echo $PATH`. The path for CMake should be displayed, along with the other paths. 
  * With Mac OS, the default version of OpenSSL is not 1.0.2 and cannot be used with the SDK. Instead, version 1.0.2 needs to be installed.
    To install OpenSSL 1.0.2:
    * Update brew `brew update && brew upgrade`
    * Run `brew info openssl` to list the versions of openssl available
      Example output:
      `openssl: stable 1.0.2l (bottled) [keg-only]
       SSL/TLS cryptography library
       https://openssl.org/
       /usr/local/Cellar/openssl/1.0.2k (1,696 files, 12MB)
         Poured from bottle on 2017-02-03 at 14:20:40
       /usr/local/Cellar/openssl/1.0.2l (1,709 files, 12.1MB)
         Poured from bottle on 2017-05-31 at 15:32:30`
      Use the latest stable 1.0.2 version available, which is 1.0.2l here   
    * Run `brew switch openssl <latest stable 1.0.2 version>`. If the version is already installed, the path for this version of OpenSSL will be displayed.
      Example output:
      `Cleaning /usr/local/Cellar/openssl/1.0.2l
       Opt link created for /usr/local/Cellar/openssl/1.0.2l`
      Use the path (`/usr/local/Cellar/openssl/1.0.2l` in the example above) in your <BASE_SDK_DIRECTORY>/network/CMakeLists.txt 
    * If not installed, run `brew install openssl --force`  after the switch operation. The path will be displayed at the end of the installation. 
  
  After that follow the below steps.
  
  * Open <BASE_SDK_DIRECTORY>/network/CMakelists.txt.in
  * Comment out the below line in default OpenSSL section by adding # in front of it
  
    `#find_package(OpenSSL REQUIRED)`
  
  * Add following two lines below the line commented out in default OpenSSL section. Replace "YOUR_OPENSSL_PATH" with the path obtained from the `brew switch` or `brew install` command
  
    `set(OPENSSL_LIBRARIES "YOUR_OPENSSL_PATH/lib/libssl.a;YOUR_OPENSSL_PATH/lib/libcrypto.a")`
    `set(OPENSSL_INCLUDE_DIR "YOUR_OPENSSL_PATH/include")`    
    
### Windows

  * Both the Websocket and OpenSSL builds work on windows. The latest version of OpenSSL 1.0.2 needs to be installed for them to work properly.
  * Download and install the latest version of OpenSSL 1.0.2 from [this link](https://slproweb.com/products/Win32OpenSSL.html)
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

### Ubuntu 14.04
  * Install CMake [CMake Installation](https://cmake.org/install/)
  * To update to OpenSSL 1.0.2
    
    * `sudo apt-get install make` (if not already installed)
    * `wget https://www.openssl.org/source/openssl-1.0.2g.tar.gz` 
    * `tar -xzvf openssl-1.0.2g.tar.gz` 
    * `cd openssl-1.0.2g`
    * `sudo ./config` 
    * `sudo make install`
    * `sudo ln -sf /usr/local/ssl/bin/openssl <which openssl>`
    * `openssl version -v` (should show correct version of OpenSSL)

  * To install libssl-dev
    * Add the following PPA sources into `/etc/apt/sources.list` (Warning: This may break your setup) : 

      `deb [arch=i386,amd64] http://archive.ubuntu.com/ubuntu xenial main restricted universe multiverse `
      `deb [arch=i386,amd64] http://security.ubuntu.com/ubuntu xenial-security main restricted universe multiverse `
      `deb [arch=i386,amd64] http://archive.ubuntu.com/ubuntu xenial-backports main restricted universe multiverse `
      `deb [arch=arm64,armhf,powerpc] http://ports.ubuntu.com/ xenial main restricted universe multiverse `
      `deb [arch=arm64,armhf,powerpc] http://ports.ubuntu.com/ xenial-security main restricted universe multiverse `
      `deb [arch=arm64,armhf,powerpc] http://ports.ubuntu.com/ xenial-backports main restricted universe multiverse `
      `deb-src http://archive.ubuntu.com/ubuntu xenial main restricted universe multiverse `
      `deb-src http://security.ubuntu.com/ubuntu xenial-security main restricted universe multiverse `
      `deb-src http://archive.ubuntu.com/ubuntu xenial-backports main restricted universe multiverse `
      `deb-src http://ports.ubuntu.com/ xenial main restricted universe multiverse `
      `deb-src http://ports.ubuntu.com/ xenial-security main restricted universe multiverse `
      `deb-src http://ports.ubuntu.com/ xenial-backports main restricted universe multiverse `

    * `sudo apt-get update`
    * `sudo dpkg --configure -a --force-all`  (to fix broken libraries)
    * `sudo apt-get -f install` (to fix broken libraries)
    * `sudo apt-get install libssl-dev`
    * Delete the above additions from `/etc/apt/sources.list` (to remove the xenial backports from the sources list) 
    * `sudo apt-get update`
