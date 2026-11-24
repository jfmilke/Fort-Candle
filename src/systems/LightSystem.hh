#pragma once

#include <glow/fwd.hh>
#include <glow/objects/UniformBuffer.hh>
#include "advanced/World.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"
#include "types/Light.hh"

namespace gamedev
{
/**
 * Collects the point lights for the renderer.
 *
 * Entities: all instances with a `PointLightEmitter`.
 *
 * Every update it rebuilds:
 *  - the list of regular point lights (free lights plus emitter lights, moved with their instance and flickered),
 *  - the one shadow-casting point light.
 *
 * `Game::render()` passes both to the `RenderSystem`.
 */
class LightSystem : public System
{
public:
    /// Stores the ECS.
    void Init(EngineECS& ecs);

    /**
     * Adds a light that is not bound to an instance. Unused.
     *
     * @param pl Light in world space; copied.
     * @return ID for `RemoveFreeLight`.
     */
    int AddFreeLight(PointLight& pl);

    /**
     * Removes a free light by ID. Unused.
     *
     * The IDs of the other free lights stay valid.
     *
     * @param light_id ID from `AddFreeLight`; unknown IDs are ignored.
     */
    void RemoveFreeLight(int light_id);

    /**
     * Rebuilds the light lists (see `BuildLights`).
     *
     * @param dt Unused.
     */
    void Update(float dt);

    /// Regular (non-shadowing) point lights in world space, as built by the last `Update`.
    const std::vector<PointLight>& GetLights();

    /// The shadow-casting point light in world space, as built by the last `Update` (kept if no emitter casts shadows).
    const PointLight& GetShadowingPointlight();

    /// Number of regular point lights.
    int GetLightCount();

private:
    /**
     * Fills `mLights` and `mShadowPL`: transforms emitter lights to world space and applies flicker.
     *
     * @param dt Unused: the flicker changes once per update.
     */
    void BuildLights(float dt);

private:
    EngineECS* mECS = nullptr;           ///< ECS (not owned).
    std::vector<PointLight> mFreeLights; ///< Lights not bound to an instance.
    std::vector<PointLight> mLights;     ///< Regular lights of the current update.
    PointLight mShadowPL;                ///< Shadow-casting light of the current update.

private:
    int mFreeLightIndex = 0;                        ///< Next free light ID.
    std::unordered_map<int, int> mExternToInternID; ///< Free light ID -> index in `mFreeLights`.
    std::unordered_map<int, int> mInternToExternID; ///< Index in `mFreeLights` -> free light ID.
};
}
