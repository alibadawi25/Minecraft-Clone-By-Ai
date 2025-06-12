#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include "config.h"
#include "renderer/simple_shader.h"
#include "renderer/camera.h"
#include "world/world.h"
#include "ui/imgui_ui.h"
#include "ui/game_state.h"
#include "ui/main_menu.h"

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow* window);
void updateWindowTitle(GLFWwindow* window, double fps);
void renderGameWorld();
void renderGameWorldFrozen(); // For paused state - no updates
void renderSettingsMenu();
void renderPauseMenu();

// Window dimensions
const unsigned int SCR_WIDTH = DEFAULT_WINDOW_WIDTH;
const unsigned int SCR_HEIGHT = DEFAULT_WINDOW_HEIGHT;

// Global variables
bool g_running = true;

// World system
World* world = nullptr;

// Camera system variables
Camera* camera = nullptr;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// UI system
ImGuiUI* ui = nullptr;
bool showUI = true;

// Game state system
GameStateManager* gameStateManager = nullptr;
MainMenu* mainMenu = nullptr;

// Mouse input variables
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool mouseCaptured = true;

// Key states for smooth input
bool keys[GLFW_KEY_LAST + 1] = { false };

// Block interaction timing
float lastBlockInteraction = 0.0f;
const float BLOCK_INTERACTION_COOLDOWN = 0.2f; // 200ms cooldown between interactions

// Block selection system
BlockType selectedBlockType = BlockType::DIRT; // Default to dirt
const BlockType HOTBAR_BLOCKS[] = {
    BlockType::DIRT,
    BlockType::STONE,
    BlockType::WOOD,
    BlockType::GRASS,
    BlockType::LEAVES
};
const int HOTBAR_SIZE = 5;

// Creative mode flying system
bool isFlying = true;  // Start with flying enabled
bool spaceWasPressed = false;
float lastSpacePress = 0.0f;
const float DOUBLE_TAP_TIME = 0.3f; // Time window for double-tap detection

int main()
{    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);    // Set up callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // Capture mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Enable face culling to prevent rendering back faces
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);  // Counter-clockwise winding is front-facing

    // Enable alpha blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);    // Initialize UI system
    ui = new ImGuiUI();
    if (!ui->initialize(window)) {
        std::cerr << "Failed to initialize UI system" << std::endl;
        delete ui;
        ui = nullptr;
    }

    // Initialize game state system
    gameStateManager = new GameStateManager();
    mainMenu = new MainMenu();

    // Initially disable mouse capture for menu
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Initialize world system
    world = new World();
    world->initialize();    // Initialize camera - position above the ground
    camera = new Camera(glm::vec3(8.0f, 70.0f, 8.0f));

    // FPS tracking variables
    int frameCount = 0;
    double elapsedTime = 0.0;
    double fps = 0.0;
    auto lastTime = std::chrono::high_resolution_clock::now();

    // Main render loop
    while (!glfwWindowShouldClose(window) && g_running)
    {
        // Calculate frame timing
        auto currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Cap deltaTime to prevent large jumps
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // FPS calculation
        elapsedTime += deltaTime;
        frameCount++;

        // Update FPS every second
        if (elapsedTime >= 1.0)
        {
            fps = frameCount / elapsedTime;
            updateWindowTitle(window, fps);
            frameCount = 0;
            elapsedTime = 0.0;
        }        // Poll events
        glfwPollEvents();        // Handle different game states
        switch (gameStateManager->getCurrentState()) {
            case GameState::MAIN_MENU:
                // Main menu mode - show cursor, clear with dark background
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                glClearColor(0.1f, 0.1f, 0.2f, 1.0f);  // Dark background
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Start UI frame
                if (ui) {
                    ui->newFrame();
                    mainMenu->render(gameStateManager);
                    mainMenu->handleInput(window, gameStateManager);
                    ui->render();
                }
                break;

            case GameState::PLAYING:
                // Game mode - capture cursor, process input, render world
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                processInput(window);

                // Set background color and clear screen
                glClearColor(0.53f, 0.81f, 0.92f, 1.0f);  // Sky blue color
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Render game world (existing rendering code)
                renderGameWorld();
                break;            case GameState::PAUSED:
                // Paused state - show cursor, render game background but with pause menu overlay
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

                // Set background color and clear screen
                glClearColor(0.53f, 0.81f, 0.92f, 1.0f);  // Sky blue color
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Render the game world (frozen in background - no updates)
                renderGameWorldFrozen();

                // Render pause menu overlay
                if (ui) {
                    ui->newFrame();
                    renderPauseMenu();
                    ui->render();
                }
                break;

            case GameState::SETTINGS:
                // Settings mode
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                glClearColor(0.1f, 0.2f, 0.1f, 1.0f);  // Dark green background
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                if (ui) {
                    ui->newFrame();
                    renderSettingsMenu();
                    ui->render();
                }
                break;

            case GameState::EXITING:
                g_running = false;
                break;        }

        // Swap front and back buffers
        glfwSwapBuffers(window);
    }// Clean up
    if (gameStateManager) {
        delete gameStateManager;
        gameStateManager = nullptr;
    }
    if (mainMenu) {
        delete mainMenu;
        mainMenu = nullptr;
    }
    if (ui) {
        delete ui;
        ui = nullptr;
    }
    if (world) {
        delete world;
    }
    if (camera) {
        delete camera;
    }

    glfwTerminate();
    return 0;
}

