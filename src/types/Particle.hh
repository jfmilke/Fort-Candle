#pragma once
#include "typed-geometry/types/vec.hh"
#include "typed-geometry/types/pos.hh"
#include "typed-geometry/types/color.hh"

namespace gamedev
{
struct Particle
{
    tg::pos3 position;
    tg::vec3 color;
    tg::vec3 velocity;
    float rotation_y;

    bool active = false;
    float lifeTime;
    float lifeRemaining;
    float size_t0;
    float size_tn;
};

struct ParticleAttributes
{
    tg::pos3 pos;
    float dummy1 = 0.0;
    alignas(16) tg::vec3 color;
    float dummy2 = 0.0;
    float rotation;
    float scale;
    float blend;
};

struct ParticleProperties
{
    float particlesPerSecond;

    tg::pos3 basePosition;
    tg::vec3 baseVelocity;
    tg::color3 baseColor;
    tg::vec3 varyPosition = tg::vec3::zero;
    tg::vec3 varyVelocity = tg::vec3::zero;
    tg::color3 varyColor = tg::color3::black;

    float baseSize;
    float varySize = 0.0f;

    float baseLife = 1.0f;
    float varyLife = 0.4f;

    float emitNew = 0.0f;
};
}
