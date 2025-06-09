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

    ImGui::Separator();    ImGui::Text("Controls:");
    ImGui::Text("F1: Toggle UI");
    ImGui::Text("WASD: Move");
    ImGui::Text("Mouse: Look around");
    ImGui::Text("Scroll: Change FOV");
    ImGui::Text("Space: Fly up / Jump");
    ImGui::Text("Shift: Fly down / Sneak");
    ImGui::Text("Left Click: Remove Block");
    ImGui::Text("Right Click: Place Block");

    ImGui::End();
}

void ImGuiUI::renderBlockInteractionUI(World* world, Camera* camera, int selectedBlockType) {
    if (!world || !camera) return;

    ImGui::Begin("Block Interaction");

    ImGui::Text("Block Interaction Controls:");
    ImGui::Separator();

    ImGui::Text("Left Click: Remove Block");
    ImGui::Text("Right Click: Place Block");
    ImGui::Separator();

    // Show current selected block type
    const char* selectedBlockName = "Unknown";
    switch(selectedBlockType) {
        case 0: selectedBlockName = "Dirt"; break;     // BlockType::DIRT
        case 1: selectedBlockName = "Grass"; break;    // BlockType::GRASS
        case 2: selectedBlockName = "Stone"; break;    // BlockType::STONE
        case 6: selectedBlockName = "Wood"; break;     // BlockType::WOOD
        case 7: selectedBlockName = "Leaves"; break;   // BlockType::LEAVES
        default: selectedBlockName = "Unknown"; break;
    }
    ImGui::Text("Selected Block: %s (Press 1-5 to change)", selectedBlockName);

    ImGui::Separator();    // Cast a ray to show what block we're looking at
    World::RaycastResult result = world->raycast(camera->getPosition(), camera->getFront(), 10.0f);

    if (result.hit) {
        ImGui::Text("Looking at Block:");
        ImGui::Text("Position: (%d, %d, %d)", result.blockPos.x, result.blockPos.y, result.blockPos.z);
        ImGui::Text("Distance: %.2f", result.distance);        // Show block type - only show if it's not AIR (which shouldn't happen due to raycasting logic)
        const char* blockTypeName = "Unknown";
        switch(result.block.type) {
            case BlockType::AIR: blockTypeName = "Air (ERROR!)"; break;
            case BlockType::GRASS: blockTypeName = "Grass"; break;
            case BlockType::DIRT: blockTypeName = "Dirt"; break;
            case BlockType::STONE: blockTypeName = "Stone"; break;
            case BlockType::WATER: blockTypeName = "Water"; break;
            case BlockType::SAND: blockTypeName = "Sand"; break;
            case BlockType::WOOD: blockTypeName = "Wood"; break;
            case BlockType::LEAVES: blockTypeName = "Leaves"; break;
            case BlockType::COBBLESTONE: blockTypeName = "Cobblestone"; break;
            case BlockType::BEDROCK: blockTypeName = "Bedrock"; break;
            default: blockTypeName = "Unknown"; break;
        }
        ImGui::Text("Block Type: %s", blockTypeName);

        // Show placement position for right-click
        ImGui::Separator();
        ImGui::Text("Placement Position: (%d, %d, %d)",
                   result.adjacentPos.x, result.adjacentPos.y, result.adjacentPos.z);
    } else {
        ImGui::Text("No block in range");
    }

    ImGui::Separator();

    // Quick debug info
    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)",
               camera->getPosition().x, camera->getPosition().y, camera->getPosition().z);
    glm::vec3 front = camera->getFront();
    ImGui::Text("Looking Direction: (%.2f, %.2f, %.2f)", front.x, front.y, front.z);

    ImGui::End();
}

void ImGuiUI::renderCrosshair() {
    // Get the main viewport
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 center = ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f);

    // Get the foreground draw list to draw on top of everything
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();

    // Crosshair properties
    float size = 10.0f;
    float thickness = 2.0f;
    ImU32 color = IM_COL32(255, 255, 255, 200); // White with slight transparency

    // Draw crosshair
    draw_list->AddLine(ImVec2(center.x - size, center.y), ImVec2(center.x + size, center.y), color, thickness);
    draw_list->AddLine(ImVec2(center.x, center.y - size), ImVec2(center.x, center.y + size), color, thickness);

    // Optional: Add a small center dot
    draw_list->AddCircleFilled(center, 1.0f, color);
}

void ImGuiUI::renderHotbar(int selectedSlot) {
    // Get the main viewport
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Position hotbar at bottom center
    ImVec2 hotbar_size = ImVec2(360.0f, 50.0f);
    ImVec2 hotbar_pos = ImVec2(
        (viewport->Size.x - hotbar_size.x) * 0.5f,
        viewport->Size.y - hotbar_size.y - 20.0f
    );

    // Create a transparent window for the hotbar
    ImGui::SetNextWindowPos(hotbar_pos);
    ImGui::SetNextWindowSize(hotbar_size);
    ImGui::Begin("Hotbar", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);    // Block type names and selection
    const char* blockTypes[] = {"Dirt", "Stone", "Wood", "Grass", "Leaves"};

    float slotSize = 40.0f;

    for (int i = 0; i < 5; i++) {
        if (i > 0) ImGui::SameLine();

        // Change color for selected slot
        if (i == selectedSlot) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.8f, 0.8f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.9f, 0.9f, 0.9f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.8f));
        }

        // Create slot button
        ImGui::Button(blockTypes[i], ImVec2(slotSize, slotSize));

        ImGui::PopStyleColor(2);

        // Show number key hint
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15.0f);
        if (i < 4) ImGui::SameLine();
        ImGui::Text("%d", i + 1);
    }

    ImGui::End();
}
