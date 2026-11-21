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

/// Top-level game state, switched by the main menu, Escape and P.
enum GameState
{
    PLAY,   ///< Running.
    PAUSED, ///< Simulation stopped, rendering continues.
    MENU    ///< Main menu shown; the camera looks at the camp from the side.
};

/**
 * The application.
 *
 * Owns the ECS, all systems and factories, the terrain, the camera and the settings, and drives them from the
 * glow-extras `GlfwApp` main loop:
 *
 *  - `init()` wires everything up (component and system registration, asset scan, preset setup, start layout).
 *  - `update()` runs the simulation systems at a fixed 60 Hz (only while playing, except audio, particles and
 *    lights).
 *  - `render()` handles mouse/keyboard input, passes the per-frame state to the `RenderSystem` and renders.
 *  - `onGui()` builds the menus, the in-game HUD and, in editor mode (key E), the debug, editor and spawn windows.
 */
class Game : public glow::glfw::GlfwApp
{
private:

    GameState state = MENU; ///< Current game state.

    // Camera
    gamedev::FreeflyCam mCamera;       ///< Smoothed game/editor camera.
    bool mEnableTopDownCamera = false; ///< Space toggles a steep top-down view.

    // Features
    gamedev::AdvancedFeatures mAdvancedFeatures; ///< Instance world, statistics, selection and gizmo state.

    gamedev::AudioSystem mAudioSystem; ///< Unused (the ECS-registered `mAudioSys` is used).

    //UI
    ImFont* font1; ///< HUD font (Source Sans 3).
    ImFont* font2; ///< Title font of the main menu (Sheila Crayon).

    float menuWidth = 280; ///< Width of the side panel and menu buttons in pixels.

private:
    bool THEBUTTONHASBEENPRESSEDOMG = false; ///< Shows the "Crazy Cheat" button.

    // Debug
    bool mShowNormals = false;   ///< Render normals.
    bool mShowWireframe = false; ///< Render wireframe.
    bool mCullFaces = true;      ///< Back-face culling.
    bool mHoldSpawns = false;    ///< Suppress the daily enemy waves and reinforcements.

    // Shadows
    bool mEnablePointShadows = true; ///< Point light shadows.
    bool mEnableDirShadows = true;   ///< Sun shadows.

    // Lighting
    bool mEnableAmbLight = true;   ///< Ambient light.
    bool mEnableDirLight = true;   ///< Sun.
    bool mEnablePointLight = true; ///< Point lights.

    // Gamelogic
    bool mEnableFogOfWar = false; ///< Fog of war.
    bool mPlayedTrack = false;    ///< The defeat sound was played.

    // FFX
    bool mEnablePostProcess = true; ///< Unused (not passed to the renderer).
    bool mEnableTonemap = true;     ///< Tonemapping.
    bool mEnableSRGBCurve = true;   ///< Gamma correction.
    bool mEnableSepia = true;       ///< Sepia.
    bool mEnableVignette = true;    ///< Vignette.
    bool mEnableDesaturate = true;  ///< Desaturation.
    bool mEnableBloom = true;       ///< Bloom.
    bool mEnableDistFadeoff = true; ///< Distance fade.

    // Particles
    bool mEnableParticleSpawn = true; ///< Simulate and draw particles.

    // Editor
    bool mEditorMode = false; ///< Editor mode (key E): debug/editor windows, spawning with the right mouse button.

    // Camera
    bool mCaptureMouseOnMouselook = true; ///< Hide and capture the cursor while looking around in free-fly mode.
    bool mEnableFreeFlyCam = false;       ///< Free-fly camera instead of the strategy camera.

private:
    // Debug: system update times of the last tick in milliseconds
    float mTime_PhysicsSys = 0.0;     ///< `PhysicsSystem`.
    float mTime_CollisionSys = 0.0;   ///< `CollisionSystem`.
    float mTime_EnvironmentSys = 0.0; ///< `EnvironmentSystem`.
    float mTime_LightSys = 0.0;       ///< `LightSystem`.
    float mTime_ParticleSys = 0.0;    ///< `ParticleSystem`.
    float mTime_GamelogicSys = 0.0;   ///< `GamelogicSystem`.
    float mTime_AnimationSys = 0.0;   ///< `AnimationSystem`.
    float mTime_CleanSys = 0.0;       ///< `CleanupSystem`.
    float mTime_AllSys = 0.0;         ///< All simulation systems together.

    // Shadows
    int mPointShadowResolution = 1024; ///< Edge length of the point shadow cube map faces.
    int mDirShadowResolution = 1024;   ///< Edge length of the sun shadow map.

    // Lighting
    tg::vec3 mAmbientColor = {0.75, 1.0, 0.9}; ///< Ambient light color.
    float mAmbientIntensity = 0.5;             ///< Ambient light intensity.

