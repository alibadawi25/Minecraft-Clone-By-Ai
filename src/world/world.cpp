#include "world.h"
#include "chunk.h"
#include "block.h"
#include "../renderer/simple_shader.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

World::World() : initialized(false), blockShader(nullptr), noiseGenerator(1337) {
    // SimpleNoise doesn't need configuration like FastNoiseLite
}

World::~World() {
    shutdown();
}

void World::initialize() {
    if (initialized) return;

    std::cout << "Initializing World..." << std::endl;

    // Initialize block registry
    BlockRegistry::initialize();    // Initialize block shader
    blockShader = new SimpleShader("shaders/block.vert", "shaders/block.frag");

    // PHASE 5: Start with no chunks - they will be loaded around the player
    // Remove the single flat chunk generation from Phase 4

    initialized = true;
    std::cout << "World initialized - chunks will be loaded dynamically around player." << std::endl;
}

void World::shutdown() {
    if (!initialized) return;

    chunks.clear();

    if (blockShader) {
        delete blockShader;
        blockShader = nullptr;
    }

    BlockRegistry::shutdown();

    initialized = false;
    std::cout << "World shutdown complete." << std::endl;
}

void World::generateFlatChunk(ChunkCoord coord) {
    auto chunk = std::make_unique<Chunk>(coord);

    // Generate simple flat terrain
    generateSimpleTerrain(chunk.get());

    // Set state to generated so mesh generation can proceed
    chunk->setState(ChunkState::GENERATED);

    // Generate the mesh
    chunk->generateMesh();

    addChunk(coord, std::move(chunk));
}

Chunk* World::getChunk(ChunkCoord coord) {
    auto it = chunks.find(coord);
    if (it != chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

void World::addChunk(ChunkCoord coord, std::unique_ptr<Chunk> chunk) {
    chunks[coord] = std::move(chunk);
}

void World::render(const glm::mat4& view, const glm::mat4& projection) {
    if (!blockShader) return;

    // Use block shader
    blockShader->use();    // Set view and projection matrices (common for all chunks)
    blockShader->setMatrix4("view", view);
    blockShader->setMatrix4("projection", projection);

    // Set lighting uniforms
    blockShader->setVector3("lightDirection", glm::vec3(0.3f, -1.0f, 0.3f));
    blockShader->setVector3("lightColor", glm::vec3(1.0f, 1.0f, 0.9f));
    blockShader->setVector3("ambientColor", glm::vec3(0.3f, 0.3f, 0.4f));

    // Bind texture atlas
    GLuint textureAtlas = BlockRegistry::getTextureAtlas();
    if (textureAtlas != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureAtlas);
        blockShader->setInt("blockTexture", 0);
    }

    // Render all chunks with proper positioning
    for (auto& pair : chunks) {
        if (pair.second && pair.second->isReady()) {
            // Calculate model matrix for this chunk's world position
            glm::vec3 chunkWorldPos = ChunkUtils::chunkToWorldPos(pair.first);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), chunkWorldPos);
            blockShader->setMatrix4("model", model);

            pair.second->render(view, projection);
        }
    }
}

BlockData World::getBlock(int x, int y, int z) {
    ChunkCoord chunkCoord = ChunkUtils::worldToChunkCoord(x, z);
    Chunk* chunk = getChunk(chunkCoord);

    if (chunk) {
        return chunk->getBlockWorld(x, y, z);
    }

    return BlockData(BlockType::AIR);
}

void World::setBlock(int x, int y, int z, BlockData block) {
    ChunkCoord chunkCoord = ChunkUtils::worldToChunkCoord(x, z);
    Chunk* chunk = getChunk(chunkCoord);

    if (chunk) {
        chunk->setBlockWorld(x, y, z, block);
        chunk->markForRemesh();
    }
}

void World::generateSimpleTerrain(Chunk* chunk) {
    if (!chunk) return;

    // Create simple flat world for Phase 4
    // Ground level at y=64, everything below is dirt, everything above is air
    const int groundLevel = 64;
    const int bedrockLevel = 0;

    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_DEPTH; ++z) {
            for (int y = 0; y < CHUNK_HEIGHT; ++y) {
                BlockType blockType = BlockType::AIR;

                if (y == bedrockLevel) {
                    blockType = BlockType::BEDROCK;
                } else if (y < groundLevel) {
                    blockType = BlockType::DIRT;
                                } else if (y == groundLevel) {
                    blockType = BlockType::GRASS;
                }
                // Everything above ground level remains air

                chunk->setBlock(x, y, z, BlockData(blockType));
            }
        }
    }
}

