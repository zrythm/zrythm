// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

#include "utils/initializable_object.h"
#include "utils/qt.h"
#include "utils/traits.h"
#include "utils/variant_helpers.h"

namespace zrythm::utils
{

enum class ObjectCloneType
{
  /**
   * Creates a snapshot of the object with the same identity.
   */
  Snapshot,

  /**
   * Creates a separately identified object.
   *
   * To be used only when duplicating objects as part of a user action.
   */
  NewIdentity,
};

/**
 * @brief Concept that checks if a type is cloneable.
 * @tparam T The type to check.
 */
template <typename T>
concept CloneableObject =
  requires (T &obj, const T &other, ObjectCloneType clone_type) {
    { init_from (obj, other, clone_type) } -> std::same_as<void>;
  };

template <CloneableObject Derived, typename... Args>
std::unique_ptr<Derived>
clone_unique (
  const Derived  &obj,
  ObjectCloneType clone_type = ObjectCloneType::Snapshot,
  Args &&... args)
{
  std::unique_ptr<Derived> cloned;
  if constexpr (utils::Initializable<Derived>)
    {
      auto result = Derived::create_unique (std::forward<Args> (args)...);
      if (!result)
        return nullptr;
      cloned = std::move (result);
    }
  else
    {
      cloned = std::make_unique<Derived> (std::forward<Args> (args)...);
    }
  init_from (*cloned, obj, clone_type);
  return cloned;
}

template <CloneableObject Derived, typename... Args>
std::shared_ptr<Derived>
clone_shared (
  const Derived  &obj,
  ObjectCloneType clone_type = ObjectCloneType::Snapshot,
  Args &&... args)
{
  std::shared_ptr<Derived> cloned;
  if constexpr (utils::Initializable<Derived>)
    {
      auto result =
        Derived::template create_shared<Derived> (std::forward<Args> (args)...);
      if (!result)
        return nullptr;
      cloned = std::move (result);
    }
  else
    {
      cloned = std::make_shared<Derived> (std::forward<Args> (args)...);
    }
  init_from (*cloned, obj, clone_type);
  return cloned;
}

template <CloneableObject Derived, typename... Args>
Derived *
clone_raw_ptr (
  const Derived  &obj,
  ObjectCloneType clone_type = ObjectCloneType::Snapshot,
  Args &&... args)
{
  auto unique_ptr = clone_unique (obj, clone_type, std::forward<Args> (args)...);
  return unique_ptr.release ();
}

template <CloneableObject Derived, typename... Args>
Derived *
clone_qobject (
  const Derived  &obj,
  QObject *       parent,
  ObjectCloneType clone_type = ObjectCloneType::Snapshot,
  Args &&... args)
  requires utils::QObjectDerived<Derived>
{
  auto * cloned = clone_raw_ptr (obj, clone_type, std::forward<Args> (args)...);
  if (cloned)
    {
      cloned->setParent (parent);
    }
  return cloned;
}

template <CloneableObject Derived, typename... Args>
utils::QObjectUniquePtr<Derived>
clone_unique_qobject (
  const Derived  &obj,
  QObject *       parent,
  ObjectCloneType clone_type = ObjectCloneType::Snapshot,
  Args &&... args)
  requires utils::QObjectDerived<Derived>
{
  return utils::QObjectUniquePtr<Derived> (
    clone_qobject (obj, parent, clone_type, std::forward<Args> (args)...));
}

/**
 * @brief Concept that checks if a type is cloneable.
 * @tparam T The type to check.
 */
template <typename T>
concept CloneableDefaultInitializableObject =
  CloneableObject<T> && std::default_initializable<T>;

/** Concept to check if a type inherits from a base class */
template <typename T, typename Base>
concept InheritsFromBase = std::is_base_of_v<Base, std::remove_pointer_t<T>>;

/** Concept to ensure all types in a variant inherit from a base class */
template <typename Variant, typename Base>
concept AllInheritFromBase = requires {
  []<typename... Ts> (std::variant<Ts...> *) {
    static_assert (
      (InheritsFromBase<Ts, Base> && ...),
      "All types in Variant must inherit from Base");
  }(static_cast<Variant *> (nullptr));
};

/**
 * @brief Clones the elements of a std::array of std::unique_ptr into the
 * destination array.
 * @tparam T The type of the elements in the array.
 * @tparam N The size of the array.
 * @param dest The destination array to clone into.
 * @param src The source array to clone from.
 */
template <typename T, std::size_t N>
void
clone_unique_ptr_array (
  std::array<std::unique_ptr<T>, N>       &dest,
  const std::array<std::unique_ptr<T>, N> &src,
  ObjectCloneType clone_type = ObjectCloneType::Snapshot)
{
  for (size_t i = 0; i < N; ++i)
    {
      if (src[i])
        {
          if constexpr (CloneableDefaultInitializableObject<T>)
            {
              dest[i] =
                clone_unique (*src[i], clone_type = ObjectCloneType::Snapshot);
            }
          else
            {
              dest[i] = std::make_unique<T> (*src[i]);
            }
        }
      else
        {
          dest[i] = nullptr;
        }
    }
}

/**
 * @brief Clones the elements of a std::vector of std::unique_ptr or
 * std::shared_ptr into the destination vector.
 * @tparam T The type of the elements in the vector.
 * @tparam Ptr The pointer type (std::unique_ptr or std::shared_ptr).
 * @param dest The destination vector to clone into.
 * @param src The source vector to clone from.
 */
template <typename T, template <typename...> class Ptr>
void
clone_ptr_vector (
  std::vector<Ptr<T>>       &dest,
  const std::vector<Ptr<T>> &src,
  ObjectCloneType            clone_type = ObjectCloneType::Snapshot)
{
  dest.clear ();
  dest.reserve (src.size ());

  for (const auto &ptr : src)
    {
      if (ptr)
        {
          if constexpr (CloneableDefaultInitializableObject<T>)
            {
              if constexpr (std::is_same_v<Ptr<T>, std::unique_ptr<T>>)
                {
                  dest.push_back (clone_unique (
                    *ptr, clone_type = ObjectCloneType::Snapshot));
                }
              else if constexpr (std::is_same_v<Ptr<T>, std::shared_ptr<T>>)
                {
                  dest.push_back (clone_shared (
                    *ptr, clone_type = ObjectCloneType::Snapshot));
                }
            }
          else
            {
              if constexpr (std::is_same_v<Ptr<T>, std::unique_ptr<T>>)
                {
                  dest.push_back (std::make_unique<T> (*ptr));
                }
              else if constexpr (std::is_same_v<Ptr<T>, std::shared_ptr<T>>)
                {
                  dest.push_back (std::make_shared<T> (*ptr));
                }
            }
        }
      else
        {
          dest.push_back (nullptr);
        }
    }
}

/**
 * @brief Clones the elements of a container of std::unique_ptr into the
 * destination container.
 * @tparam Container The type of the container.
 * @param dest The destination container to clone into.
 * @param src The source container to clone from.
 */
template <typename Container>
void
clone_unique_ptr_container (
  Container       &dest,
  const Container &src,
  ObjectCloneType  clone_type = ObjectCloneType::Snapshot)
{
  if constexpr (StdArray<Container>)
    {
      clone_unique_ptr_array (dest, src, clone_type = ObjectCloneType::Snapshot);
    }
  else
    {
      clone_ptr_vector (dest, src, clone_type = ObjectCloneType::Snapshot);
    }
}

template <typename Container, typename Variant, typename Base>
  requires AllInheritFromBase<Variant, Base>
void
clone_variant_container (
  Container       &dest,
  const Container &src,
  ObjectCloneType  clone_type = ObjectCloneType::Snapshot)
{
  if constexpr (StdArray<Container>)
    {
      for (size_t i = 0; i < src.size (); ++i)
        {
          if (src[i])
            {
              dest[i] = clone_unique_with_variant<Variant, Base> (
                src[i].get (), clone_type = ObjectCloneType::Snapshot);
            }
          else
            {
              dest[i] = nullptr;
            }
        }
    }
  else
    {
      dest.clear ();
      dest.reserve (src.size ());

      for (const auto &ptr : src)
        {
          if (ptr)
            {
              dest.push_back (clone_unique_with_variant<Variant, Base> (
                ptr.get (), clone_type = ObjectCloneType::Snapshot));
            }
          else
            {
              dest.push_back (nullptr);
            }
        }
    }
}

/**
 * Clones a container of variants.
 * @tparam Variant The variant type containing possible derived classes.
 * @tparam Container The container type (deduced automatically).
 */
template <typename Variant, typename Container>
  requires AllInheritFromBase<Variant, typename Container::value_type::element_type>
void
clone_variant_container (
  Container       &dest,
  const Container &src,
  ObjectCloneType  clone_type = ObjectCloneType::Snapshot)
{
  using Base = typename Container::value_type::element_type;

  if constexpr (StdArray<Container>)
    {
      // Handle std::array
      for (size_t i = 0; i < src.size (); ++i)
        {
          if (src[i])
            {
              dest[i] = clone_unique_with_variant<Variant, Base> (
                src[i].get (), clone_type = ObjectCloneType::Snapshot);
            }
          else
            {
              dest[i] = nullptr;
            }
        }
    }
  else
    {
      // Handle std::vector or similar containers
      dest.clear ();
      dest.reserve (src.size ());

      for (const auto &ptr : src)
        {
          if (ptr)
            {
              dest.push_back (clone_unique_with_variant<Variant, Base> (
                ptr.get (), clone_type = ObjectCloneType::Snapshot));
            }
          else
            {
              dest.push_back (nullptr);
            }
        }
    }
}

} // namespace zrythm::utils
