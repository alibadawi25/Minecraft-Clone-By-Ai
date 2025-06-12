#pragma once

#include <windows.h>
#include "../renderer/camera.h"

class InputManager {
public:
    InputManager();
    ~InputManager();

    // Set the camera to control
    void setCamera(Camera* camera);

    // Main update function - call every frame
    void update(float deltaTime);

    // Process keyboard input
    void processKeyboard(float deltaTime);

    // Alternative direct keyboard processing for ultra-smooth input
    void processKeyboardDirect(float deltaTime);

    // Process mouse movement every frame (polling)
    void processMouseEveryFrame();

    // Process mouse movement (called from window messages)
    void processMouseMovement(float xPos, float yPos);

    // Process mouse scroll
    void processMouseScroll(float yOffset);

    // Handle window messages
    void handleWindowMessage(UINT message, WPARAM wParam, LPARAM lParam);

    // Set mouse capture
    void setMouseCapture(HWND hwnd, bool capture);

    // Check if application should quit
    bool shouldQuit() const { return quit; }

    // Check if mouse is currently captured
    bool getMouseCaptured() const { return mouseCaptured; }

    // Key state checking methods
    bool isKeyDown(int key) const;
    bool isKeyUp(int key) const;

    // Check if key is currently pressed (for continuous input)
    bool isKeyPressed(int key) const;

    // Update key states for next frame (call at end of frame)
    void updateFrame();

private:
    Camera* camera;
    bool firstMouse;
    float lastX, lastY;
    bool mouseCaptured;
    bool quit;
    HWND windowHandle;

    // Key states
    bool keys[256];
    bool prevKeys[256];  // Previous frame key states for detecting up/down events

    // Internal methods
    void updateKeyState(int key, bool pressed);
};
