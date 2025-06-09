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

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow* window);
void updateWindowTitle(GLFWwindow* window, double fps);

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

    // Enable alpha blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);// Initialize UI system
    ui = new ImGuiUI();
    if (!ui->initialize(window)) {
        std::cerr << "Failed to initialize UI system" << std::endl;        delete ui;
        ui = nullptr;
    }

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
        }

        // Poll events
        glfwPollEvents();

        // Process input
        processInput(window);        // Set background color and clear screen
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);  // Sky blue color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);        // Create view and projection matrices
        glm::mat4 view = camera->getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera->getFOV()),
                                               (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                               0.1f, 1000.0f);        // Render world
        if (world) {
            world->render(view, projection, camera->getPosition());
            // PHASE 5: Update chunks around player
            world->updateChunksAroundPlayer(camera->getPosition());
            // PHASE 7: Update dirty chunk meshes after block changes
            world->updateDirtyChunks();
        }        // Render UI
        if (ui) {
            ui->newFrame();

            if (showUI) {
                // Render render distance control window
                ui->renderRenderDistanceControl(world);

                // Render debug window with performance info
                ui->renderDebugWindow(fps, world, camera);

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
            ui->renderHotbar(selectedSlot);

            ui->render();
        }

        // Swap front and back buffers
        glfwSwapBuffers(window);
    }    // Clean up
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
    if (!camera) return;

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
    }

    // Handle ESC key for mouse capture toggle
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (mouseCaptured) {
            // Release mouse capture
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            mouseCaptured = false;
        } else {
            // Re-capture mouse
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouseCaptured = true;
            firstMouse = true;
        }
    }    // Handle Alt+F4 for quit
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

    if (!mouseCaptured || !camera) return;

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

    // Only handle clicks when mouse is captured and action is press
    if (!mouseCaptured || action != GLFW_PRESS || !camera || !world) {
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
    if (camera) {
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
