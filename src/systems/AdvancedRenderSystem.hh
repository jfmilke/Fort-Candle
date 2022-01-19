#pragma once

#include "advanced/World.hh"
#include "ecs/System.hh"
#include "ecs/Engine.hh"
#include "types/Light.hh"

/***
* Tests for 2D collisions between CollisionShapes or tg::box2, tg::circle2 and tg::aabb2.
* 
* If a CollisionShape is tested it applies the transformations of the Instance first.
* Then it calls an appropriate method of a tg object.
* 
* If a tg object method is called, it is assumed, that all transformations have been applied to the object.
* 
***/

namespace gamedev
{
class RenderSystem : public System
{
public:
    void Init(std::shared_ptr<EngineECS>& ecs);

    void AddEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle, Signature entitySignature);
    void RemoveEntity(InstanceHandle& handle);
    void RemoveAllEntities();

    int Update(float dt);

    void InitResources();
    void InitParticles(glow::SharedVertexArray& vao);

    void ResizeRenderTargets(int w, int h);
    InstanceHandle ReadPickingBuffer(int x, int y);
    tg::pos3 MouseToWorld(int x, int y);
    glow::UsedProgram GetParticleShader();
    glow::SharedFramebuffer& GetParticleFramebuffer();

    long int GetDrawCalls();

    void UpdateTime(float runtime, float gametime);
    void UpdateCamera(const tg::pos3& cameraPos, const tg::vec3& cameraForward, const tg::vec3& cameraUp);
    void UpdateWindowResolution(const tg::isize2& windowDimensions);
    void UpdateDirShadowResolution(int sqrt_resolution);
    void UpdatePointShadowResolution(int sqrt_resolution);
    void UpdateAmbientLight(const AmbientLight& ambLight);
    void UpdateDirectionalLight(const DirectionalLight& dirLight);
    void UpdateRegularPointLights(const std::vector<PointLight>& pointLight);
    void UpdateShadowingPointLight(const PointLight& pointLight);
    void UpdateSepia(float strength);
    void UpdateVignette(const tg::vec3& color, float radius, float softness, float strength);
    void UpdateDesaturation(float strength);
    void UpdateDistFadeoff(const tg::vec3& color, float strength);
    void UpdateParticleCount(long int particleCount);
    void UpdateSelected(InstanceHandle handles);
    void UpdateOutlined(const std::vector<InstanceHandle>& handles, const std::vector<tg::vec4>& colors);

    void SwitchDebugShading(bool showNormals, bool showWireframe, bool cullFaces);
    void SwitchLightShading(bool ambientLight, bool directionalLight, bool pointLight);
    void SwitchShadowShading(bool pointShadows, bool dirShadows);
    void SwitchGameShading(bool fogOfWar);
    void SwitchPostProcessing(bool sRGB, bool tonemap, bool sepia, bool vignette, bool desaturate, bool bloom, bool fadeoff);
    void SwitchParticles(bool particles);

    void Clear();

    void RenderScene();
    void RenderPostProcessing();

    void RenderDepthMap();
    void RenderDepthCubemap();

    void RenderOutline(InstanceHandle handle);
    void RenderOutline(InstanceHandle handle, const tg::vec4& color);
    void RenderOutlineResolve();

    // void RenderTerrain(glow::UsedProgram& shader);
    void RenderInstances(glow::UsedProgram& shader);
    void RenderBackground();
    void RenderParticles();

    void RenderBloom();
	  void RenderDepth(glow::UsedProgram& shader);


private:
    // Renders all submeshes of a model.
    // Not the best approach, but introduced to keep the initial structure
    void RenderAuxiliary(glow::UsedProgram& shader, const Instance& instance, const tg::mat4x3& parentTransform);

private:
    void CreateGFXResources();
    void LoadGFXResources();

    void UpdateShadowMapTexture();
    void SetupSceneShader(glow::UsedProgram& program);

// ECS Specifics
private:
    std::shared_ptr<EngineECS> mECS;
    long int mDrawCalls = 0;

private:
    // Debug
    bool mRenderNormals;
    bool mRenderWireframe;
    bool mEnableCullFaces;

    // Shadows
    bool mEnablePointShadows;
    bool mEnableDirShadows;

