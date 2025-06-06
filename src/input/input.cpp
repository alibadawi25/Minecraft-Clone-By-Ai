#include "input.h"
#include <iostream>
#include <cmath>

InputManager::InputManager()
    : camera(nullptr),
      firstMouse(true),
      lastX(400.0f),
      lastY(300.0f),
      mouseCaptured(false),
      quit(false),
      windowHandle(nullptr)
{
    // Initialize key states
    for (int i = 0; i < 256; ++i) {
        keys[i] = false;
        prevKeys[i] = false;
    }
}

InputManager::~InputManager() {
    if (mouseCaptured && windowHandle) {
        setMouseCapture(windowHandle, false);
    }
}

void InputManager::setCamera(Camera* camera) {
    this->camera = camera;
}

void InputManager::processKeyboard(float deltaTime) {
    if (!camera) return;

    // Use isKeyPressed for continuous movement while held
    // WASD movement - continuous movement while held
    if (isKeyPressed('W') || isKeyPressed(VK_UP))
        camera->processKeyboard(CameraMovement::FORWARD, deltaTime);
    if (isKeyPressed('S') || isKeyPressed(VK_DOWN))
        camera->processKeyboard(CameraMovement::BACKWARD, deltaTime);
    if (isKeyPressed('A') || isKeyPressed(VK_LEFT))
        camera->processKeyboard(CameraMovement::LEFT, deltaTime);
    if (isKeyPressed('D') || isKeyPressed(VK_RIGHT))
        camera->processKeyboard(CameraMovement::RIGHT, deltaTime);

    // Space and Shift for up/down movement - continuous movement while held
    if (isKeyPressed(VK_SPACE))
        camera->processKeyboard(CameraMovement::UP, deltaTime);
    if (isKeyPressed(VK_SHIFT))
        camera->processKeyboard(CameraMovement::DOWN, deltaTime);    // ESC to quit or release mouse - use key down for single press
    if (isKeyDown(VK_ESCAPE)) {
        if (mouseCaptured) {
            // Release mouse capture first
            setMouseCapture(windowHandle, false);
        } else {
            // If mouse not captured, quit application
            quit = true;
        }
    }
}

void InputManager::update(float deltaTime) {
    // Process keyboard input every frame
    processKeyboard(deltaTime);
    // Note: Mouse processing is now handled only through window messages
    // This eliminates the double processing issue
}

// This method is no longer used - mouse processing is handled through window messages only
/*
void InputManager::processMouseEveryFrame() {
    if (!camera || !mouseCaptured || !windowHandle) return;

    // Get current cursor position
    POINT cursorPos;
    if (!GetCursorPos(&cursorPos)) return;

    // Convert to client coordinates
    if (!ScreenToClient(windowHandle, &cursorPos)) return;

    // Get window center
    RECT rect;
    GetClientRect(windowHandle, &rect);
    float centerX = rect.right / 2.0f;
    float centerY = rect.bottom / 2.0f;

    if (firstMouse) {
        lastX = centerX;
        lastY = centerY;
        firstMouse = false;
        return;
    }

    float xPos = (float)cursorPos.x;
    float yPos = (float)cursorPos.y;

    float xOffset = xPos - centerX;
    float yOffset = centerY - yPos; // Reversed since y-coordinates go from bottom to top

    // Process movement if there's any offset (more sensitive than before)
    if (abs(xOffset) > 0.1f || abs(yOffset) > 0.1f) {
        camera->processMouseMovement(xOffset, yOffset);

        // Reset cursor to center of window
        POINT center = { (int)centerX, (int)centerY };
        ClientToScreen(windowHandle, &center);
        SetCursorPos(center.x, center.y);
    }
}
*/

void InputManager::processMouseMovement(float xPos, float yPos) {
    if (!camera || !mouseCaptured || !windowHandle) return;

    // Get window center
    RECT rect;
    GetClientRect(windowHandle, &rect);
    float centerX = rect.right / 2.0f;
    float centerY = rect.bottom / 2.0f;

    if (firstMouse) {
        lastX = centerX;
        lastY = centerY;
        firstMouse = false;

        // Reset cursor to center immediately
        POINT center = { (int)centerX, (int)centerY };
        ClientToScreen(windowHandle, &center);
        SetCursorPos(center.x, center.y);
        return;
    }

    // Calculate offset from center (not from last position)
    float xOffset = xPos - centerX;
    float yOffset = centerY - yPos; // Reversed since y-coordinates go from bottom to top

    // Only process if there's meaningful movement
    if (fabs(xOffset) > 0.5f || fabs(yOffset) > 0.5f) {
        camera->processMouseMovement(xOffset, yOffset);
    }

    // Always reset cursor to center after processing movement
    // This is the key to smooth FPS-style mouse look
    POINT center = { (int)centerX, (int)centerY };
    ClientToScreen(windowHandle, &center);
    SetCursorPos(center.x, center.y);
}

