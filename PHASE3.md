# Phase 3: World Generation & Chunk System - Implementation Plan

This document outlines the implementation plan for Phase 3 of the Minecraft Clone project, focusing on world generation, chunk-based rendering, and basic block placement/destruction.

## 🎯 Phase 3 Objectives

### Primary Goals

- [x] **Phase 2 Complete**: GLFW input, camera system, cube rendering, shaders
- [ ] **Chunk System**: Implement efficient 16x16x256 chunk management
- [ ] **World Generation**: Procedural terrain using FastNoise
- [ ] **Block System**: Multiple block types with textures
- [ ] **Frustum Culling**: Only render visible chunks for performance
- [ ] **Block Interaction**: Basic block placement and destruction
- [ ] **Infinite World**: Dynamic chunk loading/unloading

### Success Criteria

- Render multiple chunks with different block types
- Smooth world generation with realistic terrain
- Maintains 60+ FPS with multiple chunks
- Player can place and break blocks
- Memory efficient chunk management
- Foundation for multiplayer world synchronization

## 🏗️ Technical Architecture

### World System Overview

```
World Manager
├── ChunkManager      // Loading, unloading, and storage
├── WorldGenerator    // Procedural terrain generation
├── BlockRegistry     // Block type definitions and textures
├── RenderManager     // Frustum culling and batch rendering
└── InteractionSystem // Block placement/destruction
```

### Component Structure

```
src/
├── main.cpp                    # Entry point and main loop
├── world/
│   ├── world.h                 # World manager interface
│   ├── world.cpp               # World implementation
│   ├── chunk.h                 # Chunk data structure
│   ├── chunk.cpp               # Chunk management
│   ├── world_generator.h       # Terrain generation
│   ├── world_generator.cpp     # Noise-based generation
│   ├── block.h                 # Block definitions
│   └── block.cpp               # Block implementation
├── renderer/
│   ├── renderer.h              # Enhanced renderer interface
│   ├── renderer.cpp            # Batch rendering system
│   ├── chunk_renderer.h        # Chunk-specific rendering
│   ├── chunk_renderer.cpp      # Mesh generation
│   ├── texture_atlas.h         # Block texture management
│   ├── texture_atlas.cpp       # Texture atlas implementation
│   ├── shader.h                # Existing shader system
│   ├── shader.cpp              # Enhanced with instancing
│   ├── camera.h                # Existing camera (enhanced)
│   └── camera.cpp              # Frustum culling support
├── input/
│   ├── input.h                 # Enhanced input system
│   └── input.cpp               # Block interaction controls
└── utils/
    ├── math_utils.h            # Chunk coordinates and utilities
    ├── math_utils.cpp          # Implementation
    ├── noise.h                 # FastNoise wrapper
    └── noise.cpp               # Noise generation utilities
```

## 📋 Implementation Tasks

### Task 1: Block System Foundation

**Priority**: High | **Estimated Time**: 4-6 hours

#### Objectives

- Define block types and properties
- Create block texture atlas system
- Implement block registry for easy expansion
- Support different block materials and properties

#### Implementation Details

```cpp
enum class BlockType : uint8_t {
    AIR = 0,
    GRASS,
    DIRT,
    STONE,
    WATER,
    SAND,
    WOOD,
    LEAVES,
    COBBLESTONE,
    BEDROCK,
    COUNT
};

struct BlockData {
    BlockType type = BlockType::AIR;
    uint8_t metadata = 0; // For rotation, water level, etc.
};

class Block {
public:
    BlockType type;
    std::string name;
    bool isSolid;
    bool isTransparent;
    float hardness;
    glm::vec2 textureCoords[6]; // 6 faces

    Block(BlockType type, const std::string& name, bool solid = true);
    bool shouldRenderFace(BlockType neighborType) const;
    glm::vec2 getTextureCoords(int face) const;
};

class BlockRegistry {
public:
    static void initialize();
    static const Block& getBlock(BlockType type);
    static GLuint getTextureAtlas();

private:
    static std::array<Block, static_cast<size_t>(BlockType::COUNT)> blocks;
    static GLuint textureAtlasID;
};
```

