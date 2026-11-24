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

#include "GameHooks.hh"
#include "GameSettings.hh"
#include "profiling/Profiler.hh"

#include <memory>
#include <optional>

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
 * Owns the ECS, all systems and factories, the terrain, the camera and the settings, and drives them from the glow-extras `GlfwApp` main loop:
 *
 *  - `init()` wires everything up (component and system registration, asset scan, preset setup, start layout).
 *  - `update()` runs the systems at a fixed 60 Hz: audio always, particles and lights except while paused, all other systems only while playing.
 *  - `render()` handles mouse/keyboard input, passes the per-frame state to the `RenderSystem` and renders.
 *  - `onGui()` builds the menus, the in-game HUD and, in editor mode (key E), the debug, editor and spawn windows.
 *
 * Every run measures its startup phases, system updates, render, GPU time and frame interval with a `profiling::Profiler`;
 * the debug UI shows the latest values.
 *
 * Outside code controls a run through two seams, both set at construction:
 *  - `GameSettings`: how the game starts (seed, vsync, input, straight into play, daily waves).
 *  - `GameHooks` (optional, not owned): notified at startup, after every played tick and at shutdown.
 *    It gets the profiler and the `GamelogicSystem`; `benchmark::Session` uses this to play scripted scenarios.
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

    gamedev::AudioSystem mAudioSystem; ///< Unused: the ECS-registered `mAudioSys` plays the sounds.

    //UI
    ImFont* font1; ///< Default UI font (Source Sans 3, 25 px): added first, so ImGui uses it for all other text.
    ImFont* font2; ///< Title font of the main menu (Sheila Crayon, 45 px).

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
    bool mEnablePostProcess = true; ///< Unused: the debug UI's "Invert colors" sets it, but the renderer never gets it.
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

    tg::pos3 mSPLPosition = {0.0, 0.0, 0.0}; ///< Unused: editable in the debug UI only.
    tg::vec3 mSPLColor = {1.0, 1.0, 1.0};    ///< Unused: editable in the debug UI only.
    float mSPLIntensity = 1.0;               ///< Unused: editable in the debug UI only.
    float mSPLRadius = 50.0;                 ///< Unused: editable in the debug UI only.

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
    bool mGameLost = false; ///< Defeat: the firewood dropped below 0 (checked by `gameUI`).
    bool mGameWon = false;  ///< Victory: all supply route upgrades were built (checked by `gameUI`).

    // Editor
    std::string mSelectedObject = "";                           ///< Preset to spawn with the right mouse button.
    gamedev::Factory* mSelectedFac;                             ///< Factory of `mSelectedObject` (TODO: not initialized, see `spawn`).
    std::vector<std::string> mNatureObjects;                    ///< Nature presets for the spawn menu.
    std::vector<std::string> mStructureObjects;                 ///< Structure presets for the spawn menu.
    std::vector<std::string> mItemObjects;                      ///< Item presets for the spawn menu.
    std::vector<std::string> mUnitObjects;                      ///< Unit presets for the spawn menu.
    gamedev::InstanceHandle mCreatedLast = {std::uint32_t(-1)}; ///< Last spawned instance; new spawns copy its rotation.

    // TODO: The file name buffers are not initialized, so the editor's text fields start with indeterminate content.
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
    std::unique_ptr<gamedev::EngineECS> mECS;                    ///< The ECS; owns the registered systems. Destroyed in `onClose`.
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

    int frameControl = 0;                                                                             ///< Counts simulation ticks. Unused.
    unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2}; ///< Unused.

    // Launch
    gamedev::GameSettings mSettings;      ///< How the game starts and runs.
    gamedev::GameHooks* mHooks = nullptr; ///< Observer of the run (e.g. a benchmark), or nullptr; not owned.

    // Profiling

    /// Zones of `mProfiler`, in the order of the debug UI and benchmark results.
    struct ProfileZones
    {
        // Startup phases of `init` (once)

        gamedev::profiling::ZoneId window; ///< Window, GL context and ImGui.
        gamedev::profiling::ZoneId engine; ///< ECS, systems, factories, terrain and render resources.
        gamedev::profiling::ZoneId assets; ///< Mesh scan and preset registration.
        gamedev::profiling::ZoneId world;  ///< Start layout (`.fac` files) and player.

        // Per tick

        gamedev::profiling::ZoneId update;      ///< The whole `update` tick (all systems and the wave logic).
        gamedev::profiling::ZoneId audio;       ///< `AudioSystem::Update`.
        gamedev::profiling::ZoneId particles;   ///< `ParticleSystem::Update` (not while particles are off).
        gamedev::profiling::ZoneId lights;      ///< `LightSystem::Update`.
        gamedev::profiling::ZoneId physics;     ///< `PhysicsSystem::Update`.
        gamedev::profiling::ZoneId collision;   ///< `CollisionSystem::Update`.
        gamedev::profiling::ZoneId environment; ///< `EnvironmentSystem::Update`.
        gamedev::profiling::ZoneId gamelogic;   ///< `GamelogicSystem::Update`.
        gamedev::profiling::ZoneId animation;   ///< `AnimationSystem::Update`.
        gamedev::profiling::ZoneId cleanup;     ///< `CleanupSystem::Update`.

        // Per frame

        gamedev::profiling::ZoneId render; ///< CPU time in `render`: submitting the GL work, plus driver waits when the GPU is the bottleneck.
        gamedev::profiling::ZoneId gpu;    ///< GPU time of `render` as glow-extras' non-blocking GPU timer last reported it (see `onFrameStart`).
        gamedev::profiling::ZoneId frame;  ///< Time between two frame starts: the frame rate the whole game reaches.
    };

    gamedev::profiling::Profiler mProfiler;                               ///< Timings of this run.
    ProfileZones mZones;                                                  ///< Zones of `mProfiler`.
    std::optional<gamedev::profiling::Clock::time_point> mLastFrameStart; ///< Start of the previous frame.


    // ctor
