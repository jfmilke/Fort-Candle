#pragma once

#include <glow/fwd.hh>
#include "advanced/World.hh"
#include "ecs/Engine.hh"
#include "ecs/System.hh"
#include "types/Particle.hh"

namespace gamedev
{
/**
 * CPU particle system, drawn as instanced cubes.
 *
 * Inspired by https://github.com/TheCherno/OneHourParticleSystem.
 *
 *  - Entities: all instances with a `ParticleEmitter`, which emit continuously.
 *  - Death and hit events (`MonsterDeath`, `PioneerDeath`, `BuildingDestroyed`, `MonsterHit`, `PioneerHit`, `BuildingHit`)
 *    emit bursts at the event subject.
 *  - Particles live in a pool whose first `mAlive` entries are alive; the pool grows by 20% when it is 80% full.
 *  - `Game::render()` uploads the instance data, and the `RenderSystem` draws the VAO from `GetParticleVAO` with the `particles` shader.
 */
class ParticleSystem : public System
{
public:
    /**
     * Loads the cube mesh, allocates the pool, defines the burst emitters and registers the event listeners.
     *
     * Needs an OpenGL context.
     *
     * @param ecs The ECS; must outlive this object.
     */
    void Init(EngineECS& ecs);

    /**
     * Adds the entity to the system.
     *
     * TODO: Meant to accept only entities with a `Render` component, but the filter has no effect.
     *
     * @param handle Entity with a `ParticleEmitter`.
     * @param entitySignature Its signature.
     */
    void AddEntity(InstanceHandle handle, Signature entitySignature) override;

    /**
     * Resizes the pool and (re)defines the per-instance attributes on the cube VAO.
     *
     * @param particleCount New pool size.
     */
    void SetupParticleCount(int particleCount);

    /// Uploads the attributes of the living particles to the instance buffer.
    void UploadParticleData();

    /// Cube VAO with the per-instance particle attributes attached.
    glow::SharedVertexArray& GetParticleVAO();

    /**
     * Emits new particles for all emitters, ages and moves the living ones and rebuilds the instance data.
     *
     * TODO: When a particle dies, the one swapped into its slot is skipped (not moved, not drawn) for this update.
     *
     * @param elapsed_time Time step in seconds.
     * @return Elapsed time in microseconds.
     */
    int Update(float elapsed_time);

    /**
     * Binds the framebuffer, uploads the instance data and draws all living particles.
     *
     * Unused: `RenderSystem::RenderParticles` draws them.
     *
     * @param shader Unused: the caller must have bound the particle shader.
     * @param framebuffer Target to draw into.
     */
    void RenderInstanced(const glow::UsedProgram& shader, const glow::SharedFramebuffer& framebuffer);

    /**
     * Spawns one particle. Grows the pool by 20% when it is 80% full.
     *
     * @param particleProperties Emitter description; positions and velocities are relative to `xform`.
     * @param xform Transform of the emitting instance (position, scale of offsets, velocity and size).
     */
    void EmitParticle(const ParticleProperties& particleProperties, transform& xform);

    /**
     * Removes a living particle by swapping it with the last living one.
     *
     * @param id Pool index of the particle, in [0, `mAlive`).
     */
    void KillParticle(int id);

    /// Number of living particles.
    long int GetParticleCount();

private:
    /// Sets up `mMonsterDeath` (dark smoke).
    void Init_MonsterDeathParticles();

    /// Sets up `mPioneerDeath` (red drops).
    void Init_PioneerDeathParticles();

    /// Sets up `mBuildingDestroyed` (grey/orange dust).
    void Init_BuildingDestroyedParticles();

private:
    /**
     * Emits a burst at the event subject; the burst type and size depend on the event type.
     *
     * @param e Death, destruction or hit event.
     */
    void ParticleListener(Event& e);

private:
    EngineECS* mECS = nullptr; ///< ECS (not owned).

    std::vector<Particle> mParticlePool;                 ///< All particles; [0, mAlive) are alive.
    std::vector<ParticleAttributes> mParticleAttributes; ///< Instance data of the living particles.

    glow::SharedVertexArray mVaoCube;      ///< Cube mesh with the instance attributes attached.
    glow::SharedArrayBuffer mVboParticles; ///< Instance buffer.

private:
    int mFreeIndex = 0; ///< Pool index for the next particle (equals `mAlive`).
    int mAlive = 0;     ///< Number of living particles.

    int MAX_PARTICLES = 100000;    ///< Current pool size.
    int INSTANCE_DATA_LENGTH = 17; ///< Instance data length in floats (4 * 4 + 1). Unused.

    ParticleProperties mMonsterDeath;      ///< Burst for monster deaths and hits.
    ParticleProperties mPioneerDeath;      ///< Burst for pioneer deaths and hits.
    ParticleProperties mBuildingDestroyed; ///< Burst for destroyed and hit buildings.
};

// Unfinished GPU particle system using the compute shader data/shaders/compute_particles.csh.
// It does not compile and is kept for reference only (see also the end of ParticleSystem.cc).
/*
class GPUParticleSystem : public System
{
public:
    void Init(EngineECS& ecs);

    void AddEntity(InstanceHandle handle, Signature entitySignature) override;

    void EmitParticle(const ParticleProperties& particleProperties, transform& xform);
    void KillParticle(int id);

    void AllocateParticleBuffer(std::uint64_t particles);
    void UpdateParticleBuffer();
    void ComputeParticles();
    void RenderParticles(glow::UsedProgram& shader);

    void Update(float elapsed_time);    

private:
    EngineECS* mECS = nullptr;

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
