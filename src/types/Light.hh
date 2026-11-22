#pragma once
#include "typed-geometry/types/vec.hh"
#include "typed-geometry/types/pos.hh"

namespace gamedev
{
/// Point light; the 16-byte alignment matches the std140 layout of the `ubPointLights` uniform buffer.
struct PointLight
{
    alignas(16) tg::pos3 position; ///< Position (world space, or instance-local inside a `PointLightEmitter`).
    alignas(16) tg::vec3 color;    ///< RGB color.
    float intensity;               ///< Brightness factor.
    float radius;                  ///< Range of the light in world units.
};

/// Directional light such as the sun.
struct DirectionalLight
{
    tg::vec3 direction; ///< Direction from the scene towards the light.
    tg::vec3 color;     ///< RGB color, premultiplied with the intensity.
};

/// Constant ambient light.
struct AmbientLight
{
    tg::vec3 color; ///< RGB color, premultiplied with the intensity.
};
}
