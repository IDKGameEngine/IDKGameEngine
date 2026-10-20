#include "idk/level/Playtest.hpp"
#include "idk/engine/CharacterCommand.hpp"
#include "idk/asset/PrimitiveShape.hpp"
#include "idk/gfx/RenderEngine.hpp"
#include "idk/EngineAPI.hpp"
#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace idk::level
{
    namespace
    {
        constexpr size_t JumpButton = 0;
        const engine::JumpCommand Jump;
    }

    Playtest::Playtest(const char *path, uint32_t frameLimit, const char *levelRoot)
        : mPath(path), mRoot(levelRoot), mFrameLimit(frameLimit)
    {
        mInput.bind(JumpButton, &Jump);
    }

    void Playtest::onInit(EngineAPI &api)
    {
        mPlayer.setPhysicsRate(api.getTimingConfig().physicsRateHz);
        auto *renderer = api.getService<gfx::RenderEngine>();
        if (!renderer || !renderer->IsInitialized())
        {
            mFailed = true;
            api.quit();
            return;
        }
        static LevelRunner::Storage storage;
        alignas(16) static std::byte scratch[64 * 1024];
        alignas(16) static std::byte physicsStorage[64 * 1024];
        idk::Arena temporary(scratch);
        idk::Arena physics(physicsStorage);
        Error error;
        if (!mPhysics.initialize(physics, error) || !mPlayer.initialize(mPhysics, error) ||
            !mRunner.initialize(storage, mPhysics, mPlayer, *renderer, mRoot, error) ||
            !mRunner.loadInitial(mPath, error))
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Playtest: %s", error.message);
            mFailed = true;
            api.quit();
            return;
        }
        asset::Primitive box;
        if (!asset::CreateShape(asset::ShapeKind::Box, temporary, box))
        {
            mFailed = true;
            api.quit();
            return;
        }
        const gfx::VertexAttribute attributes[] = {
            {0, gfx::VertexAttribType::Float3, offsetof(asset::Vertex, position)},
            {1, gfx::VertexAttribType::Float2, offsetof(asset::Vertex, uv)},
            {4, gfx::VertexAttribType::Float4, offsetof(asset::Vertex, color)}};
        mExitMesh = renderer->CreateMesh({box.vertices.data(), box.vertices.size(), sizeof(asset::Vertex),
                                          box.indices.data(), box.indices.size(), attributes});
        gfx::MaterialDesc beacon;
        beacon.shader = renderer->GetDefaultShader();
        beacon.baseColor = {1, .45f, .1f, .35f};
        beacon.blend = gfx::BlendMode::Alpha;
        beacon.doubleSided = true;
        beacon.depthWrite = false;
        mExitMaterial = renderer->CreateMaterial(beacon);
        if (!mRunner.model().isValid() || !mExitMesh.isValid() || !mExitMaterial.isValid())
        {
            mFailed = true;
            api.quit();
            return;
        }
        renderer->SetClearColor({.075f, .085f, .105f, 1});
        restart();
        auto *window = static_cast<SDL_Window *>(renderer->GetWindowHandle());
        SDL_SetWindowTitle(window,
                           "Playtest - click to look | WASD move | Space jump | R restart | Tab cursor | Esc close");
        SDL_Log("Playtest loaded %zu terrain objects at entry %.3f %.3f %.3f", mRunner.current().terrain.size(),
                mRunner.current().entry.position.x, mRunner.current().entry.position.y,
                mRunner.current().entry.position.z);
    }

    void Playtest::onShutdown(EngineAPI &api)
    {
        mInput.reset();
        mPlayer.shutdown();
        mRunner.shutdown();
        mPhysics.shutdown();
        if (auto *renderer = api.getService<gfx::RenderEngine>())
        {
            SDL_SetWindowRelativeMouseMode(static_cast<SDL_Window *>(renderer->GetWindowHandle()), false);
            renderer->DestroyMesh(mExitMesh);
            renderer->DestroyMaterial(mExitMaterial);
        }
    }

    void Playtest::restart()
    {
        const auto &entry = mRunner.current().entry;
        mController.reset(entry.position, entry.rotation.y, entry.rotation.x);
        float minimum = mRunner.current().entry.position.y;
        for (const auto &object : mRunner.current().terrain)
            for (const auto &vertex : object.vertices)
                minimum = std::min(minimum, vertex.position.y);
        mFallLimit = minimum - 50;
        mReached = mRunner.current().atExit(mPlayer.position());
    }

    void Playtest::onUpdate(EngineAPI &api)
    {
        if (mFailed)
            return;
        auto &renderer = *api.getService<gfx::RenderEngine>();
        auto *window = static_cast<SDL_Window *>(renderer.GetWindowHandle());
        const bool *keys = SDL_GetKeyboardState(nullptr);
        bool focused = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
        if (focused && keys[SDL_SCANCODE_ESCAPE])
            api.quit();
        bool tab = focused && keys[SDL_SCANCODE_TAB];
        if (tab && !mTabHeld)
            mCaptured = !mCaptured;
        mTabHeld = tab;
        if (!focused)
            mCaptured = false;
        if (focused && !mCaptured && (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK))
            mCaptured = true;
        SDL_SetWindowRelativeMouseMode(window, mCaptured);
        float mouseX = 0, mouseY = 0;
        SDL_GetRelativeMouseState(&mouseX, &mouseY);
        bool restartKey = focused && keys[SDL_SCANCODE_R];
        bool restarting = restartKey && !mRestartHeld;
        if (restarting)
        {
            restart();
            SDL_SetWindowTitle(window, "Playtest restarted - WASD move | Space jump | R restart | Esc close");
        }
        mRestartHeld = restartKey;
        if (focused && mCaptured)
        {
            engine::CharacterInput input;
            input.moveX = float(int(keys[SDL_SCANCODE_D]) - int(keys[SDL_SCANCODE_A]));
            input.moveY = float(int(keys[SDL_SCANCODE_W]) - int(keys[SDL_SCANCODE_S]));
            input.lookX = restarting ? 0 : -mouseX * .12f;
            input.lookY = restarting ? 0 : -mouseY * .12f;
            input.sprint = keys[SDL_SCANCODE_LSHIFT];
            input.buttons[JumpButton] = keys[SDL_SCANCODE_SPACE];
            mInput.update(input);
        }
        else
        {
            mInput.reset();
        }
    }

    void Playtest::onFixedUpdate(EngineAPI &api)
    {
        if (mFailed)
            return;
        mPlayer.setPhysicsRate(api.getTimingConfig().physicsRateHz);
        mController.step(float(api.getFixedDeltaTimeSec()));
        if (mPlayer.position().y < mFallLimit)
            restart();
        bool inside = mRunner.current().atExit(mPlayer.position());
        if (!inside)
            mReached = false;
        if (!mReached && inside)
        {
            mReached = true;
            auto *window = static_cast<SDL_Window *>(api.getService<gfx::RenderEngine>()->GetWindowHandle());
            const auto &destination = mRunner.current().destination;
            if (!destination.path[0])
            {
                SDL_SetWindowTitle(window, "Exit reached! - R restart | Esc return to editor");
                SDL_Log("Playtest exit reached");
            }
            else
            {
                Error error;
                if (!mRunner.pending() && mRunner.requestTransition(destination, error))
                    SDL_Log("Level transition queued: %s", destination.path);
            }
        }
    }

    void Playtest::onPreRender(EngineAPI &api)
    {
        if (mFailed)
            return;
        auto &renderer = *api.getService<gfx::RenderEngine>();
        Error error;
        auto transition = mRunner.processPending(error);
        if (transition == LevelRunner::Transition::Changed)
        {
            restart();
            SDL_SetWindowTitle(static_cast<SDL_Window *>(renderer.GetWindowHandle()),
                               "Playtest - new level | WASD move | R restart | Esc close");
            SDL_Log("Level transition committed: %s", mRunner.currentPath());
        }
        else if (transition == LevelRunner::Transition::Failed)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Level transition failed: %s", error.message);
            SDL_SetWindowTitle(static_cast<SDL_Window *>(renderer.GetWindowHandle()),
                               "Transition failed - current level retained | Leave and re-enter exit to retry");
        }
        int width = 0, height = 0;
        SDL_GetWindowSizeInPixels(static_cast<SDL_Window *>(renderer.GetWindowHandle()), &width, &height);
        if (width > 0 && height > 0)
        {
            float yaw = glm::radians(mController.yaw()), pitch = glm::radians(mController.pitch());
            auto eye = mPlayer.interpolatedPosition(float(api.getFixedUpdateAlpha())) + glm::vec3(0, 1.6f, 0);
            glm::vec3 forward(-std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch));
            gfx::Camera camera;
            camera.view = glm::lookAt(eye, eye + forward, glm::vec3(0, 1, 0));
            camera.proj = glm::perspectiveRH_NO(glm::radians(70.0f), float(width) / height, .05f, 20000.0f);
            renderer.SetCamera(camera);
            if (!renderer.SubmitModel(mRunner.model()) ||
                !renderer.Submit({mExitMesh, mExitMaterial, mRunner.current().exit.transform()}))
            {
                mFailed = true;
                api.quit();
            }
        }
        if (mFrameLimit && ++mFrames >= mFrameLimit)
            api.quit();
    }
}
