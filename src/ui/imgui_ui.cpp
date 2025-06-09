#include "imgui_ui.h"
#include "../world/world.h"
#include "../renderer/camera.h"
#include <iostream>
#include <glm/glm.hpp>

ImGuiUI::ImGuiUI() = default;

ImGuiUI::~ImGuiUI() {
    if (m_initialized) {
        shutdown();
    }
}

bool ImGuiUI::initialize(GLFWwindow* window) {
    if (m_initialized) {
        return true;
    }

    m_window = window;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    const char* glsl_version = "#version 330";
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::cerr << "Failed to initialize ImGui GLFW backend" << std::endl;
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl;
        ImGui_ImplGlfw_Shutdown();
        return false;
    }

    m_initialized = true;
    return true;
}

void ImGuiUI::shutdown() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_initialized = false;
    m_window = nullptr;
}

void ImGuiUI::newFrame() {
    if (!m_initialized) {
        return;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiUI::render() {
    if (!m_initialized) {
        return;
    }

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiUI::renderRenderDistanceControl(World* world) {
    if (!world) return;

    ImGui::Begin("Render Distance Control");

    // Get current render distance from world
    int currentDistance = world->getRenderDistance();

    // Display current render distance
    ImGui::Text("Current Render Distance: %d chunks", currentDistance);
    ImGui::Separator();

    // Slider for render distance (range: 1-32)
    int newDistance = currentDistance;
    if (ImGui::SliderInt("Render Distance", &newDistance, 1, 32)) {
        if (newDistance != currentDistance) {
            world->setRenderDistance(newDistance);
        }
    }

    ImGui::Separator();

    // Quick preset buttons
    ImGui::Text("Quick Presets:");

    if (ImGui::Button("Tiny (4)")) {
        world->setRenderDistance(4);
    }
    ImGui::SameLine();

    if (ImGui::Button("Small (8)")) {
        world->setRenderDistance(8);
    }
    ImGui::SameLine();

    if (ImGui::Button("Normal (12)")) {
        world->setRenderDistance(12);
    }
    ImGui::SameLine();

    if (ImGui::Button("Far (16)")) {
        world->setRenderDistance(16);
    }
    ImGui::SameLine();

    if (ImGui::Button("Extreme (24)")) {
        world->setRenderDistance(24);
    }

    ImGui::Separator();
    ImGui::Text("Performance Impact:");

    // Calculate chunks in view (approximate)
    int chunksInView = (2 * currentDistance + 1) * (2 * currentDistance + 1);
    ImGui::Text("Approximate chunks in view: %d", chunksInView);

    // Performance warning
    if (currentDistance > 20) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Warning: High render distance may impact performance!");
    } else if (currentDistance > 16) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "Caution: Medium-high render distance");
    } else {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Good performance expected");
    }

    ImGui::End();
}

void ImGuiUI::renderDebugWindow(double fps, World* world, Camera* camera) {
    if (!world || !camera) return;

    ImGui::Begin("Debug Information");

    ImGui::Text("Performance:");
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Frame Time: %.3f ms", 1000.0 / fps);

    ImGui::Separator();

    ImGui::Text("World:");
    ImGui::Text("Render Distance: %d chunks", world->getRenderDistance());
    ImGui::Text("Loaded Chunks: %d", world->getLoadedChunkCount());

    ImGui::Separator();

    ImGui::Text("Player Position:");
    glm::vec3 pos = camera->getPosition();
    ImGui::Text("X: %.2f", pos.x);
    ImGui::Text("Y: %.2f", pos.y);
    ImGui::Text("Z: %.2f", pos.z);

    ImGui::Separator();

    ImGui::Text("Camera:");
    ImGui::Text("FOV: %.1f degrees", camera->getFOV());
    glm::vec3 front = camera->getFront();
    ImGui::Text("Direction: (%.2f, %.2f, %.2f)", front.x, front.y, front.z);

    ImGui::Separator();

    ImGui::Text("Controls:");
    ImGui::Text("F1: Toggle UI");
    ImGui::Text("WASD: Move");
    ImGui::Text("Mouse: Look around");
    ImGui::Text("Scroll: Change FOV");
    ImGui::Text("Space: Fly up / Jump");
    ImGui::Text("Shift: Fly down / Sneak");

    ImGui::End();
}
