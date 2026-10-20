#include "Protocol.hpp"
#include <cerrno>
#include <sys/random.h>

using namespace idk;
using namespace idk::net;
using namespace idk::net::protocol;

namespace
{
    bool randomToken(uint64_t &token)
    {
        token = 0;
        size_t offset = 0;
        while (offset < sizeof(token))
        {
            ssize_t count = getrandom(reinterpret_cast<uint8_t *>(&token) + offset, sizeof(token) - offset, 0);
            if (count < 0 && errno == EINTR) continue;
            if (count <= 0) return false;
            offset += size_t(count);
        }
        return token != 0;
    }

    bool messageType(const MessageRecvInfo &message)
    {
        return !std::memcmp(message.header.payloadType.ascii, PayloadType, 8);
    }
}

Server::~Server() { stop(); }

bool Server::listen(uint16_t port, uint64_t manifest, const char *bindHost)
{
    stop();
    if (!port || !manifest) return false;
    mManifest = manifest;
    return mTransport.open(nullptr, 0, port, bindHost);
}

void Server::remove(Peer &peer)
{
    if (peer.id)
        for (size_t i = 0; i < MaxEntities; ++i)
            if (const auto *entity = mWorld.at(i); entity && entity->owner == peer.id)
                mWorld.destroy(entity->id);
    peer = {};
}

void Server::stop()
{
    for (auto &peer: mPeers)
    {
        if (peer.id)
        {
            uint8_t bytes[25];
            WireWriter out(bytes, sizeof(bytes));
            write(out, Header{Goodbye, peer.nonce, peer.token, 0});
            mTransport.sendTo(peer.endpoint, bytes, out.size(), PayloadType);
        }
        remove(peer);
    }
    mTransport.close();
    mWorld.clear();
    for (auto &input: mInputs) input = {};
    mManifest = mRevision = 0;
    mNow = 0;
}

ClientId Server::clientAt(size_t slot) const
{
    return slot < MaxClients && mPeers[slot].confirmed ? mPeers[slot].id : 0;
}

bool Server::input(EntityId id, InputState &state) const
{
    state = {};
    const auto *entity = mWorld.find(id);
    if (!entity || !entity->owner) return false;
    const auto &record = mInputs[uint32_t(id) - 1];
    if (record.entity != id || mNow - record.received >= InputTimeoutSeconds) return false;
    state = record.state;
    return true;
}

bool Server::poll(double now)
{
    if (!std::isfinite(now) || now < mNow || !isOpen()) return false;
    mNow = now;
    for (auto &peer: mPeers)
        if (peer.id && now - peer.lastSeen >= TimeoutSeconds) remove(peer);
    for (size_t i = 0; i < PollBudget; ++i)
    {
        MessageRecvInfo *message = nullptr;
        auto result = mTransport.receive(message);
        if (result == RemoteReceive::Empty) break;
        if (result == RemoteReceive::Error) { stop(); return false; }
        if (result == RemoteReceive::Message && messageType(*message)) process(*message);
    }
    return true;
}

