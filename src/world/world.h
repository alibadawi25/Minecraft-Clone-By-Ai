#pragma once

#include "chunk.h"
#include "../renderer/simple_shader.h"
#include "../utils/math_utils.h"
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

class World {
public:
    World();
    ~World();

    void initialize();
    void shutdown();

    // Generate a simple flat world for Phase 4
    void generateFlatChunk(ChunkCoord coord);    // Chunk management
    Chunk* getChunk(ChunkCoord coord);
    const Chunk* getChunk(ChunkCoord coord) const;
    void addChunk(ChunkCoord coord, std::unique_ptr<Chunk> chunk);// Rendering
    void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);    // Block access (world coordinates)
    BlockData getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockData block);// PHASE 5: Dynamic chunk management
    void updateChunksAroundPlayer(const glm::vec3& playerPos);
    // Update dirty chunk meshes
    void updateDirtyChunks();
    void loadChunk(ChunkCoord coord);
    void unloadChunk(ChunkCoord coord);
    bool isChunkLoaded(ChunkCoord coord) const;

    // Render distance management
    int getRenderDistance() const { return renderDistance; }
    void setRenderDistance(int distance);
    float getChunkUnloadDistance() const { return chunkUnloadDistance; }

    // Chunk statistics
    int getLoadedChunkCount() const { return static_cast<int>(chunks.size()); }

    // PHASE 7: Player Interaction - Raycasting
    struct RaycastResult {
        bool hit;
        glm::ivec3 blockPos;      // Position of the hit block
        glm::ivec3 adjacentPos;   // Position adjacent to the hit block (for placing)
        glm::vec3 hitPoint;       // Exact hit point in world coordinates
        glm::vec3 normal;         // Face normal of the hit
        BlockData block;          // The block that was hit
        float distance;           // Distance from ray origin to hit
    };

    // Cast a ray from camera position in camera direction to find targeted block
    RaycastResult raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance = 10.0f) const;

private:    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoord::Hash> chunks;
    bool initialized;
    SimpleShader* blockShader;
    MathUtils::SimpleNoise noiseGenerator;  // For terrain generation    // World generation settings - now dynamic
    int renderDistance;
    float chunkUnloadDistance;
    static constexpr int DEFAULT_RENDER_DISTANCE = 12;  // Default render distance

    // Simple flat world generation for Phase 4
    void generateSimpleTerrain(Chunk* chunk);    // PHASE 5: Perlin noise terrain generation
    void generatePerlinTerrain(Chunk* chunk);    // Helper function to determine chunks to load around a position
    std::vector<ChunkCoord> getChunksAroundPosition(const glm::vec3& position) const;
};