    // Lighting
    bool mEnablePointLight;
    bool mEnableDirLight;
    bool mEnableAmbLight;

    // Gamelogic
    bool mEnableFogOfWar;

    // FFX
    bool mEnablePostProcess;
    bool mEnableTonemap;
    bool mEnableSRGBCurve;
    bool mEnableSepia;
    bool mEnableVignette;
    bool mEnableDesaturate;
    bool mEnableBloom;
    bool mEnableDistFadeoff;

    // Particles
    bool mEnableParticles;

private:
    // Camera
    tg::pos3 mCameraPos;
    tg::vec3 mCameraForward;
    tg::vec3 mCameraUp;
    tg::isize2 mResolution;
    float mNearPlane;
    float mFarPlane;
    tg::mat4 mProj;
    tg::mat4 mView;

    // Time
    float mRuntime;
    float mGametime;

    // Shadows
    int mPointShadowResolution;
    int mDirShadowResolution;

    float mShadowNearPlane;
    float mShadowFarPlane;

    // Lighting
    AmbientLight mAmbLight;
    DirectionalLight mDirLight;
    std::vector<PointLight> mPointLights;
    PointLight mShadowingPointLight;

    // Gamelogic
    InstanceHandle mSelected;

    // FFX
    float mDesaturationIntensity;
    float mSepiaIntensity;
    float mVignetteIntensity;
    float mVignetteRadius;
    float mVignetteSoftness;
    tg::vec3 mVignetteColor;
    float mDistFadeoffIntensity;
    tg::vec3 mDistFadeoffColor;

    // Particles
    long int mParticleCount;

    // Selection
    std::vector<InstanceHandle> mOutlineHandles;
    std::vector<tg::vec4> mOutlineColors;

    // Paths
private:
    std::string texPath = "../data/textures/";

// Shaders
private:
    // shaders
    glow::SharedProgram mShaderOutput;
    glow::SharedProgram mShaderObject;
    glow::SharedProgram mShaderObjectSimple;
    glow::SharedProgram mShaderOutlineForward;
    glow::SharedProgram mShaderOutlineResolve;

    // Custom Low-Poly Shader
    glow::SharedProgram mShaderPoly;
    glow::SharedProgram mShaderBackground;
    glow::SharedProgram mShaderParticle;
    glow::SharedProgram mShaderBlur;
    glow::SharedProgram mShaderDepth;

// Render Targets
private:
    // intermediate framebuffer with color and depth texture
    glow::SharedFramebuffer mFramebufferScene;
    glow::SharedTextureRectangle mTargetColor;
    glow::SharedTextureRectangle mTargetBrightColor;
    glow::SharedTextureRectangle mTargetPicking;
    glow::SharedTextureRectangle mTargetDepth;

    glow::SharedFramebuffer mFramebufferOutline;
    glow::SharedTextureRectangle mTargetOutlineIntermediate;

    glow::SharedFramebuffer mFramebufferReadback;

    // Two framebuffers to create bloom
    glow::SharedFramebuffer mFramebufferPing;
    glow::SharedTextureRectangle mTargetPingColor;

    glow::SharedFramebuffer mFramebufferPong;
    glow::SharedTextureRectangle mTargetPongColor;

    glow::SharedFramebuffer mFramebufferPointShadow;
    glow::SharedTextureCubeMap mPointShadowDepthMap;
    glow::SharedFramebuffer mFramebufferDirShadow;
    glow::SharedTexture2D mDirShadowDepthMap;

    std::vector<glow::SharedTextureRectangle> mTargets;

// Render Resources
private:
    glow::SharedTextureCubeMap mTexSkybox;
    glow::SharedVertexArray mMeshQuad;

    // meshes (as vertex array obejects)
    glow::SharedVertexArray mVaoParticle;
    glow::SharedVertexArray mVaoCube;
    glow::SharedVertexArray mVaoSphere;
    glow::SharedVertexArray mVaoTest;

    // textures
    glow::SharedTexture2D mTexAlbedo;
    glow::SharedTexture2D mTexNormal;
    glow::SharedTexture2D mTexARM;
    glow::SharedTexture2D mTexTest;

    // Lights
    glow::SharedUniformBuffer mPointLightUBO;
};
}

