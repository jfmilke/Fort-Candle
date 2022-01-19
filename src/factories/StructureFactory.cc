#include "factories/StructureFactory.hh"
#include "utility/misc.hh"

// All structures can be rendered
void gamedev::StructureFactory::PostProcessRegistration(InstanceHandle presetHandle)
{
    auto renderable = mECS->CreateComponent<Render>(presetHandle);
    auto destructible = mECS->CreateComponent<Destructible>(presetHandle);
    destructible->health.max = 10.0;
    destructible->health.current = destructible->health.max;

    // Render
}

void gamedev::StructureFactory::InitResources()
{
    Init_Rescales();

    Init_StartingBonfire();
    Init_RegularBonfire();
    Init_aLantern();
    Init_Tower();
    Init_Forge();
    Init_aFences();
    Init_aStonewall();
    Init_aStoneGate();
    Init_aStonePillar();
    Init_aSpikeBarriers();
    Init_aWagon();
    Init_aBanner();
    Init_Streets();
    Init_WatchTowers();
    Init_Stonefence();
}

gamedev::InstanceHandle gamedev::StructureFactory::Register(std::string asset_identifier, bool boxshape, float scaling)
{
    std::string a_name = lowerString(asset_identifier);

    auto handle = Factory::Register(a_name, scaling);

    if (boxshape)
    {
        auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);

        // BoxShape
        shape->box = mObj->aabb3_as_box(mObj->getAABB(a_name));
    }
    else
    {
        auto shape = mECS->CreateComponent<gamedev::CircleShape>(handle);

        // CircleShape
        shape->circle = mObj->aabb3_as_circle(mObj->getAABB(a_name));
    }

    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);

    // Collider
    collider->dynamic = false;

    return handle;
}

gamedev::InstanceHandle gamedev::StructureFactory::Register(std::string asset_identifier, std::string structure_identifier, bool boxshape, float scaling)
{
    std::string a_name = lowerString(asset_identifier);
    std::string s_name = lowerString(structure_identifier);

    auto handle = Factory::Register(a_name, s_name, scaling);

    if (boxshape)
    {
        auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);

        // BoxShape
        shape->box = mObj->aabb3_as_box(mObj->getAABB(a_name));
    }
    else
    {
        auto shape = mECS->CreateComponent<gamedev::CircleShape>(handle);

        // CircleShape
        shape->circle = mObj->aabb3_as_circle(mObj->getAABB(a_name));
    }

    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);

    // Collider
    collider->dynamic = false;

    return handle;
}

gamedev::InstanceHandle gamedev::StructureFactory::Register(std::string asset_identifier, std::string structure_identifier, std::filesystem::path structure_path, bool boxshape, float scaling)
{
    std::string a_name = lowerString(asset_identifier);
    std::string s_name = lowerString(structure_identifier);

    auto handle = Factory::Register(a_name, s_name, structure_path, scaling);

    if (boxshape)
    {
        auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);

        // BoxShape
        shape->box = mObj->aabb3_as_box(mObj->getAABB(a_name));
    }
    else
    {
        auto shape = mECS->CreateComponent<gamedev::CircleShape>(handle);

        // CircleShape
        shape->circle = mObj->aabb3_as_circle(mObj->getAABB(a_name));
    }

    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);

    // Collider
    collider->dynamic = false;

    return handle;
}

void gamedev::StructureFactory::RegisterAutomatically(std::vector<std::string> asset_identifiers, float global_scaling, bool global_boxshape)
{
    for (const auto& obj : asset_identifiers)
    {
        Register(obj, global_boxshape, global_scaling);
    }
}

void gamedev::StructureFactory::Init_Rescales()
{
    std::vector<std::string> ids = {"tent0", "tent1", "tower0", "wall0", "wall1"};

    for (const auto& id : ids)
    {
        auto handle = GetPreset(id);
        mECS->GetInstanceTransform(handle).scaling = tg::size3(0.35f);
    }
}

