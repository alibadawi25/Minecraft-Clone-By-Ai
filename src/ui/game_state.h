#pragma once

enum class GameState {
    MAIN_MENU,
    PLAYING,
    PAUSED,
    SETTINGS,
    EXITING
};

class GameStateManager {
public:
    GameStateManager();
    ~GameStateManager();

    void setState(GameState newState);
    GameState getCurrentState() const { return currentState; }

    bool shouldTransition() const { return transitionRequested; }
    void clearTransition() { transitionRequested = false; }

private:
    GameState currentState;
    bool transitionRequested;
};
