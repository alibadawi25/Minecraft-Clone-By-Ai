#include "sky_renderer.h"
#include <iostream>

SkyRenderer::SkyRenderer()
    : skyShader(nullptr), skyboxVAO(0), skyboxVBO(0)
{
}

SkyRenderer::~SkyRenderer()
{
    cleanup();
}

bool SkyRenderer::initialize()
{
    std::cout << "Initializing sky renderer..." << std::endl;

    // Load sky shader
    try {
        std::cout << "Loading sky shaders..." << std::endl;
        skyShader = new SimpleShader("shaders/sky.vert", "shaders/sky.frag");
        std::cout << "Sky shaders loaded successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load sky shaders: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Failed to load sky shaders: Unknown error" << std::endl;
        return false;
    }

    std::cout << "Setting up skybox geometry..." << std::endl;
    setupSkyboxGeometry();
    std::cout << "Sky renderer initialized successfully" << std::endl;
    return true;
}

void SkyRenderer::setupSkyboxGeometry()
{
    // Simple fullscreen quad vertices (much simpler than a cube)
    float skyboxVertices[] = {
        // Positions (triangle strip for full screen quad)
        -1.0f, -1.0f, 0.999f,   // Bottom left
         1.0f, -1.0f, 0.999f,   // Bottom right
        -1.0f,  1.0f, 0.999f,   // Top left
         1.0f,  1.0f, 0.999f    // Top right
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

void SkyRenderer::render(const glm::mat4& view, const glm::mat4& projection)
{
    if (!skyShader) return;

    // Disable depth testing completely for sky
    glDisable(GL_DEPTH_TEST);

    skyShader->use();

    // Render skybox
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);  // 4 vertices for triangle strip
    glBindVertexArray(0);

    // Re-enable depth testing
    glEnable(GL_DEPTH_TEST);
}

void SkyRenderer::cleanup()
{
    if (skyboxVAO) {
        glDeleteVertexArrays(1, &skyboxVAO);
        skyboxVAO = 0;
    }
    if (skyboxVBO) {
        glDeleteBuffers(1, &skyboxVBO);
        skyboxVBO = 0;
    }
    if (skyShader) {
        delete skyShader;
        skyShader = nullptr;
    }
}