#### Deliverables

- [ ] `src/world/block.h` - Block system interface
- [ ] `src/world/block.cpp` - Block implementation
- [ ] `src/renderer/texture_atlas.h` - Texture atlas management
- [ ] `src/renderer/texture_atlas.cpp` - Atlas implementation
- [ ] Block texture atlas (16x16 textures in 256x256 atlas)

### Task 2: Chunk Data Structure

**Priority**: High | **Estimated Time**: 5-7 hours

#### Objectives

- Implement efficient 16x16x256 chunk storage
- Support fast block access and modification
- Optimize memory layout for cache efficiency
- Handle chunk state management (loading, generating, meshing, etc.)

#### Implementation Details

```cpp
constexpr int CHUNK_WIDTH = 16;
constexpr int CHUNK_HEIGHT = 256;
constexpr int CHUNK_DEPTH = 16;
constexpr int BLOCKS_PER_CHUNK = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH;

struct ChunkCoord {
    int x, z;

    ChunkCoord(int x, int z) : x(x), z(z) {}
    bool operator==(const ChunkCoord& other) const;
    bool operator<(const ChunkCoord& other) const;
};

enum class ChunkState {
    EMPTY,
    GENERATING,
    GENERATED,
    MESHING,
    READY,
    UNLOADING
};

class Chunk {
public:
    Chunk(ChunkCoord coord);
    ~Chunk();

    // Block access
    BlockData getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockData block);

    // Mesh management
    void generateMesh();
    void render(const glm::mat4& view, const glm::mat4& projection);

    // State management
    ChunkState getState() const { return state; }
    void setState(ChunkState newState) { state = newState; }

    // Utilities
    bool isInBounds(int x, int y, int z) const;
    glm::vec3 getWorldPosition() const;

private:
    ChunkCoord coord;
    ChunkState state;
    std::array<BlockData, BLOCKS_PER_CHUNK> blocks;

    // Rendering data
    GLuint VAO, VBO;
    size_t vertexCount;
    bool meshDirty;

    // Mesh generation
    void addFace(std::vector<float>& vertices, const glm::vec3& pos,
                 int face, BlockType blockType);
    bool shouldRenderFace(int x, int y, int z, int face) const;
};
```

#### Deliverables

- [ ] `src/world/chunk.h` - Chunk data structure
- [ ] `src/world/chunk.cpp` - Chunk implementation
- [ ] `src/utils/math_utils.h` - Chunk coordinate utilities
- [ ] `src/utils/math_utils.cpp` - Math utilities implementation
- [ ] Unit tests for chunk operations

### Task 3: World Generator

**Priority**: High | **Estimated Time**: 6-8 hours

#### Objectives

- Implement procedural terrain generation using FastNoise
- Create realistic heightmaps with multiple biomes
- Generate caves and underground structures
- Support deterministic generation for multiplayer

#### Implementation Details

```cpp
struct BiomeData {
    BlockType surfaceBlock;
    BlockType subsurfaceBlock;
    BlockType stoneBlock;
    float heightMultiplier;
    float densityOffset;
    int treeChance; // 0-100
};

class WorldGenerator {
public:
    WorldGenerator(uint32_t seed);

    void generateChunk(Chunk* chunk);
    void generateTerrain(Chunk* chunk);
    void generateCaves(Chunk* chunk);
    void generateStructures(Chunk* chunk);

    // Noise functions
    float getHeightAt(int worldX, int worldZ);
    float getDensityAt(int worldX, int worldY, int worldZ);
    BiomeData getBiomeAt(int worldX, int worldZ);

private:
    uint32_t seed;
    FastNoiseLite terrainNoise;
    FastNoiseLite caveNoise;
    FastNoiseLite biomeNoise;

    // Generation parameters
    static constexpr float TERRAIN_SCALE = 0.01f;
    static constexpr float CAVE_SCALE = 0.05f;
    static constexpr float CAVE_THRESHOLD = 0.3f;
    static constexpr int SEA_LEVEL = 64;
    static constexpr int BEDROCK_LEVEL = 5;

    void generateTree(Chunk* chunk, int x, int y, int z);
    BlockType getBlockForHeight(int height, int surfaceHeight, BiomeData biome);
};
```

