#pragma once

#include <imgui.h>
#include <GLFW/glfw3.h>
#include "game_state.h"

class MainMenu {
public:
    MainMenu();
    ~MainMenu();

    void render(GameStateManager* stateManager);
    void handleInput(GLFWwindow* window, GameStateManager* stateManager);

private:
    void renderBackground();
    void renderMenuButtons(GameStateManager* stateManager);
    void renderTitle();
    void handleMenuSelection(GameStateManager* stateManager, int option);

    // Menu state
    int selectedOption;
    const char* menuOptions[4] = {
        "Start Game",
        "Settings",
        "Credits",
        "Exit"
    };
};