void Server::process(const MessageRecvInfo &message)
{
    WireReader in(message.payload, size_t(message.header.payloadSize));
    Header h = readHeader(in);
    if (!in.good() || !h.nonce) return;
    const auto &sender = mTransport.sender();
    Peer *peer = nullptr, *free = nullptr;
    for (auto &entry: mPeers)
    {
        if (entry.id && entry.endpoint == sender) peer = &entry;
        if (!entry.id && !free) free = &entry;
    }
    if (h.kind == Hello)
    {
        uint64_t manifest = in.u64();
        if (!in.done() || h.token || h.sequence) return;
        RejectReason reason = RejectReason::None;
        if (manifest != mManifest) reason = RejectReason::ManifestMismatch;
        else if ((!peer && !free) || !mNextClient) reason = RejectReason::ServerFull;
        uint8_t bytes[32];
        WireWriter out(bytes, sizeof(bytes));
        if (reason != RejectReason::None)
        {
            write(out, Header{Reject, h.nonce, 0, 0});
            out.u8(uint8_t(reason));
            mTransport.sendTo(sender, bytes, out.size(), PayloadType);
            return;
        }
        if (peer && peer->nonce != h.nonce) return;
        if (!peer)
        {
            uint64_t token = 0;
            if (!randomToken(token)) return;
            peer = free;
            peer->endpoint = sender;
            peer->id = mNextClient++;
            peer->nonce = h.nonce;
            peer->token = token;
            peer->lastSeen = mNow;
        }
        write(out, Header{Welcome, peer->nonce, peer->token, 0});
        out.u32(peer->id);
        mTransport.sendTo(peer->endpoint, bytes, out.size(), PayloadType);
        return;
    }
    if (!peer || h.nonce != peer->nonce || h.token != peer->token || h.sequence <= peer->sequence) return;
    if (h.kind == Input)
    {
        EntityId id = in.u64();
        InputState state = readInput(in);
        const auto *entity = mWorld.find(id);
        if (!in.done() || !valid(state) || !peer->confirmed || !entity || entity->owner != peer->id) return;
        mInputs[uint32_t(id) - 1] = {id, state, mNow};
    }
    else if (h.kind == Heartbeat)
    {
        if (!in.done()) return;
        uint8_t bytes[25];
        WireWriter out(bytes, sizeof(bytes));
        write(out, h);
        mTransport.sendTo(peer->endpoint, bytes, out.size(), PayloadType);
    }
    else if (h.kind == Goodbye)
    {
        if (in.done()) remove(*peer);
        return;
    }
    else return;
    peer->confirmed = true;
    peer->sequence = h.sequence;
    peer->lastSeen = mNow;
}

bool Server::publish(uint64_t simulationTick)
{
    if (!isOpen() || mRevision == UINT64_MAX) return false;
    ++mRevision;
    bool success = true;
    for (const auto &peer: mPeers)
    {
        if (!peer.confirmed) continue;
        uint8_t bytes[RemoteRxTx::MAX_PAYLOAD_SIZE];
        WireWriter out(bytes, sizeof(bytes));
        write(out, Header{State, peer.nonce, peer.token, mRevision});
        out.u64(simulationTick);
        uint16_t count = 0;
        for (size_t i = 0; i < MaxEntities; ++i) if (mWorld.at(i)) ++count;
        out.u16(count);
        for (size_t i = 0; i < MaxEntities; ++i)
            if (const auto *entity = mWorld.at(i)) write(out, *entity);
        if (!out.good() || !mTransport.sendTo(peer.endpoint, bytes, out.size(), PayloadType)) success = false;
    }
    return success;
}

Client::~Client() { disconnect(); }

void Client::fail(ClientStatus status)
{
    mTransport.close();
    mStatus = status;
    mId = 0;
    mToken = mNonce = mSequence = mAckSequence = 0;
    mSnapshot = {};
    mPrevious = {};
}

bool Client::connect(const char *host, uint16_t port, uint64_t manifest, double now)
{
    disconnect();
    if (!host || !port || !manifest || !std::isfinite(now) || now < 0) return false;
    if (!randomToken(mNonce) || !mTransport.open(host, port))
    {
        fail(ClientStatus::TransportError);
        return false;
    }
    mStatus = ClientStatus::Connecting;
    mManifest = manifest;
    mNow = mLastSeen = mNextSend = now;
    return true;
}

void Client::disconnect()
{
    if (mStatus == ClientStatus::Connected) sendControl(Goodbye);
    fail(ClientStatus::Disconnected);
    mRejection = RejectReason::None;
}

bool Client::sendControl(uint8_t kind)
{
    if (mSequence == UINT64_MAX) return false;
    uint8_t bytes[33];
    WireWriter out(bytes, sizeof(bytes));
    write(out, Header{kind, mNonce, mToken, kind == Hello ? 0 : ++mSequence});
    if (kind == Hello) out.u64(mManifest);
    return mTransport.sendMsg(bytes, out.size(), PayloadType);
}

