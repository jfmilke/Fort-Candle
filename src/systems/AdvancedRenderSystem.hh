#pragma once

#include "advanced/World.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"
#include "types/Light.hh"

namespace gamedev
{
/**
 * Forward renderer for all instances with a `Render` component.
 *
 * `Game::render()` pushes the per-frame state (camera, lights, effect parameters, selection, outlines)
 * through the `Update*` / `Switch*` functions and then calls `Update`, which renders one frame:
 *
 *  1. Shadows: point-light shadow cube map (`depth` shader) and directional shadow map
 *     (`depth_dir` shader, orthographic, fitted to the visible ground, see `RenderDepthMap`).
 *  2. Scene: skybox (`background`) and all instances (`poly`) into the HDR scene framebuffer (color, bright parts, picking IDs, depth).
 *  3. Particles (`particles`) and outlines of the selected/highlighted instances (`outline_forward`).
 *  4. Bloom (5 ping-pong `blur` passes).
 *  5. Post-processing to the screen (`output`: tonemap, gamma, sepia, vignette, desaturation, distance fade).
 *  6. Outline resolve (`outline_resolve`).
 *
 * Shaders are loaded from `data/shaders/` by `InitResources`.
 * The picking buffer stores the instance handle per pixel for mouse selection.
 * Needs an OpenGL context.
 */
class RenderSystem : public System
{
public:
    /**
     * Stores the ECS and loads the quad and skybox.
     *
     * @param ecs The ECS; must outlive this object.
     */
    void Init(EngineECS& ecs);

    /**
     * Renders one frame (see class description).
     *
     * @param dt Unused.
     */
    void Update(float dt);

    /**
     * Creates render targets and framebuffers and loads textures, meshes and shaders.
     *
     * Set the shadow resolutions before (the point shadow cube map keeps the resolution it is created with).
     */
    void InitResources();

    /// Sets the particle VAO (cube with per-instance attributes, from the `ParticleSystem`).
    void InitParticles(glow::SharedVertexArray& vao);

    /**
     * Resizes all screen-sized render targets.
     *
     * @param w Width in pixels.
     * @param h Height in pixels.
     */
    void ResizeRenderTargets(int w, int h);

    /**
     * Reads the picking buffer of the last frame at one pixel.
     *
     * @param x Pixel column, from the left.
     * @param y Pixel row, from the bottom.
     * @return Instance rendered at the pixel, or an invalid handle.
     */
    InstanceHandle ReadPickingBuffer(int x, int y);

    /**
     * Unprojects a pixel with the scene depth buffer of the last frame.
     *
     * @param x Pixel column, from the left.
     * @param y Pixel row, from the bottom.
     * @return World position of the surface at the pixel.
     */
    tg::pos3 MouseToWorld(int x, int y);

    /// Binds the particle shader with the camera matrices and returns it.
    glow::UsedProgram GetParticleShader();

    /// Framebuffer particles are drawn into (the scene framebuffer).
    glow::SharedFramebuffer& GetParticleFramebuffer();

    /// Number of draw calls of the last frame.
    long int GetDrawCalls();

    /// Sets the real time and the game time of day (seconds) for animated shaders.
    void UpdateTime(float runtime, float gametime);

    /**
     * Sets the camera and recomputes view and projection (60° vertical field of view, near 0.01, far 500).
     *
     * Uses the aspect ratio of the window size from `UpdateWindowResolution`.
     *
     * @param cameraPos Camera position in world space.
     * @param cameraForward View direction.
     * @param cameraUp Up vector.
     */
    void UpdateCamera(const tg::pos3& cameraPos, const tg::vec3& cameraForward, const tg::vec3& cameraUp);

    /// Sets the window size in pixels (used for the aspect ratio and `MouseToWorld`).
    void UpdateWindowResolution(const tg::isize2& windowDimensions);

    /// Sets the edge length of the directional shadow map; it is recreated on the next frame if changed.
    void UpdateDirShadowResolution(int sqrt_resolution);

    /// Sets the edge length of the point shadow cube map faces (only used when the cube map is created).
    void UpdatePointShadowResolution(int sqrt_resolution);

    /// Sets the ambient light.
    void UpdateAmbientLight(const AmbientLight& ambLight);

    /// Sets the sun.
    void UpdateDirectionalLight(const DirectionalLight& dirLight);

