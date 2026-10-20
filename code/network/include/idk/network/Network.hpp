#pragma once

#include "libidk/message/RemoteRxTx.hpp"
#include <span>

namespace idk::net
{
    inline constexpr size_t MaxClients = 8;
    inline constexpr size_t MaxEntities = 16;
    inline constexpr double TimeoutSeconds = 5.0;
    inline constexpr double InputTimeoutSeconds = 0.25;
    using ClientId = uint32_t;   // Zero means server-owned.
    using ResourceId = uint32_t; // Stable content-manifest ID, never a local handle.
    using EntityId = uint64_t;   // High 32 bits: generation; low 32 bits: slot + 1.

    struct Transform
    {
        float position[3]{};
        float rotation[4]{0, 0, 0, 1};
    };

    struct EntityState
    {
        EntityId id = 0;
        ClientId owner = 0;
        ResourceId resource = 0;
        Transform transform;
    };

    struct InputState
    {
        float axes[3]{}; // Absolute intent in [-1, 1], not position or elapsed time.
        uint32_t buttons = 0; // Held states; unreliable, not one-shot events.
    };

    struct Snapshot
    {
        uint64_t revision = 0, tick = 0;
        size_t count = 0;
        EntityState entities[MaxEntities]{};
        const EntityState *find(EntityId) const;
    };

    class World: public idk::Immobile
    {
    public:
        EntityId spawn(ClientId owner, ResourceId resource, const Transform & = {});
        bool destroy(EntityId);
        bool setTransform(EntityId, const Transform &);
        const EntityState *find(EntityId) const;
        const EntityState *at(size_t slot) const;
        void clear();

    private:
        EntityState mEntities[MaxEntities]{};
        uint32_t mGeneration[MaxEntities]{};
    };

    // Resolve replicated resource IDs to process-local render/audio/etc. handles.
    // Loading and releasing resources remain the application's responsibility.
    template <typename Handle, size_t Capacity>
    class ResourceMap
    {
        static_assert(Capacity > 0);
        struct Entry { ResourceId id = 0; Handle handle{}; };
        Entry mEntries[Capacity]{};
    public:
        bool bind(ResourceId id, const Handle &handle)
        {
            if (!id)
                return false;
            for (auto &entry: mEntries)
                if (entry.id == id) { entry.handle = handle; return true; }
            for (auto &entry: mEntries)
                if (!entry.id) { entry = {id, handle}; return true; }
            return false;
        }
        const Handle *find(ResourceId id) const
        {
            for (const auto &entry: mEntries)
                if (id && entry.id == id) return &entry.handle;
            return nullptr;
        }
        void clear() { for (auto &entry: mEntries) entry = {}; }
    };

    // Monotonic seconds, usable by both a headless loop and engine services.
    double nowSeconds();

    class Server: public idk::Immobile
    {
    public:
        ~Server();
        // Default bind is loopback. Pass nullptr to listen on all interfaces.
        bool listen(uint16_t port, uint64_t manifest, const char *bindHost = "127.0.0.1");
        void stop();
        bool poll(double now);
        bool publish(uint64_t simulationTick);
        bool isOpen() const { return mTransport.isOpen(); }
        const char *error() const { return mTransport.error(); }
        World &world() { return mWorld; }
        const World &world() const { return mWorld; }
        ClientId clientAt(size_t slot) const;
        // Sample latest input once per simulation tick. Expired input is neutral.
        bool input(EntityId, InputState &) const;
        void simulatePacketLoss(int percent) { mTransport.simulatePacketLoss(percent); }

    private:
        struct Peer
        {
            RemoteEndpoint endpoint;
            ClientId id = 0;
            uint64_t nonce = 0, token = 0, sequence = 0;
            double lastSeen = 0;
            bool confirmed = false;
        };
        struct InputRecord
        {
            EntityId entity = 0;
            InputState state;
            double received = 0;
        };
        RemoteRxTxer mTransport;
        Peer mPeers[MaxClients]{};
        World mWorld;
        InputRecord mInputs[MaxEntities]{};
        uint64_t mManifest = 0, mRevision = 0;
        ClientId mNextClient = 1;
        double mNow = 0;
        void remove(Peer &);
        void process(const MessageRecvInfo &);
    };

    enum class ClientStatus { Disconnected, Connecting, Connected, Rejected, TimedOut, TransportError };
    enum class RejectReason: uint8_t { None, ManifestMismatch, ServerFull };

    class Client: public idk::Immobile
    {
    public:
        ~Client();
        bool connect(const char *host, uint16_t port, uint64_t manifest, double now);
        void disconnect();
        bool poll(double now);
        bool sendInput(EntityId, const InputState &);
        ClientStatus status() const { return mStatus; }
        RejectReason rejection() const { return mRejection; }
        ClientId id() const { return mId; }
        const char *error() const { return mTransport.error(); }
        const Snapshot &snapshot() const { return mSnapshot; }
        const Snapshot &previousSnapshot() const { return mPrevious; }
        void simulatePacketLoss(int percent) { mTransport.simulatePacketLoss(percent); }

    private:
        RemoteRxTxer mTransport;
        ClientStatus mStatus = ClientStatus::Disconnected;
        RejectReason mRejection = RejectReason::None;
        ClientId mId = 0;
        uint64_t mManifest = 0, mNonce = 0, mToken = 0, mSequence = 0, mAckSequence = 0;
        double mNow = 0, mLastSeen = 0, mNextSend = 0;
        Snapshot mSnapshot, mPrevious;
        bool sendControl(uint8_t kind);
        void fail(ClientStatus);
        void process(const MessageRecvInfo &);
    };
}