void World::updateChunksAroundPlayer(const glm::vec3& playerPos) {
    // std::cout << playerPos.x << std::endl;
    // Get player's chunk coordinate
    ChunkCoord playerChunk = ChunkUtils::worldToChunkCoord(playerPos);

    // Get list of chunks that should be loaded around player
    std::vector<ChunkCoord> chunksToLoad = getChunksAroundPosition(playerPos);

    // Load missing chunks
    int chunksLoaded = 0;
    for (const ChunkCoord& coord : chunksToLoad) {
        if (!isChunkLoaded(coord)) {
            loadChunk(coord);
            chunksLoaded++;
        }
    }

    // Unload distant chunks
    std::vector<ChunkCoord> chunksToUnload;

    for (const auto& pair : chunks) {
        ChunkCoord chunkCoord = pair.first;
        float chunkDistance = ChunkUtils::chunkDistance(chunkCoord, playerChunk);

        if (chunkDistance > CHUNK_UNLOAD_DISTANCE) {
            chunksToUnload.push_back(chunkCoord);
        }
    }

    int chunksUnloaded = 0;
    for (const ChunkCoord& coord : chunksToUnload) {
        unloadChunk(coord);
        chunksUnloaded++;
    }

    // Optional: Print statistics when chunks are loaded/unloaded
    if (chunksLoaded > 0 || chunksUnloaded > 0) {
        std::cout << "Chunks loaded: " << chunksLoaded << ", unloaded: " << chunksUnloaded
                  << ", total: " << chunks.size() << std::endl;
    }
}

void World::loadChunk(ChunkCoord coord) {
    if (isChunkLoaded(coord)) {
        return;  // Chunk already loaded
    }

    std::cout << "Loading chunk at (" << coord.x << ", " << coord.z << ")" << std::endl;

    auto chunk = std::make_unique<Chunk>(coord);

    // Generate terrain using Perlin noise
    generatePerlinTerrain(chunk.get());

    // Set state to generated so mesh generation can proceed
    chunk->setState(ChunkState::GENERATED);

    // Generate the mesh
    chunk->generateMesh();

    addChunk(coord, std::move(chunk));
}

void World::unloadChunk(ChunkCoord coord) {
    auto it = chunks.find(coord);
    if (it != chunks.end()) {
        std::cout << "Unloading chunk at (" << coord.x << ", " << coord.z << ")" << std::endl;
        chunks.erase(it);
    }
}

bool World::isChunkLoaded(ChunkCoord coord) const {
    return chunks.find(coord) != chunks.end();
}

std::vector<ChunkCoord> World::getChunksAroundPosition(const glm::vec3& position) const {
    std::vector<ChunkCoord> result;

    ChunkCoord centerChunk = ChunkUtils::worldToChunkCoord(position);

    // Generate chunks in a square grid around player, but check distance to avoid loading corner chunks that are too far
    for (int x = centerChunk.x - RENDER_DISTANCE; x <= centerChunk.x + RENDER_DISTANCE; ++x) {
        for (int z = centerChunk.z - RENDER_DISTANCE; z <= centerChunk.z + RENDER_DISTANCE; ++z) {
            ChunkCoord coord(x, z);
            float chunkDistance = ChunkUtils::chunkDistance(coord, centerChunk);

            // Only load chunks within the render distance (with a small buffer for square grid)
            if (chunkDistance <= RENDER_DISTANCE + 0.5f) {
                result.emplace_back(x, z);
            }
        }
    }

    return result;
}

void World::generatePerlinTerrain(Chunk* chunk) {
    if (!chunk) return;

    ChunkCoord coord = chunk->getCoord();

    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_DEPTH; ++z) {
            // Convert to world coordinates
            int worldX = coord.x * CHUNK_WIDTH + x;
            int worldZ = coord.z * CHUNK_DEPTH + z;

            // Get height from noise with multiple octaves for more interesting terrain
            float continentalness = noiseGenerator.fractalNoise2D(worldX * 0.001f, worldZ * 0.001f, 4, 0.5f);
            float heightNoise = noiseGenerator.fractalNoise2D(worldX * 0.01f, worldZ * 0.01f, 4, 0.5f);
            float detailNoise = noiseGenerator.fractalNoise2D(worldX * 0.05f, worldZ * 0.05f, 2, 0.5f);

            // Combine noise values to create varied terrain
            float combinedNoise = continentalness * 0.7f + heightNoise * 0.2f + detailNoise * 0.1f;

            // Calculate terrain height
            int baseHeight = 64;
            int terrainHeight = static_cast<int>(baseHeight + combinedNoise * 40);

            // Clamp height to valid range
            terrainHeight = std::max(5, std::min(terrainHeight, CHUNK_HEIGHT - 10));

            // Generate column of blocks
            for (int y = 0; y < CHUNK_HEIGHT; ++y) {
                BlockType blockType = BlockType::AIR;

                if (y == 0) {
                    // Bedrock layer
                    blockType = BlockType::BEDROCK;
                } else if (y < terrainHeight - 5) {
                    // Stone layer
                    blockType = BlockType::STONE;
                } else if (y < terrainHeight - 1) {
                    // Dirt layer
                    blockType = BlockType::DIRT;
                } else if (y <= terrainHeight) {
                    // Surface layer - grass above sea level, sand below
                    if (terrainHeight > 62) {
                        blockType = BlockType::GRASS;
                    } else {
                        blockType = BlockType::SAND;
                    }
                }
                // Water level at y=62
                else if (y <= 62 && terrainHeight < 62) {
                    blockType = BlockType::WATER;
                }

                if (blockType != BlockType::AIR) {
                    chunk->setBlock(x, y, z, BlockData(blockType));
                }
            }
        }
    }
}
