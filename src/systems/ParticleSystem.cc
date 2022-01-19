#include "systems/ParticleSystem.hh"
//#include <glow/objects/Program.hh>
//#include <glow/objects/ArrayBuffer.hh>
//#include <glow/objects/VertexArray.hh>
#include <glow/objects.hh>
#include "typed-geometry/types/vec.hh"
#include "typed-geometry/types/pos.hh"
#include "typed-geometry/types/mat.hh"
#include "utility/Random.hh"
#include "Mesh3D.hh"

void gamedev::ParticleSystem::AddEntity(InstanceHandle& handle, Signature entitySignature)
{
    // Only include rendered systems
    if (!mECS->TestSignature<Render>(handle))
    {
        mEntities.erase(handle);
    }

    mEntities.insert(handle);
}

void gamedev::ParticleSystem::Init(SharedEngineECS& ecs)
{
    Mesh3D cube;
    cube.loadFromFile("../data/meshes/cube.obj", true);

    mECS = ecs;
    mVaoCube = cube.createBasicVertexArray();
    mVboParticles = glow::ArrayBuffer::create();

    SetupParticleCount(MAX_PARTICLES);
    Init_MonsterDeathParticles();
    Init_PioneerDeathParticles();
    Init_BuildingDestroyedParticles();

    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::MonsterDeath, ParticleSystem::ParticleListener));
    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::PioneerDeath, ParticleSystem::ParticleListener));
    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::BuildingDestroyed, ParticleSystem::ParticleListener));
    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::BuildingHit, ParticleSystem::ParticleListener));
    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::PioneerHit, ParticleSystem::ParticleListener));
    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::MonsterHit, ParticleSystem::ParticleListener));
}

void gamedev::ParticleSystem::SetupParticleCount(int particleCount)
{
    MAX_PARTICLES = particleCount;
    mParticlePool.resize(MAX_PARTICLES);
    //mShadowParticlePool.resize(MAX_PARTICLES);

    auto vao = mVaoCube->bind();
    auto vbo = mVboParticles->bind();

    mVboParticles->defineAttribute(&ParticleAttributes::pos, "aTranslation", glow::AttributeMode::Float, 1);
    mVboParticles->defineAttribute(&ParticleAttributes::color, "aColor", glow::AttributeMode::Float, 1);
    mVboParticles->defineAttribute(&ParticleAttributes::rotation, "aRotation", glow::AttributeMode::Float, 1);
    mVboParticles->defineAttribute(&ParticleAttributes::scale, "aScale", glow::AttributeMode::Float, 1);
    mVboParticles->defineAttribute(&ParticleAttributes::blend, "aBlend", glow::AttributeMode::Float, 1);

    vao.attach(mVboParticles);
}

void gamedev::ParticleSystem::UploadParticleData()
{
    auto vbo = mVboParticles->bind();
    vbo.setData(mParticleAttributes, GL_STREAM_DRAW);
}

int gamedev::ParticleSystem::Update(float elapsed_time)
{
    auto t0 = std::chrono::steady_clock::now();

    mParticleAttributes.clear();

    // Create new particles for all emitters
    for (const auto& handle : mEntities)
    {
        auto& xform = mECS->GetInstance(handle).xform;
        auto* particleEmitter = mECS->GetComponent<ParticleEmitter>(handle);

        for (auto& pp : particleEmitter->pp)
        {
            auto& particleProperties = particleEmitter->pp;
            pp.emitNew += elapsed_time * pp.particlesPerSecond;

            for (auto i = 0; i < tg::floor(pp.emitNew); i++)
            {
                EmitParticle(pp, xform);
            }

            if (pp.emitNew > 1.0)
                pp.emitNew = 0.0;
        }
    }

    // Update all living particles
    for (auto id = 0; id < mAlive; id++)
    {
        auto& p = mParticlePool[id];

        if (p.lifeRemaining <= 0.0f)
        {
            KillParticle(id);
            continue;
        }

        p.lifeRemaining -= elapsed_time;
        p.position += p.velocity * elapsed_time;
        p.rotation_y += 0.01f * elapsed_time;
        float life = p.lifeRemaining / p.lifeTime;
        float scale = p.size_t0;
        tg::mat4 modelMatrix = tg::translation(p.position) * tg::rotation_y(tg::degree(p.rotation_y)) * tg::scaling(scale, scale, scale);

        mParticleAttributes.push_back({p.position, 0.0, p.color, 0.0, p.rotation_y, scale, life});
    }

    auto tn = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(tn - t0).count();
}

void gamedev::ParticleSystem::RenderInstanced(const glow::UsedProgram& shader, const glow::SharedFramebuffer& framebuffer)
{
    auto fb = framebuffer->bind();

    UploadParticleData();

    mVaoCube->bind().draw(mAlive);
}

