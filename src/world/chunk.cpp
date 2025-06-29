#include "chunk.h"
#include "world.h"
#include "block.h"
#include <iostream>
#include <algorithm>
#include <cmath>

// Global flag to reset static noise generators when seed changes
bool g_resetChunkNoise = false;

// Face vertices for cube mesh generation (in local coordinates)
// All faces ordered counter-clockwise when viewed from outside the cube
const std::array<std::array<glm::vec3, 4>, 6> Chunk::FACE_VERTICES = {{
    // FRONT (+Z) - looking at cube from positive Z direction
    // Counter-clockwise: bottom-left, bottom-right, top-right, top-left
    {{
        {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}
    }},
    // BACK (-Z) - looking at cube from negative Z direction
    // Counter-clockwise: bottom-right, bottom-left, top-left, top-right
    {{
        {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}
    }},
    // LEFT (-X) - looking at cube from negative X direction
    // Counter-clockwise: bottom-back, bottom-front, top-front, top-back
    {{
        {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}
    }},
    // RIGHT (+X) - looking at cube from positive X direction
    // Counter-clockwise: bottom-front, bottom-back, top-back, top-front
    {{
        {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}
    }},
    // TOP (+Y) - looking at cube from positive Y direction
    // Counter-clockwise: front-left, front-right, back-right, back-left
    {{
        {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}
    }},
    // BOTTOM (-Y) - looking at cube from negative Y direction
    // Counter-clockwise: back-left, back-right, front-right, front-left
    {{
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}
    }}
}};

// Face normals
const std::array<glm::vec3, 6> Chunk::FACE_NORMALS = {{
    {0.0f,  0.0f,  1.0f}, // FRONT
    {0.0f,  0.0f, -1.0f}, // BACK
    {-1.0f, 0.0f,  0.0f}, // LEFT
    {1.0f,  0.0f,  0.0f}, // RIGHT
    {0.0f,  1.0f,  0.0f}, // TOP
    {0.0f, -1.0f,  0.0f}  // BOTTOM
}};

Chunk::Chunk(ChunkCoord coord, World* world)
    : coord(coord), state(ChunkState::EMPTY), world(world), VAO(0), VBO(0),
      vertexCount(0), meshDirty(true), hasGeometry(false), hadAllNeighbors(false), lastNeighborCheck(0.0f) {

    // Initialize all blocks to air
    blocks.fill(BlockData(BlockType::AIR));

    // Initialize neighbor tracking
    for (int i = 0; i < 4; i++) {
        neighborsAvailable[i] = false;
    }

    // Initialize OpenGL resources
    initializeGL();
}

Chunk::~Chunk() {
    cleanupGL();
}

