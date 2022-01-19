#pragma once

#include <cstring>
#include <type_traits>

#include <clean-core/macros.hh>

namespace cc
{
template <class To, class From>
[[nodiscard]] CC_FORCE_INLINE To bit_cast(From const& src)
{
    static_assert(std::is_trivially_copyable_v<To>, "only supported for trivially copyable types");
    static_assert(std::is_trivially_copyable_v<From>, "only supported for trivially copyable types");
    static_assert(sizeof(To) == sizeof(From), "only supported if types have the same size");
    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}
}