#### Deliverables

- [ ] `src/world/world_generator.h` - World generation interface
- [ ] `src/world/world_generator.cpp` - Generation implementation
- [ ] `src/utils/noise.h` - FastNoise wrapper
- [ ] `src/utils/noise.cpp` - Noise utilities
- [ ] Multiple biome configurations
- [ ] Cave generation system

### Task 4: Chunk Manager

**Priority**: High | **Estimated Time**: 7-9 hours

#### Objectives

- Manage loading and unloading of chunks around player
- Implement efficient chunk storage and retrieval
- Handle chunk generation in background threads
- Support dynamic render distance

#### Implementation Details

```cpp
class ChunkManager {
public:
    ChunkManager(WorldGenerator* generator);
    ~ChunkManager();

    // Chunk management
    Chunk* getChunk(ChunkCoord coord);
    void loadChunk(ChunkCoord coord);
    void unloadChunk(ChunkCoord coord);

    // Player-based loading
    void updateAroundPlayer(const glm::vec3& playerPos);
    void setRenderDistance(int distance) { renderDistance = distance; }

    // Block access across chunks
    BlockData getBlockAt(int worldX, int worldY, int worldZ);
    void setBlockAt(int worldX, int worldY, int worldZ, BlockData block);

    // Rendering
    void render(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraPos);

    // Threading
    void processGenerationQueue();
    void processMeshingQueue();

private:
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>> loadedChunks;
    std::queue<ChunkCoord> generationQueue;
    std::queue<Chunk*> meshingQueue;

    WorldGenerator* worldGenerator;
    int renderDistance = 8;

    // Threading
    std::thread generationThread;
    std::mutex chunkMutex;
    std::atomic<bool> shouldStop;

    // Utilities
    ChunkCoord worldToChunkCoord(int worldX, int worldZ);
    bool isChunkInRange(ChunkCoord coord, const glm::vec3& playerPos);
    void generateChunkAsync(ChunkCoord coord);
};
```

#### Deliverables

- [ ] `src/world/chunk_manager.h` - Chunk management interface
- [ ] `src/world/chunk_manager.cpp` - Manager implementation
- [ ] Background thread chunk generation
- [ ] Dynamic loading/unloading system
- [ ] Performance profiling tools

### Task 5: Enhanced Renderer

**Priority**: Medium | **Estimated Time**: 5-7 hours

#### Objectives

- Implement frustum culling for chunk rendering
- Add batch rendering for improved performance
- Support transparency and alpha blending
- Optimize draw calls with instancing

#### Implementation Details

```cpp
class ChunkRenderer {
public:
    ChunkRenderer();
    ~ChunkRenderer();

    void renderChunks(const std::vector<Chunk*>& chunks,
                      const glm::mat4& view, const glm::mat4& projection,
                      const glm::vec3& cameraPos);

    void renderChunk(Chunk* chunk, const glm::mat4& view,
                     const glm::mat4& projection);

    // Culling
    bool isChunkInFrustum(Chunk* chunk, const glm::mat4& viewProjection);

private:
    std::unique_ptr<Shader> chunkShader;
    GLuint textureAtlas;

    // Frustum culling planes
    std::array<glm::vec4, 6> frustumPlanes;
    void extractFrustumPlanes(const glm::mat4& viewProjection);
};

// Enhanced camera with frustum culling
class Camera {
    // ... existing methods ...

    // New methods for Phase 3
    std::array<glm::vec4, 6> getFrustumPlanes() const;
    bool isPointInFrustum(const glm::vec3& point) const;
    bool isAABBInFrustum(const glm::vec3& min, const glm::vec3& max) const;
};
```

