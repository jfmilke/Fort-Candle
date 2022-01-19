#pragma once
#include "components/Component.hh"

namespace gamedev
{
struct Input : public Component
{
    using Component::Component;

    tg::pos3 targetMove = tg::pos3(0.f, 0.f, 0.f);
    tg::vec3 targetLook = tg::vec3(1.f, 0.f, 0.f);
    tg::vec3 currentLook = targetLook;
    float rotationSpeed = 1.f;
    float runningSpeed = 10.f;
    bool refresh = false;

    InstanceHandle focusInstance;
    bool focusMove = false;
    bool focusLook = false;
};
}