#include "systems/GamelogicSystem.hh"
#include "advanced/transform.hh"
#include <typed-geometry/tg.hh>
#include "glow/common/log.hh"
#include "utility/Random.hh"
#include "utility/misc.hh"

void gamedev::GamelogicSystem::AddEntity(InstanceHandle& handle, Signature entitySignature)
{
    mEntities.insert(handle);

    auto lifeform = mECS->GetComponent<Living>(handle);
    bool specialized = false;

    if (lifeform->friendly && mECS->TestSignature<Producer>(handle))
    {
        mArtisans.insert(handle);
        mPeople.erase(handle);
        specialized = true;
    }

    if (lifeform->friendly && mECS->TestSignature<Attacker>(handle))
    {
        mSoldiers.insert(handle);
        mPeople.erase(handle);
        specialized = true;
    }

    if (!lifeform->friendly && mECS->TestSignature<Attacker>(handle))
    {
        mMonsters.insert(handle);
        specialized = true;
    }

    if (lifeform->friendly && !specialized)
    {
        mPeople.insert(handle);
    }
}

void gamedev::GamelogicSystem::RemoveEntity(InstanceHandle& handle, Signature entitySignature) {

    mArtisans.erase(handle);
    mSoldiers.erase(handle);
    mMonsters.erase(handle);
    mPeople.erase(handle);
    mEntities.erase(handle);
}

void gamedev::GamelogicSystem::RemoveEntity(InstanceHandle& handle)
{
    mArtisans.erase(handle);
    mSoldiers.erase(handle);
    mMonsters.erase(handle);
    mPeople.erase(handle);
    mEntities.erase(handle);
}

void gamedev::GamelogicSystem::RemoveAllEntities()
{
    mArtisans.clear();
    mSoldiers.clear();
    mMonsters.clear();
    mPeople.clear();
    mEntities.clear();
}

void gamedev::GamelogicSystem::Init(std::shared_ptr<EngineECS>& ecs,
                                    std::shared_ptr<AudioSystem>& as,
                                    std::shared_ptr<UnitFactory>& uf,
                                    std::shared_ptr<StructureFactory>& sf,
                                    std::shared_ptr<ItemFactory>& itf,
                                    std::shared_ptr<GameObjects>& go,
                                    float globalScale)
{
    mECS = ecs;
    mAS = as;
    mUF = uf;
    mSF = sf;
    mIF = itf;
    mGO = go;
    mGlobalScale = globalScale;

    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::StaticCollision, GamelogicSystem::CollisionListener));
    mECS->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::ArrowHit, GamelogicSystem::ArrowListener));

    mAS->PlayLocalSound(tg::pos3(0, 0, 60), "water.wav", ambient, 0.4f, true, 10, 100);
    mAS->PlayLocalSound(tg::pos3(-80, 30, 90), "wind.wav", ambient, 0.5f, true, 20, 100);
    mAS->PlayLocalSound(tg::pos3(40, 0, -60), "crickets.wav", ambient, 0.5f, true, 20, 100);

    mAS->PlayGlobalSound("music.wav", music, 0.4, true);
} 

void gamedev::GamelogicSystem::AttachTerrain(std::shared_ptr<Terrain>& terrain)
{
    mTerrain = terrain;
}

int gamedev::GamelogicSystem::Update(float dt)
{
    auto t0 = std::chrono::steady_clock::now();

    mTime += dt;

    ConsumeWood(dt);
    UpdateOutlines(dt);

    for (const auto& h : mInjuredPioneers)
    {
        if (!mECS->IsLiveHandle(h))
        {
            mInjuredPioneers.erase(h);
        }
    }

    for (const auto& handle : mEntities)
    {
        const auto& instance = mECS->GetInstance(handle);

        auto lifeform = mECS->GetComponent<Living>(handle);
        auto attacker = mECS->TryGetComponent<Attacker>(handle);

        //if (attacker)
        //{
        //    if (attacker->forceAttack)
        //    {
        //        if (mECS->IsLiveHandle(attacker->attackOrder))
        //        {
        //            auto target = mECS->GetInstance(attacker->attackOrder);
        //            auto distance = tg::distance(instance.xform.translation, target.xform.translation;
        //            if (distance > attacker->weapon.range)
        //            {
        //                auto posMove = tg::normalize_safe(posT - pos) * (distance - range + 0.8f) + pos;
        //                SetMoveOrder(lifeform, posMove);
        //            }
        //
        //            SetMoveOrder(lifeform, mECS->GetInstanceTransform(attacker->attackOrder).translation)
        //        }
        //    }
        //}

        UpdateEffects(dt);

        // Do Casual
        UpdateForeignControl(instance, lifeform);
        UpdateMoveOrder(instance, lifeform, dt);
        UpdateFocus(instance, lifeform);

        // Do Productivity
        auto producer = mECS->TryGetComponent<Producer>(handle);
        if (producer)
        {
            Produce(producer, dt);
        }

        // Do Attacking
        if (attacker)
        {
            LookForEnemy(instance, lifeform, attacker);
            UpdateAttackOrder(instance, lifeform, attacker, dt);
            attacker->lastLookoutForEnemies++;

            // Tower Stuff
            UpdateTowerSoldier(instance, lifeform, attacker);
        }

        // Do Monster Stuff
        UpdateMonster(instance, lifeform, dt);

        if (mUpdateControl / 30)
            UpdateHomefire();
    }

    
    mUpdateControl++;

    auto tn = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(tn - t0).count();
}

void gamedev::GamelogicSystem::LookForEnemy(const Instance& instance, Living* lifeform, Attacker* attacker)
{
    if (attacker->attackOrder.is_valid() || attacker->lastLookoutForEnemies < 30)
    {
        return;
    }
    attacker->lastLookoutForEnemies = 0;

    // Look for instances in weapon range
    if (lifeform->friendly)
    {
        for (const auto& n : mMonsters)
        {
            auto lifeformT = mECS->GetComponent<Living>(n);

            auto attackerPosition = instance.xform.translation;
            auto targetPosition = mECS->GetInstanceTransform(n).translation;

            if (tg::distance(targetPosition, attackerPosition) <= attacker->weapon.range)
            {
                SetAttackOrder(lifeform, attacker, n);
                return;
            }
        }
    }
    // unfriendly
    else
    {
        auto range = tg::pos3(attacker->weapon.range + 1, 0.0, attacker->weapon.range + 1);
        for (const auto& n : mECS->GetHashMap().FindNear(instance.xform.translation, -range, range))
        {
            // monsters autonomously attack pioneers
            if (!mECS->TestSignature<Living>(n))
                continue;

            if (n == instance.mHandle)
                continue;

            auto lifeformT = mECS->TryGetComponent<Living>(n);

            if (lifeformT && !lifeformT->friendly)
                continue;

            if (lifeformT)
            {
                auto attackerPosition = instance.xform.translation;
                auto targetPosition = mECS->GetInstanceTransform(n).translation;
                auto distance = tg::distance(attackerPosition, targetPosition);

                SetAttackOrder(lifeform, attacker, n);
                auto moveTo = targetPosition + tg::normalize(attackerPosition - targetPosition) * (distance - attacker->weapon.range * 0.9);

                if (distance > attacker->weapon.range)
                    SetMoveOrder(lifeform, moveTo);

                return;
            }
        }
    }
    
}

void gamedev::GamelogicSystem::SetMoveOrder(Living* subject, tg::vec3 moveTo)
{
    subject->movePosition = tg::pos3(moveTo);
    subject->moveOrder = true;
    subject->forceMove = true;
}

void gamedev::GamelogicSystem::UpdateMoveOrder(const Instance& instance, Living* subject, float dt)
{
    if (!subject->moveOrder)
      return;

    if (mECS->TestSignature<InTower>(instance.mHandle))
    {
        return;
    }


    subject->idle = 0;

    // Face direction
    auto subject_dir = instance.xform.rotation_mat2D() * instance.facing;
    subject_dir.y = 0.0;

    // Target direction
    auto look_dir = tg::vec3(subject->movePosition - instance.xform.translation) * 5.f;
    look_dir.y = 0.0;

    // No focus: Look into target direction
    if (!subject->focusHandle.is_valid())
    {
        if (subject->forceMove || (tg::abs(1.f - tg::dot(subject_dir, look_dir)) > 1.f))
        {
            auto animation = mECS->TryGetComponent<Animated>(instance.mHandle);

            if (animation)
            {
                animation->rotYAnim = true;
                animation->rot_y = tg::angle_towards(subject_dir, look_dir, tg::vec3::unit_y);
            }
        }
    }

    // Update velocity if needed
    auto distance = tg::distance(subject->movePosition, tg::pos3(instance.xform.translation));
    if (subject->forceMove || (distance > 0.1))
    {
        look_dir = tg::normalize_safe(look_dir);
        auto physics = mECS->TryGetComponent<Physics>(instance.mHandle);

        if (physics)
            physics->velocity = (subject->moveSpeed * dt < distance) ? look_dir * subject->moveSpeed : look_dir * distance / dt;
    }
    // Otherwise stop
    else
    {
        subject->moveOrder = false;

        auto physics = mECS->TryGetComponent<Physics>(instance.mHandle);

        if (physics)
            physics->velocity = tg::vec3::zero;
    }

    // Force only once
    if (subject->forceMove)
        subject->forceMove = false;
}

void gamedev::GamelogicSystem::SetFocus(Living* subject, InstanceHandle focusHandle)
{
    subject->focusHandle = focusHandle;
}

void gamedev::GamelogicSystem::UpdateFocus(const Instance& instance, Living* subject)
{
    if (!subject->focusHandle.is_valid())
        return;

    if (!mECS->IsLiveHandle(subject->focusHandle))
    {
        subject->focusHandle._value = uint32_t(1);
        return;
    }

    // Face direction
    auto subject_dir = instance.xform.rotation_mat2D() * instance.facing;
    subject_dir.y = 0.0;

    // Focus direction
    const auto& focus = mECS->GetInstance(subject->focusHandle);
    auto look_to = focus.xform.translation - instance.xform.translation;
    look_to.y = 0.0;

    auto animation = mECS->TryGetComponent<Animated>(instance.mHandle);

    if (animation)
    {
        animation->rotYAnim = true;
        animation->rot_y = tg::angle_towards(subject_dir, look_to, tg::vec3::unit_y);
    }
}

void gamedev::GamelogicSystem::SetAttackOrder(Living* subject, Attacker* attacker, InstanceHandle target)
{
    subject->focusHandle = target;
    attacker->attackOrder = target;
}

