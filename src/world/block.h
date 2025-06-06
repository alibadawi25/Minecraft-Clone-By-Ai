#pragma once

#include <string>
#include <array>
#include <glm/glm.hpp>
#include <glad/glad.h>

enum class BlockType : uint8_t {
    AIR = 0,
    GRASS,
    DIRT,
    STONE,
    WATER,
    SAND,
    WOOD,
    LEAVES,
    COBBLESTONE,
    BEDROCK,
    COUNT
};

struct BlockData {
    BlockType type = BlockType::AIR;
    uint8_t metadata = 0; // For rotation, water level, etc.

    BlockData() = default;
    BlockData(BlockType t, uint8_t meta = 0) : type(t), metadata(meta) {}

    bool operator==(const BlockData& other) const {
        return type == other.type && metadata == other.metadata;
    }

    bool operator!=(const BlockData& other) const {
        return !(*this == other);
    }
};

class Block {
public:
    BlockType type;
    std::string name;
    bool isSolid;
    bool isTransparent;
    float hardness;
    glm::vec2 textureCoords[6]; // 6 faces: front, back, left, right, top, bottom

    Block() = default;
    Block(BlockType type, const std::string& name, bool solid = true,
          bool transparent = false, float hardness = 1.0f);

    // Check if a face should be rendered when adjacent to another block type
    bool shouldRenderFace(BlockType neighborType) const;

    // Get texture coordinates for a specific face (0-5)
    glm::vec2 getTextureCoords(int face) const;

    // Face indices
    enum Face {
        FRONT = 0,
        BACK = 1,
        LEFT = 2,
        RIGHT = 3,
        TOP = 4,
        BOTTOM = 5
    };
};

class BlockRegistry {
public:
    static void initialize();
    static void shutdown();

    static const Block& getBlock(BlockType type);
    static BlockType getBlockType(const std::string& name);
    static GLuint getTextureAtlas();

    // Texture atlas properties
    static constexpr int ATLAS_SIZE = 256;
    static constexpr int TEXTURE_SIZE = 16;
    static constexpr int TEXTURES_PER_ROW = ATLAS_SIZE / TEXTURE_SIZE;

private:
    static std::array<Block, static_cast<size_t>(BlockType::COUNT)> blocks;
    static GLuint textureAtlasID;
    static bool initialized;

    static void createTextureAtlas();
    static void loadBlockTextures();
    static void setupBlockTextures();
    static glm::vec2 calculateTextureCoords(int textureIndex);
};
