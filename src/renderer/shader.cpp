#include "shader.h"
#include <iostream>

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    // Load shader source code from files
    std::string vertexSource = loadShaderFile(vertexPath);
    std::string fragmentSource = loadShaderFile(fragmentPath);

    // Compile shaders
    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);

    // Create shader program
    programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    // Check for linking errors
    checkCompileErrors(programID, "PROGRAM");

    // Clean up individual shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::cout << "Shader program compiled successfully. ID: " << programID << std::endl;
}

Shader::~Shader() {
    if (programID != 0) {
        glDeleteProgram(programID);
    }
}

void Shader::use() {
    glUseProgram(programID);
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(getUniformLocation(name), (int)value);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(getUniformLocation(name), x, y, z);
}

void Shader::setVec3(const std::string& name, const float* value) const {
    glUniform3fv(getUniformLocation(name), 1, value);
}

void Shader::setMat4(const std::string& name, const float* matrix) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, matrix);
}

GLuint Shader::compileShader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    // Check for compilation errors
    std::string typeStr = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
    checkCompileErrors(shader, typeStr);

    return shader;
}

std::string Shader::loadShaderFile(const std::string& path) {
    std::string shaderCode;
    std::ifstream shaderFile;

    // Ensure ifstream objects can throw exceptions
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        // Open file
        shaderFile.open(path);
        std::stringstream shaderStream;

        // Read file's buffer contents into stream
        shaderStream << shaderFile.rdbuf();

        // Close file handler
        shaderFile.close();

        // Convert stream into string
        shaderCode = shaderStream.str();

        std::cout << "Loaded shader: " << path << " (" << shaderCode.length() << " characters)" << std::endl;
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << path << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return shaderCode;
}

void Shader::checkCompileErrors(GLuint shader, const std::string& type) {
    GLint success;
    GLchar infoLog[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << std::endl;
            std::cerr << infoLog << std::endl;
            std::cerr << "-- --------------------------------------------------- --" << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << std::endl;
            std::cerr << infoLog << std::endl;
            std::cerr << "-- --------------------------------------------------- --" << std::endl;
        }
    }
}

GLint Shader::getUniformLocation(const std::string& name) const {
    GLint location = glGetUniformLocation(programID, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name << "' doesn't exist!" << std::endl;
    }
    return location;
}
