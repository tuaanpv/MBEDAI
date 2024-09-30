cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 17)
message("CMAKE_CXX_STANDARD = ${CMAKE_CXX_STANDARD}")

set(TARGET main)
project(${TARGET})

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -pthread -lX11")
# set(CMAKE_CXX_FLAGS "-pthread")
# set(MAKEFLAGS "-j$(nproc)")
# set(ENV{MAKEFLAGS} "-j$(nproc)")
set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")

find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)

find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)

find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_library(AVUTIL_LIBRARY avutil)

find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
find_library(AVDEVICE_LIBRARY avdevice)

find_package(OpenCV 4.5 REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVDEVICE_INCLUDE_DIR})
link_directories(${OpenCV_LIBRARY_DIRS})
add_definitions(${OpenCV_DEFINITIONS})

add_subdirectory(/usr/local/include/dlib dlib_build)
# add_subdirectory(include/rtsp/xop xop)
# add_subdirectory(include/rtsp/net net)
# add_subdirectory(include/rtsp/3rdpart 3rdpart)

include_directories(include/rtsp)
include_directories(include/popen2)
include_directories(include/websocket)


# Find all executables
file(GLOB project_SRCS src/*.cpp) #src/*.h


#########################################################################
# rtsp
file(GLOB rtsp_includes 
        include/rtsp/xop/*.h 
        include/rtsp/net/*.h 
        include/rtsp/xop/*.cpp 
        include/rtsp/net/*.cpp)

file(GLOB popen2_includes 
include/popen2/*.h 
include/popen2/*.c)
# rtsp
#########################################################################


#########################################################################
# Websocket
file(GLOB websocket_includes 
        include/websocket/*.h 
        include/websocket/*.cpp)


# Add pkg-config functionality
find_package(PkgConfig REQUIRED) 

# search for libevent.pc
pkg_search_module(EVENT REQUIRED libevent)


set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
# Websocket
#########################################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(Eigen3 REQUIRED)

# Add project executable
add_executable(${TARGET} ${project_SRCS} ${rtsp_includes} ${popen2_includes} ${websocket_includes})
target_link_libraries(${TARGET} dlib::dlib ${OpenCV_LIBRARIES} 
                        ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} 
                        ${AVDEVICE_LIBRARY} ${EVENT_LIBRARIES} OpenSSL::Crypto)
target_include_directories(${TARGET} PRIVATE ${EVENT_INCLUDE_DIRS})

add_custom_target(make
    COMMAND make -j$(nproc)
)

add_custom_target(really-clean
    COMMAND make clean
    COMMAND rm -rf CMake* cmake_install.cmake compile_commands.json dlib_build
)

add_custom_target(fresh
    COMMAND make really-clean
    COMMAND cmake ..
    COMMAND make -j12
)

add_custom_target(run
    COMMAND ./${TARGET} ../images/schoolzone.mp4
)