long int gamedev::ParticleSystem::GetParticleCount() { return mAlive; }

glow::SharedVertexArray& gamedev::ParticleSystem::GetParticleVAO() { return mVaoCube; }

void gamedev::ParticleSystem::EmitParticle(const ParticleProperties& particleProperties, transform& xform)
{
    Particle& particle = mParticlePool[mFreeIndex];
    particle.active = true;
    //particle.position = affine_to_mat4(xform.transform_mat()) * particleProperties.basePosition;
    //auto vary = affine_to_mat4(xform.transform_mat()) * particleProperties.varyPosition;
    particle.position = affine_to_mat4(xform.transform_mat()) * particleProperties.basePosition;
    auto vary = xform.scale_mat() * particleProperties.varyPosition;
    particle.position.x += vary.x * (RandomFloat() - 0.5f);
    particle.position.y += vary.y * (RandomFloat() - 0.5f);
    particle.position.z += vary.z * (RandomFloat() - 0.5f);

    particle.color = tg::vec3(particleProperties.baseColor);
    particle.color = tg::saturate(particle.color + tg::vec3(particleProperties.varyColor) * (RandomFloat() - 0.5f));

    particle.rotation_y = RandomFloat() * 2 * tg::pi_scalar<float>;

    particle.velocity = particleProperties.baseVelocity;
    particle.velocity.x += particleProperties.varyVelocity.x * (RandomFloat() - 0.5f);
    particle.velocity.y += particleProperties.varyVelocity.y * (RandomFloat() - 0.5f);
    particle.velocity.z += particleProperties.varyVelocity.z * (RandomFloat() - 0.5f);
    particle.velocity *= xform.scaling;

    particle.lifeTime = particleProperties.baseLife;
    particle.lifeTime += particleProperties.varyLife * (RandomFloat() - 0.5f);
    particle.lifeRemaining = particle.lifeTime;

    float scale = tg::max(xform.scaling.depth, xform.scaling.height, xform.scaling.width);
    particle.size_t0 = scale * particleProperties.baseSize;
    particle.size_t0 -= particleProperties.varySize * RandomFloat();

    mFreeIndex++;
    mAlive++;

    if (mAlive >= MAX_PARTICLES * 0.8)
    {
        glow::error() << "Too many particles, need to increase storage!";
        MAX_PARTICLES += MAX_PARTICLES * 0.2;
        SetupParticleCount(MAX_PARTICLES);
    }
}

void gamedev::ParticleSystem::KillParticle(int id)
{
    mParticlePool[id].active = false;
    std::swap(mParticlePool[id], mParticlePool[mAlive - 1]);
    mAlive--;
    mFreeIndex--;
}

void gamedev::ParticleSystem::ParticleListener(Event& e)
{
    auto& instance = mECS->GetInstance(e.mSubject);

    if (e.mType == EventType::MonsterDeath)
    {
        for (int i = 0; i < 20; i++)
        {
            EmitParticle(mMonsterDeath, instance.xform);
        }
    }
    else if (e.mType == EventType::PioneerDeath)
    {
        for (int i = 0; i < 40; i++)
        {
            EmitParticle(mPioneerDeath, instance.xform);
        }
    }
    else if (e.mType == EventType::BuildingDestroyed)
    {
        for (int i = 0; i < 20; i++)
        {
            EmitParticle(mBuildingDestroyed, instance.xform);
        }
    }
    else if (e.mType == EventType::MonsterHit)
    {
        for (int i = 0; i < 3; i++)
        {
            EmitParticle(mMonsterDeath, instance.xform);
        }
    }
    else if (e.mType == EventType::PioneerHit)
    {
        for (int i = 0; i < 10; i++)
        {
            EmitParticle(mPioneerDeath, instance.xform);
        }
    }
    else if (e.mType == EventType::BuildingHit)
    {
        for (int i = 0; i < 2; i++)
        {
            EmitParticle(mBuildingDestroyed, instance.xform);
        }
    }
}

void gamedev::ParticleSystem::Init_MonsterDeathParticles()
{
    mMonsterDeath.baseColor = tg::color3::black;
    mMonsterDeath.baseLife = 2.f;
    mMonsterDeath.basePosition = tg::pos3::zero;
    mMonsterDeath.baseSize = 0.3f;
    mMonsterDeath.baseVelocity = tg::vec3::unit_y * 0.6f;
    mMonsterDeath.varyColor = tg::color3::white * 0.2f;
    mMonsterDeath.varyLife = 0.2f;
    mMonsterDeath.varyPosition = tg::vec3(0.5, 0.5, 0.5);
    mMonsterDeath.varySize = 0.1f;
    mMonsterDeath.varyVelocity = tg::vec3(0.3f, 0.1f, 0.3f);
}

