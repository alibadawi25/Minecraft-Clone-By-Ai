#include "buffer.h"
#include <iostream>

VertexBuffer::VertexBuffer(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : indexCount(indices.size()) {

    // Generate OpenGL objects
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Bind VAO first
    glBindVertexArray(VAO);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Setup vertex attributes
    setupVertexAttributes();

    // Unbind VAO
    glBindVertexArray(0);

    std::cout << "VertexBuffer created: " << vertices.size() << " vertices, " << indices.size() << " indices" << std::endl;
}

VertexBuffer::~VertexBuffer() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void VertexBuffer::bind() {
    glBindVertexArray(VAO);
}

void VertexBuffer::unbind() {
    glBindVertexArray(0);
}

void VertexBuffer::draw() {
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, 0);
}

void VertexBuffer::setupVertexAttributes() {
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    // Texture coordinate attribute (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    glEnableVertexAttribArray(2);
}

void GeometryGenerator::generateCube(float size, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    vertices.clear();
    indices.clear();

    float halfSize = size * 0.5f;

    // Define cube vertices (position, normal, texCoords)
    // Each face has 4 vertices

    // Front face (z = +halfSize)
    vertices.emplace_back(-halfSize, -halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f);
    vertices.emplace_back( halfSize, -halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f);
    vertices.emplace_back( halfSize,  halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f);
    vertices.emplace_back(-halfSize,  halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f);

    // Back face (z = -halfSize)
    vertices.emplace_back(-halfSize, -halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f);
    vertices.emplace_back(-halfSize,  halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f);
    vertices.emplace_back( halfSize,  halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f);
    vertices.emplace_back( halfSize, -halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f);

    // Left face (x = -halfSize)
    vertices.emplace_back(-halfSize, -halfSize, -halfSize, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f);
    vertices.emplace_back(-halfSize, -halfSize,  halfSize, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f);
    vertices.emplace_back(-halfSize,  halfSize,  halfSize, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f);
    vertices.emplace_back(-halfSize,  halfSize, -halfSize, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f);

    // Right face (x = +halfSize)
    vertices.emplace_back( halfSize, -halfSize, -halfSize,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f);
    vertices.emplace_back( halfSize,  halfSize, -halfSize,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f);
    vertices.emplace_back( halfSize,  halfSize,  halfSize,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f);
    vertices.emplace_back( halfSize, -halfSize,  halfSize,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f);

    // Bottom face (y = -halfSize)
    vertices.emplace_back(-halfSize, -halfSize, -halfSize,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f);
    vertices.emplace_back( halfSize, -halfSize, -halfSize,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f);
    vertices.emplace_back( halfSize, -halfSize,  halfSize,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f);
    vertices.emplace_back(-halfSize, -halfSize,  halfSize,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f);

    // Top face (y = +halfSize)
    vertices.emplace_back(-halfSize,  halfSize, -halfSize,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f);
    vertices.emplace_back(-halfSize,  halfSize,  halfSize,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f);
    vertices.emplace_back( halfSize,  halfSize,  halfSize,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f);
    vertices.emplace_back( halfSize,  halfSize, -halfSize,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f);

    // Define indices for each face (2 triangles per face)
    uint32_t faceIndices[] = {
        // Front face
        0, 1, 2,   2, 3, 0,
        // Back face
        4, 5, 6,   6, 7, 4,
        // Left face
        8, 9, 10,  10, 11, 8,
        // Right face
        12, 13, 14, 14, 15, 12,
        // Bottom face
        16, 17, 18, 18, 19, 16,
        // Top face
        20, 21, 22, 22, 23, 20
    };

    indices.assign(faceIndices, faceIndices + sizeof(faceIndices) / sizeof(uint32_t));
}

void GeometryGenerator::generateTexturedCube(float size, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    // For now, this is the same as generateCube
    // In the future, this could have different UV mapping
    generateCube(size, vertices, indices);
}
