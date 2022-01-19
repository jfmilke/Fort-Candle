#include "factories/UnitFactory.hh"
#include "utility/misc.hh"
#include <glow/common/log.hh>

// All units can be rendered
void gamedev::UnitFactory::PostProcessRegistration(InstanceHandle presetHandle)
{
    auto renderable = mECS->CreateComponent<Render>(presetHandle);
    auto animated = mECS->CreateComponent<gamedev::Animated>(presetHandle);

    // Render
    renderable->selectable = true;
}

void gamedev::UnitFactory::InitResources()
{ 
    Init_Monster();
    Init_Person();
    Init_Artisan();
    Init_Soldier();
}

gamedev::InstanceHandle gamedev::UnitFactory::Register(std::string asset_identifier, bool boxshape, float scaling)
{
    std::string a_name = lowerString(asset_identifier);

    auto handle = Factory::Register(a_name, scaling);

    if (boxshape)
    {
        auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
        shape->box = mObj->aabb3_as_box(mObj->getAABB(a_name));
    }
    else
    {
        auto shape = mECS->CreateComponent<gamedev::CircleShape>(handle);
        shape->circle = mObj->aabb3_as_circle(mObj->getAABB(a_name));
    }

    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto physics = mECS->CreateComponent<gamedev::Physics>(handle);

    // Collider
    collider->dynamic = true;

    // Physics
    physics->velocity = {0, 0, 0};

    return handle;
}

gamedev::InstanceHandle gamedev::UnitFactory::Register(std::string asset_identifier, std::string unit_identifier, bool boxshape, float scaling)
{
    std::string a_name = lowerString(asset_identifier);
    std::string u_name = lowerString(unit_identifier);

    auto handle = Factory::Register(a_name, u_name, scaling);
    auto& instance = mECS->GetInstance(handle);

    auto renderable = mECS->CreateComponent<Render>(handle);

    if (boxshape)
    {
        auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
        shape->box = mObj->aabb3_as_box(mObj->getAABB(a_name));
    }
    else
    {
        auto shape = mECS->CreateComponent<gamedev::CircleShape>(handle);
        shape->circle = mObj->aabb3_as_circle(mObj->getAABB(a_name));
    }

    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto physics = mECS->CreateComponent<gamedev::Physics>(handle);

    instance.xform.translation = {0, 0, 0};
    instance.xform.scale(scaling);

    // Render


    // Collider
    collider->dynamic = true;

    // Physics
    physics->velocity = {0, 0, 0};

    return handle;
}

gamedev::InstanceHandle gamedev::UnitFactory::Register(std::string asset_identifier, std::string unit_identifier, std::filesystem::path unit_path, bool boxshape, float scaling)
{
    std::string a_name = lowerString(asset_identifier);
    std::string u_name = lowerString(unit_identifier);

    auto handle = Factory::Register(a_name, u_name, unit_path, scaling);
    auto& instance = mECS->GetInstance(handle);

    auto renderable = mECS->CreateComponent<Render>(handle);

    if (boxshape)
    {
        auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
        shape->box = mObj->aabb3_as_box(mObj->getAABB(a_name));
    }
    else
    {
        auto shape = mECS->CreateComponent<gamedev::CircleShape>(handle);
        shape->circle = mObj->aabb3_as_circle(mObj->getAABB(a_name));
    }

    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto physics = mECS->CreateComponent<gamedev::Physics>(handle);

    instance.xform.translation = {0, 0, 0};
    instance.xform.scale(scaling);

    // Render


    // Collider
    collider->dynamic = true;

    // Physics
    physics->velocity = {0, 0, 0};

    return handle;
}

void gamedev::UnitFactory::RegisterAutomatically(std::vector<std::string> asset_identifiers, float global_scaling, bool global_boxshape)
{
    for (const auto& obj : asset_identifiers)
    {
        Register(obj, global_boxshape, global_scaling);
    }
}