// Process input using smooth GLFW key states
void processInput(GLFWwindow* window)
{
    (void)window; // Suppress unused parameter warning
    if (!camera || !gameStateManager) return;

    // Only process input when actually playing the game
    if (gameStateManager->getCurrentState() != GameState::PLAYING) {
        return;
    }

    // Movement input - use key states for smooth movement
    glm::vec3 movement(0.0f);

    if (keys[GLFW_KEY_W]) movement += camera->getFront();   // Forward (full camera direction)
    if (keys[GLFW_KEY_S]) movement -= camera->getFront();   // Backward (full camera direction)
    if (keys[GLFW_KEY_A]) movement -= camera->getRight();   // Left
    if (keys[GLFW_KEY_D]) movement += camera->getRight();   // Right

    // Creative mode flying: Space/Shift for vertical movement when flying
    if (isFlying) {
        if (keys[GLFW_KEY_SPACE]) movement.y += 1.0f;        // Up in world coordinates
        if (keys[GLFW_KEY_LEFT_SHIFT]) movement.y -= 1.0f;   // Down in world coordinates
    }

    // Speed modifiers (only when not using Space/Shift for flying)
    bool isSpeedBoost = !isFlying && keys[GLFW_KEY_SPACE];   // Space = speed boost (when not flying)
    bool isSlowWalk = !isFlying && keys[GLFW_KEY_LEFT_SHIFT]; // Shift = slow walk (when not flying)
    bool isSprinting = keys[GLFW_KEY_LEFT_CONTROL] || keys[GLFW_KEY_RIGHT_CONTROL]; // Ctrl = sprint

    // Apply movement if there's any input
    if (glm::length(movement) > 0.0f) {
        movement = glm::normalize(movement);

        // Calculate speed multiplier based on modifiers
        float speedMultiplier = 1.0f;
        if (isFlying) {
            speedMultiplier = 2.5f;     // Flying = faster base speed
        } else if (isSpeedBoost) {
            speedMultiplier = 3.0f;     // Space = 3x speed boost (ground only)
        } else if (isSlowWalk) {
            speedMultiplier = 0.3f;     // Shift = slow walk (ground only)
        } else if (isSprinting) {
            speedMultiplier = 2.0f;     // Ctrl = normal sprint
        }

        // Apply custom speed multiplier
        float customDeltaTime = deltaTime * speedMultiplier;
        camera->processKeyboard(movement, customDeltaTime);
    }
}

