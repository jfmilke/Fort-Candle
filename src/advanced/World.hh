#pragma once

#include <glow/fwd.hh>
#include "container/compact_pool.hh"
#include "transform.hh"
#include "types/AuxiliaryModel.hh"
#include "Constants.hh"

namespace gamedev
{
GLOW_SHARED(class, Instance);

// refers to an instance
struct InstanceHandle
{
    uint32_t _value = uint32_t(-1);

    bool is_valid() const { return _value != uint32_t(-1); }

    // Comparison
    bool operator==(const InstanceHandle& handle) const { return _value == handle._value; }
    bool operator<(const InstanceHandle& handle) const { return _value < handle._value; }
    bool operator<=(const InstanceHandle& handle) const { return _value <= handle._value; }
};

// a single "instance" in the world
// think about what state varies between your renderables
// maybe you want differing shaders? - make them part of your renderable
struct Instance
{
    // main mesh
    glow::SharedVertexArray vao;

    // main material (textures)
    glow::SharedTexture2D texAlbedo;
    glow::SharedTexture2D texNormal;
    glow::SharedTexture2D texARM; // Ambient Occlusion + Roughness + Metallic packed into R/G/B
    uint32_t albedoBias;          // packed RGBA color added to albedo

    // main transform
    transform xform;
    tg::aabb3 max_bounds;
    tg::dir3 facing;
    bool new_position;    // Indicates the spatial hash map to refresh this instance

    // Auxiliary Models (not the best approach, but keeps initial structure)
    // Note: When rendering, all auxiliary models will reference the base entities transform. No inter-auxiliary-reference possible atm.
    std::vector<AuxiliaryModel> auxModels;

    // animation info
    // for a more elaborate game, you might want separate places to store logic like this
    // ideally your render world is as separate from gameplay/logic as possible - but this is a quick way to do it
    enum class AnimationType
    {
        None = 0,
        Spin,
        SinusTravel,
        Bounce
    };

    AnimationType anim;
    float animSpeed;
    float animBaseY;

    Signature mSignature;
    InstanceHandle mHandle;

    bool destroy;
};

class World
{
public:
    [[nodiscard]] InstanceHandle createInstance(glow::SharedVertexArray const& vao,
                                                glow::SharedTexture2D const& albedo,
                                                glow::SharedTexture2D const& normal,
                                                glow::SharedTexture2D const& arm);

    Instance& getInstance(InstanceHandle handle) { return mCPoolInstances.get(handle._value); }

    transform& getInstanceTransform(InstanceHandle handle) { return mCPoolInstances.get(handle._value).xform; }

    void destroyInstance(InstanceHandle handle);

    void destroyAllInstances() { mCPoolInstances.release_all(); }

    // returns true if the given handle is valid and refers to a live instance
    bool isLiveHandle(InstanceHandle handle) const { return mCPoolInstances.is_alive(handle._value); }

    // access all live instances as a single contigous array
    uint32_t getNumLiveInstances() const { return mCPoolInstances.get_span().size(); }
    Instance const* getLiveInstances() const { return mCPoolInstances.get_span().data(); }

    // array view for the above
    cc::span<Instance const> getLiveInstancesSpan() const { return mCPoolInstances.get_span(); }
    cc::span<Instance> getLiveInstancesSpan() { return mCPoolInstances.get_span(); }

    uint32_t getMaxNumInstances() const { return uint32_t(mCPoolInstances.max_size()); }

    InstanceHandle getNthInstanceHandle(uint32_t i) const { return {mCPoolInstances.get_nth_handle(i)}; }
    Instance const& getNthInstance(uint32_t i) const { return getLiveInstancesSpan()[i]; }

    // ECS integration
    // ---------------
    void SetSignature(InstanceHandle handle, Signature signature);

    Signature GetSignature(InstanceHandle handle);

public:
    void initialize(uint32_t maxNumInstances);

    void destroy();

private:
    compact_pool<Instance> mCPoolInstances;
};

class InstanceHandleHash
{
public:
    // Use sum of lengths of first and last names
    // as hash function.
    size_t operator()(const InstanceHandle& handle) const { return std::hash<uint32_t>()(handle._value); }
};
}
