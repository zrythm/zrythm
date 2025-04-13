// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/traits.h"

#include <QVariant>

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

// Helper to check if QVariant can convert to type T
template <typename T>
concept QVariantConvertible = requires (const QVariant &qvar) {
  { qvar.template value<T> () } -> std::same_as<T>;
};

// Primary template (undefined)
template <typename Variant> struct QVariantToStdVariant;

// Specialization for std::variant
template <typename... Ts> struct QVariantToStdVariant<std::variant<Ts...>>
{
  static std::variant<Ts...> convert (const QVariant &qvar)
  {
    std::variant<Ts...> result;
    bool                success = (tryConvert<Ts> (qvar, result) || ...);
    if (!success)
      {
        throw std::runtime_error ("QVariant type not supported in std::variant");
      }
    return result;
  }

private:
  // Non-pointer specialization
  template <typename T>
  static bool tryConvert (const QVariant &qvar, std::variant<Ts...> &out)
  {
    if constexpr (QVariantConvertible<T>)
      {
        if (qvar.userType () == QMetaType::fromType<T> ().id ())
          {
            out = qvar.template value<T> ();
            return true;
          }
      }
    return false;
  }

  // Pointer specialization
  template <typename T>
  static bool tryConvert (const QVariant &qvar, std::variant<Ts...> &out)
    requires (std::is_pointer_v<T>)
  {
    if constexpr (QVariantConvertible<T>)
      {
        if (qvar.template canConvert<T> ())
          {
            auto ptr = qvar.value<T> ();
            if (ptr != nullptr)
              {
                out = ptr;
                return true;
              }
          }
      }
    return false;
  }
};

// Helper function for direct usage
template <typename Variant>
auto
qvariantToStdVariant (const QVariant &qvar)
{
  return QVariantToStdVariant<Variant>::convert (qvar);
}