// Keyboard callback for key events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode; // Suppress unused parameter warning
    // mods is now used for Alt+F4 detection    // Handle F1 key for UI toggle
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        showUI = !showUI;
    }

    // Update key states for smooth movement
    if (key >= 0 && key <= GLFW_KEY_LAST) {
        if (action == GLFW_PRESS) {
            keys[key] = true;

            // Handle double-tap Space for creative flying toggle
            if (key == GLFW_KEY_SPACE) {
                float currentTime = glfwGetTime();                if (spaceWasPressed && (currentTime - lastSpacePress) < DOUBLE_TAP_TIME) {
                    // Double-tap detected - toggle flying
                    isFlying = !isFlying;
                    spaceWasPressed = false; // Reset to prevent triple-tap issues
                } else {
                    spaceWasPressed = true;
                    lastSpacePress = currentTime;
                }
            }        } else if (action == GLFW_RELEASE) {
            keys[key] = false;
        }
    }

    // Handle number keys for hotbar selection (1-5)
    if (action == GLFW_PRESS) {
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_5) {
            int slotIndex = key - GLFW_KEY_1; // Convert to 0-4 range
            if (slotIndex < HOTBAR_SIZE) {
                selectedBlockType = HOTBAR_BLOCKS[slotIndex];
            }
        }
    }    // Handle ESC key for game state transitions
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (gameStateManager) {
            switch (gameStateManager->getCurrentState()) {
                case GameState::PLAYING:
                    // Pause the game
                    gameStateManager->setState(GameState::PAUSED);
                    break;
                case GameState::PAUSED:
                    // Resume the game
                    gameStateManager->setState(GameState::PLAYING);
                    // Reset mouse to prevent sudden camera jump when resuming
                    firstMouse = true;
                    break;
                case GameState::SETTINGS:
                    // Go back to main menu from settings
                    gameStateManager->setState(GameState::PAUSED);
                    break;
                case GameState::MAIN_MENU:
                    // Don't exit - let main menu handle exit through its UI
                    break;
                default:
                    break;
            }
        }
    }// Handle Alt+F4 for quit
    if (key == GLFW_KEY_F4 && action == GLFW_PRESS && (mods & GLFW_MOD_ALT)) {
        g_running = false;
    }    // Re-capture mouse when clicking
    if (!mouseCaptured && action == GLFW_PRESS &&
        (key == GLFW_KEY_ENTER || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured = true;
        firstMouse = true;
    }
}

// Mouse movement callback
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    (void)window; // Suppress unused parameter warning

    // Only process mouse movement when playing and mouse is captured
    if (!mouseCaptured || !camera || !gameStateManager) return;

    // Don't process mouse movement when not actively playing
    if (gameStateManager->getCurrentState() != GameState::PLAYING) return;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera->processMouseMovement(xoffset, yoffset);
}

// Mouse button callback
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void)window; // Suppress unused parameter warning
    (void)mods;   // Suppress unused parameter warning

    // Check if ImGui wants to capture mouse input
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return; // Let ImGui handle the mouse input
    }

    // Only handle clicks when mouse is captured, action is press, and we're playing
    if (!mouseCaptured || action != GLFW_PRESS || !camera || !world || !gameStateManager) {
        return;
    }

    // Don't process mouse clicks when not actively playing
    if (gameStateManager->getCurrentState() != GameState::PLAYING) {
        return;
    }

    // Check cooldown to prevent rapid clicking
    float currentTime = glfwGetTime();
    if (currentTime - lastBlockInteraction < BLOCK_INTERACTION_COOLDOWN) {
        return;
    }

    // Cast a ray from camera to find targeted block
    World::RaycastResult result = world->raycast(camera->getPosition(), camera->getFront(), 10.0f);

    if (result.hit) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            // Left-click: remove block (set to Air)
            world->setBlock(result.blockPos.x, result.blockPos.y, result.blockPos.z,
                          BlockData{BlockType::AIR});
            lastBlockInteraction = currentTime;        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            // Right-click: add block of selected type to adjacent face
            // Make sure we're not placing a block where the player is standing
            glm::vec3 playerPos = camera->getPosition();
            glm::ivec3 playerBlockPos = glm::ivec3(glm::floor(playerPos));

            // Check if the placement position would overlap with player
            if (result.adjacentPos != playerBlockPos &&
                result.adjacentPos != glm::ivec3(glm::floor(playerPos + glm::vec3(0, 1, 0)))) {
                world->setBlock(result.adjacentPos.x, result.adjacentPos.y, result.adjacentPos.z,
                              BlockData{selectedBlockType});
                lastBlockInteraction = currentTime;
            }
        }
    }
}

