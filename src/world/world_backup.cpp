#include "world.h"
#include "chunk.h"
#include "block.h"
#include "../renderer/simple_shader.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

World::World() : initialized(false), blockShader(nullptr), noiseGenerator(1337),
                  renderDistance(DEFAULT_RENDER_DISTANCE) {
    // SimpleNoise doesn't need configuration like FastNoiseLite
    chunkUnloadDistance = renderDistance * 1.5f + 1.0f;
}

World::~World() {
    shutdown();
}

void World::initialize() {
    if (initialized) return;

    // Initialize block registry
    BlockRegistry::initialize();// Initialize block shader
    blockShader = new SimpleShader("shaders/block.vert", "shaders/block.frag");

    // PHASE 5: Start with no chunks - they will be loaded around the player
    // Remove the single flat chunk generation from Phase 4    initialized = true;
}

void World::shutdown() {
    if (!initialized) return;

    chunks.clear();

    if (blockShader) {
        delete blockShader;
        blockShader = nullptr;
    }    BlockRegistry::shutdown();

    initialized = false;
}

void World::generateFlatChunk(ChunkCoord coord) {
    auto chunk = std::make_unique<Chunk>(coord, this);

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

const Chunk* World::getChunk(ChunkCoord coord) const {
    auto it = chunks.find(coord);
    if (it != chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

void World::addChunk(ChunkCoord coord, std::unique_ptr<Chunk> chunk) {
    chunks[coord] = std::move(chunk);
}

void World::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
    if (!blockShader) return;

    // Use block shader
    blockShader->use();

    // Set view and projection matrices (common for all chunks)
    blockShader->setMatrix4("view", view);
    blockShader->setMatrix4("projection", projection);

    // Set lighting uniforms - balanced daylight setup
    blockShader->setVector3("lightDirection", glm::vec3(0.2f, -0.8f, 0.1f));  // More overhead sun
    blockShader->setVector3("lightColor", glm::vec3(0.8f, 0.8f, 0.7f));        // Softer, warm sunlight
    blockShader->setVector3("ambientColor", glm::vec3(0.3f, 0.3f, 0.4f));      // Moderate ambient light

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

            pair.second->render(view, projection, cameraPos);
        }
    }
}

BlockData World::getBlock(int x, int y, int z) const {
    ChunkCoord chunkCoord = ChunkUtils::worldToChunkCoord(x, z);
    const Chunk* chunk = getChunk(chunkCoord);

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
    }    // Unload distant chunks
    std::vector<ChunkCoord> chunksToUnload;    for (const auto& pair : chunks) {
        ChunkCoord chunkCoord = pair.first;        // Use actual player position for more accurate distance calculation
        float chunkDistance = ChunkUtils::chunkDistanceToPoint(chunkCoord, playerPos);

        if (chunkDistance > chunkUnloadDistance * CHUNK_WIDTH) {
            chunksToUnload.push_back(chunkCoord);
        }
    }

    int chunksUnloaded = 0;
    for (const ChunkCoord& coord : chunksToUnload) {
        unloadChunk(coord);
        chunksUnloaded++;    }
}

void World::loadChunk(ChunkCoord coord) {    if (isChunkLoaded(coord)) {
        return;  // Chunk already loaded
    }

    auto chunk = std::make_unique<Chunk>(coord, this);

    // Generate terrain using Perlin noise
    generatePerlinTerrain(chunk.get());

    // Set state to generated so mesh generation can proceed
    chunk->setState(ChunkState::GENERATED);    // Add chunk to the world first
    addChunk(coord, std::move(chunk));

    // Generate the mesh
    Chunk* loadedChunk = getChunk(coord);
    if (loadedChunk) {
        loadedChunk->generateMesh();
    }
}

void World::unloadChunk(ChunkCoord coord) {
    auto it = chunks.find(coord);
    if (it != chunks.end()) {
        chunks.erase(it);
    }
}

bool World::isChunkLoaded(ChunkCoord coord) const {
    return chunks.find(coord) != chunks.end();
}

std::vector<ChunkCoord> World::getChunksAroundPosition(const glm::vec3& position) const {
    std::vector<ChunkCoord> result;    ChunkCoord centerChunk = ChunkUtils::worldToChunkCoord(position);

    // Generate chunks in a square grid around player, but check distance to avoid loading corner chunks that are too far
    for (int x = centerChunk.x - renderDistance; x <= centerChunk.x + renderDistance; ++x) {
        for (int z = centerChunk.z - renderDistance; z <= centerChunk.z + renderDistance; ++z) {
            ChunkCoord coord(x, z);
            // Use actual player position for more accurate distance calculation
            float chunkDistance = ChunkUtils::chunkDistanceToPoint(coord, position);

            // Only load chunks within the render distance (convert to world units)
            if (chunkDistance <= renderDistance * CHUNK_WIDTH + (CHUNK_WIDTH / 2.0f)) {
                result.emplace_back(x, z);
            }
        }
    }

    return result;
}

