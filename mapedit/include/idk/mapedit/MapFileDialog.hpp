#pragma once

#include <SDL3/SDL_dialog.h>
#include <atomic>

namespace idk::mapedit
{
    // SDL may finish on another thread or after the editor shuts down. Keep this
    // mailbox alive until its callback returns; mapedit uses process storage.
    // show/poll are main-thread operations. Callbacks only copy bounded results.
    class MapFileDialog
    {
    public:
        static constexpr size_t PathCapacity = 1024;

        enum class Kind
        {
            OpenMap,
            SaveMap,
            BuildLevel
        };

        enum class Outcome
        {
            Selected,
            Cancelled,
            Error
        };

        struct Result
        {
            Kind kind = Kind::OpenMap;
            Outcome outcome = Outcome::Cancelled;
            char path[PathCapacity]{};
            char error[512]{};
        };

        bool show(Kind, SDL_Window *parent, const char *initialPath);
        bool poll(Result &);
        bool busy() const;

    private:
        enum class State
        {
            Idle,
            Pending,
            Ready
        };

        std::atomic<State> mState{State::Idle};
        Result mResult;
        char mLocation[4096]{};

        void fail(const char *message);
        static void SDLCALL callback(void *, const char *const *, int);
    };
}
