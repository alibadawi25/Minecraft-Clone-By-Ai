# Phase 2: Cube Rendering - Implementation Plan

This document outlines the implementation plan for Phase 2 of the Minecraft Clone project, focusing on basic 3D cube rendering capabilities.

## 🎯 Phase 2 Objectives

### Primary Goals

- [x] **Phase 1 Complete**: Window setup, OpenGL context, input handling, FPS display
- [ ] **Vertex Buffer Management**: Implement VBO/VAO system for efficient geometry
- [ ] **Shader System**: Create modular shader compilation and management
- [ ] **Basic Cube Rendering**: Render a single textured cube
- [ ] **Camera System**: Implement basic first-person camera with mouse/keyboard controls
- [ ] **Basic Lighting**: Simple directional lighting for visual depth

### Success Criteria

- Single cube renders correctly with texture
- Camera movement responds smoothly to input
- Maintains 60+ FPS performance
- Clean, modular code architecture
- Preparation for multi-cube world rendering

## 🏗️ Technical Architecture

### Rendering Pipeline Overview

```
Input → Camera Update → Scene Setup → Shader Binding → Geometry Rendering → Present
```

### Component Structure

```
src/
├── main.cpp              # Entry point and main loop
├── renderer/
│   ├── renderer.h        # Main renderer interface
│   ├── renderer.cpp      # Renderer implementation
│   ├── shader.h          # Shader management
│   ├── shader.cpp        # Shader compilation/linking
│   ├── buffer.h          # VBO/VAO management
│   ├── buffer.cpp        # Buffer implementation
│   └── camera.h          # Camera system
│   └── camera.cpp        # Camera implementation
├── window/
│   ├── window.h          # Window management interface
│   └── window.cpp        # Window implementation
└── input/
    ├── input.h           # Input handling interface
    └── input.cpp         # Input processing
```

## 📋 Implementation Tasks

### Task 1: Shader System Foundation

**Priority**: High | **Estimated Time**: 4-6 hours

#### Objectives

- Create reusable shader compilation system
- Implement shader program management
- Add error handling and validation
- Support vertex and fragment shaders

#### Implementation Details

```cpp
class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    void use();
    void setMatrix4(const std::string& name, const glm::mat4& matrix);
    void setVector3(const std::string& name, const glm::vec3& vector);
    void setFloat(const std::string& name, float value);

private:
    GLuint programID;
    GLuint compileShader(const std::string& source, GLenum type);
    std::string loadShaderFile(const std::string& path);
};
```

#### Deliverables

- [ ] `src/renderer/shader.h` - Shader class interface
- [ ] `src/renderer/shader.cpp` - Shader implementation
- [ ] `shaders/basic.vert` - Basic vertex shader
- [ ] `shaders/basic.frag` - Basic fragment shader
- [ ] Unit tests for shader compilation

### Task 2: Vertex Buffer Management

**Priority**: High | **Estimated Time**: 3-4 hours

#### Objectives

- Implement VBO/VAO wrapper classes
- Create efficient geometry data structures
- Support different vertex formats
- Enable easy mesh creation and modification

#### Implementation Details

```cpp
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

class VertexBuffer {
public:
    VertexBuffer(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    ~VertexBuffer();

    void bind();
    void unbind();
    void draw();

private:
    GLuint VAO, VBO, EBO;
    size_t indexCount;
};
```

#### Deliverables

- [ ] `src/renderer/buffer.h` - Buffer management interface
- [ ] `src/renderer/buffer.cpp` - Buffer implementation
- [ ] Cube geometry generation function
- [ ] Performance benchmarks for buffer operations

### Task 3: Camera System

**Priority**: High | **Estimated Time**: 4-5 hours

#### Objectives

- Implement first-person camera controls
- Support mouse look and keyboard movement
- Provide view and projection matrices
- Smooth interpolation and movement

#### Implementation Details

```cpp
class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f));

    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix(float aspectRatio);

    void processKeyboard(CameraMovement direction, float deltaTime);
    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
    void processMouseScroll(float yOffset);

private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;
    float movementSpeed;
    float mouseSensitivity;
    float zoom;
};
```

#### Deliverables

- [ ] `src/renderer/camera.h` - Camera class interface
- [ ] `src/renderer/camera.cpp` - Camera implementation
- [ ] Input integration for camera controls
- [ ] Smooth movement interpolation

### Task 4: Basic Cube Rendering

**Priority**: High | **Estimated Time**: 2-3 hours

#### Objectives

- Generate cube geometry with normals and texture coordinates
- Render single cube with basic shader
- Apply simple texture mapping
- Implement basic transformation matrices

#### Implementation Details

```cpp
class CubeRenderer {
public:
    CubeRenderer();
    ~CubeRenderer();

    void render(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection);
    void setTexture(GLuint textureID);

private:
    std::unique_ptr<VertexBuffer> cubeBuffer;
    std::unique_ptr<Shader> cubeShader;
    GLuint textureID;
};
```

#### Deliverables

- [ ] Cube geometry generation
- [ ] Basic texture loading (using STB)
- [ ] Cube rendering class
- [ ] Integration with main render loop

### Task 5: Input System Enhancement

**Priority**: Medium | **Estimated Time**: 2-3 hours

