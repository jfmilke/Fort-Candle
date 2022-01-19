#pragma once
#include "components/Component.hh"

namespace gamedev
{
// Marks object to be rendered by the Rendering System
struct Render : Component
{
    using Component::Component;
    
    transform* inheritTransform = nullptr;
    bool selectable = false;
    bool targetable = false;
};
}