#### Deliverables

- [ ] `src/renderer/chunk_renderer.h` - Chunk rendering system
- [ ] `src/renderer/chunk_renderer.cpp` - Rendering implementation
- [ ] Enhanced camera with frustum culling
- [ ] Batch rendering optimizations
- [ ] Performance measurement tools

### Task 6: Block Interaction System

**Priority**: Medium | **Estimated Time**: 4-6 hours

#### Objectives

- Implement block placement and destruction
- Add raycasting for block selection
- Support different tool types and block hardness
- Visual feedback for block targeting

#### Implementation Details

```cpp
struct RaycastHit {
    bool hit;
    glm::vec3 position;
    glm::vec3 normal;
    BlockData block;
    float distance;
};

class InteractionSystem {
public:
    InteractionSystem(ChunkManager* chunkManager);

    // Raycasting
    RaycastHit raycastBlocks(const glm::vec3& origin, const glm::vec3& direction,
                             float maxDistance = 5.0f);

    // Block interaction
    bool placeBlock(const glm::vec3& position, BlockType blockType);
    bool breakBlock(const glm::vec3& position);

    // Input handling
    void handleMouseClick(bool leftClick, const glm::vec3& cameraPos,
                          const glm::vec3& cameraFront);

    // Visual feedback
    void renderBlockOutline(const glm::vec3& blockPos, const glm::mat4& view,
                            const glm::mat4& projection);

private:
    ChunkManager* chunkManager;
    std::unique_ptr<Shader> outlineShader;
    GLuint outlineVAO, outlineVBO;

    BlockType selectedBlockType = BlockType::STONE;
    glm::vec3 targetedBlock = glm::vec3(-1); // Invalid position

    void updateTargetedBlock(const glm::vec3& cameraPos, const glm::vec3& cameraFront);
};
```

#### Deliverables

- [ ] `src/input/interaction.h` - Block interaction interface
- [ ] `src/input/interaction.cpp` - Interaction implementation
- [ ] Raycasting algorithm for block selection
- [ ] Visual block outline rendering
- [ ] Enhanced input handling for building

### Task 7: World Management System

**Priority**: Medium | **Estimated Time**: 3-5 hours

#### Objectives

- Coordinate all world systems (chunks, generation, rendering)
- Implement save/load functionality (basic)
- Handle world boundaries and edge cases
- Prepare foundation for multiplayer synchronization

#### Implementation Details

```cpp
class World {
public:
    World(uint32_t seed = 0);
    ~World();

    // Initialization
    bool initialize();
    void shutdown();

    // Update loop
    void update(float deltaTime, const glm::vec3& playerPos);
    void render(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraPos);

    // Block access
    BlockData getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, BlockData block);

    // Player interaction
    void handlePlayerInput(const glm::vec3& cameraPos, const glm::vec3& cameraFront,
                           bool leftClick, bool rightClick);

    // Configuration
    void setRenderDistance(int distance);
    int getRenderDistance() const;

private:
    std::unique_ptr<ChunkManager> chunkManager;
    std::unique_ptr<WorldGenerator> worldGenerator;
    std::unique_ptr<ChunkRenderer> chunkRenderer;
    std::unique_ptr<InteractionSystem> interactionSystem;

    uint32_t worldSeed;
    glm::vec3 lastPlayerPos;

    // Statistics
    int chunksLoaded = 0;
    int chunksRendered = 0;
    float updateTime = 0.0f;
    float renderTime = 0.0f;
};
```

#### Deliverables

- [ ] `src/world/world.h` - World management interface
- [ ] `src/world/world.cpp` - World implementation
- [ ] Integration with main game loop
- [ ] Basic save/load system (binary format)
- [ ] Performance monitoring and statistics

## 🎨 Shader Enhancements

