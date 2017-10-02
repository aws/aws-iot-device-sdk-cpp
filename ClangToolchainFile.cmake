# The OpenSSL paths in the network/CMakeLists.txt needs to be replaced with the path to the cross-compiled OpenSSL library for
# the required paltform

# General CMAKE cross compile settings
SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)

# Set compiler
SET(CMAKE_CXX_COMPILER clang++)

# Set linker
SET(CMAKE_LINKER ld)

# Set compiler flags
SET(CMAKE_CXX_FLAGS ${COMMON_FLAGS} -std=c++11)

# Set archiving tool
SET(CMAKE_AR ar CACHE FILEPATH "Archiver")

# Set randomizing tool for static libraries
SET(CMAKE_RANLIB ranlib)

# Set strip tool
SET(CMAKE_STRIP strip)

# Set objdump tool
SET(CMAKE_OBJDUMP objdump)

# Set objcopy tool
SET(CMAKE_OBJCOPY objcopy)

# Set nm tool
SET(CMAKE_NM nm)