void gamedev::ParticleSystem::Init_PioneerDeathParticles()
{
    mPioneerDeath.baseColor = tg::color3::red;
    mPioneerDeath.baseLife = 1.3f;
    mPioneerDeath.basePosition = tg::pos3(0, 0.3f, 0);
    mPioneerDeath.baseSize = 0.1f;
    mPioneerDeath.baseVelocity = tg::vec3(0.0f, -0.1f, 0.0f);
    mPioneerDeath.varyColor = tg::color3::red * 0.3f;
    mPioneerDeath.varyLife = 0.2f;
    mPioneerDeath.varyPosition = tg::vec3(0.5, 0.5, 0.5);
    mPioneerDeath.varySize = 0.02f;
    mPioneerDeath.varyVelocity = tg::vec3(1.0f, -0.1f, 1.0f);
}

void gamedev::ParticleSystem::Init_BuildingDestroyedParticles()
{
    mBuildingDestroyed.baseColor = tg::color3::white * 0.5;
    mBuildingDestroyed.baseLife = 2.f;
    mBuildingDestroyed.basePosition = tg::pos3::zero;
    mBuildingDestroyed.baseSize = 0.1f;
    mBuildingDestroyed.baseVelocity = tg::vec3::unit_y * 0.6f;
    mBuildingDestroyed.varyColor = tg::color3(255 / 255.f, 211 / 255.f, 155 / 255.f);
    mBuildingDestroyed.varyLife = 0.2f;
    mBuildingDestroyed.varyPosition = tg::vec3(0.5, 0.5, 0.5);
    mBuildingDestroyed.varySize = 0.05f;
    mBuildingDestroyed.varyVelocity = tg::vec3(0.3f, 0.1f, 0.3f);
}


