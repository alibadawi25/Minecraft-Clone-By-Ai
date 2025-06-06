#include "block.h"
#include <iostream>
#include <vector>
#include <stb_image.h>
#include <unordered_map>

// Initialize static members
std::array<Block, static_cast<size_t>(BlockType::COUNT)> BlockRegistry::blocks;
GLuint BlockRegistry::textureAtlasID = 0;
bool BlockRegistry::initialized = false;

Block::Block(BlockType type, const std::string& name, bool solid, bool transparent, float hardness)
    : type(type), name(name), isSolid(solid), isTransparent(transparent), hardness(hardness) {
    // Initialize all faces to same texture coordinates (will be set properly in BlockRegistry)
    for (int i = 0; i < 6; ++i) {
        textureCoords[i] = glm::vec2(0.0f, 0.0f);
    }
}

bool Block::shouldRenderFace(BlockType neighborType) const {
    if (neighborType == BlockType::AIR) {
        return true; // Always render faces adjacent to air
    }

    const Block& neighbor = BlockRegistry::getBlock(neighborType);

    // Don't render face if neighbor is solid and opaque
    if (neighbor.isSolid && !neighbor.isTransparent) {
        return false;
    }

    // Don't render face between identical transparent blocks
    if (isTransparent && neighbor.isTransparent && type == neighborType) {
        return false;
    }

    return true;
}

glm::vec2 Block::getTextureCoords(int face) const {
    if (face >= 0 && face < 6) {
        return textureCoords[face];
    }
    return glm::vec2(0.0f, 0.0f);
}

void BlockRegistry::initialize() {
    if (initialized) return;

    std::cout << "Initializing Block Registry..." << std::endl;

    // Initialize all blocks
    loadBlockTextures();
    createTextureAtlas();

    // Define block properties
    blocks[static_cast<size_t>(BlockType::AIR)] = Block(
        BlockType::AIR, "Air", false, true, 0.0f
    );

    blocks[static_cast<size_t>(BlockType::GRASS)] = Block(
        BlockType::GRASS, "Grass", true, false, 0.6f
    );

    blocks[static_cast<size_t>(BlockType::DIRT)] = Block(
        BlockType::DIRT, "Dirt", true, false, 0.5f
    );

    blocks[static_cast<size_t>(BlockType::STONE)] = Block(
        BlockType::STONE, "Stone", true, false, 1.5f
    );

    blocks[static_cast<size_t>(BlockType::WATER)] = Block(
        BlockType::WATER, "Water", false, true, 0.0f
    );

    blocks[static_cast<size_t>(BlockType::SAND)] = Block(
        BlockType::SAND, "Sand", true, false, 0.5f
    );

    blocks[static_cast<size_t>(BlockType::WOOD)] = Block(
        BlockType::WOOD, "Wood", true, false, 2.0f
    );

    blocks[static_cast<size_t>(BlockType::LEAVES)] = Block(
        BlockType::LEAVES, "Leaves", true, true, 0.2f
    );

    blocks[static_cast<size_t>(BlockType::COBBLESTONE)] = Block(
        BlockType::COBBLESTONE, "Cobblestone", true, false, 2.0f
    );

    blocks[static_cast<size_t>(BlockType::BEDROCK)] = Block(
        BlockType::BEDROCK, "Bedrock", true, false, -1.0f // Unbreakable
    );

    // Set up texture coordinates for each block
    setupBlockTextures();

    initialized = true;
    std::cout << "Block Registry initialized with " << static_cast<int>(BlockType::COUNT) << " block types." << std::endl;
}

void BlockRegistry::shutdown() {
    if (textureAtlasID != 0) {
        glDeleteTextures(1, &textureAtlasID);
        textureAtlasID = 0;
    }
    initialized = false;
}

const Block& BlockRegistry::getBlock(BlockType type) {
    if (!initialized) {
        initialize();
    }

    size_t index = static_cast<size_t>(type);
    if (index < blocks.size()) {
        return blocks[index];
    }

    // Return air block as default
    return blocks[0];
}

BlockType BlockRegistry::getBlockType(const std::string& name) {
    if (!initialized) {
        initialize();
    }

    for (size_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].name == name) {
            return static_cast<BlockType>(i);
        }
    }

    return BlockType::AIR;
}

