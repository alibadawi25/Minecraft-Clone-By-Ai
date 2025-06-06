# Minecraft Clone - OpenGL Implementation

A modern OpenGL-based Minecraft clone built with C++ and the Windows API, featuring real-time rendering, FPS tracking, and a modular build system.

## 🚀 Features

- **Phase 1 Complete**: Window setup, OpenGL context, input handling, and real-time FPS display
- **Windows API Native**: No external windowing dependencies (GLFW-free implementation)
- **Modern OpenGL**: Uses OpenGL 4.6 with GLAD loader for function management
- **High-Performance**: 60+ FPS with optimized rendering pipeline
- **Real-time FPS**: Live FPS counter displayed in window title
- **Modular Build System**: CMake-based with external library integration
- **Cross-Compiler Support**: MinGW64 and MSVC compatible

## 📋 Prerequisites

### Required Tools

- **MinGW64** (GCC 14.2.0+ recommended) or **Visual Studio 2022**
- **CMake** (3.16 or higher)
- **Git** (for dependency management)

### Graphics Requirements

- **OpenGL 4.6** compatible graphics card
- **Windows 10/11** with updated graphics drivers

## 🔧 Quick Start

### 1. Clone and Setup

```bash
git clone <your-repo-url>
cd ai-karim
```

### 2. Install Dependencies (MinGW64)

```bash
# Run the dependency installer
install_deps_mingw.bat
```

### 3. Build (Release)

```bash
# Build release version
build_mingw.bat
```

### 4. Run

```bash
# Run the application
run_mingw.bat
```

## 🏗️ Build System

### Build Scripts

| Script            | Description                      |
| ----------------- | -------------------------------- |
| `build_mingw.bat` | Release build with optimizations |
| `build_debug.bat` | Debug build with symbols         |
| `run_mingw.bat`   | Run release build                |
| `run_debug.bat`   | Run debug build                  |
| `clean.bat`       | Clean all build artifacts        |

### Manual Building

#### Release Build