void gamedev::GamelogicSystem::UpdateAttackOrder(const Instance& instance, Living* subject, Attacker* attacker, float dt)
{
    const int updatesPerSecond = 60;

    // Check if target handle is stale
    if (!mECS->IsLiveHandle(attacker->attackOrder))
    {
        subject->focusHandle._value = std::uint32_t(-1);
        attacker->attackOrder._value = std::uint32_t(-1);
    }

    auto animation = mECS->TryGetComponent<Animated>(instance.mHandle);

    // No target, but weapon: sheath weapon
    if (!attacker->attackOrder.is_valid())
    {
        attacker->idle++;

        if (attacker->idle >= 5 * updatesPerSecond)
        {
            //SheathWeapon(attacker);
        }

        MoveWeapon(instance, attacker);
        
        return;
    }

    attacker->idle = 0;

    // if (!attacker->attackOrder.is_valid())
    // {
    //     if (attacker->weaponHandle.is_valid())
    //     {
    //         SheathWeapon(attacker);
    //         MoveWeapon(instance, attacker);
    //     }
    // 
    //     return;
    // }


    // Business as usual:
    // Test Range
    auto targetInstance = mECS->GetInstance(attacker->attackOrder);
    if (tg::distance_sqr(targetInstance.xform.translation, instance.xform.translation) > tg::pow2(attacker->weapon.range))
    {
        subject->focusHandle._value = std::uint32_t(-1);
        attacker->attackOrder._value = std::uint32_t(-1);
        return;
    }

    // Enemy within range
    EquipWeapon(attacker);

    MoveWeapon(instance, attacker);

    AimWeapon(attacker);

    // Maybe shoot
    if (attacker->currentCooldown <= 0.f)
    {
        auto angle = tg::dot(instance.xform.forward_vec(), tg::normalize(targetInstance.xform.translation - instance.xform.translation));
        if (angle < 0.96)
            return;

        if (attacker->weapon.type == WeaponType::Bow)
            ShootArrow(attacker, dt);  
        else if (attacker->weapon.type == WeaponType::Claws)
            AttackClaw(attacker, dt);

        attacker->currentCooldown = attacker->weapon.cooldown;
    }
    else
    {
        attacker->currentCooldown -= dt;
    }
}

void gamedev::GamelogicSystem::AttackClaw(Attacker* attacker, float dt)
{
    auto mortal = mECS->TryGetComponent<Mortal>(attacker->attackOrder);
    if (mortal)
    {
        mInjuredPioneers.insert(attacker->attackOrder);

        mortal->health.current -= attacker->weapon.damage;

        Event e(EventType::PioneerHit, {attacker->handle_value}, attacker->attackOrder);
        mECS->SendFunctionalEvent(e);

        if (mortal->health.current <= 0.0f)
        {
            Kill(attacker->attackOrder, true, {attacker->handle_value});

            auto killedAttacker = mECS->TryGetComponent<Attacker>(attacker->attackOrder);
            if (killedAttacker && killedAttacker->weaponHandle.is_valid())
            {
                mECS->MarkDestruction(killedAttacker->weaponHandle);
            }
            auto position = mECS->GetInstanceTransform(attacker->attackOrder).translation;
            mAS->PlayLocalSound(tg::pos3(position), "claw4.wav", effect, 1.f);
        }
        else
        {
            vector<string> clawSounds = {"claw1.wav", "claw2.wav", "claw3.wav"};
            mAS->PlayLocalSound(attacker->attackOrder, clawSounds[int(RandomFloat(0, clawSounds.size() - 0.01))], effect, 1.f);
        }
    }

    auto destructible = mECS->TryGetComponent<Destructible>(attacker->attackOrder);
    if (destructible)
    {
        destructible->health.current -= attacker->weapon.damage;

        Event e(EventType::BuildingHit, {attacker->handle_value}, attacker->attackOrder);
        mECS->SendFunctionalEvent(e);

        if (destructible->health.current <= 0.0f)
        {
            Destroy(attacker->attackOrder, {attacker->handle_value});
        }
        else
        {
            vector<string> clawSounds = {"claw1.wav", "claw2.wav", "claw3.wav"};
            mAS->PlayLocalSound(attacker->attackOrder, clawSounds[int(RandomFloat(0, clawSounds.size() - 0.01))], effect, 1.f);
        }
    }
}

void gamedev::GamelogicSystem::ShootArrow(Attacker* attacker, float dt)
{
    auto arrowArc = GetArrowArc(attacker, dt);

    if (arrowArc.xMax <= 0.0f)
        return;

    const auto& instance = mECS->GetInstance({attacker->handle_value});
    const auto& bow = mECS->GetInstance(attacker->weaponHandle);
    const auto& target = mECS->GetInstance(attacker->attackOrder);

    auto arrowHandle = mIF->Create("arrow", tg::pos3(bow.xform.translation) + bow.xform.forward_vec() * 0.2);
    auto& arrow = mECS->GetInstance(arrowHandle);
    arrow.xform.translation.y = bow.xform.translation.y + 0.3f;
    auto dir = arrowArc.velocity;
    dir.y = 0.0;
    dir = tg::normalize(dir);
    arrow.xform.rotation = tg::quat::from_axis_angle(tg::dir3::pos_y, tg::angle_towards(arrow.xform.forward_vec(), dir, tg::vec3::unit_y));

    auto arrowComp = mECS->GetComponent<Arrow>(arrowHandle);
    arrowComp->target = attacker->attackOrder;
    arrowComp->shotBy = {attacker->handle_value};

    auto physicsComp = mECS->GetComponent<Physics>(arrowHandle);
    physicsComp->velocity = arrowArc.velocity;
    physicsComp->velocity.y = 0.0;
    physicsComp->forceGround = false;
    physicsComp->gravity = false;
    physicsComp->arrowArc = true;
    physicsComp->arc_x = 0.0;
    physicsComp->arc_xMax = arrowArc.xMax;
    physicsComp->arc_yMax = arrowArc.yMax;
    physicsComp->arc_yBase = arrowArc.yBase;

    auto particleComp = mECS->CreateComponent<ParticleEmitter>(arrowHandle);
    ParticleProperties pp;
    pp.baseColor = tg::color3::white * 0.6f;
    pp.baseLife = 0.3f;
    pp.basePosition = tg::pos3::zero;
    pp.baseSize = 0.02f;
    pp.baseVelocity = tg::vec3::unit_y * -0.05f;
    pp.varyColor = tg::color3::white * 0.9f;
    pp.varyLife = 0.0f;
    pp.varyPosition = tg::vec3(0.02, 0.02, 0.02);
    pp.varySize = 0.01f;
    pp.varyVelocity = tg::vec3(0.05f, 0.05f, 0.05f);
    pp.particlesPerSecond = 80.f;
    particleComp->pp.push_back(pp);

    vector<string> arrowSounds = {"arrow1.wav", "arrow2.wav", "arrow3.wav", "arrow4.wav"};
    mAS->PlayLocalSound(attacker->weaponHandle, arrowSounds[int(RandomFloat(0, arrowSounds.size() - 0.01))], effect, 1.f);
}

void gamedev::GamelogicSystem::EquipWeapon(Attacker* attacker)
{
    if (attacker->weapon.type == WeaponType::Bow)
    {
        if (!attacker->weaponHandle.is_valid())
        {
            attacker->weaponHandle = mIF->Create("bow");
            auto& weapon = mECS->GetInstance(attacker->weaponHandle);
            weapon.xform.scaling = tg::size3(0.0f);
        }

        // Animate..
        auto bowAnimation = mECS->TryGetComponent<Animated>(attacker->weaponHandle);
        if (bowAnimation)
        {
            bowAnimation->scaleAnim = true;
            bowAnimation->scale = 0.6f;
        }
        // ..or teleport
        else
        {
            mECS->GetInstanceTransform(attacker->weaponHandle).scaling = tg::size3(0.6f);
        }
    }
}

void gamedev::GamelogicSystem::SheathWeapon(Attacker* attacker)
{
    // No weapon
    if (!attacker->weaponHandle.is_valid())
    {
        return;
    }

    auto animation = mECS->TryGetComponent<Animated>(attacker->weaponHandle);

    // Animate..
    if (animation)
    {
        animation->scaleAnim = true;
        animation->scale = 0.f;
    }
    // ..or teleport
    else
    {
        mECS->GetInstanceTransform(attacker->weaponHandle).scaling = tg::size3(0.0f);
    }
}

void gamedev::GamelogicSystem::MoveWeapon(const Instance& instance, Attacker* attacker)
{
    // No weapon
    if (!attacker->weaponHandle.is_valid())
    {
        return;
    }

    auto& weapon = mECS->GetInstance(attacker->weaponHandle);
    auto animation = mECS->TryGetComponent<Animated>(attacker->weaponHandle);

    if (attacker->weapon.type == WeaponType::Bow)
    {
        if (animation)
        {
            animation->rotQuatAnim = true;
            animation->rot_quat = instance.xform.rotation;
        }

        weapon.xform.translation = instance.xform.translation + instance.xform.forward_vec() * 0.1 + instance.xform.right_vec() * 0.2;
        weapon.xform.translation.y = instance.xform.translation.y + 0.05f;
    }
}

void gamedev::GamelogicSystem::AimWeapon(Attacker* attacker)
{
    // No weapon
    if (!attacker->weaponHandle.is_valid())
    {
        return;
    }

    auto& instance = mECS->GetInstance({attacker->handle_value});
    auto& weapon = mECS->GetInstance(attacker->weaponHandle);
    auto animation = mECS->TryGetComponent<Animated>(attacker->weaponHandle);

    if (attacker->weapon.type == WeaponType::Bow)
    {
        weapon.xform.translation = instance.xform.translation + instance.xform.forward_vec() * 0.3 + instance.xform.right_vec() * 0.2;
        weapon.xform.translation.y = instance.xform.translation.y + 0.05f;

        if (animation)
        {
            animation->rotQuatAnim = true;
            animation->rot_quat = instance.xform.rotation * tg::quat::from_axis_angle(tg::dir3::pos_x, -45_deg);
        }
        else
        {
            weapon.xform.rotate(tg::quat::from_axis_angle(tg::dir3::pos_x, -45_deg));
        }
    }
}


