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
    void generateFlatChunk(ChunkCoord coord);

    // Chunk management
    Chunk* getChunk(ChunkCoord coord);
    void addChunk(ChunkCoord coord, std::unique_ptr<Chunk> chunk);    // Rendering
    void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);// Block access (world coordinates)
    BlockData getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, BlockData block);    // PHASE 5: Dynamic chunk management
    void updateChunksAroundPlayer(const glm::vec3& playerPos);
    void loadChunk(ChunkCoord coord);
    void unloadChunk(ChunkCoord coord);
    bool isChunkLoaded(ChunkCoord coord) const;

    // Render distance management
    int getRenderDistance() const { return renderDistance; }
    void setRenderDistance(int distance);
    float getChunkUnloadDistance() const { return chunkUnloadDistance; }

    // Chunk statistics
    int getLoadedChunkCount() const { return static_cast<int>(chunks.size()); }

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
