cmake_minimum_required(VERSION 3.20)

# Configure for clang
## Not completely sure this section is needed, but just in case
#SET (CMAKE_C_COMPILER                         "clang")
#SET (CMAKE_C_FLAGS                            "-Wall")
#SET (CMAKE_C_FLAGS_DEBUG                      "-g")
#SET (CMAKE_C_FLAGS_RELEASE                    "-O3 -DNDEBUG")

## clang++
SET (CMAKE_CXX_COMPILER         "clang++")
SET (CMAKE_CXX_FLAGS            "-Wall --stdlib=libc++")
SET (CMAKE_CXX_FLAGS_DEBUG      "-g")
SET (CMAKE_CXX_FLAGS_RELEASE    "-O3 -DNDEBUG")

# set the linker
#SET (CMAKE_CXX_LINK_EXECUTABLE    "/usr/bin/lld")

# set release
#SET (CMAKE_RELEASE_TYPE    Release)

# specify the C++ standard
## this should be equal to -std=c++20
SET (CMAKE_CXX_STANDARD             20)
SET (CMAKE_CXX_STANDARD_REQUIRED    True)

# set the project name
project(
  NOSO-2M
  VERSION 0.2.4
  LANGUAGES CXX
  HOMEPAGE_URL "https://github.com/f04ever/noso-2m")

# Find required packages
## pthread
SET (CMAKE_THREAD_PREFER_PTHREAD    True)
find_package(Threads REQUIRED)

## ncurses
#SET (CURSES_NEED_NCURSES    True)
#find_package(Curses REQUIRED)

# add the executable
add_executable(noso-2m md5-c.cpp noso-2m.cpp)

# configure defaults
SET (${PROJECT_NAME}_DEFAULT_CONFIG_FILENAME          "noso-2m.cfg")
SET (${PROJECT_NAME}_DEFAULT_LOGGING_FILENAME         "noso-2m.log")
SET (${PROJECT_NAME}_DEFAULT_POOL_URL_LIST            "f04ever;devnoso")
SET (${PROJECT_NAME}_DEFAULT_MINER_ADDRESS            "N3G1HhkpXvmLcsWFXySdAxX3GZpkMFS")
SET (${PROJECT_NAME}_DEFAULT_MINER_ID                 0)
SET (${PROJECT_NAME}_DEFAULT_THREADS_COUNT            2)
SET (${PROJECT_NAME}_DEFAULT_NODE_INET_TIMEOSEC       10)
SET (${PROJECT_NAME}_DEFAULT_POOL_INET_TIMEOSEC       60)
SET (${PROJECT_NAME}_DEFAULT_TIMESTAMP_DIFFERENCES    3)

# take care of the configurations file
configure_file(noso-2m_config.h.in noso-2m_config.h)

# add folders to search for include files
target_include_directories(noso-2m PUBLIC
  "${PROJECT_BINARY_DIR}"
#  "${CURSES_INCLUDE_DIRS}"
)

# add the libs to the linker
target_link_libraries(noso-2m PUBLIC
  pthread
#  c++
#  c++abi
  ncurses
#  "${CURSES_LIBRARIES}"
  form
#  termcap
)

