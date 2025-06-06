#pragma once

#include "chunk.h"
#include "../renderer/simple_shader.h"
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>

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
    void setBlock(int x, int y, int z, BlockData block);

private:
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoord::Hash> chunks;
    bool initialized;
    SimpleShader* blockShader;

    // Simple flat world generation for Phase 4
    void generateSimpleTerrain(Chunk* chunk);
};
