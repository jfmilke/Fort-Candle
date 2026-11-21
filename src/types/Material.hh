#pragma once
#include <glow/fwd.hh>
#include <typed-geometry/tg.hh>


namespace gamedev
{
/// Surface material with PBR parameters and textures. Unused by the game.
struct Material
{
    // material
    float metallic = 0.0;     ///< Metalness [0, 1].
    float reflectivity = 0.3; ///< Reflectivity [0, 1].
    float translucency = 0.0; ///< Translucency [0, 1].

    // textures
    float textureScale = 1.0f;          ///< Texture coordinate scale.
    glow::SharedTexture2D texAlbedo;    ///< Base color.
    glow::SharedTexture2D texAO;        ///< Ambient occlusion.
    glow::SharedTexture2D texHeight;    ///< Height map.
    glow::SharedTexture2D texNormal;    ///< Normal map.
    glow::SharedTexture2D texRoughness; ///< Roughness.

    bool opaque = true; ///< False for transparent materials.
};
}
