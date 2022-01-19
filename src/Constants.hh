#pragma once
#include <bitset>

// Maximum amount of different Component Types
#define MAX_COMPONENTS 32U

// Maximum amount of living entities
#define MAX_INSTANCES 2048

// Indicator which components an entity has or which a system needs (simple bitwise comparison)
using Signature = std::bitset<MAX_COMPONENTS>;

// ID of a certain Component Type
using ComponentType = std::uint8_t;

