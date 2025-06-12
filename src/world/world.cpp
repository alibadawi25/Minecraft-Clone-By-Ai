// filepath: c:\Users\aliba\OneDrive\Documents\ai-karim\src\world\world.cpp
/**
 * @file world.cpp
 * @brief World management implementation with chunk loading, terrain generation, and rendering
 */

#include "world.h"
#include "chunk.h"
#include "block.h"
#include "../renderer/simple_shader.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

World::World() : initialized(false), blockShader(nullptr), highlightShader(nullptr),
                  highlightVAO(0), highlightVBO(0), targetedBlockValid(false),
                  noiseGenerator(1337), renderDistance(DEFAULT_RENDER_DISTANCE) {
    chunkUnloadDistance = renderDistance * CHUNK_UNLOAD_MULTIPLIER + 1.0f;
}

World::~World() {
    shutdown();
}

void World::initialize() {
    if (initialized) return;    // Initialize block registry
    BlockRegistry::initialize();

    // Initialize block shader
    blockShader = new SimpleShader("shaders/block.vert", "shaders/block.frag");

    // Initialize highlight shader and geometry
    highlightShader = new SimpleShader("shaders/highlight.vert", "shaders/highlight.frag");
    initializeHighlightGeometry();

    // Start with no chunks - they will be loaded around the player
    initialized = true;
}

void World::shutdown() {
    if (!initialized) return;

    chunks.clear();    if (blockShader) {
        delete blockShader;
    blockShader = nullptr;
    }

    // Cleanup highlight resources
    if (highlightShader) {
        delete highlightShader;
        highlightShader = nullptr;
    }

    if (highlightVAO != 0) {
        glDeleteVertexArrays(1, &highlightVAO);
        highlightVAO = 0;
    }

    if (highlightVBO != 0) {
        glDeleteBuffers(1, &highlightVBO);
        highlightVBO = 0;
    }BlockRegistry::shutdown();

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

    // Update frustum for culling
    glm::mat4 viewProjection = projection * view;
    viewFrustum.updateFromMatrix(viewProjection);

    // Configure shader
    blockShader->use();
    blockShader->setMatrix4("view", view);
    blockShader->setMatrix4("projection", projection);

    // Set lighting uniforms
    blockShader->setVector3("lightDirection", glm::vec3(0.2f, -0.8f, 0.1f));
    blockShader->setVector3("lightColor", glm::vec3(0.8f, 0.8f, 0.7f));
    blockShader->setVector3("ambientColor", glm::vec3(0.3f, 0.3f, 0.4f));    // Set fog uniforms based on render distance (more aggressive fog to hide generation)
    float renderDistanceWorldUnits = renderDistance * CHUNK_WIDTH;
    float fogNear = renderDistanceWorldUnits * 0.60f;  // Start fog earlier
    float fogFar = renderDistanceWorldUnits * 0.90f;   // End fog before render edge
    blockShader->setFloat("fogNear", fogNear);
    blockShader->setFloat("fogFar", fogFar);
    // Match fog color to sky horizon color: #87CEEB (Sky Blue)
    blockShader->setVector3("fogColor", glm::vec3(0.529f, 0.808f, 0.922f));// Bind texture atlas
    GLuint textureAtlas = BlockRegistry::getTextureAtlas();
    if (textureAtlas != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureAtlas);
        blockShader->setInt("blockTexture", 0);
    }
      // Enable blending for water transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable depth testing but allow depth writes for proper sorting
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE); // Allow depth writes for solid blocks

    // Render chunks with frustum culling
    int chunksRendered = 0;
    int chunksCulled = 0;

    for (auto& pair : chunks) {
        if (pair.second && pair.second->isReady()) {
            // Calculate chunk bounding box for frustum culling
            glm::vec3 chunkWorldPos = ChunkUtils::chunkToWorldPos(pair.first);
            glm::vec3 chunkMin = chunkWorldPos;
            glm::vec3 chunkMax = chunkWorldPos + glm::vec3(CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_DEPTH);

            // Frustum culling test
            if (viewFrustum.containsAABB(chunkMin, chunkMax)) {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), chunkWorldPos);
                blockShader->setMatrix4("model", model);
                pair.second->render(view, projection, cameraPos);
                chunksRendered++;
            } else {
                chunksCulled++;
            }
        }
    }

    // Store statistics for debugging
    lastRenderedChunks = chunksRendered;
    lastCulledChunks = chunksCulled;

    // Disable blending after rendering
    glDisable(GL_BLEND);
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
        // Convert world coordinates to local chunk coordinates
        glm::ivec3 localPos = ChunkUtils::worldToLocal(x, y, z);

        chunk->setBlockWorld(x, y, z, block);
        chunk->markForRemesh();

        // Check if this block is at a chunk boundary and notify neighboring chunks
        // A block is at a boundary if it's at the edge of the chunk (0 or 15 for x,z)
        std::vector<ChunkCoord> neighborsToUpdate;

        if (localPos.x == 0) {
            // Block is at the left edge, notify left neighbor
            neighborsToUpdate.emplace_back(chunkCoord.x - 1, chunkCoord.z);
        }
        if (localPos.x == CHUNK_WIDTH - 1) {
            // Block is at the right edge, notify right neighbor
            neighborsToUpdate.emplace_back(chunkCoord.x + 1, chunkCoord.z);
        }
        if (localPos.z == 0) {
            // Block is at the front edge, notify front neighbor
            neighborsToUpdate.emplace_back(chunkCoord.x, chunkCoord.z - 1);
        }
        if (localPos.z == CHUNK_DEPTH - 1) {
            // Block is at the back edge, notify back neighbor
            neighborsToUpdate.emplace_back(chunkCoord.x, chunkCoord.z + 1);
        }

        // Mark neighboring chunks for remeshing
        for (const ChunkCoord& neighborCoord : neighborsToUpdate) {
            Chunk* neighborChunk = getChunk(neighborCoord);
            if (neighborChunk) {
                neighborChunk->markForRemesh();
            }
        }
    }
}

