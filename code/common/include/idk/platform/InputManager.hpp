#pragma once

#include "libidk/Service.hpp"


namespace idk
{
    class InputManager: public idk::Service
    {
    private:

    public:
        virtual void onInit(EngineAPI&) final;
        virtual void onShutdown(EngineAPI&) final;
        virtual void onUpdate(EngineAPI&) final;

        bool isKeyDown(int keycode) const;
        bool isMouseButtonDown(int button) const;
        void getMousePosition(int& x, int& y) const;
    };

}

