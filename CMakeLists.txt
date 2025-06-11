cmake_minimum_required(VERSION 3.16)
project(MinecraftClone VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Project options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_WARNINGS "Enable compiler warnings" ON)
option(ENABLE_DEBUG_INFO "Enable debug information" ON)

# Platform detection
if(WIN32)
    message(STATUS "Building for Windows")
    add_definitions(-DPLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Building for Linux")
    add_definitions(-DPLATFORM_LINUX)
elseif(APPLE)
    message(STATUS "Building for macOS")
    add_definitions(-DPLATFORM_MACOS)
endif()

# Compiler-specific options
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    if(ENABLE_DEBUG_INFO)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(ENABLE_WARNINGS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    endif()
    if(ENABLE_DEBUG_INFO)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    endif()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)
if(NOT OpenGL_FOUND)
    message(FATAL_ERROR "OpenGL not found!")
endif()

# Set up external libraries paths
set(EXTERNAL_DIR "C:/Users/aliba/OneDrive/Documents/ai-karim/external")
set(GLAD_DIR "${EXTERNAL_DIR}/glad")
set(STB_DIR "${EXTERNAL_DIR}/stb")
set(FASTNOISE_DIR "${EXTERNAL_DIR}/fastNoise")
set(GLM_DIR "${EXTERNAL_DIR}/glm")
set(GLFW_DIR "${EXTERNAL_DIR}/glfw")

# Verify external libraries exist
if(NOT EXISTS "${GLAD_DIR}/src/glad.c")
    message(FATAL_ERROR "GLAD library not found at ${GLAD_DIR}")
endif()

if(NOT EXISTS "${STB_DIR}/stb_image.h")
    message(FATAL_ERROR "STB library not found at ${STB_DIR}")
endif()

# Verify GLM exists
if(NOT EXISTS "${GLM_DIR}/glm/glm.hpp")
    message(FATAL_ERROR "GLM library not found at ${GLM_DIR}")
endif()

# Verify GLFW exists
if(NOT EXISTS "${GLFW_DIR}/include/GLFW/glfw3.h")
    message(FATAL_ERROR "GLFW library not found at ${GLFW_DIR}")
endif()

# Add GLAD library
add_library(glad STATIC
    ${GLAD_DIR}/src/glad.c
)
target_include_directories(glad PUBLIC
    ${GLAD_DIR}/include
)

# Add STB library (header-only with implementation)
add_library(stb STATIC
    ${STB_DIR}/stb_image.cpp
)
target_include_directories(stb PUBLIC
    ${STB_DIR}
)

# Add FastNoise library
if(EXISTS "${FASTNOISE_DIR}/FastNoiseLite.cpp")
    add_library(fastnoise STATIC
        ${FASTNOISE_DIR}/FastNoiseLite.cpp
    )
    target_include_directories(fastnoise PUBLIC
        ${FASTNOISE_DIR}
    )
    set(FASTNOISE_AVAILABLE TRUE)
    message(STATUS "FastNoise library found and added")
else()
    set(FASTNOISE_AVAILABLE FALSE)
    message(STATUS "FastNoise library not found - will be available in later phases")
endif()

# Add GLFW library
if(WIN32)
    if(MINGW)
        set(GLFW_LIB_PATH "${GLFW_DIR}/lib-mingw-w64/libglfw3.a")
    else()
        set(GLFW_LIB_PATH "${GLFW_DIR}/lib-vc2022/glfw3.lib")
    endif()
else()
    set(GLFW_LIB_PATH "${GLFW_DIR}/lib/libglfw3.a")
endif()

add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION "${GLFW_LIB_PATH}"
    INTERFACE_INCLUDE_DIRECTORIES "${GLFW_DIR}/include"
)

# Create include directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Collect source files explicitly (instead of GLOB for better dependency tracking)
set(WORLD_SOURCES
    src/world/block.cpp
    src/world/chunk.cpp
    src/world/world.cpp
)

set(UTILS_SOURCES
    src/utils/math_utils.cpp
)

set(RENDERER_SOURCES
    src/renderer/camera.cpp
    src/renderer/simple_shader.cpp
    src/renderer/texture.cpp
)

set(UI_SOURCES
    src/ui/imgui_ui.cpp
)

# Add ImGui library
set(IMGUI_DIR "${EXTERNAL_DIR}/imgui")
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLFW_DIR}/include
    ${GLAD_DIR}/include
)

# Add main executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    ${WORLD_SOURCES}
    ${UTILS_SOURCES}
    ${RENDERER_SOURCES}
    ${UI_SOURCES}
)

# Set executable properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "MinecraftClone"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glad
    stb
    imgui
    glfw
    OpenGL::GL
)

# Add FastNoise if available
if(FASTNOISE_AVAILABLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE fastnoise)
    target_compile_definitions(${PROJECT_NAME} PRIVATE FASTNOISE_AVAILABLE)
endif()

# Platform-specific linking
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        gdi32
        user32
        kernel32
    )
    # Set subsystem for console application to avoid WinMain requirement
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-mconsole"
    )
    if(MINGW)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            opengl32
        )
    endif()
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        X11
        GL
        pthread
        dl
    )
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GLAD_DIR}/include
    ${STB_DIR}
    ${GLM_DIR}
    ${GLFW_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

if(FASTNOISE_AVAILABLE)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FASTNOISE_DIR})
endif()

# Add compile definitions for build configuration
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
    GLFW_BUILD
    PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Print build information
message(STATUS "")
message(STATUS "=== Build Configuration ===")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "")
message(STATUS "=== External Libraries ===")
message(STATUS "GLAD: ${GLAD_DIR}")
message(STATUS "STB: ${STB_DIR}")
message(STATUS "GLM: ${GLM_DIR}")
message(STATUS "GLFW: ${GLFW_DIR}")
message(STATUS "FastNoise: ${FASTNOISE_AVAILABLE}")
message(STATUS "OpenGL: ${OPENGL_gl_LIBRARY}")
message(STATUS "===========================")
message(STATUS "")