// Mouse scroll callback
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)window;  // Suppress unused parameter warning
    (void)xoffset; // Suppress unused parameter warning

    // Only process scroll when playing
    if (camera && gameStateManager && gameStateManager->getCurrentState() == GameState::PLAYING) {
        camera->processMouseScroll(yoffset);
    }
}

// Framebuffer size callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    (void)window; // Suppress unused parameter warning
    glViewport(0, 0, width, height);
}

// Update window title with FPS
void updateWindowTitle(GLFWwindow* window, double fps)
{
    std::ostringstream titleStream;
    titleStream << WINDOW_TITLE << " - FPS: " << std::fixed << std::setprecision(1) << fps;
    glfwSetWindowTitle(window, titleStream.str().c_str());
}

// Render the game world (3D scene)
void renderGameWorld()
{
    // Create view and projection matrices
    glm::mat4 view = camera->getViewMatrix();

    // Calculate far plane distance based on render distance
    float farDistance = 1000.0f; // Default fallback
    if (world) {
        // Convert render distance (chunks) to world units
        float renderDistanceChunks = world->getRenderDistance();
        float diagonalDistance = renderDistanceChunks * 1.414f; // sqrt(2) ≈ 1.414
        float renderDistanceWorldUnits = diagonalDistance * 16.0f;
        farDistance = renderDistanceWorldUnits + (3.0f * 16.0f) + 256.0f + 50.0f;
        farDistance = std::max(farDistance, 300.0f);
    }

    glm::mat4 projection = glm::perspective(glm::radians(camera->getFOV()),
                                           (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                           0.1f, farDistance);

    // Render world
    if (world) {
        world->render(view, projection, camera->getPosition());

        // Update block highlighting based on where the camera is looking
        if (mouseCaptured && camera) {
            World::RaycastResult result = world->raycast(camera->getPosition(), camera->getFront(), 10.0f);
            if (result.hit) {
                world->setTargetedBlock(result.blockPos);
            } else {
                world->clearTargetedBlock();
            }
        } else {
            world->clearTargetedBlock();
        }

        // Render block highlight after world geometry but before UI
        world->renderBlockHighlight(view, projection, camera->getPosition());

        // Update chunks around player
        world->updateChunksAroundPlayer(camera->getPosition());
        // Update dirty chunk meshes after block changes
        world->updateDirtyChunks();
    }

    // Render UI
    if (ui) {
        ui->newFrame();

        if (showUI) {
            // Render render distance control window
            ui->renderRenderDistanceControl(world);

            // Render debug window with performance info
            ui->renderDebugWindow(0.0, world, camera); // FPS will be handled separately

            // Render block interaction UI - pass selected block type as int
            ui->renderBlockInteractionUI(world, camera, static_cast<int>(selectedBlockType));
        }

        // Calculate selected slot index for hotbar
        int selectedSlot = 0;
        for (int i = 0; i < HOTBAR_SIZE; i++) {
            if (HOTBAR_BLOCKS[i] == selectedBlockType) {
                selectedSlot = i;
                break;
            }
        }

        // Always render crosshair and hotbar
        ui->renderCrosshair();
        ui->renderHotbar(selectedSlot);        ui->render();
    }
}

// Render the game world (frozen for pause state - no updates)
void renderGameWorldFrozen()
{
    // Create view and projection matrices
    glm::mat4 view = camera->getViewMatrix();

    // Calculate far plane distance based on render distance
    float farDistance = 1000.0f; // Default fallback
    if (world) {
        // Convert render distance (chunks) to world units
        float renderDistanceChunks = world->getRenderDistance();
        float diagonalDistance = renderDistanceChunks * 1.414f; // sqrt(2) ≈ 1.414
        float renderDistanceWorldUnits = diagonalDistance * 16.0f;
        farDistance = renderDistanceWorldUnits + (3.0f * 16.0f) + 256.0f + 50.0f;
        farDistance = std::max(farDistance, 300.0f);
    }

    glm::mat4 projection = glm::perspective(glm::radians(camera->getFOV()),
                                           (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                           0.1f, farDistance);

    // Render world (without updates)
    if (world) {
        world->render(view, projection, camera->getPosition());

        // Clear any targeted block since we're paused
        world->clearTargetedBlock();

        // Render block highlight (but don't update it)
        world->renderBlockHighlight(view, projection, camera->getPosition());

        // DO NOT update chunks or dirty chunks while paused
        // DO NOT call world->updateChunksAroundPlayer() or world->updateDirtyChunks()
    }

    // Don't render game UI elements while paused (no crosshair, hotbar, debug windows)
    // The pause menu will be rendered separately
}

// Render the settings menu
void renderSettingsMenu()
{
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Settings")) {
        ImGui::Text("Game Settings");
        ImGui::Separator();        // Graphics settings
        if (ImGui::CollapsingHeader("Graphics")) {
            if (world) {
                int renderDistance = world->getRenderDistance();
                if (ImGui::SliderInt("Render Distance", &renderDistance, 4, 32)) {
                    world->setRenderDistance(renderDistance);
                }
            }

            static bool vsync = true;
            ImGui::Checkbox("VSync", &vsync);
        }

        // UI settings
        if (ImGui::CollapsingHeader("UI")) {
            ImGui::Checkbox("Show Debug Windows", &showUI);
            ImGui::Text("Toggle debug windows (F1 also works)");
        }        // Controls settings
        if (ImGui::CollapsingHeader("Controls")) {
            if (camera) {
                ImGui::SliderFloat("Mouse Sensitivity", &camera->mouseSensitivity, 0.01f, 1.0f, "%.3f");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Adjust mouse look sensitivity");
                }
            }
            ImGui::Text("Use mouse to look around when in game");
        }        // World settings
        if (ImGui::CollapsingHeader("World")) {
            static int worldSeed = 12345;
            ImGui::InputInt("World Seed", &worldSeed);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Seed for world generation (requires world regeneration)");
            }            if (ImGui::Button("Generate New World")) {
                if (world && camera) {
                    // Regenerate world with new seed
                    world->regenerateWorld(static_cast<unsigned int>(worldSeed));

                    // Reset player to default spawn position
                    camera->position = glm::vec3(8.0f, 70.0f, 8.0f);
                    camera->yaw = -90.0f;  // Face forward
                    camera->pitch = 0.0f;  // Level view

                    // Update camera vectors to reflect the new yaw/pitch
                    camera->processMouseMovement(0.0f, 0.0f);

                    // Reset mouse to prevent camera jump
                    firstMouse = true;
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Generate a new world with the current seed and reset player position");
            }
        }ImGui::Separator();

        if (ImGui::Button("Back to Game")) {
            gameStateManager->setState(GameState::PLAYING);
        }
    }
    ImGui::End();
}

