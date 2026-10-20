#include "idk/engine/NetService.hpp"

void idk::NetService::onUpdate(EngineAPI &)
{
    double now = net::nowSeconds();
    if (mClient) mClient->poll(now);
    if (mServer) mServer->poll(now);
}

void idk::NetService::onShutdown(EngineAPI &)
{
    if (mClient) mClient->disconnect();
    if (mServer) mServer->stop();
}