BlockData Chunk::getBlock(int x, int y, int z) const {
    if (!isInBounds(x, y, z)) {
        return BlockData(BlockType::AIR);
    }
    return blocks[getBlockIndex(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, BlockData block) {
    if (!isInBounds(x, y, z)) {
        return;
    }

    int index = getBlockIndex(x, y, z);
    if (blocks[index] != block) {
        blocks[index] = block;
        markForRemesh();
    }
}

BlockData Chunk::getBlockSafe(int x, int y, int z) const {
    return getBlock(x, y, z);
}

void Chunk::setBlockSafe(int x, int y, int z, BlockData block) {
    setBlock(x, y, z, block);
}

BlockData Chunk::getBlockWorld(int worldX, int worldY, int worldZ) const {
    glm::ivec3 local = ChunkUtils::worldToLocal(worldX, worldY, worldZ);
    return getBlock(local.x, local.y, local.z);
}

void Chunk::setBlockWorld(int worldX, int worldY, int worldZ, BlockData block) {
    glm::ivec3 local = ChunkUtils::worldToLocal(worldX, worldY, worldZ);
    setBlock(local.x, local.y, local.z, block);
}

void Chunk::generateMesh() {
    if (!meshDirty || (state != ChunkState::GENERATED && state != ChunkState::READY)) {
        return;
    }

    // Prevent infinite mesh regeneration
    if (state == ChunkState::MESHING) {
        return;
    }    setState(ChunkState::MESHING);

    std::vector<float> vertices;
    vertices.reserve(MAX_VERTICES_PER_CHUNK * VERTEX_STRIDE);

    // First pass: Render solid blocks for proper depth testing
    for (int y = 0; y < CHUNK_HEIGHT; y++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            for (int x = 0; x < CHUNK_WIDTH; x++) {
                BlockData blockData = getBlock(x, y, z);

                // Skip air blocks
                if (blockData.type == BlockType::AIR) {
                    continue;
                }

                const Block& block = BlockRegistry::getBlock(blockData.type);

                // FIRST PASS: Only render solid, opaque blocks
                if (!block.isSolid || block.isTransparent) {
                    continue;
                }

                glm::vec3 blockPos(x, y, z);

                // Check each face for visibility
                for (int face = 0; face < 6; face++) {
                    if (shouldRenderFace(x, y, z, static_cast<CubeFace>(face))) {
                        addFace(vertices, blockPos, static_cast<CubeFace>(face), blockData.type);
                    }
                }
            }
        }
    }

    // SECOND PASS: Render transparent blocks last for proper blending
    for (int y = 0; y < CHUNK_HEIGHT; y++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            for (int x = 0; x < CHUNK_WIDTH; x++) {
                BlockData blockData = getBlock(x, y, z);

                // Skip air blocks
                if (blockData.type == BlockType::AIR) {
                    continue;
                }

                const Block& block = BlockRegistry::getBlock(blockData.type);                // SECOND PASS: Only render transparent blocks
                if (!block.isTransparent) {
                    continue;
                }

                glm::vec3 blockPos(x, y, z);

                // Check each face for visibility - use water-specific culling for water blocks
                for (int face = 0; face < 6; face++) {
                    bool shouldRender;
                    if (blockData.type == BlockType::WATER) {
                        // Use water-specific face culling to prevent holes at chunk boundaries
                        shouldRender = shouldRenderWaterFace(x, y, z, static_cast<CubeFace>(face));
                    } else {
                        // Use normal face culling for other transparent blocks
                        shouldRender = shouldRenderFace(x, y, z, static_cast<CubeFace>(face));
                    }

                    if (shouldRender) {
                        addFace(vertices, blockPos, static_cast<CubeFace>(face), blockData.type);
                    }
                }
            }
        }
    }

    // Update OpenGL buffers
    vertexCount = vertices.size() / 8; // 8 floats per vertex (pos3 + normal3 + uv2)
    hasGeometry = vertexCount > 0;

    if (hasGeometry) {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                     vertices.data(), GL_STATIC_DRAW);

        // Position attribute (location 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

        // Normal attribute (location 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                              (void*)(3 * sizeof(float)));

        // Texture coordinate attribute (location 2)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                              (void*)(6 * sizeof(float)));

        glBindVertexArray(0);    }    meshDirty = false;
    setState(ChunkState::READY);
}

