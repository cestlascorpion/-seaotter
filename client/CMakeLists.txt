cmake_minimum_required(VERSION 2.8)

project(client)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wconversion -Wsizeof-pointer-memaccess -Wfloat-equal \
                     -Wconversion-null -Woverflow -Wshadow -std=c++11 -pthread -g -o3")

include_directories(${PROTOBUF_INCLUDE_DIRS})

include_directories(${PROTO_DIR})
include_directories(${COMMON_DIR})
include_directories(${CLIENT_DIR})

file(GLOB SOURCE ${COMMON_DIR}/*.cpp ${CLIENT_DIR}/*.cpp)
file(GLOB HEADER ${COMMON_DIR}/*.h ${CLIENT_DIR}/*.h)

add_executable(${PROJECT_NAME} ${SOURCE} ${HEADER} ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(${PROJECT_NAME} ${PROTOBUF_LIBRARIES})
