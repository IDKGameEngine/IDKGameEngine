#include "idk/platform/PlatformManager.hpp"
#include "idk/EngineAPI.hpp"

#include <SDL3/SDL_filesystem.h>
#include <steam/steam_api.h>
#include <filesystem>


idk::PlatformManager::PlatformManager()
:   mSteamRuntimeDetected( bool(std::getenv("SteamEnv")) )
{

}


void idk::PlatformManager::onInit(idk::EngineAPI&)
{
    namespace fs = std::filesystem;

    if (mSteamRuntimeDetected)
    {
        IDK_ASSERT(SteamAPI_Init(), "SteamAPI_Init failure");
    }

    fs::current_path(fs::path(SDL_GetBasePath()) / fs::path(IDK_CONTENT_BASEPATH));
}


void idk::PlatformManager::onShutdown(idk::EngineAPI&)
{
    if (mSteamRuntimeDetected)
    {
        SteamAPI_Shutdown();
    }
}


void idk::PlatformManager::onUpdate(idk::EngineAPI&)
{

}
