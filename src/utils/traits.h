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
concept IsPointer = std::is_pointer_v<T>;

// Concept to check if a type is a variant of pointers (allows std::nullptr_t)
template <typename T>
concept VariantOfPointers = requires {
  []<typename... Ts> (std::variant<Ts...> *) {
    static_assert (
      ((IsPointer<Ts> || std::is_null_pointer_v<Ts>) && ...),
      "All types in the variant must be pointers");
  }(static_cast<T *> (nullptr));
};

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
using base_type = std::remove_reference_t<
  remove_smart_pointer_t<std::remove_pointer_t<std::decay_t<T>>>>;

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

/**
 * @brief A type trait that checks if a type is the same as or a pointer to a
 * specified type.
 *
 * @tparam T The type to compare against.
 */
template <typename T> struct type_is
{
  /**
   * @brief Function call operator that checks if the given type is the same as
   * or a pointer to T.
   *
   * @tparam U The type to check.
   * @param u An instance of the type to check (unused).
   * @return true if U is the same as T, std::unique_ptr<T>, or T*; false
   * otherwise.
   */
  template <typename U> bool operator() (const U &u) const;
};

/**
 * @brief Overload of the pipe operator (|) to filter elements of a specific
 * type from a range.
 *
 * This overload works with ranges of std::unique_ptr and raw pointers. It
 * transforms the range elements into pointers of type T* using dynamic_cast for
 * std::unique_ptr and raw pointers. Elements that cannot be converted to T* are
 * filtered out.
 *
 * @tparam T The type to filter for.
 * @tparam R The type of the range.
 * @param r The range to filter.
 * @param _ An instance of type_is<T> (unused).
 * @return A new range containing only elements of std::unique_ptr<T> or T*,
 * converted to T*.
 *
 * @note This overload requires C++20 and the <ranges> library.
 *
 * Example usage:
 * @code
 * std::vector<std::unique_ptr<Track>> tracks1;
 * for (auto track : tracks1 | type_is<AudioTrack>{})
 * {
 *   // `track` is of type `AudioTrack*`
 *   // ...
 * }
 *
 * std::vector<Track*> tracks2;
 * for (auto track : tracks2 | type_is<AudioTrack>{})
 * {
 *   // `track` is of type `AudioTrack*`
 *   // ...
 * }
 * @endcode
 */
template <typename T, typename R>
auto
operator| (R &&r, type_is<T>)
{
  return std::forward<R> (r) | std::views::transform ([] (const auto &u) {
           using U = std::decay_t<decltype (u)>;
           if constexpr (is_unique_ptr_v<U> || is_shared_ptr_v<U>)
             {
               using PtrType = typename std::pointer_traits<U>::element_type;
               return dynamic_cast<T *> (static_cast<PtrType *> (u.get ()));
             }
           else if constexpr (std::is_pointer_v<U>)
             {
               return dynamic_cast<T *> (u);
             }
           else
             {
               return static_cast<T *> (nullptr);
             }
         })
         | std::views::filter ([] (const auto &ptr) { return ptr != nullptr; });
}

template <typename Derived, typename Base>
concept DerivedButNotBase =
  std::derived_from<Derived, Base> && !std::same_as<Derived, Base>;

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
  requires (!IsPointer<BaseT>)
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

#endif // __UTILS_TRAITS_H__