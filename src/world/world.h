#pragma once

#include "chunk.h"
#include "../renderer/simple_shader.h"
#include "../utils/math_utils.h"
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>

#ifdef FASTNOISE_AVAILABLE
#include "FastNoiseLite.h"
#endif

// Terrain generation settings structure
struct TerrainSettings {
    float baseFrequency = 0.0015f;
    float mountainFrequency = 0.0045f;
    int maxTerrainHeight = 80;
    int waterLevel = 20;
    int horizontalRadius = 4;  // Chunk render distance
    unsigned int baseSeed = 1337;
    unsigned int mountainSeed = 2674;
};

// Global terrain settings instance
inline TerrainSettings gTerrainSettings;

class World {
public:
    World();
    ~World();

    void initialize();
    void shutdown();

    // World regeneration with new seed
    void regenerateWorld(unsigned int newSeed);

    // Reset static noise generators in chunks for new seed
    static void resetChunkNoiseGenerators();

    // Generate a simple flat world
    void generateFlatChunk(ChunkCoord coord);    // Chunk management
    Chunk* getChunk(ChunkCoord coord);
    const Chunk* getChunk(ChunkCoord coord) const;
    void addChunk(ChunkCoord coord, std::unique_ptr<Chunk> chunk);// Rendering
    void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);    // Block access (world coordinates)
    BlockData getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockData block);

    // Dynamic chunk management
    void updateChunksAroundPlayer(const glm::vec3& playerPos);
    // Update dirty chunk meshes
    void updateDirtyChunks();
    void loadChunk(ChunkCoord coord);
    void unloadChunk(ChunkCoord coord);
    bool isChunkLoaded(ChunkCoord coord) const;

    // Neighbor notification system
    void notifyNeighborsOfNewChunk(ChunkCoord newChunkCoord);

    // Render distance management
    int getRenderDistance() const { return renderDistance; }
    void setRenderDistance(int distance);
    float getChunkUnloadDistance() const { return chunkUnloadDistance; }    // Chunk statistics
    int getLoadedChunkCount() const { return static_cast<int>(chunks.size()); }

    // Rendering statistics for optimization debugging
    int getRenderedChunkCount() const { return lastRenderedChunks; }
    int getCulledChunkCount() const { return lastCulledChunks; }

    // Player Interaction - Raycasting
    struct RaycastResult {
        bool hit;
        glm::ivec3 blockPos;      // Position of the hit block
        glm::ivec3 adjacentPos;   // Position adjacent to the hit block (for placing)
        glm::vec3 hitPoint;       // Exact hit point in world coordinates
        glm::vec3 normal;         // Face normal of the hit
        BlockData block;          // The block that was hit
        float distance;           // Distance from ray origin to hit
    };    // Cast a ray from camera position in camera direction to find targeted block
    RaycastResult raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance = 10.0f) const;

    // Block highlighting
    void renderBlockHighlight(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);
    void setTargetedBlock(const glm::ivec3& blockPos);
    void clearTargetedBlock();
    bool hasTargetedBlock() const { return targetedBlockValid; }

private:    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoord::Hash> chunks;
    bool initialized;    // Performance constants
    static constexpr int DEFAULT_RENDER_DISTANCE = 12;
    static constexpr float CHUNK_UNLOAD_MULTIPLIER = 1.5f;

    // Helper function to get max meshes per frame based on render distance
    int getMaxMeshesPerFrame() const { return std::max(1, renderDistance / 4); }

    // Terrain generation constants
    static constexpr int BASE_HEIGHT = 64;
    static constexpr int WATER_LEVEL = 20;
    static constexpr int STONE_DEPTH = 5;

    // Rendering components
    SimpleShader* blockShader;
    SimpleShader* highlightShader;
    GLuint highlightVAO, highlightVBO;

    // Block highlighting
    glm::ivec3 targetedBlockPos;
    bool targetedBlockValid;    // World generation
    MathUtils::SimpleNoise noiseGenerator;

#ifdef FASTNOISE_AVAILABLE
    // Advanced mountain generation with FastNoise
    FastNoiseLite mountainNoise;       // Large scale mountains
    FastNoiseLite ridgeNoise;          // Sharp mountain ridges
    FastNoiseLite detailNoise;         // Fine surface details
    FastNoiseLite domainWarpNoise;     // Domain warping for natural terrain
#endif

    // Optimization systems
    mutable MathUtils::Frustum viewFrustum;
    mutable int lastRenderedChunks = 0;
    mutable int lastCulledChunks = 0;

    // World settings
    int renderDistance;
    float chunkUnloadDistance;    // Terrain generation methods
    void generateSimpleTerrain(Chunk* chunk);
    void generatePerlinTerrain(Chunk* chunk);
    std::vector<ChunkCoord> getChunksAroundPosition(const glm::vec3& position) const;

#ifdef FASTNOISE_AVAILABLE
    // Mountain generation setup
    void setupMountainGeneration(unsigned int seed);
    float getTerrainHeight(float worldX, float worldZ) const;
#endif// Block highlighting helper
    void initializeHighlightGeometry();
};
