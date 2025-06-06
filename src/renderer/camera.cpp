#include "camera.h"
#include "../input/input.h"
#include <windows.h>
#include <windows.h>

// Constructor with vectors
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : front(glm::vec3(0.0f, 0.0f, -1.0f)),
      movementSpeed(SPEED),
      mouseSensitivity(SENSITIVITY),
      zoom(ZOOM)
#ifndef GLFW_BUILD
      , inputManager(nullptr)
#endif
{
    this->position = position;
    this->worldUp = up;
    this->yaw = yaw;
    this->pitch = pitch;
    updateCameraVectors();
}

// Constructor with scalar values
Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : front(glm::vec3(0.0f, 0.0f, -1.0f)),
      movementSpeed(SPEED),
      mouseSensitivity(SENSITIVITY),
      zoom(ZOOM)
#ifndef GLFW_BUILD
      , inputManager(nullptr)
#endif
{
    this->position = glm::vec3(posX, posY, posZ);
    this->worldUp = glm::vec3(upX, upY, upZ);
    this->yaw = yaw;
    this->pitch = pitch;
    updateCameraVectors();
}

// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

// Returns the projection matrix
glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float nearPlane, float farPlane) const
{
    return glm::perspective(glm::radians(zoom), aspectRatio, nearPlane, farPlane);
}

// Processes input received from any keyboard-like input system
void Camera::processKeyboard(CameraMovement direction, float deltaTime)
{
    float velocity = movementSpeed * deltaTime;

    if (direction == CameraMovement::FORWARD)
        position += front * velocity;
    if (direction == CameraMovement::BACKWARD)
        position -= front * velocity;
    if (direction == CameraMovement::LEFT)
        position -= right * velocity;
    if (direction == CameraMovement::RIGHT)
        position += right * velocity;
    if (direction == CameraMovement::UP)
        position += worldUp * velocity;
    if (direction == CameraMovement::DOWN)
        position -= worldUp * velocity;
}

// Processes direct keyboard input using GetAsyncKeyState for ultra-smooth movement
void Camera::updateMovement(float deltaTime)
{
    float velocity = movementSpeed * deltaTime;

    // Use GetAsyncKeyState for immediate key state checking
    // This bypasses Windows message queue for more responsive input

    // Calculate movement direction vector for smooth diagonal movement
    glm::vec3 moveDirection(0.0f);

    // WASD movement - accumulate direction vectors
    if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8000)
        moveDirection += front;
    if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8000)
        moveDirection -= front;
    if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000)
        moveDirection -= right;
    if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000)
        moveDirection += right;

    // Normalize diagonal movement to prevent faster diagonal movement
    if (glm::length(moveDirection) > 0.0f) {
        moveDirection = glm::normalize(moveDirection);
        position += moveDirection * velocity;
    }

    // Space and Shift for up/down movement (separate from horizontal movement)
    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
        position += worldUp * velocity;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        position -= worldUp * velocity;
}

// Processes input received from a mouse input system
void Camera::processMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    yaw += xOffset;
    pitch += yOffset;

    // Make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
    }

    // Update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

// Processes input received from a mouse scroll-wheel event
void Camera::processMouseScroll(float yOffset)
{
    zoom -= yOffset;
    if (zoom < 1.0f)
        zoom = 1.0f;
    if (zoom > 45.0f)
        zoom = 45.0f;
}

// Calculates the front vector from the Camera's (updated) Euler Angles
void Camera::updateCameraVectors()
{
    // Calculate the new Front vector
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    // Also re-calculate the Right and Up vector
    right = glm::normalize(glm::cross(front, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    up = glm::normalize(glm::cross(right, front));
}

#ifndef GLFW_BUILD
// Set the input manager reference for input processing
void Camera::setInputManager(InputManager* inputManager)
{
    this->inputManager = inputManager;
}

// Update camera with input processing using InputManager
void Camera::updateWithInput(float deltaTime)
{
    if (!inputManager) return;

    float velocity = movementSpeed * deltaTime;

    // Use InputManager's key checking methods for smooth movement
    // WASD movement - continuous movement while held
    if (inputManager->isKeyPressed('W') || inputManager->isKeyPressed(VK_UP))
        position += front * velocity;
    if (inputManager->isKeyPressed('S') || inputManager->isKeyPressed(VK_DOWN))
        position -= front * velocity;
    if (inputManager->isKeyPressed('A') || inputManager->isKeyPressed(VK_LEFT))
        position -= right * velocity;
    if (inputManager->isKeyPressed('D') || inputManager->isKeyPressed(VK_RIGHT))
        position += right * velocity;

    // Space and Shift for up/down movement - continuous movement while held
    if (inputManager->isKeyPressed(VK_SPACE))
        position += worldUp * velocity;
    if (inputManager->isKeyPressed(VK_SHIFT))
        position -= worldUp * velocity;
}
#endif

// Process keyboard movement with a direction vector (for GLFW)
void Camera::processKeyboard(const glm::vec3& movement, float deltaTime)
{
    float velocity = movementSpeed * deltaTime;
    position += movement * velocity;
}
