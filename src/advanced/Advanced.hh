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
 * The demo functions (`createDemoInstances`, `drawWorld`, `drawPolyWorld`, `drawOutlineIfSelected`,
 * `updateAnimationsInWorld`) are leftovers and not called by the game.
 */
struct AdvancedFeatures
{
    gamedev::StatisticsState* mStats = nullptr; ///< Frame statistics shown in the debug UI.
    float* mStatFrametime = nullptr;            ///< Stat "frametime": time per frame in milliseconds.
    float* mStatNumUpdates = nullptr;           ///< Stat "num_updates": `update()` ticks per frame.
    float* mStatNumDraws = nullptr;             ///< Stat "num_drawcalls": draw calls per frame.

    gamedev::World mWorld;                      ///< Storage of all instances, wrapped by the ECS.
    std::shared_ptr<gamedev::EngineECS> mECS;   ///< Used to keep the ECS consistent when the UI destroys instances.
    gamedev::InstanceHandle mInstanceGenerated; ///< Demo instance (unused).

    gamedev::InstanceHandle mSelectedInstance; ///< Instance picked with the mouse; outlined and editable in the editor.
    gamedev::EditorState mEditorState;         ///< ImGuizmo state for manipulating the selection.

    /// Creates the statistics and the instance pool (2048 instances, same as `MAX_INSTANCES`).
    void initialize()
    {
        // stat setup - track various metrics as your game runs
        mStats = gamedev::initializeStatistics(100, 256);
        mStatFrametime = gamedev::getStat(mStats, "frametime", gamedev::StatType::timing);     // time spent per frame in milliseconds
        mStatNumUpdates = gamedev::getStat(mStats, "num_updates", gamedev::StatType::counter); // amount of update() ticks per frame
        mStatNumDraws = gamedev::getStat(mStats, "num_drawcalls", gamedev::StatType::counter); // amount of drawcalls per frame

        // initialize world
        mWorld.initialize(2048);
    }

    /// Sets the ECS that is notified when instances are destroyed through this object.
    void setECS(std::shared_ptr<gamedev::EngineECS> ecs);

    /// Frees the statistics and all instances, and clears all components and systems.
    void destroy();

    /// Demo: creates a grid of animated spheres and cubes plus one instance of `vaoGenerated`.
    void createDemoInstances(glow::SharedVertexArray const& vaoSphere,
                             glow::SharedVertexArray const& vaoCube,
                             glow::SharedVertexArray const& vaoGenerated,
                             glow::SharedTexture2D const& texAlbedo,
                             glow::SharedTexture2D const& texNormal,
                             glow::SharedTexture2D const& texARM);

    /// Demo: draws every instance with all three textures using the bound `shader`.
    void drawWorld(glow::UsedProgram& shader);

    /// Demo: draws every instance with only the albedo texture using the bound `shader`.
    void drawPolyWorld(glow::UsedProgram& shader);

    /// Demo: clears `tgtOutline` and draws the selected instance into `fbOutline` with `shaderOutline`.
    void drawOutlineIfSelected(glow::SharedTextureRectangle const& tgtOutline,
                               glow::SharedFramebuffer const& fbOutline,
                               glow::SharedProgram const& shaderOutline,
                               tg::mat4 const& proj,
                               tg::mat4 const& view);

    /**
     * Draws the "Objects" window (edit or destroy the selection, destroy all instances) and the statistics window.
     * Sets `outFlyToSelection`/`outCreateNewInstance` when the corresponding button was pressed.
     */
    void runImgui(bool& outFlyToSelection, bool& outCreateNewInstance);

    /// Demo: advances the `Instance::anim` animations by `dt` seconds; `currentTime` is the total time in seconds.
    void updateAnimationsInWorld(float dt, float currentTime);

    /// Adds `draws` to this frame's draw call statistic.
    void updateStatDraws(float draws);
};
}
