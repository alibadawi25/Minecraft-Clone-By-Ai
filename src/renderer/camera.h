#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Forward declaration
class InputManager;

// Camera movement directions
enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// Default camera values
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  8.0f;  // Increased movement speed for more Minecraft-like feel
const float SPRINT_MULTIPLIER = 2.0f;  // Sprint speed multiplier (Ctrl+movement)
const float SENSITIVITY =  0.15f; // Slightly increased for better responsiveness
const float ZOOM        =  45.0f;

class Camera {
public:
    // Camera attributes
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // Euler angles
    float yaw;
    float pitch;    // Camera options
    float movementSpeed;
    float mouseSensitivity;
    float zoom;

#ifndef GLFW_BUILD
    // Input manager for handling input processing
    InputManager* inputManager;
#endif

    // Constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW,
           float pitch = PITCH);

    // Constructor with scalar values
    Camera(float posX, float posY, float posZ,
           float upX, float upY, float upZ,
           float yaw, float pitch);

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 getViewMatrix() const;

    // Returns the projection matrix
    glm::mat4 getProjectionMatrix(float aspectRatio, float nearPlane = 0.1f, float farPlane = 100.0f) const;    // Processes input received from any keyboard-like input system
    void processKeyboard(CameraMovement direction, float deltaTime);    // Processes direct keyboard input using GetAsyncKeyState for smooth movement
    void updateMovement(float deltaTime);

#ifndef GLFW_BUILD
    // Set the input manager reference for input processing
    void setInputManager(InputManager* inputManager);

    // Update camera with input processing using InputManager
    void updateWithInput(float deltaTime);
#endif

    // Processes input received from a mouse input system
    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);

    // Processes input received from a mouse scroll-wheel event
    void processMouseScroll(float yOffset);    // Get camera position
    const glm::vec3& getPosition() const { return position; }

    // Get camera front vector
    const glm::vec3& getFront() const { return front; }

    // Get camera right vector
    const glm::vec3& getRight() const { return right; }

    // Get camera up vector
    const glm::vec3& getUp() const { return up; }

    // Get field of view
    float getFOV() const { return zoom; }    // Process keyboard movement with a direction vector (for GLFW)
    void processKeyboard(const glm::vec3& movement, float deltaTime);

    // Process keyboard movement with sprint support (for GLFW)
    void processKeyboardWithSprint(const glm::vec3& movement, float deltaTime, bool isSprinting = false);

private:
    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();
};
