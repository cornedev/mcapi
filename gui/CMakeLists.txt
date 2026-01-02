cmake_minimum_required(VERSION 3.16)

project(cclauncher LANGUAGES CXX)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(LibArchive REQUIRED)
if(NOT WIN32)
    find_package(CURL REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
set(RUNTIME_DIR ${CMAKE_SOURCE_DIR}/runtime)
set(RUNTIME_BIN_DIR ${RUNTIME_DIR}/bin)
set(FONT_DIR ${RUNTIME_DIR}/fonts)
set(GFX_DIR  ${CMAKE_SOURCE_DIR}/gfx)

set(SOURCES
    gui.cpp
    ../api/ccapi_assets.cpp
    ../api/ccapi_process.cpp
    ../api/ccapi_java.cpp

    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp

    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set(ICON_RC gfx/icon.rc)

set(FONTS
    ${FONT_DIR}/arial.ttf
)

set(GFX
    ${GFX_DIR}/clilauncher.png
    ${GFX_DIR}/bg_day.png
)

add_executable(cclauncher
    ${SOURCES}
    ${ICON_RC}
)

set_target_properties(cclauncher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_include_directories(cclauncher PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${RUNTIME_DIR}/include
)

target_compile_options(cclauncher PRIVATE
    -Wall
    -Wformat
)

if (WIN32)
    set(CURL_LIB ${RUNTIME_DIR}/lib/libcurl.dll.a)
    target_link_libraries(cclauncher PRIVATE
        ${CURL_LIB}
        zip
        LibArchive::LibArchive
        glfw
        opengl32
        gdi32
        imm32
    )
elseif(APPLE)
    target_link_libraries(cclauncher PRIVATE
        CURL::libcurl
        zip
        LibArchive::LibArchive
        glfw
        OpenGL::GL
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
else()
    target_link_libraries(cclauncher PRIVATE
        CURL::libcurl
        zip
        LibArchive::LibArchive
        glfw
        OpenGL::GL
    )
endif()

add_custom_command(TARGET cclauncher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:cclauncher>/fonts
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FONTS}
            $<TARGET_FILE_DIR:cclauncher>/fonts

    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:cclauncher>/gfx
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${GFX}
            $<TARGET_FILE_DIR:cclauncher>/gfx
            
    COMMENT "Copying files to build directory..."
)

if(WIN32)
    file(GLOB RUNTIME_DLLS
        "${RUNTIME_BIN_DIR}/*.dll"
    )

    add_custom_command(TARGET cclauncher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${RUNTIME_DLLS}
                $<TARGET_FILE_DIR:cclauncher>
        COMMENT "Copying runtime dlls to build directory..."
    )
endif()