void Chunk::render(const glm::mat4& /* view */, const glm::mat4& /* projection */, const glm::vec3& cameraPos) {
    if (!isReady() || !hasGeometry) {
        return;
    }// Distance-based culling for performance
    // Calculate distance from camera to chunk center
    glm::vec3 chunkCenter = getWorldPosition() + glm::vec3(CHUNK_WIDTH * 0.5f, CHUNK_HEIGHT * 0.5f, CHUNK_DEPTH * 0.5f);
    float distance = glm::length(cameraPos - chunkCenter);

    // Skip rendering if too far away - use world's render distance setting
    float maxRenderDistance = 256.0f; // Default fallback
    if (world) {
        // Calculate max render distance based on world's render distance setting
        // Add some buffer for diagonal chunks and safety margin
        float renderDistanceChunks = static_cast<float>(world->getRenderDistance());
        float diagonalDistance = renderDistanceChunks * 1.414f; // sqrt(2) for diagonal chunks
        maxRenderDistance = diagonalDistance * CHUNK_WIDTH + (CHUNK_WIDTH * 2.0f); // Add 2 chunk buffer
    }

    if (distance > maxRenderDistance) {
        return;
    }

    // TODO: Shader management should be handled by a renderer class
    // For now, assume shader is bound externally

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

void Chunk::clearMesh() {
    if (VAO != 0) {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    vertexCount = 0;
    hasGeometry = false;
    meshDirty = true;
}

glm::vec3 Chunk::getWorldPosition() const {
    return ChunkUtils::chunkToWorldPos(coord);
}

bool Chunk::isInBounds(int x, int y, int z) const {
    return x >= 0 && x < CHUNK_WIDTH &&
           y >= 0 && y < CHUNK_HEIGHT &&
           z >= 0 && z < CHUNK_DEPTH;
}

bool Chunk::shouldRenderFace(int x, int y, int z, CubeFace face) const {
    // Calculate adjacent block position in local coordinates
    glm::ivec3 adjacentPos(x, y, z);
    switch (face) {
        case CubeFace::FRONT:  adjacentPos.z += 1; break;
        case CubeFace::BACK:   adjacentPos.z -= 1; break;
        case CubeFace::LEFT:   adjacentPos.x -= 1; break;
        case CubeFace::RIGHT:  adjacentPos.x += 1; break;
        case CubeFace::TOP:    adjacentPos.y += 1; break;
        case CubeFace::BOTTOM: adjacentPos.y -= 1; break;
    }

    // Get the current block
    BlockData currentBlock = getBlock(x, y, z);
    const Block& current = BlockRegistry::getBlock(currentBlock.type);

    // Check if adjacent position is within this chunk
    if (isInBounds(adjacentPos.x, adjacentPos.y, adjacentPos.z)) {
        BlockData adjacentBlock = getBlock(adjacentPos.x, adjacentPos.y, adjacentPos.z);

        // Use the Block class's sophisticated face culling logic
        return current.shouldRenderFace(adjacentBlock.type);
    }    // At chunk boundary - check neighboring chunks through World
    if (world) {
        // Convert current block position to world coordinates
        glm::vec3 chunkWorldPos = getWorldPosition();
        glm::ivec3 currentBlockWorldPos(
            static_cast<int>(chunkWorldPos.x) + x,  // x is the local block coordinate (0-15)
            y,
            static_cast<int>(chunkWorldPos.z) + z   // z is the local block coordinate (0-15)
        );

        // Calculate adjacent block world position by applying face offset
        glm::ivec3 worldAdjacentPos = currentBlockWorldPos;
        switch (face) {
            case CubeFace::FRONT:  worldAdjacentPos.z += 1; break;
            case CubeFace::BACK:   worldAdjacentPos.z -= 1; break;
            case CubeFace::LEFT:   worldAdjacentPos.x -= 1; break;
            case CubeFace::RIGHT:  worldAdjacentPos.x += 1; break;
            case CubeFace::TOP:    worldAdjacentPos.y += 1; break;
            case CubeFace::BOTTOM: worldAdjacentPos.y -= 1; break;
        }

        // Check if the neighboring chunk exists
        ChunkCoord neighborChunkCoord = ChunkUtils::worldToChunkCoord(worldAdjacentPos.x, worldAdjacentPos.z);
        const Chunk* neighborChunk = world->getChunk(neighborChunkCoord);        if (neighborChunk) {
            // Neighboring chunk exists, get the actual adjacent block
            BlockData adjacentBlock = world->getBlock(worldAdjacentPos.x, worldAdjacentPos.y, worldAdjacentPos.z);
            return current.shouldRenderFace(adjacentBlock.type);
        } else {
            // Neighboring chunk doesn't exist yet - use smarter assumptions
            // For water blocks specifically, we need to be more careful about face culling

            if (current.type == BlockType::WATER) {
                // Water blocks should render faces at chunk boundaries
                // This prevents the "missing water" effect when neighboring chunks aren't loaded
                return true;
            }

            // For solid blocks, make educated guesses based on height and terrain
            const int WATER_LEVEL = gTerrainSettings.waterLevel; // Use actual water level

            if (y <= WATER_LEVEL) {
                // Below/at water level
                if (face == CubeFace::TOP) {
                    // Always render top faces - important for water surface visibility
                    return true;
                } else if (face == CubeFace::BOTTOM) {
                    // Render bottom faces only near bedrock to avoid overdraw
                    return y <= 5;
                } else {
                    // For side faces below water level:
                    // - Always render transparent block faces
                    // - For solid blocks, assume neighbor might exist (conservative culling)
                    return current.isTransparent || (current.isSolid && y < WATER_LEVEL - 5);
                }
            } else {
                // Above water level - assume air in neighboring chunks (render face)
                return true;
            }
        }
    }

    // Fallback if no world pointer available
    return true;
}

void Chunk::addFace(std::vector<float>& vertices, const glm::vec3& pos,
                    CubeFace face, BlockType blockType) {

    const auto& faceVertices = FACE_VERTICES[static_cast<int>(face)];
    const glm::vec3& normal = FACE_NORMALS[static_cast<int>(face)];

    // Get texture coordinates for this block type
    const Block& block = BlockRegistry::getBlock(blockType);
    glm::vec2 texCoords = block.getTextureCoords(static_cast<int>(face));

    // Calculate texture size in UV space
    float textureSize = 1.0f / BlockRegistry::TEXTURES_PER_ROW;

    // Define UV coordinates for the quad corners
    // Note: texCoords gives us the top-left corner of the texture tile
    // Face vertices are ordered: 0=bottom-left, 1=bottom-right, 2=top-right, 3=top-left
    // OpenGL UV space: (0,0) = bottom-left, (1,1) = top-right
    // But images are loaded with (0,0) = top-left, so we need to flip Y
    // Standard UV mapping: (0,0) = top-left, (1,1) = bottom-right in texture space
    std::array<glm::vec2, 4> uvCoords = {{
        {texCoords.x, texCoords.y + textureSize},           // 0: bottom-left
        {texCoords.x + textureSize, texCoords.y + textureSize}, // 1: bottom-right
        {texCoords.x + textureSize, texCoords.y},           // 2: top-right
        {texCoords.x, texCoords.y}                            // 3: top-left
    }};

    // Create two triangles for the face (quad split)
    // Triangle 1: 0, 1, 2
    for (int i : {0, 1, 2}) {
        glm::vec3 worldPos = pos + faceVertices[i];

        // Position
        vertices.push_back(worldPos.x);
        vertices.push_back(worldPos.y);
        vertices.push_back(worldPos.z);

        // Normal
        vertices.push_back(normal.x);
        vertices.push_back(normal.y);        vertices.push_back(normal.z);

        // Texture coordinates (normal U, V)
        vertices.push_back(uvCoords[i].x);
        vertices.push_back(uvCoords[i].y);
    }

    // Triangle 2: 0, 2, 3
    for (int i : {0, 2, 3}) {
        glm::vec3 worldPos = pos + faceVertices[i];

        // Position
        vertices.push_back(worldPos.x);
        vertices.push_back(worldPos.y);
        vertices.push_back(worldPos.z);

        // Normal
        vertices.push_back(normal.x);
        vertices.push_back(normal.y);
        vertices.push_back(normal.z);

        // Texture coordinates (normal U, V)
        vertices.push_back(uvCoords[i].x);
        vertices.push_back(uvCoords[i].y);
    }
}

void Chunk::addQuadVertices(std::vector<float>& /* vertices */,
                           const std::array<glm::vec3, 4>& /* corners */,
                           const glm::vec3& /* normal */,
                           const glm::vec2& /* texCoords */) {
    // This is a helper function for more complex face generation
    // Currently using simpler addFace method
}

int Chunk::getBlockIndex(int x, int y, int z) const {
    return y * CHUNK_WIDTH * CHUNK_DEPTH + z * CHUNK_WIDTH + x;
}

glm::vec3 Chunk::indexToLocal(int index) const {
    int x = index % CHUNK_WIDTH;
    int z = (index / CHUNK_WIDTH) % CHUNK_DEPTH;
    int y = index / (CHUNK_WIDTH * CHUNK_DEPTH);
    return glm::vec3(x, y, z);
}

glm::vec3 Chunk::localToWorld(int x, int y, int z) const {
    glm::vec3 chunkWorld = getWorldPosition();
    return glm::vec3(chunkWorld.x + x, y, chunkWorld.z + z);
}

void Chunk::initializeGL() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

void Chunk::cleanupGL() {
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }

    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
}

// ChunkUtils namespace implementation
namespace ChunkUtils {

    ChunkCoord worldToChunkCoord(int worldX, int worldZ) {
        // Floor division to handle negative coordinates correctly
        int chunkX = worldX >= 0 ? worldX / CHUNK_WIDTH : (worldX - CHUNK_WIDTH + 1) / CHUNK_WIDTH;
        int chunkZ = worldZ >= 0 ? worldZ / CHUNK_DEPTH : (worldZ - CHUNK_DEPTH + 1) / CHUNK_DEPTH;
        return ChunkCoord(chunkX, chunkZ);
    }

    ChunkCoord worldToChunkCoord(const glm::vec3& worldPos) {
        return worldToChunkCoord(static_cast<int>(std::floor(worldPos.x)),
                                static_cast<int>(std::floor(worldPos.z)));
    }

    glm::ivec3 worldToLocal(int worldX, int worldY, int worldZ) {
        ChunkCoord chunkCoord = worldToChunkCoord(worldX, worldZ);
        int localX = worldX - chunkCoord.x * CHUNK_WIDTH;
        int localZ = worldZ - chunkCoord.z * CHUNK_DEPTH;

        // Ensure positive local coordinates
        if (localX < 0) localX += CHUNK_WIDTH;
        if (localZ < 0) localZ += CHUNK_DEPTH;

        return glm::ivec3(localX, worldY, localZ);
    }

    glm::ivec3 worldToLocal(const glm::vec3& worldPos) {
        return worldToLocal(static_cast<int>(std::floor(worldPos.x)),
                           static_cast<int>(std::floor(worldPos.y)),
                           static_cast<int>(std::floor(worldPos.z)));
    }

    glm::vec3 chunkToWorldPos(const ChunkCoord& coord) {
        return glm::vec3(coord.x * CHUNK_WIDTH, 0, coord.z * CHUNK_DEPTH);
    }

    float chunkDistance(const ChunkCoord& a, const ChunkCoord& b) {
        float dx = static_cast<float>(a.x - b.x);
        float dz = static_cast<float>(a.z - b.z);
        return std::sqrt(dx * dx + dz * dz);
    }

    float chunkDistanceToPoint(const ChunkCoord& chunk, const glm::vec3& point) {
        // Calculate distance to nearest point on chunk (edge-to-edge distance)
        glm::vec3 chunkMin = chunkToWorldPos(chunk);
        glm::vec3 chunkMax = chunkMin + glm::vec3(CHUNK_WIDTH, 0, CHUNK_DEPTH);

        // Clamp point to chunk bounds to find nearest point
        float nearestX = std::max(chunkMin.x, std::min(point.x, chunkMax.x));
        float nearestZ = std::max(chunkMin.z, std::min(point.z, chunkMax.z));

        // Calculate distance from player to nearest point on chunk
        float dx = point.x - nearestX;
        float dz = point.z - nearestZ;
        return std::sqrt(dx * dx + dz * dz);
    }    bool isValidChunkCoord(int /* x */, int /* z */) {
        // For infinite worlds, all chunk coordinates are valid
        // Could add limits here if needed
        return true;
    }

    bool isValidBlockCoord(int x, int y, int z) {
        return x >= 0 && x < CHUNK_WIDTH &&
               y >= 0 && y < CHUNK_HEIGHT &&
               z >= 0 && z < CHUNK_DEPTH;
    }
}

void Chunk::generate() {
#ifdef FASTNOISE_AVAILABLE
    static FastNoiseLite baseNoise;
    static FastNoiseLite mountainNoise;
    static bool initialized = false;

    // Check if we need to reset (for new seeds)
    if (g_resetChunkNoise) {
        initialized = false;
        g_resetChunkNoise = false;
    }

    if (!initialized) {
        // Base terrain: rolling hills
        baseNoise.SetSeed(gTerrainSettings.baseSeed);
        baseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        baseNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
        baseNoise.SetFractalOctaves(5);
        baseNoise.SetFractalLacunarity(2.0f);
        baseNoise.SetFractalGain(0.5f);
        baseNoise.SetRotationType3D(FastNoiseLite::RotationType3D_ImproveXZPlanes);

        // Mountain ridges
        mountainNoise.SetSeed(gTerrainSettings.mountainSeed);
        mountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        mountainNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
        mountainNoise.SetFractalOctaves(3);

        initialized = true;
    }

    baseNoise.SetFrequency(gTerrainSettings.baseFrequency);
    mountainNoise.SetFrequency(gTerrainSettings.mountainFrequency);

    int worldX0 = coord.x * CHUNK_WIDTH;
    int worldZ0 = coord.z * CHUNK_DEPTH;
    int waterLevel = gTerrainSettings.waterLevel;

    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            float worldX = float(worldX0 + x);
            float worldZ = float(worldZ0 + z);

            // Base terrain height
            float baseHeight = baseNoise.GetNoise(worldX, worldZ);
            baseHeight = baseHeight * 0.5f + 0.5f; // Normalize to [0,1]

            // Mountain detail for higher elevations
            float mountainDetail = 0.0f;
            if (baseHeight > 0.6f) {
                float rawMountainNoise = mountainNoise.GetNoise(worldX, worldZ);
                float ridged = 1.0f - std::abs(rawMountainNoise);
                ridged = std::pow(ridged, 3.0f);
                mountainDetail = ridged * (baseHeight - 0.6f) * 2.5f;
            }

            // Combine heights
            float combinedHeight = baseHeight + mountainDetail;
            if (combinedHeight > 1.5f) combinedHeight = 1.5f;

            int maxTerrainHeight = gTerrainSettings.maxTerrainHeight;
            int height = int(combinedHeight * maxTerrainHeight);

            // Generate terrain column
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                if (y > height && y <= waterLevel) {
                    setBlock(x, y, z, BlockData(BlockType::WATER));
                }
                else if (y > height) {
                    setBlock(x, y, z, BlockData(BlockType::AIR));
                }
                else {
                    bool isMountain = (height >= 50);
                    bool nearWater = (height >= waterLevel - 1) && (height <= waterLevel + 1);

                    if (y == height) {
                        // Surface layer
                        if (isMountain)
                            setBlock(x, y, z, BlockData(BlockType::STONE));
                        else if (nearWater)
                            setBlock(x, y, z, BlockData(BlockType::SAND));
                        else if (height > waterLevel + 1)
                            setBlock(x, y, z, BlockData(BlockType::GRASS));
                        else
                            setBlock(x, y, z, BlockData(BlockType::SAND));
                    }
                    else if (y >= height - 4) {
                        // Subsurface layer (up to 4 blocks deep)
                        if (isMountain)
                            setBlock(x, y, z, BlockData(BlockType::STONE));
                        else if (nearWater)
                            setBlock(x, y, z, BlockData(BlockType::SAND));
                        else if (height > waterLevel + 1)
                            setBlock(x, y, z, BlockData(BlockType::DIRT));
                        else
                            setBlock(x, y, z, BlockData(BlockType::SAND));
                    }
                    else {
                        // Deep underground - all stone
                        setBlock(x, y, z, BlockData(BlockType::STONE));
                    }
                }
            }
        }
    }

    // Set state to generated
    setState(ChunkState::GENERATED);