// Inspired by:
gamedev::arcEq gamedev::GamelogicSystem::GetArrowArc(Attacker* attacker, float dt)
{
    // Get instances
    const auto& bow = mECS->GetInstance(attacker->weaponHandle);
    const auto& target = mECS->GetInstance(attacker->attackOrder);

    // Projectile speed + target velocity
    auto vm_a = attacker->weapon.missileSpeed;
    auto v_q = tg::vec3::zero;

    auto targetPhysics = mECS->TryGetComponent<Physics>(attacker->attackOrder);
    if (targetPhysics)
    {
        //v_q = targetPhysics->velocity;
        //v_q.y = 0;
        v_q = (target.xform.translation - targetPhysics->lastPosition) / dt;
        v_q.y = 0;
    }

    // Initial positions
    tg::vec3 p = bow.xform.translation;
    p.y = 0;
    tg::vec3 q0 = target.xform.translation;
    q0.y = 0;
    auto pq0 = q0 - p;

    // Less projectile speed if target is nearby
    auto lambdaDist = tg::saturate(tg::length(pq0) / 3.f);
    vm_a = tg::mix(tg::length(v_q) * 1.1f, vm_a, lambdaDist);

    // Calculate time until impact
    // (dot(v_q, v_q) - vm_a^2)t^2 + 2*dot(v_q, pq0)t + dot(pq0, pq0) = 0
    auto a = tg::dot(v_q, v_q) - vm_a * vm_a;
    auto b = 2.f * tg::dot(v_q, pq0);
    auto c = tg::dot(pq0, pq0);
    auto t1t2 = solveQuadratic(a, b, c);
    auto t = tg::max(t1t2.first, t1t2.second);

    // Time must not be in the past
    if (t <= 0.f)
        return {0.f, 0.f};

    // Future position (target)
    auto q1 = q0 + t * v_q;
    q1.y = 0;
    auto pq1 = q1 - p;

    // Let's fake some gravity
    auto arcLengthX = tg::length(pq1);
    // Less arc height if target is nearby
    auto lambdaRange = tg::saturate(arcLengthX / 2.0f);
    float arcHeightMax = tg::mix(1.5f, 3.f, lambdaRange);

    return {arcLengthX, arcHeightMax, bow.xform.translation.y, tg::normalize(pq1) * vm_a};
}

bool gamedev::GamelogicSystem::LookForProduction(InstanceHandle handle)
{
    if (mProductionBuildings.empty())
        return false;

    auto producer = mECS->GetComponent<Producer>(handle);
    producer->produceOrder = mProductionBuildings[int(RandomFloat(0, mProductionBuildings.size() - 0.01))];

    auto pBulding = mECS->GetInstance(producer->produceOrder);

    // SetMoveOrder(handle, tg::pos3(pBulding.xform.translation));

    return true;
}

void gamedev::GamelogicSystem::SpawnPlayer()
{
    mBonfire = mSF->Create("starting_bonfire", mGlobalScale * mPlayerStart);
    mAS->PlayLocalSound(mBonfire, "fire.wav", ambient, 1.0, true, 3, 50);
    
    {
        auto handle = mUF->Create("person_soldier", mGlobalScale * mPlayerStart - mGlobalScale * tg::vec3(1.0, 0.0, 0.0));
        mUnits.push_back(handle);
    }

    for (int i = 0; i < 1; i++)
    {
        tg::vec3 varyPos = tg::vec3::zero;
        varyPos.x = RandomFloat(0.5, 1.0);
        varyPos.z = RandomFloat(0.5, 1.0);

        auto handle = mUF->Create("person_soldier", mGlobalScale * mPlayerStart - mGlobalScale * tg::vec3(-1.0, 0.0, 0.0) + varyPos);
        mUnits.push_back(handle);
    }

    for (int i = 0; i < 1; i++)
    {
        tg::vec3 varyPos = tg::vec3::zero;
        varyPos.x = RandomFloat(0.5, 2.0);
        varyPos.z = RandomFloat(0.5, 2.0);

        auto handle = mUF->Create("person", mGlobalScale * mPlayerStart - mGlobalScale * tg::vec3(-1.0, 0.0, 0.0) + varyPos);
        mUnits.push_back(handle);
    }
    
    mHome.center = mGlobalScale * mPlayerStart;
    mHome.radius = 10;
    CalculateFreeSlots();

    UpdateHomefire();

    UpgradeFortifications();
    SpawnHouse(false);
    SpawnHouse(false);

    SetReinforcementWay();
    SetEnemySpawns();
    
    //TestReinforcementWay();
    //TestFreeSlots();
}

void gamedev::GamelogicSystem::SpawnForge()
{
    auto index = int(RandomFloat(0.0, mFreeOuterSlots.size() - 0.1));
    auto rotation = tg::angle_towards(tg::vec3::unit_z, mHome.center - mFreeOuterSlots[index], tg::vec3::unit_y);
    auto position = mFreeOuterSlots[index];
    position += tg::normalize(position - mHome.center) * 0.5;

    auto forge = mSF->Create("forge", position, tg::quat::from_axis_angle(tg::dir3::pos_y, rotation));

    std::swap(mFreeOuterSlots[index], mFreeOuterSlots.back());
    mFreeOuterSlots.pop_back();

    mOutlinedHandles.push_back(forge);
    mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));


    mForges++;
}

void gamedev::GamelogicSystem::SpawnHouse(bool highlighted)
{
    auto index = int(RandomFloat(0.0, mFreeInnerSlots.size() - 0.1));
    auto randomScale = tg::size3(gamedev::RandomFloat(0.8, 1.2));
    auto randomRotation = gamedev::RandomFloat() * 360_deg;

    auto tent = mSF->Create("tent0", mFreeInnerSlots[index], tg::quat::from_axis_angle(tg::dir3::pos_y, randomRotation));
    mECS->GetInstanceTransform(tent).scale(randomScale);

    std::swap(mFreeInnerSlots[index], mFreeInnerSlots.back());
    mFreeInnerSlots.pop_back();

    if (highlighted)
    {
        mOutlinedHandles.push_back(tent);
        mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));
    }
    

    mHomes++;
}

void gamedev::GamelogicSystem::SetEnemySpawns()
{
    mEnemySpawns.push_back(tg::pos3(10, 0, -44));
    mEnemySpawns.push_back(tg::pos3(23, 0, -24));
    mEnemySpawns.push_back(tg::pos3(41, 0, -4));
    mEnemySpawns.push_back(tg::pos3(-25, 0, 44));
    mEnemySpawns.push_back(tg::pos3(-40, 0, 15));
    mEnemySpawns.push_back(tg::pos3(-39, 0, 12));
}


void gamedev::GamelogicSystem::Produce(Producer* producer, float dt)
{
    //if (!producer->producing)
    //    return;

    mGold += producer->income * 3.f * dt + mForges * 3.f * 2.f * producer->income * dt;
}

void gamedev::GamelogicSystem::SpawnEnemyWave(int number) 
{ 
    //return;

    // random number generator
    //tg::rng rng;
    //rng.seed(0);

    //for (int i = 0; i < number; i++)
    //{
    //    auto direction = tg::uniform<tg::dir2>(rng);
    //    tg::pos3 position = mHome.center + mHome.radius * mGlobalScale * tg::vec3(direction.x * 3, 0, direction.y * 3);
    //    auto handle = mUF->Create("creature", position);
    //    mCreatures.push_back(handle);
    //
    //    auto lifeform = mECS->GetComponent<Living>(handle);
    //    lifeform->moveOrder = true;
    //    lifeform->movePosition = mHome.center;
    //}

    for (int i = 0; i < number; i++)
    {
        auto location = mEnemySpawns[int(RandomFloat(0.0, mEnemySpawns.size() - 0.1))];
        location += tg::vec3(RandomFloat(-2, 2), 0.0, RandomFloat(-2, 2));
        auto handle = mUF->Create("creature", location);
        mCreatures.push_back(handle);

        auto lifeform = mECS->GetComponent<Living>(handle);
        lifeform->moveOrder = true;
        lifeform->movePosition = mHome.center;
    }


    mAS->PlayGlobalSound("waveDrum1.wav", effect, 1.2f);
}

void gamedev::GamelogicSystem::SpawnReinforcements(int number) 
{
    if (GetPioneerCount() >= GetMaxPioneers())
        return;

    //mReinforcementsStart = mHome.center + mHome.radius * mGlobalScale * tg::vec3(0.0, 0.0, -1.0);
    mReinforcementsStart = mReinforcementWay[5];


    // random number generator
    tg::rng rng;
    rng.seed(0);

    for (int i = 0; i < number; i++)
    {
        auto direction = tg::uniform<tg::dir2>(rng);
        tg::pos3 position = mReinforcementsStart + tg::vec3(direction.x, 0, direction.y);
        auto handle = mUF->Create("person", position);

        auto fc = mECS->CreateComponent<ForeignControl>(handle);
        fc->waypoints = mReinforcementWay;
        fc->currentIndex = 5;

        //auto lifeform = mECS->GetComponent<Living>(handle);
        //lifeform->moveOrder = true;
        //lifeform->movePosition = mHome.center;
    }
}

void gamedev::GamelogicSystem::Select(InstanceHandle selected) { mSelected = selected; }
void gamedev::GamelogicSystem::Deselect() { mSelected = {std::uint32_t(-1)}; }

void gamedev::GamelogicSystem::Rightclick(tg::pos3 world_position)
{
    if (mSelected.is_valid())
    {
        auto lifeform = mECS->TryGetComponent<Living>(mSelected);

        if (lifeform)
        {
            //SetMoveOrder(mSelected, world_position);
            SetMoveOrder(lifeform, tg::vec3(world_position));
            mAS->PlayGlobalSound("interfaceClick.wav", ui, 0.7f);
        }
    }
}

void gamedev::GamelogicSystem::Rightclick(InstanceHandle handle)
{
    if (mSelected.is_valid() && handle.is_valid())
    {
        auto lifeform = mECS->TryGetComponent<Living>(mSelected);
        auto lifeformT = mECS->TryGetComponent<Living>(handle);

        if (lifeform)
        {
            auto pos = mECS->GetInstanceTransform(mSelected).translation;
            auto posT = mECS->GetInstanceTransform(handle).translation;
            float range = 0.2;

            auto attacker = mECS->TryGetComponent<Attacker>(mSelected);
            if (lifeformT && !lifeformT->friendly && attacker)
            {
                range = attacker->weapon.range * 0.9;
                SetAttackOrder(lifeform, attacker, handle);

                attacker->forceAttack = true;
            }

            auto distance = tg::distance(pos, posT);

            if (distance > range)
            {
                auto posMove = tg::normalize_safe(posT - pos) * (distance - range + 0.8f) + pos;
                SetMoveOrder(lifeform, posMove);
            }

            // SetMoveOrder(mSelected, world_position);
            mAS->PlayGlobalSound("interfaceClick.wav", ui, 0.7f);
        }

        mOutlinedHandles.push_back(handle);
        mOutlinedColors.push_back(tg::vec4(1.0, 0.0, 0.0, 1.0));
    }
}

void gamedev::GamelogicSystem::Kill(InstanceHandle killedHandle, bool killedPioneer, InstanceHandle killerHandle)
{

    if (killedPioneer)
    {
        Event e(EventType::PioneerDeath, killerHandle, killedHandle);
        mECS->SendFunctionalEvent(e);

        auto random = RandomFloat(0.0, 10.0);

        if (random < 1.0)
            mAS->PlayLocalSound(tg::pos3(mECS->GetInstanceTransform(killedHandle).translation), "wilhelm.wav", effect, 0.2, false, 3.0, 50.0);
        else
            mAS->PlayLocalSound(tg::pos3(mECS->GetInstanceTransform(killedHandle).translation), "pioneerDeathAlternative.wav", effect, 0.2, false, 3.0, 50.0);
    }
    else
    {
        Event e(EventType::MonsterDeath, killerHandle, killedHandle);
        mECS->SendFunctionalEvent(e);

        mAS->PlayLocalSound(tg::pos3(mECS->GetInstanceTransform(killedHandle).translation), "monsterDeath.wav", effect, 0.2, false, 3.0, 50.0);

    }

    mECS->MarkDestruction(killedHandle);
}

