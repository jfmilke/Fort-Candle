#pragma once

#include <glow/fwd.hh>
#include "container/compact_pool.hh"
#include "transform.hh"
#include "types/AuxiliaryModel.hh"
#include "Constants.hh"

namespace gamedev
{
GLOW_SHARED(struct, Instance);

/// Stable handle of an `Instance` in the `World`; this is the entity ID of the ECS.
struct InstanceHandle
{
    uint32_t _value = uint32_t(-1); ///< Pool handle; `uint32_t(-1)` means invalid.

    /// True unless this is the invalid handle. Does not check whether the instance is still alive.
    bool is_valid() const { return _value != uint32_t(-1); }

    /// Equal if both refer to the same pool slot handle.
    bool operator==(const InstanceHandle& handle) const { return _value == handle._value; }

    /// Orders by handle value (for `std::set`/`std::map`).
    bool operator<(const InstanceHandle& handle) const { return _value < handle._value; }

    /// Orders by handle value.
    bool operator<=(const InstanceHandle& handle) const { return _value <= handle._value; }
};

/**
 * An entity of the game world: render data, transform and ECS bookkeeping in one struct.
 *
 * Instances live in the `World`'s compact pool, which moves the last instance into a freed slot whenever an instance
 * is destroyed. References and pointers to an `Instance` (including `Component::instance`) are therefore only safe
 * until the next destruction. Keep the `InstanceHandle` instead.
 */
struct Instance
{
    glow::SharedVertexArray vao; ///< Main mesh.

    glow::SharedTexture2D texAlbedo; ///< Base color texture (usually the shared palette texture).
    glow::SharedTexture2D texNormal; ///< Normal map.
    glow::SharedTexture2D texARM;    ///< Ambient occlusion + roughness + metallic packed into R/G/B.
    uint32_t albedoBias;             ///< Packed RGBA tint (RGB in the high 24 bits); shaders blend albedo towards RGB by A.

    transform xform;      ///< World transform.
    tg::aabb3 max_bounds; ///< Local-space bounds used for the spatial hash map and spawn checks.
    tg::dir3 facing;      ///< Direction the unrotated mesh faces (`+z` by default); rotate with `xform` for the heading.
    bool new_position;    ///< Moved since creation; `PhysicsSystem` then publishes the position (TODO: never reset).

    /**
     * Additional meshes drawn with this instance; each `AuxiliaryModel::xform` is relative to `xform` (not to each
     * other). Unused by the game: `RenderSystem::RenderAuxiliary` logs a message for every model it draws.
     */
    std::vector<AuxiliaryModel> auxModels;

    /**
     * Procedural demo animation of the course framework. Only `AdvancedFeatures::updateAnimationsInWorld` applies it,
     * and the game never calls that; the editor's "Objects" window can still change these fields.
     */
    enum class AnimationType
    {
        None = 0,    ///< No animation.
        Spin,        ///< Rotate about the y axis.
        SinusTravel, ///< Move along a circle on the ground plane.
        Bounce       ///< Move up and down around `animBaseY`.
    };

    AnimationType anim; ///< Active procedural animation.
    float animSpeed;    ///< Speed factor of `anim`.
    float animBaseY;    ///< Rest height for `AnimationType::Bounce`.

    Signature mSignature;   ///< Component types this instance has.
    InstanceHandle mHandle; ///< Own handle.

    bool destroy; ///< Marked for destruction; `CleanupSystem` destroys it at the end of the update.
};

/**
 * Storage of all instances (entities) in a fixed-size compact pool.
 *
 * Course framework code. Owned by `AdvancedFeatures` and wrapped by `EngineECS`.
 *
 * Use `EngineECS` instead of the `World` directly, so that components, systems and the spatial hash map stay
 * consistent.
 */
class World
{
public:
    /// Creates an instance with the given mesh and textures, identity transform, no animation and empty signature.
    [[nodiscard]] InstanceHandle createInstance(glow::SharedVertexArray const& vao,
                                                glow::SharedTexture2D const& albedo,
                                                glow::SharedTexture2D const& normal,
                                                glow::SharedTexture2D const& arm);

    /// Returns a live instance. The reference is invalidated by the next destruction.
    Instance& getInstance(InstanceHandle handle) { return mCPoolInstances.get(handle._value); }

    /// Returns a live instance's transform. The reference is invalidated by the next destruction.
    transform& getInstanceTransform(InstanceHandle handle) { return mCPoolInstances.get(handle._value).xform; }

    /// Frees the instance; the last instance moves into its slot. Does not touch components or systems.
    void destroyInstance(InstanceHandle handle);

    /// Frees all instances. Does not touch components or systems.
    void destroyAllInstances() { mCPoolInstances.release_all(); }

    /// True if `handle` is valid and refers to a live instance.
    bool isLiveHandle(InstanceHandle handle) const { return mCPoolInstances.is_alive(handle._value); }

    /// Number of live instances; they are stored as one contiguous array.
    uint32_t getNumLiveInstances() const { return mCPoolInstances.get_span().size(); }

    /// Pointer to the first of `getNumLiveInstances()` live instances.
    Instance const* getLiveInstances() const { return mCPoolInstances.get_span().data(); }

    /// All live instances as a span.
    cc::span<Instance const> getLiveInstancesSpan() const { return mCPoolInstances.get_span(); }

    /// All live instances as a mutable span.
    cc::span<Instance> getLiveInstancesSpan() { return mCPoolInstances.get_span(); }

    /// Capacity given to `initialize`.
    uint32_t getMaxNumInstances() const { return uint32_t(mCPoolInstances.max_size()); }

    /// Handle of the `i`-th live instance (index into the packed array).
    InstanceHandle getNthInstanceHandle(uint32_t i) const { return {mCPoolInstances.get_nth_handle(i)}; }

    /// The `i`-th live instance (index into the packed array).
    Instance const& getNthInstance(uint32_t i) const { return getLiveInstancesSpan()[i]; }

    // ECS integration
    // ---------------

    /// Stores the component signature of a live instance (asserts otherwise).
    void SetSignature(InstanceHandle handle, Signature signature);

    /// Returns the component signature of a live instance (asserts otherwise).
    Signature GetSignature(InstanceHandle handle);

public:
    /// Allocates the pool for `maxNumInstances` instances. Must be called before creating instances.
    void initialize(uint32_t maxNumInstances);

    /// Destroys all instances and frees the pool.
    void destroy();

private:
    compact_pool<Instance> mCPoolInstances; ///< Instance storage.
};

/// Hash functor for `InstanceHandle` keys in unordered containers.
class InstanceHandleHash
{
public:
    /// Hashes the handle value.
    size_t operator()(const InstanceHandle& handle) const { return std::hash<uint32_t>()(handle._value); }
};
}
