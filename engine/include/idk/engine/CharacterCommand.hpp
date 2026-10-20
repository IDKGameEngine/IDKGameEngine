#pragma once

#include "CharacterController.hpp"

namespace idk::engine
{
    // Commands borrow their receiver at execution time and can be shared by
    // multiple controllers. Subclass this interface for game-specific actions.
    class CharacterCommand
    {
    public:
        virtual ~CharacterCommand() = default;
        virtual bool execute(CharacterController &) const = 0;
    };

    class JumpCommand final: public CharacterCommand
    {
    public:
        bool execute(CharacterController &controller) const override { return controller.jump(); }
    };

    class CrouchCommand final: public CharacterCommand
    {
    public:
        explicit CrouchCommand(bool crouching) : mCrouching(crouching) {}

        bool execute(CharacterController &controller) const override { return controller.crouch(mCrouching); }

    private:
        bool mCrouching;
    };

    class AttackCommand final: public CharacterCommand
    {
    public:
        bool execute(CharacterController &controller) const override { return controller.attack(); }
    };
}
