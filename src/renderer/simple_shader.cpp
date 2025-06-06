#include "simple_shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

SimpleShader::SimpleShader(const std::string& vertexPath, const std::string& fragmentPath) {
    // Load shader source code
    std::string vertexCode = loadShaderFromFile(vertexPath);
    std::string fragmentCode = loadShaderFromFile(fragmentPath);

    // Compile shaders
    unsigned int vertex = compileShader(vertexCode, GL_VERTEX_SHADER);
    unsigned int fragment = compileShader(fragmentCode, GL_FRAGMENT_SHADER);

    // Create shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex);
    glAttachShader(shaderProgram, fragment);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // Clean up individual shaders
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

SimpleShader::~SimpleShader() {
    glDeleteProgram(shaderProgram);
}

void SimpleShader::use() {
    glUseProgram(shaderProgram);
}

void SimpleShader::setFloat(const std::string& name, float value) {
    int location = glGetUniformLocation(shaderProgram, name.c_str());
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void SimpleShader::setInt(const std::string& name, int value) {
    int location = glGetUniformLocation(shaderProgram, name.c_str());
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void SimpleShader::setVector3(const std::string& name, const glm::vec3& value) {
    int location = glGetUniformLocation(shaderProgram, name.c_str());
    if (location != -1) {
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
}

void SimpleShader::setMatrix4(const std::string& name, const glm::mat4& matrix) {
    int location = glGetUniformLocation(shaderProgram, name.c_str());
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }
}

std::string SimpleShader::loadShaderFromFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int SimpleShader::compileShader(const std::string& source, GLenum type) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    // Check for compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}
