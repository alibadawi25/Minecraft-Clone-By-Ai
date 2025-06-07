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
    void addChunk(ChunkCoord coord, std::unique_ptr<Chunk> chunk);

    // Rendering
    void render(const glm::mat4& view, const glm::mat4& projection);

    // Block access (world coordinates)
    BlockData getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, BlockData block);    // PHASE 5: Dynamic chunk management
    void updateChunksAroundPlayer(const glm::vec3& playerPos);
    void loadChunk(ChunkCoord coord);
    void unloadChunk(ChunkCoord coord);
    bool isChunkLoaded(ChunkCoord coord) const;

private:    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoord::Hash> chunks;
    bool initialized;
    SimpleShader* blockShader;
    MathUtils::SimpleNoise noiseGenerator;  // For terrain generation    // World generation settings
    static constexpr int RENDER_DISTANCE = 3;  // 7x7 grid around player (-3 to +3)
    static constexpr float CHUNK_UNLOAD_DISTANCE = 6.0f;  // Distance in chunks to unload (must be > RENDER_DISTANCE * sqrt(2))

    // Simple flat world generation for Phase 4
    void generateSimpleTerrain(Chunk* chunk);

    // PHASE 5: Perlin noise terrain generation
    void generatePerlinTerrain(Chunk* chunk);

    // Helper function to determine chunks to load around a position
    std::vector<ChunkCoord> getChunksAroundPosition(const glm::vec3& position) const;
};
