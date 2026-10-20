#pragma once

#include "idk/EngineAPI.hpp"
#include "idk/EngineClock.hpp"
#include "libidk/dsa/Raii.hpp"

namespace idk
{
    struct EngineEvent;

    class Engine: public idk::ServiceManager
    {
    public:
        // By default, load engine.cfg alongside the executable.
        explicit Engine(const char *configPath = nullptr);
        ~Engine();

        // Configure before startApplication(). Invalid values leave settings intact.
        bool setTimingConfig(const EngineTimingConfig &);
        bool setFixedUpdateRate(double rateHz);
        bool startApplication(idk::Service *app);

    private:
        idk::RaiiFunc<void(Engine*)> mServiceRaii;
        idk::EngineAPI               mApi;
        idk::EngineClock             mClock{};
        idk::EngineTiming            mTiming;
        const char                 *mTimingError = nullptr;
        bool                        mStarted = false;
        char                        mTimingPath[4096]{};

        void engineDeltaTime();
        void engineUpdate();
        void engineRender();
        void reloadTimingConfig();

        void processEvent(const EngineEvent&);

        static void onEngineEventCallback(EngineAPI&, const EngineEvent&);

    };
}
