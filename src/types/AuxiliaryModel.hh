#pragma once
#include <glow/fwd.hh>
#include "advanced/transform.hh"
#include "typed-geometry/types/pos.hh"
#include "types/Properties.hh"
#include <string>

namespace gamedev
{
struct AuxiliaryModel
{
      glow::SharedVertexArray vao;

      glow::SharedTexture2D texAlbedo;
      glow::SharedTexture2D texNormal;
      glow::SharedTexture2D texARM;
      uint32_t albedoBias;

      transform xform;
};
}