    /// Sets the regular point lights (uploaded to the `ubPointLights` uniform buffer).
    void UpdateRegularPointLights(const std::vector<PointLight>& pointLight);

    /// Sets the shadow-casting point light.
    void UpdateShadowingPointLight(const PointLight& pointLight);

    /// Sets the sepia strength [0, 1].
    void UpdateSepia(float strength);

    /// Sets the vignette color, radius, softness and strength (screen-space fractions).
    void UpdateVignette(const tg::vec3& color, float radius, float softness, float strength);

    /// Sets the desaturation strength [0, 1].
    void UpdateDesaturation(float strength);

    /// Sets the color and strength of the fade-out with distance from the camera.
    void UpdateDistFadeoff(const tg::vec3& color, float strength);

    /// Sets the number of particles to draw.
    void UpdateParticleCount(long int particleCount);

    /// Sets the selected instance, outlined in orange.
    void UpdateSelected(InstanceHandle handles);

    /// Sets additional instances to outline, each with its RGBA color (both vectors must have the same size).
    void UpdateOutlined(const std::vector<InstanceHandle>& handles, const std::vector<tg::vec4>& colors);

    /// Debug switches: show normals, wireframe, back-face culling.
    void SwitchDebugShading(bool showNormals, bool showWireframe, bool cullFaces);

    /// Enables the ambient, directional and point lights.
    void SwitchLightShading(bool ambientLight, bool directionalLight, bool pointLight);

    /// Enables the point and directional shadows.
    void SwitchShadowShading(bool pointShadows, bool dirShadows);

    /// Enables the fog of war in the scene shader.
    void SwitchGameShading(bool fogOfWar);

    /// Enables the post-processing effects.
    void SwitchPostProcessing(bool sRGB, bool tonemap, bool sepia, bool vignette, bool desaturate, bool bloom, bool fadeoff);

    /// Enables drawing particles.
    void SwitchParticles(bool particles);

    /// Clears all scene render targets (color to the background color, depth to 1, picking to invalid).
    void Clear();

    /// Draws all instances with the `poly` shader into the bound scene framebuffer.
    void RenderScene();

    /// Combines scene, bloom and effects into the default framebuffer (`output` shader).
    void RenderPostProcessing();

    /**
     * Fits the sun's orthographic shadow camera to the visible ground and renders the directional shadow map.
     *
     * The map covers the smallest circle around the ground the camera sees (view rays clipped to the ground plane y = -1
     * and 150 units of view depth), extruded up to y = 8 for receivers; casters up to y = 24 are rendered.
     * The circle is stable when the camera pans or turns, its radius is quantized, and the map is moved in whole texels,
     * so shadow edges do not flicker while the camera moves.
     * Assumes that the play area is around y = 0.
     */
    void RenderDepthMap();

    /// Renders the six faces of the point shadow cube map around the shadow-casting light.
    void RenderDepthCubemap();

    /**
     * Outlines an instance in the default selection color (orange).
     *
     * @param handle Instance to outline; nothing happens if it is not live.
     */
    void RenderOutline(InstanceHandle handle);

    /**
     * Draws an instance into the outline target (depth test `GL_EQUAL` against the scene).
     *
     * @param handle Instance to outline; nothing happens if it is not live.
     * @param color RGBA outline color.
     */
    void RenderOutline(InstanceHandle handle, const tg::vec4& color);

    /// Composites the outline target onto the screen.
    void RenderOutlineResolve();

    /**
     * Draws all instances and their auxiliary models.
     *
     * @param shader Bound program; receives `uModel`, `uTexAlbedo`, `uPickingID` and `uAlbedoBiasPacked` per instance.
     */
    void RenderInstances(glow::UsedProgram& shader);

    /// Draws the skybox.
    void RenderBackground();

    /// Draws all particles instanced into the scene framebuffer.
    void RenderParticles();

    /// Blurs the bright parts of the scene (5 horizontal + vertical ping-pong passes).
    void RenderBloom();

