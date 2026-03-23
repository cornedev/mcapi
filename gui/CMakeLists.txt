cmake_minimum_required(VERSION 3.19)

project(mcapi_gui LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent)
find_package(nlohmann_json REQUIRED)
find_package(LibArchive REQUIRED)
if(NOT WIN32)
    find_package(CURL REQUIRED)
endif()

qt_standard_project_setup()

set(RUNTIME_DIR ${CMAKE_SOURCE_DIR}/runtime)
set(RUNTIME_BIN_DIR ${RUNTIME_DIR}/bin)
set(PLATFORMS_DIR ${RUNTIME_DIR}/platforms)
set(CMAKE_AUTORCC ON)

if(WIN32)
    set(ICON_RC gfx/icon.rc)
else()
    set(ICON_RC "")
endif()

qt_add_executable(mcapi_gui
    WIN32 MACOSX_BUNDLE
    main.cpp
    gui.cpp
    gui.h
    gui.ui
    resources.qrc
    ../api/mcapi_assets.cpp
    ../api/mcapi_process.cpp
    ../api/mcapi_java.cpp
    ${ICON_RC}
)

target_link_libraries(mcapi_gui PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Concurrent
    ${RUNTIME_DIR}/include
)

install(TARGETS mcapi_gui
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if (WIN32)
    set(CURL_LIB ${RUNTIME_DIR}/lib/libcurl.dll.a)
    target_link_libraries(mcapi_gui PRIVATE
        ${CURL_LIB}
        LibArchive::LibArchive
        nlohmann_json::nlohmann_json
    )
elseif(APPLE)
    target_link_libraries(mcapi_gui PRIVATE
        CURL::libcurl
        LibArchive::LibArchive
        nlohmann_json::nlohmann_json
    )
else()
    target_link_libraries(mcapi_gui PRIVATE
        CURL::libcurl
        LibArchive::LibArchive
        nlohmann_json::nlohmann_json
    )
endif()

if(WIN32)
    file(GLOB PLATFORMS "${PLATFORMS_DIR}/*.dll")

    add_custom_command(TARGET mcapi_gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                $<TARGET_FILE_DIR:mcapi_gui>/platforms
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${PLATFORMS}
                $<TARGET_FILE_DIR:mcapi_gui>/platforms
    )
endif()

if(WIN32)
    file(GLOB RUNTIME_DLLS "${RUNTIME_BIN_DIR}/*.dll")

    add_custom_command(TARGET mcapi_gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${RUNTIME_DLLS}
                $<TARGET_FILE_DIR:mcapi_gui>
        COMMENT "Copying runtime dlls to build directory..."
    )
endif()