// This incorrect void raycast method is removed - the correct one is below

void World::updateDirtyChunks() {
    // Check all loaded chunks for those that need mesh regeneration
    for (auto& pair : chunks) {
        Chunk* chunk = pair.second.get();
        if (chunk && chunk->needsRemeshing() && chunk->getState() == ChunkState::GENERATED) {
            // Regenerate mesh for this chunk
            chunk->generateMesh();
        }
    }
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

void World::setRenderDistance(int distance) {
    // Clamp render distance to reasonable values
    renderDistance = std::max(2, std::min(32, distance));
    chunkUnloadDistance = renderDistance * 1.5f + 1.0f;
}

// PHASE 7: Player Interaction - Raycasting Implementation
World::RaycastResult World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
    RaycastResult result;
    result.hit = false;
    result.distance = maxDistance;

    // Normalize direction vector
    glm::vec3 rayDir = glm::normalize(direction);

    // DDA (Digital Differential Analyzer) algorithm for voxel traversal
    // Current position in the ray
    glm::vec3 rayPos = origin;

    // Step direction for each axis
    glm::ivec3 stepDir = glm::ivec3(
        rayDir.x > 0 ? 1 : -1,
        rayDir.y > 0 ? 1 : -1,
        rayDir.z > 0 ? 1 : -1
    );

    // Current voxel position
    glm::ivec3 voxelPos = glm::ivec3(glm::floor(rayPos));

    // Calculate delta distances (distance to cross one voxel in each direction)
    glm::vec3 deltaDist = glm::vec3(
        rayDir.x == 0 ? 1e30f : std::abs(1.0f / rayDir.x),
        rayDir.y == 0 ? 1e30f : std::abs(1.0f / rayDir.y),
        rayDir.z == 0 ? 1e30f : std::abs(1.0f / rayDir.z)
    );

    // Calculate initial side distances
    glm::vec3 sideDist;
    if (rayDir.x < 0) {
        sideDist.x = (rayPos.x - voxelPos.x) * deltaDist.x;
    } else {
        sideDist.x = (voxelPos.x + 1.0f - rayPos.x) * deltaDist.x;
    }
    if (rayDir.y < 0) {
        sideDist.y = (rayPos.y - voxelPos.y) * deltaDist.y;
    } else {
        sideDist.y = (voxelPos.y + 1.0f - rayPos.y) * deltaDist.y;
    }
    if (rayDir.z < 0) {
        sideDist.z = (rayPos.z - voxelPos.z) * deltaDist.z;
    } else {
        sideDist.z = (voxelPos.z + 1.0f - rayPos.z) * deltaDist.z;
    }

    // Track which side was hit (for calculating normals)
    int hitSide = 0; // 0=x, 1=y, 2=z

    // Previous voxel position (for placing blocks)
    glm::ivec3 prevVoxelPos = voxelPos;

    // Step through voxels
    float currentDistance = 0.0f;
    while (currentDistance < maxDistance) {
        // Check if current voxel contains a solid block
        BlockData block = getBlock(voxelPos.x, voxelPos.y, voxelPos.z);
        if (block.type != BlockType::AIR) {
            // Hit a solid block
            result.hit = true;
            result.blockPos = voxelPos;
            result.adjacentPos = prevVoxelPos;
            result.block = block;
            result.distance = currentDistance;

            // Calculate hit point
            result.hitPoint = origin + rayDir * currentDistance;

            // Calculate face normal based on which side was hit
            result.normal = glm::vec3(0.0f);
            if (hitSide == 0) { // X side
                result.normal.x = stepDir.x > 0 ? -1.0f : 1.0f;
            } else if (hitSide == 1) { // Y side
                result.normal.y = stepDir.y > 0 ? -1.0f : 1.0f;
            } else { // Z side
                result.normal.z = stepDir.z > 0 ? -1.0f : 1.0f;
            }

            break;
        }

        // Store previous position for block placement
        prevVoxelPos = voxelPos;

        // Move to next voxel
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            // Step in X direction
            sideDist.x += deltaDist.x;
            voxelPos.x += stepDir.x;
            currentDistance = sideDist.x - deltaDist.x;
            hitSide = 0;
        } else if (sideDist.y < sideDist.z) {
            // Step in Y direction
            sideDist.y += deltaDist.y;
            voxelPos.y += stepDir.y;
            currentDistance = sideDist.y - deltaDist.y;
            hitSide = 1;
        } else {
            // Step in Z direction
            sideDist.z += deltaDist.z;
            voxelPos.z += stepDir.z;
            currentDistance = sideDist.z - deltaDist.z;
            hitSide = 2;
        }
    }

    return result;
}

