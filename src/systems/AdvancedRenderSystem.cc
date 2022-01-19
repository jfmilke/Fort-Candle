#include "systems/AdvancedRenderSystem.hh"
#include <glow/fwd.hh>

#include <limits>

#include <typed-geometry/tg.hh>

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
#include <glow/data/TextureData.hh>

#include <glow-extras/geometry/FullscreenTriangle.hh>
#include <glow-extras/geometry/Quad.hh>
#include <glow-extras/geometry/UVSphere.hh>

#include "Mesh3D.hh"

#include "types/AuxiliaryModel.hh"

#include <algorithm>

// System Stuff
// ============
void gamedev::RenderSystem::AddEntity(InstanceHandle& handle, Signature entitySignature)
{
    mEntities.insert(handle);
}

void gamedev::RenderSystem::RemoveEntity(InstanceHandle& handle, Signature entitySignature)
{
    mEntities.erase(handle);
}

void gamedev::RenderSystem::RemoveEntity(InstanceHandle& handle)
{
    mEntities.erase(handle);
}

void gamedev::RenderSystem::RemoveAllEntities()
{
    mEntities.clear();
}

void gamedev::RenderSystem::Init(std::shared_ptr<EngineECS>& ecs)
{
    mECS = ecs;
    
    mMeshQuad = glow::geometry::Quad<>().generate();

    mTexSkybox = glow::TextureCubeMap::createFromData(glow::TextureData::createFromFileCube(
        texPath + "skybox/Left_fhd.png",
        texPath + "skybox/Right_fhd.png",
        texPath + "skybox/Up_fhd.png",
        texPath + "skybox/Down_fhd.png",
        texPath + "skybox/Front_fhd.png",
        texPath + "skybox/Back_fhd.png",
        glow::ColorSpace::sRGB));
}

void gamedev::RenderSystem::InitResources()
{
    CreateGFXResources();
    LoadGFXResources();
}

void gamedev::RenderSystem::InitParticles(glow::SharedVertexArray& vao)
{
    mVaoParticle = vao;
}

