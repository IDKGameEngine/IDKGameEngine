#include "idk/platform/EventManager.hpp"
#include "idk/EngineAPI.hpp"

#include <SDL3/SDL.h>


void idk::EventManager::onInit(idk::EngineAPI&)
{
    if (false == SDL_Init(SDL_INIT_EVENTS))
    {
        VLOG_FATAL("{}", SDL_GetError());
    }
}


void idk::EventManager::onShutdown(idk::EngineAPI&)
{

}


// void idk::EventManager::onUpdate(idk::EngineAPI &api)
// {
//     SDL_Event e;
//     while (SDL_PollEvent(&e))
//     {
//         for (uintptr_t addr: mEventCallbacks)
//         {
//             reinterpret_cast<EventCallback>(addr)(&e);
//         }
//         if (e.type == SDL_EVENT_QUIT)
//         {
//             api.dispatchEvent(EngineEvent::T_EngineCtl, EngineEvent::S_Shutdown);
//         }
//         else if ((e.type == SDL_EVENT_KEY_UP) && (e.key.scancode == SDL_SCANCODE_ESCAPE))
//         {
//             api.dispatchEvent(EngineEvent::T_EngineCtl, EngineEvent::S_Shutdown);
//         }
//         else
//         {
//             api.dispatchEvent(EngineEvent::T_Platform, 0, reinterpret_cast<uint64_t>(&e));
//         }
//     }
// }


void idk::EventManager::onUpdate(EngineAPI &api)
{
    SDL_Event sdlEvent;

    while (SDL_PollEvent(&sdlEvent))
    {
        for (GenericCallback &func: mGenericCallbacks)
        {
            func(api, (const void*)(&sdlEvent));
        }

        switch (sdlEvent.type)
        {
            case SDL_EVENT_QUIT:
            {
                EngineEvent event(EngineEvent::Quit);
                dispatch(api, mEngineCallbacks, event);
                break;
            }

            case SDL_EVENT_WINDOW_RESIZED:
            {
                WindowEvent event(
                    WindowEvent::Resized,
                    sdlEvent.window.data1,
                    sdlEvent.window.data2
                );
                dispatch(api, mWindowCallbacks, event);
                break;
            }

            case SDL_EVENT_WINDOW_MINIMIZED:
            {
                WindowEvent event(WindowEvent::Minimized);
                dispatch(api, mWindowCallbacks, event);
                break;
            }

            case SDL_EVENT_WINDOW_RESTORED:
            {
                WindowEvent event(WindowEvent::Restored);
                dispatch(api, mWindowCallbacks, event);
                break;
            }

            case SDL_EVENT_KEY_DOWN:
            {
                if (sdlEvent.key.scancode == SDL_SCANCODE_F5 && !sdlEvent.key.repeat)
                    api.requestTimingReload();
                InputEvent event(InputEvent::KeyDown);
                dispatch(api, mInputCallbacks, event);
                break;
            }

            case SDL_EVENT_KEY_UP:
            {
                InputEvent event(InputEvent::KeyUp);
                dispatch(api, mInputCallbacks, event);
                break;
            }

            // ...
        }
    }
}


bool idk::EventManager::addGenericCallback(GenericCallback func)
{
    if (mGenericCallbacks.full())
    {
        VLOG_WARN("[EventManager::addCallback] Cannot add callback: at capacity");
        return false;
    }
    mGenericCallbacks.push(func);
    return true;
}

bool idk::EventManager::addEngineCallback(EngineCallback func)
{
    return addCallback(mEngineCallbacks, func);
}

bool idk::EventManager::addWindowCallback(WindowCallback func)
{
    return addCallback(mWindowCallbacks, func);
}

bool idk::EventManager::addInputCallback(InputCallback func)
{
    return addCallback(mInputCallbacks, func);
}
