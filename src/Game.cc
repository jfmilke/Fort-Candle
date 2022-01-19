#include "Game.hh"

#include <limits>

// glow OpenGL wrapper
#include <glow/common/log.hh>
#include <glow/common/scoped_gl.hh>
#include <glow/objects/ArrayBuffer.hh>
#include <glow/objects/UniformBuffer.hh>
#include <glow/objects/Framebuffer.hh>
#include <glow/objects/TextureCubeMap.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Texture2D.hh>
#include <glow/objects/TextureRectangle.hh>
#include <glow/objects/VertexArray.hh>

// extra functionality of glow
#include <glow-extras/debugging/imgui-util.hh>
#include <glow-extras/geometry/FullscreenTriangle.hh>
#include <glow-extras/geometry/Quad.hh>
#include <glow-extras/geometry/UVSphere.hh>

#include "utility/Random.hh"

#include <GLFW/glfw3.h> // window/input framework

#include <imgui/imgui.h> // UI framework

#include <typed-geometry/tg.hh> // math library

#include <typed-geometry/feature/std-interop.hh> // support for writing tg objects to streams

#include <typed-geometry/feature/bezier.hh> // bezier curves for procgen

#include "Components.hh"


#define GAMEDEV_ADVANCED 1

Game::Game() : GlfwApp(Gui::ImGui) {}

void Game::init()
{
    // enable vertical synchronization to synchronize rendering to monitor refresh rate
    setVSync(true);

    // disable built-in camera
    setUseDefaultCamera(false);

    setEnableDebugOverlay(false);

    // set the window resolution
    setWindowWidth(1920);
    setWindowHeight(1080);

    // IMPORTANT: call to base class
    GlfwApp::init();

    // set the GUI color theme
    // glow::debugging::applyGlowImguiTheme(true);
    applyCustomImguiTheme();

    // set window title
    setTitle("Fort Candle");

    // set initial camera state
    mEnableTopDownCamera = false;
    /*mCamera.target.position = tg::pos3(-5.f,1.f,0.f);
    mCamera.target.forward = tg::normalize(tg::vec3(1.f, -0.7f, 0.f));*/
    mCamera.physical = mCamera.target;

    mAdvancedFeatures.initialize();

    // Game Assets
    mGameAssets = std::make_shared<gamedev::GameObjects>();
    mGameAssets->initialize();

    // ECS
    mECS = std::make_shared<gamedev::EngineECS>();
    mECS->Init(mAdvancedFeatures.mWorld);

    mAdvancedFeatures.setECS(mECS);

    // Register components before use
    mECS->RegisterComponent<gamedev::Prototype>();
    mECS->RegisterComponent<gamedev::Collider>();
    mECS->RegisterComponent<gamedev::CircleShape>();
    mECS->RegisterComponent<gamedev::BoxShape>();
    mECS->RegisterComponent<gamedev::Render>();
    mECS->RegisterComponent<gamedev::Physics>();
    mECS->RegisterComponent<gamedev::PointLightEmitter>();
    mECS->RegisterComponent<gamedev::ParticleEmitter>();
    mECS->RegisterComponent<gamedev::Living>();
    mECS->RegisterComponent<gamedev::Mortal>();
    mECS->RegisterComponent<gamedev::Destructible>();
    mECS->RegisterComponent<gamedev::Producer>();
    mECS->RegisterComponent<gamedev::Climber>();
    mECS->RegisterComponent<gamedev::Attacker>();
    mECS->RegisterComponent<gamedev::Arrow>();
    mECS->RegisterComponent<gamedev::SoundEmitter>();
    mECS->RegisterComponent<gamedev::Animated>(); 
    mECS->RegisterComponent<gamedev::Dweller>();
    mECS->RegisterComponent<gamedev::InTower>();
    mECS->RegisterComponent<gamedev::Tower>();
    mECS->RegisterComponent<gamedev::ForeignControl>();

    // Register systems managed by ECS before use
    mCollisionSys = mECS->RegisterSystem<gamedev::CollisionSystem>();
    {
        Signature signature;
        signature.set(mECS->GetComponentType<gamedev::Collider>());
        mECS->SetSystemSignature<gamedev::CollisionSystem>(signature);
    }
    mRenderSys = mECS->RegisterSystem<gamedev::RenderSystem>();
    {
        Signature signature;
        signature.set(mECS->GetComponentType<gamedev::Render>());
        mECS->SetSystemSignature<gamedev::RenderSystem>(signature);
    }
    mPhysicsSys = mECS->RegisterSystem<gamedev::PhysicsSystem>();
    {
        Signature signature;
        signature.set(mECS->GetComponentType<gamedev::Physics>());
        mECS->SetSystemSignature<gamedev::PhysicsSystem>(signature);
    }
    mLightSys = mECS->RegisterSystem<gamedev::LightSystem>();
    {
        Signature signature;
        signature.set(mECS->GetComponentType<gamedev::PointLightEmitter>());
        mECS->SetSystemSignature<gamedev::LightSystem>(signature);
    }
    mParticleSys = mECS->RegisterSystem<gamedev::ParticleSystem>();
    {
        Signature signature;
        signature.set(mECS->GetComponentType<gamedev::ParticleEmitter>());
        mECS->SetSystemSignature<gamedev::ParticleSystem>(signature);
    }

    mGamelogicSys = mECS->RegisterSystem<gamedev::GamelogicSystem>();
    {
        Signature signature;
        signature.set(mECS->GetComponentType<gamedev::Living>());
        mECS->SetSystemSignature<gamedev::GamelogicSystem>(signature);
    }

    mAnimationSys = mECS->RegisterSystem<gamedev::AnimationSystem>();
    {
        Signature signature;
        signature.set(mECS->GetComponentType<gamedev::Animated>());
        mECS->SetSystemSignature<gamedev::AnimationSystem>(signature);
    }

    mAudioSys = mECS->RegisterSystem<gamedev::AudioSystem>();
    {
        Signature signature;
        signature.set(mECS->GetComponentType<gamedev::SoundEmitter>());
        mECS->SetSystemSignature<gamedev::AudioSystem>(signature);
    }
    mCleanSys = mECS->RegisterSystem<gamedev::CleanupSystem>();

    // Create autonomous systems not managed by ECS before use
    mEnvironmentSys = std::make_shared<gamedev::EnvironmentSystem>(); 

    // Initialize systems
    mRenderSys->Init(mECS);
    mPhysicsSys->Init(mECS);
    mEnvironmentSys->Init(mECS);
    mLightSys->Init(mECS);
    mParticleSys->Init(mECS);
    mAnimationSys->Init(mECS);
    mCleanSys->Init(mECS);
    mAudioSys->Init(mECS);

    // Factories
    mUnitFac = std::make_shared<gamedev::UnitFactory>();
    mStructFac = std::make_shared<gamedev::StructureFactory>();
    mNatureFac = std::make_shared<gamedev::NatureFactory>();
    mItemFac= std::make_shared<gamedev::ItemFactory>();

    mGamelogicSys->Init(mECS, mAudioSys, mUnitFac, mStructFac, mItemFac, mGameAssets, 3.0);

    mItemFac->Init(mECS, mGameAssets);
    mUnitFac->Init(mECS, mGameAssets);
    mStructFac->Init(mECS, mGameAssets);
    mNatureFac->Init(mECS, mGameAssets);

    // Terrain
    mTerrain = std::make_shared<gamedev::Terrain>();
    mTerrain->Init(mECS);
    mTerrain->setGroundMesh("../data/meshes/world_ground.obj");
    mTerrain->setWaterMesh("../data/meshes/world_water.obj");
    mTerrain->setGroundTexture(mGameAssets->mTexAlbedoPolyPalette);
    mTerrain->setWaterTexture(mGameAssets->mTexAlbedoPolyPalette);
    mTerrain->CreateGround(tg::pos3(0), tg::size3(3));
    mTerrain->CreateWater(tg::pos3(0), tg::size3(3));

    mPhysicsSys->SetTerrain(mTerrain);
    mItemFac->AttachTerrain(mTerrain);
    mUnitFac->AttachTerrain(mTerrain);
    mStructFac->AttachTerrain(mTerrain);
    mNatureFac->AttachTerrain(mTerrain);
    mGamelogicSys->AttachTerrain(mTerrain);
    mCollisionSys->Init(mECS, mTerrain->GetSize());

    // Initialize GFX
    mRenderSys->UpdateWindowResolution(getWindowSize());
    mRenderSys->UpdateDirShadowResolution(mDirShadowResolution);
    mRenderSys->UpdatePointShadowResolution(mPointShadowResolution);
    mRenderSys->InitResources();
    mRenderSys->InitParticles(mParticleSys->GetParticleVAO());

    mGameAssets->scanMeshes();
    mUnitFac->RegisterAutomatically(mGameAssets->getObjectsByFolder("units"), 0.25f, true);
    mNatureFac->RegisterAutomatically(mGameAssets->getObjectsByFolder("nature"), 0.8f);
    mStructFac->RegisterAutomatically(mGameAssets->getObjectsByFolder("structures"), 2.f, true);
    mItemFac->RegisterAutomatically(mGameAssets->getObjectsByFolder("items"), 0.8f);

    mStructFac->InitResources();
    mUnitFac->InitResources();
    mItemFac->InitResources();

    fillSpawnMenu(mNatureFac->GetNameIdentifiers(), mItemFac->GetNameIdentifiers(), mUnitFac->GetNameIdentifiers(), mStructFac->GetNameIdentifiers());

    // Initialize Game State
    mGamelogicSys->SpawnPlayer();

    if (state == MENU)
    {
        mCamera.target.position = tg::pos3(1.34, 1.83, 24.92);
        mCamera.target.forward = tg::normalize(tg::vec3(-0.49, -0.19, -0.85));
        mCamera.physical = mCamera.target;
    }
    else
    {
        mCamera.target.position = mGamelogicSys->mHome.center + tg::vec3::unit_y * 5.0 - tg::vec3::unit_x * 7.0;
        mCamera.target.forward = tg::normalize(tg::vec3(1.f, -0.7f, 0.f));
        mCamera.physical = mCamera.target;
    }

    mNatureFac->LoadObjects();
    mStructFac->LoadObjects("startStructs");
    mItemFac->LoadObjects();

    mEnvironmentSys->SetMinutesPerDay(0.5);
}

