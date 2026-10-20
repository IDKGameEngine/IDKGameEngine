#pragma once

#include "idk/network/Network.hpp"
#include "libidk/message/Wire.hpp"
#include <cmath>

namespace idk::net::protocol
{
    inline constexpr const char *PayloadType = "IDKNET1";
    inline constexpr size_t PollBudget = 64;
    enum Kind: uint8_t { Hello = 1, Welcome, Reject, Heartbeat, Input, State, Goodbye };
    struct Header { uint8_t kind; uint64_t nonce, token, sequence; };

    inline void write(WireWriter &out, const Header &h)
    {
        out.u8(h.kind); out.u64(h.nonce); out.u64(h.token); out.u64(h.sequence);
    }
    inline Header readHeader(WireReader &in)
    {
        Header h{};
        h.kind = in.u8(); h.nonce = in.u64(); h.token = in.u64(); h.sequence = in.u64();
        return h;
    }
    inline bool valid(const Transform &t)
    {
        for (float v: t.position) if (!std::isfinite(v)) return false;
        for (float v: t.rotation) if (!std::isfinite(v)) return false;
        return true;
    }
    inline bool valid(const InputState &state)
    {
        for (float v: state.axes) if (!std::isfinite(v) || v < -1 || v > 1) return false;
        return true;
    }
    inline bool validId(EntityId id)
    {
        return uint32_t(id) > 0 && uint32_t(id) <= MaxEntities && (id >> 32) != 0;
    }
    inline void write(WireWriter &out, const EntityState &e)
    {
        out.u64(e.id); out.u32(e.owner); out.u32(e.resource);
        for (float v: e.transform.position) out.f32(v);
        for (float v: e.transform.rotation) out.f32(v);
    }
    inline EntityState readEntity(WireReader &in)
    {
        EntityState e;
        e.id = in.u64(); e.owner = in.u32(); e.resource = in.u32();
        for (float &v: e.transform.position) v = in.f32();
        for (float &v: e.transform.rotation) v = in.f32();
        return e;
    }
    inline void write(WireWriter &out, const InputState &state)
    {
        for (float v: state.axes) out.f32(v);
        out.u32(state.buttons);
    }
    inline InputState readInput(WireReader &in)
    {
        InputState state;
        for (float &v: state.axes) v = in.f32();
        state.buttons = in.u32();
        return state;
    }
    static_assert(25 + 8 + 2 + 44 * MaxEntities <= RemoteRxTx::MAX_PAYLOAD_SIZE);
}