void gamedev::GamelogicSystem::Destroy(InstanceHandle destroyedHandle, InstanceHandle destroyerHandle)
{
    Event e(EventType::BuildingDestroyed, destroyerHandle, destroyedHandle);
    mECS->SendFunctionalEvent(e);

    
    auto position = mECS->GetInstanceTransform(destroyedHandle).translation;
    mAS->PlayLocalSound(tg::pos3(position), "explosion1.wav", effect, 1.f, false, 3, 50);

    auto attacker = mECS->TryGetComponent<Attacker>(destroyerHandle);
    if (attacker)
    {
        attacker->attackOrder = {std::uint32_t(-1)};
    }

    bool wall = false;

    for (const auto& w : mWalls)
        if (destroyedHandle == w)
        {
            wall = true;
            mDestroyedWalls++;
        }

    // Walls can be repaired
    if (wall)
        mECS->TryCreateComponent<Prototype>(destroyedHandle);
    // buildings must be rebuilt
    else
        mECS->MarkDestruction(destroyedHandle);
}

/*
void gamedev::GamelogicSystem::UpdateAttack(InstanceHandle handle, float dt)
{
    auto instance = mECS->GetInstance(handle);
    auto living = mECS->TryGetComponent<Living>(handle);
    auto attacker = mECS->TryGetComponent<Attacker>(handle);

    living->focusHandle = {std::uint32_t(-1)};

    float minDistance = attacker->weapon.range;
    attacker->attackOrder = {std::uint32_t(-1)};

    std::vector<InstanceHandle> enemies;

    if (living->friendly) 
    {
        enemies = mCreatures;

        // equip bow
        if (attacker->weaponHandle.is_valid())
        {
            auto& bowInstance = mECS->GetInstance(attacker->weaponHandle);
            bowInstance.xform.translation = tg::vec3(instance.xform.translation) + instance.xform.forward_vec() * 0.2;
            bowInstance.xform.translation.y = instance.xform.translation.y + 0.05f;

            auto t_facing = bowInstance.xform.rotation_mat2D() * tg::vec3(bowInstance.facing);
            auto look_dir = instance.xform.forward_vec();
            t_facing.y, look_dir.y = 0.0;
            auto angle = tg::angle_towards(t_facing, look_dir, tg::vec3::unit_y);
            bowInstance.xform.rotate(tg::quat::from_axis_angle(tg::dir3::pos_y, angle));
        }
        else
        {
            attacker->weaponHandle = mIF->Create("bow", tg::pos3(instance.xform.translation) + instance.xform.forward_vec() * 0.2);
            auto& bowInstance = mECS->GetInstance(attacker->weaponHandle);
            bowInstance.xform.translation.y = instance.xform.translation.y + 0.05f;
            bowInstance.xform.scale(0.6f);
        }
    }
    else
    {
        enemies = mUnits;
    }

    // find closest enemy
    for (const auto& enemy : enemies) 
    { 
        auto enemyInstance = mECS->GetInstance(enemy);

        float distance = tg::distance_sqr(tg::pos3(enemyInstance.xform.translation), tg::pos3(instance.xform.translation));

        if (distance < minDistance)
        {
            minDistance = distance;
            attacker->attackOrder = enemy;
        }
    }

    // attack
    if (attacker->attackOrder.is_valid())
    {
        living->focusHandle = attacker->attackOrder;

        auto target = mECS->GetInstance(attacker->attackOrder);

        // soldier
        if (living->friendly)
        {
            // shoot arrows
            if (attacker->currentCooldown <= 0)
            {
                auto arrow = mIF->Create("arrow", tg::pos3(instance.xform.translation) + instance.xform.forward_vec() * 0.2);
                //mArrows.push_back({arrow, 5.f});
                auto arrowComp = mECS->GetComponent<Arrow>(arrow);
                arrowComp->target = attacker->attackOrder;

                auto& arrowInstance = mECS->GetInstance(arrow);
                arrowInstance.xform.translation.y = instance.xform.translation.y + 0.3f;

                auto t_facing = arrowInstance.xform.rotation_mat2D() * tg::vec3(arrowInstance.facing);
                auto look_dir = tg::normalize_safe(target.xform.translation - arrowInstance.xform.translation);
                t_facing.y, look_dir.y = 0.0;
                auto angle = tg::angle_towards(t_facing, look_dir, tg::vec3::unit_y);
                arrowInstance.xform.rotate(tg::quat::from_axis_angle(tg::dir3::pos_y, angle));

                auto physicsComp = mECS->GetComponent<Physics>(arrow);
                physicsComp->gravity = false;
                physicsComp->velocity = tg::normalize_safe(tg::pos3(target.xform.translation) - tg::pos3(instance.xform.translation)) * 5;

                attacker->currentCooldown = attacker->weapon.cooldown * 50;
                mAS->PlayLocalSound(attacker->weaponHandle, "test_mono.wav");
            }
            else
            {
                attacker->currentCooldown -= dt;
            } 
        }
        else
        {
            // creature
        }
    }
}
*/

void gamedev::GamelogicSystem::CollisionListener(Event& e)
{
    auto lifeform = mECS->TryGetComponent<Living>(e.mSubject);

    if (lifeform)
    {
        auto position = tg::pos3(mECS->GetInstanceTransform(e.mSubject).translation);
        auto physics = mECS->TryGetComponent<Physics>(e.mSubject);

        auto movePosition = lifeform->movePosition;
        position.y = 0.f;
        movePosition.y = 0.f;

        if (physics && (tg::distance_sqr(position, movePosition) < 1.0))
        {
            physics->velocity = tg::vec3::zero;
            lifeform->moveOrder = false;
        }

        // Monster
        if (!lifeform->friendly)
        {
            auto attacker = mECS->TryGetComponent<Attacker>(e.mSubject);
            auto structure = mECS->TryGetComponent<Destructible>(e.mSender);

            if (structure && attacker)
            {
                lifeform->moveOrder = false;
                SetAttackOrder(lifeform, attacker, e.mSender);
            }
        }
    }
}

void gamedev::GamelogicSystem::ArrowListener(Event& e)
{
    auto arrow = mECS->TryGetComponent<Arrow>(e.mSender);
    auto lifeform = mECS->TryGetComponent<Living>(e.mSubject);
    auto mortal = mECS->TryGetComponent<Mortal>(e.mSubject);
    mortal->health.current -= arrow->damage;

    if (!lifeform->friendly)
    {
        Event e(EventType::MonsterHit, e.mSender, e.mSubject);
        mECS->SendFunctionalEvent(e);
    }

    if (arrow && lifeform && mortal && (mortal->health.current <= 0.0f))
    {
        Kill(e.mSubject, false, arrow->shotBy);
    }

    // Destroy arrow
    mECS->MarkDestruction(e.mSender);
}

// Algorithm:
// 1. Take 360� of a circle
// 2. Subtract the angles occupied by fixed segments (i.e. gate)
// 3. Divide remaining angles by the angles of the reoccuring segments (i.e. walls, pillars)
// 4. If the prior result was fractional, scale the gate to accomodate for the (missing) fractional wall
// 5. Spawn, rotate, spawn, rotate, ...
//
void gamedev::GamelogicSystem::SpawnStoneWalls()
{
    if (!mWalls.empty())
        DeleteFortifications();

    mAS->PlayGlobalSound("construction2.wav", effect, 0.5f);

    // Get the scalings of the used presets
    auto wScale = mECS->GetInstanceTransform(mSF->GetPreset("astonewall")).scaling;
    auto pScale = mECS->GetInstanceTransform(mSF->GetPreset("astonepillar")).scaling; 
    auto gScale = mECS->GetInstanceTransform(mSF->GetPreset("astonegate")).scaling; 

    // Get the original (unmodified) AABBs and scale them according to the presets
    auto wSize = wScale * tg::size_of(mGO->getAABB("astonewall"));
    auto pSize = pScale * tg::size_of(mGO->getAABB("astonepillar"));
    auto gSize = gScale * tg::size_of(mGO->getAABB("astonegate"));

    // Calculate all parameters (https://mathworld.wolfram.com/CircularSegment.html)
    // Walls
    float wLength = wSize.width;                                          // segment length
    auto wTheta = 2.f * tg::asin(wLength / (2.f * mHome.radius));         // angle spanned by segment
    auto wOffs = mHome.radius - (mHome.radius * tg::cos(wTheta / 2.f));   // offset from the circles boundary, all segments will reside completely within the circle

    // Pillars
    float pLength = pSize.width * 0.6;                                    // pretend they are slimmer so they will nicely overlap with the walls
    auto pTheta = 2.f * tg::asin(pLength / (2.f * mHome.radius));
    auto pOffs = mHome.radius - (mHome.radius * tg::cos(pTheta / 2.f));

    // Gate
    float gLength = gSize.width;
    auto gTheta = 2.f * tg::asin(gLength / (2.f * mHome.radius));
    auto gOffs = mHome.radius - (mHome.radius * tg::cos(gTheta / 2.f));
    tg::size3 gScaleFix(1.0);                                                // prepare the fixing of the gates scale if a fractional result of wSegments remains

    // Start with the circles 360� and subtract all elements angles which are not covered by the following for-loop
    auto circumcircle = 360_deg;
    circumcircle -= gTheta;                             // Gate
    circumcircle -= 2 * wTheta;                         // Usually: Walls without a successive pillar. BUT: It looks better if set to 2 instead of 1, idk
    auto wSegments = circumcircle / (wTheta + pTheta);  // Count how many wall segments (including a pillar) can be spawned

    // If it does not fit quite well, scale the gate
    if (tg::fract(wSegments) > 0.001)
    {
        // Revert subtraction of gate angle
        circumcircle += gTheta;

        // Calculate bigger gate angle
        gTheta += circumcircle - (int(wSegments) * (wTheta + pTheta));

        // Calculate resulting scaling factor
        gScaleFix.width = (2 * mHome.radius * tg::sin(gTheta / 2.f)) / gLength + 0.1; // Make it a little bit bigger so that adjacent walls overlap
        gLength = gScaleFix.width * gLength;
        gScaleFix.height = 1.0f + (gScaleFix.width - 1.0f) * 0.6f;
        gScaleFix.depth = 1.0f + (gScaleFix.width - 1.0f) * 0.6f;
        
        // Subtract (fixed) gate angle again
        circumcircle -= gTheta;

        // Recalculate segments
        wSegments = circumcircle / (wTheta + pTheta);
    }

    // Add the prior subtracted wall segments for the for-loop
    wSegments += 2;

    // Spawn gate
    tg::vec3 translation =  mHome.radius * tg::vec3::unit_z * -1;
    auto gHandle = mSF->Create("astonegate", mHome.center + translation - gOffs);
    mECS->GetInstanceTransform(gHandle).scale(gScaleFix);

    mWalls.push_back(gHandle);

    // Calculate (Half-)Rotation Matrices
    tg::mat4 wRot = tg::rotation_y(wTheta);
    tg::mat4 pRot = tg::rotation_y(pTheta);
    tg::mat4 gRot = tg::rotation_y(gTheta);

    tg::mat4 wRotHalf = tg::rotation_y(wTheta/2.f);
    tg::mat4 pRotHalf = tg::rotation_y(pTheta/2.f);
    tg::mat4 gRotHalf = tg::rotation_y(gTheta/2.f);

    // Apply Rotation for the first wall segment
    translation = wRotHalf * gRotHalf * translation;

    int p = 0;
    int w = 0;
    for (int i = 0; i < int(wSegments) - 1; i++)
    {
        mWalls.push_back(mSF->Create("astonewall", mHome.center + translation - wOffs, tg::quat::from_axis_angle(tg::dir3::pos_y, (gTheta/2.f + wTheta/2.f) + w * (wTheta/2.f + pTheta/2.f) + p * (pTheta/2.f + wTheta/2.f))));
        translation = pRotHalf * wRotHalf * translation;

        mOutlinedHandles.push_back(mWalls.back());
        mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));

        w++;

        mWalls.push_back(mSF->Create("astonepillar", mHome.center + translation - pOffs, tg::quat::from_axis_angle(tg::dir3::pos_y, (gTheta/2.f + wTheta/2.f) + w * (wTheta/2.f + pTheta/2.f) + p * (pTheta/2.f + wTheta/2.f))));
        translation = wRotHalf * pRotHalf * translation;

        mOutlinedHandles.push_back(mWalls.back());
        mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));
        
        p++;
    }
    // Last wall segment, without a consecutive pillar
    mWalls.push_back(mSF->Create("astonewall", mHome.center + translation, tg::quat::from_axis_angle(tg::dir3::pos_y, (gTheta/2.f + wTheta/2.f) + w * (wTheta/2.f + pTheta/2.f) + p * (pTheta/2.f + wTheta/2.f))));

    mOutlinedHandles.push_back(mWalls.back());
    mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));

    w++;
}