void Game::onFrameStart()
{
    gamedev::newStatFrame(mAdvancedFeatures.mStats);
}

// update game in 60 Hz fixed timestep
void Game::update(float elapsedSeconds)
{

    *mAdvancedFeatures.mStatNumUpdates += 1.f;

    mAudioSys->SetMasterVolume(mMasterVolume / 100.f);
    mAudioSys->SetVolume(0, mEffectVolume / 100.f);
    mAudioSys->SetVolume(1, mAmbientVolume / 100.f);
    mAudioSys->SetVolume(2, mMusicVolume / 100.f);
    mAudioSys->SetVolume(3, mUIVolume / 100.f);
    mAudioSys->UpdateListener(mCamera.physical.position, mCamera.physical.forward);

    mAudioSys->Update(elapsedSeconds);

    if (state == PLAY || state == MENU)
    {
        mTime_ParticleSys = (mEnableParticleSpawn) ? mParticleSys->Update(elapsedSeconds) / 1000.f : 0.f;
        mTime_LightSys = mLightSys->Update(elapsedSeconds) / 1000.f;
    }

    if (state == PLAY)
    {
        auto t_0 = std::chrono::steady_clock::now();
        mTime_PhysicsSys = mPhysicsSys->Update(elapsedSeconds) / 1000.f;
        mTime_CollisionSys = mCollisionSys->Update(elapsedSeconds) / 1000.f;
        mTime_EnvironmentSys = mEnvironmentSys->Update(elapsedSeconds) / 1000.f;
        mTime_GamelogicSys = mGamelogicSys->Update(elapsedSeconds) / 1000.f;
        mTime_AnimationSys = mAnimationSys->Update(elapsedSeconds) / 1000.f;

        if (mEnvironmentSys->mNewDay)
        {
            auto day = mEnvironmentSys->getDayCount();

            if (!mHoldSpawns)
            {
                if (!(day % 1))
                    mGamelogicSys->SpawnEnemyWave(day + 3);

                if (!(day % 1))
                    mGamelogicSys->SpawnReinforcements(day / 2 + 2);
            }
        }

        mTime_CleanSys = mCleanSys->Update() / 1000.f;

        auto t_n = std::chrono::steady_clock::now();
        mTime_AllSys = std::chrono::duration_cast<std::chrono::microseconds>(t_n - t_0).count() / 1000.f;

        frameControl++;
    }
}