void gamedev::StructureFactory::Init_StartingBonfire()
{
    Register("bonfire", "starting_bonfire", false, 0.5);

    auto handle = mIDToPreset[mObjectNameToID["starting_bonfire"]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<Render>(handle);
    //auto shape = mECS->CreateComponent<gamedev::CircleShape>(handle);
    auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto light = mECS->CreateComponent<gamedev::PointLightEmitter>(handle);
    auto particles = mECS->CreateComponent<gamedev::ParticleEmitter>(handle);

    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(1.2);

    // Render

    // Shape
    //shape->circle = mObj->aabb3_as_circle(mObj->getAABB("bonfire"));
    shape->box = mObj->aabb3_as_box(mObj->getAABB("bonfire"));

    // Collider
    collider->dynamic = false;

    // Light
    //light->pl.position = shape->circle.center + tg::vec3(0, 1, 0); // Light above center
    light->pl.position = shape->box.center + tg::vec3(0, 1, 0); // Light above center
    light->pl.color = tg::vec3(255, 108, 33) / 255;
    light->pl.intensity = 20.0;
    light->pl.radius = 15.0;
    light->shadowing = true;

    // Particles: Fire
    particles->pp.resize(2);
    particles->pp[0].baseColor = tg::color3(255, 108, 33) / 255;
    particles->pp[0].baseLife = 0.8;
    particles->pp[0].basePosition = tg::pos3(0.0, -0.2, 0.0);
    particles->pp[0].baseSize = 0.1;
    particles->pp[0].baseVelocity = tg::vec3(0.0, 0.5, 0.0);
    particles->pp[0].varyColor = tg::color3(0.1, 0.05, 0.0);
    particles->pp[0].varyLife = 0;
    particles->pp[0].varyPosition = tg::vec3(0.7, 0.0, 0.7);
    particles->pp[0].varyVelocity = tg::vec3(0.1, 0.2, 0.1);
    particles->pp[0].particlesPerSecond = 151;

    // Particles: Ember
    particles->pp[1].baseColor = tg::color3(255, 108, 33) / 255;
    particles->pp[1].baseLife = 0.8;
    particles->pp[1].basePosition = tg::pos3(0.0, -0.2, 0.0);
    particles->pp[1].baseSize = 0.01;
    particles->pp[1].baseVelocity = tg::vec3(0.0, 0.8, 0.0);
    particles->pp[1].varyColor = tg::color3(0.1, 0.05, 0.0);
    particles->pp[1].varyLife = 0;
    particles->pp[1].varyPosition = tg::vec3(0.7, 0.0, 0.7);
    particles->pp[1].varyVelocity = tg::vec3(0.1, 0.3, 0.1);
    particles->pp[1].particlesPerSecond = 151;
}

void gamedev::StructureFactory::Init_aLantern()
{
    auto handle = mIDToPreset[mObjectNameToID["astreetlantern"]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<Render>(handle);
    auto shape = mECS->CreateComponent<gamedev::CircleShape>(handle);
    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto light = mECS->CreateComponent<gamedev::PointLightEmitter>(handle);
    auto destructible = mECS->CreateComponent<Destructible>(handle);

    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(0.5);


    // Destructible
    destructible->health.max = 10.0;
    destructible->health.current = destructible->health.max;

    // Shape
    shape->circle = mObj->aabb3_as_circle(mObj->getAABB("aStreetLantern"));

    // Collider
    collider->dynamic = false;

    // Light
    light->pl.position = tg::pos3(0.77, 1.8, 0);
    light->pl.color = tg::vec3(255, 108, 33) / 255;
    light->pl.intensity = 20.0;
    light->pl.radius = 10.0;
}

void gamedev::StructureFactory::Init_RegularBonfire()
{
    auto handle = mIDToPreset[mObjectNameToID["bonfire"]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<Render>(handle);
    // auto shape = mECS->CreateComponent<gamedev::CircleShape>(handle);
    auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto light = mECS->CreateComponent<gamedev::PointLightEmitter>(handle);
    auto particles = mECS->CreateComponent<gamedev::ParticleEmitter>(handle);

    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(0.25);

    // Render

    // Shape
    //shape->circle = mObj->aabb3_as_circle(mObj->getAABB("bonfire"));
    shape->box = mObj->aabb3_as_box(mObj->getAABB("bonfire"));

    // Collider
    collider->dynamic = false;

    // Light
    //light->pl.position = shape->circle.center + tg::vec3(0, 1, 0); // Light above center
    light->pl.position = shape->box.center + tg::vec3(0, 1, 0); // Light above center
    light->pl.color = tg::vec3(255, 108, 33) / 255;
    light->pl.intensity = 20.0;
    light->pl.radius = 3.0;

    // Particles: Fire
    particles->pp.resize(2);
    particles->pp[0].baseColor = tg::color3(255, 108, 33) / 255;
    particles->pp[0].baseLife = 0.5;
    particles->pp[0].basePosition = tg::pos3(0.0, -0.2, 0.0);
    particles->pp[0].baseSize = 0.1;
    particles->pp[0].baseVelocity = tg::vec3(0.0, 0.5, 0.0);
    particles->pp[0].varyColor = tg::color3(0.1, 0.05, 0.0);
    particles->pp[0].varyLife = 0;
    particles->pp[0].varyPosition = tg::vec3(0.7, 0.0, 0.7);
    particles->pp[0].varyVelocity = tg::vec3(0.1, 0.2, 0.1);
    particles->pp[0].particlesPerSecond = 151;
    // Particles: Ember
    particles->pp[1].baseColor = tg::color3(255, 108, 33) / 255;
    particles->pp[1].baseLife = 0.7;
    particles->pp[1].basePosition = tg::pos3(0.0, -1., 0.0);
    particles->pp[1].baseSize = 0.01;
    particles->pp[1].baseVelocity = tg::vec3(0.0, 0.8, 0.0);
    particles->pp[1].varyColor = tg::color3(0.1, 0.05, 0.0);
    particles->pp[1].varyLife = 0;
    particles->pp[1].varyPosition = tg::vec3(0.7, 0.0, 0.7);
    particles->pp[1].varyVelocity = tg::vec3(0.1, 0.3, 0.1);
    particles->pp[1].particlesPerSecond = 151;
}

void gamedev::StructureFactory::Init_Tower()
{
    auto handle = mIDToPreset[mObjectNameToID["tower0"]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<Render>(handle);
    auto shape = mECS->CreateComponent<gamedev::CircleShape>(handle);
    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto destructible = mECS->CreateComponent<Destructible>(handle);


    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(0.25);

    // Destructible
    destructible->health.max = 10.0;
    destructible->health.current = destructible->health.max;

    // Shape
    shape->circle = mObj->aabb3_as_circle(mObj->getAABB("tower0"));

    // Collider
    collider->dynamic = false;
}

void gamedev::StructureFactory::Init_Forge() 
{
    auto handle = mIDToPreset[mObjectNameToID["forge"]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<Render>(handle);
    auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto destructible = mECS->CreateComponent<Destructible>(handle);
    auto particles = mECS->CreateComponent<gamedev::ParticleEmitter>(handle);

    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(0.33);

    // Destructible
    destructible->health.max = 10.0;
    destructible->health.current = destructible->health.max;

    // Shape
    // auto aabb = mObj->getAABB("forgefurnace");
    shape->box = mObj->aabb3_as_box(mObj->getAABB("forgefurnace"));

    // Collider
    collider->dynamic = false;

    // Particles: Fire
    particles->pp.resize(2);
    particles->pp[0].baseColor = tg::color3(255, 51, 0) / 255;
    particles->pp[0].baseLife = 0.5;
    particles->pp[0].basePosition = tg::pos3(-2.2, 0.52, -1.72);
    particles->pp[0].baseSize = 0.1;
    particles->pp[0].baseVelocity = tg::vec3(0.0, 0.5, 0.0);
    particles->pp[0].varyColor = tg::color3(0.1, 0.05, 0.0);
    particles->pp[0].varyLife = 0;
    particles->pp[0].varyPosition = tg::vec3(0.7, 0.0, 0.7);
    particles->pp[0].varyVelocity = tg::vec3(0.1, 0.2, 0.1);
    particles->pp[0].particlesPerSecond = 151;

    // Particles: Smoke
    particles->pp[1].baseColor = tg::color3::black;
    particles->pp[1].baseLife = 2.5;
    particles->pp[1].basePosition = tg::pos3(-2.8, 2.54, -2.67);
    particles->pp[1].baseSize = 0.1;
    particles->pp[1].baseVelocity = tg::vec3(0.4, 1.5, 0.0);
    particles->pp[1].varyColor = tg::color3::white * 0.1;
    particles->pp[1].varyLife = 0;
    particles->pp[1].varyPosition = tg::vec3(0.1, 0.0, 0.1);
    particles->pp[1].varyVelocity = tg::vec3(0.9, 0.4, 0.1);
    particles->pp[1].particlesPerSecond = 80;
}

void gamedev::StructureFactory::Init_aFences()
{
    std::vector<std::string> names = {"afence0", "afence0partial", "afence1", "afence1partial"};

    for (const auto& n : names)
    {
        auto handle = mIDToPreset[mObjectNameToID[n]];
        auto& instance = mECS->GetInstance(handle);

        mECS->RemoveAllComponents(handle);

        auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
        auto renderable = mECS->CreateComponent<gamedev::Render>(handle);
        auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
        auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
        auto destructible = mECS->CreateComponent<Destructible>(handle);


        instance.xform.translation = {0, 0, 0};
        instance.xform.scaling = tg::size3(1);

        // Destructible
        destructible->health.max = 10.0;
        destructible->health.current = destructible->health.max;

        // Shape
        shape->box = mObj->aabb3_as_box(mObj->getAABB(n));

        // Collider
        collider->dynamic = false;
    }
}

void gamedev::StructureFactory::Init_aStonewall()
{
  auto handle = mIDToPreset[mObjectNameToID["astonewall"]];
  auto& instance = mECS->GetInstance(handle);
  
  mECS->RemoveAllComponents(handle);
  
  auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
  auto renderable = mECS->CreateComponent<gamedev::Render>(handle);
  auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
  auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
  auto destructible = mECS->CreateComponent<Destructible>(handle);

  
  instance.xform.translation = {0, 0, 0};
  instance.xform.scaling = tg::size3(1.5);
  
  // Destructible
  destructible->health.max = 20.0;
  destructible->health.current = destructible->health.max;
  
  // Shape
  shape->box = mObj->aabb3_as_box(mObj->getAABB("astonewall"));
  
  // Collider
  collider->dynamic = false;
}


void gamedev::StructureFactory::Init_aStoneGate()
{
    std::string id = "astonegate";

    auto handle = mIDToPreset[mObjectNameToID[id]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<gamedev::Render>(handle);
    auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
    auto destructible = mECS->CreateComponent<Destructible>(handle);


    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(1.5);

    // Destructible
    destructible->health.max = 20.0;
    destructible->health.current = destructible->health.max;

    // Shape
    shape->box = mObj->aabb3_as_box(mObj->getAABB(id));
}


void gamedev::StructureFactory::Init_aStonePillar()
{
    std::string id = "astonepillar";

    auto handle = mIDToPreset[mObjectNameToID[id]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<gamedev::Render>(handle);
    auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto destructible = mECS->CreateComponent<Destructible>(handle);


    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(1.5);

    // Destructible
    destructible->health.max = 20.0;
    destructible->health.current = destructible->health.max;

    // Shape
    shape->box = mObj->aabb3_as_box(mObj->getAABB(id));

    // Collider
    collider->dynamic = false;
}

void gamedev::StructureFactory::Init_aSpikeBarriers()
{
    std::string id = "aspikebarriers";

    auto handle = mIDToPreset[mObjectNameToID[id]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<gamedev::Render>(handle);
    auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto destructible = mECS->CreateComponent<Destructible>(handle);


    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(1.5);

    // Destructible
    destructible->health.max = 5.0;
    destructible->health.current = destructible->health.max;

    // Shape
    shape->box = mObj->aabb3_as_box(mObj->getAABB(id));

    // Collider
    collider->dynamic = false;
}

void gamedev::StructureFactory::Init_aWagon()
{
    std::string id = "awagon";

    auto handle = mIDToPreset[mObjectNameToID[id]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<gamedev::Render>(handle);
    auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
    auto destructible = mECS->CreateComponent<Destructible>(handle);
    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(2);

    // Destructible
    destructible->health.max = 10.0;
    destructible->health.current = destructible->health.max;


    // Shape
    shape->box = mObj->aabb3_as_box(mObj->getAABB(id));
}


void gamedev::StructureFactory::Init_aBanner()
{
    std::string id = "abanner";

    auto handle = mIDToPreset[mObjectNameToID[id]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<gamedev::Render>(handle);
    auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
    auto destructible = mECS->CreateComponent<Destructible>(handle);


    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(2);

    // Destructible
    destructible->health.max = 10.0;
    destructible->health.current = destructible->health.max;

    // Shape
    shape->box = mObj->aabb3_as_box(mObj->getAABB(id));
}

void gamedev::StructureFactory::Init_Streets()
{
    std::vector<std::string> names
        = {"astoneroad0", "astoneroad1", "astoneroad2", "astoneroad3", "awoodroad0", "awoodroad1", "awoodroad2", "awoodroad3"};

    for (const auto& n : names)
    {
        auto handle = mIDToPreset[mObjectNameToID[n]];
        auto& instance = mECS->GetInstance(handle);

        mECS->RemoveAllComponents(handle);

        auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
        auto renderable = mECS->CreateComponent<gamedev::Render>(handle);

        instance.xform.translation = {0, 0, 0};
        instance.xform.scaling = tg::size3(0.5);
        instance.xform.scaling.height = 1.2;
    }
}

void gamedev::StructureFactory::Init_WatchTowers()
{
    std::vector<std::string> names = {"awatchtower0", "awatchtower1"};

    for (const auto& n : names)
    {
        auto handle = mIDToPreset[mObjectNameToID[n]];
        auto& instance = mECS->GetInstance(handle);

        mECS->RemoveAllComponents(handle);

        auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
        auto renderable = mECS->CreateComponent<gamedev::Render>(handle);
        auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
        auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
        auto tower = mECS->CreateComponent<gamedev::Tower>(handle);

        instance.xform.translation = {0, 0, 0};
        instance.xform.scaling = tg::size3(0.7);

        // Shape
        shape->box = mObj->aabb3_as_box(mObj->getAABB(n));

        // Collider
        collider->dynamic = false;

        // Tower
        tower->position = tg::pos3(0, 2.14, 0);
    }
}

void gamedev::StructureFactory::Init_Stonefence()
{
    auto handle = mIDToPreset[mObjectNameToID["astonefence0"]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<gamedev::Render>(handle);
    auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);

    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(0.5);

    // Shape
    shape->box = mObj->aabb3_as_box(mObj->getAABB("astonefence0"));

    // Collider
    collider->dynamic = false;
}