```bash
mkdir build_mingw
cd build_mingw
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

#### Debug Build

```bash
mkdir build_debug
cd build_debug
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build . --parallel
```

## 🎯 Development Setup

### VS Code Configuration

The project includes complete VS Code workspace configuration:

- **IntelliSense**: C++ language support with path resolution
- **Build Tasks**: Integrated build system (Ctrl+Shift+P → Tasks: Run Task)
- **Debugging**: GDB debugging with breakpoint support
- **Extensions**: Recommended extensions for C++ development

### Recommended VS Code Extensions

Install these extensions for the best development experience:

```bash
# Install via VS Code Extensions view or command palette
ms-vscode.cpptools          # C/C++ IntelliSense and debugging
ms-vscode.cmake-tools       # CMake integration
ms-vscode.hexeditor         # Binary file editing
twxs.cmake                  # CMake syntax highlighting
ms-vscode.cpptools-themes   # C++ syntax themes
```

## 📁 Project Structure

```
ai-karim/
├── src/                    # Source code
│   └── main.cpp           # Main application entry point
├── include/               # Header files
│   └── config.h          # Project configuration
├── external/             # External libraries
│   ├── glad/            # OpenGL function loader
│   ├── stb/             # Image processing library
│   └── FastNoise/       # Terrain generation library
├── .vscode/             # VS Code configuration
│   ├── tasks.json       # Build tasks
│   ├── launch.json      # Debug configuration
│   ├── c_cpp_properties.json  # IntelliSense settings
│   └── settings.json    # Workspace settings
├── build_mingw/         # Release build output
├── build_debug/         # Debug build output
└── CMakeLists.txt       # Build configuration
```

## 🔍 Technical Details

### Graphics Pipeline

- **Context**: OpenGL 4.6 Core Profile
- **Loading**: GLAD-based function loading
- **Rendering**: Immediate mode (Phase 1), VBO/VAO planned (Phase 2)
- **FPS Tracking**: High-resolution timer (QueryPerformanceCounter)

### External Libraries

- **GLAD**: OpenGL function loading and management
- **STB**: Image processing and texture loading
- **FastNoise**: Procedural noise generation for terrain

### Performance Metrics

- **Target FPS**: 60+ FPS sustained
- **Frame Time**: <16.67ms per frame
- **Memory**: Optimized for low memory footprint

## 🎮 Controls (Phase 1)

| Key      | Action           |
| -------- | ---------------- |
| `ESC`    | Exit application |
| `Alt+F4` | Exit application |

## 📚 Documentation

### Project Documentation

- **[📋 Project Status](STATUS.md)** - Current development status and metrics
- **[🗺️ Project Roadmap](ROADMAP.md)** - Long-term development plan and vision
- **[🔧 Development Guide](DEVELOPMENT.md)** - Comprehensive developer documentation
- **[📈 Phase 2 Plan](PHASE2.md)** - Detailed Phase 2 implementation guide
- **[📝 Changelog](CHANGELOG.md)** - Version history and progress tracking

### Quick Reference

- **Getting Started**: Follow the Quick Start section above
- **Building Issues**: Check [Troubleshooting](#🛠️-troubleshooting) section
- **Development Setup**: See [DEVELOPMENT.md](DEVELOPMENT.md) for detailed setup
- **Contributing**: Review [DEVELOPMENT.md](DEVELOPMENT.md) contribution guidelines

### API Documentation

- **OpenGL**: [LearnOpenGL](https://learnopengl.com/) - Comprehensive tutorials
- **GLAD**: [GLAD Documentation](https://glad.dav1d.de/) - OpenGL loader
- **STB**: [STB Libraries](https://github.com/nothings/stb) - Image processing
- **FastNoise**: [FastNoise2](https://github.com/Auburn/FastNoise2) - Terrain generation

## 🗺️ Roadmap

### ✅ Phase 1: Foundation (COMPLETE)

- [x] Window creation and management
- [x] OpenGL context initialization
- [x] Input system setup
- [x] FPS tracking and display
- [x] Build system configuration

### 🔄 Phase 2: Basic Rendering (READY TO BEGIN)

- [ ] Vertex buffer management (VBO/VAO)
- [ ] Shader compilation and management
- [ ] Basic cube rendering
- [ ] Camera system implementation
- [ ] Basic lighting setup

### 📅 Phase 3: World Generation (PLANNED)

- [ ] Chunk-based world system
- [ ] Procedural terrain generation
- [ ] Block type management
- [ ] LOD (Level of Detail) system

### 📅 Phase 4: Game Mechanics (PLANNED)

- [ ] Player movement and physics
- [ ] Block placement/destruction
- [ ] Collision detection
- [ ] Inventory system

## 🛠️ Troubleshooting

### Build Issues

```bash
# Clean build if experiencing issues
clean.bat
build_mingw.bat
```

### OpenGL Issues

- Ensure graphics drivers are updated
- Check OpenGL version: `glxinfo | grep "OpenGL version"` (Linux) or use GPU-Z (Windows)
- Verify GPU supports OpenGL 4.6

### Performance Issues

- Enable VSync in graphics driver settings
- Close unnecessary background applications
- Monitor GPU temperature and throttling

## 📝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- OpenGL community for excellent documentation
- GLAD project for OpenGL loading
- STB libraries for image processing
- FastNoise for procedural generation algorithms

```
.\build\Release\MinecraftClone.exe
```

## Controls

- **ESC**: Quit the application

## Phase 1 Features ✅

- [x] Create main loop with GLFW and OpenGL context
- [x] Create a window titled "Minecraft Clone"
- [x] Set background color and clear screen every frame
- [x] Add basic input handling (ESC to quit)
- [x] Print OpenGL version for verification

## What's Next?

Phase 2 will add cube rendering with basic vertex data and shaders.