    float mSunTheta = 70;                     ///< Sun polar angle in degrees.
    float mSunPhi = -64;                      ///< Sun azimuth in degrees.
    tg::vec3 mSunColor = {0.985, 1.0, 0.876}; ///< Sun color.
    float mSunIntensity = 1.0;                ///< Sun intensity.

    tg::pos3 mSPLPosition = {0.0, 0.0, 0.0}; ///< Unused (editable in the debug UI only).
    tg::vec3 mSPLColor = {1.0, 1.0, 1.0};    ///< Unused (editable in the debug UI only).
    float mSPLIntensity = 1.0;               ///< Unused (editable in the debug UI only).
    float mSPLRadius = 50.0;                 ///< Unused (editable in the debug UI only).

    // FFX
    float mDesaturationStrength = 0.3;                        ///< Unused: the desaturation comes from the `GamelogicSystem`.
    float mSepiaStrength = 0.2;                               ///< Sepia strength.
    float mVignetteIntensity = 0.5;                           ///< Vignette strength.
    float mVignetteRadius = 0.75;                             ///< Unused: the vignette radius comes from the `GamelogicSystem`.
    float mVignetteSoftness = 0.4;                            ///< Vignette softness.
    tg::vec3 mVignetteColor = tg::vec3::zero;                 ///< Vignette color.
    float mDistFadeoffIntensity = 4.0;                        ///< Distance fade strength.
    tg::vec3 mDistFadeoffColor = tg::vec3(9, 34, 80) / 255.0; ///< Distance fade color.

    // Audio (volumes in percent)
    float mMasterVolume = 100.0;  ///< Master volume.
    float mEffectVolume = 100.0;  ///< Effect volume.
    float mAmbientVolume = 100.0; ///< Ambient volume.
    float mMusicVolume = 60.0;    ///< Music volume.
    float mUIVolume = 100.0;      ///< Interface volume.

    // Misc
    float* f;                 ///< Unused.
    bool mFullscreen = false; ///< Requested fullscreen state (F11); applied in `render()`.

    bool mNewGame = true;   ///< Show the intro window (and "New Game" instead of "Continue").
    bool mGameLost = false; ///< Defeat: the firewood ran out.
    bool mGameWon = false;  ///< Victory: all supply route upgrades were built.

    // Editor
    std::string mSelectedObject = "";                           ///< Preset to spawn with the right mouse button.
    gamedev::Factory* mSelectedFac;                             ///< Factory of `mSelectedObject`.
    std::vector<std::string> mNatureObjects;                    ///< Nature presets for the spawn menu.
    std::vector<std::string> mStructureObjects;                 ///< Structure presets for the spawn menu.
    std::vector<std::string> mItemObjects;                      ///< Item presets for the spawn menu.
    std::vector<std::string> mUnitObjects;                      ///< Unit presets for the spawn menu.
    gamedev::InstanceHandle mCreatedLast = {std::uint32_t(-1)}; ///< Last spawned instance; new spawns copy its rotation.

    char mFnUnits[256];   ///< File name (without extension) for saving spawned units.
    char mFnStructs[256]; ///< File name (without extension) for saving spawned structures.
    char mFnNature[256];  ///< File name (without extension) for saving spawned nature.
    char mFnItems[256];   ///< File name (without extension) for saving spawned items.

private:
    // Custom Low-Poly Assets
    gamedev::SharedGameObjects mGameAssets; ///< Asset manager.

    // Terrain
    std::shared_ptr<gamedev::Terrain> mTerrain; ///< Ground, water and height map.

    // ECS
    std::shared_ptr<gamedev::EngineECS> mECS;                    ///< The ECS.
    std::shared_ptr<gamedev::CollisionSystem> mCollisionSys;     ///< See `CollisionSystem`.
    std::shared_ptr<gamedev::RenderSystem> mRenderSys;           ///< See `RenderSystem`.
    std::shared_ptr<gamedev::PhysicsSystem> mPhysicsSys;         ///< See `PhysicsSystem`.
    std::shared_ptr<gamedev::EnvironmentSystem> mEnvironmentSys; ///< See `EnvironmentSystem` (not ECS-registered).
    std::shared_ptr<gamedev::LightSystem> mLightSys;             ///< See `LightSystem`.
    std::shared_ptr<gamedev::ParticleSystem> mParticleSys;       ///< See `ParticleSystem`.
    std::shared_ptr<gamedev::GamelogicSystem> mGamelogicSys;     ///< See `GamelogicSystem`.
    std::shared_ptr<gamedev::AnimationSystem> mAnimationSys;     ///< See `AnimationSystem`.
    std::shared_ptr<gamedev::CleanupSystem> mCleanSys;           ///< See `CleanupSystem`.
    std::shared_ptr<gamedev::AudioSystem> mAudioSys;             ///< See `AudioSystem`.

