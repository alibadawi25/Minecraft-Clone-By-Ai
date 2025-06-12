#include "main_menu.h"
#include <iostream>

MainMenu::MainMenu() : selectedOption(0) {
}

MainMenu::~MainMenu() {
}

void MainMenu::render(GameStateManager* stateManager) {
    // Get display size
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;

    // Create fullscreen window for menu
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(displaySize);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoSavedSettings |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("MainMenu", nullptr, windowFlags)) {
        renderBackground();
        renderTitle();
        renderMenuButtons(stateManager);
    }
    ImGui::End();
}

void MainMenu::renderBackground() {
    // Draw a dark overlay background
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();

    // Gradient background from dark blue to black
    drawList->AddRectFilledMultiColor(
        windowPos,
        ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y),
        IM_COL32(20, 30, 60, 255),   // Dark blue
        IM_COL32(20, 30, 60, 255),   // Dark blue
        IM_COL32(10, 15, 30, 255),   // Darker blue
        IM_COL32(10, 15, 30, 255)    // Darker blue
    );
}

void MainMenu::renderTitle() {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;

    // Title text
    const char* title = "MINECRAFT CLONE";

    // Calculate title position (centered, top third)
    ImGui::PushFont(io.Fonts->Fonts[0]); // Default font, but we'll make it bigger with scale

    float titleScale = 3.0f;
    ImGui::SetWindowFontScale(titleScale);

    ImVec2 titleSize = ImGui::CalcTextSize(title);
    ImVec2 titlePos = ImVec2(
        (displaySize.x - titleSize.x) * 0.5f,
        displaySize.y * 0.2f
    );

    ImGui::SetCursorPos(titlePos);

    // Add glow effect with multiple text renders
    ImU32 glowColor = IM_COL32(100, 150, 255, 100);
    ImU32 textColor = IM_COL32(255, 255, 255, 255);

    // Render glow (offset text in multiple directions)
    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            if (i == 0 && j == 0) continue;
            ImGui::SetCursorPos(ImVec2(titlePos.x + i, titlePos.y + j));
            ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 0.3f), title);
        }
    }

    // Render main title text
    ImGui::SetCursorPos(titlePos);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), title);

    ImGui::SetWindowFontScale(1.0f); // Reset font scale
    ImGui::PopFont();
}

void MainMenu::renderMenuButtons(GameStateManager* stateManager) {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;

    // Menu button styling
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 15));

    // Calculate button area (centered)
    float buttonWidth = 200.0f;
    float buttonHeight = 40.0f;
    float totalHeight = (sizeof(menuOptions) / sizeof(menuOptions[0])) * (buttonHeight + 15) - 15;

    ImVec2 buttonAreaStart = ImVec2(
        (displaySize.x - buttonWidth) * 0.5f,
        displaySize.y * 0.5f - totalHeight * 0.5f
    );

    ImGui::SetCursorPos(buttonAreaStart);

    // Render menu buttons
    for (int i = 0; i < 4; i++) {
        ImVec2 buttonPos = ImVec2(buttonAreaStart.x, buttonAreaStart.y + i * (buttonHeight + 15));
        ImGui::SetCursorPos(buttonPos);

        // Highlight selected option
        if (i == selectedOption) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.9f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.6f, 1.0f, 0.9f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.3f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.4f, 0.7f, 0.8f));
        }
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.7f, 1.0f, 1.0f));

        if (ImGui::Button(menuOptions[i], ImVec2(buttonWidth, buttonHeight))) {
            selectedOption = i;
            handleMenuSelection(stateManager, i);
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::PopStyleVar(2);
}

void MainMenu::handleMenuSelection(GameStateManager* stateManager, int option) {
    switch (option) {
        case 0: // Start Game
            std::cout << "Starting game..." << std::endl;
            stateManager->setState(GameState::PLAYING);
            break;
        case 1: // Settings
            std::cout << "Opening settings..." << std::endl;
            stateManager->setState(GameState::SETTINGS);
            break;
        case 2: // Credits
            std::cout << "Showing credits..." << std::endl;
            // For now, just print to console
            break;
        case 3: // Exit
            std::cout << "Exiting game..." << std::endl;
            stateManager->setState(GameState::EXITING);
            break;
    }
}

void MainMenu::handleInput(GLFWwindow* window, GameStateManager* stateManager) {
    // Handle keyboard navigation
    static bool upPressed = false;
    static bool downPressed = false;
    static bool enterPressed = false;

    bool upCurrentlyPressed = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ||
                             glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool downCurrentlyPressed = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ||
                               glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool enterCurrentlyPressed = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS ||
                                glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    // Up key - navigate up
    if (upCurrentlyPressed && !upPressed) {
        selectedOption = (selectedOption - 1 + 4) % 4;
    }

    // Down key - navigate down
    if (downCurrentlyPressed && !downPressed) {
        selectedOption = (selectedOption + 1) % 4;
    }

    // Enter key - select option
    if (enterCurrentlyPressed && !enterPressed) {
        handleMenuSelection(stateManager, selectedOption);
    }

    upPressed = upCurrentlyPressed;
    downPressed = downCurrentlyPressed;
    enterPressed = enterCurrentlyPressed;
}
