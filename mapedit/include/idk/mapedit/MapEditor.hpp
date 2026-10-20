#pragma once

#include "MapSession.hpp"
#include "MapFileDialog.hpp"
#include "PlaytestProcess.hpp"
#include "idk/editor/EditorViewport.hpp"
#include "idk/editor/ScenePreview.hpp"
#include "libidk/Service.hpp"

namespace idk::mapedit
{
    class MapEditorApplication final: public idk::Service
    {
    public:
        void onInit(idk::EngineAPI &) override;
        void onShutdown(idk::EngineAPI &) override;
        void onPreRender(idk::EngineAPI &) override;
        void onMidRender(idk::EngineAPI &) override;

    private:
        enum class Action
        {
            None,
            New,
            Open,
            Save,
            Replace,
            Build,
            Play,
            Stop
        };

        bool mInitialized = false;
        MapSession mSession;
        editor::ScenePreview mPreview;
        editor::EditorViewport mViewport;
        char mPath[MapFileDialog::PathCapacity]{};
        char mOverwritePath[MapFileDialog::PathCapacity]{};
        bool mConfirmOverwrite = false;
        bool mAwaitingFile = false;
        char mBuildPath[MapFileDialog::PathCapacity] = "level.idklvl";
        PlaytestProcess mPlaytest;
        const char *mTimingConfigPath = nullptr; // Borrowed from the engine.

        Action draw(gfx::RenderEngine &);
        Action drawFiles();
        Action drawDialogs();
        void drawTools();
        void drawObjects();
        void drawInspector();
        void execute(Action, gfx::RenderEngine &);
        void chooseFile(MapFileDialog::Kind, gfx::RenderEngine &);
        void pollFileDialog(gfx::RenderEngine &);
        void openMap(const char *path, gfx::RenderEngine &);
        void saveMap(const char *path, bool overwrite = false);
        void buildLevel(bool play);
        void pollPlaytest();
        static void onGenericEventCallback(idk::EngineAPI &, const void *);
    };
} // namespace idk::mapedit
