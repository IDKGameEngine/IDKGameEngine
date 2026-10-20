#include "idk/engine/Engine.hpp"
#include "idk/gfx/RenderEngine.hpp"
#include <cstdio>
#include <thread>
#include <cstdlib>
#include <SDL3/SDL_filesystem.h>


static void EngineServiceRaiiFunc(idk::Engine *E)
{
    E->addService<idk::PlatformManager>();
    E->addService<idk::AudioManager>();
    E->addService<idk::EventManager>();
    E->addService<idk::InputManager>();
    E->addService<idk::VideoManager>("GameWindow", 1280, 720);
    E->addService<idk::gfx::RenderEngine>();
}


idk::Engine::Engine(const char *configPath)
:   mServiceRaii(EngineServiceRaiiFunc, this),
    mApi(this)
{
    if (configPath)
    {
        if (!realpath(configPath, mTimingPath))
            mTimingError = "Could not resolve configuration path";
    }
    else
    {
        const char *base = SDL_GetBasePath();
        int length = base ? std::snprintf(mTimingPath, sizeof(mTimingPath), "%sengine.cfg", base) : -1;
        if (length < 0 || size_t(length) >= sizeof(mTimingPath))
            mTimingError = "Could not locate configuration beside executable";
    }
    mApi.mTimingConfigPath = mTimingPath;
    if (!mTimingError)
        mTimingError = mApi.mTimingConfig.loadFile(mTimingPath, !configPath);
    mTiming.configure(mApi.mTimingConfig);
    mApi.mFixedDeltaTimeSec = mTiming.fixedDeltaTimeSec();

    mApi.mEvent->addEngineCallback(Engine::onEngineEventCallback);
    // mApi.mEvent->addEngineCallback()
}


idk::Engine::~Engine()
{
    
}


bool idk::Engine::setTimingConfig(const EngineTimingConfig &config)
{
    if (mStarted || !mTiming.configure(config))
        return false;
    mApi.mTimingConfig = config;
    mApi.mFixedDeltaTimeSec = mTiming.fixedDeltaTimeSec();
    mTimingError = nullptr;
    return true;
}

bool idk::Engine::setFixedUpdateRate(double rateHz)
{
    auto config = mApi.mTimingConfig;
    config.fixedRateHz = rateHz;
    return setTimingConfig(config);
}

bool idk::Engine::startApplication(idk::Service *app)
{
    if (mStarted || !app)
        return false;
    if (mTimingError)
    {
        std::fprintf(stderr, "Timing configuration '%s': %s. Fixed: 4..1000 Hz; input/render: 0 or 1..1000 Hz; "
                             "physics: 30..1000 Hz; vsync: 0 or 1.\n", mTimingPath, mTimingError);
        return false;
    }
    mStarted = true;
    addService(app);

    dispatchInit(mApi);
    mClock.tick(); // Initialization time is not simulation time.
    while (!mApi.mShouldQuit.load())
    {
        engineDeltaTime();
        engineUpdate();
        if (mApi.mShouldQuit.load())
            break;
        if (mApi.mTimingReloadRequested)
            reloadTimingConfig();
        if (mTiming.renderDue())
            engineRender();
        if (!mApi.mShouldQuit.load())
        {
            // Account for callback/render work before sleeping. VSync or a slow
            // render can still limit polling on this single-threaded loop.
            mTiming.advance(mClock.tick());
            double wait = mTiming.waitTimeSec();
            if (wait > 0)
                std::this_thread::sleep_for(std::chrono::duration<double>(wait));
        }
    }
    dispatchShutdown(mApi);
    return true;
}

void idk::Engine::reloadTimingConfig()
{
    mApi.mTimingReloadRequested = false;
    EngineTimingConfig config;
    if (const char *error = config.loadFile(mTimingPath))
    {
        std::fprintf(stderr, "Timing reload failed (%s): %s. Current settings retained.\n", mTimingPath, error);
        return;
    }
    if (config.vsync != mApi.mTimingConfig.vsync)
    {
        auto *renderer = getService<gfx::RenderEngine>();
        if (!renderer || !renderer->SetVSync(config.vsync))
        {
            std::fputs("Timing reload failed: could not change VSync. Current settings retained.\n", stderr);
            return;
        }
    }
    mTiming.reconfigure(config);
    mApi.mTimingConfig = config;
    mApi.mFixedDeltaTimeSec = mTiming.fixedDeltaTimeSec();
    mApi.mFixedUpdateAlpha = mTiming.interpolationAlpha();
    std::fprintf(stderr, "Timing reloaded (%s): input=%g, fixed=%g, physics=%g, render=%g Hz, vsync=%d\n",
                 mTimingPath, config.inputRateHz, config.fixedRateHz, config.physicsRateHz,
                 config.renderRateHz, int(config.vsync));
}


void idk::Engine::engineDeltaTime()
{
    mTiming.advance(mClock.tick());
}

void idk::Engine::engineUpdate()
{
    // Variable rate update
    // -----------------------------------------------------
    if (mTiming.inputDue())
    {
        mApi.mInputDeltaTimeSec = mTiming.consumeInput();
        mApi.mDeltaTimeSec = mApi.mInputDeltaTimeSec;
        dispatchUpdate(mApi);
    }
    if (mApi.mShouldQuit.load())
        return;
    // -----------------------------------------------------

    // Fixed rate update
    // -----------------------------------------------------
    while (mTiming.stepFixed())
    {
        dispatchFixedUpdate(mApi);
    }
    mApi.mFixedUpdateAlpha = mTiming.interpolationAlpha();
    // -----------------------------------------------------
}

void idk::Engine::engineRender()
{
    mApi.mRenderDeltaTimeSec = mTiming.consumeRender();
    mApi.mDeltaTimeSec = mApi.mRenderDeltaTimeSec;
    dispatchPreRender(mApi);
    dispatchMidRender(mApi);
    dispatchPostRender(mApi);
}



void idk::Engine::onEngineEventCallback(idk::EngineAPI &api, const idk::EngineEvent &e)
{
    using namespace idk;

    switch (e.type)
    {
        case EngineEvent::Quit:
            api.quit();
            break;
        case EngineEvent::Pause:
            break;
        case EngineEvent::Resume:
            break;
        default:
            break;
    }
}

