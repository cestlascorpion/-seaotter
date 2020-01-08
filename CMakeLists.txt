cmake_minimum_required(VERSION 2.8)

project(SeaOtter)

find_package(Protobuf REQUIRED)
if (PROTOBUF_FOUND)
    message("protobuf found")
else ()
    message(FATAL_ERROR "Cannot find Protobuf")
endif ()

set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/proto/CallReport.proto)

set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)
set(CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/client)
set(SERVER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/server)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(proto)
add_subdirectory(server)
add_subdirectory(client)