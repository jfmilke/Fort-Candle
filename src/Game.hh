#pragma once

#include <glow/fwd.hh>

#include <glow-extras/glfw/GlfwApp.hh>

#include "FreeflyCam.hh"
#include "Mesh3D.hh"
#include "GameObjects.hh" // custom game assets
#include "advanced/Advanced.hh"

#include "Terrain.hh"

// ECS
#include "ecs/Engine.hh"
#include "systems/CollisionSystem.hh"
#include "systems/AdvancedRenderSystem.hh"
#include "systems/PhysicsSystem.hh"
#include "systems/EnvironmentSystem.hh"
#include "systems/LightSystem.hh"
#include "systems/ParticleSystem.hh"
#include "systems/GamelogicSystem.hh"
#include "systems/AnimationSystem.hh"
#include "systems/CleanupSystem.hh"
#include "systems/AudioSystem.hh"

// Factories
#include "factories/UnitFactory.hh"
#include "factories/StructureFactory.hh"
#include "factories/NatureFactory.hh"
#include "factories/ItemFactory.hh"

enum GameState
{
    PLAY,
    PAUSED,
    MENU
};

class Game : public glow::glfw::GlfwApp
{
private:

    GameState state = MENU;

    // Camera
    gamedev::FreeflyCam mCamera;
    bool mEnableTopDownCamera = false;

    // Features
    gamedev::AdvancedFeatures mAdvancedFeatures;

    gamedev::AudioSystem mAudioSystem;

    //UI
    ImFont* font1;
    ImFont* font2;

    float menuWidth = 280;

private:
    bool THEBUTTONHASBEENPRESSEDOMG = false;

    // Debug
    bool mShowNormals = false;
    bool mShowWireframe = false;
    bool mCullFaces = true;
    bool mHoldSpawns = false;

    // Shadows
    bool mEnablePointShadows = true;
    bool mEnableDirShadows = true;

    // Lighting
    bool mEnableAmbLight = true;
    bool mEnableDirLight = true;
    bool mEnablePointLight = true;

    // Gamelogic
    bool mEnableFogOfWar = false;
    bool mPlayedTrack = false;

    // FFX
    bool mEnablePostProcess = true;
    bool mEnableTonemap = true;
    bool mEnableSRGBCurve = true;
    bool mEnableSepia = true;
    bool mEnableVignette = true;
    bool mEnableDesaturate = true;
    bool mEnableBloom = true;
    bool mEnableDistFadeoff = true;

    // Particles
    bool mEnableParticleSpawn = true;

    // Editor
	  bool mEditorMode = false;

    // Camera
    bool mCaptureMouseOnMouselook = true;
    bool mEnableFreeFlyCam = false;

private:
    // Debug
    float mTime_PhysicsSys = 0.0;
    float mTime_CollisionSys = 0.0;
    float mTime_EnvironmentSys = 0.0;
    float mTime_LightSys = 0.0;
    float mTime_ParticleSys = 0.0;
    float mTime_GamelogicSys = 0.0;
    float mTime_AnimationSys = 0.0;
    float mTime_CleanSys = 0.0;
    float mTime_AllSys = 0.0;

    // Shadows
    int mPointShadowResolution = 1024;
    int mDirShadowResolution = 1024;

    // Lighting
    tg::vec3 mAmbientColor = {0.75, 1.0, 0.9};
    float mAmbientIntensity = 0.5;

    float mSunTheta = 70;
    float mSunPhi = -64;
    tg::vec3 mSunColor = {0.985, 1.0, 0.876};
    float mSunIntensity = 1.0;

    tg::pos3 mSPLPosition = {0.0, 0.0, 0.0};
    tg::vec3 mSPLColor = {1.0, 1.0, 1.0};
    float mSPLIntensity = 1.0;
    float mSPLRadius = 50.0;

    // FFX
    float mDesaturationStrength = 0.3;
    float mSepiaStrength = 0.2;
    float mVignetteIntensity = 0.5;
    float mVignetteRadius = 0.75;
    float mVignetteSoftness = 0.4;
    tg::vec3 mVignetteColor = tg::vec3::zero;
    float mDistFadeoffIntensity = 4.0;
    tg::vec3 mDistFadeoffColor = tg::vec3(9, 34, 80) / 255.0;

