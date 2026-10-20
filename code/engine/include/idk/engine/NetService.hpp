#pragma once

#include "libidk/Service.hpp"
#include "idk/network/Network.hpp"

namespace idk
{
    // Borrows an already opened session. Session must outlive the service.
    // Simulation and snapshot publication remain in the game's fixed update.
    class NetService final: public Service
    {
    public:
        explicit NetService(net::Client &client): mClient(&client) {}
        explicit NetService(net::Server &server): mServer(&server) {}
        void onInit(EngineAPI &) override {}
        void onUpdate(EngineAPI &) override;
        void onShutdown(EngineAPI &) override;

    private:
        net::Client *mClient = nullptr;
        net::Server *mServer = nullptr;
    };
}
