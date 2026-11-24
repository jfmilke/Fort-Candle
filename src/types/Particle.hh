#pragma once
#include "typed-geometry/types/vec.hh"
#include "typed-geometry/types/pos.hh"
#include "typed-geometry/types/color.hh"

namespace gamedev
{
/// A simulated particle in the `ParticleSystem` pool (CPU side).
struct Particle
{
    tg::pos3 position; ///< World position.
    tg::vec3 color;    ///< RGB color.
    tg::vec3 velocity; ///< World units per second.
    float rotation_y;  ///< Rotation about the y axis in radians.

    bool active = false; ///< Set on emission and cleared on death; not read.
    float lifeTime;      ///< Total lifetime in seconds.
    float lifeRemaining; ///< Remaining lifetime in seconds.
    float size_t0;       ///< Size (uniform scale of the cube mesh).
    float size_tn;       ///< Unused.
};

/// Per-instance vertex data of one particle, uploaded every frame (layout matches the `particles` shader inputs).
struct ParticleAttributes
{
    tg::pos3 pos;               ///< World position (`aTranslation`).
    float dummy1 = 0.0;         ///< Padding.
    alignas(16) tg::vec3 color; ///< RGB color (`aColor`).
    float dummy2 = 0.0;         ///< Padding.
    float rotation;             ///< Rotation about y in radians (`aRotation`).
    float scale;                ///< Uniform scale (`aScale`).
    float blend;                ///< Remaining fraction of the lifetime, 1 -> 0 (`aBlend`).
};

/// Description of a particle emitter. Random variations are uniform in [-vary / 2, vary / 2] unless noted.
struct ParticleProperties
{
    float particlesPerSecond; ///< Emission rate (continuous emitters only).

    tg::pos3 basePosition;                    ///< Spawn position in the emitting instance's local space.
    tg::vec3 baseVelocity;                    ///< Initial velocity, scaled by the instance scale.
    tg::color3 baseColor;                     ///< Color.
    tg::vec3 varyPosition = tg::vec3::zero;   ///< Position variation per axis, scaled by the instance scale.
    tg::vec3 varyVelocity = tg::vec3::zero;   ///< Velocity variation per axis, scaled like `baseVelocity`.
    tg::color3 varyColor = tg::color3::black; ///< Color variation (result clamped to [0, 1]).

    float baseSize;        ///< Size, scaled by the largest instance scale axis.
    float varySize = 0.0f; ///< Random amount in [0, varySize] subtracted from the size.

    float baseLife = 1.0f; ///< Lifetime in seconds.
    float varyLife = 0.4f; ///< Lifetime variation in seconds.

    float emitNew = 0.0f; ///< Accumulated particles to emit (internal state of the `ParticleSystem`).
};
}