void gamedev::GamelogicSystem::SpawnWoodWalls()
{
    if (!mWalls.empty())
        DeleteFortifications();

    mAS->PlayGlobalSound("construction1.wav", effect, 0.5f);

    // Get the scalings of the used presets
    auto wScale = mECS->GetInstanceTransform(mSF->GetPreset("aFence0")).scaling;
    auto gScale = mECS->GetInstanceTransform(mSF->GetPreset("astonegate")).scaling;   // Placeholder

    // Get the original (unmodified) AABBs and scale them according to the presets
    auto wSize = wScale * tg::size_of(mGO->getAABB("aFence0"));
    auto gSize = gScale * tg::size_of(mGO->getAABB("astonegate"));

    // Calculate all parameters (https://mathworld.wolfram.com/CircularSegment.html)
    // Walls
    float wLength = wSize.width;
    auto wTheta = 2.f * tg::asin(wLength / (2.f * mHome.radius));
    auto wOffs = mHome.radius - (mHome.radius * tg::cos(wTheta / 2.f));

    // Gate (Placeholder)
    float gLength = gSize.width;
    auto gTheta = 2.f * tg::asin(gLength / (2.f * mHome.radius));
    auto gOffs = mHome.radius - (mHome.radius * tg::cos(gTheta / 2.f));
    tg::size3 gScaleFix(1.0);

    auto circumcircle = 360_deg;
    circumcircle -= gTheta;
    circumcircle -= 2 * wTheta;
    auto wSegments = circumcircle / (wTheta);

    if (tg::fract(wSegments) > 0.001)
    {
        circumcircle += gTheta;

        gTheta += circumcircle - (int(wSegments) * wTheta);

        gScaleFix.width = (2 * mHome.radius * tg::sin(gTheta / 2.f)) / gLength + 0.1; // Make it a little bit bigger so that adjacent walls overlap
        gLength = gScaleFix.width * gLength;
        gScaleFix.height = 1.0f + (gScaleFix.width - 1.0f) * 0.6f;
        gScaleFix.depth = 1.0f + (gScaleFix.width - 1.0f) * 0.6f;

        circumcircle -= gTheta;

        wSegments = circumcircle / (wTheta);
    }

    // Add the prior subtracted wall segments for the for-loop
    wSegments += 2;

    tg::vec3 translation = mHome.radius * tg::vec3::unit_z * -1;

    // Calculate (Half-)Rotation Matrices
    tg::mat4 wRot = tg::rotation_y(wTheta);
    tg::mat4 gRot = tg::rotation_y(gTheta);

    tg::mat4 wRotHalf = tg::rotation_y(wTheta / 2.f);
    tg::mat4 gRotHalf = tg::rotation_y(gTheta / 2.f);

    // Apply Rotation for the first wall segment
    translation = wRotHalf * gRotHalf * translation;

    int p = 0;
    int w = 0;
    for (int i = 0; i < int(wSegments) - 1; i++)
    {
        mWalls.push_back(mSF->Create("aFence0", mHome.center + translation - wOffs,
                    tg::quat::from_axis_angle(tg::dir3::pos_y,
                                              (gTheta / 2.f + wTheta / 2.f) + w * wTheta)));

        mOutlinedHandles.push_back(mWalls.back());
        mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));

        translation = wRot * translation;
        w++;
    }
    // Last wall segment, without a consecutive pillar
    mWalls.push_back(mSF->Create("aFence0", mHome.center + translation,
                tg::quat::from_axis_angle(tg::dir3::pos_y, (gTheta / 2.f + wTheta / 2.f) + w * wTheta)));

    mOutlinedHandles.push_back(mWalls.back());
    mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));

    w++;
}

void gamedev::GamelogicSystem::SpawnSpikes()
{
    mAS->PlayGlobalSound("construction3.wav", effect, 0.5f);

    float homeRadius = mHome.radius;

    // Get the scalings of the used presets
    auto swScale = mECS->GetInstanceTransform(mSF->GetPreset("astonewall")).scaling;
    auto wScale = mECS->GetInstanceTransform(mSF->GetPreset("aspikebarriers")).scaling;
    auto gScale = mECS->GetInstanceTransform(mSF->GetPreset("astonegate")).scaling; // Placeholder

    // Get the original (unmodified) AABBs and scale them according to the presets
    auto swSize = swScale * tg::size_of(mGO->getAABB("astonewall"));
    auto wSize = wScale * tg::size_of(mGO->getAABB("aspikebarriers"));
    auto gSize = gScale * tg::size_of(mGO->getAABB("astonegate"));

    // Stone Walls (just for the measure)
    float swLength = swSize.width;
    auto swTheta = 2.f * tg::asin(swLength / (2.f * mHome.radius));
    auto swOffs = mHome.radius - (mHome.radius * tg::cos(swTheta / 2.f));

    homeRadius += swSize.depth;

    // Calculate all parameters (https://mathworld.wolfram.com/CircularSegment.html)
    // Walls
    float wLength = wSize.width;
    auto wTheta = 2.f * tg::asin(wLength / (2.f * mHome.radius));
    auto wOffs = homeRadius - (homeRadius * tg::cos(wTheta / 2.f));

    // Gate (Placeholder)
    float gLength = gSize.width;
    auto gTheta = 2.f * tg::asin(gLength / (2.f * mHome.radius));
    auto gOffs = homeRadius - (homeRadius * tg::cos(gTheta / 2.f));
    tg::size3 gScaleFix(1.0);

    auto circumcircle = 360_deg;
    circumcircle -= gTheta;
    circumcircle -= 2 * wTheta;
    auto wSegments = circumcircle / (wTheta);

    if (tg::fract(wSegments) > 0.001)
    {
        circumcircle += gTheta;

        gTheta += circumcircle - (int(wSegments) * wTheta);

        gScaleFix.width = (2 * homeRadius * tg::sin(gTheta / 2.f)) / gLength + 0.1; // Make it a little bit bigger so that adjacent walls overlap
        gLength = gScaleFix.width * gLength;
        gScaleFix.height = 1.0f + (gScaleFix.width - 1.0f) * 0.6f;
        gScaleFix.depth = 1.0f + (gScaleFix.width - 1.0f) * 0.6f;

        circumcircle -= gTheta;

        wSegments = circumcircle / (wTheta);
    }

    // Add the prior subtracted wall segments for the for-loop
    wSegments += 2;

    tg::vec3 translation = homeRadius * tg::vec3::unit_z * -1;

    // Calculate (Half-)Rotation Matrices
    tg::mat4 wRot = tg::rotation_y(wTheta);
    tg::mat4 gRot = tg::rotation_y(gTheta);

    tg::mat4 wRotHalf = tg::rotation_y(wTheta / 2.f);
    tg::mat4 gRotHalf = tg::rotation_y(gTheta / 2.f);

    // Apply Rotation for the first wall segment
    translation = wRotHalf * gRotHalf * translation;

    int p = 0;
    int w = 0;
    for (int i = 0; i < int(wSegments) - 1; i++)
    {
        auto spikes = mSF->Create("aspikebarriers", mHome.center + translation - wOffs, tg::quat::from_axis_angle(tg::dir3::pos_y, (gTheta / 2.f + wTheta / 2.f) + w * wTheta + 180_deg));
        mECS->GetInstanceTransform(spikes).move_absolute(tg::vec3::unit_y * (-0.2 * wSize.height));
        mWalls.push_back(spikes);

        mOutlinedHandles.push_back(mWalls.back());
        mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));

        auto e = Event(EventType::NewPosition, spikes, spikes);
        mECS->SendFunctionalEvent(e);

        translation = wRot * translation;
        w++;
    }
    // Last wall segment, without a consecutive pillar
    auto spikes = mSF->Create("aspikebarriers", mHome.center + translation,
                              tg::quat::from_axis_angle(tg::dir3::pos_y, (gTheta / 2.f + wTheta / 2.f) + w * wTheta + 180_deg));
    mWalls.push_back(spikes);
    mECS->GetInstanceTransform(spikes).move_absolute(tg::vec3::unit_y * (-0.2 * wSize.height));

    mOutlinedHandles.push_back(mWalls.back());
    mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));

    auto e = Event(EventType::NewPosition, spikes, spikes);
    mECS->SendFunctionalEvent(e);


    w++;
}