    /**
     * Draws all instances for a shadow pass, with depth test and back-face culling.
     *
     * @param shader Bound program; receives `uModel` per instance.
     */
    void RenderDepth(glow::UsedProgram& shader);


private:
    /**
     * Draws the auxiliary models of an instance and logs a message per model. Unused: no instance has any.
     *
     * @param shader Bound program; receives `uModel`, `uTexAlbedo` and `uAlbedoBiasPacked` per model.
     * @param instance Instance whose `auxModels` are drawn.
     * @param parentTransform Model matrix of `instance`; the models' transforms are relative to it.
     */
    void RenderAuxiliary(glow::UsedProgram& shader, const Instance& instance, const tg::mat4x3& parentTransform);

private:
    /// Creates the render targets (1x1 until `ResizeRenderTargets`), the point shadow cube map and the framebuffers.
    void CreateGFXResources();

    /// Loads textures, meshes and shaders and sets up the point light uniform buffer.
    void LoadGFXResources();

    /// (Re)creates the directional shadow map if its resolution changed.
    void UpdateShadowMapTexture();

    /**
     * Sets the uniforms shared by the scene and shadow passes (time, camera, lights, shadow maps, switches).
     *
     * @param program Bound `poly` program.
     */
    void SetupSceneShader(glow::UsedProgram& program);

    // ECS Specifics
private:
    EngineECS* mECS = nullptr; ///< ECS (not owned).
    long int mDrawCalls = 0;   ///< Draw calls of the current frame.

private:
    // Debug
    bool mRenderNormals;   ///< Show normals instead of shading.
    bool mRenderWireframe; ///< Render polygons as lines.
    bool mEnableCullFaces; ///< Cull back faces.

    // Shadows
    bool mEnablePointShadows; ///< Point light shadows on.
    bool mEnableDirShadows;   ///< Sun shadows on.

    // Lighting
    bool mEnablePointLight; ///< Point lights on.
    bool mEnableDirLight;   ///< Sun on.
    bool mEnableAmbLight;   ///< Ambient light on.

    // Gamelogic
    bool mEnableFogOfWar; ///< Fog of war on.

    // FFX
    bool mEnablePostProcess; ///< Passed as `uShowPostProcess` (inverts the foreground); never set (false).
    bool mEnableTonemap;     ///< Tonemapping on.
    bool mEnableSRGBCurve;   ///< Gamma correction on.
    bool mEnableSepia;       ///< Sepia on.
    bool mEnableVignette;    ///< Vignette on.
    bool mEnableDesaturate;  ///< Desaturation on.
    bool mEnableBloom;       ///< Bloom on.
    bool mEnableDistFadeoff; ///< Distance fade on.

    // Particles
    bool mEnableParticles; ///< Particles on.

private:
    // Camera
    tg::pos3 mCameraPos;     ///< Camera position.
    tg::vec3 mCameraForward; ///< View direction.
    tg::vec3 mCameraUp;      ///< Up vector.
    tg::isize2 mResolution;  ///< Window size in pixels.
    float mNearPlane;        ///< Camera near plane.
    float mFarPlane;         ///< Camera far plane.
    tg::mat4 mProj;          ///< Camera projection.
    tg::mat4 mView;          ///< Camera view matrix.

    // Time
    float mRuntime;  ///< Real time in seconds.
    float mGametime; ///< Game time of day in seconds.

    // Shadows
    int mPointShadowResolution; ///< Edge length of the point shadow cube map faces.
    int mDirShadowResolution;   ///< Edge length of the directional shadow map.

    float mShadowNearPlane; ///< Unused.
    float mShadowFarPlane;  ///< Far plane of the point shadow cube map, which stores distance / far (`uFar` in the shaders).

    tg::mat4 mDirShadowViewProj;     ///< World to clip space of the directional shadow map (`uShadowViewProj`).
    float mDirShadowTexelSize = 0.f; ///< Edge length of a directional shadow map texel in world units (`uDirShadowTexel`).

    // Lighting
    AmbientLight mAmbLight;               ///< Ambient light.
    DirectionalLight mDirLight;           ///< Sun.
    std::vector<PointLight> mPointLights; ///< Regular point lights.
    PointLight mShadowingPointLight;      ///< Shadow-casting point light.

    // Gamelogic
    InstanceHandle mSelected; ///< Selected instance (outlined).

    // FFX
    float mDesaturationIntensity; ///< Desaturation strength.
    float mSepiaIntensity;        ///< Sepia strength.
    float mVignetteIntensity;     ///< Vignette strength.
    float mVignetteRadius;        ///< Vignette radius.
    float mVignetteSoftness;      ///< Vignette softness.
    tg::vec3 mVignetteColor;      ///< Vignette color.
    float mDistFadeoffIntensity;  ///< Distance fade strength.
    tg::vec3 mDistFadeoffColor;   ///< Distance fade color.