GLuint BlockRegistry::getTextureAtlas() {
    if (!initialized) {
        initialize();
    }
    return textureAtlasID;
}

void BlockRegistry::createTextureAtlas() {
    // For now, create a simple procedural texture atlas
    // In a real implementation, this would load actual texture files

    const int atlasSize = ATLAS_SIZE;
    const int textureSize = TEXTURE_SIZE;
    const int texturesPerRow = TEXTURES_PER_ROW;

    // Create atlas data (RGBA)
    std::vector<unsigned char> atlasData(atlasSize * atlasSize * 4);

    // Generate simple colored textures for each block type
    std::vector<glm::vec3> blockColors = {
        glm::vec3(0.0f, 0.0f, 0.0f),       // AIR (transparent)
        glm::vec3(0.4f, 0.7f, 0.2f),       // GRASS (green)
        glm::vec3(0.6f, 0.4f, 0.2f),       // DIRT (brown)
        glm::vec3(0.5f, 0.5f, 0.5f),       // STONE (gray)
        glm::vec3(0.2f, 0.4f, 0.8f),       // WATER (blue)
        glm::vec3(0.9f, 0.8f, 0.6f),       // SAND (tan)
        glm::vec3(0.6f, 0.3f, 0.1f),       // WOOD (brown)
        glm::vec3(0.2f, 0.6f, 0.2f),       // LEAVES (dark green)
        glm::vec3(0.4f, 0.4f, 0.4f),       // COBBLESTONE (dark gray)
        glm::vec3(0.1f, 0.1f, 0.1f)        // BEDROCK (black)
    };

    for (int blockIndex = 0; blockIndex < static_cast<int>(BlockType::COUNT); ++blockIndex) {
        int texX = (blockIndex % texturesPerRow) * textureSize;
        int texY = (blockIndex / texturesPerRow) * textureSize;

        glm::vec3 color = blockColors[blockIndex];

        for (int y = 0; y < textureSize; ++y) {
            for (int x = 0; x < textureSize; ++x) {
                int pixelX = texX + x;
                int pixelY = texY + y;
                int index = (pixelY * atlasSize + pixelX) * 4;

                // Add some texture variation
                float variation = 0.9f + 0.2f * ((x + y) % 3) / 3.0f;

                atlasData[index + 0] = static_cast<unsigned char>(color.r * 255 * variation); // R
                atlasData[index + 1] = static_cast<unsigned char>(color.g * 255 * variation); // G
                atlasData[index + 2] = static_cast<unsigned char>(color.b * 255 * variation); // B
                atlasData[index + 3] = (blockIndex == 0) ? 0 : 255; // A (transparent for air)
            }
        }
    }

    // Create OpenGL texture
    glGenTextures(1, &textureAtlasID);
    glBindTexture(GL_TEXTURE_2D, textureAtlasID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasSize, atlasSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlasData.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Created texture atlas (ID: " << textureAtlasID << ") with size " << atlasSize << "x" << atlasSize << std::endl;
}

void BlockRegistry::loadBlockTextures() {
    // This will be expanded later to load actual texture files
    // For now, we'll use the procedural textures created in createTextureAtlas()
}

glm::vec2 BlockRegistry::calculateTextureCoords(int textureIndex) {
    float u = (textureIndex % TEXTURES_PER_ROW) * (1.0f / TEXTURES_PER_ROW);
    float v = (textureIndex / TEXTURES_PER_ROW) * (1.0f / TEXTURES_PER_ROW);
    return glm::vec2(u, v);
}

void BlockRegistry::setupBlockTextures() {
    // Set up texture coordinates for each block type
    // For now, each block uses the same texture on all faces

    for (int i = 0; i < static_cast<int>(BlockType::COUNT); ++i) {
        glm::vec2 texCoords = calculateTextureCoords(i);

        // Set all faces to use the same texture coordinates
        for (int face = 0; face < 6; ++face) {
            blocks[i].textureCoords[face] = texCoords;
        }
    }

    // Special case for grass: different texture for top face
    Block& grass = blocks[static_cast<size_t>(BlockType::GRASS)];
    grass.textureCoords[Block::TOP] = calculateTextureCoords(static_cast<int>(BlockType::GRASS));
    grass.textureCoords[Block::BOTTOM] = calculateTextureCoords(static_cast<int>(BlockType::DIRT));
    // Sides can remain grass texture
}
