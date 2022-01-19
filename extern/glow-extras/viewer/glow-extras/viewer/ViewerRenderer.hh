#pragma once

#include <vector>

#include <glow/common/shared.hh>

#include <glow/objects/Framebuffer.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Texture2D.hh>
#include <glow/objects/TextureRectangle.hh>

#include <glow/util/TexturePool.hh>

#include <glow-extras/geometry/Quad.hh>
#include <glow-extras/vector/backend/opengl.hh>
#include <glow-extras/vector/image2D.hh>

#include <typed-geometry/tg.hh>

#include "CameraController.hh"
#include "RenderInfo.hh"
#include "renderables/Renderable.hh"

namespace glow::viewer
{
struct SubViewData;

class ViewerRenderer
{
private:
    // == Shaders ==
    SharedProgram mShaderBackground;
    SharedProgram mShaderOutline;
    SharedProgram mShaderSSAO;
    SharedProgram mShaderAccum;
    SharedProgram mShaderOutput;
    SharedProgram mShaderGround;
    SharedProgram mShaderShadow;

    // == Mesh ==
    SharedVertexArray mMeshQuad;

    // == Framebuffers ==
    SharedFramebuffer mFramebuffer;
    SharedFramebuffer mFramebufferColor;
    SharedFramebuffer mFramebufferSSAO;
    SharedFramebuffer mFramebufferOutput;
    SharedFramebuffer mFramebufferShadow;
    SharedFramebuffer mFramebufferShadowSoft;

    // == Other ==
    tg::rng mRng;
    vector::OGLRenderer mVectorRenderer;
    vector::image2D mVectorImage;

    // == Config ==
    int const mMinAccumCnt = 128;
    int const mMinSSAOCnt = 8196;
    int const mMinShadowCnt = 1024;
    int const mAccumPerFrame = 1;
    float const mDepthThresholdFactor = 1.0f;
    float const mNormalThreshold = 0.9f;
    float const mGroundOffsetFactor = 0.001f;
    float const mSunOffsetFactor = 1.0f;
    float const mSunScaleFactor = 1.0f;
    float const mShadowStrength = 0.95f;

    int mSSAOSamples = 16;
    int mShadowSamplesPerFrame = 8;
    bool mIsCurrentFrameFullyConverged = true;

    bool mReverseZEnabled = true;

public:
    TexturePool<TextureRectangle> texturePoolRect;
    TexturePool<Texture2D> texturePool2D;

    ViewerRenderer();

    void beginFrame(tg::color3 const& clearColor);
    void renderSubview(tg::isize2 const& res, tg::ipos2 const& offset, SubViewData& subViewData, Scene const& scene, CameraController& cam);
    void endFrame(float approximateRenderTime = 0.f);
    void maximizeSamples();
    void set_2d_transform(tg::mat3x2 const& transform) { mVectorRenderer.set_global_transform(transform); }

    bool isReverseZEnabled() const { return mReverseZEnabled; }

    tg::optional<tg::pos3> query3DPosition(tg::isize2 resolution, tg::ipos2 pixel, SubViewData const& subViewData, CameraController const& cam) const;
};


// Cross-frame persistent data per subview
struct SubViewData
{
    static auto constexpr shadowMapSize = tg::isize2(2048, 2048);

private:
    // For pooled texture allocation
    ViewerRenderer* const renderer;
    GLOW_NON_COPYABLE(SubViewData);

public:
    SharedTextureRectangle targetAccumWrite;
    SharedTextureRectangle targetAccumRead;
    SharedTextureRectangle targetDepth;
    SharedTextureRectangle targetOutput;
    SharedTextureRectangle targetSsao;
    SharedTexture2D shadowMapSoft;

    int accumCount = 0;
    int ssaoSampleCount = 0;
    int shadowSampleCount = 0;

    tg::mat4 lastView = tg::mat4::identity;
    tg::pos3 lastPos = tg::pos3::zero;

    size_t lastHash = 0;

    SubViewData(int w, int h, ViewerRenderer* r) : renderer(r)
    {
        auto const size = TextureRectangle::SizeT(w, h);

        targetAccumRead = renderer->texturePoolRect.alloc({GL_RGBA32F, size});
        targetAccumWrite = renderer->texturePoolRect.alloc({GL_RGBA32F, size});

        if (renderer->isReverseZEnabled())
            targetDepth = renderer->texturePoolRect.alloc({GL_DEPTH_COMPONENT32F, size});
        else
            targetDepth = renderer->texturePoolRect.alloc({GL_DEPTH_COMPONENT32, size});

        targetOutput = renderer->texturePoolRect.alloc({GL_RGBA16F, size});
        targetSsao = renderer->texturePoolRect.alloc({GL_R32F, size});

        shadowMapSoft = renderer->texturePool2D.alloc({GL_R32F, shadowMapSize, 0});
    }

    ~SubViewData()
    {
        renderer->texturePoolRect.free(&targetAccumRead);
        renderer->texturePoolRect.free(&targetAccumWrite);
        renderer->texturePoolRect.free(&targetDepth);
        renderer->texturePoolRect.free(&targetOutput);
        renderer->texturePoolRect.free(&targetSsao);

        renderer->texturePool2D.free(&shadowMapSoft);
    }

    void clearAccumBuffer()
    {
        accumCount = 0;
        ssaoSampleCount = 0;
    }

    void clearShadowMap() { shadowSampleCount = 0; }
};
}