void World::updateDirtyChunks() {
    // Check all loaded chunks for those that need mesh regeneration
    for (auto& pair : chunks) {
        Chunk* chunk = pair.second.get();
        if (chunk && chunk->needsRemeshing() && chunk->getState() == ChunkState::GENERATED) {
            // Regenerate mesh for this chunk
            chunk->generateMesh();
        }
    }
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

void World::setRenderDistance(int distance) {
    // Clamp render distance to reasonable values
    renderDistance = std::max(2, std::min(32, distance));
    chunkUnloadDistance = renderDistance * 1.5f + 1.0f;
}

// PHASE 7: Player Interaction - Raycasting Implementation
World::RaycastResult World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
    RaycastResult result;
    result.hit = false;
    result.distance = maxDistance;

    // Normalize direction vector
    glm::vec3 rayDir = glm::normalize(direction);

    // DDA (Digital Differential Analyzer) algorithm for voxel traversal
    // Current position in the ray
    glm::vec3 rayPos = origin;

    // Step direction for each axis
    glm::ivec3 stepDir = glm::ivec3(
        rayDir.x > 0 ? 1 : -1,
        rayDir.y > 0 ? 1 : -1,
        rayDir.z > 0 ? 1 : -1
    );

    // Current voxel position
    glm::ivec3 voxelPos = glm::ivec3(glm::floor(rayPos));

    // Calculate delta distances (distance to cross one voxel in each direction)
    glm::vec3 deltaDist = glm::vec3(
        rayDir.x == 0 ? 1e30f : std::abs(1.0f / rayDir.x),
        rayDir.y == 0 ? 1e30f : std::abs(1.0f / rayDir.y),
        rayDir.z == 0 ? 1e30f : std::abs(1.0f / rayDir.z)
    );

    // Calculate initial side distances
    glm::vec3 sideDist;
    if (rayDir.x < 0) {
        sideDist.x = (rayPos.x - voxelPos.x) * deltaDist.x;
    } else {
        sideDist.x = (voxelPos.x + 1.0f - rayPos.x) * deltaDist.x;
    }
    if (rayDir.y < 0) {
        sideDist.y = (rayPos.y - voxelPos.y) * deltaDist.y;
    } else {
        sideDist.y = (voxelPos.y + 1.0f - rayPos.y) * deltaDist.y;
    }
    if (rayDir.z < 0) {
        sideDist.z = (rayPos.z - voxelPos.z) * deltaDist.z;
    } else {
        sideDist.z = (voxelPos.z + 1.0f - rayPos.z) * deltaDist.z;
    }

    // Track which side was hit (for calculating normals)
    int hitSide = 0; // 0=x, 1=y, 2=z

    // Previous voxel position (for placing blocks)
    glm::ivec3 prevVoxelPos = voxelPos;

    // Step through voxels
    float currentDistance = 0.0f;
    while (currentDistance < maxDistance) {
        // Check if current voxel contains a solid block
        BlockData block = getBlock(voxelPos.x, voxelPos.y, voxelPos.z);
        if (block.type != BlockType::AIR) {
            // Hit a solid block
            result.hit = true;
            result.blockPos = voxelPos;
            result.adjacentPos = prevVoxelPos;
            result.block = block;
            result.distance = currentDistance;

            // Calculate hit point
            result.hitPoint = origin + rayDir * currentDistance;

            // Calculate face normal based on which side was hit
            result.normal = glm::vec3(0.0f);
            if (hitSide == 0) { // X side
                result.normal.x = stepDir.x > 0 ? -1.0f : 1.0f;
            } else if (hitSide == 1) { // Y side
                result.normal.y = stepDir.y > 0 ? -1.0f : 1.0f;
            } else { // Z side
                result.normal.z = stepDir.z > 0 ? -1.0f : 1.0f;
            }

            break;
        }

        // Store previous position for block placement
        prevVoxelPos = voxelPos;

        // Move to next voxel
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            // Step in X direction
            sideDist.x += deltaDist.x;
            voxelPos.x += stepDir.x;
            currentDistance = sideDist.x - deltaDist.x;
            hitSide = 0;
        } else if (sideDist.y < sideDist.z) {
            // Step in Y direction
            sideDist.y += deltaDist.y;
            voxelPos.y += stepDir.y;
            currentDistance = sideDist.y - deltaDist.y;
            hitSide = 1;
        } else {
            // Step in Z direction
            sideDist.z += deltaDist.z;
            voxelPos.z += stepDir.z;
            currentDistance = sideDist.z - deltaDist.z;
            hitSide = 2;
        }
    }

    return result;
}

