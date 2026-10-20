#pragma once

#include <algorithm>
#include <cmath>

namespace idk
{
    struct EngineTimingConfig
    {
        static constexpr double MaxDeltaTimeSec = .25;
        double fixedRateHz = 30;
        double inputRateHz = 0; // Zero polls once per render.
        double renderRateHz = 0; // Zero leaves rendering uncapped.
        double physicsRateHz = 120; // Minimum player substep frequency.
        bool vsync = true;

        bool valid() const;
        // Read [timing] through libidk::CfgParser. Failure leaves this unchanged.
        // Return a static error/setting name or nullptr. Missing keys keep defaults.
        const char *loadFile(const char *path, bool optional = false);
    };

    // Single-threaded scheduling, independent of SDL and wall-clock sleeping.
    // Input/render deadlines skip missed polls/frames; physics catches up.
    class EngineTiming
    {
    public:
        bool configure(const EngineTimingConfig &config)
        {
            if (!config.valid())
                return false;
            mConfig = config;
            mTime = mFixedAccum = mLastInput = mLastRender = 0;
            mNextInput = mNextRender = 0;
            return true;
        }

        // Apply at an update boundary. Preserve interpolation phase, elapsed
        // time and the last input/render timestamps instead of restarting time.
        bool reconfigure(const EngineTimingConfig &config)
        {
            if (!config.valid())
                return false;
            double alpha = interpolationAlpha();
            mConfig = config;
            mFixedAccum = alpha * fixedDeltaTimeSec();
            mNextInput = mNextRender = mTime;
            return true;
        }

        void advance(double dt)
        {
            if (!std::isfinite(dt) || dt <= 0)
                return;
            dt = std::min(dt, EngineTimingConfig::MaxDeltaTimeSec);
            mTime += dt;
            mFixedAccum += dt;
        }

        bool renderDue() const { return mConfig.renderRateHz == 0 || mTime + Epsilon >= mNextRender; }
        bool inputDue() const
        {
            return mConfig.inputRateHz == 0 ? renderDue() : mTime + Epsilon >= mNextInput;
        }

        double consumeInput() { return consume(mConfig.inputRateHz, mLastInput, mNextInput); }
        double consumeRender() { return consume(mConfig.renderRateHz, mLastRender, mNextRender); }

        double fixedDeltaTimeSec() const { return 1.0 / mConfig.fixedRateHz; }

        bool stepFixed()
        {
            double step = fixedDeltaTimeSec();
            if (mFixedAccum + Epsilon < step)
                return false;
            mFixedAccum = std::max(0.0, mFixedAccum - step);
            return true;
        }

        double interpolationAlpha() const { return mFixedAccum / fixedDeltaTimeSec(); }

        double waitTimeSec() const
        {
            if (renderDue() || inputDue())
                return 0;
            double wait = std::min(mNextRender - mTime, fixedDeltaTimeSec() - mFixedAccum);
            if (mConfig.inputRateHz != 0)
                wait = std::min(wait, mNextInput - mTime);
            return std::max(0.0, wait);
        }

    private:
        static constexpr double Epsilon = 1e-10;
        EngineTimingConfig mConfig;
        double mTime = 0, mFixedAccum = 0;
        double mLastInput = 0, mLastRender = 0;
        double mNextInput = 0, mNextRender = 0;

        double consume(double rate, double &last, double &next)
        {
            double dt = mTime - last;
            last = mTime;
            if (rate != 0)
            {
                double period = 1.0 / rate;
                next += (std::floor(std::max(0.0, mTime - next + Epsilon) / period) + 1) * period;
            }
            return dt;
        }
    };
}
