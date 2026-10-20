#pragma once

#include "libidk/Service.hpp"


namespace idk
{
    class PlatformManager: public idk::Service
    {
    public:
        PlatformManager();
        virtual void onInit(EngineAPI&) final;
        virtual void onShutdown(EngineAPI&) final;
        virtual void onUpdate(EngineAPI&) final;

    private:
        bool mSteamRuntimeDetected;

    };
}
