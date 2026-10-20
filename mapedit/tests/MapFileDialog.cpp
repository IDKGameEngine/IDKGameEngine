#include "idk/mapedit/MapFileDialog.hpp"
#include <SDL3/SDL_error.h>
#include <cstdio>
#include <cstring>
#include <thread>

namespace
{
    SDL_DialogFileCallback pending = nullptr;
    void *pendingUser = nullptr;
    SDL_Window *dialogParent = nullptr;
    const SDL_DialogFileFilter *dialogFilters = nullptr;
    const char *dialogLocation = nullptr;
    int dialogFilterCount = 0, launches = 0;
    bool dialogSave = false, dialogMultiple = true, synchronous = false;

    void record(SDL_DialogFileCallback callback, void *userdata, SDL_Window *parent,
                const SDL_DialogFileFilter *filters, int count, const char *location)
    {
        pending = callback;
        pendingUser = userdata;
        dialogParent = parent;
        dialogFilters = filters;
        dialogFilterCount = count;
        dialogLocation = location;
        ++launches;
        if (synchronous)
        {
            const char *files[] = {"/tmp/synchronous", nullptr};
            callback(userdata, files, 0);
        }
    }

    void selectFromThread()
    {
        char path[] = "/tmp/map with spaces.idkmap";
        const char *files[] = {path, nullptr};
        pending(pendingUser, files, 0);
        // SDL's list becomes invalid after callback return.
        std::memset(path, 'x', sizeof(path) - 1);
    }

    void errorFromThread()
    {
        SDL_SetError("Native file picker unavailable");
        pending(pendingUser, nullptr, -1);
        SDL_ClearError();
    }

    void select(const char *path)
    {
        const char *files[] = {path, nullptr};
        pending(pendingUser, files, 0);
    }
}

// Test the SDL boundary without depending on a desktop picker being available.
extern "C" void SDLCALL SDL_ShowOpenFileDialog(SDL_DialogFileCallback callback, void *userdata, SDL_Window *parent,
                                               const SDL_DialogFileFilter *filters, int count, const char *location,
                                               bool multiple)
{
    dialogSave = false;
    dialogMultiple = multiple;
    record(callback, userdata, parent, filters, count, location);
}

extern "C" void SDLCALL SDL_ShowSaveFileDialog(SDL_DialogFileCallback callback, void *userdata, SDL_Window *parent,
                                               const SDL_DialogFileFilter *filters, int count, const char *location)
{
    dialogSave = true;
    record(callback, userdata, parent, filters, count, location);
}

#define CHECK(value)                                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(value))                                                                                                  \
        {                                                                                                              \
            std::fprintf(stderr, "Check failed: %s (%d)\n", #value, __LINE__);                                         \
            return false;                                                                                              \
        }                                                                                                              \
    } while (false)

