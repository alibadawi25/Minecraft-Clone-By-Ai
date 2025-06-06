# Development Guide - Minecraft Clone

This guide provides detailed information for developers working on the Minecraft Clone project.

## 🛠️ Development Environment Setup

### Prerequisites Installation

#### 1. MinGW64 Setup

```bash
# Download and install MSYS2
# Visit: https://www.msys2.org/

# Install MinGW64 toolchain
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-gdb

# Add to PATH: C:\msys64\mingw64\bin
```

#### 2. VS Code Extensions

Install the recommended extensions from the Extensions panel:

- **C/C++**: IntelliSense, debugging, and code browsing
- **CMake Tools**: CMake integration and project management
- **GitLens**: Enhanced Git capabilities
- **Error Lens**: Inline error and warning display
- **TODO Tree**: Task and comment management

### Project Configuration

#### VS Code Workspace

The `.vscode` folder contains pre-configured settings for:

- **IntelliSense**: C++ language server configuration
- **Build Tasks**: Integrated build system
- **Debugging**: GDB debugging setup
- **Code Formatting**: Consistent code style

#### Build System

- **CMake**: Modern build system with dependency management
- **MinGW64**: Cross-platform GCC toolchain
- **External Libraries**: Automated integration

## 🏗️ Build System Deep Dive

### CMake Configuration

#### Main CMakeLists.txt Structure

```cmake
# Project setup
cmake_minimum_required(VERSION 3.16)
project(MinecraftClone VERSION 1.0.0)

# C++ standard and compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DDEBUG")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
endif()

# External libraries and dependencies
# Source files and target configuration
```

#### Build Configurations

| Configuration      | Flags            | Use Case                        |
| ------------------ | ---------------- | ------------------------------- |
| **Debug**          | `-g -O0 -DDEBUG` | Development, debugging          |
| **Release**        | `-O3 -DNDEBUG`   | Production, performance testing |
| **RelWithDebInfo** | `-O2 -g`         | Profiling, optimized debugging  |

### External Dependencies

#### GLAD (OpenGL Loading)

- **Purpose**: Modern OpenGL function loading
- **Version**: OpenGL 4.6 Core Profile
- **Integration**: Source files included in `external/glad/`

#### STB Libraries

- **stb_image.h**: Image loading and processing
- **Usage**: Texture loading for blocks and UI
- **Integration**: Header-only library in `external/stb/`

#### FastNoise

- **Purpose**: Procedural noise generation
- **Usage**: Terrain generation algorithms
- **Integration**: Source library in `external/FastNoise/`

## 🎯 Development Workflow

### Building and Testing

#### Quick Development Cycle

```bash
# 1. Make changes to source code
# 2. Build debug version
build_debug.bat

# 3. Run and test
run_debug.bat

# 4. Check for issues and repeat
```

#### Performance Testing

```bash
# Build optimized version
build_mingw.bat

# Run performance tests
run_mingw.bat

# Monitor FPS in window title
```

### VS Code Integration

#### Build Tasks (Ctrl+Shift+P → Tasks: Run Task)

- **Build Debug**: Compile debug version
- **Build Release**: Compile release version
- **Clean**: Remove build artifacts
- **Run Debug**: Execute debug build
- **Run Release**: Execute release build

#### Debugging (F5)

- **Breakpoints**: Click line numbers to set/remove
- **Variables**: Watch panel shows variable values
- **Call Stack**: Navigate function call hierarchy
- **Debug Console**: Execute expressions during debugging

### Code Organization

#### File Structure Guidelines

```
src/
├── main.cpp              # Application entry point
├── window/               # Window management
├── renderer/             # Rendering system
├── input/                # Input handling
├── world/                # World generation and management
├── blocks/               # Block types and properties
├── player/               # Player system
└── utils/                # Utility functions

include/
├── config.h              # Project configuration
├── window/               # Window headers
├── renderer/             # Rendering headers
└── common/               # Shared definitions
```

#### Coding Standards

##### Naming Conventions

```cpp
// Classes: PascalCase
class RenderEngine { };

// Functions: camelCase
void updateFrameBuffer();

// Variables: camelCase
int frameCount;

// Constants: UPPER_SNAKE_CASE
const int MAX_CHUNKS = 256;

// File names: snake_case
// render_engine.cpp, input_handler.h
```

##### Code Style

```cpp
// Indentation: 4 spaces (no tabs)
// Braces: Allman style for functions, K&R for control structures
void functionName()
{
    if (condition) {
        // Code here
    }
}

// Comments: Clear and descriptive
/**
 * @brief Updates the game world state
 * @param deltaTime Time elapsed since last update
 */
void updateWorld(float deltaTime);
```

## 🔧 Advanced Development Topics

### OpenGL Programming

#### Context Management

```cpp
// OpenGL context creation (Windows API)
HGLRC context = wglCreateContext(deviceContext);
wglMakeCurrent(deviceContext, context);

// GLAD initialization
if (!gladLoadGL()) {
    // Handle error
}
```

