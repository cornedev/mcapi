cmake_minimum_required(VERSION 3.16)

project(ccapi_cli LANGUAGES CXX)
find_package(LibArchive REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RUNTIME_DIR ${CMAKE_SOURCE_DIR}/runtime)
set(RUNTIME_BIN_DIR ${RUNTIME_DIR}/bin)

set(SOURCES
    cli.cpp
    ../api/ccapi_assets.cpp
    ../api/ccapi_process.cpp
    ../api/ccapi_java.cpp
)

set(ICON_RC gfx/icon.rc)

add_executable(ccapi_cli
    ${SOURCES}
    ${ICON_RC}
)

set_target_properties(ccapi_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_include_directories(ccapi_cli PRIVATE
    ${RUNTIME_DIR}/include
)

target_compile_options(ccapi_cli PRIVATE
    -Wall
    -Wextra
)

set(CURL_LIB ${RUNTIME_DIR}/lib/libcurl.dll.a)

target_link_libraries(ccapi_cli PRIVATE
    ${CURL_LIB}
    zip
    LibArchive::LibArchive
)

file(GLOB RUNTIME_DLLS
    "${RUNTIME_BIN_DIR}/*.dll"
)

add_custom_command(TARGET ccapi_cli POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${RUNTIME_DLLS}
            $<TARGET_FILE_DIR:ccapi_cli>
    COMMENT "Copying runtime DLLs to build directory"
)
