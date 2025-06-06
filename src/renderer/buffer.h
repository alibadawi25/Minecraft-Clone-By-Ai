#pragma once

#include <glad/glad.h>
#include <vector>

/**
 * @brief Vertex structure containing position, normal, and texture coordinates
 */
struct Vertex {
    float position[3];  ///< Position (x, y, z)
    float normal[3];    ///< Normal vector (x, y, z)
    float texCoords[2]; ///< Texture coordinates (u, v)

    /**
     * @brief Constructor for Vertex
     */
    Vertex(float px, float py, float pz, float nx, float ny, float nz, float u, float v) {
        position[0] = px; position[1] = py; position[2] = pz;
        normal[0] = nx; normal[1] = ny; normal[2] = nz;
        texCoords[0] = u; texCoords[1] = v;
    }
};

/**
 * @brief Vertex Buffer Object wrapper for efficient geometry management
 */
class VertexBuffer {
public:
    /**
     * @brief Constructor that creates VBO/VAO from vertex and index data
     * @param vertices Vector of vertex data
     * @param indices Vector of index data for indexed drawing
     */
    VertexBuffer(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    /**
     * @brief Destructor that cleans up OpenGL resources
     */
    ~VertexBuffer();

    /**
     * @brief Bind the VAO for rendering
     */
    void bind();

    /**
     * @brief Unbind the VAO
     */
    void unbind();

    /**
     * @brief Draw the geometry using indexed rendering
     */
    void draw();

    /**
     * @brief Get the number of indices
     * @return Number of indices in the buffer
     */
    size_t getIndexCount() const { return indexCount; }

    /**
     * @brief Get the VAO ID
     * @return OpenGL VAO ID
     */
    GLuint getVAO() const { return VAO; }

private:
    GLuint VAO;        ///< Vertex Array Object ID
    GLuint VBO;        ///< Vertex Buffer Object ID
    GLuint EBO;        ///< Element Buffer Object ID
    size_t indexCount; ///< Number of indices

    /**
     * @brief Setup vertex attributes
     */
    void setupVertexAttributes();
};

/**
 * @brief Utility class for generating common 3D geometry
 */
class GeometryGenerator {
public:
    /**
     * @brief Generate a cube with specified size
     * @param size Size of the cube (default: 1.0f)
     * @param vertices Output vector for vertex data
     * @param indices Output vector for index data
     */
    static void generateCube(float size, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

    /**
     * @brief Generate a textured cube with proper UVs
     * @param size Size of the cube
     * @param vertices Output vector for vertex data
     * @param indices Output vector for index data
     */
    static void generateTexturedCube(float size, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
};
