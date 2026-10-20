#include "Protocol.hpp"
#include <SDL3_net/SDL_net.h>
#include <cstdio>
#include <limits>
#include <unistd.h>

#define CHECK(value) do { if (!(value)) { \
    std::fprintf(stderr, "Check failed: %s (%d)\n", #value, __LINE__); return false; } } while (false)

using namespace idk;
using namespace idk::net;
using namespace idk::net::protocol;

namespace
{
    uint16_t nextPort()
    {
        static uint16_t port = uint16_t(30000 + (getpid() % 10000));
        return port++;
    }

    uint16_t listen(Server &server, uint64_t manifest = 42)
    {
        for (unsigned i = 0; i < 100; ++i)
        {
            uint16_t port = nextPort();
            if (server.listen(port, manifest)) return port;
        }
        return 0;
    }

    uint16_t listen(RemoteRxTxer &transport)
    {
        for (unsigned i = 0; i < 100; ++i)
        {
            uint16_t port = nextPort();
            if (transport.open(nullptr, 0, port, "127.0.0.1")) return port;
        }
        return 0;
    }

    RemoteReceive receive(RemoteRxTxer &transport, MessageRecvInfo *&message)
    {
        for (unsigned i = 0; i < 1000; ++i)
        {
            auto result = transport.receive(message);
            if (result != RemoteReceive::Empty) return result;
            SDL_Delay(1);
        }
        return RemoteReceive::Empty;
    }

    bool codecAndWorld()
    {
        uint8_t bytes[16]{};
        WireWriter writer(bytes, sizeof(bytes));
        writer.u32(0x12345678); writer.u64(0x0102030405060708); writer.f32(1);
        CHECK(writer.good() && writer.size() == 16);
        const uint8_t expected[]{0x12,0x34,0x56,0x78,1,2,3,4,5,6,7,8,0x3f,0x80,0,0};
        CHECK(!std::memcmp(bytes, expected, sizeof(bytes)));
        writer.bytes(bytes, SIZE_MAX);
        CHECK(!writer.good());
        WireReader reader(bytes, sizeof(bytes));
        CHECK(reader.u32() == 0x12345678 && reader.u64() == 0x0102030405060708 && reader.f32() == 1);
        CHECK(reader.done());
        reader.bytes(bytes, SIZE_MAX);
        CHECK(!reader.good());
        WireWriter nullWriter(nullptr, 1);
        nullWriter.u8(0);
        CHECK(!nullWriter.good());
        World world;
        CHECK(!world.spawn(0, 0));
        EntityId old = world.spawn(0, 1);
        CHECK(old && world.destroy(old) && !world.destroy(old));
        EntityId current = world.spawn(7, 2);
        CHECK(current != old && uint32_t(current) == uint32_t(old));
        CHECK(!world.find(old) && !world.setTransform(old, {}));
        Transform invalid;
        invalid.position[0] = std::numeric_limits<float>::quiet_NaN();
        CHECK(!world.setTransform(current, invalid));
        for (size_t i = 1; i < MaxEntities; ++i) CHECK(world.spawn(0, 1));
        CHECK(!world.spawn(0, 1));
        world.clear();
        CHECK(!world.find(current) && world.spawn(0, 1) != current);
        ResourceMap<int, 2> resources;
        CHECK(!resources.bind(0, 4));
        CHECK(resources.bind(1, 101) && resources.bind(2, 202));
        CHECK(!resources.bind(3, 303));
        CHECK(resources.bind(1, 404) && *resources.find(1) == 404);
        CHECK(!resources.find(0) && !resources.find(3));
        resources.clear();
        CHECK(!resources.find(1));
        return true;
    }

    bool transportChecks()
    {
        RemoteRxTxer server, a, b;
        uint16_t port = listen(server);
        CHECK(port && a.open("127.0.0.1", port) && b.open("127.0.0.1", port));
        RemoteRxTxer conflict;
        CHECK(!conflict.open(nullptr, 0, port, "127.0.0.1"));
        CHECK(!a.sendMsg(nullptr, 1) && !a.sendMsg("x", RemoteRxTx::MAX_PAYLOAD_SIZE + 1));
        CHECK(a.sendMsg("a", 2, "test"));
        MessageRecvInfo *message = nullptr;
        CHECK(receive(server, message) == RemoteReceive::Message && message->isType("test"));
        RemoteEndpoint first = server.sender();
        CHECK(first.port() && first.port() != port);
        CHECK(server.sendMsg("reply", 6));
        CHECK(!std::strcmp(static_cast<char *>(message->payload), "a")); // Send cannot overwrite receive.
        CHECK(receive(a, message) == RemoteReceive::Message && !std::strcmp(static_cast<char *>(message->payload), "reply"));
        CHECK(b.sendMsg("b", 2));
        CHECK(receive(server, message) == RemoteReceive::Message && !(server.sender() == first));
        CHECK(server.sendTo(first, "first", 6));
        CHECK(server.sendMsg("second", 7));
        CHECK(receive(a, message) == RemoteReceive::Message && !std::strcmp(static_cast<char *>(message->payload), "first"));
        CHECK(receive(b, message) == RemoteReceive::Message && !std::strcmp(static_cast<char *>(message->payload), "second"));

        NET_Address *address = NET_ResolveHostname("127.0.0.1");
        CHECK(address && NET_WaitUntilResolved(address, 1000) == NET_SUCCESS);
        NET_DatagramSocket *raw = NET_CreateDatagramSocket(nullptr, 0, 0);
        CHECK(raw);
        uint8_t bytes[1200]{};
        // Truncated, forged sizes, wrong version, trailing bytes, oversize.
        for (unsigned variant = 0; variant < 5; ++variant)
        {
            WireWriter out(bytes, sizeof(bytes));
            out.u64(variant == 2 ? 0 : RemoteRxTx::WIRE_VERSION);
            out.u64(32); out.u64(variant == 1 ? UINT64_MAX : 0); out.u64(0);
            int size = variant == 0 ? 8 : (variant == 3 ? 33 : (variant == 4 ? 1200 : 32));
            CHECK(NET_SendDatagram(raw, address, port, bytes, size));
            CHECK(receive(server, message) == RemoteReceive::Invalid && !message);
        }
        CHECK(a.sendMsg(nullptr, 0));
        CHECK(receive(server, message) == RemoteReceive::Message && message->header.payloadSize == 0);
        NET_DestroyDatagramSocket(raw);
        NET_UnrefAddress(address);
        // Endpoint references can outlive a receive, but release before the last transport closes.
        first.reset();
        return true;
    }

    void pump(Server &server, Client *clients, size_t count, double &now, unsigned steps = 20)
    {
        for (unsigned step = 0; step < steps; ++step)
        {
            now += 0.002;
            server.poll(now);
            for (size_t i = 0; i < count; ++i) clients[i].poll(now);
            SDL_Delay(1);
        }
    }

    bool sessions()
    {
        Server server;
        Client clients[2];
        double now = 100;
        uint16_t port = listen(server);
        CHECK(port && clients[0].connect("127.0.0.1", port, 42, now));
        CHECK(clients[1].connect("127.0.0.1", port, 42, now));
        pump(server, clients, 2, now);
        CHECK(clients[0].status() == ClientStatus::Connected && clients[1].status() == ClientStatus::Connected);
        CHECK(clients[0].id() != clients[1].id() && server.clientAt(0) && server.clientAt(1));
        EntityId a = server.world().spawn(clients[0].id(), 1);
        EntityId b = server.world().spawn(clients[1].id(), 2);
        CHECK(a && b && server.publish(1));
        pump(server, clients, 2, now);
        CHECK(clients[0].snapshot().count == 2 && clients[1].snapshot().find(a));
        InputState input;
        input.axes[0] = 1;
        CHECK(clients[0].sendInput(a, input));
        CHECK(!clients[0].sendInput(b, input));
        pump(server, clients, 2, now);
        InputState received;
        CHECK(server.input(a, received) && received.axes[0] == 1);
        CHECK(!server.input(b, received) && received.axes[0] == 0);
        input.axes[0] = 2;
        CHECK(!clients[0].sendInput(a, input));
        now += InputTimeoutSeconds;
        CHECK(server.poll(now) && !server.input(a, received));

        // A lost spawn/despawn/update is repaired by a later full snapshot.
        server.simulatePacketLoss(100);
        CHECK(server.world().destroy(a));
        EntityId replacement = server.world().spawn(clients[0].id(), 3);
        CHECK(replacement != a && !server.input(replacement, received));
        Transform transform;
        transform.position[0] = 12;
        CHECK(server.world().setTransform(b, transform) && server.publish(2));
        pump(server, clients, 2, now);
        CHECK(clients[0].snapshot().find(a) && !clients[0].snapshot().find(replacement));
        server.simulatePacketLoss(0);
        CHECK(server.publish(3));
        pump(server, clients, 2, now);
        CHECK(!clients[0].snapshot().find(a) && clients[0].snapshot().find(replacement));
        CHECK(clients[1].snapshot().find(b)->transform.position[0] == 12);
        CHECK(clients[0].previousSnapshot().tick == 1);

        ClientId oldId = clients[0].id();
        clients[0].disconnect();
        pump(server, clients, 2, now);
        CHECK(!server.world().find(replacement) && server.world().find(b));
        CHECK(clients[0].connect("127.0.0.1", port, 42, now));
        pump(server, clients, 2, now);
        CHECK(clients[0].id() && clients[0].id() != oldId && clients[0].snapshot().count == 0);
        CHECK(server.publish(4));
        pump(server, clients, 2, now);
        CHECK(clients[0].snapshot().count == 1);
        now += TimeoutSeconds + 1;
        CHECK(!clients[1].poll(now) && clients[1].status() == ClientStatus::TimedOut);
        CHECK(clients[1].snapshot().count == 0 && server.poll(now));
        CHECK(!server.world().find(b) && !server.clientAt(0) && !server.clientAt(1));
        return true;
    }

    bool admission()
    {
        Server server;
        Client clients[MaxClients + 1];
        double now = 100;
        uint16_t port = listen(server);
        CHECK(port && clients[0].connect("127.0.0.1", port, 99, now));
        pump(server, clients, 1, now);
        CHECK(clients[0].status() == ClientStatus::Rejected &&
              clients[0].rejection() == RejectReason::ManifestMismatch && !server.clientAt(0));
        for (size_t i = 0; i < MaxClients; ++i) CHECK(clients[i].connect("127.0.0.1", port, 42, now));
        pump(server, clients, MaxClients, now);
        for (size_t i = 0; i < MaxClients; ++i) CHECK(clients[i].status() == ClientStatus::Connected);
        CHECK(clients[MaxClients].connect("127.0.0.1", port, 42, now));
        pump(server, clients, MaxClients + 1, now);
        CHECK(clients[MaxClients].status() == ClientStatus::Rejected &&
              clients[MaxClients].rejection() == RejectReason::ServerFull);
        server.stop();
        for (size_t i = 0; i < MaxClients; ++i)
        {
            for (unsigned j = 0; j < 20 && clients[i].status() == ClientStatus::Connected; ++j)
            {
                clients[i].poll(now += .002);
                SDL_Delay(1);
            }
            CHECK(clients[i].status() == ClientStatus::Disconnected);
        }
        return true;
    }

    bool retryAndTimeout()
    {
        Server server;
        Client client;
        double now = 100;
        uint16_t port = listen(server);
        CHECK(port && client.connect("127.0.0.1", port, 42, now));
        server.simulatePacketLoss(100);
        pump(server, &client, 1, now);
        CHECK(client.status() == ClientStatus::Connecting);
        server.simulatePacketLoss(0);
        now += .3;
        pump(server, &client, 1, now);
        CHECK(client.status() == ClientStatus::Connected && server.clientAt(0));
        // Invalid/duplicate traffic cannot keep a lost connection alive.
        client.simulatePacketLoss(100);
        now += TimeoutSeconds + .1;
        CHECK(!client.poll(now) && client.status() == ClientStatus::TimedOut);
        CHECK(server.poll(now) && !server.clientAt(0));
        CHECK(client.connect("127.0.0.1", port, 42, now));
        server.simulatePacketLoss(100);
        pump(server, &client, 1, now);
        now += TimeoutSeconds + .1;
        CHECK(!client.poll(now) && client.status() == ClientStatus::TimedOut);
        return true;
    }

    bool sendPacket(RemoteRxTxer &transport, Header header, EntityId entity = 0, InputState input = {})
    {
        uint8_t bytes[128];
        WireWriter out(bytes, sizeof(bytes));
        write(out, header);
        if (header.kind == Hello) out.u64(42);
        if (header.kind == Input) { out.u64(entity); write(out, input); }
        return transport.sendMsg(bytes, out.size(), PayloadType);
    }

    bool hostileInput()
    {
        Server server;
        RemoteRxTxer raw;
        double now = 100;
        uint16_t port = listen(server);
        CHECK(port && raw.open("127.0.0.1", port));
        CHECK(sendPacket(raw, {Hello, 1234, 0, 0}));
        pump(server, nullptr, 0, now);
        MessageRecvInfo *message = nullptr;
        CHECK(receive(raw, message) == RemoteReceive::Message);
        WireReader in(message->payload, size_t(message->header.payloadSize));
        Header welcome = readHeader(in);
        ClientId id = in.u32();
        CHECK(in.done() && welcome.kind == Welcome && welcome.token && id && !server.clientAt(0));
        CHECK(sendPacket(raw, {Heartbeat, 1234, welcome.token, 1}));
        pump(server, nullptr, 0, now);
        CHECK(server.clientAt(0) == id);
        EntityId own = server.world().spawn(id, 1), other = server.world().spawn(0, 1);
        InputState input;
        input.axes[0] = .5f;
        CHECK(sendPacket(raw, {Input, 1234, welcome.token, 3}, own, input));
        input.axes[0] = -.5f;
        CHECK(sendPacket(raw, {Input, 1234, welcome.token, 2}, own, input)); // Reordering.
        CHECK(sendPacket(raw, {Input, 1234, welcome.token, 4}, other, input)); // Ownership.
        CHECK(sendPacket(raw, {Input, 1234, welcome.token + 1, 4}, own, input)); // Token.
        CHECK(sendPacket(raw, {Input, 1235, welcome.token, 4}, own, input)); // Session nonce.
        input.axes[0] = std::numeric_limits<float>::infinity();
        CHECK(sendPacket(raw, {Input, 1234, welcome.token, 4}, own, input));
        pump(server, nullptr, 0, now);
        CHECK(server.input(own, input) && input.axes[0] == .5f);
        CHECK(!server.input(other, input));
        CHECK(server.world().destroy(own));
        EntityId replacement = server.world().spawn(id, 1);
        input.axes[0] = 1;
        CHECK(sendPacket(raw, {Input, 1234, welcome.token, 5}, own, input));
        pump(server, nullptr, 0, now);
        CHECK(!server.input(replacement, input));
        return true;
    }

    bool snapshotValidation()
    {
        RemoteRxTxer server, rogue;
        Client client;
        uint16_t port = listen(server);
        double now = 100;
        CHECK(port && client.connect("127.0.0.1", port, 42, now) && client.poll(now));
        MessageRecvInfo *message = nullptr;
        CHECK(receive(server, message) == RemoteReceive::Message);
        WireReader hello(message->payload, size_t(message->header.payloadSize));
        Header h = readHeader(hello);
        RemoteEndpoint endpoint = server.sender();
        uint8_t bytes[RemoteRxTx::MAX_PAYLOAD_SIZE];
        WireWriter welcome(bytes, sizeof(bytes));
        write(welcome, Header{Welcome, h.nonce, 777, 0}); welcome.u32(1);
        CHECK(server.sendTo(endpoint, bytes, welcome.size(), PayloadType));
        for (unsigned i = 0; i < 20; ++i) { client.poll(now += .002); SDL_Delay(1); }
        CHECK(client.status() == ClientStatus::Connected && rogue.open(nullptr, 0));
        EntityState entity{(uint64_t(1) << 32) | 1, 1, 1, {}};
        for (unsigned variant = 0; variant < 9; ++variant)
        {
            WireWriter out(bytes, sizeof(bytes));
            write(out, Header{State, h.nonce, variant == 6 ? 778ULL : 777ULL, variant == 1 ? 1ULL : uint64_t(variant + 1)});
            out.u64(variant + 1); out.u16(variant == 3 ? uint16_t(MaxEntities + 1) : (variant == 4 ? 2 : 1));
            entity.transform.position[0] = variant == 5 ? std::numeric_limits<float>::quiet_NaN() : float(variant);
            write(out, entity);
            if (variant == 4) write(out, entity); // Duplicate slot.
            auto &sender = variant == 7 ? rogue : server;
            CHECK(sender.sendTo(endpoint, bytes, out.size() - (variant == 2 ? 1 : 0), PayloadType));
            for (unsigned i = 0; i < 10; ++i) { client.poll(now += .002); SDL_Delay(1); }
            CHECK(client.snapshot().count == 1);
            // Only first and last snapshots are valid. All rejects are atomic.
            CHECK(client.snapshot().entities[0].transform.position[0] == (variant == 8 ? 8 : 0));
        }
        CHECK(client.snapshot().revision == 9 && client.previousSnapshot().revision == 1);
        // A later empty snapshot destroys all replicas, even after a lost despawn.
        WireWriter empty(bytes, sizeof(bytes));
        write(empty, Header{State, h.nonce, 777, 10}); empty.u64(10); empty.u16(0);
        CHECK(server.sendTo(endpoint, bytes, empty.size(), PayloadType));
        for (unsigned i = 0; i < 10; ++i) { client.poll(now += .002); SDL_Delay(1); }
        CHECK(client.snapshot().count == 0 && client.snapshot().revision == 10);
        return true;
    }
}

int main()
{
    if (!codecAndWorld() || !transportChecks() || !sessions() || !admission() || !retryAndTimeout() ||
        !hostileInput() || !snapshotValidation())
        return 1;
    std::puts("Networking: wire validation, endpoints, sessions, ownership, snapshots, loss, timeout and capacity passed");
    return 0;
}
