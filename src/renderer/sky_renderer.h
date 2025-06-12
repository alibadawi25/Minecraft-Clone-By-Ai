#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "simple_shader.h"

class SkyRenderer
{
public:
    SkyRenderer();
    ~SkyRenderer();

    bool initialize();
    void render(const glm::mat4& view, const glm::mat4& projection);
    void cleanup();

private:
    SimpleShader* skyShader;
    unsigned int skyboxVAO;
    unsigned int skyboxVBO;

    void setupSkyboxGeometry();
};
