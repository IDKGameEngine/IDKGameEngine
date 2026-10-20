#include "idk/engine/CharacterInputHandler.hpp"
#include "idk/engine/CharacterCommand.hpp"

namespace idk::engine
{
    bool CharacterInputHandler::bind(size_t button, const CharacterCommand *pressed, const CharacterCommand *released)
    {
        if (button >= CharacterInput::MaxButtons || mBindings[button].held)
            return false;
        mBindings[button] = {pressed, released, false};
        return true;
    }

    void CharacterInputHandler::update(const CharacterInput &input)
    {
        // Movement uses the orientation from this frame's look input.
        mController.look(input.lookX, input.lookY);
        mController.move(input.moveX, input.moveY, input.sprint);
        for (size_t i = 0; i < CharacterInput::MaxButtons; ++i)
        {
            auto &binding = mBindings[i];
            if (input.buttons[i] == binding.held)
                continue;
            binding.held = input.buttons[i];
            const auto *command = binding.held ? binding.pressed : binding.released;
            if (command)
                command->execute(mController);
        }
    }

    void CharacterInputHandler::reset()
    {
        for (auto &binding : mBindings)
        {
            if (binding.held && binding.released)
                binding.released->execute(mController);
            binding.held = false;
        }
        mController.clearInput();
    }
}
