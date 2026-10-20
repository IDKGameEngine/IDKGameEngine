#pragma once

#include "libidk/Service.hpp"
#include "idk/Event.hpp"

namespace idk
{
    class EventManager: public idk::Service
    {
    public:
        template <typename EventType>
        using CallbackType = void (*)(EngineAPI&, const EventType&);

        using GenericCallback = void (*)(EngineAPI&, const void*);
        using EngineCallback  = CallbackType<EngineEvent>;
        using WindowCallback  = CallbackType<WindowEvent>;
        using InputCallback   = CallbackType<InputEvent>;

        virtual void onInit(EngineAPI&) final;
        virtual void onShutdown(EngineAPI&) final;
        virtual void onUpdate(EngineAPI&) final;

        bool addGenericCallback(GenericCallback);
        bool addEngineCallback(EngineCallback);
        bool addWindowCallback(WindowCallback);
        bool addInputCallback(InputCallback);

    private:
        template <typename FuncType>
        using CallbackList = InplaceList<FuncType, 16>;

        CallbackList<GenericCallback> mGenericCallbacks;
        CallbackList<EngineCallback>  mEngineCallbacks;
        CallbackList<WindowCallback>  mWindowCallbacks;
        CallbackList<InputCallback>   mInputCallbacks;

        template <typename FuncType>
        bool addCallback(CallbackList<FuncType> &callbacks, FuncType func)
        {
            if (callbacks.full())
            {
                VLOG_WARN("[EventManager::addCallback] Cannot add callback: at capacity");
                return false;
            }
            callbacks.push(func);
            return true;
        }

        template <typename EventType>
        void dispatch(EngineAPI &api, CallbackList<CallbackType<EventType>> &callbacks, const EventType& event)
        {
            for (auto func: callbacks)
            {
                func(api, event);
            }
        }

    };
}