void gamedev::UnitFactory::Init_Monster()
{
    auto handle = mIDToPreset[mObjectNameToID["creature"]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<Render>(handle);
    auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto lifeform = mECS->CreateComponent<gamedev::Living>(handle);
    auto attacker = mECS->CreateComponent<gamedev::Attacker>(handle);
    auto mortal = mECS->CreateComponent<gamedev::Mortal>(handle);
    auto physics = mECS->CreateComponent<gamedev::Physics>(handle);
    auto animated = mECS->CreateComponent<gamedev::Animated>(handle);

    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(0.25);

    // Render
    renderable->selectable = false;
    renderable->targetable = true;

    // Shape
    shape->box = mObj->aabb3_as_box(mObj->getAABB("creature"));

    // Collider
    collider->dynamic = true;

    // Evil
    lifeform->name = "Algaz";
    lifeform->idle= 0.0;
    lifeform->friendly = false;

    // Attacker
    attacker->weapon.cooldown = 1.5f;
    attacker->weapon.damage = 1;
    attacker->weapon.range = 1.0;
    attacker->weapon.type = WeaponType::Claws;

    // mortal
    mortal->armor = 0;
    mortal->health = {2, 2};

    // physics
    physics->velocity = tg::vec3::zero;
}

void gamedev::UnitFactory::Init_Person()
{
    auto handle = mIDToPreset[mObjectNameToID["person"]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<Render>(handle);
    auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto lifeform = mECS->CreateComponent<gamedev::Living>(handle);
    auto mortal = mECS->CreateComponent<gamedev::Mortal>(handle);
    auto physics = mECS->CreateComponent<gamedev::Physics>(handle);
    auto animated = mECS->CreateComponent<gamedev::Animated>(handle);
    auto dweller = mECS->CreateComponent<gamedev::Dweller>(handle);

    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(0.25);

    // Render
    renderable->selectable = true;

    // Shape
    shape->box = mObj->aabb3_as_box(mObj->getAABB("person"));

    // Collider
    collider->dynamic = true;

    // Friendly
    lifeform->name = "Hans";
    lifeform->idle = 0.0;
    lifeform->friendly = true;

    // mortal
    mortal->armor = 0;
    mortal->health = {2, 2};

    // physics
    physics->velocity = tg::vec3::zero;
}

void gamedev::UnitFactory::Init_Artisan()
{
    auto handle = mIDToPreset[mObjectNameToID["person_artisan"]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<Render>(handle);
    auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto lifeform = mECS->CreateComponent<gamedev::Living>(handle);
    auto producer = mECS->CreateComponent<gamedev::Producer>(handle);
    auto mortal = mECS->CreateComponent<gamedev::Mortal>(handle);
    auto physics = mECS->CreateComponent<gamedev::Physics>(handle);
    auto animated = mECS->CreateComponent<gamedev::Animated>(handle);
    auto dweller = mECS->CreateComponent<gamedev::Dweller>(handle);


    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(0.25);

    // Render
    renderable->selectable = true;

    // Shape
    shape->box = mObj->aabb3_as_box(mObj->getAABB("person_artisan"));

    // Collider
    collider->dynamic = true;

    // Friendly
    lifeform->name = "Jürgen";
    lifeform->idle = 0.0;
    lifeform->friendly = true;

    // Producer
    producer->income = 0.1;

    // mortal
    mortal->armor = 0;
    mortal->health = {2, 2};

    // physics
    physics->velocity = tg::vec3::zero;
}

void gamedev::UnitFactory::Init_Soldier() 
{
    auto handle = mIDToPreset[mObjectNameToID["person_soldier"]];
    auto& instance = mECS->GetInstance(handle);

    mECS->RemoveAllComponents(handle);

    auto prototype = mECS->CreateComponent<gamedev::Prototype>(handle);
    auto renderable = mECS->CreateComponent<Render>(handle);
    auto shape = mECS->CreateComponent<gamedev::BoxShape>(handle);
    auto collider = mECS->CreateComponent<gamedev::Collider>(handle);
    auto lifeform = mECS->CreateComponent<gamedev::Living>(handle);
    auto attacker = mECS->CreateComponent<gamedev::Attacker>(handle);
    auto mortal = mECS->CreateComponent<gamedev::Mortal>(handle);
    auto climber = mECS->CreateComponent<gamedev::Climber>(handle);
    auto physics = mECS->CreateComponent<gamedev::Physics>(handle);
    auto animated = mECS->CreateComponent<gamedev::Animated>(handle);
    auto dweller = mECS->CreateComponent<gamedev::Dweller>(handle);


    instance.xform.translation = {0, 0, 0};
    instance.xform.scaling = tg::size3(0.25);

    // Render
    renderable->selectable = true;

    // Shape
    shape->box = mObj->aabb3_as_box(mObj->getAABB("person_soldier"));

    // Collider
    collider->dynamic = true;

    // Friendly
    lifeform->name = "Siegbert";
    lifeform->idle = 0.0;
    lifeform->friendly = true;

    // Attacker
    attacker->weapon.cooldown = 1.5f;
    attacker->weapon.damage = 2;
    attacker->weapon.range = 8;
    attacker->weapon.type = WeaponType::Bow;
    attacker->weapon.missileSpeed = 3.f;

    // mortal
    mortal->armor = 0;
    mortal->health = {2, 2};

    // climber
    climber->elevated = false;
    climber->speed = 1;

    // physics
    physics->velocity = tg::vec3::zero;
}