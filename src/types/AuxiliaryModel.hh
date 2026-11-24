#pragma once
#include <glow/fwd.hh>
#include <string>
#include "advanced/transform.hh"
#include "typed-geometry/types/pos.hh"
#include "types/Properties.hh"

namespace gamedev
{
/**
 * Extra mesh drawn together with an `Instance` (see `Instance::auxModels`).
 *
 * Unused: no instance has auxiliary models.
 */
struct AuxiliaryModel
{
    glow::SharedVertexArray vao; ///< Mesh.

    glow::SharedTexture2D texAlbedo; ///< Base color texture.
    glow::SharedTexture2D texNormal; ///< Normal map (not bound by the renderer).
    glow::SharedTexture2D texARM;    ///< AO/roughness/metallic (not bound by the renderer).
    uint32_t albedoBias;             ///< Packed RGBA tint, see `Instance::albedoBias`.

    transform xform; ///< Transform relative to the owning instance.
};
}