#### Shader Management

```cpp
// Vertex shader compilation
GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);

// Fragment shader compilation
GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

// Program linking
GLuint program = linkProgram(vertexShader, fragmentShader);
```

### Performance Optimization

#### Rendering Pipeline

- **Batch Rendering**: Minimize draw calls
- **Frustum Culling**: Skip invisible geometry
- **Level of Detail**: Reduce complexity at distance
- **Texture Atlasing**: Combine textures to reduce binding

#### Memory Management

- **Object Pooling**: Reuse frequently allocated objects
- **Chunk Loading**: Dynamic world loading/unloading
- **Buffer Management**: Efficient GPU memory usage

### Profiling and Debugging

#### Performance Monitoring

```cpp
// High-resolution timing
LARGE_INTEGER frequency, start, end;
QueryPerformanceFrequency(&frequency);
QueryPerformanceCounter(&start);

// ... code to profile ...

QueryPerformanceCounter(&end);
double elapsed = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
```

#### Debug Output

```cpp
#ifdef DEBUG
    #define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif

DEBUG_PRINT("Frame time: %.2f ms\n", frameTime);
```

## 🎮 Game Architecture

### Component Systems

#### Rendering System

- **Vertex Buffers**: Efficient geometry storage
- **Shader Programs**: GPU-based rendering
- **Texture Management**: Image loading and binding
- **Camera System**: View and projection matrices

#### World System

- **Chunk Management**: Divide world into manageable sections
- **Block System**: Efficient block storage and retrieval
- **Generation**: Procedural world creation
- **Persistence**: Save/load world data

#### Input System

- **Event Processing**: Handle Windows messages
- **Input Mapping**: Convert raw input to game actions
- **Response Time**: Minimize input latency

### Phase Development Plan

#### Phase 2: Basic Rendering

```cpp
// Planned features:
- Vertex Buffer Objects (VBO)
- Vertex Array Objects (VAO)
- Basic shader system
- Cube rendering
- Camera implementation
```

#### Phase 3: World Generation

```cpp
// Planned features:
- Chunk-based world system
- Noise-based terrain generation
- Block type management
- LOD implementation
```

#### Phase 4: Game Mechanics

```cpp
// Planned features:
- Player movement and physics
- Block placement/destruction
- Collision detection
- Basic inventory system
```

## 🐛 Troubleshooting

### Common Build Issues

#### CMake Configuration Errors

```bash
# Clear CMake cache
rm -rf build_*/
rm CMakeCache.txt

# Reconfigure
cmake .. -G "MinGW Makefiles"
```

#### Linking Errors

```bash
# Check library paths in CMakeLists.txt
# Verify external dependencies are built
# Check compiler and linker flags
```

### Runtime Issues

#### OpenGL Context Errors

- Verify graphics drivers are updated
- Check OpenGL version compatibility
- Ensure context creation succeeds

#### Performance Issues

- Monitor GPU usage and temperature
- Check for memory leaks
- Profile rendering pipeline bottlenecks

### Development Tools

#### Debugging Commands

```bash
# GDB debugging
gdb ./minecraft_clone.exe
(gdb) break main
(gdb) run
(gdb) step
```

#### Memory Analysis

```bash
# Valgrind (Linux/WSL)
valgrind --tool=memcheck ./minecraft_clone

# Windows: Application Verifier
# Visual Studio: Diagnostic Tools
```

## 📚 Resources

### OpenGL Learning

- [LearnOpenGL](https://learnopengl.com/) - Comprehensive OpenGL tutorial
- [OpenGL Reference](https://www.opengl.org/sdk/docs/) - Official documentation
- [OpenGL Wiki](https://www.khronos.org/opengl/wiki/) - Community resources

### C++ Development

- [cppreference.com](https://en.cppreference.com/) - C++ language reference
- [Modern C++ Guidelines](https://github.com/isocpp/CppCoreGuidelines) - Best practices
- [CMake Documentation](https://cmake.org/documentation/) - Build system guide

### Game Development

- [Game Programming Patterns](http://gameprogrammingpatterns.com/) - Architecture patterns
- [Real-Time Rendering](http://www.realtimerendering.com/) - Advanced rendering techniques
- [Minecraft Technical](https://minecraft.wiki/) - Game mechanics reference

## 🤝 Contributing

### Code Review Process

1. Create feature branch from `main`
2. Implement changes with tests
3. Ensure code follows style guidelines
4. Submit pull request with description
5. Address review feedback
6. Merge after approval

### Testing Guidelines

- Test on both Debug and Release builds
- Verify performance benchmarks
- Check memory usage and leaks
- Test on different hardware configurations

### Documentation Standards

- Update README for user-facing changes
- Add inline code documentation
- Update this development guide for architectural changes
- Include examples for new APIs