void gamedev::GamelogicSystem::StartProduction(InstanceHandle handle)
{
    auto instance = mECS->GetInstance(handle);
    auto lifeform = mECS->TryGetComponent<Living>(handle);
    auto producer = mECS->TryGetComponent<Producer>(handle);
    auto particles = mECS->TryGetComponent<ParticleEmitter>(handle);

    if ((!lifeform) || (lifeform->moveOrder))
        return;

    if (particles && !particles->pp.empty())
        return;

    ParticleProperties pp;

    pp.baseColor = {255, 204, 153};
    pp.baseLife = 1.5;
    pp.basePosition = tg::pos3::zero + instance.xform.translation + tg::vec3::unit_y * 0.5;
    pp.baseSize = 0.5;
    pp.baseVelocity = tg::vec3::unit_y;

    pp.varyColor = {0, 0, 0};
    pp.varyLife = 0.5;
    pp.varyPosition = tg::vec3{0.2, 0.2, 0.2};
    pp.varySize = 0.4;
    pp.varyVelocity = tg::vec3{0.3, 0.2, 0.3};
    
    pp.particlesPerSecond = 20;

    if (!particles)
        particles = mECS->CreateComponent<ParticleEmitter>(handle);
    
    particles->pp.push_back(pp);
}

void gamedev::GamelogicSystem::StopProduction(InstanceHandle handle)
{
    mECS->TryRemoveComponent<Producer>(handle);
    mECS->GetComponent<Producer>(handle)->producing = false;
}

gamedev::InstanceHandle gamedev::GamelogicSystem::SpawnObjectRandomly(tg::pos3 position, std::string object_id)
{
    auto randomSize= tg::size3(RandomFloat(1.0, 1.4));
    auto randomAngle = RandomFloat() * 360_deg;

    auto handle = mSF->Create(object_id, position, tg::quat::from_axis_angle(tg::dir3::pos_y, randomAngle));
    mECS->GetInstanceTransform(handle).scale(randomSize);
    return handle;
}

gamedev::InstanceHandle gamedev::GamelogicSystem::TryConstructObject(std::string object_id)
{
    auto instance = mECS->GetInstance(mSF->GetPreset(object_id));

    // You have 20 tries to find a free spot, good luck
    for (int i = 0; i < 20; i++)
    {
        tg::pos3 randomPosition
            = mHome.center + RandomFloat(1.0, mHome.radius - 1.0) * tg::vec3::unit_x + RandomFloat(1.0, mHome.radius - 1.0) * tg::vec3::unit_z;

        auto potential_colliders = mECS->GetHashMap().FindNear(tg::vec3(randomPosition), instance.max_bounds);
        
        bool freeSpot = true;
        for (const auto& pc : potential_colliders)
        {
            if (tg::intersects(instance.max_bounds, mECS->GetInstance(pc).max_bounds))
            {
                freeSpot = false;
            }
        }

        if (freeSpot)
            return SpawnObjectRandomly(randomPosition, object_id);
    }

    return {std::uint32_t(-1)};
}

gamedev::InstanceHandle gamedev::GamelogicSystem::TryConstructRandomObject(std::vector<std::string> object_ids)
{
    auto instance = mECS->GetInstance(mSF->GetPreset(object_ids[0]));

    // You have 20 tries to find a free spot, good luck
    for (int i = 0; i < 20; i++)
    {
        tg::pos3 randomPosition = mHome.center + RandomFloat(-mHome.radius + 1, mHome.radius - 1.0) * tg::vec3::unit_x
                                  + RandomFloat(-mHome.radius + 1, mHome.radius - 1.0) * tg::vec3::unit_z;

        auto potential_colliders = mECS->GetHashMap().FindNear(tg::vec3(randomPosition), instance.max_bounds);
        
        bool freeSpot = true;
        for (const auto& pc : potential_colliders)
        {
            if (tg::intersects(instance.max_bounds, mECS->GetInstance(pc).max_bounds))
            {
                freeSpot = false;
            }
        }

        if (freeSpot)
            return SpawnObjectRandomly(randomPosition, object_ids[int(RandomFloat(0, object_ids.size() - 0.01))]);
    }

    return {std::uint32_t(-1)};
}

gamedev::InstanceHandle gamedev::GamelogicSystem::TryConstructHome()
{
    auto newHome = TryConstructRandomObject({"tent0", "tent1"});

    if (newHome.is_valid())
    {
        mDwellings.push_back(newHome);
    }

    return newHome;
}

bool gamedev::GamelogicSystem::WithinFort(InstanceHandle handle)
{
    auto position = tg::pos3(mECS->GetInstanceTransform(handle).translation);

    if (tg::distance_sqr(position, mHome.center) < mHome.radius * mHome.radius)
        return true;

    return false;
}

void gamedev::GamelogicSystem::SpawnWoodStartingFences()
{
    if (!mWalls.empty())
        DeleteFortifications();

    // Get the scalings of the used presets
    auto wScale = mECS->GetInstanceTransform(mSF->GetPreset("aFence1")).scaling;
    auto gScale = mECS->GetInstanceTransform(mSF->GetPreset("astonegate")).scaling; // Placeholder

    // Get the original (unmodified) AABBs and scale them according to the presets
    auto wSize = wScale * tg::size_of(mGO->getAABB("aFence1"));
    auto gSize = gScale * tg::size_of(mGO->getAABB("astonegate"));

    // Calculate all parameters (https://mathworld.wolfram.com/CircularSegment.html)
    // Walls
    float wLength = wSize.width;
    auto wTheta = 2.f * tg::asin(wLength / (2.f * mHome.radius));
    auto wOffs = mHome.radius - (mHome.radius * tg::cos(wTheta / 2.f));

    // Gate (Placeholder)
    float gLength = gSize.width;
    auto gTheta = 2.f * tg::asin(gLength / (2.f * mHome.radius));
    auto gOffs = mHome.radius - (mHome.radius * tg::cos(gTheta / 2.f));
    tg::size3 gScaleFix(1.0);

    auto circumcircle = 360_deg;
    circumcircle -= gTheta;
    circumcircle -= 2 * wTheta;
    auto wSegments = circumcircle / (wTheta);

    if (tg::fract(wSegments) > 0.001)
    {
        circumcircle += gTheta;

        gTheta += circumcircle - (int(wSegments) * wTheta);

        gScaleFix.width = (2 * mHome.radius * tg::sin(gTheta / 2.f)) / gLength + 0.1; // Make it a little bit bigger so that adjacent walls overlap
        gLength = gScaleFix.width * gLength;
        gScaleFix.height = 1.0f + (gScaleFix.width - 1.0f) * 0.6f;
        gScaleFix.depth = 1.0f + (gScaleFix.width - 1.0f) * 0.6f;

        circumcircle -= gTheta;

        wSegments = circumcircle / (wTheta);
    }

    // The fences are placed sparsely
    wSegments /= 6;

    wSegments += 0;

    tg::vec3 translation = mHome.radius * tg::vec3::unit_z * -1;

    tg::mat4 wRotLarge = tg::rotation_y(6.f * wTheta);

    tg::mat4 wRotHalf = tg::rotation_y(wTheta / 2.f);
    tg::mat4 gRotHalf = tg::rotation_y(gTheta / 2.f);

    translation = wRotHalf * gRotHalf * translation;

    int p = 0;
    int w = 0;
    for (int i = 0; i < int(wSegments) - 1; i++)
    {
        auto randomness = RandomFloat() * 10_deg;

        mWalls.push_back(mSF->Create("aFence1", mHome.center + translation - wOffs, tg::quat::from_axis_angle(tg::dir3::pos_y, (gTheta / 2.f + wTheta / 2.f) + w * wTheta * 6.f + randomness)));

        randomness = RandomFloat() * 2_deg;

        translation = tg::rotation_y(6.f * wTheta + randomness) * translation;
        w++;
    }
    mWalls.push_back(mSF->Create("aFence1", mHome.center + translation,
                                 tg::quat::from_axis_angle(tg::dir3::pos_y, (gTheta / 2.f + wTheta / 2.f) + w * wTheta * 6.f)));

    w++;
}

int gamedev::GamelogicSystem::GetPioneerCount() { return mPeople.size() + mArtisans.size() + mSoldiers.size(); }
int gamedev::GamelogicSystem::GetArtisanCount() { return mArtisans.size(); }
int gamedev::GamelogicSystem::GetSoldierCount() { return mSoldiers.size(); }
std::string gamedev::GamelogicSystem::GetFortificationUpgrade()
{
  switch (mFortificationLevel)
  {
  case 0:
      return "Fences";

  case 1:
      return "Wooden Barricades";

  case 2:
      return "Stonewalls";
    
  case 3:
      return "Add Spikes";

  default:
      return "None left!";
  }
}

void gamedev::GamelogicSystem::DeleteFortifications()
{
  for (const auto& handle : mWalls)
  {
      if (mECS->IsLiveHandle(handle))
          mECS->DestroyInstance(handle);
  }

  mWalls.clear();
}

void gamedev::GamelogicSystem::UpgradeFortifications()
{
    auto costs = GetFortificationPrice();

    if (!Pay(GetFortificationPrice()))
    {
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        return;
    }

    switch (mFortificationLevel)
    {
    case 0:
        SpawnWoodStartingFences();
        break;

    case 1:
        SpawnWoodWalls();
        break;

    case 2:
        SpawnStoneWalls();
        break;

    case 3:
        SpawnSpikes();
        break;

    default:
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        break;
    }

    mFortificationLevel++;
}

void gamedev::GamelogicSystem::AssignArtisan()
{
    if (mPeople.empty())
    {
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        return;
    }

    if(!Pay(GetArtisanPrice()))
    {
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        return;
    }

    bool assigned = false;

    for (auto h : mPeople)
    {
        auto& instance = mECS->GetInstance(h);

        if (mECS->TestSignature<ForeignControl>(h) && tg::distance_sqr(instance.xform.translation, tg::vec3(mHome.center)) > mHome.radius * mHome.radius + 5.0)
        {
            continue;
        }
        
        const auto& filthyPeasant = mECS->GetInstance(h);
        auto artisan = mUF->Create("person_artisan", tg::pos3(filthyPeasant.xform.translation), filthyPeasant.xform.rotation, filthyPeasant.xform.scaling);
        mECS->MarkDestruction(filthyPeasant.mHandle);

        mOutlinedHandles.push_back(artisan);
        mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));

        mAS->PlayGlobalSound("interfaceClick.wav", ui);

        assigned = true;
        return;
    }

    if (!assigned)
    {
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        mGold += GetArtisanPrice();
    }

    //auto first = mPeople.begin();
    //
    //const auto& filthyPeasant = mECS->GetInstance(*first);
    //auto artisan = mUF->Create("person_artisan", tg::pos3(filthyPeasant.xform.translation), filthyPeasant.xform.rotation, filthyPeasant.xform.scaling);
    //mECS->MarkDestruction(filthyPeasant.mHandle);
    //
    //mOutlinedHandles.push_back(artisan);
    //mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));
    //
    //mAS->PlayGlobalSound("interfaceClick.wav", ui);
}
void gamedev::GamelogicSystem::AssignSoldier()
{
    if (mPeople.empty())
    {
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        return;
    }

    if (!Pay(GetSoldierPrice()))
    {
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        return;
    }

    bool assigned = false;

    for (auto h : mPeople)
    {
        auto& instance = mECS->GetInstance(h);

        if (mECS->TestSignature<ForeignControl>(h) && tg::distance_sqr(instance.xform.translation, tg::vec3(mHome.center)) > mHome.radius * mHome.radius + 5.0)
        {
            continue;
        }

        const auto& filthyPeasant = mECS->GetInstance(h);
        auto soldier = mUF->Create("person_soldier", tg::pos3(filthyPeasant.xform.translation), filthyPeasant.xform.rotation, filthyPeasant.xform.scaling);
        mECS->MarkDestruction(filthyPeasant.mHandle);

        mOutlinedHandles.push_back(soldier);
        mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));

        mAS->PlayGlobalSound("interfaceClick.wav", ui);

        assigned = true;
        return;
    }

    if (!assigned)
    {
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        mGold += GetArtisanPrice();
    }

    //auto first = mPeople.begin();
    //
    //const auto& filthyPeasant = mECS->GetInstance(*first);
    //auto soldier = mUF->Create("person_soldier", tg::pos3(filthyPeasant.xform.translation), filthyPeasant.xform.rotation, filthyPeasant.xform.scaling);
    //mECS->MarkDestruction(filthyPeasant.mHandle);
    //
    //mOutlinedHandles.push_back(soldier);
    //mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));
    //
    //mAS->PlayGlobalSound("interfaceClick.wav", ui);
}

