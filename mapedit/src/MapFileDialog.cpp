#include "idk/mapedit/MapFileDialog.hpp"
#include <SDL3/SDL_error.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>

namespace idk::mapedit
{
    bool MapFileDialog::show(Kind kind, SDL_Window *parent, const char *initialPath)
    {
        State expected = State::Idle;
        if (!mState.compare_exchange_strong(expected, State::Pending, std::memory_order_acq_rel))
            return false;
        mResult = {};
        mResult.kind = kind;
        mLocation[0] = 0;

        // Use an absolute default location, starting in the content directory
        // for a new document. Keep the buffer alive through the callback.
        int length = 0;
        if (initialPath && initialPath[0] == '/')
            length = std::snprintf(mLocation, sizeof(mLocation), "%s", initialPath);
        else if (getcwd(mLocation, sizeof(mLocation)))
        {
            size_t used = std::strlen(mLocation);
            const char *separator = used && mLocation[used - 1] == '/' ? "" : "/";
            int appended = std::snprintf(mLocation + used, sizeof(mLocation) - used, "%s%s", separator,
                                         initialPath ? initialPath : "");
            length = appended < 0 ? -1 : int(used) + appended;
        }
        else
            mLocation[0] = 0;
        if (length < 0 || size_t(length) >= sizeof(mLocation))
        {
            fail("The initial file dialog path is too long.");
            return true;
        }

        static constexpr SDL_DialogFileFilter maps[] = {{"Level maps (*.idkmap)", "idkmap"}, {"All files", "*"}};
        static constexpr SDL_DialogFileFilter levels[] = {{"Runtime levels (*.idklvl)", "idklvl"}};
        const char *location = mLocation[0] ? mLocation : nullptr;
        if (kind == Kind::OpenMap)
            SDL_ShowOpenFileDialog(callback, this, parent, maps, int(SDL_arraysize(maps)), location, false);
        else
            SDL_ShowSaveFileDialog(callback, this, parent, kind == Kind::SaveMap ? maps : levels, 1, location);
        return true;
    }

    bool MapFileDialog::poll(Result &result)
    {
        if (mState.load(std::memory_order_acquire) != State::Ready)
            return false;
        result = mResult;
        mState.store(State::Idle, std::memory_order_release);
        return true;
    }

    bool MapFileDialog::busy() const
    {
        return mState.load(std::memory_order_acquire) != State::Idle;
    }

    void MapFileDialog::fail(const char *message)
    {
        mResult.outcome = Outcome::Error;
        std::snprintf(mResult.error, sizeof(mResult.error), "%s", message);
        mState.store(State::Ready, std::memory_order_release);
    }

    void SDLCALL MapFileDialog::callback(void *userdata, const char *const *files, int)
    {
        auto &dialog = *static_cast<MapFileDialog *>(userdata);
        if (!files)
        {
            const char *error = SDL_GetError();
            dialog.fail(error[0] ? error : "Could not open the file dialog.");
            return;
        }
        auto &result = dialog.mResult;
        if (files[0] && files[0][0])
        {
            const char *extension = "";
            if (result.kind == Kind::SaveMap)
                extension = ".idkmap";
            else if (result.kind == Kind::BuildLevel)
                extension = ".idklvl";
            size_t length = std::strlen(files[0]);
            size_t extra = std::strlen(extension);
            if (extra && length >= extra && !SDL_strcasecmp(files[0] + length - extra, extension))
                extra = 0;
            if (length >= sizeof(result.path) || extra >= sizeof(result.path) - length)
            {
                dialog.fail("The selected path is too long for the editor.");
                return;
            }
            std::memcpy(result.path, files[0], length);
            std::memcpy(result.path + length, extension, extra);
            result.path[length + extra] = 0;
            result.outcome = Outcome::Selected;
        }
        // SDL frees the file list on return. Never access editor or rendering
        // state here; poll() applies the copied result on the main thread.
        dialog.mState.store(State::Ready, std::memory_order_release);
    }
}