void World::updateDirtyChunks() {
    // Check all loaded chunks for those that need mesh regeneration
    for (auto& pair : chunks) {
        Chunk* chunk = pair.second.get();
        if (chunk && chunk->needsRemeshing() && chunk->getState() == ChunkState::GENERATED) {
            // Regenerate mesh for this chunk
            chunk->generateMesh();
        }
    }
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

void World::setRenderDistance(int distance) {
    // Clamp render distance to reasonable values
    renderDistance = std::max(2, std::min(32, distance));
    chunkUnloadDistance = renderDistance * 1.5f + 1.0f;
}

// PHASE 7: Player Interaction - Raycasting Implementation
World::RaycastResult World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
    RaycastResult result;
    result.hit = false;
    result.distance = maxDistance;

    // Normalize direction vector
    glm::vec3 rayDir = glm::normalize(direction);

    // DDA (Digital Differential Analyzer) algorithm for voxel traversal
    // Current position in the ray
    glm::vec3 rayPos = origin;

    // Step direction for each axis
    glm::ivec3 stepDir = glm::ivec3(
        rayDir.x > 0 ? 1 : -1,
        rayDir.y > 0 ? 1 : -1,
        rayDir.z > 0 ? 1 : -1
    );

    // Current voxel position
    glm::ivec3 voxelPos = glm::ivec3(glm::floor(rayPos));

    // Calculate delta distances (distance to cross one voxel in each direction)
    glm::vec3 deltaDist = glm::vec3(
        rayDir.x == 0 ? 1e30f : std::abs(1.0f / rayDir.x),
        rayDir.y == 0 ? 1e30f : std::abs(1.0f / rayDir.y),
        rayDir.z == 0 ? 1e30f : std::abs(1.0f / rayDir.z)
    );

    // Calculate initial side distances
    glm::vec3 sideDist;
    if (rayDir.x < 0) {
        sideDist.x = (rayPos.x - voxelPos.x) * deltaDist.x;
    } else {
        sideDist.x = (voxelPos.x + 1.0f - rayPos.x) * deltaDist.x;
    }
    if (rayDir.y < 0) {
        sideDist.y = (rayPos.y - voxelPos.y) * deltaDist.y;
    } else {
        sideDist.y = (voxelPos.y + 1.0f - rayPos.y) * deltaDist.y;
    }
    if (rayDir.z < 0) {
        sideDist.z = (rayPos.z - voxelPos.z) * deltaDist.z;
    } else {
        sideDist.z = (voxelPos.z + 1.0f - rayPos.z) * deltaDist.z;
    }

    // Track which side was hit (for calculating normals)
    int hitSide = 0; // 0=x, 1=y, 2=z

    // Previous voxel position (for placing blocks)
    glm::ivec3 prevVoxelPos = voxelPos;

    // Step through voxels
    float currentDistance = 0.0f;
    while (currentDistance < maxDistance) {
        // Check if current voxel contains a solid block
        BlockData block = getBlock(voxelPos.x, voxelPos.y, voxelPos.z);
        if (block.type != BlockType::AIR) {
            // Hit a solid block
            result.hit = true;
            result.blockPos = voxelPos;
            result.adjacentPos = prevVoxelPos;
            result.block = block;
            result.distance = currentDistance;

            // Calculate hit point
            result.hitPoint = origin + rayDir * currentDistance;

            // Calculate face normal based on which side was hit
            result.normal = glm::vec3(0.0f);
            if (hitSide == 0) { // X side
                result.normal.x = stepDir.x > 0 ? -1.0f : 1.0f;
            } else if (hitSide == 1) { // Y side
                result.normal.y = stepDir.y > 0 ? -1.0f : 1.0f;
            } else { // Z side
                result.normal.z = stepDir.z > 0 ? -1.0f : 1.0f;
            }

            break;
        }

        // Store previous position for block placement
        prevVoxelPos = voxelPos;

        // Move to next voxel
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            // Step in X direction
            sideDist.x += deltaDist.x;
            voxelPos.x += stepDir.x;
            currentDistance = sideDist.x - deltaDist.x;
            hitSide = 0;
        } else if (sideDist.y < sideDist.z) {
            // Step in Y direction
            sideDist.y += deltaDist.y;
            voxelPos.y += stepDir.y;
            currentDistance = sideDist.y - deltaDist.y;
            hitSide = 1;
        } else {
            // Step in Z direction
            sideDist.z += deltaDist.z;
            voxelPos.z += stepDir.z;
            currentDistance = sideDist.z - deltaDist.z;
            hitSide = 2;
        }
    }

    return result;
}

