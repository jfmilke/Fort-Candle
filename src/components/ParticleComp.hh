#pragma once
#include <vector>
#include "components/Component.hh"
#include "types/Particle.hh"

namespace gamedev
{
struct ParticleEmitter : Component
{
    using Component::Component;

    std::vector<ParticleProperties> pp;
};
}