void World::generateSimpleTerrain(Chunk* chunk) {
    if (!chunk) return;

    // Create simple flat world
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
    addChunk(coord, std::move(chunk));    // Mark the chunk for remeshing instead of generating mesh immediately
    // This allows all neighboring chunks to be loaded first, ensuring cross-chunk face culling works
    Chunk* loadedChunk = getChunk(coord);
    if (loadedChunk) {
        loadedChunk->markForRemesh();

        // IMPORTANT: Also mark neighboring chunks for remeshing
        // When a new chunk is loaded, its neighbors need to update their faces
        // because they might have been rendering boundary faces that should now be culled
        std::vector<ChunkCoord> neighborCoords = {
            ChunkCoord(coord.x - 1, coord.z),     // Left
            ChunkCoord(coord.x + 1, coord.z),     // Right
            ChunkCoord(coord.x, coord.z - 1),     // Front
            ChunkCoord(coord.x, coord.z + 1)      // Back
        };

        for (const ChunkCoord& neighborCoord : neighborCoords) {
            Chunk* neighborChunk = getChunk(neighborCoord);
            if (neighborChunk && neighborChunk->isReady()) {  // KEY: Only remesh if chunk is READY
                neighborChunk->markForRemesh();
            }
        }
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

void World::updateDirtyChunks() {
    // Throttle mesh generation to prevent frame drops - dynamically based on render distance
    int meshesGenerated = 0;
    int maxMeshesThisFrame = getMaxMeshesPerFrame();

    for (auto& pair : chunks) {
        if (meshesGenerated >= maxMeshesThisFrame) {
            break; // Limit mesh generation per frame
        }

        Chunk* chunk = pair.second.get();
        if (chunk && chunk->needsRemeshing()) {
            if (chunk->getState() == ChunkState::GENERATED || chunk->getState() == ChunkState::READY) {
                chunk->generateMesh();
                meshesGenerated++;
            }
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
            float detailNoise = noiseGenerator.fractalNoise2D(worldX * 0.05f, worldZ * 0.05f, 2, 0.5f);            // Combine noise values to create varied terrain
            float combinedNoise = continentalness * 0.7f + heightNoise * 0.2f + detailNoise * 0.1f;

            // Calculate terrain height
            int terrainHeight = static_cast<int>(BASE_HEIGHT + combinedNoise * 40);

            // Clamp height to valid range
            terrainHeight = std::max(5, std::min(terrainHeight, CHUNK_HEIGHT - 10));

            // Generate column of blocks
            for (int y = 0; y < CHUNK_HEIGHT; ++y) {
                BlockType blockType = BlockType::AIR;                if (y == 0) {
                    blockType = BlockType::BEDROCK;
                } else if (y < terrainHeight - STONE_DEPTH) {
                    blockType = BlockType::STONE;
                } else if (y < terrainHeight - 1) {
                    blockType = BlockType::DIRT;
                } else if (y <= terrainHeight) {
                    // Surface layer - grass above water level, sand below
                    if (terrainHeight > WATER_LEVEL) {
                        blockType = BlockType::GRASS;
                    } else {
                        blockType = BlockType::SAND;
                    }
                }
                // Water fills areas below water level where terrain is also below water level
                else if (y <= WATER_LEVEL && terrainHeight < WATER_LEVEL) {
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
    renderDistance = std::max(2, std::min(32, distance));
    chunkUnloadDistance = renderDistance * CHUNK_UNLOAD_MULTIPLIER + 1.0f;
}

// Player Interaction - Raycasting Implementation
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
    while (currentDistance < maxDistance) {        // Check if current voxel contains a solid block
        BlockData block = getBlock(voxelPos.x, voxelPos.y, voxelPos.z);
        if (block.type != BlockType::AIR) {
            // Hit a solid block
            result.hit = true;
            result.blockPos = voxelPos;
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

            // Calculate adjacent position for block placement based on hit face normal
            result.adjacentPos = result.blockPos + glm::ivec3(result.normal);

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

// Block highlighting implementation
void World::initializeHighlightGeometry() {
    // Create wireframe cube vertices for block outline
    float vertices[] = {
        // Bottom face outline
        0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  // 0 -> 1
        1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 1.0f,  // 1 -> 2
        1.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  // 2 -> 3
        0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.0f,  // 3 -> 0

        // Top face outline
        0.0f, 1.0f, 0.0f,  1.0f, 1.0f, 0.0f,  // 4 -> 5
        1.0f, 1.0f, 0.0f,  1.0f, 1.0f, 1.0f,  // 5 -> 6
        1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 1.0f,  // 6 -> 7
        0.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f,  // 7 -> 4

        // Vertical edges
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  // 0 -> 4
        1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 0.0f,  // 1 -> 5
        1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f,  // 2 -> 6
        0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 1.0f   // 3 -> 7
    };

    glGenVertexArrays(1, &highlightVAO);
    glGenBuffers(1, &highlightVBO);

    glBindVertexArray(highlightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, highlightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void World::renderBlockHighlight(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& /* cameraPos */) {
    if (!targetedBlockValid || !highlightShader) {
        return;
    }

    // Enable blending for semi-transparent outline
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth writing but keep depth testing
    glDepthMask(GL_FALSE);

    // Use highlight shader
    highlightShader->use();
    highlightShader->setMatrix4("view", view);
    highlightShader->setMatrix4("projection", projection);

    // Create model matrix for the targeted block
    glm::mat4 model = glm::translate(glm::mat4(1.0f),
        glm::vec3(targetedBlockPos.x, targetedBlockPos.y, targetedBlockPos.z));
    highlightShader->setMatrix4("model", model);

    // Set highlight color and alpha
    highlightShader->setVector3("highlightColor", glm::vec3(1.0f, 1.0f, 1.0f)); // White outline
    highlightShader->setFloat("alpha", 0.5f); // Semi-transparent

    // Render wireframe outline
    glBindVertexArray(highlightVAO);
    glDrawArrays(GL_LINES, 0, 24); // 12 edges * 2 vertices each
    glBindVertexArray(0);

    // Restore OpenGL state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void World::setTargetedBlock(const glm::ivec3& blockPos) {
    targetedBlockPos = blockPos;
    targetedBlockValid = true;
}

void World::clearTargetedBlock() {
    targetedBlockValid = false;
}

void World::regenerateWorld(unsigned int newSeed) {
    // Clear all existing chunks
    chunks.clear();

    // Update the noise generator with new seed
    noiseGenerator = MathUtils::SimpleNoise(newSeed);

    // Clear targeted block
    clearTargetedBlock();

    // The world is ready for new chunk generation with the new seed
    // Chunks will be generated as needed when the player moves around
}