// render game variable timestep
void Game::render(float elapsedSeconds)
{
    if (mFullscreen != GlfwApp::isFullscreen())
        toggleFullscreen();

    *mAdvancedFeatures.mStatFrametime = (elapsedSeconds * 1000.f); // in milliseconds

    updateCamera(elapsedSeconds);

    if (state != MENU)
    {
        // ToDo: Put Animation Update here (see "mAdvancedFeatures.updateAnimationsInWorld(..)")
        // Selecting Instances
        if (!ImGui::GetIO().WantCaptureMouse && input().isMouseButtonReleased(GLFW_MOUSE_BUTTON_1))
        {
            // Get possible selection
            auto const mousePos = input().getMousePosition();
            auto target = mRenderSys->ReadPickingBuffer(int(mousePos.x), getWindowHeight() - int(mousePos.y));

            mAdvancedFeatures.mSelectedInstance = target;

            if (!mEditorMode)
            {
                auto renderable = mECS->TryGetComponent<gamedev::Render>(mAdvancedFeatures.mSelectedInstance);
                if (renderable && !renderable->selectable)
                {
                    mAdvancedFeatures.mSelectedInstance = {std::uint32_t(-1)};
                    mGamelogicSys->Deselect();
                }
                else
                {
                    mGamelogicSys->Select(target);
                }
            }
        }

        // Pause game
        if (input().isKeyPressed(GLFW_KEY_P))
            togglePause();

        // Flying to Instances
        if (input().isKeyDown(GLFW_KEY_F))
            flyToSelectedinstance();

        // Toggle top down camera
        if (input().isKeyPressed(GLFW_KEY_SPACE) && !mEnableFreeFlyCam)
        {
            if (mEnableTopDownCamera)
            {
                mCamera.target.forward = tg::normalize(tg::vec3(0.2f, -0.1f, 0.0f));
                mEnableTopDownCamera = false;
            }
            else
            {
                mCamera.target.forward = tg::normalize(tg::vec3(0.01f, -0.99f, 0.0f));
                mEnableTopDownCamera = true;
            }
        }
    }

    // Fullscreen
    if (input().isKeyPressed(GLFW_KEY_F11))
        mFullscreen ? mFullscreen = false : mFullscreen = true;

    // Editor mode
    if (input().isKeyPressed(GLFW_KEY_E))
        toggleEditorMode();

    // Show menu
    if (input().isKeyPressed(GLFW_KEY_ESCAPE))
        toggleMenu();

    auto runtime = getCurrentTime();

    // Capture mouse clicks in worldspace
    if (!ImGui::GetIO().WantCaptureMouse && input().isMouseButtonReleased(GLFW_MOUSE_BUTTON_2))
    {
        auto const mousePos = input().getMousePosition();
        auto target = mRenderSys->ReadPickingBuffer(int(mousePos.x), getWindowHeight() - int(mousePos.y));
        auto w_position = mRenderSys->MouseToWorld(mousePos.x, getWindowHeight() - mousePos.y);
        w_position.y = mTerrain->heightAt(w_position);

        if (!mEditorMode)
        {
            auto renderable = mECS->TryGetComponent<gamedev::Render>(target);

            if (target.is_valid() && renderable && renderable->targetable)
            {
                mGamelogicSys->Rightclick(target);
            }
            else
            {
                mGamelogicSys->Rightclick(w_position);
            }
        }
        else
        {
            if (mSelectedFac == mNatureFac.get())
            {
                spawnRandomly(w_position);
            }
            else
            {
                spawn(w_position);
            }
        }
    }

    mRenderSys->UpdateTime(runtime, mEnvironmentSys->getGameTime());
    mRenderSys->UpdateWindowResolution(getWindowSize());
    mRenderSys->UpdateCamera(mCamera.physical.position, mCamera.physical.forward, tg::vec3::unit_y);
    mRenderSys->UpdateSepia(mSepiaStrength);
   // mRenderSys->UpdateDesaturation(mDesaturationStrength);
    mRenderSys->UpdateDesaturation(mGamelogicSys->GetDesaturation());
    //mRenderSys->UpdateVignette(mVignetteColor, mVignetteRadius, mVignetteSoftness, mVignetteIntensity);
    mRenderSys->UpdateVignette(mVignetteColor, mGamelogicSys->GetVignetteRadius(), mVignetteSoftness, mVignetteIntensity);
    mRenderSys->UpdateDistFadeoff(mDistFadeoffColor, mDistFadeoffIntensity);
    mRenderSys->UpdateSelected(mAdvancedFeatures.mSelectedInstance);
    mRenderSys->UpdateOutlined(mGamelogicSys->GetOutlinedHandles(), mGamelogicSys->GetOutlinedColors());
    mRenderSys->UpdateSelected(mAdvancedFeatures.mSelectedInstance);

    mRenderSys->UpdateParticleCount(mParticleSys->GetParticleCount());
    if (mEnableDirShadows)
        mRenderSys->UpdateDirShadowResolution(mDirShadowResolution);
    if (mEnablePointShadows)
        mRenderSys->UpdatePointShadowResolution(mPointShadowResolution);
    if (mEnableAmbLight)
        mRenderSys->UpdateAmbientLight({mEnvironmentSys->GenerateAmbientlight(mAmbientColor, mAmbientIntensity)});
    if (mEnableDirLight)
        mRenderSys->UpdateDirectionalLight(mEnvironmentSys->GenerateSunlight(mSunTheta, mSunPhi, mSunColor, mSunIntensity));
    if (mEnablePointLight)
    {
        mRenderSys->UpdateRegularPointLights(mLightSys->GetLights());
        //mRenderSys->UpdateShadowingPointLight(mEnvironmentSys->GeneratePointlight(mGamelogicSys->mHome.center + tg::vec3::unit_y * 1.f, mSPLColor, mSPLIntensity, mSPLRadius));
        mRenderSys->UpdateShadowingPointLight(mLightSys->GetShadowingPointlight());
    }

    mRenderSys->SwitchDebugShading(mShowNormals, mShowWireframe, mCullFaces);
    mRenderSys->SwitchLightShading(mEnableAmbLight, mEnableDirLight, mEnablePointLight);
    mRenderSys->SwitchShadowShading(mEnablePointShadows, mEnableDirShadows);
    mRenderSys->SwitchGameShading(mEnableFogOfWar);
    mRenderSys->SwitchPostProcessing(mEnableSRGBCurve, mEnableTonemap, mEnableSepia, mEnableVignette, mEnableDesaturate, mEnableBloom, mEnableDistFadeoff);
    mRenderSys->SwitchParticles(mEnableParticleSpawn);

    if (mEnableParticleSpawn)
        mParticleSys->UploadParticleData();

    mRenderSys->Update(elapsedSeconds);

    mAdvancedFeatures.updateStatDraws(mRenderSys->GetDrawCalls());
}

