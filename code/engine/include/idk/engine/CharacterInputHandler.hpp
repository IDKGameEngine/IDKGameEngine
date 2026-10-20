#pragma once

#include "CharacterInput.hpp"

namespace idk::engine
{
    class CharacterCommand;
    class CharacterController;

    // Fixed-size, non-owning bindings. The controller and bound commands must
    // outlive this handler. Each button has optional press and release commands.
    class CharacterInputHandler
    {
    public:
        explicit CharacterInputHandler(CharacterController &controller) : mController(controller) {}

        CharacterInputHandler(const CharacterInputHandler &) = delete;
        CharacterInputHandler &operator=(const CharacterInputHandler &) = delete;

        // Null commands unbind an edge. Release/reset a held button before rebinding.
        bool bind(size_t button, const CharacterCommand *pressed, const CharacterCommand *released = nullptr);
        // Call once per input frame, even when no physics tick is due.
        void update(const CharacterInput &);
        // Release held actions and cancel pending intent on focus/capture loss.
        void reset();

    private:
        struct Binding
        {
            const CharacterCommand *pressed = nullptr;
            const CharacterCommand *released = nullptr;
            bool held = false;
        };

        CharacterController &mController;
        Binding mBindings[CharacterInput::MaxButtons]{};
    };
}