int gamedev::GamelogicSystem::GetGold() { return tg::ifloor(mGold); }

int gamedev::GamelogicSystem::GetArtisanPrice() { return 5; }
int gamedev::GamelogicSystem::GetSoldierPrice() { return 5; }
int gamedev::GamelogicSystem::GetFortificationPrice()
{
    switch (mFortificationLevel)
    {
    case 0:
        return 0;

    case 1:
        return 10;

    case 2:
        return 20;

    case 3:
        return 10;
    }

    return 0;
}

bool gamedev::GamelogicSystem::Pay(int gold)
{
    if (mGold < gold)
    {
        return false;
    }
    
    mGold -= gold;
    return true;
}

void gamedev::GamelogicSystem::CRAZY_CHEAT_YOU_WONT_BELIEVE()
{
    glow::log() << "motherlode";
    mGold += 20;
}

void gamedev::GamelogicSystem::UpdateHomefire()
{
    float light_intensity;
    float velo_intensity;
    float size_intensity;
    float base_lifetime;
    int particles_per_second;

    auto oldLevel = mFireLevel;

    const float reference_light_intensity = 30.0;

    const tg::vec3 reference_fire_velocity = tg::vec3(0.0, 1.0, 0.0);
    const tg::vec3 reference_ember_velocity = tg::vec3(0.0, 1.2, 0.0);

    const tg::vec3 reference_fire_vary_velocity = tg::vec3(0.1, 0.2, 0.1);
    const tg::vec3 reference_ember_vary_velocity = tg::vec3(0.1, 0.3, 0.1);

    if (mFireWood > 100.f)
    {
        mFireLevel = 6;
        velo_intensity = 1.6;
        size_intensity = 0.14;
        base_lifetime = 1.4;
        particles_per_second = 181;

        light_intensity = 2.5;

        mDesaturationIntensityTarget = 0.0;
        mVignetteRadiusTarget = 0.9;
    }
    else if (mFireWood > 80.f)
    {
        mFireLevel = 5;
        velo_intensity = 1.4;
        size_intensity = 0.13;
        base_lifetime = 1.2;
        particles_per_second = 161;

        light_intensity = 2.1;

        mDesaturationIntensityTarget = 0.0;
        mVignetteRadiusTarget = 0.9;

    }
    else if (mFireWood > 60.f)
    {
        mFireLevel = 4;
        velo_intensity = 1.2;
        size_intensity = 0.12;
        base_lifetime = 1.2;
        particles_per_second = 141;

        light_intensity = 1.7;

        mDesaturationIntensityTarget = 0.1;
        mVignetteRadiusTarget = 0.87;
    }
    else if (mFireWood > 40.f)
    {
        mFireLevel = 3;
        velo_intensity = 1.0;
        size_intensity = 0.11;
        base_lifetime = 1.1;
        particles_per_second = 121;

        light_intensity = 1.3;

        mDesaturationIntensityTarget = 0.2;
        mVignetteRadiusTarget = 0.85;

    }
    else if (mFireWood > 20.f)
    {
        mFireLevel = 2;
        velo_intensity = 0.8;
        size_intensity = 0.1;
        base_lifetime = 1.1;
        particles_per_second = 100;

        light_intensity = 0.7;

        mDesaturationIntensityTarget = 0.3;
        mVignetteRadiusTarget = 0.8;
    }
    else if (mFireWood > 0.f)
    {
        mFireLevel = 1;
        velo_intensity = 0.5;
        size_intensity = 0.07;
        base_lifetime = 1.0;
        particles_per_second = 50;

        light_intensity = 0.3;

        mDesaturationIntensityTarget = 0.5;
        mVignetteRadiusTarget = 0.5;

        if (mFireWood > 10.0)
        {
            mDesaturationIntensityTarget = 0.4;
            mVignetteRadiusTarget = 0.7;
        }
    }
    else if (mFireWood <= 0.f)
    {
        mFireLevel = 0;
        velo_intensity = 0.5;
        size_intensity = 0.01;
        base_lifetime = 2.f;
        particles_per_second = 12;

        light_intensity = 0.0;

        mDesaturationIntensityTarget = 0.8;
        mVignetteRadiusTarget = 0.35;
    }

    if (oldLevel != mFireLevel)
    {
        auto pe = mECS->TryGetComponent<ParticleEmitter>(mBonfire);
        if (pe)
        {
            auto& fire = pe->pp[0];
            auto& ember = pe->pp[1];

            fire.baseLife = base_lifetime;
            fire.baseVelocity = reference_fire_velocity * velo_intensity;
            fire.varyVelocity = reference_fire_vary_velocity * velo_intensity;
            fire.particlesPerSecond = particles_per_second;

            ember.baseLife = 1.1 * base_lifetime;
            ember.baseVelocity = reference_ember_velocity * velo_intensity;
            ember.varyVelocity = reference_ember_vary_velocity * velo_intensity;
            ember.particlesPerSecond = particles_per_second * 1.1;

            if (mFireWood <= 0.f)
            {
                ember.particlesPerSecond = 0;
                fire.baseColor = tg::color3::black;
                fire.varyColor = tg::color3::white * 0.1f;
            }
            else
            {
                fire.baseColor = tg::color3(255, 108, 33) / 255;
                fire.varyColor = tg::color3(0.1, 0.05, 0.0);
            }
        }

        auto ple = mECS->TryGetComponent<PointLightEmitter>(mBonfire);
        if (ple)
        {
            ple->pl.intensity = light_intensity * reference_light_intensity;
            ple->pl.radius = 30;
        }
    }

    auto se = mECS->TryGetComponent<SoundEmitter>(mBonfire);
    if (se)
    {
        se->volume = tg::max(mFireWood / 80.f, 0);
    }
}

void gamedev::GamelogicSystem::ConsumeWood(float dt)
{
    // Consume every 4 seconds
    if (int(mTime - dt) < int(mTime))
    {
        mFireWood -= 0.1;
    }
}

int gamedev::GamelogicSystem::GetWoodPrice() { return 2.0; }

int gamedev::GamelogicSystem::GetWood() { return int(mFireWood); }

void gamedev::GamelogicSystem::AddWood()
{
    if (Pay(GetWoodPrice()))
    {
        mFireWood += 5.0;
        UpdateHomefire();

        Event e(EventType::MoreWood, mBonfire, mBonfire);
        mECS->SendFunctionalEvent(e);

        mAS->PlayGlobalSound("wood.wav", effect, 0.5);
    }
}

gamedev::PointLight gamedev::GamelogicSystem::GetHomePointlight()
{
    auto pl = mECS->TryGetComponent<PointLightEmitter>(mBonfire);
    return pl->pl;
}

float gamedev::GamelogicSystem::GetVignetteIntensity() { return mVignetteIntensity; }
float gamedev::GamelogicSystem::GetVignetteSoftness() { return mVignetteSoftness; }
float gamedev::GamelogicSystem::GetVignetteRadius() { return mVignetteRadius; }
tg::vec3 gamedev::GamelogicSystem::GetVignetteColor() { return mVignetteColor; }
float gamedev::GamelogicSystem::GetDesaturation() { return mDesaturationIntensity; }

void gamedev::GamelogicSystem::UpdateVignette()
{
    // Do sth
}
void gamedev::GamelogicSystem::UpdateDesaturation()
{
    // Do sth
}

void gamedev::GamelogicSystem::UpdateEffects(float dt)
{
    mVignetteIntensity += (mVignetteIntensityTarget - mVignetteIntensity) * 0.1 * dt;
    mVignetteRadius += (mVignetteRadiusTarget - mVignetteRadius) * 0.1 * dt;
    mVignetteSoftness += (mVignetteSoftnessTarget - mVignetteSoftness) * 0.1 * dt;
    mVignetteColor += (mVignetteColorTarget - mVignetteColor) * 0.1 * dt;

    mDesaturationIntensity += (mDesaturationIntensityTarget - mDesaturationIntensity) * 0.1 * dt;
}

void gamedev::GamelogicSystem::RepairFortifications()
{
    if (!Pay(GetRepairFortificationsPrice()))
    {
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        return;
    }

    for (const auto& w : mWalls)
    {
        if (mECS->TestSignature<Prototype>(w))
        {
            mECS->RemoveComponent<Prototype>(w);
            auto destructible = mECS->TryGetComponent<Destructible>(w);
            if (destructible)
            {
                destructible->health.current = destructible->health.max;
            }
        }
    }

    mDestroyedWalls = 0;
    mAS->PlayGlobalSound("construction1.wav", effect, 0.5f);
}

int gamedev::GamelogicSystem::GetRepairFortificationsPrice()
{
    return mDestroyedWalls * 1;
}

void gamedev::GamelogicSystem::CalculateFreeSlots()
{
    int rings = tg::max(mHome.radius / slotSize, 0);

    // Inner slots
    for (int r = 2; r < tg::max(rings - 1, 0); r++)
    {
        auto radius = r * int(mHome.radius / rings);
        int slots = 2.f * tg::pi_scalar<float> * radius / slotSize;

        for (int s = 0; s < slots; s++)
        {
            auto angle = 2.f * tg::asin(slotSize / (2 * radius));

            auto cos = tg::cos(s * angle);
            auto sin = tg::sin(s * angle);

            if (sin < -0.8)
                continue;

            auto x = mHome.center.x + radius * cos;
            auto z = mHome.center.z + radius * sin;
            mFreeInnerSlots.push_back(tg::pos3(x, 0.0f, z));
        }
    }

    // Outer slots
    auto radius = (rings - 1) * int(mHome.radius / rings);
    int slots = 2.f * tg::pi_scalar<float> * radius / slotSize;

    for (int s = 0; s < slots; s++)
    {
        auto angle = 2.f * tg::asin(slotSize / (2 * radius));

        auto cos = tg::cos(s * angle);
        auto sin = tg::sin(s * angle);

        if (sin < -0.8)
            continue;

        auto x = mHome.center.x + radius * cos;
        auto z = mHome.center.z + radius * sin;
        mFreeOuterSlots.push_back(tg::pos3(x, 0.0f, z));
    }



    int i = 0;
}

