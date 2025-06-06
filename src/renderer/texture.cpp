#include "texture.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../../external/stb/stb_image.h"

Texture::Texture() : ID(0), width(0), height(0), nrChannels(0) {
    glGenTextures(1, &ID);
}

Texture::~Texture() {
    if (ID != 0) {
        glDeleteTextures(1, &ID);
    }
}

bool Texture::loadFromFile(const std::string& path) {
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return false;
    }

    generateTexture(data, width, height, nrChannels);
    stbi_image_free(data);

    std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ", " << nrChannels << " channels)" << std::endl;
    return true;
}

bool Texture::createSolidColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    unsigned char data[4] = {r, g, b, a};
    width = 1;
    height = 1;
    nrChannels = 4;

    generateTexture(data, 1, 1, 4);

    std::cout << "Created solid color texture: RGB(" << (int)r << ", " << (int)g << ", " << (int)b << ", " << (int)a << ")" << std::endl;
    return true;
}

void Texture::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::generateTexture(unsigned char* data, int width, int height, int channels) {
    glBindTexture(GL_TEXTURE_2D, ID);

    GLenum format = GL_RGB;
    if (channels == 1)
        format = GL_RED;
    else if (channels == 3)
        format = GL_RGB;
    else if (channels == 4)
        format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}