void World::updateDirtyChunks() {
    // Check all loaded chunks for those that need mesh regeneration
    for (auto& pair : chunks) {
        Chunk* chunk = pair.second.get();
        if (chunk && chunk->needsRemeshing() && chunk->getState() == ChunkState::GENERATED) {
            // Regenerate mesh for this chunk
            chunk->generateMesh();
        }
    }
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

void World::setRenderDistance(int distance) {
    // Clamp render distance to reasonable values
    renderDistance = std::max(2, std::min(32, distance));
    chunkUnloadDistance = renderDistance * 1.5f + 1.0f;
}

// PHASE 7: Player Interaction - Raycasting Implementation
World::RaycastResult World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
    RaycastResult result;
    result.hit = false;
    result.distance = maxDistance;

    // Normalize direction vector
    glm::vec3 rayDir = glm::normalize(direction);

    // DDA (Digital Differential Analyzer) algorithm for voxel traversal
    // Current position in the ray
    glm::vec3 rayPos = origin;

    // Step direction for each axis
    glm::ivec3 stepDir = glm::ivec3(
        rayDir.x > 0 ? 1 : -1,
        rayDir.y > 0 ? 1 : -1,
        rayDir.z > 0 ? 1 : -1
    );

    // Current voxel position
    glm::ivec3 voxelPos = glm::ivec3(glm::floor(rayPos));

    // Calculate delta distances (distance to cross one voxel in each direction)
    glm::vec3 deltaDist = glm::vec3(
        rayDir.x == 0 ? 1e30f : std::abs(1.0f / rayDir.x),
        rayDir.y == 0 ? 1e30f : std::abs(1.0f / rayDir.y),
        rayDir.z == 0 ? 1e30f : std::abs(1.0f / rayDir.z)
    );

    // Calculate initial side distances
    glm::vec3 sideDist;
    if (rayDir.x < 0) {
        sideDist.x = (rayPos.x - voxelPos.x) * deltaDist.x;
    } else {
        sideDist.x = (voxelPos.x + 1.0f - rayPos.x) * deltaDist.x;
    }
    if (rayDir.y < 0) {
        sideDist.y = (rayPos.y - voxelPos.y) * deltaDist.y;
    } else {
        sideDist.y = (voxelPos.y + 1.0f - rayPos.y) * deltaDist.y;
    }
    if (rayDir.z < 0) {
        sideDist.z = (rayPos.z - voxelPos.z) * deltaDist.z;
    } else {
        sideDist.z = (voxelPos.z + 1.0f - rayPos.z) * deltaDist.z;
    }

    // Track which side was hit (for calculating normals)
    int hitSide = 0; // 0=x, 1=y, 2=z

    // Previous voxel position (for placing blocks)
    glm::ivec3 prevVoxelPos = voxelPos;

    // Step through voxels
    float currentDistance = 0.0f;
    while (currentDistance < maxDistance) {
        // Check if current voxel contains a solid block
        BlockData block = getBlock(voxelPos.x, voxelPos.y, voxelPos.z);
        if (block.type != BlockType::AIR) {
            // Hit a solid block
            result.hit = true;
            result.blockPos = voxelPos;
            result.adjacentPos = prevVoxelPos;
            result.block = block;
            result.distance = currentDistance;

            // Calculate hit point
            result.hitPoint = origin + rayDir * currentDistance;

            // Calculate face normal based on which side was hit
            result.normal = glm::vec3(0.0f);
            if (hitSide == 0) { // X side
                result.normal.x = stepDir.x > 0 ? -1.0f : 1.0f;
            } else if (hitSide == 1) { // Y side
                result.normal.y = stepDir.y > 0 ? -1.0f : 1.0f;
            } else { // Z side
                result.normal.z = stepDir.z > 0 ? -1.0f : 1.0f;
            }

            break;
        }

        // Store previous position for block placement
        prevVoxelPos = voxelPos;

        // Move to next voxel
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            // Step in X direction
            sideDist.x += deltaDist.x;
            voxelPos.x += stepDir.x;
            currentDistance = sideDist.x - deltaDist.x;
            hitSide = 0;
        } else if (sideDist.y < sideDist.z) {
            // Step in Y direction
            sideDist.y += deltaDist.y;
            voxelPos.y += stepDir.y;
            currentDistance = sideDist.y - deltaDist.y;
            hitSide = 1;
        } else {
            // Step in Z direction
            sideDist.z += deltaDist.z;
            voxelPos.z += stepDir.z;
            currentDistance = sideDist.z - deltaDist.z;
            hitSide = 2;
        }
    }

    return result;
}

