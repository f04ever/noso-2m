cmake_minimum_required(VERSION 3.10)

# Configure for clang
## Not completely sure this section is needed, but just in case
SET (CMAKE_C_COMPILER                         "clang")
SET (CMAKE_C_FLAGS                            "-Wall")
SET (CMAKE_C_FLAGS_DEBUG                      "-g")
SET (CMAKE_C_FLAGS_RELEASE                    "-O3 -DNDEBUG")

## clang++
SET (CMAKE_CXX_COMPILER                       "clang++")
SET (CMAKE_CXX_FLAGS_DEBUG                    "-g")
SET (CMAKE_CXX_FLAGS_RELEASE                  "-O3 -DNDEBUG")

# set the linker
SET (CMAKE_LINKER                             "lld")

# set release
#SET (CMAKE_RELEASE_TYPE                       Release)

# specify the C++ standard
## this should be equal to -std=c++20
SET (CMAKE_CXX_STANDARD                       20)
SET (CMAKE_CXX_STANDARD_REQUIRED              True)

# set the project name
project(noso-2m VERSION 0.2.4)

# add the executable
add_executable(noso-2m md5-c.cpp noso-2m.cpp)

# configure defaults
SET (noso-2m_DEFAULT_CONFIG_FILENAME          "noso-2m.cfg")
SET (noso-2m_DEFAULT_LOGGING_FILENAME         "noso-2m.log")
SET (noso-2m_DEFAULT_POOL_URL_LIST            "f04ever;devnoso")
SET (noso-2m_DEFAULT_MINER_ADDRESS            "N3G1HhkpXvmLcsWFXySdAxX3GZpkMFS")
SET (noso-2m_DEFAULT_MINER_ID                 0)
SET (noso-2m_DEFAULT_THREADS_COUNT            2)
SET (noso-2m_DEFAULT_NODE_INET_TIMEOSEC       10)
SET (noso-2m_DEFAULT_POOL_INET_TIMEOSEC       60)
SET (noso-2m_DEFAULT_TIMESTAMP_DIFFERENCES    3)

# take care of the configurations file
configure_file(noso-2m_config.h.in noso-2m_config.h)

# add folders to search for include files
target_include_directories(noso-2m PUBLIC "${PROJECT_BINARY_DIR}")

# add the libs
target_link_libraries(noso-2m PUBLIC pthread c++abi ncurses form termcap)

