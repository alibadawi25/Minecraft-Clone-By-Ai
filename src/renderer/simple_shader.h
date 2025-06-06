#pragma once

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

class SimpleShader {
public:
    unsigned int shaderProgram;

    SimpleShader(const std::string& vertexPath, const std::string& fragmentPath);
    ~SimpleShader();    void use();
    void setFloat(const std::string& name, float value);
    void setInt(const std::string& name, int value);
    void setVector3(const std::string& name, const glm::vec3& value);
    void setMatrix4(const std::string& name, const glm::mat4& matrix);

private:
    std::string loadShaderFromFile(const std::string& path);
    unsigned int compileShader(const std::string& source, GLenum type);
};
