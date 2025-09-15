// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <array>
#include <memory>
#include <optional>
#include <ranges>
#include <type_traits>
#include <variant>
#include <vector>

#include <type_safe/strong_typedef.hpp>

// Helper to detect if DerivedT inherits from any specialization of BaseTemplateT
template <template <typename...> class BaseTemplateT, typename DerivedT>
struct is_derived_from_template
{
private:
  template <typename... Args>
  static constexpr std::true_type test (const BaseTemplateT<Args...> *);

  static constexpr std::false_type test (...);

public:
  static constexpr bool value =
    decltype (test (std::declval<DerivedT *> ()))::value;
};

// Convenience variable template (C++17+)
template <template <typename...> class BaseTemplateT, typename DerivedT>
inline constexpr bool is_derived_from_template_v =
  is_derived_from_template<BaseTemplateT, DerivedT>::value;

namespace internal_test
{
template <typename T> class MyTemplateBase
{
};
template <typename T> class OtherTemplateBase
{
};
class OtherBase
{
};
class Derived : public MyTemplateBase<int>
{
};

static_assert (is_derived_from_template_v<MyTemplateBase, Derived>);
static_assert (!is_derived_from_template_v<MyTemplateBase, int>);
static_assert (!is_derived_from_template_v<MyTemplateBase, OtherBase>);
static_assert (!is_derived_from_template_v<OtherTemplateBase, Derived>);
}

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

template <typename T>
concept StdVariant = requires {
  requires requires (T v) {
    []<typename... Vs> (std::variant<Vs...> &) { }(v);
  };
};
static_assert (StdVariant<std::variant<float, int>>);
static_assert (StdVariant<std::variant<>>);
static_assert (!StdVariant<float>);

// Concept to check if a type is a variant of pointers (allows std::nullptr_t)
template <typename T>
concept VariantOfPointers = requires (T v) {
  requires StdVariant<T>;
  []<typename... Vs> (std::variant<Vs...> &)
    requires (std::is_pointer_v<Vs> && ...)
  { }(v);
};

static_assert (VariantOfPointers<std::variant<std::nullptr_t *, int *>>);
static_assert (!VariantOfPointers<std::variant<std::nullptr_t *, int>>);

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

/// @brief An improved version of std::decay_t that also removes raw and smart
/// pointers.
template <typename T>
using base_type = std::remove_cvref_t<
  remove_smart_pointer_t<std::remove_pointer_t<std::decay_t<T>>>>;
static_assert (std::is_same_v<base_type<int * const &>, int>);
static_assert (std::is_same_v<base_type<std::shared_ptr<int> const &>, int>);

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

template <typename Derived, typename Base>
concept DerivedButNotBase =
  std::derived_from<Derived, Base> && !std::same_as<Derived, Base>;

template <typename Derived, template <typename> class BaseTemplate>
concept DerivedFromCRTPBase = std::is_base_of_v<BaseTemplate<Derived>, Derived>;

namespace detail_test
{
template <typename T> class CRTPBase
{
  CRTPBase () = default;
  friend T;
};
class Derived : public CRTPBase<Derived>
{
};
class NonDerived
{
};
class OtherNonDerived : public CRTPBase<NonDerived>
{
};
static_assert (DerivedFromCRTPBase<Derived, CRTPBase>);
static_assert (!DerivedFromCRTPBase<NonDerived, CRTPBase>);
static_assert (!DerivedFromCRTPBase<OtherNonDerived, CRTPBase>);
}; // namespace detail_test

// Trick to print the tparam type during compilation
// i.e.: No type named 'something_made_up' in 'tparam'
#define DEBUG_TEMPLATE_PARAM(tparam) \
  [[maybe_unused]] typedef typename tparam::something_made_up X;

template <typename R, typename T>
concept RangeOf =
  std::ranges::range<R>
  && (std::derived_from<std::ranges::range_value_t<R>, T> ||
    // handle pointers too
  (IsRawPointer<std::ranges::range_value_t<R>> && IsRawPointer<T> && std::derived_from<std::remove_pointer_t<std::ranges::range_value_t<R>>, std::remove_pointer_t<T>>) );

namespace detail
{
struct build_test_type
{
  template <typename... Args>
  build_test_type (Args &&...) { } // Accept any constructor arguments
};
}
/**
 * @brief Concept that checks if a type is a builder for objects.
 */
template <typename T>
concept ObjectBuilder = requires (T t) {
  {
    t.template build<detail::build_test_type> ()
  } -> std::same_as<std::unique_ptr<detail::build_test_type>>;
};
namespace object_builder_test
{
struct ValidBuilder
{
  template <typename T> auto build () { return std::make_unique<T> (); }
};
struct InvalidBuilder
{
  template <typename T> void build () { }
};
static_assert (ObjectBuilder<ValidBuilder>);
static_assert (!ObjectBuilder<InvalidBuilder>);
};

template <typename T>
concept EnumType = std::is_enum_v<T>;