void InputManager::processMouseScroll(float yOffset) {
    if (!camera) return;
    camera->processMouseScroll(yOffset);
}

void InputManager::handleWindowMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_KEYDOWN:
            // Register all key down events including repeats for smooth movement
            updateKeyState((int)wParam, true);
            break;

        case WM_KEYUP:
            updateKeyState((int)wParam, false);
            break;

        case WM_MOUSEMOVE:
            if (mouseCaptured) {
                float xPos = (float)LOWORD(lParam);
                float yPos = (float)HIWORD(lParam);
                processMouseMovement(xPos, yPos);
            }
            break;

        case WM_MOUSEWHEEL:
            {
                float yOffset = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
                processMouseScroll(yOffset);
            }
            break;

        case WM_LBUTTONDOWN:
            // Click to capture mouse only if not already captured
            if (!mouseCaptured && windowHandle) {
                setMouseCapture(windowHandle, true);
            }
            break;

        case WM_RBUTTONDOWN:
            // Right click to release mouse if captured
            if (mouseCaptured && windowHandle) {
                setMouseCapture(windowHandle, false);
            }
            break;
    }
}

void InputManager::setMouseCapture(HWND hwnd, bool capture) {
    windowHandle = hwnd;
    mouseCaptured = capture;

    if (capture) {
        // Hide cursor and capture it
        ShowCursor(FALSE);
        SetCapture(hwnd);

        // Get window center
        RECT rect;
        GetClientRect(hwnd, &rect);
        float centerX = rect.right / 2.0f;
        float centerY = rect.bottom / 2.0f;

        // Convert to screen coordinates
        POINT center = { (int)centerX, (int)centerY };
        ClientToScreen(hwnd, &center);

        // Set cursor to center and lock it there
        SetCursorPos(center.x, center.y);

        // Reset mouse position tracking
        firstMouse = true;
        lastX = centerX;
        lastY = centerY;

        std::cout << "Mouse captured - Use WASD to move, mouse to look around" << std::endl;
        std::cout << "Press ESC to release mouse, ESC again to quit" << std::endl;
    } else {
        // Show cursor and release capture
        ShowCursor(TRUE);
        ReleaseCapture();

        // Reset first mouse flag
        firstMouse = true;

        std::cout << "Mouse released - Left click to recapture, ESC to quit" << std::endl;
    }
}

void InputManager::updateKeyState(int key, bool pressed) {
    if (key >= 0 && key < 256) {
        keys[key] = pressed;
    }
}

bool InputManager::isKeyPressed(int key) const {
    if (key >= 0 && key < 256) {
        return keys[key];
    }
    return false;
}

bool InputManager::isKeyDown(int key) const {
    if (key >= 0 && key < 256) {
        // Key was just pressed this frame (not pressed last frame, but pressed now)
        return keys[key] && !prevKeys[key];
    }
    return false;
}

bool InputManager::isKeyUp(int key) const {
    if (key >= 0 && key < 256) {
        // Key was just released this frame (pressed last frame, but not pressed now)
        return !keys[key] && prevKeys[key];
    }
    return false;
}

void InputManager::updateFrame() {
    // Copy current key states to previous states for next frame
    for (int i = 0; i < 256; ++i) {
        prevKeys[i] = keys[i];
    }
}

// Alternative method for ultra-smooth keyboard input using GetAsyncKeyState
void InputManager::processKeyboardDirect(float deltaTime) {
    if (!camera) return;

    // Use GetAsyncKeyState for immediate key state checking
    // This bypasses Windows message queue for more responsive input

    // WASD movement
    if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8000)
        camera->processKeyboard(CameraMovement::FORWARD, deltaTime);
    if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8000)
        camera->processKeyboard(CameraMovement::BACKWARD, deltaTime);
    if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000)
        camera->processKeyboard(CameraMovement::LEFT, deltaTime);
    if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000)
        camera->processKeyboard(CameraMovement::RIGHT, deltaTime);

    // Space and Shift for up/down movement
    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
        camera->processKeyboard(CameraMovement::UP, deltaTime);
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        camera->processKeyboard(CameraMovement::DOWN, deltaTime);

    // ESC handling (only check once per frame)
    static bool escPressed = false;
    bool escCurrentlyPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

    if (escCurrentlyPressed && !escPressed) {
        if (mouseCaptured) {
            setMouseCapture(windowHandle, false);
        } else {
            quit = true;
        }
    }
    escPressed = escCurrentlyPressed;
}
