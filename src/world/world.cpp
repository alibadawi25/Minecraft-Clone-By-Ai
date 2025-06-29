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

#ifdef FASTNOISE_AVAILABLE
    // Initialize mountain generation noise
    setupMountainGeneration(1337);
#endif
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
    }    auto chunk = std::make_unique<Chunk>(coord, this);

    // Generate terrain using the improved chunk-based generation
    chunk->generate();    // Add chunk to the world first
    addChunk(coord, std::move(chunk));

    // Notify neighbors that a new chunk is available
    notifyNeighborsOfNewChunk(coord);

    // Mark the chunk for remeshing instead of generating mesh immediately
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

void World::notifyNeighborsOfNewChunk(ChunkCoord newChunkCoord) {
    // Notify all 4 neighboring chunks that a new neighbor is available
    ChunkCoord neighborCoords[4] = {
        {newChunkCoord.x, newChunkCoord.z + 1}, // North
        {newChunkCoord.x, newChunkCoord.z - 1}, // South
        {newChunkCoord.x + 1, newChunkCoord.z}, // East
        {newChunkCoord.x - 1, newChunkCoord.z}  // West
    };

    for (int i = 0; i < 4; i++) {
        Chunk* neighborChunk = getChunk(neighborCoords[i]);
        if (neighborChunk) {
            neighborChunk->markNeighborDirty();
        }
    }
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
        Chunk* chunk = pair.second.get();
        if (chunk) {
            // Check for neighbor changes periodically
            chunk->updateFromNeighbors();

            // Generate mesh if needed
            if (meshesGenerated < maxMeshesThisFrame && chunk->needsRemeshing()) {
                if (chunk->getState() == ChunkState::GENERATED || chunk->getState() == ChunkState::READY) {
                    chunk->generateMesh();
                    meshesGenerated++;
                }
            }
        }
    }
}

// Note: Legacy generatePerlinTerrain method below is no longer used
// Terrain generation now handled by efficient Chunk::generate() method
// with static FastNoise generators and better performance

void World::generatePerlinTerrain(Chunk* chunk) {
    if (!chunk) return;

    ChunkCoord coord = chunk->getCoord();

    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_DEPTH; ++z) {
            // Convert to world coordinates
            int worldX = coord.x * CHUNK_WIDTH + x;
            int worldZ = coord.z * CHUNK_DEPTH + z;

            // Natural terrain generation (Minecraft-style)
            float worldXf = static_cast<float>(worldX);
            float worldZf = static_cast<float>(worldZ);            // Advanced FastNoise terrain generation with smoothing
#ifdef FASTNOISE_AVAILABLE
            // Apply gentle domain warping for natural terrain distortion
            float warpedX = worldXf;
            float warpedZ = worldZf;
            domainWarpNoise.DomainWarp(warpedX, warpedZ);

            // Get terrain height using the sophisticated FastNoise system
            float terrainHeightFloat = getTerrainHeight(warpedX, warpedZ);

            // Apply gentle smoothing by reducing extreme height variations
            // This prevents sudden cliff faces while maintaining terrain variety
            float smoothingFactor = 0.85f;
            terrainHeightFloat = BASE_HEIGHT + (terrainHeightFloat - BASE_HEIGHT) * smoothingFactor;

            int terrainHeight = static_cast<int>(terrainHeightFloat);
#else
            // Fallback to simple noise if FastNoise not available
            float heightNoise = noiseGenerator.fractalNoise2D(worldXf * 0.01f, worldZf * 0.01f, 4, 0.5f);

            // Apply non-linear transformation: make higher areas steeper, lower areas flatter
            float normalizedNoise = (heightNoise + 1.0f) * 0.5f; // Convert from [-1,1] to [0,1]
            float steepnessFactor = 2.2f; // Higher = more contrast between high/low areas
            float transformedNoise = std::pow(normalizedNoise, steepnessFactor);
            transformedNoise = transformedNoise * 2.0f - 1.0f;
            int terrainHeight = BASE_HEIGHT + static_cast<int>(transformedNoise * 55.0f);
            terrainHeight = std::max(9, std::min(terrainHeight, CHUNK_HEIGHT - 30));
#endif

            // Generate blocks in column with natural layering
            for (int y = 0; y < CHUNK_HEIGHT; ++y) {
                BlockType blockType = BlockType::AIR;

                if (y == 0) {
                    // Bedrock at bottom
                    blockType = BlockType::BEDROCK;
                } else if (y <= terrainHeight) {
                    // Natural layering based on depth from surface
                    int depthFromSurface = terrainHeight - y;                    if (depthFromSurface == 0) {
                        // Surface layer
                        if (terrainHeight >= 70) {
                            blockType = BlockType::STONE; // Stone surface on high mountains
                        } else if (terrainHeight > WATER_LEVEL + 2) {
                            blockType = BlockType::GRASS;                        } else if (terrainHeight >= WATER_LEVEL - 2) {
                            blockType = BlockType::SAND; // Beach/shore                        } else {
                            blockType = BlockType::SAND; // Underwater surface
                        }
                    } else if (depthFromSurface <= 3) {
                        // Shallow subsurface - dirt layer
                        if (terrainHeight >= 65) {
                            blockType = BlockType::STONE; // Stone appears shallow on high mountains
                        } else if (terrainHeight >= WATER_LEVEL - 2) {
                            blockType = BlockType::DIRT;
                        } else {
                            blockType = BlockType::SAND; // Underwater becomes sand
                        }
                    } else if (terrainHeight >= 60 && depthFromSurface <= 6) {
                        // Mountains - stone appears closer to surface
                        blockType = BlockType::STONE;
                    } else if (terrainHeight >= 50 && depthFromSurface <= 5) {
                        // Hills - some stone near surface
                        blockType = BlockType::STONE;
                    } else if (depthFromSurface > 8) {
                        // Deep underground - all stone
                        blockType = BlockType::STONE;
                    } else {
                        // Default layer based on whether underwater or not
                        if (terrainHeight < WATER_LEVEL) {
                            blockType = BlockType::SAND; // Underwater areas use sand
                        } else {
                            blockType = BlockType::DIRT; // Above water areas use dirt
                        }
                    }
                }
                // Fill water in low areas
                else if (y <= WATER_LEVEL && terrainHeight < WATER_LEVEL) {
                    blockType = BlockType::WATER;
                }                if (blockType != BlockType::AIR) {
                    chunk->setBlock(x, y, z, BlockData(blockType));
                }
            }
        }
    }
}

