#include "idk/platform/InputManager.hpp"
#include "idk/EngineAPI.hpp"

#include <SDL3/SDL.h>


void idk::InputManager::onInit(idk::EngineAPI&)
{

}


void idk::InputManager::onShutdown(idk::EngineAPI&)
{

}


void idk::InputManager::onUpdate(idk::EngineAPI&)
{

}


// void idk::InputManager::onEvent(idk::EngineAPI&, const void *event)
// {
//     const SDL_Event &e = *static_cast<const SDL_Event*>(event);

//     switch (e.type)
//     {
//     // case SDL_EVENT_KEY_DOWN:
//     //     VLOG_INFO("[InputManager::onEvent] KEY DOWN");
//     //     break;
//     // case SDL_EVENT_KEY_UP:
//     //     VLOG_INFO("[InputManager::onEvent] KEY UP");
//     //     break;
//     default:
//         break;
//     }
// }


bool idk::InputManager::isKeyDown(int keycode) const
{
    (void)keycode;
    return false;
}


bool idk::InputManager::isMouseButtonDown(int button) const
{
    (void)button;
    return false;
}


void idk::InputManager::getMousePosition(int& x, int& y) const
{
    x = 0;
    y = 0;
}

