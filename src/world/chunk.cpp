#include "chunk.h"
#include "world.h"
#include "block.h"
#include <iostream>
#include <algorithm>
#include <cmath>

// Face vertices for cube mesh generation (in local coordinates)
const std::array<std::array<glm::vec3, 4>, 6> Chunk::FACE_VERTICES = {{
    // FRONT
    {{
        {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}
    }},
    // BACK
    {{
        {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}
    }},
    // LEFT
    {{
        {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}
    }},
    // RIGHT
    {{
        {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}
    }},
    // TOP
    {{
        {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}
    }},
    // BOTTOM
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
      vertexCount(0), meshDirty(true), hasGeometry(false) {

    // Initialize all blocks to air
    blocks.fill(BlockData(BlockType::AIR));    // Initialize OpenGL resources
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
    }

    setState(ChunkState::MESHING);

    std::vector<float> vertices;
    vertices.reserve(BLOCKS_PER_CHUNK * 6 * 4 * 8); // Estimate max vertices

    // Iterate through all blocks in the chunk
    for (int y = 0; y < CHUNK_HEIGHT; y++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            for (int x = 0; x < CHUNK_WIDTH; x++) {
                BlockData blockData = getBlock(x, y, z);

                // Skip air blocks
                if (blockData.type == BlockType::AIR) {
                    continue;
                }

                const Block& block = BlockRegistry::getBlock(blockData.type);

                // Skip non-solid blocks for now (transparent blocks need special handling)
                if (!block.isSolid) {
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

void Chunk::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
    if (!isReady() || !hasGeometry) {
        return;
    }

    // Distance-based culling for performance
    // Calculate distance from camera to chunk center
    glm::vec3 chunkCenter = getWorldPosition() + glm::vec3(CHUNK_WIDTH * 0.5f, CHUNK_HEIGHT * 0.5f, CHUNK_DEPTH * 0.5f);
    float distance = glm::length(cameraPos - chunkCenter);

    // Skip rendering if too far away (adjust this value based on performance needs)
    const float MAX_RENDER_DISTANCE = 256.0f; // About 16 chunks in each direction
    if (distance > MAX_RENDER_DISTANCE) {
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
    }

    // At chunk boundary, always render the face (no cross-chunk culling for now)
    // TODO: Implement cross-chunk face culling by checking neighboring chunks
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

void Chunk::addQuadVertices(std::vector<float>& vertices,
                           const std::array<glm::vec3, 4>& corners,
                           const glm::vec3& normal,
                           const glm::vec2& texCoords) {
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
    }

    bool isValidChunkCoord(int x, int z) {
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
