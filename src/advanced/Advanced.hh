#pragma once

#include <glow/fwd.hh>

#include "Editor.hh"
#include "Utility.hh"
#include "World.hh"

namespace gamedev
{
class EngineECS;

/**
 * Course framework helpers that the game still uses.
 *
 * Owned by `Game`. Holds:
 *  - the instance `World`,
 *  - the frame statistics,
 *  - the selected instance and the editor gizmo state.
 *
 * The demo functions are leftovers of the course framework and unused:
 * `createDemoInstances`, `drawWorld`, `drawPolyWorld`, `drawOutlineIfSelected` and `updateAnimationsInWorld`.
 */
struct AdvancedFeatures
{
    gamedev::StatisticsState* mStats = nullptr; ///< Frame statistics shown in the debug UI.
    float* mStatFrametime = nullptr;            ///< Stat "frametime": time per frame in milliseconds.
    float* mStatNumUpdates = nullptr;           ///< Stat "num_updates": `update()` ticks per frame.
    float* mStatNumDraws = nullptr;             ///< Stat "num_drawcalls": draw calls per frame.

    gamedev::World mWorld;                      ///< Storage of all instances, wrapped by the ECS.
    gamedev::EngineECS* mECS = nullptr;         ///< Notified when instances are destroyed through this object (UI, `destroy`); not owned.
    gamedev::InstanceHandle mInstanceGenerated; ///< Demo instance. Unused.

    gamedev::InstanceHandle mSelectedInstance; ///< Instance picked with the mouse; outlined and editable in the editor.
    gamedev::EditorState mEditorState;         ///< ImGuizmo state for manipulating the selection.

    /**
     * Creates the statistics and the instance pool.
     *
     * Room for 100 stats with a history of 256 frames each, and for `MAX_INSTANCES` instances.
     */
    void initialize()
    {
        // stat setup - track various metrics as your game runs
        mStats = gamedev::initializeStatistics(100, 256);
        mStatFrametime = gamedev::getStat(mStats, "frametime", gamedev::StatType::timing);     // time spent per frame in milliseconds
        mStatNumUpdates = gamedev::getStat(mStats, "num_updates", gamedev::StatType::counter); // amount of update() ticks per frame
        mStatNumDraws = gamedev::getStat(mStats, "num_drawcalls", gamedev::StatType::counter); // amount of drawcalls per frame

        // initialize world
        mWorld.initialize(MAX_INSTANCES);
    }

    /// Sets the ECS that is notified when instances are destroyed through this object.
    void setECS(gamedev::EngineECS* ecs);

    /// Frees the statistics and all instances, and clears all components, systems and the spatial hash map.
    void destroy();

    /**
     * Demo: creates a grid of animated spheres and cubes plus one travelling instance of `vaoGenerated`. Unused.
     *
     * The grid instances get spin or bounce animations and color biases from a fixed palette.
     * Selects the first instance.
     *
     * @param vaoSphere Mesh of the grid instances at even x and every third z.
     * @param vaoCube Mesh of the other grid instances.
     * @param vaoGenerated Mesh of the extra instance (`mInstanceGenerated`).
     * @param texAlbedo Albedo texture of all instances.
     * @param texNormal Normal map of all instances.
     * @param texARM AO/roughness/metallic texture of all instances.
     */
    void createDemoInstances(glow::SharedVertexArray const& vaoSphere,
                             glow::SharedVertexArray const& vaoCube,
                             glow::SharedVertexArray const& vaoGenerated,
                             glow::SharedTexture2D const& texAlbedo,
                             glow::SharedTexture2D const& texNormal,
                             glow::SharedTexture2D const& texARM);

    /**
     * Demo: draws every instance with all three textures and adds the draw calls to the statistics. Unused.
     *
     * @param shader Bound program; receives `uTexAlbedo`, `uTexNormal`, `uTexARM`, `uModel`, `uPickingID` and `uAlbedoBiasPacked` per instance.
     */
    void drawWorld(glow::UsedProgram& shader);

    /**
     * Demo: like `drawWorld`, but only binds the albedo texture. Unused.
     *
     * @param shader Bound program; receives `uTexAlbedo`, `uModel`, `uPickingID` and `uAlbedoBiasPacked` per instance.
     */
    void drawPolyWorld(glow::UsedProgram& shader);

    /**
     * Demo: clears the outline target and draws the selected instance into it. Unused.
     *
     * Uses the depth test `GL_EQUAL`, so `fbOutline` needs the depth buffer of the already rendered scene.
     *
     * @param tgtOutline Color target of `fbOutline`; cleared to transparent black.
     * @param fbOutline Framebuffer to draw the selection into.
     * @param shaderOutline Program used for the selection; receives `uProj`, `uView` and `uModel`.
     * @param proj Camera projection matrix.
     * @param view Camera view matrix.
     */
    void drawOutlineIfSelected(glow::SharedTextureRectangle const& tgtOutline,
                               glow::SharedFramebuffer const& fbOutline,
                               glow::SharedProgram const& shaderOutline,
                               tg::mat4 const& proj,
                               tg::mat4 const& view);

    /**
     * Draws the editor's "Objects" window and the statistics window.
     *
     * The "Objects" window edits the selection's color bias and demo animation, destroys the selection, or destroys all instances.
     * Instances are destroyed immediately and removed from the ECS.
     *
     * TODO: `Game::onGui` ignores both output flags, so "Focus on Selection" and "Spawn Instance" do nothing
     * (the **F** key still flies to the selection).
     *
     * @param[out] outFlyToSelection Set to true when "Focus on Selection" was pressed; left unchanged otherwise.
     * @param[out] outCreateNewInstance Set to true when "Spawn Instance" was pressed; left unchanged otherwise.
     */
    void runImgui(bool& outFlyToSelection, bool& outCreateNewInstance);

    /**
     * Demo: applies the `Instance::anim` animations of all instances. Unused.
     *
     * @param dt Time step in seconds (spin and travel speed).
     * @param currentTime Total time in seconds (bounce and travel phase).
     */
    void updateAnimationsInWorld(float dt, float currentTime);

    /// Adds `draws` to this frame's draw call statistic.
    void updateStatDraws(float draws);
};
}
