#pragma once

#include <glad/glad.h>  // Must be included before any OpenGL headers
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <functional>

// Forward declarations
class World;
class Camera;

class ImGuiUI {
public:
    ImGuiUI();
    ~ImGuiUI();

    bool initialize(GLFWwindow* window);
    void shutdown();
    void newFrame();
    void render();    // UI rendering methods
    void renderRenderDistanceControl(class World* world);
    void renderDebugWindow(double fps, class World* world, class Camera* camera);

private:
    bool m_initialized = false;
    GLFWwindow* m_window = nullptr;
};
