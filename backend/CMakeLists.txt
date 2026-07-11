cmake_minimum_required(VERSION 3.14)
project(server VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies ────────────────────────────────────────
find_package(gRPC REQUIRED)
find_package(absl REQUIRED)
find_package(CURL REQUIRED)
find_program(PROTOC_EXECUTABLE protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN_EXECUTABLE NAMES grpc_cpp_plugin grpc_cpp_plugin.exe REQUIRED)

# ── Proto config ────────────────────────────────────────
set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
file(GLOB PROTO_FILES ${PROTO_DIR}/*.proto)

# ── Generate backend C++ protos ─────────────────────────
set(GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/gen)
set(BACKEND_GEN_SRCS)
set(BACKEND_GEN_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)

    set(CUR_SRCS
        ${GEN_DIR}/${PROTO_NAME}.pb.cc
        ${GEN_DIR}/${PROTO_NAME}.grpc.pb.cc
    )
    set(CUR_HDRS
        ${GEN_DIR}/${PROTO_NAME}.pb.h
        ${GEN_DIR}/${PROTO_NAME}.grpc.pb.h
    )

    add_custom_command(
        OUTPUT ${CUR_SRCS} ${CUR_HDRS}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GEN_DIR}
        COMMAND ${PROTOC_EXECUTABLE}
            --grpc_out=${GEN_DIR}
            --cpp_out=${GEN_DIR}
            -I ${PROTO_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
            ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating C++ proto: ${PROTO_NAME}"
    )

    list(APPEND BACKEND_GEN_SRCS ${CUR_SRCS})
    list(APPEND BACKEND_GEN_HDRS ${CUR_HDRS})
endforeach()

# ── Server executable ───────────────────────────────────
file(GLOB_RECURSE SERVER_SRCS CONFIGURE_DEPENDS src/*.cpp)
list(REMOVE_ITEM SERVER_SRCS ${GEN_DIR}/*.cc)
list(APPEND SERVER_SRCS ${BACKEND_GEN_SRCS})

file(GLOB_RECURSE SERVER_HDRS CONFIGURE_DEPENDS src/*.h)

add_executable(server ${SERVER_SRCS} ${SERVER_HDRS} ${BACKEND_GEN_HDRS})

target_include_directories(server PRIVATE
    ${GEN_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(server
    gRPC::grpc++
    protobuf::libprotobuf
    absl::strings
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    CURL::libcurl
)