// =========================================================================================
// =========================================================================================
/*
void gamedev::GPUParticleSystem::AddEntity(InstanceHandle& handle, Signature entitySignature)
{
    // Only include rendered systems
    if (!mECS->TestSignature<Render>(handle))
    {
        mEntities.erase(handle);
    }

    mEntities.insert(handle);
}

void gamedev::GPUParticleSystem::Init(SharedEngineECS& ecs)
{
    Mesh3D cube;
    cube.loadFromFile("../data/meshes/cube.obj", true);

    mECS = ecs;
    mVaoParticle = cube.createBasicVertexArray();
    mVboParticleData = glow::ArrayBuffer::create();
    mSsboParticleData = glow::ShaderStorageBuffer::createAliased(mVboParticleData);
    mSsboDeadParticles = glow::ShaderStorageBuffer::create();
    mSsboAliveParticles1 = glow::ShaderStorageBuffer::create();
    mSsboAliveParticles2 = glow::ShaderStorageBuffer::create();

    mProgramComputeParticles = glow::Program::createFromFile("../data/shaders/compute_particles");

    AllocateParticleBuffer(mMaxGenParticles);
}

void gamedev::GPUParticleSystem::AllocateParticleBuffer(std::uint64_t particles)
{
    mVboParticleData->bind().setData(particles * PARTICLE_DATA_LENGTH, nullptr, GL_STREAM_DRAW);
    mSsboDeadParticles->bind().setData(particles * sizeof(std::uint32_t), 0, GL_STREAM_DRAW);
    mSsboAliveParticles1->bind().setData(particles * sizeof(std::uint32_t), 0, GL_STREAM_DRAW);
    mSsboAliveParticles2->bind().setData(particles * sizeof(std::uint32_t), 0, GL_STREAM_DRAW);

    mParticleBufferSize = particles;
}

void gamedev::GPUParticleSystem::UpdateParticleBuffer()
{
    auto vbo = mVboParticles->bind();
    glBufferSubData(GL_SHADER_STORAGE_BUFFER);
    
    vbo.setData(particles * PARTICLE_DATA_LENGTH, nullptr, GL_STREAM_DRAW);

    mParticleBufferSize = particles;
}

void gamedev::GPUParticleSystem::SetupParticleCount(int particleCount)
{
    MAX_PARTICLES = particleCount;
    mParticlePool.resize(MAX_PARTICLES);
    // mShadowParticlePool.resize(MAX_PARTICLES);

    auto vao = mVaoCube->bind();
    auto vbo = mVboParticles->bind();

    mVboParticles->defineAttribute(&ParticleAttributes::pos, "aTranslation", glow::AttributeMode::Float, 1);
    mVboParticles->defineAttribute(&ParticleAttributes::color, "aColor", glow::AttributeMode::Float, 1);
    mVboParticles->defineAttribute(&ParticleAttributes::rotation, "aRotation", glow::AttributeMode::Float, 1);
    mVboParticles->defineAttribute(&ParticleAttributes::scale, "aScale", glow::AttributeMode::Float, 1);
    mVboParticles->defineAttribute(&ParticleAttributes::blend, "aBlend", glow::AttributeMode::Float, 1);

    vao.attach(mVboParticles);
}

void gamedev::GPUParticleSystem::UploadParticleData()
{
    auto vbo = mVboParticles->bind();
    vbo.setData(mParticleAttributes, GL_STREAM_DRAW);
}

void gamedev::GPUParticleSystem::Update(float elapsed_time)
{
    mParticleAttributes.clear();

    std::uint64_t upperBound = 0;

    // Create new particles for all emitters
    for (const auto& handle : mEntities)
    {
        auto& xform = mECS->GetInstance(handle).xform;
        auto* particleEmitter = mECS->GetComponent<ParticleEmitter>(handle);

        for (auto& pp : particleEmitter->pp)
        {
            auto& particleProperties = particleEmitter->pp;
            pp.emitNew += elapsed_time * pp.particlesPerSecond;

            upperBound += pp.particlesPerSecond * (pp.baseLife + pp.varyLife + 1)

            for (auto i = 0; i < tg::floor(pp.emitNew); i++)
            {
                EmitParticle(pp, xform);
            }

            if (pp.emitNew > 1.0)
                pp.emitNew = 0.0;
        }
    }

    if (upperBound > mMaxGenParticles)
    {
        // First check if a new buffer needs to be allocated
        if (upperBound > mParticleBufferSize)
        {
            AllocateParticleBuffer(upperBound * 1.5);
        }
        else if (upperBound > 0.9 * mParticleBufferSize)
        {
            AllocateParticleBuffer(mParticleBufferSize * 1.5);
        }

        // Then update the number of max generated particles
        mMaxGenParticles = upperBound;
    }

    // Update all living particles
    for (auto id = 0; id < mAlive; id++)
    {
        auto& p = mParticlePool[id];

        if (p.lifeRemaining <= 0.0f)
        {
            KillParticle(id);
            continue;
        }

        p.lifeRemaining -= elapsed_time;
        p.position += p.velocity * elapsed_time;
        p.rotation_y += 0.01f * elapsed_time;
        float life = p.lifeRemaining / p.lifeTime;
        float scale = p.size_t0;
        tg::mat4 modelMatrix = tg::translation(p.position) * tg::rotation_y(tg::degree(p.rotation_y)) * tg::scaling(scale, scale, scale);

        mParticleAttributes.push_back({p.position, p.color, p.rotation_y, scale, life});
    }
}

void gamedev::GPUParticleSystem::RenderParticles(glow::UsedProgram& shader)
{
    mVaoCube->bind().draw(mAlive);
}

void gamedev::GPUParticleSystem::EmitParticle(const ParticleProperties& particleProperties, transform& xform)
{
    Particle& particle = mParticlePool[mFreeIndex];
    particle.active = true;
    particle.position = tg::mat4(xform.transform_mat()) * particleProperties.basePosition;
    auto vary = tg::mat4(xform.transform_mat()) * particleProperties.varyPosition;
    particle.position.x += vary.x * (RandomFloat() - 0.5f);
    particle.position.y += vary.y * (RandomFloat() - 0.5f);
    particle.position.z += vary.z * (RandomFloat() - 0.5f);

    particle.color = tg::vec3(particleProperties.baseColor);
    particle.color = tg::saturate(particle.color + tg::vec3(particleProperties.varyColor) * (RandomFloat() - 0.5f));

    particle.rotation_y = RandomFloat() * 2 * tg::pi_scalar<float>;

    particle.velocity = particleProperties.baseVelocity;
    particle.velocity += particleProperties.varyVelocity * (RandomFloat() - 0.5f);

    particle.lifeTime = particleProperties.baseLife;
    particle.lifeTime += particleProperties.varyLife * (RandomFloat() - 0.5f);
    particle.lifeRemaining = particle.lifeTime;

    float scale = tg::max(xform.scaling.depth, xform.scaling.height, xform.scaling.width);
    particle.size_t0 = scale * particleProperties.baseSize;
    particle.size_t0 -= particleProperties.varySize * RandomFloat();

    mFreeIndex++;
    mAlive++;

    if (mAlive >= MAX_PARTICLES * 0.8)
    {
        glow::error() << "Too many particles, need to increase storage!";
        MAX_PARTICLES += MAX_PARTICLES * 0.2;
        SetupParticleCount(MAX_PARTICLES);
    }
}

void gamedev::GPUParticleSystem::KillParticle(int id)
{
    mParticlePool[id].active = false;
    std::swap(mParticlePool[id], mParticlePool[mAlive - 1]);
    mAlive--;
    mFreeIndex--;
}
*/