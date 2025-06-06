#pragma once

// Project version
#define MINECRAFT_CLONE_VERSION_MAJOR 1
#define MINECRAFT_CLONE_VERSION_MINOR 0
#define MINECRAFT_CLONE_VERSION_PATCH 0

// Platform detection
#ifdef PLATFORM_WINDOWS
    #define PLATFORM_NAME "Windows"
#elif defined(PLATFORM_LINUX)
    #define PLATFORM_NAME "Linux"
#elif defined(PLATFORM_MACOS)
    #define PLATFORM_NAME "macOS"
#else
    #define PLATFORM_NAME "Unknown"
#endif

// Build configuration
#ifdef DEBUG_BUILD
    #define BUILD_TYPE "Debug"
    #define DEBUG_MODE 1
#else
    #define BUILD_TYPE "Release"
    #define DEBUG_MODE 0
#endif

// Window settings
#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600
#define WINDOW_TITLE "Minecraft Clone v1.0.0"

// OpenGL settings
#define OPENGL_VERSION_MAJOR 3
#define OPENGL_VERSION_MINOR 3

// Debug macros
#if DEBUG_MODE
    #include <iostream>
    #define DEBUG_LOG(x) std::cout << "[DEBUG] " << x << std::endl
    #define DEBUG_ERROR(x) std::cerr << "[ERROR] " << x << std::endl
#else
    #define DEBUG_LOG(x)
    #define DEBUG_ERROR(x)
#endif

// Utility macros
#define UNUSED(x) ((void)(x))