// Render the pause menu overlay
void renderPauseMenu()
{
    // Create a semi-transparent background overlay
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;

    // Full screen overlay window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(displaySize);

    ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoBackground;

    if (ImGui::Begin("PauseOverlay", nullptr, overlayFlags)) {
        // Draw semi-transparent background
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();

        drawList->AddRectFilled(
            windowPos,
            ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y),
            IM_COL32(0, 0, 0, 120)  // Semi-transparent black overlay
        );
    }
    ImGui::End();

    // Pause menu window (centered)
    ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_Always);

    if (ImGui::Begin("Game Paused", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
        // Center the title
        float windowWidth = ImGui::GetWindowSize().x;
        float textWidth = ImGui::CalcTextSize("GAME PAUSED").x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

        ImGui::Text("GAME PAUSED");
        ImGui::Separator();
        ImGui::Spacing();

        // Menu buttons (centered)
        float buttonWidth = 200.0f;
        ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);        if (ImGui::Button("Resume Game", ImVec2(buttonWidth, 30))) {
            gameStateManager->setState(GameState::PLAYING);
            // Reset mouse to prevent sudden camera jump when resuming
            firstMouse = true;
        }        ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("Settings", ImVec2(buttonWidth, 30))) {
            gameStateManager->setState(GameState::SETTINGS);
        }

        ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("Exit to Main Menu", ImVec2(buttonWidth, 30))) {
            gameStateManager->setState(GameState::MAIN_MENU);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Press ESC to resume");
    }
    ImGui::End();
}