int gamedev::RenderSystem::Update(float dt)
{
    auto t0 = std::chrono::steady_clock::now();

    mDrawCalls = 0;

    if (mEnablePointShadows && mEnablePointLight || mEnableDirShadows && mEnableDirLight)
        UpdateShadowMapTexture();

    if (mEnablePointShadows && mEnablePointLight)
        RenderDepthCubemap();

    Clear();

    GLOW_SCOPED(enable, GL_CULL_FACE);
    GLOW_SCOPED(enable, GL_DEPTH_TEST);

    if (mEnableDirShadows && mEnableDirLight)
        RenderDepthMap();

    GLOW_SCOPED(polygonMode, mRenderWireframe ? GL_LINE : GL_FILL);

    if (!mEnableCullFaces)
        glDisable(GL_CULL_FACE);

    {
        {
            auto fb = mFramebufferScene->bind();
            RenderBackground();

            RenderScene();
        }

        {
            if (mEnableParticles)
                RenderParticles();

            {
                GLOW_SCOPED(enable, GL_BLEND);
                GLOW_SCOPED(blendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                if (mECS->IsLiveHandle(mSelected))
                    RenderOutline(mSelected);

                for (int i = 0; i < mOutlineHandles.size(); i++)
                {
                    RenderOutline(mOutlineHandles[i], mOutlineColors[i]);
                }
            }
        }
    }

    // post-process does not need depth test or culling
    GLOW_SCOPED(disable, GL_DEPTH_TEST);
    GLOW_SCOPED(disable, GL_CULL_FACE);

    if (mEnableBloom)
        RenderBloom();

    RenderPostProcessing();

    GLOW_SCOPED(enable, GL_BLEND);
    GLOW_SCOPED(blendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    RenderOutlineResolve();

    auto tn = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(tn - t0).count();
}

// Updating Stuff
// ==============
void gamedev::RenderSystem::UpdateOutlined(const std::vector<InstanceHandle>& handles, const std::vector<tg::vec4>& colors)
{
    mOutlineHandles = handles;
    mOutlineColors = colors;
}

void gamedev::RenderSystem::UpdateSelected(InstanceHandle handle)
{
    mSelected = handle;
}

void gamedev::RenderSystem::UpdateParticleCount(long int particleCount) { mParticleCount = particleCount; }

void gamedev::RenderSystem::UpdateWindowResolution(const tg::isize2& windowDimensions) { mResolution = windowDimensions; }
void gamedev::RenderSystem::UpdateDirShadowResolution(int sqrt_resolution) { mDirShadowResolution = sqrt_resolution; }
void gamedev::RenderSystem::UpdatePointShadowResolution(int sqrt_resolution) { mPointShadowResolution = sqrt_resolution; }

void gamedev::RenderSystem::UpdateAmbientLight(const AmbientLight& ambLight) { mAmbLight = ambLight; };
void gamedev::RenderSystem::UpdateDirectionalLight(const DirectionalLight& dirLight) { mDirLight = dirLight; };
void gamedev::RenderSystem::UpdateRegularPointLights(const std::vector<PointLight>& pointLights) { mPointLights = pointLights; };
void gamedev::RenderSystem::UpdateShadowingPointLight(const PointLight& pointLight) { mShadowingPointLight = pointLight; };

void gamedev::RenderSystem::UpdateSepia(float sepiaStrength)
{
    mSepiaIntensity = sepiaStrength;
}

void gamedev::RenderSystem::UpdateVignette(const tg::vec3& color, float radius, float softness, float strength)
{
    mVignetteColor = color;
    mVignetteRadius = radius;
    mVignetteSoftness = softness;
    mVignetteIntensity = strength;
}

void gamedev::RenderSystem::UpdateDesaturation(float desaturationStrength)
{
    mDesaturationIntensity = desaturationStrength;
}

void gamedev::RenderSystem::UpdateDistFadeoff(const tg::vec3& color, float strength)
{
    mDistFadeoffColor = color;
    mDistFadeoffIntensity = strength;
}

void gamedev::RenderSystem::UpdateTime(float runtime, float gametime)
{
    mRuntime = runtime;
    mGametime = gametime;
}

void gamedev::RenderSystem::UpdateCamera(const tg::pos3& cameraPos, const tg::vec3& cameraForward, const tg::vec3& cameraUp)
{
    auto aspectRatio = float(mResolution.width) / float(mResolution.height);

    mCameraPos = cameraPos;
    mCameraForward = cameraForward;
    mCameraUp = cameraUp;

    mShadowNearPlane = 0.01;
    mShadowFarPlane = 500.0;
    mNearPlane = 0.01;
    mFarPlane = 500.0;

    mProj = tg::perspective_opengl(60_deg, aspectRatio, mNearPlane, mFarPlane);
    mView = tg::look_at_opengl(cameraPos, cameraForward, cameraUp);
}

void gamedev::RenderSystem::UpdateShadowMapTexture()
{
    if (mDirShadowDepthMap && (int)mDirShadowDepthMap->getWidth() == mDirShadowResolution)
        return; // already done

    glow::info() << "Creating " << mDirShadowResolution << " x " << mDirShadowResolution << " shadow map";

    // auto shadowColorMap = glow::Texture2D::createStorageImmutable(mShadowMapResolution, mShadowMapResolution, GL_R8, 1);
    mDirShadowDepthMap = glow::Texture2D::createStorageImmutable(mDirShadowResolution, mDirShadowResolution, GL_DEPTH_COMPONENT32F, 0);
    {
        auto tex = mDirShadowDepthMap->bind();
        // tex.setMinFilter(GL_LINEAR); // no mip-maps
        tex.setFilter(GL_NEAREST, GL_NEAREST);
        // tex.setWrap(GL_REPEAT, GL_REPEAT);
        tex.setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
        tex.setBorderColor({1.0f, 1.0f, 1.0f, 1.0f});

        // set depth compare parameters for shadow mapping
        tex.setCompareMode(GL_COMPARE_REF_TO_TEXTURE);
        tex.setCompareFunc(GL_LESS);
    }
    // add a dummy color target (in hope that some drivers calm their mammaries)
    // mFramebufferDirShadow = glow::Framebuffer::create({{"fColor", shadowColorMap}}, mDirShadowDepthMap);
    mFramebufferDirShadow = glow::Framebuffer::createDepthOnly(mDirShadowDepthMap);
}

// Switching Stuff
// ===============
void gamedev::RenderSystem::SwitchLightShading(bool ambientLight, bool directionalLight, bool pointLight)
{
    mEnableAmbLight = ambientLight;
    mEnableDirLight = directionalLight;
    mEnablePointLight = pointLight;
}

void gamedev::RenderSystem::SwitchDebugShading(bool showNormals, bool showWireframe, bool cullFaces)
{
    mRenderNormals = showNormals;
    mRenderWireframe = showWireframe;
    mEnableCullFaces = cullFaces;
}

void gamedev::RenderSystem::SwitchShadowShading(bool pointShadows, bool dirShadows)
{
    mEnablePointShadows = pointShadows;
    mEnableDirShadows = dirShadows;
}

void gamedev::RenderSystem::SwitchGameShading(bool fogOfWar) { mEnableFogOfWar = fogOfWar; }

void gamedev::RenderSystem::SwitchPostProcessing(bool sRGB, bool tonemap, bool sepia, bool vignette, bool desaturate, bool bloom, bool fadeoff)
{
    mEnableSRGBCurve = sRGB;
    mEnableTonemap = tonemap;
    mEnableSepia = sepia;
    mEnableVignette = vignette;
    mEnableDesaturate = desaturate;
    mEnableBloom = bloom;
    mEnableDistFadeoff = fadeoff;
}

void gamedev::RenderSystem::SwitchParticles(bool particles) { mEnableParticles = particles; }


glow::SharedFramebuffer& gamedev::RenderSystem::GetParticleFramebuffer() { return mFramebufferScene; }

glow::UsedProgram gamedev::RenderSystem::GetParticleShader()
{
    auto shader = mShaderParticle->use();
    shader["uProj"] = mProj;
    shader["uView"] = mView;
    return shader;
}

long int gamedev::RenderSystem::GetDrawCalls() { return mDrawCalls; }

// Rendering Stuff
// ===============
void gamedev::RenderSystem::Clear()
{
    // clear scene targets - depth to 1 (= far plane), scene to background color, picking to -1u (= invalid)
    float const depthVal = 1.f;
    std::uint32_t const pickingVal = std::uint32_t(-1);
    tg::color3 mBackgroundColor = {.10f, .46f, .83f};

    mTargetColor->clear(GL_RGB, GL_FLOAT, tg::data_ptr(mBackgroundColor));
    mTargetBrightColor->clear(GL_RGBA, GL_FLOAT, tg::data_ptr(tg::vec4::zero));
    mTargetDepth->clear(GL_DEPTH_COMPONENT, GL_FLOAT, &depthVal);
    mTargetPicking->clear(GL_RED_INTEGER, GL_UNSIGNED_INT, &pickingVal);
    mTargetPingColor->clear(GL_RGBA, GL_FLOAT, tg::data_ptr(tg::vec4::zero));
    mTargetPongColor->clear(GL_RGBA, GL_FLOAT, tg::data_ptr(tg::vec4::zero));
    mTargetOutlineIntermediate->clear(GL_RGBA, GL_FLOAT, tg::data_ptr(tg::vec4::zero));
}

void gamedev::RenderSystem::RenderInstances(glow::UsedProgram& shader)
{
    for (const auto& handle : mEntities)
    {
        //if (mECS->TestSignature<Arrow>(handle))
        //    continue;

        gamedev::Instance const& instance = mECS->GetInstance(handle);

        // bind the textures
        shader["uTexAlbedo"] = instance.texAlbedo;

        // compute and upload the model matrix
        tg::mat4x3 const modelMatrix = instance.xform.transform_mat();
        shader["uModel"] = modelMatrix;

        // set the instance handle value so it can be written to the picking buffer
        shader["uPickingID"] = handle._value;

        // set the color bias
        shader["uAlbedoBiasPacked"] = instance.albedoBias;

        // bind and draw the mesh
        instance.vao->bind().draw();

        // do the same for auxiliary meshes if existent
        if (!instance.auxModels.empty())
          RenderAuxiliary(shader, instance, modelMatrix);
    }

    mDrawCalls += mEntities.size();
}

void gamedev::RenderSystem::RenderDepth(glow::UsedProgram& shader)
{
    auto world = mECS->GetInstanceManager();

    GLOW_SCOPED(enable, GL_DEPTH_TEST);
    GLOW_SCOPED(enable, GL_CULL_FACE);

    for (const auto& handle : mEntities)
    {
        // if (mECS->TestSignature<Arrow>(handle))
        //     continue;

        gamedev::Instance const& instance = mECS->GetInstance(handle);

        // compute and upload the model matrix
        tg::mat4x3 const modelMatrix = instance.xform.transform_mat();
        shader["uModel"] = modelMatrix;

        // bind and draw the mesh
        instance.vao->bind().draw();
    }

    mDrawCalls += mEntities.size();
}

void gamedev::RenderSystem::RenderAuxiliary(glow::UsedProgram& shader, const Instance& instance, const tg::mat4x3& parentTransform)
{
    for (const auto& model : instance.auxModels)
    {
        glow::info() << "Here should be nothing!";

        // bind the textures
        shader["uTexAlbedo"] = model.texAlbedo;

        // compute and upload the model matrix
        tg::mat4x3 const modelMatrix = parentTransform * tg::mat4(model.xform.transform_mat());
        shader["uModel"] = modelMatrix;

        // set the color bias
        shader["uAlbedoBiasPacked"] = model.albedoBias;

        // bind and draw the mesh
        model.vao->bind().draw();
    }

    mDrawCalls += instance.auxModels.size();
}

void gamedev::RenderSystem::RenderBackground()
{
    auto shader = mShaderBackground->use();

    shader["uProj"] = mProj;
    shader["uView"] = mView;
    shader["uInvProj"] = tg::inverse(mProj);
    shader["uInvView"] = tg::inverse(mView);
    shader["uRunTime"] = mRuntime;

    shader.setTexture("uTexCubeMap", mTexSkybox);

    GLOW_SCOPED(disable, GL_DEPTH_TEST);
    GLOW_SCOPED(disable, GL_CULL_FACE);

    mMeshQuad->bind().draw();

    mDrawCalls++;
}

void gamedev::RenderSystem::RenderBloom()
{
    // Create Bloom effect
    {
        auto shader = mShaderBlur->use();
        // Start with the texture holding only the bright parts of the image
        shader["uTexImage"] = mTargetBrightColor;
        // Blur multiple times, the more, the bloomiger
        int blur_passes = 5;
        for (int i = 0; i < blur_passes; i++)
        {
            // Alternate between two framebuffers for each pass
            {
                bool horizontal_pass = true;
                auto fb = mFramebufferPing->bind();
                shader["horizontal_pass"] = horizontal_pass;
                if (i > 0)
                    shader["uTexImage"] = mTargetPongColor;

                glow::geometry::FullscreenTriangle::draw();
                
                mDrawCalls++;
            }

            // Alternate between two framebuffers for each pass
            {
                bool horizontal_pass = false;
                auto fb = mFramebufferPong->bind();
                shader["horizontal_pass"] = horizontal_pass;
                shader["uTexImage"] = mTargetPingColor;

                glow::geometry::FullscreenTriangle::draw();

                mDrawCalls++;
            }
        }
    }
}

void gamedev::RenderSystem::RenderPostProcessing()
{
    // draw a fullscreen triangle for outputting the framebuffer and applying a post-process
    {
        auto shader = mShaderOutput->use();

        shader["uTexColor"] = mTargetColor;
        shader["uTexBright"] = mTargetBrightColor;
        shader["uTexBloom"] = mTargetPongColor;
        shader["uTexDepth"] = mTargetDepth;
        shader["uEnableTonemap"] = mEnableTonemap;
        shader["uEnableGamma"] = mEnableSRGBCurve;
        shader["uShowPostProcess"] = mEnablePostProcess;
        shader["uNear"] = mNearPlane;
        shader["uFar"] = mFarPlane;
        shader["uInvProj"] = tg::inverse(mProj);
        shader["uInvView"] = tg::inverse(mView);
        shader["uCamPos"] = mCameraPos;
        shader["uEnableSepia"] = mEnableSepia;
        shader["uEnableVignette"] = mEnableVignette;
        shader["uEnableDesaturate"] = mEnableDesaturate;
        shader["uEnableDistFade"] = mVignetteIntensity;
        shader["uDesaturationStrength"] = mDesaturationIntensity;
        shader["uSepiaStrength"] = mSepiaIntensity;
        shader["uVignetteColor"] = mVignetteColor;
        shader["uVignetteRadius"] = mVignetteRadius;
        shader["uVignetteSoftness"] = mVignetteSoftness;
        shader["uVignetteStrength"] = mVignetteIntensity;
        shader["uDistFadeColor"] = mDistFadeoffColor;
        shader["uDistFadeStrength"] = mDistFadeoffIntensity;


        /*shader["uPointShadowMap"] = mPointShadowDepthMap;
        shader["uDirShadowMap"] = mDirShadowDepthMap;*/


        glow::geometry::FullscreenTriangle::draw();

        mDrawCalls++;
    }
}

void gamedev::RenderSystem::RenderOutlineResolve()
{
    auto shader = mShaderOutlineResolve->use();

    shader["uTexColor"] = mTargetColor;
    shader["uTexOutline"] = mTargetOutlineIntermediate;

    glow::geometry::FullscreenTriangle::draw();

    mDrawCalls++;
}

void gamedev::RenderSystem::ResizeRenderTargets(int w, int h)
{
    // resize all framebuffer textures
    for (auto const& t : mTargets)
        t->bind().resize(w, h);
}

gamedev::InstanceHandle gamedev::RenderSystem::ReadPickingBuffer(int x, int y)
{
    // note: this is naive, efficient readback in OpenGL should be done with a PBO and n-buffering
    uint32_t readValue = uint32_t(-1);
    {
        auto fb = mFramebufferReadback->bind();
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &readValue);
    }
    return {readValue};
}

tg::pos3 gamedev::RenderSystem::MouseToWorld(int x, int y)
{
    auto const resolution = mResolution;

    tg::vec4 p;
    p.x = x / float(resolution.width) * 2 - 1;
    p.y = y / float(resolution.height) * 2 - 1;
    p.w = 1.0;
    {
        auto fb = mFramebufferScene->bind();
        glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &p.z);
    }
    p.z = 2 * p.z - 1;


    p = tg::inverse(mProj) * p;
    p /= p.w;
    p = tg::inverse(mView) * p;

    return tg::pos3(p);
}