#else
    // Fallback to simple flat terrain if FastNoise not available
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                if (y == 0) {
                    setBlock(x, y, z, BlockData(BlockType::BEDROCK));
                } else if (y <= gTerrainSettings.waterLevel) {
                    if (y == gTerrainSettings.waterLevel) {
                        setBlock(x, y, z, BlockData(BlockType::GRASS));
                    } else {
                        setBlock(x, y, z, BlockData(BlockType::DIRT));
                    }
                } else if (y <= gTerrainSettings.waterLevel + 5) {
                    setBlock(x, y, z, BlockData(BlockType::WATER));
                } else {
                    setBlock(x, y, z, BlockData(BlockType::AIR));
                }
            }
        }
    }
    setState(ChunkState::GENERATED);
#endif
}

bool Chunk::shouldRenderWaterFace(int x, int y, int z, CubeFace face) const {
    // Optimized water face culling: only render top faces unless adjacent to air

    // Calculate adjacent block position
    glm::ivec3 adjacentPos(x, y, z);
    switch (face) {
        case CubeFace::FRONT:  adjacentPos.z += 1; break;
        case CubeFace::BACK:   adjacentPos.z -= 1; break;
        case CubeFace::LEFT:   adjacentPos.x -= 1; break;
        case CubeFace::RIGHT:  adjacentPos.x += 1; break;
        case CubeFace::TOP:    adjacentPos.y += 1; break;
        case CubeFace::BOTTOM: adjacentPos.y -= 1; break;
    }

    // Check if adjacent position is within this chunk
    if (isInBounds(adjacentPos.x, adjacentPos.y, adjacentPos.z)) {
        BlockData adjacentBlock = getBlock(adjacentPos.x, adjacentPos.y, adjacentPos.z);

        // Only render face if adjacent block is air
        // This aggressively culls underwater faces for better performance
        return adjacentBlock.type == BlockType::AIR;
    }

    // At chunk boundary - check neighboring chunks
    if (world) {
        glm::vec3 chunkWorldPos = getWorldPosition();
        glm::ivec3 worldAdjacentPos(
            static_cast<int>(chunkWorldPos.x) + adjacentPos.x,
            adjacentPos.y,
            static_cast<int>(chunkWorldPos.z) + adjacentPos.z
        );

        BlockData adjacentBlock = world->getBlock(worldAdjacentPos.x, worldAdjacentPos.y, worldAdjacentPos.z);

        // Only render if adjacent is air
        if (adjacentBlock.type == BlockType::AIR) {
            return true;
        } else {
            return false;
        }
    }

    // At chunk boundary with no neighbor loaded - only render top faces to prevent holes
    // This ensures water surface is visible while culling underwater faces
    return face == CubeFace::TOP;
}

