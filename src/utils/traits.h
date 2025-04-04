// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
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

// Helper struct to merge multiple std::variant types
template <typename... Variants> struct merge_variants;

// Type alias for easier use of the merge_variants struct
template <typename... Variants>
using merge_variants_t = typename merge_variants<Variants...>::type;

// Base case: single variant, no merging needed
template <typename Variant> struct merge_variants<Variant>
{
  using type = Variant;
};

// Recursive case: merge two variants and continue with the rest
template <typename... Types1, typename... Types2, typename... Rest>
struct merge_variants<std::variant<Types1...>, std::variant<Types2...>, Rest...>
{
  // Merge the first two variants and recursively merge with the rest
  using type = merge_variants_t<std::variant<Types1..., Types2...>, Rest...>;
};

// Usage example:
// using Variant1 = std::variant<int, float>;
// using Variant2 = std::variant<char, double>;
// using MergedVariant = merge_variants_t<Variant1, Variant2>;
// MergedVariant is now std::variant<int, float, char, double>

/** @brief Helper struct to convert a variant to a variant of references */
template <typename Variant> struct to_reference_variant_impl;

/** @brief Specialization for std::variant */
template <typename... Ts> struct to_reference_variant_impl<std::variant<Ts...>>
{
  /** @brief The resulting variant type with reference_wrappers */
  using type = std::variant<std::reference_wrapper<Ts>...>;
};

/**
 * @brief Converts a variant to a variant of reference_wrappers
 * @tparam Variant The original variant type
 */
template <typename Variant>
using to_reference_variant = typename to_reference_variant_impl<Variant>::type;

// same for const ref
template <typename Variant> struct to_const_reference_variant_impl;
template <typename... Ts>
struct to_const_reference_variant_impl<std::variant<Ts...>>
{
  using type = std::variant<std::reference_wrapper<const Ts>...>;
};
template <typename Variant>
using to_const_reference_variant =
  typename to_const_reference_variant_impl<Variant>::type;

/** @brief Helper struct to convert a variant to a variant of unique_ptr's */
template <typename Variant> struct to_unique_ptr_variant_impl;

/** @brief Specialization for std::variant */
template <typename... Ts> struct to_unique_ptr_variant_impl<std::variant<Ts...>>
{
  /** @brief The resulting variant type with reference_wrappers */
  using type = std::variant<std::unique_ptr<Ts>...>;
};

/**
 * @brief Converts a variant to a variant of reference_wrappers
 * @tparam Variant The original variant type
 */
template <typename Variant>
using to_unique_ptr_variant = typename to_unique_ptr_variant_impl<Variant>::type;

/** @brief Helper struct to convert a variant to a variant of pointers */
template <typename Variant> struct to_pointer_variant_impl;

/** @brief Specialization for std::variant */
template <typename... Ts> struct to_pointer_variant_impl<std::variant<Ts...>>
{
  /** @brief The resulting variant type with pointers */
  using type = std::variant<std::add_pointer_t<Ts>...>;
};

/**
 * @brief Converts a variant to a variant of pointers
 * @tparam Variant The original variant type
 */
template <typename Variant>
using to_pointer_variant = typename to_pointer_variant_impl<Variant>::type;

/**
 * @brief Converts a base pointer to a variant type.
 *
 * This function takes a base pointer and attempts to convert it to the
 * specified variant type. If the base pointer is null, a variant with a null
 * pointer is returned. Otherwise, the function uses dynamic_cast to attempt to
 * convert the base pointer to each type in the variant, and returns the first
 * successful conversion.
 *
 * @tparam Base The base type of the pointer.
 * @tparam Variant The variant type to convert to.
 * @param base_ptr The base pointer to convert.
 * @return The converted variant type.
 */
template <typename Variant, typename Base>
  requires std::is_pointer_v<std::variant_alternative_t<0, Variant>>
auto
convert_to_variant (const Base * base_ptr) -> Variant
{
  if (!base_ptr)
    {
      return Variant{};
    }

  Variant result{};

  auto trycast = [&]<typename T> () {
    if (auto derived = dynamic_cast<const T *> (base_ptr))
      {
        result = const_cast<T *> (derived);
        return true;
      }
    return false;
  };

  auto try_all_casts = [&]<typename... Ts> (std::variant<Ts...> *) {
    return (trycast.template operator()<std::remove_pointer_t<Ts>> () || ...);
  };

  try_all_casts (static_cast<Variant *> (nullptr));

  return result;
}

/**
 * @brief Converts a variant that includes derived pointers to a base pointer.
 *
 * This function takes a pointer variant and attempts to convert it to the
 * specified base pointer type. The function uses std::visit to iterate
 * through the variant types and return the first successful conversion.
 *
 * @tparam BaseT The base pointer type to convert to.
 * @tparam PtrVariantT The variant pointer type to convert from.
 * @param variant The variant pointer to convert.
 * @return The converted base pointer.
 * @throw std::runtime_error if no conversion is possible.
 */
template <typename BaseT, typename PtrVariantT>
static BaseT *
get_ptr_variant_as_base_ptr (PtrVariantT &&variant)
  requires (!IsRawPointer<BaseT>)
           && VariantOfPointers<std::remove_reference_t<PtrVariantT>>
{
  return std::visit (
    [] (auto &&obj) -> BaseT * {
      using CurT = base_type<decltype (obj)>;
      static_assert (CompleteType<CurT>);
      static_assert (CompleteType<BaseT>);
      if constexpr (std::derived_from<CurT, BaseT>)
        {
          return obj;
        }
      else
        {
          throw std::runtime_error ("Invalid variant type");
        }
    },
    std::forward<PtrVariantT> (variant));
}

/**
 * @brief Overload pattern.
 *
 * Usage: `auto result = std::visit (overload {
 *   [] (int x) { return x * 2; },
 *   [] (const std::string &str) { return str.size (); },
 *   [] (const std::vector<int> &vec) { return vec.size (); }
 * }, var);`
 */
template <class... Ts> struct overload : Ts...
{
  using Ts::operator()...;
};

template <typename T>
concept IsVariant = requires {
  []<typename... Ts> (std::variant<Ts...> *) {}(static_cast<T *> (nullptr));
};

// Trick to print the tparam type during compilation
// i.e.: No type named 'something_made_up' in 'tparam'
#define DEBUG_TEMPLATE_PARAM(tparam) \
  [[maybe_unused]] typedef typename tparam::something_made_up X;

#endif // __UTILS_TRAITS_H__
