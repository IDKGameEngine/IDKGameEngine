#pragma once

#include "libidk/Types.hpp"
#include "libidk/Assert.hpp"
#include "libidk/metric.hpp"
#include <chrono>


namespace idk
{
    class EngineClock
    {
    private:
        std::chrono::steady_clock::time_point mPrevious;

    public:
        EngineClock()
        :   mPrevious(std::chrono::steady_clock::now())
        {
            
        }

        double tick()
        {
            const auto now = std::chrono::steady_clock::now();
            const double dt = std::chrono::duration<double>(now - mPrevious).count();
            mPrevious = now;
            return dt;
        }
    };
    
}
