#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"
#include "types/Particle.hh"

namespace gamedev
{
// Inspired by https://github.com/TheCherno/OneHourParticleSystem

class ParticleSystem : public System
{
public:
    void Init(SharedEngineECS& ecs);

    void AddEntity(InstanceHandle& handle, Signature entitySignature);

    void SetupParticleCount(int particleCount);
    void UploadParticleData();
    glow::SharedVertexArray& GetParticleVAO();

    int Update(float elapsed_time);
    void RenderInstanced(const glow::UsedProgram& shader, const glow::SharedFramebuffer& framebuffer);
    void EmitParticle(const ParticleProperties& particleProperties, transform& xform);
    void KillParticle(int id);

    long int GetParticleCount();

private:
    void Init_MonsterDeathParticles();
    void Init_PioneerDeathParticles();
    void Init_BuildingDestroyedParticles();

private:
    void ParticleListener(Event& e);

private:
    std::shared_ptr<EngineECS> mECS;

    std::vector<Particle> mParticlePool;
    std::vector<ParticleAttributes> mParticleAttributes;

    glow::SharedVertexArray mVaoCube;
    glow::SharedArrayBuffer mVboParticles;

private:
    int mFreeIndex = 0;
    int mAlive = 0;

    int MAX_PARTICLES = 100000;
    int INSTANCE_DATA_LENGTH = 17; // 4 * 4 + 1

    ParticleProperties mMonsterDeath;
    ParticleProperties mPioneerDeath;
    ParticleProperties mBuildingDestroyed;
};

/*
class GPUParticleSystem : public System
{
public:
    void Init(SharedEngineECS& ecs);

    void AddEntity(InstanceHandle& handle, Signature entitySignature);

    void EmitParticle(const ParticleProperties& particleProperties, transform& xform);
    void KillParticle(int id);

    void AllocateParticleBuffer(std::uint64_t particles);
    void UpdateParticleBuffer();
    void ComputeParticles();
    void RenderParticles(glow::UsedProgram& shader);

    void Update(float elapsed_time);    

private:
    std::shared_ptr<EngineECS> mECS;

    std::vector<Particle> mParticlePool;
    std::vector<ParticleAttributes> mParticleAttributes;

    glow::SharedVertexArray mVaoParticle;
    glow::SharedArrayBuffer mVboParticleData;
    glow::SharedShaderStorageBuffer mSsboParticleData;
    glow::SharedShaderStorageBuffer mSsboDeadParticles;
    glow::SharedShaderStorageBuffer mSsboAliveParticles1;
    glow::SharedShaderStorageBuffer mSsboAliveParticles2;

    glow::SharedProgram mProgramComputeParticles;

private:
    long unsigned int mFreeIndex = 0;
    long unsigned int mAlive = 0;

    long unsigned int mMaxGenParticles = 100000;
    long unsigned int mParticleBufferSize = 0;

    int PARTICLE_DATA_LENGTH = 
    int INSTANCE_DATA_LENGTH = 17; // 4 * 4 + 1
};
*/
}
