#pragma once

#include <glad/glad.h>
#include <string>

class Texture {
public:
    unsigned int ID;
    int width, height, nrChannels;

    // Constructor
    Texture();

    // Destructor
    ~Texture();

    // Load texture from file
    bool loadFromFile(const std::string& path);

    // Create a simple solid color texture
    bool createSolidColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);

    // Bind texture
    void bind(unsigned int slot = 0) const;

    // Unbind texture
    void unbind() const;

private:
    void generateTexture(unsigned char* data, int width, int height, int channels);
};
