layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430) buffer ParticlesData
{
    vec3 position[];
    // Particle Data Buffer
};

layout(std430) buffer ParticlesDead
{
    uint deadId[];
};

layout(std430) buffer ParticlesAlive
{
    uint AliveId[];
};

uniform uDt;
uniform uAlive;
uniform uDead;

void main()
{
    uint real_emit = min(request_emit, uDead);

    position += uDt * velocity;
    scale -= uDt * scale_degrade;
    life -= Dt;
    y_rotation += Dt;
}