    // Audio
    float mMasterVolume = 100.0;
    float mEffectVolume = 100.0;
    float mAmbientVolume = 100.0;
    float mMusicVolume = 60.0;
    float mUIVolume = 100.0;

    // Misc
    float* f;
    bool mFullscreen = false;

    bool mNewGame = true;
    bool mGameLost = false;
    bool mGameWon = false;

    // Editor
    std::string mSelectedObject = "";
    gamedev::Factory* mSelectedFac;
    std::vector<std::string> mNatureObjects;
    std::vector<std::string> mStructureObjects;
    std::vector<std::string> mItemObjects;
    std::vector<std::string> mUnitObjects;
    gamedev::InstanceHandle mCreatedLast = {std::uint32_t(-1)};

    char mFnUnits[256];
    char mFnStructs[256];
    char mFnNature[256];
    char mFnItems[256];

private:
    // Custom Low-Poly Assets
    gamedev::SharedGameObjects mGameAssets;

    // Terrain
    std::shared_ptr<gamedev::Terrain> mTerrain;

    // ECS
    std::shared_ptr<gamedev::EngineECS> mECS;
    std::shared_ptr<gamedev::CollisionSystem> mCollisionSys;
    std::shared_ptr<gamedev::RenderSystem> mRenderSys;
    std::shared_ptr<gamedev::PhysicsSystem> mPhysicsSys;
    std::shared_ptr<gamedev::EnvironmentSystem> mEnvironmentSys;
    std::shared_ptr<gamedev::LightSystem> mLightSys;
    std::shared_ptr<gamedev::ParticleSystem> mParticleSys;
    std::shared_ptr<gamedev::GamelogicSystem> mGamelogicSys;
    std::shared_ptr<gamedev::AnimationSystem> mAnimationSys;
    std::shared_ptr<gamedev::CleanupSystem> mCleanSys;
    std::shared_ptr<gamedev::AudioSystem> mAudioSys;

    // Factories
    std::shared_ptr<gamedev::UnitFactory> mUnitFac;
    std::shared_ptr<gamedev::StructureFactory> mStructFac;
    std::shared_ptr<gamedev::NatureFactory> mNatureFac;
    std::shared_ptr<gamedev::ItemFactory> mItemFac;

    // Globals
    std::string texPath = "../data/textures/"; 
    std::string meshPath = "../data/meshes/"; 
    std::string shaderPath = "../data/shaders/";
    std::string audioPath = "../data/audio/";

    int frameControl = 0;
    unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};


    // ctor
public:
    Game();

    // events
public:
    //
    // virtual methods
    //
    void init() override;                       // called once after OpenGL is set up
    void onFrameStart() override;               // called at the start of a frame
    void update(float elapsedSeconds) override; // called in 60 Hz fixed timestep
    void render(float elapsedSeconds) override; // called once per frame (variable timestep)
    void onGui() override;                      // called once per frame to set up UI
    void onClose() override;

    void onResize(int w, int h) override; // called when window is resized
    bool onMouseScroll(double sx, double sy) override;

    //
    // your own methods
    //
    void updateCamera(float elapsedSeconds);
    void fillSpawnMenu(std::vector<std::string> nature, std::vector<std::string> items, std::vector<std::string> units, std::vector<std::string> structures);

    void natureSpawnMenu();
    void itemSpawnMenu();
    void structureSpawnMenu();
    void unitSpawnMenu();

    void gameUI();
    void menuUI();
    void applyCustomImguiTheme();

    bool spawnNature(std::string id);
    bool spawnItem(std::string id);
    bool spawnStructure(std::string id);
    bool spawnUnit(std::string id);

    void spawn(tg::pos3 position);
    void spawnRandomly(tg::pos3 position);

    // read the picking buffer at the specified pixel coordinate
    //gamedev::InstanceHandle readPickingBuffer(int x, int y);

    void flyToSelectedinstance();

    void toggleEditorMode() { mEditorMode ? mEditorMode = false : mEditorMode = true; }
    void togglePause() { state == PLAY ? state = PAUSED : state = PLAY; }
    void toggleMenu();

    void newGame();
};
