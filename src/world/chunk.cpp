#include "chunk.h"
#include "world.h"
#include "block.h"
#include <iostream>
#include <algorithm>
#include <cmath>

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

                const Block& block = BlockRegistry::getBlock(blockData.type);

                // SECOND PASS: Only render transparent blocks
                if (!block.isTransparent) {
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
        const Chunk* neighborChunk = world->getChunk(neighborChunkCoord);

        if (neighborChunk) {
            // Neighboring chunk exists, get the actual adjacent block
            BlockData adjacentBlock = world->getBlock(worldAdjacentPos.x, worldAdjacentPos.y, worldAdjacentPos.z);
            return current.shouldRenderFace(adjacentBlock.type);
        } else {
            // Neighboring chunk doesn't exist yet - check if we should generate terrain assumption
            // For cross-chunk face culling during initial generation, we need to make an educated guess
            // about what terrain would be in the neighboring chunk

            // If we're at or below sea level and the current block is solid, assume neighbor might be solid too
            // This prevents underwater terrain from having too many exposed faces during initial generation
            const int SEA_LEVEL = 64; // Should match world generation

            if (y <= SEA_LEVEL && current.isSolid) {
                // For solid blocks at/below sea level, be more conservative about rendering faces
                // This reduces visual artifacts during initial chunk loading
                if (face == CubeFace::TOP) {
                    // Always render top faces - sky is always visible
                    return true;
                } else if (face == CubeFace::BOTTOM) {
                    // Usually don't render bottom faces underground
                    return y <= 5; // Only render near bedrock level
                } else {
                    // For side faces, assume neighbor might be solid (conservative approach)
                    // This reduces flickering during chunk loading
                    return current.isTransparent; // Only render if current block is transparent
                }
            } else {
                // Above sea level or non-solid blocks - default to rendering the face
                // This ensures proper terrain visibility and water transparency
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
