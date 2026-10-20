#include "idk/EngineTiming.hpp"
#include "libidk/cfgparser.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <unistd.h>

#define CHECK(value)                                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(value))                                                                                                  \
        {                                                                                                              \
            std::fprintf(stderr, "Check failed: %s (%d)\n", #value, __LINE__);                                          \
            return false;                                                                                              \
        }                                                                                                              \
    } while (false)

namespace
{
    class ConfigFile
    {
    public:
        char path[64] = "/tmp/idk-timing-XXXXXX";
        ConfigFile()
        {
            int fd = mkstemp(path);
            if (fd >= 0)
                close(fd);
            else
                path[0] = 0;
        }
        ~ConfigFile() { if (path[0]) unlink(path); }
        bool write(const char *text)
        {
            std::FILE *file = std::fopen(path, "wb");
            if (!file)
                return false;
            bool written = std::fputs(text, file) >= 0;
            return std::fclose(file) == 0 && written;
        }
    };

    bool fileChecks()
    {
        ConfigFile file;
        CHECK(file.path[0]);
        CHECK(file.write("# Runtime settings\r\n[timing] {\r\n\tfixed_hz = 20\r\ninput_hz = 240\r\n"
                         "render_hz = 59.94\r\nphysics_hz = 240\r\nvsync = 0\r\n}"));
        idk::EngineTimingConfig config;
        CHECK(!config.loadFile(file.path));
        CHECK(config.fixedRateHz == 20 && config.inputRateHz == 240 && config.renderRateHz == 59.94 &&
              config.physicsRateHz == 240 && !config.vsync);
        const char *invalid[] = {
            "[timing] {\nfixed_hz = nan\n}", "[timing] {\nfixed_hz = 0\n}",
            "[timing] {\nfixed_hz = 1001\n}", "[timing] {\nfixed_hz = 60Hz\n}",
            "[timing] {\nfixed_hz = 60\nvsync = yes\n}", "[timing] {\ninput_hz = -1\n}",
            "[timing] {\nphysics_hz = 29\n}", "[timing] {\nrender_hz = inf\n}",
            "[timing] {\nfixed_hz = \n}", "[timing] {\nfixed_hz = 60\n",
            "[timing] {\nfixed_hz = 60\nfixed_hz = 120\n}", "[timing] {\nfixed_hz = \"60\n}",
            "[timing] {\n[fixed_hz] {}\n}", "[timing] {\nfiixed_hz = 60\n}",
            "[timing] nonsense", "?", "[timing] {}\n[timing] {}"};
        for (const char *text : invalid)
        {
            CHECK(file.write(text));
            CHECK(config.loadFile(file.path));
            CHECK(config.fixedRateHz == 20 && config.inputRateHz == 240 && config.renderRateHz == 59.94);
        }
        // Existing nested/quoted syntax, whitespace, comments, and transactional reload.
        idk::CfgParser parser;
        CHECK(file.write("[libidk] {\n[window] {\ntitle = \"hello world\"\nwidth = 1280\n}\n}"));
        CHECK(parser.load(file.path));
        CHECK(!std::strcmp(parser["libidk"]["window"]["title"].toStr(), "hello world"));
        CHECK(parser["libidk"]["window"]["width"].toI32() == 1280);
        CHECK(file.write("[broken] {"));
        CHECK(!parser.load(file.path));
        CHECK(parser["libidk"]["window"]["width"].toI32() == 1280);
        CHECK(file.write(""));
        CHECK(parser.load(file.path) && !parser.find("libidk"));
        config = {};
        CHECK(!config.loadFile(file.path));
        CHECK(config.fixedRateHz == 30 && config.physicsRateHz == 120 && config.vsync);
        CHECK(file.write("[timing] { input_hz = 120\n}"));
        CHECK(!config.loadFile(file.path));
        CHECK(config.inputRateHz == 120 && config.fixedRateHz == 30);
        CHECK(unlink(file.path) == 0);
        CHECK(config.loadFile(file.path));
        CHECK(!config.loadFile(file.path, true));
        return true;
    }

    bool scheduleChecks(double inputRate, double renderRate, unsigned expectedInputs, unsigned expectedRenders)
    {
        idk::EngineTimingConfig config;
        config.fixedRateHz = 60;
        config.inputRateHz = inputRate;
        config.renderRateHz = renderRate;
        idk::EngineTiming timing;
        CHECK(timing.configure(config));
        unsigned inputs = 0, renders = 0, ticks = 0;
        double inputTime = 0, renderTime = 0;
        for (unsigned i = 0; i <= 2000; ++i)
        {
            if (i)
                timing.advance(.001);
            if (timing.inputDue())
            {
                inputTime += timing.consumeInput();
                ++inputs;
            }
            while (timing.stepFixed())
                ++ticks;
            CHECK(timing.interpolationAlpha() >= 0 && timing.interpolationAlpha() < 1);
            if (timing.renderDue())
            {
                renderTime += timing.consumeRender();
                ++renders;
            }
        }
        CHECK(inputs == expectedInputs && renders == expectedRenders && ticks == 120);
        CHECK(std::abs(inputTime - 2) < 1e-8 && std::abs(renderTime - 2) < 1e-8);
        return true;
    }

    bool checks()
    {
        // Poll faster or slower than rendering; fixed simulation keeps its rate.
        CHECK(scheduleChecks(240, 60, 481, 121));
        CHECK(scheduleChecks(30, 120, 61, 241));
        CHECK(scheduleChecks(0, 60, 121, 121));
        CHECK(scheduleChecks(240, 0, 481, 2001));
        CHECK(scheduleChecks(0, 0, 2001, 2001));

        idk::EngineTimingConfig config;
        config.fixedRateHz = 60;
        config.inputRateHz = 240;
        config.renderRateHz = 30;
        idk::EngineTiming timing;
        CHECK(timing.configure(config));
        timing.consumeInput();
        timing.consumeRender();
        CHECK(std::abs(timing.waitTimeSec() - 1.0 / 240) < 1e-8);
        // A delayed frame polls once, renders once, and catches physics up.
        timing.advance(.12);
        CHECK(timing.inputDue() && timing.renderDue());
        CHECK(std::abs(timing.consumeInput() - .12) < 1e-8);
        CHECK(std::abs(timing.consumeRender() - .12) < 1e-8);
        CHECK(!timing.inputDue() && !timing.renderDue());
        unsigned ticks = 0;
        while (timing.stepFixed())
            ++ticks;
        CHECK(ticks == 7 && std::abs(timing.interpolationAlpha() - .2) < 1e-8);
        config.fixedRateHz = 120;
        CHECK(timing.reconfigure(config));
        CHECK(std::abs(timing.interpolationAlpha() - .2) < 1e-8);
        CHECK(timing.inputDue() && timing.renderDue());
        timing.consumeInput();
        timing.consumeRender();
        // Long stalls retain the existing 250 ms catch-up budget.
        timing.advance(1);
        ticks = 0;
        while (timing.stepFixed())
            ++ticks;
        CHECK(ticks == 30);
        double alpha = timing.interpolationAlpha();
        timing.advance(-1);
        timing.advance(std::numeric_limits<double>::quiet_NaN());
        CHECK(timing.interpolationAlpha() == alpha);
        config.fixedRateHz = 0;
        CHECK(!timing.configure(config));
        CHECK(std::abs(timing.fixedDeltaTimeSec() - 1.0 / 120) < 1e-8);
        return fileChecks();
    }
}

int main()
{
    if (!checks())
        return 1;
    std::puts("Independent input/render scheduling, physics catch-up and timing configuration checks passed");
    return 0;
}