bool checks()
{
    using Dialog = idk::mapedit::MapFileDialog;
    using Kind = Dialog::Kind;
    using Outcome = Dialog::Outcome;
    static Dialog dialog;
    Dialog::Result result;
    auto *parent = reinterpret_cast<SDL_Window *>(uintptr_t(1));
    CHECK(!dialog.busy() && !dialog.poll(result));
    CHECK(dialog.show(Kind::OpenMap, parent, "level/playground.idkmap"));
    CHECK(dialog.busy() && !dialog.poll(result));
    CHECK(!dialog.show(Kind::SaveMap, parent, nullptr) && launches == 1);
    CHECK(dialogParent == parent && !dialogSave && !dialogMultiple);
    CHECK(dialogFilterCount == 2 && !std::strcmp(dialogFilters[0].pattern, "idkmap"));
    CHECK(!std::strcmp(dialogFilters[1].pattern, "*"));
    CHECK(dialogLocation && dialogLocation[0] == '/' && std::strstr(dialogLocation, "/level/playground.idkmap"));
    std::thread selected(selectFromThread);
    selected.join();
    CHECK(dialog.busy() && !dialog.show(Kind::OpenMap, parent, nullptr));
    CHECK(dialog.poll(result) && result.kind == Kind::OpenMap && result.outcome == Outcome::Selected);
    CHECK(!std::strcmp(result.path, "/tmp/map with spaces.idkmap"));
    CHECK(!dialog.busy() && !dialog.poll(result));

    CHECK(dialog.show(Kind::OpenMap, parent, ""));
    CHECK(dialogLocation && dialogLocation[std::strlen(dialogLocation) - 1] == '/');
    select("/tmp/legacy-map");
    CHECK(dialog.poll(result) && !std::strcmp(result.path, "/tmp/legacy-map"));

    CHECK(dialog.show(Kind::SaveMap, parent, "/tmp/previous.idkmap"));
    CHECK(dialogSave && dialogParent == parent);
    CHECK(dialogFilterCount == 1 && !std::strcmp(dialogFilters[0].pattern, "idkmap"));
    CHECK(!std::strcmp(dialogLocation, "/tmp/previous.idkmap"));
    select("/tmp/new map");
    CHECK(dialog.poll(result) && result.kind == Kind::SaveMap && result.outcome == Outcome::Selected);
    CHECK(!std::strcmp(result.path, "/tmp/new map.idkmap"));

    CHECK(dialog.show(Kind::SaveMap, parent, nullptr));
    select("/tmp/map.IDKMAP");
    CHECK(dialog.poll(result) && !std::strcmp(result.path, "/tmp/map.IDKMAP"));
    CHECK(dialog.show(Kind::BuildLevel, parent, "level.idklvl"));
    CHECK(dialogSave && dialogFilterCount == 1 && !std::strcmp(dialogFilters[0].pattern, "idklvl"));
    select("/tmp/runtime");
    CHECK(dialog.poll(result) && result.kind == Kind::BuildLevel);
    CHECK(!std::strcmp(result.path, "/tmp/runtime.idklvl"));

    const Kind kinds[] = {Kind::OpenMap, Kind::SaveMap, Kind::BuildLevel};
    for (auto kind : kinds)
    {
        CHECK(dialog.show(kind, parent, nullptr));
        select(nullptr);
        CHECK(dialog.poll(result) && result.kind == kind && result.outcome == Outcome::Cancelled);
        CHECK(!result.path[0] && !result.error[0] && !dialog.busy());
        CHECK(dialog.show(kind, parent, nullptr));
        select("");
        CHECK(dialog.poll(result) && result.outcome == Outcome::Cancelled);
        CHECK(dialog.show(kind, parent, nullptr));
        std::thread error(errorFromThread);
        error.join();
        CHECK(dialog.poll(result) && result.outcome == Outcome::Error && !result.path[0]);
        CHECK(!std::strcmp(result.error, "Native file picker unavailable"));
    }

    CHECK(dialog.show(Kind::OpenMap, parent, nullptr));
    SDL_ClearError();
    pending(pendingUser, nullptr, -1);
    CHECK(dialog.poll(result) && result.outcome == Outcome::Error && result.error[0]);

    char maximum[Dialog::PathCapacity];
    std::memset(maximum, 'a', sizeof(maximum) - 1);
    maximum[sizeof(maximum) - 1] = 0;
    CHECK(dialog.show(Kind::OpenMap, parent, nullptr));
    select(maximum);
    CHECK(dialog.poll(result) && result.outcome == Outcome::Selected);
    CHECK(!std::strcmp(result.path, maximum));
    // Appending the save extension must reject overflow rather than truncate.
    CHECK(dialog.show(Kind::SaveMap, parent, nullptr));
    select(maximum);
    CHECK(dialog.poll(result) && result.outcome == Outcome::Error && !result.path[0]);
    maximum[sizeof(maximum) - 8] = 0;
    CHECK(dialog.show(Kind::SaveMap, parent, nullptr));
    select(maximum);
    CHECK(dialog.poll(result) && result.outcome == Outcome::Selected);
    CHECK(std::strlen(result.path) == Dialog::PathCapacity - 1);
    CHECK(!std::strcmp(result.path + Dialog::PathCapacity - 8, ".idkmap"));

    char tooLong[Dialog::PathCapacity + 1];
    std::memset(tooLong, 'a', sizeof(tooLong) - 1);
    tooLong[sizeof(tooLong) - 1] = 0;
    CHECK(dialog.show(Kind::BuildLevel, parent, nullptr));
    select(tooLong);
    CHECK(dialog.poll(result) && result.outcome == Outcome::Error && !result.path[0]);

    char initial[4097];
    std::memset(initial, 'a', sizeof(initial) - 1);
    initial[0] = '/';
    initial[sizeof(initial) - 1] = 0;
    int before = launches;
    CHECK(dialog.show(Kind::SaveMap, parent, initial));
    CHECK(dialog.poll(result) && result.outcome == Outcome::Error && launches == before);

    synchronous = true;
    CHECK(dialog.show(Kind::SaveMap, parent, nullptr));
    CHECK(dialog.poll(result) && result.outcome == Outcome::Selected);
    CHECK(!std::strcmp(result.path, "/tmp/synchronous.idkmap"));
    synchronous = false;
    // The mailbox may outlive an editor that is no longer listening.
    CHECK(dialog.show(Kind::OpenMap, parent, nullptr));
    std::thread late(selectFromThread);
    late.join();
    CHECK(dialog.poll(result) && !dialog.busy());
    return true;
}

int main()
{
    if (!checks())
        return 1;
    std::puts("SDL map open/save dialog, cancellation, callback and path checks passed");
    return 0;
}