### Chunk Vertex Shader (chunk.vert)

```glsl
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aTextureIndex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out float TextureIndex;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    TextureIndex = aTextureIndex;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
```

### Chunk Fragment Shader (chunk.frag)

```glsl
#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in float TextureIndex;

out vec4 FragColor;

uniform sampler2D textureAtlas;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float ambientStrength;

// Fog parameters
uniform float fogNear;
uniform float fogFar;
uniform vec3 fogColor;

void main()
{
    // Sample texture from atlas
    vec4 texColor = texture(textureAtlas, TexCoord);
    if (texColor.a < 0.1)
        discard;

    // Ambient lighting
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-lightDirection);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting (minimal for blocks)
    float specularStrength = 0.1;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8);
    vec3 specular = specularStrength * spec * lightColor;

    // Combine lighting
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;

    // Apply fog
    float distance = length(viewPos - FragPos);
    float fogFactor = clamp((fogFar - distance) / (fogFar - fogNear), 0.0, 1.0);
    result = mix(fogColor, result, fogFactor);

    FragColor = vec4(result, texColor.a);
}
```

### Block Outline Shader (outline.vert)

```glsl
#version 460 core

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
```

### Block Outline Fragment Shader (outline.frag)

```glsl
#version 460 core

out vec4 FragColor;

uniform vec3 outlineColor;
uniform float alpha;

void main()
{
    FragColor = vec4(outlineColor, alpha);
}
```

## 🔧 Build System Updates

### CMakeLists.txt Additions

```cmake
# Add new source directories
file(GLOB_RECURSE WORLD_SOURCES "src/world/*.cpp")
file(GLOB_RECURSE UTILS_SOURCES "src/utils/*.cpp")

# Update executable with new sources
add_executable(${PROJECT_NAME}
    src/main.cpp
    ${WORLD_SOURCES}
    ${UTILS_SOURCES}
    src/renderer/simple_shader.cpp
    src/renderer/camera.cpp
    src/renderer/shader.cpp
    src/renderer/buffer.cpp
    src/renderer/chunk_renderer.cpp
    src/renderer/texture_atlas.cpp
    src/input/interaction.cpp
)

# Copy asset directories to build folder
file(COPY textures DESTINATION ${CMAKE_BINARY_DIR})
file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})

# Add threading support
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
```

### Directory Structure

```
assets/
├── textures/
│   ├── blocks/
│   │   ├── grass_top.png
│   │   ├── grass_side.png
│   │   ├── dirt.png
│   │   ├── stone.png
│   │   ├── water.png
│   │   ├── sand.png
│   │   ├── wood.png
│   │   ├── leaves.png
│   │   ├── cobblestone.png
│   │   └── bedrock.png
│   └── atlas.png           # Generated texture atlas
└── shaders/
    ├── chunk.vert
    ├── chunk.frag
    ├── outline.vert
    └── outline.frag
```

## 🧪 Testing Strategy

### Unit Tests

- [ ] Block system functionality
- [ ] Chunk coordinate conversions
- [ ] Noise generation consistency
- [ ] Raycasting accuracy
- [ ] Memory management (no leaks)

### Integration Tests

- [ ] Chunk loading/unloading cycles
- [ ] World generation determinism
- [ ] Block placement/destruction
- [ ] Multi-chunk rendering
- [ ] Performance under load

### Performance Tests

- [ ] Chunk generation speed
- [ ] Mesh generation optimization
- [ ] Rendering performance (FPS)
- [ ] Memory usage with many chunks
- [ ] Threading efficiency

### Visual Tests

- [ ] Terrain looks realistic
- [ ] Block textures display correctly
- [ ] Chunk boundaries are seamless
- [ ] Lighting appears natural
- [ ] Block interactions feel responsive

## 📊 Performance Targets

### Frame Rate Goals

- **Target**: 60+ FPS with 8 chunk render distance
- **Minimum**: 30+ FPS with 16 chunk render distance
- **Method**: Frustum culling, batch rendering, LOD

