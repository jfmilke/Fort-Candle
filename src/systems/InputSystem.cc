#include "systems/InputSystem.hh"
#include "advanced/transform.hh"

void gamedev::InputSystem::AddEntity(InstanceHandle& handle, Signature entitySignature) {
    mEntities.insert(handle);
}

void gamedev::InputSystem::RemoveEntity(InstanceHandle& handle, Signature entitySignature) {
    mEntities.erase(handle);
}

void gamedev::InputSystem::RemoveEntity(InstanceHandle& handle) {
    mEntities.erase(handle);
}

void gamedev::InputSystem::RemoveAllEntities() { mEntities.clear(); }

void gamedev::InputSystem::Init(std::shared_ptr<EngineECS>& ecs) {
    mECS = ecs;
}

int gamedev::InputSystem::Update(float dt)
{
    auto t0 = std::chrono::steady_clock::now();

    for (const auto& handle : mEntities)
    {
        auto instance = mECS->GetInstance(handle);

        auto input = mECS->GetComponent<gamedev::Input>(handle);
        auto physics = mECS->TryGetComponent<gamedev::Physics>(handle);

        auto move = input->targetMove - tg::pos3(instance.xform.translation);
        
        // Update focus
        if (input->focusInstance.is_valid())
        {
            InstructFocus(handle, input->focusInstance, input->focusMove, input->focusLook);
        }

        if (physics && (tg::length_sqr(move) < 0.1))
        {
            
        }

        // Update velocity if angle between move-vector and current velocity-vector is above some epsilon
        if (physics && (tg::length_sqr(tg::cross(physics->velocity, move)) > 0.001))
        {
            physics->velocity = tg::normalize_safe(move) * input->runningSpeed;
        }
    }

    auto tn = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(tn - t0).count();
}

void gamedev::InputSystem::InstructMove(InstanceHandle handle, tg::pos3 targetPosition)
{
    mECS->GetComponent<gamedev::Input>(handle)->targetMove = targetPosition;
}

void gamedev::InputSystem::InstructLook(InstanceHandle handle, tg::vec3 targetDirection)
{
    mECS->GetComponent<gamedev::Input>(handle)->targetLook = targetDirection;
}

void gamedev::InputSystem::InstructFocus(InstanceHandle active_handle, InstanceHandle passive_handle, bool focusMove, bool focusLook)
{
    if (!focusMove && !focusLook)
    {
        return InstructStopFocus(active_handle);
    }

    auto input = mECS->GetComponent<gamedev::Input>(active_handle);
    auto activePosition = tg::pos3(mECS->GetInstance(active_handle).xform.translation);
    auto passivePosition = tg::pos3(mECS->GetInstance(passive_handle).xform.translation);
    auto lookDir = passivePosition - activePosition;

    input->focusMove = focusMove;
    input->focusLook = focusLook;
    
    if (focusMove && input->targetMove != passivePosition)
    {
        input->targetMove = passivePosition;
        input->refresh = true;
    }

    if (focusLook && lookDir != input->targetLook)
    {
        input->targetLook = lookDir;
        input->refresh = true;
    }
}

void gamedev::InputSystem::InstructStopInput(InstanceHandle handle)
{
    auto input = mECS->GetComponent<gamedev::Input>(handle);
    auto instance = mECS->GetInstance(handle);

    input->targetMove = tg::pos3(instance.xform.translation);
}

void gamedev::InputSystem::InstructStopFocus(InstanceHandle handle)
{
    auto input = mECS->GetComponent<gamedev::Input>(handle);
    auto instance = mECS->GetInstance(handle);

    input->targetMove = tg::pos3(instance.xform.translation);
    input->focusMove = false;
    input->focusLook = false;
    input->focusInstance = {std::uint32_t(-1)};
}