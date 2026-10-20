#include "Protocol.hpp"
#include <chrono>

using namespace idk::net;

double idk::net::nowSeconds()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

const EntityState *Snapshot::find(EntityId id) const
{
    for (size_t i = 0; i < count; ++i)
        if (id && entities[i].id == id) return &entities[i];
    return nullptr;
}

EntityId World::spawn(ClientId owner, ResourceId resource, const Transform &transform)
{
    if (!resource || !protocol::valid(transform))
        return 0;
    for (size_t i = 0; i < MaxEntities; ++i)
    {
        // Exhausted generation slots are retired rather than reusing an old ID.
        if (!mEntities[i].id && mGeneration[i] != UINT32_MAX)
        {
            EntityId id = (uint64_t(++mGeneration[i]) << 32) | (i + 1);
            mEntities[i] = {id, owner, resource, transform};
            return id;
        }
    }
    return 0;
}

const EntityState *World::find(EntityId id) const
{
    if (!protocol::validId(id)) return nullptr;
    const auto &entity = mEntities[uint32_t(id) - 1];
    return entity.id == id ? &entity : nullptr;
}

const EntityState *World::at(size_t slot) const
{
    return slot < MaxEntities && mEntities[slot].id ? &mEntities[slot] : nullptr;
}

bool World::destroy(EntityId id)
{
    if (!find(id)) return false;
    mEntities[uint32_t(id) - 1] = {};
    return true;
}

bool World::setTransform(EntityId id, const Transform &transform)
{
    if (!find(id) || !protocol::valid(transform)) return false;
    mEntities[uint32_t(id) - 1].transform = transform;
    return true;
}

void World::clear()
{
    for (auto &entity: mEntities) entity = {};
}