void World::updateDirtyChunks() {
    // Check all loaded chunks for those that need mesh regeneration
    for (auto& pair : chunks) {
        Chunk* chunk = pair.second.get();
        if (chunk && chunk->needsRemeshing() && chunk->getState() == ChunkState::GENERATED) {
            // Regenerate mesh for this chunk
            chunk->generateMesh();
        }
    }
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

void World::setRenderDistance(int distance) {
    // Clamp render distance to reasonable values
    renderDistance = std::max(2, std::min(32, distance));
    chunkUnloadDistance = renderDistance * 1.5f + 1.0f;
}

// PHASE 7: Player Interaction - Raycasting Implementation
World::RaycastResult World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
    RaycastResult result;
    result.hit = false;
    result.distance = maxDistance;

    // Normalize direction vector
    glm::vec3 rayDir = glm::normalize(direction);

    // DDA (Digital Differential Analyzer) algorithm for voxel traversal
    // Current position in the ray
    glm::vec3 rayPos = origin;

    // Step direction for each axis
    glm::ivec3 stepDir = glm::ivec3(
        rayDir.x > 0 ? 1 : -1,
        rayDir.y > 0 ? 1 : -1,
        rayDir.z > 0 ? 1 : -1
    );

    // Current voxel position
    glm::ivec3 voxelPos = glm::ivec3(glm::floor(rayPos));

    // Calculate delta distances (distance to cross one voxel in each direction)
    glm::vec3 deltaDist = glm::vec3(
        rayDir.x == 0 ? 1e30f : std::abs(1.0f / rayDir.x),
        rayDir.y == 0 ? 1e30f : std::abs(1.0f / rayDir.y),
        rayDir.z == 0 ? 1e30f : std::abs(1.0f / rayDir.z)
    );

    // Calculate initial side distances
    glm::vec3 sideDist;
    if (rayDir.x < 0) {
        sideDist.x = (rayPos.x - voxelPos.x) * deltaDist.x;
    } else {
        sideDist.x = (voxelPos.x + 1.0f - rayPos.x) * deltaDist.x;
    }
    if (rayDir.y < 0) {
        sideDist.y = (rayPos.y - voxelPos.y) * deltaDist.y;
    } else {
        sideDist.y = (voxelPos.y + 1.0f - rayPos.y) * deltaDist.y;
    }
    if (rayDir.z < 0) {
        sideDist.z = (rayPos.z - voxelPos.z) * deltaDist.z;
    } else {
        sideDist.z = (voxelPos.z + 1.0f - rayPos.z) * deltaDist.z;
    }

    // Track which side was hit (for calculating normals)
    int hitSide = 0; // 0=x, 1=y, 2=z

    // Previous voxel position (for placing blocks)
    glm::ivec3 prevVoxelPos = voxelPos;

    // Step through voxels
    float currentDistance = 0.0f;
    while (currentDistance < maxDistance) {
        // Check if current voxel contains a solid block
        BlockData block = getBlock(voxelPos.x, voxelPos.y, voxelPos.z);
        if (block.type != BlockType::AIR) {
            // Hit a solid block
            result.hit = true;
            result.blockPos = voxelPos;
            result.adjacentPos = prevVoxelPos;
            result.block = block;
            result.distance = currentDistance;

            // Calculate hit point
            result.hitPoint = origin + rayDir * currentDistance;

            // Calculate face normal based on which side was hit
            result.normal = glm::vec3(0.0f);
            if (hitSide == 0) { // X side
                result.normal.x = stepDir.x > 0 ? -1.0f : 1.0f;
            } else if (hitSide == 1) { // Y side
                result.normal.y = stepDir.y > 0 ? -1.0f : 1.0f;
            } else { // Z side
                result.normal.z = stepDir.z > 0 ? -1.0f : 1.0f;
            }

            break;
        }

        // Store previous position for block placement
        prevVoxelPos = voxelPos;

        // Move to next voxel
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            // Step in X direction
            sideDist.x += deltaDist.x;
            voxelPos.x += stepDir.x;
            currentDistance = sideDist.x - deltaDist.x;
            hitSide = 0;
        } else if (sideDist.y < sideDist.z) {
            // Step in Y direction
            sideDist.y += deltaDist.y;
            voxelPos.y += stepDir.y;
            currentDistance = sideDist.y - deltaDist.y;
            hitSide = 1;
        } else {
            // Step in Z direction
            sideDist.z += deltaDist.z;
            voxelPos.z += stepDir.z;
            currentDistance = sideDist.z - deltaDist.z;
            hitSide = 2;
        }
    }

    return result;
}