// Update the GUI
void Game::onGui()
{
    // for more information on what ImGui is capable off, see: https://github.com/ocornut/imgui

    // all GUI-related objects (sliders, buttons, etc) have to be within this Begin-Scope
    // the scope is closed with a corresponding End call
    if (state != MENU)
    {
        if (mEditorMode)
        {
            if (ImGui::Begin("GameDev Project SS21"))
            {
                if (ImGui::TreeNodeEx("Controls", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::TextUnformatted("WASD  - Move");
                    ImGui::TextUnformatted("Q/E   - Move up and down");
                    ImGui::TextUnformatted("Shift - Speed up");
                    ImGui::TextUnformatted("Ctrl  - Slow down");
                    ImGui::TextUnformatted("RMB   - Mouselook");
                    ImGui::TextUnformatted("LMB   - Select objects");
                    ImGui::TextUnformatted("F     - Fly to selected object");
                    ImGui::NewLine();
                    ImGui::Text("Cam pos: %.2f %.2f %.2f", mCamera.physical.position.x, mCamera.physical.position.y, mCamera.physical.position.z);
                    ImGui::Text("Cam fwd: %.2f %.2f %.2f", mCamera.physical.forward.x, mCamera.physical.forward.y, mCamera.physical.forward.z);
                    auto const mousePos = input().getMousePosition();
                    ImGui::Text("Cursor pos: %.2f %.2f", mousePos.x, mousePos.y);
                    auto const mouseDelta = input().getMouseDeltaF();
                    ImGui::Text("Cursor delta: %.2f %.2f", mouseDelta.x, mouseDelta.y);
                    ImGui::Checkbox("Capture mouse during mouselook", &mCaptureMouseOnMouselook);
                    ImGui::Checkbox("Enable FreeflyCam", &mEnableFreeFlyCam);
                    ImGui::NewLine();

                    if (ImGui::Checkbox("Editor Mode", &mEditorMode) && !mEditorMode)
                    {
                        mAdvancedFeatures.mSelectedInstance = {std::uint32_t(-1)}; // Deselect when switching editor mode
                        mGamelogicSys->Deselect();
                    }
                    ImGui::Checkbox("Hold Spawns", &mHoldSpawns);
                    ImGui::TreePop();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::TreeNodeEx("Graphics Settings", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Checkbox("Show Normals", &mShowNormals);
                    ImGui::Checkbox("Show Wireframe", &mShowWireframe);
                    ImGui::Checkbox("Enable Tonemapping", &mEnableTonemap);
                    ImGui::Checkbox("Enable sRGB Output", &mEnableSRGBCurve);
                    ImGui::Checkbox("Invert colors", &mEnablePostProcess);
                    ImGui::Checkbox("Cull Faces", &mCullFaces);
                    ImGui::TreePop();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::TreeNodeEx("Time", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("Game Time: %.2f", mEnvironmentSys->getGameTime());
                    ImGui::Text("Day: %i", mEnvironmentSys->getDayCount());

                    ImGui::TreePop();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::TreeNodeEx("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Checkbox("Enable Point Lights", &mEnablePointLight);
                    ImGui::Checkbox("Enable Dir Lights", &mEnableDirLight);
                    ImGui::Checkbox("Enable Amb Light", &mEnableAmbLight);
                    ImGui::NewLine();

                    if (mEnablePointLight)
                    {
                        ImGui::Text("Point Light");
                        ImGui::InputFloat3("PPosition", &mSPLPosition.x);
                        ImGui::ColorEdit3("PColor", &mSPLColor.x);
                        ImGui::SliderFloat("PIntensity", &mSPLIntensity, 0.f, 100.f);
                        ImGui::SliderFloat("PRadius", &mSPLRadius, 0.f, 100.f);
                        ImGui::NewLine();
                    }


                    if (mEnableDirLight)
                    {
                        ImGui::Text("Sun Light");
                        ImGui::SliderFloat2("SPosition", &mSunTheta, -360.f, 360.f);
                        ImGui::ColorEdit3("SColor", &mSunColor.x);
                        ImGui::SliderFloat("SIntensity", &mSunIntensity, 0.f, 10.f);
                        ImGui::NewLine();
                    }

                    if (mEnableAmbLight)
                    {
                        ImGui::Text("Ambient Light");
                        ImGui::SliderFloat("Desaturation Strength", &mDesaturationStrength, 0.f, 1.f);
                        ImGui::SliderFloat("AIntensity", &mAmbientIntensity, 0.f, 1.f);
                    }


                    ImGui::TreePop();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::TreeNodeEx("Shadowing", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Checkbox("Enable Point Shadows", &mEnablePointShadows);
                    ImGui::Checkbox("Enable Dir Shadows", &mEnableDirShadows);

                    ImGui::TreePop();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::TreeNodeEx("FFX", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Checkbox("Sepia", &mEnableSepia);
                    if (mEnableSepia)
                    {
                        ImGui::SliderFloat("Sepia Strength", &mSepiaStrength, 0.f, 1.f);
                    }
                    ImGui::Checkbox("Desaturation", &mEnableDesaturate);
                    if (mEnableDesaturate)
                    {
                        ImGui::SliderFloat("Desaturation Strength", &mDesaturationStrength, 0.f, 1.f);
                    }
                    ImGui::Checkbox("Vignette", &mEnableVignette);
                    if (mEnableVignette)
                    {
                        ImGui::SliderFloat("Vignette Intensity", &mVignetteIntensity, 0.f, 1.f);
                        ImGui::SliderFloat("Vignette Radius", &mVignetteRadius, 0.f, 1.f);
                        ImGui::SliderFloat("Vignette Softness", &mVignetteSoftness, 0.f, 1.f);
                        ImGui::ColorEdit3("Vignette Color", &mVignetteColor.x);
                    }
                    ImGui::Checkbox("Distance Fadeoff", &mEnableDistFadeoff);
                    if (mEnableDistFadeoff)
                    {
                        ImGui::SliderFloat("Fadeoff Intensity", &mDistFadeoffIntensity, 0.f, 10.f);
                        ImGui::ColorEdit3("Fadeoff Color", &mDistFadeoffColor.x);
                    }

                    ImGui::Checkbox("Bloom", &mEnableBloom);
                    ImGui::Checkbox("Particles", &mEnableParticleSpawn);

                    ImGui::TreePop();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::TreeNodeEx("Gamelogic", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Checkbox("Enable Fog of War", &mEnableFogOfWar);

                    ImGui::TreePop();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::TreeNodeEx("Audio Volume", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::SliderFloat("Master", &mMasterVolume, 0.f, 100.f);
                    ImGui::SliderFloat("Effects", &mEffectVolume, 0.f, 100.f);
                    ImGui::SliderFloat("Ambient", &mAmbientVolume, 0.f, 100.f);
                    ImGui::SliderFloat("Music", &mMusicVolume, 0.f, 100.f);
                    ImGui::SliderFloat("UI", &mUIVolume, 0.f, 100.f);

                    ImGui::TreePop();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::TreeNodeEx("Performance (ms)", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("All Systems: %.2f", mTime_AllSys);
                    ImGui::Text("Animations: %.2f", mTime_AnimationSys);
                    ImGui::Text("Cleaning: %.2f", mTime_CleanSys);
                    ImGui::Text("Collisions: %.2f", mTime_CollisionSys);
                    ImGui::Text("Environment: %.2f", mTime_EnvironmentSys);
                    ImGui::Text("Gamelogic: %.2f", mTime_GamelogicSys);
                    ImGui::Text("Lights: %.2f", mTime_LightSys);
                    ImGui::Text("Particles: %.2f", mTime_ParticleSys);
                    ImGui::Text("Physics: %.2f", mTime_PhysicsSys);

                    ImGui::TreePop();
                }
            }
            ImGui::End();
        }

        bool requestedFlyToSelection = false, requestedCreateNewInstance = false;

        if (mEditorMode)
        {
            mAdvancedFeatures.runImgui(requestedFlyToSelection, requestedCreateNewInstance);

            if (ImGui::Begin("Spawn Menu"))
            {
                unitSpawnMenu();
                natureSpawnMenu();
                structureSpawnMenu();
                itemSpawnMenu();
            }
            ImGui::End();

            if (mAdvancedFeatures.mWorld.isLiveHandle(mAdvancedFeatures.mSelectedInstance))
            {
                // a valid instance is selected, get a reference to its transform
                gamedev::transform& transformToManipulate = mAdvancedFeatures.mWorld.getInstanceTransform(mAdvancedFeatures.mSelectedInstance);

                // calculate camera matrices
                auto const resolution = getWindowSize();
                float const aspectRatio = resolution.width / float(resolution.height);
                tg::mat4 const proj = tg::perspective_opengl(60_deg, aspectRatio, 0.01f, 500.f);
                tg::mat4 const view = tg::look_at_opengl(mCamera.physical.position, mCamera.physical.forward, {0, 1, 0});

                // run the editor (some Imgui and a gizmo to move it around)
                gamedev::manipulateTransform(mAdvancedFeatures.mEditorState, view, proj, transformToManipulate);
                if (mECS->TestSignature<gamedev::Physics>(mAdvancedFeatures.mSelectedInstance))
                {
                    gamedev::manipulatePhysics(mAdvancedFeatures.mEditorState, mECS->GetComponent<gamedev::Physics>(mAdvancedFeatures.mSelectedInstance));
                }

                // debug
                if (false)
                {
                    if (mECS->TestSignature<gamedev::BoxShape>(mAdvancedFeatures.mSelectedInstance))
                    {
                        gamedev::showCollisionShape(mAdvancedFeatures.mEditorState, mECS->GetComponent<gamedev::BoxShape>(mAdvancedFeatures.mSelectedInstance),
                                                    mECS->GetInstanceTransform(mAdvancedFeatures.mSelectedInstance));
                    }
                }
            }
        }
        else
        {
            gameUI();

            if (state == PAUSED)
            {
                ImGui::SetNextWindowPos({(float)getWindowWidth() - 100, 100}, 0, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize({150, 90});
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
                if (ImGui::Begin("Paused", 0, window_flags))
                {
                    // ImGui::NewLine();
                    ImGui::Text("Game Paused");
                    ImGui::NewLine();
                }
                ImGui::End();
            }
        }
    }
    else
    {
        menuUI();
    } 
}

void Game::onClose()
{
    mAdvancedFeatures.destroy();

    GlfwApp::onClose();
}

// Called when window is resized
void Game::onResize(int w, int h) { mRenderSys->ResizeRenderTargets(w, h); }

// called once per frame
void Game::updateCamera(float elapsedSeconds)
{
    float speedMultiplier = 15.f;

    // shift / ctrl: speed up and slow down camera movement
    if (isKeyDown(GLFW_KEY_LEFT_SHIFT))
        speedMultiplier *= 4.f;

    if (isKeyDown(GLFW_KEY_LEFT_CONTROL))
        speedMultiplier *= 0.25f;

    if (state != MENU)
    {
        // FreeFly Camera
        if (mEnableFreeFlyCam)
        {
            auto const deltaMove = tg::vec3((isKeyDown(GLFW_KEY_A) ? 1.f : 0.f) - (isKeyDown(GLFW_KEY_D) ? 1.f : 0.f), // x: left and right (A/D keys)
                                            (isKeyDown(GLFW_KEY_E) ? 1.f : 0.f) - (isKeyDown(GLFW_KEY_Q) ? 1.f : 0.f), // y: up and down (E/Q keys)
                                            (isKeyDown(GLFW_KEY_W) ? 1.f : 0.f) - (isKeyDown(GLFW_KEY_S) ? 1.f : 0.f) // z: forward and back (W/S keys)
                                            )
                                   * elapsedSeconds * speedMultiplier;

            // apply relative move
            mCamera.target.moveRelative(deltaMove, true);

            // hold right mouse button: look around
            bool rightMB = isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);

            // if mouse down and UI does not capture it: hide mouse and move camera
            if (rightMB && !ImGui::GetIO().WantCaptureMouse)
            {
                // capture mouse
                if (mCaptureMouseOnMouselook)
                {
                    setCursorMode(glow::glfw::CursorMode::Disabled);
                }

                auto mouse_delta = input().getMouseDelta() * 0.001f;
                mCamera.target.mouselook(mouse_delta.x, mouse_delta.y);
            }
            else
            {
                // uncapture mouse
                if (mCaptureMouseOnMouselook)
                {
                    setCursorMode(glow::glfw::CursorMode::Normal);
                }
            }

            // interpolate physical state to target
            mCamera.interpolateToTarget(elapsedSeconds);
        }
        // Game Camera
        else
        {
            bool middleMB = isMouseButtonDown(GLFW_MOUSE_BUTTON_MIDDLE);

            // camera rotation
            if (middleMB && !ImGui::GetIO().WantCaptureMouse)
            {
                setCursorMode(glow::glfw::CursorMode::Disabled);

                auto mouse_delta = input().getMouseDelta() * 0.001f;

                auto angle = tg::angle_between(mCamera.physical.forward, tg::vec3(0, -1, 0));
                auto distance = tan(angle) * mCamera.physical.position.y;

                auto focus = mCamera.physical.position + normalize(mCamera.physical.forward) * hypot(mCamera.physical.position.y, distance);
                auto origin_to_focus = focus - tg::pos3::zero;

                mCamera.physical.position = tg::translation(origin_to_focus) * tg::rotation_y(60_deg * mouse_delta.x)
                                            * tg::translation(-origin_to_focus) * mCamera.physical.position;
                mCamera.physical.forward = tg::normalize(focus - mCamera.physical.position); // tg::rotation_y(60_deg * mouse_delta.x) * mCamera.physical.forward;

                mCamera.target.position = mCamera.physical.position;
                mCamera.target.forward = mCamera.physical.forward;
            }
            // camera movement
            else
            {
                setCursorMode(glow::glfw::CursorMode::Normal);

                auto const resolution = getWindowSize();
                auto const mousePos = input().getMousePosition();
                int offset = 2;
                int directionX = 0;
                int directionY = 0;

                if (mousePos.x < offset && mousePos.x >= 0)
                    directionX = 1;
                else if (resolution.width - mousePos.x < offset && mousePos.x <= resolution.width)
                    directionX = 2;

                if (mousePos.y < offset && mousePos.y >= 0)
                    directionY = 1;
                else if (resolution.height - mousePos.y < offset && mousePos.y <= resolution.height)
                    directionY = 2;

                auto const deltaMove
                    = tg::vec3(((isKeyDown(GLFW_KEY_A) || directionX == 1) ? 1.f : 0.f) - ((isKeyDown(GLFW_KEY_D) || directionX == 2) ? 1.f : 0.f), // x: left and right (A/D keys)
                               0.f,
                               ((isKeyDown(GLFW_KEY_W) || directionY == 1) ? 1.f : 0.f) - ((isKeyDown(GLFW_KEY_S) || directionY == 2) ? 1.f : 0.f) // z: forward and back (W/S keys)
                               )
                      * elapsedSeconds * speedMultiplier;

                auto const deltaZoom = tg::vec3(0.f, 0.f, (isKeyDown(GLFW_KEY_E) ? 1.f : 0.f) - (isKeyDown(GLFW_KEY_Q) ? 1.f : 0.f)) * elapsedSeconds * speedMultiplier;

                // apply relative move
                mCamera.target.moveRelative(deltaMove, false);
                mCamera.target.moveRelative(deltaZoom, true);

                // interpolate physical state to target
                mCamera.interpolateToTarget(elapsedSeconds);
            }
        }
    }
    else
    {
        mCamera.interpolateToTarget(elapsedSeconds);
    }
}

bool Game::onMouseScroll(double sx, double sy)
{
    if (state != MENU)
    {
        auto minScroll = 1;
        auto maxScroll = 25;
        if (!mEnableFreeFlyCam && (mCamera.physical.position.y - sy > minScroll) && (mCamera.physical.position.y - sy < maxScroll))
        {
            mCamera.target.moveRelative(tg::vec3(0.f, 0.f, sy), true);
        }
    }
    return true;
}

void Game::flyToSelectedinstance()
{
    if (mAdvancedFeatures.mWorld.isLiveHandle(mAdvancedFeatures.mSelectedInstance))
    {
        auto const& instanceTransform = mAdvancedFeatures.mWorld.getInstanceTransform(mAdvancedFeatures.mSelectedInstance);
        float distance = 1.5f * (instanceTransform.scaling.width + instanceTransform.scaling.height + instanceTransform.scaling.depth);
        auto const offset = normalize(mCamera.physical.forward) * -distance;

        mCamera.target.setFocus(tg::pos3(instanceTransform.translation), offset);
    }
}

void Game::fillSpawnMenu(std::vector<std::string> nature, std::vector<std::string> items, std::vector<std::string> units, std::vector<std::string> structures)
{
    mNatureObjects = nature;
    mItemObjects = items;
    mUnitObjects = units;
    mStructureObjects = structures;
}

void Game::natureSpawnMenu()
{
    if (ImGui::TreeNodeEx("Nature", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild("ChildR", ImVec2(0, 260), true);
        ImGui::Columns(2);
        for (int i = 0; i < mNatureObjects.size(); i++)
        {
            if (ImGui::Button(mNatureObjects[i].data(), ImVec2(-FLT_MIN, 0.0f)))
            {
                mSelectedObject = mNatureObjects[i];
                mSelectedFac = mNatureFac.get();
            }
            ImGui::NextColumn();
        }
        ImGui::EndChild();

        ImGui::InputText("Filename", mFnNature, 256);


        if (ImGui::Button("Save Nature"))
            mNatureFac->SaveObjects();

        ImGui::SameLine();

        if (ImGui::Button("Save created Nature"))
            mNatureFac->SaveMarkedObjects("../data/config/" + std::string(mFnNature) + ".fac");

        ImGui::SameLine();

        if (ImGui::Button("Load Nature"))
            mNatureFac->LoadObjects();

        ImGui::PopStyleVar();

        ImGui::TreePop();
    }
}
void Game::itemSpawnMenu()
{
    if (ImGui::TreeNodeEx("Items", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild("ChildR", ImVec2(0, 260), true);
        ImGui::Columns(2);
        for (int i = 0; i < mItemObjects.size(); i++)
        {
            if (ImGui::Button(mItemObjects[i].data(), ImVec2(-FLT_MIN, 0.0f)))
            {
                mSelectedObject = mItemObjects[i];
                mSelectedFac = mItemFac.get();
            }
            ImGui::NextColumn();
        }
        ImGui::EndChild();

        ImGui::InputText("Filename", mFnItems, 256);


        if (ImGui::Button("Save Items"))
            mItemFac->SaveObjects();

        ImGui::SameLine();

        if (ImGui::Button("Save created Items"))
            mItemFac->SaveMarkedObjects("../data/config/" + std::string(mFnItems) + ".fac");

        ImGui::SameLine();

        if (ImGui::Button("Load Items"))
            mItemFac->LoadObjects();

        ImGui::PopStyleVar();

        ImGui::TreePop();
    }
}
void Game::structureSpawnMenu()
{
    if (ImGui::TreeNodeEx("Structures", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild("ChildR", ImVec2(0, 260), true);
        ImGui::Columns(2);
        for (int i = 0; i < mStructureObjects.size(); i++)
        {
            if (ImGui::Button(mStructureObjects[i].data(), ImVec2(-FLT_MIN, 0.0f)))
            {
                mSelectedObject =mStructureObjects[i];
                mSelectedFac = mStructFac.get();
            }

            ImGui::NextColumn();
        }
        ImGui::EndChild();

        ImGui::InputText("Filename", mFnStructs, 256);

        if (ImGui::Button("Save all Structures"))
            mStructFac->SaveObjects();

        ImGui::SameLine();

        if (ImGui::Button("Save created Structures"))
            mStructFac->SaveMarkedObjects("../data/config/" + std::string(mFnStructs) + ".fac");

        ImGui::SameLine();

        if (ImGui::Button("Load Structures"))
            mStructFac->LoadObjects();

        ImGui::PopStyleVar();

        ImGui::TreePop();
    }
}
void Game::unitSpawnMenu()
{
    if (ImGui::TreeNodeEx("Units", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild("ChildR", ImVec2(0, 260), true);
        ImGui::Columns(2);
        for (int i = 0; i < mUnitObjects.size(); i++)
        {
            if (ImGui::Button(mUnitObjects[i].data(), ImVec2(-FLT_MIN, 0.0f)))
            {
                mSelectedObject = mUnitObjects[i];
                mSelectedFac = mUnitFac.get();
            }
            ImGui::NextColumn();
        }
        ImGui::EndChild();

        ImGui::InputText("Filename", mFnUnits, 256);

        if (ImGui::Button("Save all Units"))
            mUnitFac->SaveObjects();

        ImGui::SameLine();

        if (ImGui::Button("Save created Units"))
            mUnitFac->SaveMarkedObjects("../data/config/" + std::string(mFnUnits) + ".fac");

        ImGui::SameLine();

        if (ImGui::Button("Load Units"))
            mUnitFac->LoadObjects();

        ImGui::PopStyleVar();

        ImGui::TreePop();
    }
}

void Game::newGame() 
{
    toggleMenu();
    togglePause();

    // TODO: Initialize a new game
}

void Game::gameUI()
{
    if (mGamelogicSys->GetWood() < 0)
        mGameLost = true;

    if (mGamelogicSys->GetSupplyLevel() >= 3)
        mGameWon = true;

    if (mNewGame)
    {
        ImGui::SetNextWindowPos({(float)getWindowWidth() / 2, (float)getWindowHeight() / 2}, 0, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize({350, 400});
        if (ImGui::Begin("Intro", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            ImGui::TextWrapped("You're the chief of this pioneer camp and are ordered to establish a supply route.");
            ImGui::NewLine();
            ImGui::TextWrapped("Hold out against the evil lurking in the woods - and watch out that the fire never dies.");
            ImGui::NewLine();
            ImGui::TextWrapped("We will send fresh voluntaries every few days. Just in case some of them happen to ... disappear.");
            ImGui::NewLine();

            if (ImGui::Button("I won't disappoint!", {330, 50}))
            {
                mNewGame = false;
                togglePause();
            }
        }
        ImGui::End();
    }
    else if (mGameLost)
    {
        ImGui::SetNextWindowPos({(float)getWindowWidth() / 2, (float)getWindowHeight() / 2}, 0, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize({350, 400});
        if (ImGui::Begin("You Lost!", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            ImGui::TextWrapped("You did your best to defend the camp, but in the end you were overrun.");
            ImGui::NewLine();
            ImGui::TextWrapped("The fire you lit has been extinquished and the forest once again returns to darkness.");
            ImGui::NewLine();
            ImGui::TextWrapped("But maybe others after you will have better luck.");
            ImGui::NewLine();

            if (ImGui::Button("Main Menu", {330, 50}))
            {
                mGameLost = false;
                toggleMenu();
            }

            if (!mPlayedTrack)
            {
                mAudioSys->PlayGlobalSound("lost.wav", gamedev::SoundType::effect, 0.5, false, gamedev::SoundPriority::high);
                mPlayedTrack = true;
            }
        }
        ImGui::End();
    }
    else if (mGameWon)
    {
        ImGui::SetNextWindowPos({(float)getWindowWidth() / 2, (float)getWindowHeight() / 2}, 0, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize({350, 400});
        if (ImGui::Begin("You Won!", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            ImGui::TextWrapped("You have proven yourself to be a capable leader.");
            ImGui::NewLine();
            ImGui::TextWrapped("Not only where you able to fend off the ever increasing attacks of the enemy...");
            ImGui::NewLine();
            ImGui::TextWrapped("...you also managed to establish a supply route that will ensure the survival of the camp.");
            ImGui::NewLine();

            if (ImGui::Button("Begin", {330, 50}))
            {
                mGameWon = false;
                toggleMenu();
            }
        }
        ImGui::End();
    }
    else
    {
        ImGui::SetNextWindowPos({0.f, 0.f}, 0);
        ImGui::SetNextWindowSize({menuWidth, (float)getWindowHeight()});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        if (ImGui::Begin("Governors Book", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            if (ImGui::TreeNodeEx("Records", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
            {
                ImGui::Text("Gold: %i", mGamelogicSys->GetGold());
                ImGui::Text("Firewood: %i", mGamelogicSys->GetWood());
                ImGui::NewLine();

                auto artisans = mGamelogicSys->GetArtisanCount();
                auto soldiers = mGamelogicSys->GetSoldierCount();
                auto total = mGamelogicSys->GetPioneerCount();

                ImGui::Text("Current Pioneers: %i / %i", total, mGamelogicSys->GetMaxPioneers());
                // ImGui::Text("Maximum: %i", mGamelogicSys->GetMaxPioneers());
                ImGui::Text("Unassigned: %i", total - soldiers - artisans);
                ImGui::Text("Artisans: %i", artisans);
                ImGui::Text("Soldiers: %i", soldiers);
                ImGui::NewLine();

                ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("Expenses", ImGuiTreeNodeFlags_Framed))
            {
                // ImGui::Text("Expenses");
                ImGui::Text("Train Artisans: %i g", mGamelogicSys->GetArtisanPrice());
                ImGui::Text("Train Milita: %i g", mGamelogicSys->GetSoldierPrice());
                ImGui::Text("Chop Wood: %i g", mGamelogicSys->GetWoodPrice());
                ImGui::Text("Construct Home: %i g", mGamelogicSys->GetHomePrice());
                ImGui::Text("Construct Artisans Place: %i g", mGamelogicSys->GetProductionPrice());
                auto supplyLevel = mGamelogicSys->GetSupplyLevel();
                if (supplyLevel < 3)
                    ImGui::Text("Supply Route (%i/2): %i g", mGamelogicSys->GetSupplyLevel() + 1, mGamelogicSys->GetSupplyPrice());
                ImGui::Text("Upgrade Fortifications: %i g", mGamelogicSys->GetFortificationPrice());
                ImGui::Text("Repair Fortifications: %i g", mGamelogicSys->GetRepairFortificationsPrice());
                ImGui::Text("Heal Injured: %i g", mGamelogicSys->GetHealPrice() * mGamelogicSys->GetInjured());
                ImGui::NewLine();

                ImGui::TreePop();
            }

            ImGui::Separator();
            ImGui::NewLine();

            if (ImGui::Button("Train Artisan", {(menuWidth - 30) / 2, 50}))
                mGamelogicSys->AssignArtisan();
            ImGui::SameLine();
            if (ImGui::Button("Train Militia", {(menuWidth - 30) / 2, 50}))
                mGamelogicSys->AssignSoldier();

            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();

            if (ImGui::Button("Chop Wood", {menuWidth - 20, 50}))
                mGamelogicSys->AddWood();
            if (ImGui::Button("Construct Home", {menuWidth - 20, 50}))
                mGamelogicSys->ConstructHome();
            if (ImGui::Button("Construct Artisans Place", {menuWidth - 20, 50}))
                mGamelogicSys->UpgradeProduction();
            if (ImGui::Button("Heal Injured", {menuWidth - 20, 50}))
                mGamelogicSys->Heal();

            ImGui::NewLine();
            ImGui::Separator();
            //ImGui::NewLine();
            ImGui::Text("Supply Route Progress");
            if (mGamelogicSys->GetSupplyLevel() == 0)
            {
                if (ImGui::Button("Enclose Route", {menuWidth - 20, 50}))
                    mGamelogicSys->UpgradeSupply();
            }
            else if (mGamelogicSys->GetSupplyLevel() == 1)
            {
                if (ImGui::Button("Fortify Route", {menuWidth - 20, 50}))
                    mGamelogicSys->UpgradeSupply();
            }
            else
            {
                if (ImGui::Button("Man Towers", {menuWidth - 20, 50}))
                    mGamelogicSys->UpgradeSupply();
            }
            

            ImGui::NewLine();
            ImGui::Separator();
            //ImGui::NewLine();

            ImGui::TextUnformatted("Fortifications");
            if (ImGui::Button(mGamelogicSys->GetFortificationUpgrade().data(), {menuWidth - 20, 50}))
                mGamelogicSys->UpgradeFortifications();
            auto repairPrice = mGamelogicSys->GetRepairFortificationsPrice();
            if (repairPrice)
                if (ImGui::Button("Repair", {menuWidth - 20, 50}))
                    mGamelogicSys->RepairFortifications();

            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();

            if (ImGui::Button("Don't press this button  ", {menuWidth - 20, 50}))
                THEBUTTONHASBEENPRESSEDOMG = true;

            if (THEBUTTONHASBEENPRESSEDOMG)
            {
                if (ImGui::Button("Crazy Cheat", {menuWidth - 20, 50}))
                    mGamelogicSys->CRAZY_CHEAT_YOU_WONT_BELIEVE();
            }

            //ImGui::TreePop();
        }
        ImGui::PopStyleVar();
        ImGui::End();
    }
}

void Game::menuUI()
{
    ImGui::SetNextWindowPos({(float)getWindowWidth() / 4, (float)getWindowHeight() / 2}, 0, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize({menuWidth, 450});
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("Main Menu", 0, window_flags | ImGuiWindowFlags_NoTitleBar))
    {
        ImGui::NewLine();
        ImGui::NewLine();

        ImGui::PushFont(font2);
        string title = "Fort Candle";
        float font_size = ImGui::GetFontSize() * title.size() / 2;
        ImGui::SameLine(ImGui::GetWindowSize().x / 2 - font_size + (font_size / 2));

        ImGui::Text(title.c_str());
        ImGui::PopFont();

        ImGui::NewLine();
        ImGui::Separator();
        ImGui::NewLine();

        if (mNewGame)
        {
            if (ImGui::Button("New Game", {menuWidth - 20, 50}))
                newGame();
        }
        else
        {
            if (ImGui::Button("Continue", {menuWidth - 20, 50}))
                toggleMenu();
        }

        if (ImGui::Button("Options", {menuWidth - 20, 50}))
            ImGui::OpenPopup("Game Options");

        ImGui::SetNextWindowPos({(float)getWindowWidth() / 2, (float)getWindowHeight() / 2}, 0, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize({500, 350});
        if (ImGui::BeginPopup("Game Options", window_flags))
        {
            if (ImGui::BeginTabBar("Tabbar"))
            {
                if (ImGui::BeginTabItem("Graphics"))
                {
                    ImGui::NewLine();
                    ImGui::Checkbox("Fullscreen", &mFullscreen);
                    ImGui::NewLine();
                    ImGui::Checkbox("Enable Point Shadows", &mEnablePointShadows);
                    ImGui::Checkbox("Enable Directional Shadows", &mEnableDirShadows);
                    ImGui::Checkbox("Enable Bloom", &mEnableBloom);
                    ImGui::NewLine();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Audio"))
                {
                    ImGui::NewLine();
                    ImGui::SliderFloat("Master", &mMasterVolume, 0.f, 100.f);
                    ImGui::NewLine();
                    ImGui::Separator();
                    ImGui::SliderFloat("Effects", &mEffectVolume, 0.f, 100.f);
                    ImGui::Spacing();
                    ImGui::SliderFloat("Ambient", &mAmbientVolume, 0.f, 100.f);
                    ImGui::Spacing();
                    ImGui::SliderFloat("Music", &mMusicVolume, 0.f, 100.f);
                    ImGui::Spacing();
                    ImGui::SliderFloat("UI", &mUIVolume, 0.f, 100.f);

                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            if (ImGui::Button("Back"))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        if (ImGui::Button("Controls", {menuWidth - 20, 50}))
            ImGui::OpenPopup("Controls");

        ImGui::SetNextWindowPos({(float)getWindowWidth() / 2, (float)getWindowHeight() / 2}, 0, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize({500, 300});
        if (ImGui::BeginPopup("Controls", window_flags))
        {
            ImGui::BeginChild("Child", {450, 250});
            ImGui::Columns(2);
            ImGui::TextUnformatted("WASD");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Move Camera ");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Q/E or Mouseweel");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Zoom");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Press Mouseweel");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Rotate Camera");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Left Mouse Button");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Select Units");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Right Mouse Button");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Move Units");
            ImGui::NextColumn();
            ImGui::TextUnformatted("P");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Pause/Unpause Game");
            ImGui::NextColumn();
            ImGui::TextUnformatted("E");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Toggle Editor Mode");
            ImGui::NextColumn();
            ImGui::TextUnformatted("F11");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Toggle Fullscreen");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Esc");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Open Main Menu");
            ImGui::EndChild();

            if (ImGui::Button("Back"))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        ImGui::NewLine();
        ImGui::Separator();
        ImGui::NewLine();

        if (ImGui::Button("Quit", {menuWidth - 20, 50}))
            ImGui::OpenPopup("Quit?");

        ImGui::SetNextWindowPos({(float)getWindowWidth() / 2, (float)getWindowHeight() / 2}, 0, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopup("Quit?", window_flags))
        {
            ImGui::Text("Really Quit?");
            ImGui::NewLine();
            if (ImGui::Button("Yes", ImVec2(120, 50)))
                GlfwApp::requestClose();
            ImGui::SameLine();
            if (ImGui::Button("No", ImVec2(120, 50)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

bool Game::spawnNature(std::string id)
{
    tg::pos3 pos = mCamera.physical.position + mCamera.physical.forward * 10;
    pos.y = mTerrain->heightAt(pos);

    mNatureFac->Create(id, pos);
    return true;
}
bool Game::spawnItem(std::string id)
{
    tg::pos3 pos = mCamera.physical.position + mCamera.physical.forward * 10;
    pos.y = mTerrain->heightAt(pos);

    mItemFac->Create(id, pos);
    return true;
}
bool Game::spawnStructure(std::string id)
{
    tg::pos3 pos = mCamera.physical.position + mCamera.physical.forward * 10;
    pos.y = mTerrain->heightAt(pos);

    mStructFac->Create(id, pos);
    return true;
}
bool Game::spawnUnit(std::string id)
{
    tg::pos3 pos = mCamera.physical.position + mCamera.physical.forward * 10;
    pos.y = mTerrain->heightAt(pos);

    mUnitFac->Create(id, pos);
    return true;
}

void Game::spawn(tg::pos3 position)
{
    tg::quat rotation(tg::quat::identity);

    if (mECS->IsLiveHandle(mCreatedLast))
    {
        rotation = mECS->GetInstanceTransform(mCreatedLast).rotation;
    }

    if (mSelectedFac && !mSelectedObject.empty())
        if (rotation != tg::quat::identity)
            mCreatedLast = mSelectedFac->Create(mSelectedObject, position, rotation);
        else
            mCreatedLast = mSelectedFac->Create(mSelectedObject, position);

    mSelectedFac->MarkLastCreated();
}

void Game::spawnRandomly(tg::pos3 position)
{
    auto randomScale = tg::size3(gamedev::RandomFloat(0.8, 1.2));
    auto randomRotation = gamedev::RandomFloat() * 360_deg;

    if (mSelectedFac && !mSelectedObject.empty())
        mSelectedFac->Create(mSelectedObject, position, tg::quat::from_axis_angle(tg::dir3::pos_y, randomRotation), randomScale);

    mSelectedFac->MarkLastCreated();
}

void Game::toggleMenu()
{
    if (state != MENU)
    {
        mCamera.target.position = tg::pos3(1.34, 1.83, 24.92);
        mCamera.target.forward = tg::normalize(tg::vec3(-0.49, -0.19, -0.85));

        state = MENU;
    }
    else
    {
        mCamera.target.position = mGamelogicSys->mHome.center + tg::vec3::unit_y * 5.0 - tg::vec3::unit_x * 7.0;
        mCamera.target.forward = tg::normalize(tg::vec3(1.f, -0.7f, 0.f));

        state = PLAY;
    }
}

void Game::applyCustomImguiTheme() 
{
    ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontFromFileTTF("../data/fonts/Calibri.ttf", 20);
    font1 = io.Fonts->AddFontFromFileTTF("../data/fonts/SourceSans3.ttf", 25);
    font2 = io.Fonts->AddFontFromFileTTF("../data/fonts/SheilaCrayon.ttf", 45);

    auto& style = ImGui::GetStyle();
    style.FrameRounding = 4.0f;
    style.WindowBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    style.GrabRounding = 4.0f;
    style.WindowTitleAlign = {0.5f, 0.5f};
    style.WindowPadding = {10, 10};
    style.PopupRounding = 4.0f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.73f, 0.75f, 0.74f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.09f, 0.94f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.84f, 0.66f, 0.66f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.84f, 0.66f, 0.66f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.47f, 0.22f, 0.22f, 0.67f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.47f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.47f, 0.22f, 0.22f, 0.67f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.34f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.71f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.84f, 0.66f, 0.66f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.47f, 0.22f, 0.22f, 0.65f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.71f, 0.39f, 0.39f, 0.65f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
    colors[ImGuiCol_Header] = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.84f, 0.66f, 0.66f, 0.65f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.84f, 0.66f, 0.66f, 0.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
    colors[ImGuiCol_Tab] = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
    colors[ImGuiCol_TabActive] = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
