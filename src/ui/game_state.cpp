#include "game_state.h"

GameStateManager::GameStateManager()
    : currentState(GameState::MAIN_MENU), transitionRequested(false) {
}

GameStateManager::~GameStateManager() {
}

void GameStateManager::setState(GameState newState) {
    if (currentState != newState) {
        currentState = newState;
        transitionRequested = true;
    }
}