void World::updateDirtyChunks() {
    // Check all loaded chunks for those that need mesh regeneration
    for (auto& pair : chunks) {
        Chunk* chunk = pair.second.get();
        if (chunk && chunk->needsRemeshing() && chunk->getState() == ChunkState::GENERATED) {
            // Regenerate mesh for this chunk
            chunk->generateMesh();
        }
    }
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

void World::setRenderDistance(int distance) {
    // Clamp render distance to reasonable values
    renderDistance = std::max(2, std::min(32, distance));
    chunkUnloadDistance = renderDistance * 1.5f + 1.0f;
}

// PHASE 7: Player Interaction - Raycasting Implementation
World::RaycastResult World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
    RaycastResult result;
    result.hit = false;
    result.distance = maxDistance;

    // Normalize direction vector
    glm::vec3 rayDir = glm::normalize(direction);

    // DDA (Digital Differential Analyzer) algorithm for voxel traversal
    // Current position in the ray
    glm::vec3 rayPos = origin;

    // Step direction for each axis
    glm::ivec3 stepDir = glm::ivec3(
        rayDir.x > 0 ? 1 : -1,
        rayDir.y > 0 ? 1 : -1,
        rayDir.z > 0 ? 1 : -1
    );

    // Current voxel position
    glm::ivec3 voxelPos = glm::ivec3(glm::floor(rayPos));

    // Calculate delta distances (distance to cross one voxel in each direction)
    glm::vec3 deltaDist = glm::vec3(
        rayDir.x == 0 ? 1e30f : std::abs(1.0f / rayDir.x),
        rayDir.y == 0 ? 1e30f : std::abs(1.0f / rayDir.y),
        rayDir.z == 0 ? 1e30f : std::abs(1.0f / rayDir.z)
    );

    // Calculate initial side distances
    glm::vec3 sideDist;
    if (rayDir.x < 0) {
        sideDist.x = (rayPos.x - voxelPos.x) * deltaDist.x;
    } else {
        sideDist.x = (voxelPos.x + 1.0f - rayPos.x) * deltaDist.x;
    }
    if (rayDir.y < 0) {
        sideDist.y = (rayPos.y - voxelPos.y) * deltaDist.y;
    } else {
        sideDist.y = (voxelPos.y + 1.0f - rayPos.y) * deltaDist.y;
    }
    if (rayDir.z < 0) {
        sideDist.z = (rayPos.z - voxelPos.z) * deltaDist.z;
    } else {
        sideDist.z = (voxelPos.z + 1.0f - rayPos.z) * deltaDist.z;
    }

    // Track which side was hit (for calculating normals)
    int hitSide = 0; // 0=x, 1=y, 2=z

    // Previous voxel position (for placing blocks)
    glm::ivec3 prevVoxelPos = voxelPos;

    // Step through voxels
    float currentDistance = 0.0f;
    while (currentDistance < maxDistance) {
        // Check if current voxel contains a solid block
        BlockData block = getBlock(voxelPos.x, voxelPos.y, voxelPos.z);
        if (block.type != BlockType::AIR) {
            // Hit a solid block
            result.hit = true;
            result.blockPos = voxelPos;
            result.adjacentPos = prevVoxelPos;
            result.block = block;
            result.distance = currentDistance;

            // Calculate hit point
            result.hitPoint = origin + rayDir * currentDistance;

            // Calculate face normal based on which side was hit
            result.normal = glm::vec3(0.0f);
            if (hitSide == 0) { // X side
                result.normal.x = stepDir.x > 0 ? -1.0f : 1.0f;
            } else if (hitSide == 1) { // Y side
                result.normal.y = stepDir.y > 0 ? -1.0f : 1.0f;
            } else { // Z side
                result.normal.z = stepDir.z > 0 ? -1.0f : 1.0f;
            }

            break;
        }

        // Store previous position for block placement
        prevVoxelPos = voxelPos;

        // Move to next voxel
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            // Step in X direction
            sideDist.x += deltaDist.x;
            voxelPos.x += stepDir.x;
            currentDistance = sideDist.x - deltaDist.x;
            hitSide = 0;
        } else if (sideDist.y < sideDist.z) {
            // Step in Y direction
            sideDist.y += deltaDist.y;
            voxelPos.y += stepDir.y;
            currentDistance = sideDist.y - deltaDist.y;
            hitSide = 1;
        } else {
            // Step in Z direction
            sideDist.z += deltaDist.z;
            voxelPos.z += stepDir.z;
            currentDistance = sideDist.z - deltaDist.z;
            hitSide = 2;
        }
    }

    return result;
}