// Terrain generation is now handled by Chunk::generate() method
// This provides better performance with static noise generators and cleaner code organization

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

    // Update the terrain settings with new seeds
    gTerrainSettings.baseSeed = newSeed;
    gTerrainSettings.mountainSeed = newSeed + 1337;  // Offset for different patterns

    // Update the legacy noise generator (kept for compatibility)
    noiseGenerator = MathUtils::SimpleNoise(newSeed);

    // Force re-initialization of static noise generators in Chunk::generate()
    // This is done by calling a static reset function
    resetChunkNoiseGenerators();

    // Clear targeted block
    clearTargetedBlock();

    // The world is ready for new chunk generation with the new seed
    // Chunks will be generated as needed when the player moves around
}

#ifdef FASTNOISE_AVAILABLE
void World::setupMountainGeneration(unsigned int seed) {
    // ADVANCED MINECRAFT-INSPIRED NOISE SETUP WITH MODERN FASTNOISE FEATURES

    // Continentalness noise - large scale landmasses (like Minecraft 1.18+)
    mountainNoise.SetSeed(seed);
    mountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    mountainNoise.SetFractalType(FastNoiseLite::FractalType_Ridged); // Ridged for more dramatic continents
    mountainNoise.SetFractalOctaves(5);
    mountainNoise.SetFractalLacunarity(2.1f);
    mountainNoise.SetFractalGain(0.6f);
    mountainNoise.SetFractalWeightedStrength(0.8f); // Enhanced contrast
    mountainNoise.SetFrequency(0.0006f);  // Very low frequency for large continents    // Height noise - medium scale hills and mountains with smoother variation
    ridgeNoise.SetSeed(seed + 1000);
    ridgeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S); // Smoother variant
    ridgeNoise.SetFractalType(FastNoiseLite::FractalType_FBm); // Back to FBm for smoother transitions
    ridgeNoise.SetFractalOctaves(6);
    ridgeNoise.SetFractalLacunarity(2.1f);
    ridgeNoise.SetFractalGain(0.5f); // Reduced gain for smoother terrain
    ridgeNoise.SetFractalWeightedStrength(0.7f); // Reduced contrast
    ridgeNoise.SetFrequency(0.003f);  // Medium frequency for hills and mountains

    // Detail noise for surface variation - back to smoother OpenSimplex
    detailNoise.SetSeed(seed + 2000);
    detailNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); // Smoother than cellular
    detailNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    detailNoise.SetFractalOctaves(3);
    detailNoise.SetFractalLacunarity(2.0f);
    detailNoise.SetFractalGain(0.4f);
    detailNoise.SetFrequency(0.01f);  // Higher frequency for surface details

    // Gentler domain warp to prevent steep walls
    domainWarpNoise.SetSeed(seed + 3000);
    domainWarpNoise.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
    domainWarpNoise.SetDomainWarpAmp(15.0f);  // Much reduced warp to prevent cliff faces
    domainWarpNoise.SetFrequency(0.0015f);
    domainWarpNoise.SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);
    domainWarpNoise.SetFractalOctaves(3);
    domainWarpNoise.SetFractalLacunarity(2.2f);
    domainWarpNoise.SetFractalGain(0.7f);
}