void gamedev::GamelogicSystem::TestFreeSlots()
{
    for (auto i = mFreeInnerSlots.size() - 1; i > 0; i--)
    {
        mSF->Create("tent0", mFreeInnerSlots[i]);
        mFreeInnerSlots.pop_back();
    }

    for (auto i = mFreeOuterSlots.size() - 1; i > 0; i--)
    {
        mSF->Create("tent0", mFreeOuterSlots[i]);
        mFreeOuterSlots.pop_back();
    }
}

int gamedev::GamelogicSystem::GetMaxPioneers() { return mHomes * 4; }
int gamedev::GamelogicSystem::GetHomePrice() { return 5; }
int gamedev::GamelogicSystem::GetProductionPrice() { return 10; }
void gamedev::GamelogicSystem::UpgradeProduction()
{
    if (!Pay(GetProductionPrice()) || mFreeOuterSlots.empty())
    {
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        return;
    }

    SpawnForge();
    
    mAS->PlayGlobalSound("construction1.wav", effect, 0.5f);
}
void gamedev::GamelogicSystem::ConstructHome()
{
    if (!Pay(GetHomePrice()) || mFreeInnerSlots.empty())
    {
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        return;
    }

    SpawnHouse();

    mAS->PlayGlobalSound("tent.wav", effect, 1.f);
}

const std::vector<gamedev::InstanceHandle>& gamedev::GamelogicSystem::GetOutlinedHandles() { return mOutlinedHandles; }
const std::vector<tg::vec4>& gamedev::GamelogicSystem::GetOutlinedColors() { return mOutlinedColors; }

void gamedev::GamelogicSystem::UpdateOutlines(float dt)
{
    for (int i = 0; i < mOutlinedColors.size(); i++)
    {
        auto& c = mOutlinedColors[i];
        
        // Don't update current selections
        if (c.w <= -1.f)
        {
            continue;
        }

        // Only update fading highlights
        c.w -= dt;

        if ((c.w <= 0.f) || !mECS->IsLiveHandle(mOutlinedHandles[i]))
        {
            std::swap(mOutlinedHandles[i], mOutlinedHandles.back());
            std::swap(mOutlinedColors[i], mOutlinedColors.back());
            mOutlinedHandles.pop_back();
            mOutlinedColors.pop_back();
        }
    }
}

// Monster Targets:
// 1. Get to fire
// 2. Destroy blocking structures
// 3. Kill any pioneer in reach
void gamedev::GamelogicSystem::UpdateMonster(const Instance& instance, Living* subject, float dt)
{
    if (subject->friendly)
    {
        return;
    }

    auto attacker = mECS->GetComponent<Attacker>(instance.mHandle);

    // Steal firewood if not attacking
    auto fireDist_sqr = tg::max(tg::distance_sqr(tg::pos3::zero + instance.xform.translation, mHome.center) - 0.5, 0.0);
    if ((fireDist_sqr < 2.f) && !attacker->attackOrder.is_valid())
    {
        subject->moveOrder = false;
        subject->forceMove = false;
        if (subject->focusHandle._value != mBonfire._value)
                SetFocus(subject, mBonfire);
        StealFirewood(instance, dt);
    }

    // Set Moveorder again if no move-order is in place & if not attacking
    if ((fireDist_sqr > 2.f) && !attacker->attackOrder.is_valid())
    {
        if (subject->moveOrder == false)
        {
            SetMoveOrder(subject, (mHome.center + tg::normalize(instance.xform.translation - tg::vec3(mHome.center))) - tg::pos3::zero);
        }
    }
}

void gamedev::GamelogicSystem::StealFirewood(const Instance& instance, float dt)
{
    mFireWood -= dt/2.f;
}

int gamedev::GamelogicSystem::GetSupplyLevel() { return mSupplyLevel; }
int gamedev::GamelogicSystem::GetSupplyPrice()
{
    switch (mSupplyLevel)
    {
    case 0:
        return 50;

    case 1:
        return 50;

    case 2:
        return mTowers.size() * GetSoldierPrice();
    }

    return 0;
}

void gamedev::GamelogicSystem::UpgradeSupply()
{
    if (!Pay(GetSupplyPrice()))
    {
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        return;
    }

    if (mSupplyLevel == 0)
    {
        auto fences = mSF->LoadObjectsMarked("supplyroute_fences");
        mSupplyLevel++;

        for (const auto& f : fences)
        {
            mOutlinedHandles.push_back(f);
            mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));
        }

        Event e(EventType::Supply1, {std::uint32_t(-1)}, {std::uint32_t(-1)});
        mECS->SendFunctionalEvent(e);

        mAS->PlayGlobalSound("supplyUpgrade.wav", effect, 1.f);
    }
    else if (mSupplyLevel == 1)
    {
        mTowers = mSF->LoadObjectsMarked("supplyroute_towers");
        mSupplyLevel++;

        for (const auto& t : mTowers)
        {
            mOutlinedHandles.push_back(t);
            mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));
        }

        Event e(EventType::Supply2, {std::uint32_t(-1)}, {std::uint32_t(-1)});
        mECS->SendFunctionalEvent(e);

        mAS->PlayGlobalSound("supplyUpgrade.wav", effect, 1.f);
    }
    else
    {
        mSupplyLevel++;

        for (auto& h : mTowers)
        {
            const auto& xform = mECS->GetInstanceTransform(h);
            auto towerHandle = mSF->Create("awatchtower1", tg::pos3(xform.translation), xform.rotation, xform.scaling);
            mECS->GetInstanceTransform(towerHandle).translation = xform.translation;
            auto tower = mECS->GetComponent<Tower>(towerHandle);

            auto soldierHandle = mUF->Create("person_soldier");
            mECS->CreateComponent<InTower>(soldierHandle);
            mECS->GetInstanceTransform(soldierHandle).translation = tg::vec3(xform.translation + tg::vec3(tower->position) * xform.scaling);

            mOutlinedHandles.push_back(towerHandle);
            mOutlinedColors.push_back(tg::vec4(0.0, 153.0 / 255.0, 1.0, 1.0));

            mECS->MarkDestruction(h);
        }

        mAS->PlayGlobalSound("victory.wav", effect, 0.5f);
    }
}

void gamedev::GamelogicSystem::SetReinforcementWay()
{
    std::vector<tg::pos3> positions;

    // Blender values
    tg::pos3 initial(-22.4, 0.0, 37.0);
    positions.push_back(initial);
    positions.push_back(tg::pos3(-22.3, 0.0, 32.51));
    positions.push_back(tg::pos3(-20.0, 0.0, 27.3));
    positions.push_back(tg::pos3(-19.7, 0.0, 22.4));
    positions.push_back(tg::pos3(-19, 0.0, 17.7));
    positions.push_back(tg::pos3(-17.3, 0.0, 15.2));
    positions.push_back(tg::pos3(-15, 0.0, 13.4));
    positions.push_back(tg::pos3(-10.9, 0.0, 12));
    positions.push_back(tg::pos3(-8.4, 0.0, 9.7));
    positions.push_back(tg::pos3(-0.5, 0.0, 5.7));
    positions.push_back(tg::pos3(0.4, 0.0, -5.3));

    // internal values
    for (auto& p : positions)
    {
        p.z *= -1;
        p *= mGlobalScale;
    }

    mReinforcementWay = positions;
}

void gamedev::GamelogicSystem::TestReinforcementWay()
{
    for (const auto& p : mReinforcementWay)
    {
        mSF->Create("tent0", p);
    }
}

void gamedev::GamelogicSystem::UpdateTowerSoldier(const Instance& instance, Living* lifeform, Attacker* attacker)
{
    if (!mECS->TestSignature<InTower>(instance.mHandle))
    {
        return;
    }

    lifeform->moveOrder = false;
    lifeform->forceMove = false;
}

void gamedev::GamelogicSystem::UpdateForeignControl(const Instance& instance, Living* subject)
{
    auto fc = mECS->TryGetComponent<ForeignControl>(instance.mHandle);
    if (!fc)
    {
        return;
    }

    // Initial
    if (fc->currentTarget == tg::pos3::zero)
    {
        if (fc->waypoints.size() < 2)
        {
            mECS->RemoveComponent<ForeignControl>(instance.mHandle);
            return;
        }

        fc->currentIndex++;
        //fc->currentTarget = tg::pos3(instance.xform.translation) + (fc->waypoints[fc->currentIndex] - fc->waypoints[fc->currentIndex - 1]);
        fc->currentTarget = fc->waypoints[fc->currentIndex];
        fc->currentTarget.y = mTerrain->heightAt(fc->currentTarget);

        SetMoveOrder(subject, tg::vec3(fc->currentTarget));
        return;
    }

    // If near target
    if (tg::distance_sqr(instance.xform.translation, tg::vec3(fc->currentTarget)) < 0.5)
    {
        fc->currentIndex++;

        if (fc->currentIndex >= fc->waypoints.size())
        {
            mECS->RemoveComponent<ForeignControl>(instance.mHandle);
            return;
        }

        //fc->currentTarget = tg::pos3(instance.xform.translation) + (fc->waypoints[fc->currentIndex] - fc->waypoints[fc->currentIndex - 1]);
        fc->currentTarget = fc->waypoints[fc->currentIndex];
        fc->currentTarget.y = mTerrain->heightAt(fc->currentTarget);

        SetMoveOrder(subject, tg::vec3(fc->currentTarget));
    }
}

int gamedev::GamelogicSystem::GetHealPrice() { return 2.f; }

void gamedev::GamelogicSystem::Heal()
{
    if (!GetInjured())
    {
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        return;
    }

    if (!Pay(GetHealPrice() * GetInjured()))
    {
        mAS->PlayGlobalSound("interfaceNegative.wav", ui);
        return;
    }

    for (const auto& h : mInjuredPioneers)
    {
        if (!mECS->IsLiveHandle(h))
        {
            mInjuredPioneers.erase(h);
            continue;
        }

        auto mortal = mECS->TryGetComponent<Mortal>(h);
        mortal->health.current = mortal->health.max;
        mInjuredPioneers.erase(h);
    }

    mAS->PlayGlobalSound("heal.wav", ui);
}

int gamedev::GamelogicSystem::GetInjured()
{
    return mInjuredPioneers.size();
}