    // Particles
    long int mParticleCount; ///< Number of particles to draw.

    // Selection
    std::vector<InstanceHandle> mOutlineHandles; ///< Additionally outlined instances.
    std::vector<tg::vec4> mOutlineColors;        ///< Outline color per entry of `mOutlineHandles`.

    // Paths
private:
    std::string texPath = "../data/textures/"; ///< Texture folder (skybox).

    // Shaders
private:
    // shaders
    glow::SharedProgram mShaderOutput;         ///< Post-processing to the screen.
    glow::SharedProgram mShaderObject;         ///< PBR object shader. Unused.
    glow::SharedProgram mShaderObjectSimple;   ///< Simple object shader. Unused.
    glow::SharedProgram mShaderOutlineForward; ///< Draws outlined instances.
    glow::SharedProgram mShaderOutlineResolve; ///< Composites the outlines.

    // Custom Low-Poly Shader
    glow::SharedProgram mShaderPoly;       ///< Scene.
    glow::SharedProgram mShaderBackground; ///< Skybox.
    glow::SharedProgram mShaderParticle;   ///< Instanced particles.
    glow::SharedProgram mShaderBlur;       ///< Bloom blur.
    glow::SharedProgram mShaderDepth;      ///< Point shadow cube map.
    glow::SharedProgram mShaderDepthDir;   ///< Directional shadow map.

    // Render Targets
private:
    // intermediate framebuffer with color and depth texture
    glow::SharedFramebuffer mFramebufferScene;       ///< HDR scene: color, bright, picking, depth.
    glow::SharedTextureRectangle mTargetColor;       ///< HDR scene color.
    glow::SharedTextureRectangle mTargetBrightColor; ///< Bright parts for bloom.
    glow::SharedTextureRectangle mTargetPicking;     ///< Instance handle per pixel.
    glow::SharedTextureRectangle mTargetDepth;       ///< Scene depth.

    glow::SharedFramebuffer mFramebufferOutline;             ///< Outline color + scene depth.
    glow::SharedTextureRectangle mTargetOutlineIntermediate; ///< Outline colors.

    glow::SharedFramebuffer mFramebufferReadback; ///< Picking target only, for reading it back.

    // Two framebuffers to create bloom
    glow::SharedFramebuffer mFramebufferPing;      ///< Bloom pass A.
    glow::SharedTextureRectangle mTargetPingColor; ///< Bloom pass A result.

    glow::SharedFramebuffer mFramebufferPong;      ///< Bloom pass B.
    glow::SharedTextureRectangle mTargetPongColor; ///< Bloom pass B result (final bloom).

    glow::SharedFramebuffer mFramebufferPointShadow; ///< Point shadow pass.
    glow::SharedTextureCubeMap mPointShadowDepthMap; ///< Point shadow depth cube map.
    glow::SharedFramebuffer mFramebufferDirShadow;   ///< Directional shadow pass.
    glow::SharedTexture2D mDirShadowDepthMap;        ///< Directional shadow depth map.

    std::vector<glow::SharedTextureRectangle> mTargets; ///< All screen-sized targets (resized together).

    // Render Resources
private:
    glow::SharedTextureCubeMap mTexSkybox; ///< Skybox.
    glow::SharedVertexArray mMeshQuad;     ///< Quad for the skybox.

    // meshes (as vertex array obejects)
    glow::SharedVertexArray mVaoParticle; ///< Particle cube with instance attributes.
    glow::SharedVertexArray mVaoCube;     ///< Test cube. Unused.
    glow::SharedVertexArray mVaoSphere;   ///< Test sphere. Unused.
    glow::SharedVertexArray mVaoTest;     ///< Test tree. Unused.

    // textures
    glow::SharedTexture2D mTexAlbedo; ///< Test texture. Unused.
    glow::SharedTexture2D mTexNormal; ///< Test texture. Unused.
    glow::SharedTexture2D mTexARM;    ///< Test texture. Unused.
    glow::SharedTexture2D mTexTest;   ///< Test texture. Unused.

    // Lights
    glow::SharedUniformBuffer mPointLightUBO; ///< `ubPointLights` uniform buffer.
};
}