### Memory Usage

- **Target**: <500MB with 16 chunk render distance
- **Monitoring**: Chunk memory profiling
- **Optimization**: Efficient chunk unloading

### Generation Speed

- **Target**: <100ms per chunk generation
- **Method**: Background threading
- **Optimization**: Optimized noise algorithms

### Render Performance

- **Target**: <8ms render time per frame
- **Method**: Batch rendering, frustum culling
- **Optimization**: Vertex buffer optimization

## 🚀 Phase 3 Milestones

### Milestone 1: Block System (Week 1)

- [ ] Block types and properties implemented
- [ ] Texture atlas system working
- [ ] Basic chunk data structure
- [ ] Simple world generation (flat terrain)

### Milestone 2: World Generation (Week 2)

- [ ] Procedural terrain with FastNoise
- [ ] Multiple biomes implemented
- [ ] Cave generation working
- [ ] Chunk management system

### Milestone 3: Rendering Optimization (Week 3)

- [ ] Frustum culling implemented
- [ ] Batch rendering optimized
- [ ] Multiple chunks rendering smoothly
- [ ] Performance targets met

### Milestone 4: Interaction System (Week 4)

- [ ] Block placement and destruction
- [ ] Raycasting and block selection
- [ ] Visual feedback for targeting
- [ ] Enhanced input handling

### Milestone 5: Polish & Integration (Week 5)

- [ ] Save/load functionality
- [ ] Performance optimization
- [ ] Bug fixes and stability
- [ ] Documentation updates
- [ ] Phase 4 planning

## 🔄 Transition to Phase 4

### Preparation Tasks

- [ ] Multiplayer architecture research
- [ ] Network protocol design
- [ ] Entity system planning
- [ ] Physics engine evaluation

### Technical Debt Review

- [ ] Code architecture optimization
- [ ] Memory management improvements
- [ ] Rendering pipeline refinement
- [ ] Threading model enhancement

### Future Features Planning

- [ ] Lighting system (dynamic lighting)
- [ ] Water simulation
- [ ] Redstone-like mechanics
- [ ] Advanced world generation (structures, villages)

## 📚 Learning Resources

### World Generation

- [Perlin Noise Tutorial](https://adrianb.io/2014/08/09/perlinnoise.html)
- [Minecraft World Generation](https://minecraft.fandom.com/wiki/World_generation)
- [FastNoise Documentation](https://github.com/Auburn/FastNoiseLite)

### Chunk Systems

- [Minecraft Technical Details](https://minecraft.fandom.com/wiki/Chunk)
- [Voxel Engine Development](https://sites.google.com/site/letsmakeavoxelengine/)
- [Chunk Loading Strategies](https://gamedev.stackexchange.com/questions/60630/what-are-some-good-strategies-for-chunk-loading-in-infinite-worlds)

### Performance Optimization

- [OpenGL Performance Tips](https://www.khronos.org/opengl/wiki/Performance)
- [3D Culling Techniques](https://learnopengl.com/Guest-Articles/2021/Scene/Frustum-Culling)
- [Batch Rendering](https://learnopengl.com/Advanced-OpenGL/Instancing)

### Threading

- [C++ Threading](https://en.cppreference.com/w/cpp/thread)
- [Producer-Consumer Pattern](https://en.wikipedia.org/wiki/Producer%E2%80%93consumer_problem)
- [Game Engine Threading](https://gameprogrammingpatterns.com/game-loop.html)

---

**Next Phase**: [Phase 4: Multiplayer & Networking](PHASE4.md) - Client-server architecture and network synchronization

**Current Status**: Phase 2 Complete ✅ | **Next Milestone**: Block System Foundation (Week 1)

This comprehensive plan builds upon your excellent Phase 2 foundation to create a fully functional voxel world with infinite terrain generation, efficient chunk management, and interactive block placement/destruction. The modular architecture ensures easy expansion into multiplayer functionality in Phase 4.
