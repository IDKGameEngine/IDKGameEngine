#pragma once

#include "libidk/Types.hpp"


namespace idk
{
    struct EngineEvent
    {
        enum EventType: int32_t
        {
            Quit = 1,
            Pause,
            Resume,
        };

        EventType type;

        constexpr explicit EngineEvent(EventType t)
        :   type(t)
        {

        }
    };


    struct WindowEvent
    {
        enum EventType: int32_t
        {
            Resized = 1,
            Minimized,
            Restored,
            FocusGained,
            FocusLost,
        };

        EventType type;
        int width{};
        int height{};

        constexpr explicit WindowEvent(EventType t)
        :   type(t)
        {

        }

        constexpr WindowEvent(EventType t, int w, int h)
        :   type(t), width(w), height(h)
        {

        }
    };


    struct InputEvent
    {
        enum EventType: int32_t
        {
            KeyDown = 1,
            KeyUp,
            MouseButtonDown,
            MouseButtonUp,
            MouseMove,
            MouseWheel,
        };

        EventType type;

        constexpr explicit InputEvent(EventType t)
        :   type(t)
        {
            
        }

    };
}