#### Objectives

- Enhance input handling for camera controls
- Add mouse capture for first-person view
- Implement smooth input processing
- Support configurable key bindings

#### Implementation Details

```cpp
class InputManager {
public:
    void processInput(HWND hwnd, float deltaTime);
    void processMouseMovement(float xPos, float yPos);
    void processMouseScroll(float offset);

    bool isKeyPressed(int key);
    void setCamera(Camera* camera);

private:
    Camera* camera;
    bool firstMouse;
    float lastX, lastY;
};
```

#### Deliverables

- [ ] Enhanced input processing
- [ ] Mouse capture and release
- [ ] Configurable input sensitivity
- [ ] Input state management

### Task 6: Basic Lighting

**Priority**: Medium | **Estimated Time**: 3-4 hours

#### Objectives

- Implement simple directional lighting
- Add basic material properties
- Support ambient, diffuse, and specular lighting
- Prepare for more complex lighting systems

#### Implementation Details

```cpp
struct Light {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};
```

#### Deliverables

- [ ] Lighting shader implementation
- [ ] Material system
- [ ] Light management class
- [ ] Phong lighting model

## 🎨 Shader Development

### Basic Vertex Shader (basic.vert)

```glsl
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
```

### Basic Fragment Shader (basic.frag)

```glsl
#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D ourTexture;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main()
{
    // Ambient lighting
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-lightDirection);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * texture(ourTexture, TexCoord).rgb;
    FragColor = vec4(result, 1.0);
}
```

## 🔧 Build System Updates

### CMakeLists.txt Additions

```cmake
# Add GLM for mathematics
find_package(glm REQUIRED)

# Include additional source directories
add_subdirectory(src/renderer)
add_subdirectory(src/window)
add_subdirectory(src/input)

# Link additional libraries
target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    ${GLAD_LIBRARIES}
    glm::glm
)

# Copy shader files to build directory
file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY textures DESTINATION ${CMAKE_BINARY_DIR})
```

### New Dependencies

- **GLM**: OpenGL Mathematics library for vectors and matrices
- **STB_IMAGE**: Image loading for textures
- **Additional headers**: For enhanced functionality

## 🧪 Testing Strategy

### Unit Tests

- [ ] Shader compilation and linking
- [ ] Buffer creation and management
- [ ] Camera matrix calculations
- [ ] Input processing accuracy

### Integration Tests

- [ ] Complete render pipeline
- [ ] Camera movement smoothness
- [ ] Performance under load
- [ ] Memory leak detection

### Visual Tests

- [ ] Cube renders correctly
- [ ] Textures display properly
- [ ] Lighting appears realistic
- [ ] Camera movement feels responsive

## 📊 Performance Targets

### Frame Rate Goals

- **Target**: 60+ FPS sustained
- **Minimum**: 30+ FPS under load
- **Method**: Frame time monitoring and optimization

### Memory Usage

- **Target**: <100MB for single cube
- **Monitoring**: Memory profiling tools
- **Optimization**: Efficient buffer management

### Render Time Budget

- **Total Frame**: 16.67ms (60 FPS)
- **Render**: <10ms
- **Update**: <3ms
- **Input**: <1ms
- **Other**: <2.67ms

## 🚀 Phase 2 Milestones

### Milestone 1: Foundation (Week 1)

- [ ] Shader system implemented and tested
- [ ] Basic vertex buffer management working
- [ ] Simple cube geometry generation

### Milestone 2: Rendering (Week 2)

- [ ] Single cube renders with texture
- [ ] Camera system functional
- [ ] Input controls responsive

### Milestone 3: Polish (Week 3)

- [ ] Basic lighting implemented
- [ ] Performance optimization completed
- [ ] Code cleanup and documentation

### Milestone 4: Preparation (Week 4)

- [ ] Phase 3 architecture planning
- [ ] Multi-cube rendering foundation
- [ ] World system design document

## 🔄 Transition to Phase 3

### Preparation Tasks

- [ ] Chunk system architecture design
- [ ] World generation algorithm research
- [ ] Performance profiling for multi-cube scenarios
- [ ] Memory management strategy for large worlds

### Technical Debt Review

- [ ] Code refactoring opportunities
- [ ] API simplification possibilities
- [ ] Performance bottleneck identification
- [ ] Documentation completeness audit

## 📚 Learning Resources

### OpenGL Specific

- [LearnOpenGL - Hello Triangle](https://learnopengl.com/Getting-started/Hello-Triangle)
- [LearnOpenGL - Shaders](https://learnopengl.com/Getting-started/Shaders)
- [LearnOpenGL - Camera](https://learnopengl.com/Getting-started/Camera)

### Mathematics

- [GLM Documentation](https://glm.g-truc.net/0.9.9/index.html)
- [3D Math Primer](http://gamemath.com/)
- [Linear Algebra for Graphics](https://paroj.github.io/gltut/)

### Game Development

- [Real-Time Rendering](http://www.realtimerendering.com/)
- [Game Engine Architecture](https://www.gameenginebook.com/)
- [OpenGL SuperBible](https://www.openglsuperbible.com/)

---

**Next Phase**: [Phase 3: World Generation](PHASE3.md) - Chunk system and procedural terrain generation
