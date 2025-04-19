// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_TRAITS_H__
#define __UTILS_TRAITS_H__

#include <array>
#include <memory>
#include <ranges>
#include <type_traits>
#include <variant>
#include <vector>

#include "type_safe/strong_typedef.hpp"

// Helper concept to check if a type is a container
template <typename T>
concept ContainerType = requires (T t) {
  typename T::value_type;
  typename T::iterator;
  typename T::const_iterator;
  { t.begin () } -> std::same_as<typename T::iterator>;
  { t.end () } -> std::same_as<typename T::iterator>;
  { t.size () } -> std::convertible_to<std::size_t>;
};

// Dependent false (for preventing static_assertions from being executed before
// template instantiations)
template <typename...> struct dependent_false : std::false_type
{
};

template <typename... Ts>
using dependent_false_t = typename dependent_false<Ts...>::type;

template <typename... Ts>
constexpr auto dependent_false_v = dependent_false<Ts...>::value;

// Concept that checks if a type is a final class
template <typename T>
concept FinalClass = std::is_final_v<T> && std::is_class_v<T>;

template <typename T>
concept CompleteType = requires { sizeof (T); };

// Concept to check if a type is a raw pointer
template <typename T>
concept IsRawPointer = std::is_pointer_v<T>;

// Concept to check if a type is a variant of pointers (allows std::nullptr_t)
template <typename T>
concept VariantOfPointers = requires {
  []<typename... Ts> (std::variant<Ts...> *) {
    static_assert (
      ((IsRawPointer<Ts> || std::is_null_pointer_v<Ts>) && ...),
      "All types in the variant must be pointers");
  }(static_cast<T *> (nullptr));
};

// Concept to check if a type is a vector of variants of pointers (allows
// std::nullptr_t)
template <typename T>
concept VectorOfVariantPointers = requires {
  typename T::value_type;
  requires VariantOfPointers<typename T::value_type>;
  requires ContainerType<T>;
};

template <typename T>
concept OptionalType = requires {
  typename T::value_type;
  requires std::same_as<T, std::optional<typename T::value_type>>;
};

template <typename T>
concept StrongTypedef = requires (T t) { typename T::strong_typedef; };

// Primary template
template <typename T> struct remove_smart_pointer
{
  using type = T;
};

// Specialization for std::unique_ptr
template <typename T, typename Deleter>
struct remove_smart_pointer<std::unique_ptr<T, Deleter>>
{
  using type = T;
};

// Specialization for std::shared_ptr
template <typename T> struct remove_smart_pointer<std::shared_ptr<T>>
{
  using type = T;
};

// Specialization for std::weak_ptr
template <typename T> struct remove_smart_pointer<std::weak_ptr<T>>
{
  using type = T;
};

// Helper alias template (C++11 and later)
template <typename T>
using remove_smart_pointer_t = typename remove_smart_pointer<T>::type;

template <typename T>
using base_type = std::remove_cvref_t<
  remove_smart_pointer_t<std::remove_pointer_t<std::decay_t<T>>>>;

static_assert (std::is_same_v<base_type<int * const &>, int>);

// Concept to check if a type is derived from the base type pointed to by
// another type
template <typename Derived, typename Base>
concept DerivedFromPointee =
  std::derived_from<base_type<Derived>, base_type<Base>>;

template <typename T>
concept StdArray = requires {
  typename std::array<typename T::value_type, std::tuple_size<T>::value>;
  requires std::same_as<
    T, std::array<typename T::value_type, std::tuple_size<T>::value>>;
};

static_assert (StdArray<std::array<unsigned char, 3>>);
static_assert (!StdArray<std::vector<float>>);

template <typename T> struct is_unique_ptr : std::false_type
{
};
template <typename T> struct is_unique_ptr<std::unique_ptr<T>> : std::true_type
{
};
template <typename T> constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

static_assert (is_unique_ptr_v<std::unique_ptr<int>>);
static_assert (!is_unique_ptr_v<std::shared_ptr<int>>);

template <typename T> struct is_shared_ptr : std::false_type
{
};
template <typename T> struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
{
};
template <typename T> constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

template <typename T>
concept SmartPtr = is_unique_ptr_v<T> || is_shared_ptr_v<T>;

template <typename T>
concept SmartPtrToContainer =
  SmartPtr<T> && ContainerType<typename T::element_type>;

template <typename T>
concept FloatingPointContainer =
  ContainerType<T> && std::is_floating_point_v<typename T::value_type>;

template <typename Derived, typename Base>
concept DerivedButNotBase =
  std::derived_from<Derived, Base> && !std::same_as<Derived, Base>;

template <typename Derived, template <typename> class BaseTemplate>
concept DerivedFromTemplatedBase = requires {
  []<typename T> (const BaseTemplate<T> *) {
  }(static_cast<Derived *> (nullptr));
};

template <typename T>
concept IsVariant = requires {
  []<typename... Ts> (std::variant<Ts...> *) { }(static_cast<T *> (nullptr));
};

// Trick to print the tparam type during compilation
// i.e.: No type named 'something_made_up' in 'tparam'
#define DEBUG_TEMPLATE_PARAM(tparam) \
  [[maybe_unused]] typedef typename tparam::something_made_up X;

template <typename R, typename T>
concept RangeOf =
  std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, T>;

#endif // __UTILS_TRAITS_H__
