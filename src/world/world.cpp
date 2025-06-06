#include "world.h"
#include "block.h"
#include "../renderer/simple_shader.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

World::World() : initialized(false), blockShader(nullptr) {
}

World::~World() {
    shutdown();
}

void World::initialize() {
    if (initialized) return;

    std::cout << "Initializing World..." << std::endl;

    // Initialize block registry
    BlockRegistry::initialize();

    // Initialize block shader
    blockShader = new SimpleShader("shaders/block.vert", "shaders/block.frag");

    // Create a single chunk at origin for Phase 4
    generateFlatChunk(ChunkCoord(0, 0));

    initialized = true;
    std::cout << "World initialized with flat terrain." << std::endl;
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
    blockShader->use();

    // Set matrices
    glm::mat4 model = glm::mat4(1.0f);
    blockShader->setMatrix4("model", model);
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

    // Render all chunks
    for (auto& pair : chunks) {
        if (pair.second && pair.second->isReady()) {
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
