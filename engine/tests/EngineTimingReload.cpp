#include "idk/engine/Engine.hpp"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <unistd.h>

namespace
{
    bool writeConfig(const char *path, const char *text)
    {
        std::FILE *file = std::fopen(path, "wb");
        if (!file)
            return false;
        bool written = std::fputs(text, file) >= 0;
        return std::fclose(file) == 0 && written;
    }

    class ReloadCheck final: public idk::Service
    {
    public:
        explicit ReloadCheck(const char *path) : mPath(path) {}
        bool passed = true;
        bool completed = false;

        void onInit(idk::EngineAPI &) override {}
        void onShutdown(idk::EngineAPI &) override {}
        void onUpdate(idk::EngineAPI &api) override
        {
            ++mUpdates;
            const auto &config = api.getTimingConfig();
            if (mUpdates <= 4)
                passed &= config.fixedRateHz == 30 && config.inputRateHz == 120;
            else
                passed &= config.fixedRateHz == 60 && config.inputRateHz == 240 &&
                          config.renderRateHz == 120 && config.physicsRateHz == 240 &&
                          std::abs(api.getFixedDeltaTimeSec() - 1.0 / 60) < 1e-8;
            if (mUpdates == 1)
                passed &= writeConfig(mPath, "[timing] {\nfixed_hz=60\ninput_hz=240\nrender_hz=120\n"
                                             "physics_hz=240\nvsync=0\n}");
            // Editing alone must not reload. Exercise the actual F5 event path.
            if (mUpdates == 3)
            {
                SDL_Event event{};
                event.type = SDL_EVENT_KEY_DOWN;
                event.key.scancode = SDL_SCANCODE_F5;
                passed &= SDL_PushEvent(&event);
            }
            if (mUpdates == 5)
            {
                passed &= writeConfig(mPath, "[timing] {\nfixed_hz=bad\n}");
                api.requestTimingReload();
            }
            // A failed reload preserves all active rates.
            completed = mUpdates == 8;
            if (!passed || mUpdates == 8)
                api.quit();
        }

    private:
        const char *mPath;
        unsigned mUpdates = 0;
    };
}

int main()
{
    char path[] = "/tmp/idk-reload-XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0)
        return 1;
    close(fd);
    if (!writeConfig(path, "[timing] {\nfixed_hz=30\ninput_hz=120\nrender_hz=60\nvsync=0\n}"))
    {
        unlink(path);
        return 1;
    }
    idk::Engine engine(path);
    ReloadCheck check(path);
    bool ran = engine.startApplication(&check);
    unlink(path);
    if (!ran || !check.passed || !check.completed)
        return 1;
    std::puts("Manual F5/API timing reload, no automatic reload, and failure retention checks passed");
    return 0;
}
