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

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
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

// Mouse input variables
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool mouseCaptured = true;

// Key states for smooth input
bool keys[GLFW_KEY_LAST + 1] = { false };

int main()
{
    std::cout << "Starting " << WINDOW_TITLE << std::endl;
    std::cout << "Platform: " << PLATFORM_NAME << std::endl;
    std::cout << "Build Type: " << BUILD_TYPE << std::endl;

    // Initialize GLFW
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
    glfwMakeContextCurrent(window);

    // Set up callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // Capture mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Initialize world system
    world = new World();
    world->initialize();

    // Initialize camera - position above the ground
    camera = new Camera(glm::vec3(8.0f, 70.0f, 8.0f));

    // Print system information
    std::cout << "=== Minecraft Clone v" << MINECRAFT_CLONE_VERSION_MAJOR
              << "." << MINECRAFT_CLONE_VERSION_MINOR
              << "." << MINECRAFT_CLONE_VERSION_PATCH << " ===" << std::endl;    std::cout << "Platform: " << PLATFORM_NAME << " (" << BUILD_TYPE << ")" << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Press ESC to toggle mouse capture, Alt+F4 or close window to quit" << std::endl;
    std::cout << "Hold Ctrl while moving to sprint" << std::endl;
    std::cout << "========================================" << std::endl;

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
            world->render(view, projection);
            // PHASE 5: Update chunks around player
            world->updateChunksAroundPlayer(camera->getPosition());
        }

        // Swap front and back buffers
        glfwSwapBuffers(window);
    }    // Clean up
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

    if (keys[GLFW_KEY_W]) movement += camera->getFront();
    if (keys[GLFW_KEY_S]) movement -= camera->getFront();
    if (keys[GLFW_KEY_A]) movement -= camera->getRight();
    if (keys[GLFW_KEY_D]) movement += camera->getRight();
    if (keys[GLFW_KEY_SPACE]) movement += camera->getUp();
    if (keys[GLFW_KEY_LEFT_SHIFT]) movement -= camera->getUp();

    // Check if sprinting (Ctrl key held)
    bool isSprinting = keys[GLFW_KEY_LEFT_CONTROL] || keys[GLFW_KEY_RIGHT_CONTROL];

    // Normalize diagonal movement to prevent faster movement
    if (glm::length(movement) > 0.0f) {
        movement = glm::normalize(movement);

        // Apply sprint multiplier if sprinting
        if (isSprinting) {
            camera->processKeyboardWithSprint(movement, deltaTime, true);
        } else {
            camera->processKeyboard(movement, deltaTime);
        }
    }
}

// Keyboard callback for key events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode; // Suppress unused parameter warning
    // mods is now used for Alt+F4 detection

    // Update key states for smooth movement
    if (key >= 0 && key <= GLFW_KEY_LAST) {
        if (action == GLFW_PRESS) {
            keys[key] = true;
        } else if (action == GLFW_RELEASE) {
            keys[key] = false;
        }
    }    // Handle ESC key for mouse capture toggle
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (mouseCaptured) {
            // Release mouse capture
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            mouseCaptured = false;
            std::cout << "Mouse capture released. Press ESC again to re-capture." << std::endl;
        } else {
            // Re-capture mouse
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouseCaptured = true;
            firstMouse = true;
            std::cout << "Mouse captured. Press ESC to release." << std::endl;
        }
    }

    // Handle Alt+F4 for quit
    if (key == GLFW_KEY_F4 && action == GLFW_PRESS && (mods & GLFW_MOD_ALT)) {
        g_running = false;
        std::cout << "Alt+F4 pressed - quitting application." << std::endl;
    }

    // Re-capture mouse when clicking
    if (!mouseCaptured && action == GLFW_PRESS &&
        (key == GLFW_KEY_ENTER || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured = true;
        firstMouse = true;
        std::cout << "Mouse captured. Press ESC to release." << std::endl;
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
