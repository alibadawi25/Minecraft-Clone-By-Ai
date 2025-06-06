#pragma once

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

/**
 * @brief Shader class for managing OpenGL shaders
 *
 * This class handles shader compilation, linking, and uniform management
 * for vertex and fragment shaders.
 */
class Shader {
public:
    /**
     * @brief Constructor that builds the shader program from vertex and fragment shader files
     * @param vertexPath Path to the vertex shader file
     * @param fragmentPath Path to the fragment shader file
     */
    Shader(const std::string& vertexPath, const std::string& fragmentPath);

    /**
     * @brief Destructor that cleans up OpenGL resources
     */
    ~Shader();

    /**
     * @brief Use/activate the shader program
     */
    void use();

    /**
     * @brief Set a boolean uniform value
     * @param name Name of the uniform variable
     * @param value Boolean value to set
     */
    void setBool(const std::string& name, bool value) const;

    /**
     * @brief Set an integer uniform value
     * @param name Name of the uniform variable
     * @param value Integer value to set
     */
    void setInt(const std::string& name, int value) const;

    /**
     * @brief Set a float uniform value
     * @param name Name of the uniform variable
     * @param value Float value to set
     */
    void setFloat(const std::string& name, float value) const;

    /**
     * @brief Set a vec3 uniform value
     * @param name Name of the uniform variable
     * @param x X component
     * @param y Y component
     * @param z Z component
     */
    void setVec3(const std::string& name, float x, float y, float z) const;

    /**
     * @brief Set a vec3 uniform value from array
     * @param name Name of the uniform variable
     * @param value Pointer to 3 float values
     */
    void setVec3(const std::string& name, const float* value) const;

    /**
     * @brief Set a mat4 uniform value
     * @param name Name of the uniform variable
     * @param matrix Pointer to 16 float values representing a 4x4 matrix
     */
    void setMat4(const std::string& name, const float* matrix) const;

    /**
     * @brief Get the shader program ID
     * @return OpenGL program ID
     */
    GLuint getID() const { return programID; }

private:
    GLuint programID; ///< OpenGL shader program ID

    /**
     * @brief Compile a shader from source code
     * @param source Shader source code
     * @param type Shader type (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
     * @return Compiled shader ID
     */
    GLuint compileShader(const std::string& source, GLenum type);

    /**
     * @brief Load shader source code from file
     * @param path Path to the shader file
     * @return Shader source code as string
     */
    std::string loadShaderFile(const std::string& path);

    /**
     * @brief Check for shader compilation errors
     * @param shader Shader ID to check
     * @param type Shader type string for error reporting
     */
    void checkCompileErrors(GLuint shader, const std::string& type);

    /**
     * @brief Get uniform location with caching
     * @param name Uniform variable name
     * @return Uniform location
     */
    GLint getUniformLocation(const std::string& name) const;
};
