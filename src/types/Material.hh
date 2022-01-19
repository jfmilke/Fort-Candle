#pragma once
#include <glow/fwd.hh>
#include <typed-geometry/tg.hh>


namespace gamedev
{
struct Material
{
    // material
    float metallic = 0.0;
    float reflectivity = 0.3;
    float translucency = 0.0;

    // textures
    float textureScale = 1.0f;
    glow::SharedTexture2D texAlbedo;
    glow::SharedTexture2D texAO;
    glow::SharedTexture2D texHeight;
    glow::SharedTexture2D texNormal;
    glow::SharedTexture2D texRoughness;

    bool opaque = true;
};
}
