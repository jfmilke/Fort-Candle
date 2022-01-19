#pragma once

#include <glow/fwd.hh>

#include "Editor.hh"
#include "Utility.hh"
#include "World.hh"

namespace gamedev
{
class EngineECS;

struct AdvancedFeatures
{
    gamedev::StatisticsState* mStats = nullptr;
    float* mStatFrametime = nullptr;
    float* mStatNumUpdates = nullptr;
    float* mStatNumDraws = nullptr;

    gamedev::World mWorld;
    std::shared_ptr<gamedev::EngineECS> mECS;
    gamedev::InstanceHandle mInstanceGenerated;

    gamedev::InstanceHandle mSelectedInstance;
    gamedev::EditorState mEditorState;

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

    void setECS(std::shared_ptr<gamedev::EngineECS> ecs);

    void destroy();

    void createDemoInstances(glow::SharedVertexArray const& vaoSphere,
                             glow::SharedVertexArray const& vaoCube,
                             glow::SharedVertexArray const& vaoGenerated,
                             glow::SharedTexture2D const& texAlbedo,
                             glow::SharedTexture2D const& texNormal,
                             glow::SharedTexture2D const& texARM);

    void drawWorld(glow::UsedProgram& shader);

    void drawPolyWorld(glow::UsedProgram& shader);

    void drawOutlineIfSelected(glow::SharedTextureRectangle const& tgtOutline,
                               glow::SharedFramebuffer const& fbOutline,
                               glow::SharedProgram const& shaderOutline,
                               tg::mat4 const& proj,
                               tg::mat4 const& view);

    void runImgui(bool& outFlyToSelection, bool& outCreateNewInstance);

    void updateAnimationsInWorld(float dt, float currentTime);

    void updateStatDraws(float draws);
};
}