void World::updateDirtyChunks() {
    // Check all loaded chunks for those that need mesh regeneration
    for (auto& pair : chunks) {
        Chunk* chunk = pair.second.get();
        if (chunk && chunk->needsRemeshing() && chunk->getState() == ChunkState::GENERATED) {
            // Regenerate mesh for this chunk
            chunk->generateMesh();
        }
    }
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

void World::setRenderDistance(int distance) {
    // Clamp render distance to reasonable values
    renderDistance = std::max(2, std::min(32, distance));
    chunkUnloadDistance = renderDistance * 1.5f + 1.0f;
}

// PHASE 7: Player Interaction - Raycasting Implementation
World::RaycastResult World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
    RaycastResult result;
    result.hit = false;
    result.distance = maxDistance;

    // Normalize direction vector
    glm::vec3 rayDir = glm::normalize(direction);

    // DDA (Digital Differential Analyzer) algorithm for voxel traversal
    // Current position in the ray
    glm::vec3 rayPos = origin;

    // Step direction for each axis
    glm::ivec3 stepDir = glm::ivec3(
        rayDir.x > 0 ? 1 : -1,
        rayDir.y > 0 ? 1 : -1,
        rayDir.z > 0 ? 1 : -1
    );

    // Current voxel position
    glm::ivec3 voxelPos = glm::ivec3(glm::floor(rayPos));

    // Calculate delta distances (distance to cross one voxel in each direction)
    glm::vec3 deltaDist = glm::vec3(
        rayDir.x == 0 ? 1e30f : std::abs(1.0f / rayDir.x),
        rayDir.y == 0 ? 1e30f : std::abs(1.0f / rayDir.y),
        rayDir.z == 0 ? 1e30f : std::abs(1.0f / rayDir.z)
    );

    // Calculate initial side distances
    glm::vec3 sideDist;
    if (rayDir.x < 0) {
        sideDist.x = (rayPos.x - voxelPos.x) * deltaDist.x;
    } else {
        sideDist.x = (voxelPos.x + 1.0f - rayPos.x) * deltaDist.x;
    }
    if (rayDir.y < 0) {
        sideDist.y = (rayPos.y - voxelPos.y) * deltaDist.y;
    } else {
        sideDist.y = (voxelPos.y + 1.0f - rayPos.y) * deltaDist.y;
    }
    if (rayDir.z < 0) {
        sideDist.z = (rayPos.z - voxelPos.z) * deltaDist.z;
    } else {
        sideDist.z = (voxelPos.z + 1.0f - rayPos.z) * deltaDist.z;
    }

    // Track which side was hit (for calculating normals)
    int hitSide = 0; // 0=x, 1=y, 2=z

    // Previous voxel position (for placing blocks)
    glm::ivec3 prevVoxelPos = voxelPos;

    // Step through voxels
    float currentDistance = 0.0f;
    while (currentDistance < maxDistance) {
        // Check if current voxel contains a solid block
        BlockData block = getBlock(voxelPos.x, voxelPos.y, voxelPos.z);
        if (block.type != BlockType::AIR) {
            // Hit a solid block
            result.hit = true;
            result.blockPos = voxelPos;
            result.adjacentPos = prevVoxelPos;
            result.block = block;
            result.distance = currentDistance;

            // Calculate hit point
            result.hitPoint = origin + rayDir * currentDistance;

            // Calculate face normal based on which side was hit
            result.normal = glm::vec3(0.0f);
            if (hitSide == 0) { // X side
                result.normal.x = stepDir.x > 0 ? -1.0f : 1.0f;
            } else if (hitSide == 1) { // Y side
                result.normal.y = stepDir.y > 0 ? -1.0f : 1.0f;
            } else { // Z side
                result.normal.z = stepDir.z > 0 ? -1.0f : 1.0f;
            }

            break;
        }

        // Store previous position for block placement
        prevVoxelPos = voxelPos;

        // Move to next voxel
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            // Step in X direction
            sideDist.x += deltaDist.x;
            voxelPos.x += stepDir.x;
            currentDistance = sideDist.x - deltaDist.x;
            hitSide = 0;
        } else if (sideDist.y < sideDist.z) {
            // Step in Y direction
            sideDist.y += deltaDist.y;
            voxelPos.y += stepDir.y;
            currentDistance = sideDist.y - deltaDist.y;
            hitSide = 1;
        } else {
            // Step in Z direction
            sideDist.z += deltaDist.z;
            voxelPos.z += stepDir.z;
            currentDistance = sideDist.z - deltaDist.z;
            hitSide = 2;
        }
    }

    return result;
}

void World::updateDirtyChunks() {
    // Check all loaded chunks for those that need mesh regeneration
    for (auto& pair : chunks) {
        Chunk* chunk = pair.second.get();
        if (chunk && chunk->needsRemeshing() && chunk->getState() == ChunkState::GENERATED) {
            // Regenerate mesh for this chunk
            chunk->generateMesh();
        }
    }
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

void World::setRenderDistance(int distance) {
    // Clamp render distance to reasonable values
    renderDistance = std::max(2, std::min(32, distance));
    chunkUnloadDistance = renderDistance * 1.5f + 1.0f;
}

// PHASE 7: Player Interaction - Raycasting Implementation
World::RaycastResult World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
    RaycastResult result;
    result.hit = false;
    result.distance = maxDistance;

    // Normalize direction vector
    glm::vec3 rayDir = glm::normalize(direction);

    // DDA (Digital Differential Analyzer) algorithm for voxel traversal
    // Current position in the ray
    glm::vec3 rayPos = origin;

    // Step direction for each