public:
    /**
     * Creates the app with the ImGui integration of `GlfwApp` and registers the profiler zones.
     *
     * @param settings How the game starts and runs.
     * @param hooks Observer of the run, or nullptr; not owned, must outlive `run()`.
     */
    explicit Game(gamedev::GameSettings settings = {}, gamedev::GameHooks* hooks = nullptr);

    // events
public:
    //
    // virtual methods
    //
    /**
     * Called once after OpenGL is set up:
     *  - creates and connects all systems, factories and assets,
     *  - creates the camp (`GamelogicSystem::SpawnPlayer`),
     *  - loads the start layout (`nature.fac`, `startStructs.fac`, `items.fac`),
     *  - sets a day length of 0.5 real minutes.
     */
    void init() override;

    /**
     * Called at the start of each frame, after input polling (local glow-extras patch, see CLAUDE.md):
     * starts a new statistics frame and records the frame interval and the GPU time of `render`.
     *
     * The GPU time comes from glow-extras' non-blocking GPU timer and is not exactly per frame:
     * it is usually 1-3 frames old, averages all measurements that finished since the last frame, and repeats the previous value if none did.
     * It is not recorded until the timer has a first result.
     */
    void onFrameStart() override;

    /**
     * Called at a fixed 60 Hz. Updates:
     *  - audio (always),
     *  - particles and lights (except while paused),
     *  - while playing: all other simulation systems, the daily enemy waves (day + 3 monsters) and reinforcements (day / 2 + 2 pioneers),
     *    and the deferred destruction (`CleanupSystem`, last).
     *
     * Then, if a tick was played, notifies `GameHooks::OnTickEnd` and closes the window if it asks to.
     *
     * @param elapsedSeconds Fixed time step in seconds (1/60).
     */
    void update(float elapsedSeconds) override;

    /**
     * Called once per frame (variable timestep):
     * camera and mouse/keyboard input (`processInput`; only if `GameSettings::inputEnabled`), renderer state, and rendering.
     *
     * Cursor capture for camera rotation (`setCursorMode`) takes effect at the start of the next frame,
     * when glow-extras applies it in `updateInput()`.
     * While the cursor is disabled, glow-extras switches GLFW to raw (unaccelerated) mouse motion if the platform supports it.
     *
     * @param elapsedSeconds Frame time in seconds.
     */
    void render(float elapsedSeconds) override;

    /// Called once per frame to build the UI: main menu, HUD and dialogs, or the editor windows.
    void onGui() override;

    /**
     * Notifies `GameHooks::OnClosing`, then releases the statistics, all instances, the factories, the systems and the ECS
     * while the GL context still exists.
     */
    void onClose() override;

    /**
     * Called when the framebuffer is resized: resizes the render targets.
     *
     * @param w New framebuffer width in pixels.
     * @param h New framebuffer height in pixels.
     */
    void onResize(int w, int h) override;

    /**
     * Zooms the strategy camera along the view direction, unless in the menu or free-fly mode.
     *
     * Only zooms while the camera height minus `sy` stays between 1 and 25.
     *
     * TODO: Does not call `GlfwApp::onMouseScroll`, so ImGui never sees the scroll wheel
     * (UI windows cannot be scrolled with it) and scrolling over the UI zooms the camera.
     *
     * @param sx Horizontal scroll offset (ignored).
     * @param sy Vertical scroll offset; positive zooms in.
     * @return Always true (event consumed).
     */
    bool onMouseScroll(double sx, double sy) override;

    //
    // your own methods
    //
    /**
     * Moves the camera target from keyboard/mouse input and interpolates the camera towards it.
     *
     * Left Shift speeds up (x4), left Ctrl slows down (x0.25); the base speed is 15 units per second.
     * In the menu, the camera is only interpolated.
     *
     *  - Free-fly camera: WASD moves, Q/E down/up, right mouse button looks around (capturing the cursor if `mCaptureMouseOnMouselook`).
     *  - Strategy camera: WASD or the cursor at the window border (2 px) moves parallel to the ground, Q/E zooms along the view direction,
     *    and the middle mouse button rotates the camera about the point it looks at on the ground (applied directly, without interpolation).
     *
     * TODO: E also toggles the editor mode (in `render`), so moving up or zooming in with E switches the editor on/off.
     *
     * @param elapsedSeconds Frame time in seconds.
     */
    void updateCamera(float elapsedSeconds);

    /// Handles the mouse and keyboard input of a frame: selection, orders, editor spawning and key shortcuts (pause, menu, editor, ...).
    void processInput();

    /**
     * Registers the zones of `Game` in a profiler.
     *
     * @param profiler Profiler to register in.
     * @return The registered zones.
     */
    static ProfileZones registerProfileZones(gamedev::profiling::Profiler& profiler);

    /**
     * Runs the systems and the wave logic of one 60 Hz tick (see `update`).
     *
     * @param elapsedSeconds Fixed time step in seconds (1/60).
     */
    void updateSystems(float elapsedSeconds);

    /// Where the game stands: won, lost or still running.
    gamedev::GameOutcome outcome() const;

    /// Stores the preset names shown in the editor's spawn menu.
    void fillSpawnMenu(std::vector<std::string> nature, std::vector<std::string> items, std::vector<std::string> units, std::vector<std::string> structures);

    /// Spawn menu section for nature presets: select one, save all (`nature.fac`) or the spawned ones, load `nature.fac`.
    void natureSpawnMenu();

    /// Spawn menu section for item presets: select one, save all (`items.fac`) or the spawned ones, load `items.fac`.
    void itemSpawnMenu();

    /// Spawn menu section for structures: select one, save all (`structures.fac`) or the spawned ones, load `structures.fac`.
    void structureSpawnMenu();

    /// Spawn menu section for unit presets: select one, save all (`units.fac`) or the spawned ones, load `units.fac`.
    void unitSpawnMenu();

    /**
     * In-game HUD: resources, prices and action buttons; intro, defeat and victory dialogs.
     *
     * Also sets `mGameLost` and `mGameWon` from the game state, so defeat and victory are only detected while the HUD is shown (not in editor mode).
     */
    void gameUI();

    /// Main menu: new game/continue, options, controls, quit.
    void menuUI();

    /// Loads the UI fonts and sets the ImGui style and colors. Must run before the font atlas is built.
    void applyCustomImguiTheme();

    /**
     * Creates a nature preset 10 units in front of the camera, on the ground. Unused.
     *
     * @param id Preset name or path.
     * @return Always true.
     */
    bool spawnNature(std::string id);

    /**
     * Creates an item preset 10 units in front of the camera, on the ground. Unused.
     *
     * @param id Preset name or path.
     * @return Always true.
     */
    bool spawnItem(std::string id);

    /**
     * Creates a structure preset 10 units in front of the camera, on the ground. Unused.
     *
     * @param id Preset name or path.
     * @return Always true.
     */
    bool spawnStructure(std::string id);

    /**
     * Creates a unit preset 10 units in front of the camera, on the ground. Unused.
     *
     * @param id Preset name or path.
     * @return Always true.
     */
    bool spawnUnit(std::string id);

    /**
     * Editor: creates the selected preset (`mSelectedObject`) and marks it for "Save created ...".
     *
     * Copies the rotation of the previous spawn (`mCreatedLast`), so a rotation set with the gizmo carries over.
     *
     * TODO: `MarkLastCreated` is called even if nothing was created:
     * it marks the previous instance again, or, if no preset was selected yet, is called through the uninitialized `mSelectedFac` (crash).
     *
     * @param position World position; the height is taken from the terrain.
     */
    void spawn(tg::pos3 position);

    /**
     * Editor: like `spawn`, but with a random rotation about y and a random uniform scale (0.8 to 1.2); used for nature.
     *
     * Does not update `mCreatedLast`.
     *
     * TODO: Same unconditional `MarkLastCreated` as in `spawn`.
     *
     * @param position World position; the height is taken from the terrain.
     */
    void spawnRandomly(tg::pos3 position);

    /**
     * Focuses the camera on the selected instance (while F is held).
     *
     * Keeps the view direction and moves the camera target to 1.5 times the sum of the instance's scale components away from it.
     */
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
