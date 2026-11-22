#pragma once
#include <bitset>

/// Maximum number of distinct component types (bits in a `Signature`).
#define MAX_COMPONENTS 32U

/**
 * Maximum number of live instances: the capacity of every `ComponentArray`.
 *
 * The `World` pool gets the same size, but as a literal in `AdvancedFeatures::initialize` (keep both in sync).
 */
#define MAX_INSTANCES 2048

/// Bitset of component types: which components an entity has, or which a system requires (checked bitwise).
using Signature = std::bitset<MAX_COMPONENTS>;

/// ID of a registered component type, used as bit index into a `Signature`.
using ComponentType = std::uint8_t;
