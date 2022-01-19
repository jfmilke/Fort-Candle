#pragma once

#include <type_traits>
#include <utility> // declval

namespace cc
{
namespace detail
{
template <class A, class B>
[[maybe_unused]] static auto test_op_less(int) -> decltype(bool(std::declval<A>() < std::declval<B>()), std::true_type{});
template <class A, class B>
[[maybe_unused]] static auto test_op_less(char) -> std::false_type;

template <class A, class B>
[[maybe_unused]] static auto test_op_greater(int) -> decltype(bool(std::declval<A>() > std::declval<B>()), std::true_type{});
template <class A, class B>
[[maybe_unused]] static auto test_op_greater(char) -> std::false_type;

template <class A, class B>
[[maybe_unused]] static auto test_op_less_or_equal(int) -> decltype(bool(std::declval<A>() <= std::declval<B>()), std::true_type{});
template <class A, class B>
[[maybe_unused]] static auto test_op_less_or_equal(char) -> std::false_type;

template <class A, class B>
[[maybe_unused]] static auto test_op_greater_or_equal(int) -> decltype(bool(std::declval<A>() >= std::declval<B>()), std::true_type{});
template <class A, class B>
[[maybe_unused]] static auto test_op_greater_or_equal(char) -> std::false_type;

template <class A, class B>
[[maybe_unused]] static auto test_op_equal(int) -> decltype(bool(std::declval<A>() == std::declval<B>()), std::true_type{});
template <class A, class B>
[[maybe_unused]] static auto test_op_equal(char) -> std::false_type;

template <class A, class B>
[[maybe_unused]] static auto test_op_not_equal(int) -> decltype(bool(std::declval<A>() != std::declval<B>()), std::true_type{});
template <class A, class B>
[[maybe_unused]] static auto test_op_not_equal(char) -> std::false_type;

template <class A, class B>
[[maybe_unused]] static auto test_op_left_shift(int) -> decltype(std::declval<A>() << std::declval<B>(), std::true_type{});
template <class A, class B>
[[maybe_unused]] static auto test_op_left_shift(char) -> std::false_type;

template <class A, class B>
[[maybe_unused]] static auto test_op_subscript(int) -> decltype(std::declval<A>()[std::declval<B>()], std::true_type{});
template <class A, class B>
[[maybe_unused]] static auto test_op_subscript(char) -> std::false_type;
} // namespace detail

template <class A, class B = A>
constexpr bool has_operator_less = decltype(detail::test_op_less<A const&, B const&>(0))::value;
template <class A, class B = A>
constexpr bool has_operator_less_or_equal = decltype(detail::test_op_less_or_equal<A const&, B const&>(0))::value;
template <class A, class B = A>
constexpr bool has_operator_greater = decltype(detail::test_op_greater<A const&, B const&>(0))::value;
template <class A, class B = A>
constexpr bool has_operator_greater_or_equal = decltype(detail::test_op_greater_or_equal<A const&, B const&>(0))::value;
template <class A, class B = A>
constexpr bool has_operator_equal = decltype(detail::test_op_equal<A const&, B const&>(0))::value;
template <class A, class B = A>
constexpr bool has_operator_not_equal = decltype(detail::test_op_not_equal<A const&, B const&>(0))::value;

template <class A, class B>
constexpr bool has_operator_left_shift = decltype(detail::test_op_left_shift<A, B>(0))::value;

template <class A, class B>
constexpr bool has_operator_subscript = decltype(detail::test_op_subscript<A, B>(0))::value;
}