bool Client::sendInput(EntityId id, const InputState &state)
{
    const auto *entity = mSnapshot.find(id);
    if (mStatus != ClientStatus::Connected || !entity || entity->owner != mId ||
        !valid(state) || mSequence == UINT64_MAX) return false;
    uint8_t bytes[49];
    WireWriter out(bytes, sizeof(bytes));
    write(out, Header{Input, mNonce, mToken, ++mSequence});
    out.u64(id);
    write(out, state);
    return mTransport.sendMsg(bytes, out.size(), PayloadType);
}

bool Client::poll(double now)
{
    if (!std::isfinite(now) || now < mNow ||
        (mStatus != ClientStatus::Connecting && mStatus != ClientStatus::Connected)) return false;
    mNow = now;
    if (now - mLastSeen >= TimeoutSeconds)
    {
        fail(ClientStatus::TimedOut);
        return false;
    }
    for (size_t i = 0; i < PollBudget; ++i)
    {
        MessageRecvInfo *message = nullptr;
        auto result = mTransport.receive(message);
        if (result == RemoteReceive::Empty) break;
        if (result == RemoteReceive::Error) { fail(ClientStatus::TransportError); return false; }
        if (result == RemoteReceive::Message && messageType(*message) && mTransport.sender() == mTransport.remote())
            process(*message);
        if (mStatus != ClientStatus::Connecting && mStatus != ClientStatus::Connected) return false;
    }
    if (now >= mNextSend)
    {
        if (!sendControl(mStatus == ClientStatus::Connecting ? Hello : Heartbeat))
        {
            fail(ClientStatus::TransportError);
            return false;
        }
        mNextSend = now + 0.25;
    }
    return true;
}

void Client::process(const MessageRecvInfo &message)
{
    WireReader in(message.payload, size_t(message.header.payloadSize));
    Header h = readHeader(in);
    if (!in.good() || h.nonce != mNonce) return;
    if (mStatus == ClientStatus::Connecting)
    {
        if (h.kind == Reject && !h.token && !h.sequence)
        {
            auto reason = RejectReason(in.u8());
            if (!in.done() || (reason != RejectReason::ManifestMismatch && reason != RejectReason::ServerFull)) return;
            mRejection = reason;
            fail(ClientStatus::Rejected);
        }
        else if (h.kind == Welcome && h.token && !h.sequence)
        {
            ClientId id = in.u32();
            if (!in.done() || !id) return;
            mId = id;
            mToken = h.token;
            mStatus = ClientStatus::Connected;
            mLastSeen = mNextSend = mNow;
        }
        return;
    }
    if (h.token != mToken) return;
    if (h.kind == Goodbye && in.done())
    {
        fail(ClientStatus::Disconnected);
    }
    else if (h.kind == Heartbeat && in.done() && h.sequence > mAckSequence && h.sequence <= mSequence)
    {
        mAckSequence = h.sequence;
        mLastSeen = mNow;
    }
    else if (h.kind == State && h.sequence > mSnapshot.revision)
    {
        Snapshot next;
        next.revision = h.sequence;
        next.tick = in.u64();
        next.count = in.u16();
        if (!in.good() || next.count > MaxEntities) return;
        bool occupied[MaxEntities]{};
        for (size_t i = 0; i < next.count; ++i)
        {
            auto &entity = next.entities[i];
            entity = readEntity(in);
            if (!in.good() || !validId(entity.id) || !entity.resource || !valid(entity.transform)) return;
            size_t slot = uint32_t(entity.id) - 1;
            if (occupied[slot]) return;
            occupied[slot] = true;
        }
        if (!in.done()) return;
        mPrevious = mSnapshot;
        mSnapshot = next;
        mLastSeen = mNow;
    }
}
