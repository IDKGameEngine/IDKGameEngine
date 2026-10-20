#pragma once

#include "libidk/Service.hpp"


namespace idk
{
    class VideoManager: public idk::Service
    {
    private:
        const char      *mTitle;
        void            *mWin;
        int32_t          mWidth;
        int32_t          mHeight;

    public:
        VideoManager(const char *title, int w, int h);

        virtual void onInit(EngineAPI&) final;
        virtual void onShutdown(EngineAPI&) final;

        void *getWindowHandle() { return mWin; }

        int   getWidth()  { return mWidth; };
        int   getHeight() { return mHeight; };

        void  setWindowFullscreen(bool fullscreen);
        void  setWindowVisibility(bool visible);
        void  setWindowResolution(int w, int h);
        void  setRenderResolution(int w, int h);

    };
}

