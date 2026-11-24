#pragma once

#include <compare>
#include <functional>
#include <glow/fwd.hh>
#include "Constants.hh"
#include "container/compact_pool.hh"
#include "transform.hh"
#include "types/AuxiliaryModel.hh"

namespace gamedev
{
GLOW_SHARED(struct, Instance);

/**
 * Stable handle of an `Instance` in the `World`; this is the entity ID of the ECS.
 *
 * The value encodes the pool slot and a generation, so a handle to a destroyed instance never matches a newer instance in the same slot.
 * A default-constructed handle is invalid.
 */
struct InstanceHandle
{
    uint32_t _value = uint32_t(-1); ///< Pool handle; `uint32_t(-1)` means invalid.

    /// The invalid handle (same as `InstanceHandle{}`).
    static constexpr InstanceHandle Invalid() { return {}; }

    /// True unless this is the invalid handle. Does not check whether the instance is still alive.
    bool is_valid() const { return _value != uint32_t(-1); }

    /// Compares and orders by handle value (for `std::set`/`std::map`).
    auto operator<=>(const InstanceHandle&) const = default;
};

/**
 * An entity of the game world: render data, transform and ECS bookkeeping in one struct.
 *
 * Instances live in the `World`'s compact pool, which moves the last instance into a freed slot whenever an instance is destroyed.
 * References and pointers to an `Instance` are therefore only safe until the next destruction.
 * Keep the `InstanceHandle` instead.
 */
struct Instance
{
    glow::SharedVertexArray vao; ///< Main mesh.

    glow::SharedTexture2D texAlbedo; ///< Base color texture (usually the shared palette texture).
    glow::SharedTexture2D texNormal; ///< Normal map; not read by the game's shaders.
    glow::SharedTexture2D texARM;    ///< Ambient occlusion + roughness + metallic packed into R/G/B; not read by the game's shaders.
    uint32_t albedoBias;             ///< Packed RGBA tint (RGB in the high 24 bits); shaders blend albedo towards RGB by A (0: none).

    transform xform; ///< World transform.

    /// Bounds relative to `xform.translation` (scaled mesh AABB, not rotated); used for the spatial hash map and spawn checks.
    tg::aabb3 max_bounds;

    tg::dir3 facing; ///< Direction the unrotated mesh faces (`+z` by default); rotate with `xform` for the heading.

    /**
     * Position changed; `PhysicsSystem` then sends `EventType::NewPosition` and updates the spatial hash map.
     *
     * TODO: `World::createInstance` sets it and nothing resets it, so it is always true.
     */
    bool new_position;

    /**
     * Additional meshes drawn with this instance; each `AuxiliaryModel::xform` is relative to `xform` (not to each other).
     *
     * Unused: nothing adds models, and `RenderSystem::RenderAuxiliary` would log a message for each one.
     */
    std::vector<AuxiliaryModel> auxModels;

    /**
     * Procedural demo animation of the course framework.
     *
     * Unused: only `AdvancedFeatures::updateAnimationsInWorld` applies it, and the game never calls that.
     * The editor's "Objects" window can still change these fields.
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
 * Use `EngineECS` instead of the `World` directly, so that components, systems and the spatial hash map stay consistent.
 */
class World
{
public:
    /**
     * Creates an instance with identity transform, no tint, no animation and an empty signature.
     *
     * Asserts if the pool is full. Use `EngineECS::CreateInstance` instead, which wraps this.
     *
     * @param vao Main mesh.
     * @param albedo Base color texture.
     * @param normal Normal map.
     * @param arm Ambient occlusion + roughness + metallic texture.
     * @return Handle of the new instance.
     */
    [[nodiscard]] InstanceHandle createInstance(glow::SharedVertexArray const& vao,
                                                glow::SharedTexture2D const& albedo,
                                                glow::SharedTexture2D const& normal,
                                                glow::SharedTexture2D const& arm);

    /// Returns a live instance. The reference is invalidated by the next destruction.
    Instance& getInstance(InstanceHandle handle) { return mCPoolInstances.get(handle._value); }

    /// Returns a live instance's transform. The reference is invalidated by the next destruction.
    transform& getInstanceTransform(InstanceHandle handle) { return mCPoolInstances.get(handle._value).xform; }

    /**
     * Frees an instance; the last instance moves into its slot.
     *
     * Does not touch components, systems or the spatial hash map (see `EngineECS::DestroyInstance`).
     *
     * @param handle Valid handle of a live instance (asserts if invalid).
     */
    void destroyInstance(InstanceHandle handle);

    /// Frees all instances. Does not touch components or systems.
    void destroyAllInstances() { mCPoolInstances.release_all(); }

    /// True if `handle` is valid and refers to a live instance.
    bool isLiveHandle(InstanceHandle handle) const { return mCPoolInstances.is_alive(handle._value); }

    /// Returned by `SlotIndex` for invalid or dead handles.
    static constexpr uint32_t kNoSlot = uint32_t(-1);

    /**
     * Stable storage slot of a live instance, in [0, `getMaxNumInstances()`).
     *
     * Unlike the packed index, the slot does not change when other instances are destroyed,
     * so the ECS uses it to index its per-instance arrays (see `ComponentArray`).
     *
     * @param handle Any handle; dead and invalid handles are allowed.
     * @return The slot, or `kNoSlot` if `handle` does not refer to a live instance.
     */
    uint32_t SlotIndex(InstanceHandle handle) const
    {
        return isLiveHandle(handle) ? uint32_t(mCPoolInstances.get_slot_index(handle._value)) : kNoSlot;
    }

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
    /**
     * Allocates the pool. Must be called once before creating instances.
     *
     * @param maxNumInstances Capacity; at most `MAX_INSTANCES`, the size of the component arrays (`AdvancedFeatures::initialize` passes exactly that).
     */
    void initialize(uint32_t maxNumInstances);

    /// Destroys all instances and frees the pool.
    void destroy();

private:
    compact_pool<Instance> mCPoolInstances; ///< Instance storage.
};

}

/// Lets `InstanceHandle` be a key of `std::unordered_map`/`std::unordered_set` without a custom hasher.
template <>
struct std::hash<gamedev::InstanceHandle>
{
    size_t operator()(gamedev::InstanceHandle handle) const noexcept { return std::hash<uint32_t>()(handle._value); }
};