float World::getTerrainHeight(float worldX, float worldZ) const {
    // ADVANCED MINECRAFT-INSPIRED TERRAIN GENERATION WITH MODERN FASTNOISE

    // Continentalness - large scale land masses (like Minecraft's continent noise)
    float continentNoise = mountainNoise.GetNoise(worldX, worldZ);
    continentNoise = (continentNoise + 1.0f) * 0.5f; // 0 to 1

    // Height variation - medium scale hills and valleys with terracing
    float heightNoise = ridgeNoise.GetNoise(worldX, worldZ);

    // Detail - organic surface variation using cellular noise
    float surfaceDetail = detailNoise.GetNoise(worldX, worldZ);
    surfaceDetail = (surfaceDetail + 1.0f) * 0.5f; // Normalize cellular noise

    // Advanced terrain blending with erosion simulation
    float terrainType = continentNoise;

    // Apply terrain-dependent erosion effects
    float erosion = 1.0f;
    if (terrainType > 0.6f) {
        // Mountains experience more erosion creating valleys and plateaus
        erosion = 0.7f + 0.3f * surfaceDetail;
    }    float finalHeight;
    if (terrainType < 0.25f) {
        // Deep ocean/lowlands (10-30 blocks) - flatter with subtle variation
        finalHeight = 10.0f + terrainType * 80.0f + heightNoise * 6.0f + surfaceDetail * 2.0f;
    } else if (terrainType < 0.45f) {
        // Shallow waters/beaches (30-50 blocks) - gentle slopes
        finalHeight = 30.0f + (terrainType - 0.25f) * 100.0f + heightNoise * 8.0f + surfaceDetail * 3.0f;
    } else if (terrainType < 0.65f) {
        // Plains/rolling hills (50-75 blocks) - moderate variation
        finalHeight = 50.0f + (terrainType - 0.45f) * 125.0f + heightNoise * 12.0f * erosion + surfaceDetail * 4.0f;
    } else if (terrainType < 0.85f) {
        // Foothills/high hills (75-100 blocks) - more dramatic but controlled
        finalHeight = 75.0f + (terrainType - 0.65f) * 125.0f + heightNoise * 18.0f * erosion + surfaceDetail * 5.0f;
    } else {
        // High mountains (100-120 blocks) - controlled extreme variation
        float mountainFactor = (terrainType - 0.85f) * 6.67f; // 0 to 1
        finalHeight = 100.0f + mountainFactor * 20.0f + heightNoise * 22.0f * erosion + surfaceDetail * 6.0f;

        // Create controlled mountain peaks (reduced amplification)
        if (heightNoise > 0.4f) {
            finalHeight += (heightNoise - 0.4f) * 12.0f; // Reduced peak amplification
        }
    }

    // Clamp to valid range with more generous mountain heights
    return std::max(5.0f, std::min(finalHeight, static_cast<float>(CHUNK_HEIGHT - 10)));
}
#endif

void World::resetChunkNoiseGenerators() {
    // This function forces the static noise generators in Chunk::generate()
    // to be re-initialized with new seeds on the next chunk generation
    // We do this by calling a static function in the Chunk class
    Chunk::resetStaticNoiseGenerators();
}
