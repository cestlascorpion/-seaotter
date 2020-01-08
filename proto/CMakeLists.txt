cmake_minimum_required(VERSION 2.8)

set(OUT_CPP ${PROTO_DIR}/)

get_filename_component(FIL_WE ${PROTO_FILE} NAME_WE)

set(ARGS "-I=${PROTO_DIR} --cpp_out=${OUT_CPP} --proto_path==${PROTOBUF_INCLUDE_DIRS}")
set(PROTOBUF_COMMAND "${PROTOBUF_PROTOC_EXECUTABLE} ${ARGS} ${PROTO_FILE}")
exec_program(${PROTOBUF_COMMAND})

set(PROTO_SRCS ${OUT_CPP}${FIL_WE}.pb.cc PARENT_SCOPE)
set(PROTO_HDRS ${OUT_CPP}${FIL_WE}.pb.h PARENT_SCOPE)
