#pragma once
#include <vector>
#include "components/Component.hh"
#include "types/Particle.hh"

namespace gamedev
{
/// Continuous particle emitters attached to an instance, spawned by the `ParticleSystem`.
struct ParticleEmitter : Component
{
    using Component::Component;

    std::vector<ParticleProperties> pp; ///< One entry per emitter.
};
}
