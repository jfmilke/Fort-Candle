#include "World.hh"
#include <assert.h>
#include <clean-core/assert.hh>

gamedev::InstanceHandle gamedev::World::createInstance(glow::SharedVertexArray const& vao,
                                                       glow::SharedTexture2D const& albedo,
                                                       glow::SharedTexture2D const& normal,
                                                       glow::SharedTexture2D const& arm)
{
    auto const res = mCPoolInstances.acquire();

    auto& newNode = mCPoolInstances.get(res);
    newNode.vao = vao;
    newNode.texAlbedo = albedo;
    newNode.texNormal = normal;
    newNode.texARM = arm;
    newNode.albedoBias = 0u;

    newNode.xform = {};
    newNode.anim = Instance::AnimationType::None;
    newNode.animSpeed = 1.f;
    newNode.animBaseY = 0.f;

    newNode.mHandle = {res};

    newNode.max_bounds = {tg::pos3::zero, tg::pos3::zero};
    newNode.facing = tg::dir3::pos_z;
    newNode.new_position = true;

    newNode.destroy = false;

    return {res};
}

void gamedev::World::destroyInstance(InstanceHandle handle)
{
    CC_ASSERT(handle.is_valid());

    mCPoolInstances.release(handle._value);
}

void gamedev::World::initialize(uint32_t maxNumInstances) { mCPoolInstances.initialize(maxNumInstances, cc::system_allocator); }

void gamedev::World::destroy() { mCPoolInstances.destroy(); }

void gamedev::World::SetSignature(InstanceHandle handle, Signature signature)
{
    assert(isLiveHandle(handle) && "Entity not valid.");

    getInstance(handle).mSignature = signature;
}

Signature gamedev::World::GetSignature(InstanceHandle handle)
{
    assert(isLiveHandle(handle) && "Entity not valid.");

    return getInstance(handle).mSignature;
}