void Chunk::checkNeighbors() {
    if (!world) return;

    // Check all 4 neighboring chunks (N, S, E, W)
    ChunkCoord neighborCoords[4] = {
        {coord.x, coord.z + 1}, // North
        {coord.x, coord.z - 1}, // South
        {coord.x + 1, coord.z}, // East
        {coord.x - 1, coord.z}  // West
    };

    bool prevNeighborState[4];
    for (int i = 0; i < 4; i++) {
        prevNeighborState[i] = neighborsAvailable[i];
        neighborsAvailable[i] = (world->getChunk(neighborCoords[i]) != nullptr);
    }

    // Check if neighbor availability changed
    bool neighborStateChanged = false;
    for (int i = 0; i < 4; i++) {
        if (prevNeighborState[i] != neighborsAvailable[i]) {
            neighborStateChanged = true;
            break;
        }
    }

    // If neighbors changed and we now have all neighbors, mark for remesh
    bool currentlyHasAllNeighbors = hasAllNeighbors();
    if (neighborStateChanged && currentlyHasAllNeighbors && !hadAllNeighbors) {
        markForRemesh();
    }

    hadAllNeighbors = currentlyHasAllNeighbors;
}

bool Chunk::hasAllNeighbors() const {
    for (int i = 0; i < 4; i++) {
        if (!neighborsAvailable[i]) {
            return false;
        }
    }
    return true;
}

void Chunk::markNeighborDirty() {
    // Reset neighbor state to force recheck
    for (int i = 0; i < 4; i++) {
        neighborsAvailable[i] = false;
    }
    hadAllNeighbors = false;
}

void Chunk::updateFromNeighbors() {
    // Get current time (simplified - in a real engine you'd get actual time)
    static float timeCounter = 0.0f;
    timeCounter += 0.016f; // Assume ~60 FPS

    // Check neighbors every 0.5 seconds
    if (timeCounter - lastNeighborCheck > 0.5f) {
        checkNeighbors();
        lastNeighborCheck = timeCounter;
    }
}

void Chunk::resetStaticNoiseGenerators() {
    // Set the global flag to force re-initialization on next generate() call
    g_resetChunkNoise = true;
}
