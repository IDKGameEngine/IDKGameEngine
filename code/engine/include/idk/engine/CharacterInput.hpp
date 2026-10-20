#pragma once

#include <cstddef>

namespace idk::engine
{
    // Device-independent input for one frame. The caller assigns button slots.
    struct CharacterInput
    {
        static constexpr size_t MaxButtons = 16;

        // Right/forward axes, normally in [-1, 1]. The controller chooses their
        // frame of reference (for example, local or world space).
        float moveX = 0.0f;
        float moveY = 0.0f;

        // Changes in yaw and pitch, in degrees.
        float lookX = 0.0f;
        float lookY = 0.0f;

        bool sprint = false;
        bool buttons[MaxButtons]{};
    };
}