void gamedev::RenderSystem::RenderDepthCubemap()
{
    float shadowNear = 0.01f;
    float shadowFar = 500.f;

    const tg::pos3& lightPos = mShadowingPointLight.position;

    // create depth cubemap transformation matrices
    float const shadowWidth = mPointShadowResolution, shadowHeight = mPointShadowResolution;
    tg::mat4 shadowProj = tg::perspective_opengl(90_deg, shadowWidth / (float)shadowHeight, shadowNear, shadowFar);
    std::vector<tg::mat4> shadowTransforms;

    shadowTransforms.push_back(shadowProj * tg::look_at_opengl(lightPos, lightPos + tg::vec3(1.0f, 0.0f, 0.0f), tg::vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(shadowProj * tg::look_at_opengl(lightPos, lightPos + tg::vec3(-1.0f, 0.0f, 0.0f), tg::vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(shadowProj * tg::look_at_opengl(lightPos, lightPos + tg::vec3(0.0f, 1.0f, 0.0f), tg::vec3(0.0f, 0.0f, 1.0f)));
    shadowTransforms.push_back(shadowProj * tg::look_at_opengl(lightPos, lightPos + tg::vec3(0.0f, -1.0f, 0.0f), tg::vec3(0.0f, 0.0f, -1.0f)));
    shadowTransforms.push_back(shadowProj * tg::look_at_opengl(lightPos, lightPos + tg::vec3(0.0f, 0.0f, 1.0f), tg::vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(shadowProj * tg::look_at_opengl(lightPos, lightPos + tg::vec3(0.0f, 0.0f, -1.0f), tg::vec3(0.0f, -1.0f, 0.0f)));

    {
        // Render Cubemap
        float const depthVal = 1.f;
        mPointShadowDepthMap->clear(GL_DEPTH_COMPONENT, GL_FLOAT, &depthVal);

        {
            auto fb = mFramebufferPointShadow->bind();
            auto shader = mShaderDepth->use();

            shader["uFar"] = shadowFar;
            shader["uLightPos"] = lightPos;

            for (int i = 0; i < 6; i++)
            {
                fb.attachDepth(mPointShadowDepthMap, 0, i);
                shader["uShadowMatrix"] = shadowTransforms[i];

                RenderDepth(shader);
            }
        }
    }
}

void gamedev::RenderSystem::RenderDepthMap()
{
    {
        //float const depthVal = 1.f;
        //mDirShadowDepthMap->clear(GL_DEPTH_COMPONENT, GL_FLOAT, &depthVal);

        auto fb_shadow = mFramebufferDirShadow->bind();
        glClear(GL_DEPTH_BUFFER_BIT);

        auto shader = mShaderPoly->use();
        SetupSceneShader(shader);

        // set up shadow camera
        auto shadowCamDis = 12 * mCameraPos.y; // render far enough
        auto shadowSize = 6 * mCameraPos.y; // render wide enough

        // calculate center of camera rotation
        auto angle = tg::angle_between(mCameraForward, tg::vec3(0, -1, 0));
        auto distance = tan(angle) * mCameraPos.y;

        auto focus = mCameraPos + normalize(mCameraForward) * hypot(mCameraPos.y, distance);

        // place shadowmap more or less in center of visible area
        auto sun = mDirLight;
        auto shadowDir = focus + 4 * length(mCameraPos - focus) * sun.direction;
        auto shadowPos = shadowDir + sun.direction;

        auto shadowview = tg::look_at_opengl(shadowPos, shadowDir, {0, 1, 0});
        auto shadowproj = tg::orthographic(-shadowSize, shadowSize, -shadowSize, shadowSize, 0.1f, shadowCamDis);

        shader["uProj"] = shadowproj;
        shader["uView"] = shadowview;
        shader["uShadowViewProj"] = shadowproj * shadowview;
        shader["uShadowsPass"] = true;

        shader["uFar"] = mShadowFarPlane;

        shader.setTexture("uDirShadowMap", mDirShadowDepthMap);
        shader.setTexture("uPointShadowMap", mPointShadowDepthMap);

        GLOW_SCOPED(cullFace, GL_FRONT);

        //glCullFace(GL_FRONT);
        RenderInstances(shader);
        //glCullFace(GL_BACK);
    }
}

void gamedev::RenderSystem::RenderScene()
{
    auto shader = mShaderPoly->use();

    SetupSceneShader(shader);


    shader["uProj"] = mProj;
    shader["uView"] = mView;
    shader["uShadowsPass"] = false;

    shader.setTexture("uDirShadowMap", mDirShadowDepthMap);
    shader.setTexture("uPointShadowMap", mPointShadowDepthMap);

    RenderInstances(shader);
}

void gamedev::RenderSystem::RenderOutline(InstanceHandle handle) { RenderOutline(handle, tg::vec4(1.f, 0.55f, 0.f, 1.f)); }

void gamedev::RenderSystem::RenderOutline(InstanceHandle handle, const tg::vec4& color)
{
    if (!mECS->IsLiveHandle(handle))
        return;

    // tg::vec4 clear = {0, 0, 0, 0};
    // mTargetOutlineIntermediate->clear(GL_RGBA, GL_FLOAT, tg::data_ptr(clear));

    auto fb = mFramebufferOutline->bind();

    // enable depth test and backface culling for this scope
    // the GLOW_SCOPED macro stores the current state and restores it at the end of the scope
    GLOW_SCOPED(enable, GL_DEPTH_TEST);
    GLOW_SCOPED(enable, GL_CULL_FACE);

    GLOW_SCOPED(depthFunc, GL_EQUAL);


    auto shader = mShaderOutlineForward->use();
    shader["uProj"] = mProj;
    shader["uView"] = mView;

    auto const& instance = mECS->GetInstance(handle);

    tg::mat4x3 const modelMatrix = instance.xform.transform_mat();
    shader["uModel"] = modelMatrix;
    shader["uColor"] = color;

    instance.vao->bind().draw();

    mDrawCalls++;
}

void gamedev::RenderSystem::RenderParticles()
{
    auto fb = mFramebufferScene->bind();
    auto shader = mShaderParticle->use();

    shader["uProj"] = mProj;
    shader["uView"] = mView;

    mVaoParticle->bind().draw(mParticleCount);

    mDrawCalls++;
}


// Setting Stuff Up
// ================
void gamedev::RenderSystem::SetupSceneShader(glow::UsedProgram& shader)
{
    float metalness = 0.0;
    float reflectivity = 0.3;

    // Common uniforms
    shader["uRuntime"] = mRuntime;
    shader["uGametime"] = mGametime;
    shader["uCamPos"] = mCameraPos;

    // todo: Maybe make this per model
    shader["uMetalness"] = metalness;
    shader["uReflectivity"] = reflectivity;

    // Lights
    // -> Ambient
    shader["uAmbientLight"] = mAmbLight.color;
    // -> Sunlight
    auto sun = mDirLight;
    shader["uSunLight.direction"] = sun.direction;
    shader["uSunLight.color"] = 1.0 * sun.color;
    // -> Real PointLights
    mPointLightUBO->bind().setData(mPointLights);
    shader["uPointLightCount"] = int(mPointLights.size());
    // -> Shadowing PointLight
    shader["uLightPos"] = mShadowingPointLight.position;
    shader["uLightColor"] = mShadowingPointLight.color;
    shader["uLightIntensity"] = mShadowingPointLight.intensity;
    shader["uLightRadius"] = mShadowingPointLight.radius;

    shader["uEnableAmbientL"] = mEnableAmbLight;
    shader["uEnableDirectionalL"] = mEnableDirLight;
    shader["uEnablePointL"] = mEnablePointLight;

    // Shadows
    // -> Sunlight Shadows
    shader["uDirShadowMap"] = mDirShadowDepthMap;

    // Render options
    shader["uRenderNormals"] = mRenderNormals;
    shader["uFogOfWar"] = mEnableFogOfWar;

    shader["uPointShadows"] = mEnablePointShadows;
    shader["uDirShadows"] = mEnableDirShadows;
}

void gamedev::RenderSystem::CreateGFXResources()
{
    {
        // create render targets
        // size is 1x1 for now and is changed onResize
        mTargets.push_back(mTargetColor = glow::TextureRectangle::create(1, 1, GL_R11F_G11F_B10F));
        mTargets.push_back(mTargetBrightColor = glow::TextureRectangle::create(1, 1, GL_RGBA16F));
        mTargets.push_back(mTargetPicking = glow::TextureRectangle::create(1, 1, GL_R32UI));
        mTargets.push_back(mTargetDepth = glow::TextureRectangle::create(1, 1, GL_DEPTH_COMPONENT32F));
        mTargets.push_back(mTargetOutlineIntermediate = glow::TextureRectangle::create(1, 1, GL_RGBA8));
        mTargets.push_back(mTargetPingColor = glow::TextureRectangle::create(1, 1, GL_RGBA16F));
        mTargets.push_back(mTargetPongColor = glow::TextureRectangle::create(1, 1, GL_RGBA16F));

        // create framebuffer for shadows
        mFramebufferPointShadow = glow::Framebuffer::create();
        {
            mPointShadowDepthMap = glow::TextureCubeMap::create(mPointShadowResolution, mPointShadowResolution, GL_DEPTH_COMPONENT32F);
            {
                auto boundSm = mPointShadowDepthMap->bind();
                for (int i = 0; i < 6; i++)
                    boundSm.setData(GL_DEPTH_COMPONENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mPointShadowResolution, mPointShadowResolution,
                                    GL_DEPTH_COMPONENT, GL_FLOAT, NULL, 0);
                boundSm.setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
                boundSm.setFilter(GL_NEAREST, GL_NEAREST);
            }
            auto boundFb = mFramebufferPointShadow->bind();
            boundFb.attachDepth(mPointShadowDepthMap);
            boundFb.checkComplete();
        }

        // create framebuffer for HDR scene
        mFramebufferScene = glow::Framebuffer::create();
        {
            auto boundFb = mFramebufferScene->bind();
            boundFb.attachColor("fColor", mTargetColor);
            boundFb.attachColor("fBright", mTargetBrightColor);
            boundFb.attachColor("fPickID", mTargetPicking);
            boundFb.attachDepth(mTargetDepth);
            boundFb.checkComplete();
        }

        // create framebuffer for the outline (depth tested)
        mFramebufferOutline = glow::Framebuffer::create("fColor", mTargetOutlineIntermediate, mTargetDepth);

        // create framebuffer specifically for reading back the picking buffer
        mFramebufferReadback = glow::Framebuffer::create("fPickID", mTargetPicking);

        mFramebufferPing = glow::Framebuffer::create("fColor", mTargetPingColor);
        mFramebufferPong = glow::Framebuffer::create("fColor", mTargetPongColor);
    }
}

void gamedev::RenderSystem::LoadGFXResources()
{
    // load gfx resources
    {
        // color textures are usually sRGB and data textures (like normal maps) Linear
        mTexAlbedo = glow::Texture2D::createFromFile("../data/textures/concrete.albedo.jpg", glow::ColorSpace::sRGB);
        mTexNormal = glow::Texture2D::createFromFile("../data/textures/concrete.normal.jpg", glow::ColorSpace::Linear);
        mTexARM = glow::Texture2D::createFromFile("../data/textures/concrete.arm.jpg", glow::ColorSpace::Linear);

        // UV sphere
        mVaoSphere = glow::geometry::make_uv_sphere(64, 32);

        // cube.obj contains a cube with normals, tangents, and texture coordinates
        Mesh3D cubeMesh;
        cubeMesh.loadFromFile("../data/meshes/cube.obj", false /* do not interpolate tangents for cubes */);
        mVaoCube = cubeMesh.createVertexArray(); // upload to gpu

        // A test object
        Mesh3D testMesh;
        testMesh.loadFromFile("../data/meshes/extern/nature/pine_tree_mystic_2.obj", false);
        mVaoTest = testMesh.createVertexArray();
        mTexTest = glow::Texture2D::createFromFile("../data/textures/BigPalette.png", glow::ColorSpace::sRGB);

        // automatically takes .fsh and .vsh shaders and combines them into a program
        mShaderObject = glow::Program::createFromFile("../data/shaders/object");
        mShaderObjectSimple = glow::Program::createFromFiles({"../data/shaders/object.vsh", "../data/shaders/object_simple.fsh"});
        mShaderOutput = glow::Program::createFromFiles({"../data/shaders/fullscreen_tri.vsh", "../data/shaders/output.fsh"});
        mShaderOutlineForward = glow::Program::createFromFile("../data/shaders/outline_forward");
        mShaderOutlineResolve = glow::Program::createFromFiles({"../data/shaders/fullscreen_tri.vsh", "../data/shaders/outline_resolve.fsh"});

        // Custom Shader
        mShaderPoly = glow::Program::createFromFile("../data/shaders/poly");
        mShaderBackground = glow::Program::createFromFile("../data/shaders/background");
        mShaderParticle = glow::Program::createFromFile("../data/shaders/particles");
        mShaderBlur = glow::Program::createFromFiles({"../data/shaders/fullscreen_tri.vsh", "../data/shaders/blur.fsh"});
        // mShaderSSS = glow::Program::createFromFiles({"../data/shaders/shadows_screenspace.vsh", "../data/shaders/shadows_screenspace.fsh"});

        // mShaderDepth = glow::Program::createFromFiles({"../data/shaders/depth.vsh", "../data/shaders/depth.fsh", "../data/shaders/depth.gsh"});
        mShaderDepth = glow::Program::createFromFiles({"../data/shaders/depth.vsh", "../data/shaders/depth.fsh"});

        // Light UBO
        mPointLightUBO = glow::UniformBuffer::create();
        mShaderPoly->setUniformBuffer("ubPointLights", mPointLightUBO);

        glow::geometry::FullscreenTriangle::init();
    }
}