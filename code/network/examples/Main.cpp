#include "idk/network/Network.hpp"
#include <charconv>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <thread>

using namespace idk::net;

namespace
{
    constexpr uint64_t Manifest = 1; // Demo content: resource 1 is a moving object.
    volatile std::sig_atomic_t running = 1;
    void stop(int) { running = 0; }

    bool number(const char *text, uint32_t &value)
    {
        auto result = std::from_chars(text, text + std::strlen(text), value);
        return result.ec == std::errc{} && !*result.ptr && value > 0;
    }

    void simulate(Server &server)
    {
        for (size_t i = 0; i < MaxClients; ++i)
        {
            ClientId owner = server.clientAt(i);
            if (!owner) continue;
            bool exists = false;
            for (size_t j = 0; j < MaxEntities; ++j)
                if (const auto *entity = server.world().at(j); entity && entity->owner == owner) exists = true;
            if (!exists) server.world().spawn(owner, 1);
        }
        for (size_t i = 0; i < MaxEntities; ++i)
        {
            const auto *entity = server.world().at(i);
            if (!entity) continue;
            InputState input;
            server.input(entity->id, input);
            float length = std::sqrt(input.axes[0] * input.axes[0] + input.axes[1] * input.axes[1] +
                                     input.axes[2] * input.axes[2]);
            float scale = (3.0f / 30.0f) / (length > 1 ? length : 1);
            Transform transform = entity->transform;
            for (size_t axis = 0; axis < 3; ++axis) transform.position[axis] += input.axes[axis] * scale;
            server.world().setTransform(entity->id, transform);
        }
    }
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "local";
    bool local = !std::strcmp(mode, "local"), serverMode = !std::strcmp(mode, "server");
    bool clientMode = !std::strcmp(mode, "client");
    uint32_t port = 28080, ticks = 300;
    int index = clientMode ? 3 : 2;
    if ((!local && !serverMode && !clientMode) || (clientMode && argc < 3) || argc > index + 2 ||
        (argc > index && !number(argv[index], port)) || port > 65535 ||
        (argc > index + 1 && !number(argv[index + 1], ticks)))
    {
        std::fputs("Usage: idk_net_demo local [port] [ticks]\n"
                   "       idk_net_demo server [port] [ticks]\n"
                   "       idk_net_demo client hostname [port] [ticks]\n", stderr);
        return 2;
    }
    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);
    Server server;
    Client client;
    if ((local || serverMode) && !server.listen(uint16_t(port), Manifest, local ? "127.0.0.1" : nullptr))
    {
        std::fprintf(stderr, "Listen failed: %s\n", server.error());
        return 1;
    }
    if ((local || clientMode) && !client.connect(local ? "127.0.0.1" : argv[2], uint16_t(port), Manifest, nowSeconds()))
    {
        std::fprintf(stderr, "Connect failed: %s\n", client.error());
        return 1;
    }
    std::printf("%s on UDP port %u, %u ticks at 30 Hz\n", mode, port, ticks);
    double nextTick = nowSeconds();
    uint64_t tick = 0;
    while (running && tick < ticks)
    {
        double now = nowSeconds();
        if ((local || serverMode) && !server.poll(now)) return 1;
        if ((local || clientMode) && !client.poll(now))
        {
            std::fprintf(stderr, "Session ended: status=%d rejection=%d %s\n",
                         int(client.status()), int(client.rejection()), client.error());
            return 1;
        }
        if (now >= nextTick)
        {
            ++tick;
            nextTick += 1.0 / 30.0;
            // Avoid an unbounded catch-up loop after suspending the process.
            if (now - nextTick > 0.25) nextTick = now;
            if (local || clientMode)
                for (const auto &entity: std::span(client.snapshot().entities, client.snapshot().count))
                    if (entity.owner == client.id())
                    {
                        InputState input;
                        input.axes[0] = 1;
                        client.sendInput(entity.id, input);
                    }
            if (local || serverMode)
            {
                simulate(server);
                if (!server.publish(tick)) return 1;
            }
            if ((local || clientMode) && tick % 30 == 0)
                for (const auto &entity: std::span(client.snapshot().entities, client.snapshot().count))
                    std::printf("tick=%llu entity=%llu owner=%u resource=%u x=%.2f\n",
                                static_cast<unsigned long long>(client.snapshot().tick),
                                static_cast<unsigned long long>(entity.id), entity.owner, entity.resource,
                                entity.transform.position[0]);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    bool received = serverMode || client.snapshot().count > 0;
    client.disconnect();
    server.stop();
    return received ? 0 : 1;
}