    // Factories
    std::shared_ptr<gamedev::UnitFactory> mUnitFac;        ///< Units.
    std::shared_ptr<gamedev::StructureFactory> mStructFac; ///< Structures.
    std::shared_ptr<gamedev::NatureFactory> mNatureFac;    ///< Nature.
    std::shared_ptr<gamedev::ItemFactory> mItemFac;        ///< Items.

    // Globals (unused; the paths are hard-coded where they are used)
    std::string texPath = "../data/textures/";   ///< Texture folder.
    std::string meshPath = "../data/meshes/";    ///< Mesh folder.
    std::string shaderPath = "../data/shaders/"; ///< Shader folder.
    std::string audioPath = "../data/audio/";    ///< Audio folder.

    int frameControl = 0;                                                                             ///< Counts simulation ticks (unused).
    unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2}; ///< Unused.


    // ctor
public:
    /// Creates the app with the ImGui integration of `GlfwApp`.
    Game();

    // events
public:
    //
    // virtual methods
    //
    /**
     * Called once after OpenGL is set up: creates and connects all systems, factories and assets and loads the start
     * layout (`nature.fac`, `startStructs.fac`, `items.fac`).
     */
    void init() override;

    /// Called at the start of each frame (course-framework hook in glow-extras): starts a new statistics frame.
    void onFrameStart() override;

    /**
     * Called at a fixed 60 Hz: updates audio, particles and lights, and while playing all simulation systems, the daily
     * enemy waves and reinforcements, and the deferred destruction (`CleanupSystem`, last).
     */
    void update(float elapsedSeconds) override;

    /**
     * Called once per frame (variable timestep): camera, mouse/keyboard input (selection, orders, editor spawning, key
     * shortcuts), renderer state, and rendering.
     */
    void render(float elapsedSeconds) override;

    /// Called once per frame to build the UI: main menu, HUD and dialogs, or the editor windows.
    void onGui() override;

    /// Releases the statistics and all instances before the window closes.
    void onClose() override;

    /// Called when the window is resized: resizes the render targets.
    void onResize(int w, int h) override;

    /// Zooms the strategy camera (height kept between 1 and 25). Always consumes the event.
    bool onMouseScroll(double sx, double sy) override;

    //
    // your own methods
    //
    /// Moves the camera from keyboard/mouse input (free-fly or strategy camera) and interpolates it.
    void updateCamera(float elapsedSeconds);

    /// Stores the preset names shown in the editor's spawn menu.
    void fillSpawnMenu(std::vector<std::string> nature, std::vector<std::string> items, std::vector<std::string> units, std::vector<std::string> structures);

    /// Spawn menu section for nature presets (select, save, load).
    void natureSpawnMenu();

    /// Spawn menu section for item presets (select, save, load).
    void itemSpawnMenu();

    /// Spawn menu section for structure presets (select, save, load).
    void structureSpawnMenu();

    /// Spawn menu section for unit presets (select, save, load).
    void unitSpawnMenu();

    /// In-game HUD: resources, prices and action buttons; intro, defeat and victory dialogs.
    void gameUI();

    /// Main menu: new game/continue, options, controls, quit.
    void menuUI();

    /// Loads the UI fonts and sets the ImGui style and colors. Must run before the font atlas is built.
    void applyCustomImguiTheme();

    /// Creates nature preset `id` 10 units in front of the camera, on the ground. Unused.
    bool spawnNature(std::string id);

    /// Creates item preset `id` 10 units in front of the camera, on the ground. Unused.
    bool spawnItem(std::string id);

    /// Creates structure preset `id` 10 units in front of the camera, on the ground. Unused.
    bool spawnStructure(std::string id);

    /// Creates unit preset `id` 10 units in front of the camera, on the ground. Unused.
    bool spawnUnit(std::string id);

    /// Editor: creates the selected preset at `position` (with the rotation of the previous spawn) and marks it for saving.
    void spawn(tg::pos3 position);

    /// Editor: like `spawn`, with random rotation about y and random scale (0.8-1.2); used for nature.
    void spawnRandomly(tg::pos3 position);

    /// Focuses the camera on the selected instance (key F).
    void flyToSelectedinstance();

    /// Toggles the editor mode.
    void toggleEditorMode() { mEditorMode ? mEditorMode = false : mEditorMode = true; }

    /// Toggles between paused and playing (from the menu state, switches to playing).
    void togglePause() { state == PLAY ? state = PAUSED : state = PLAY; }

    /// Switches between the menu and playing, moving the camera between the menu view and the camp.
    void toggleMenu();

    /// Leaves the menu paused, behind the intro dialog (which unpauses). Does not reset the world (see TODO).
    void newGame();
};
