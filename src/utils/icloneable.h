// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_ICLONEABLE_H__
#define __UTILS_ICLONEABLE_H__

#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

#include "utils/initializable_object.h"
#include "utils/traits.h"
#include "utils/variant_helpers.h"

/**
 * @addtogroup utils
 *
 * @{
 */

using namespace zrythm;

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

#define DEFINE_ICLONEABLE_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* helpers */ \
  /* ================================================================ */ \
  Q_INVOKABLE ClassType * clone##ClassType () const \
  { \
    return clone_raw_ptr (ObjectCloneType::NewIdentity); \
  }

/**
 * Interface for objects that can be cloned.
 *
 * Clones are mainly used for serialization, so inheriting classes are only
 * expected to copy serializable members.
 *
 * @note Throws ZrythmException if cloning fails.
 */
template <typename Derived> class ICloneable
{
public:
  virtual ~ICloneable () = default;

  friend Derived;

private:
  constexpr Derived * get_derived () { return static_cast<Derived *> (this); }
  constexpr const Derived * get_derived () const
  {
    return static_cast<const Derived *> (this);
  }

public:
  template <typename... Args>
  std::unique_ptr<Derived> clone_unique (
    ObjectCloneType clone_type = ObjectCloneType::Snapshot,
    Args &&... args) const
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
    cloned->init_after_cloning (*get_derived (), clone_type);
    return cloned;
  }

  template <typename... Args>
  std::shared_ptr<Derived> clone_shared (
    ObjectCloneType clone_type = ObjectCloneType::Snapshot,
    Args &&... args) const
  {
    std::shared_ptr<Derived> cloned;
    if constexpr (utils::Initializable<Derived>)
      {
        auto result = Derived::template create_shared<Derived> (
          std::forward<Args> (args)...);
        if (!result)
          return nullptr;
        cloned = std::move (result);
      }
    else
      {
        cloned = std::make_shared<Derived> (std::forward<Args> (args)...);
      }
    cloned->init_after_cloning (*get_derived (), clone_type);
    return cloned;
  }

  template <typename... Args>
  Derived * clone_raw_ptr (
    ObjectCloneType clone_type = ObjectCloneType::Snapshot,
    Args &&... args) const
  {
    auto unique_ptr = clone_unique (clone_type, std::forward<Args> (args)...);
    return unique_ptr.release ();
  }

  template <typename... Args>
  Derived * clone_qobject (
    QObject *       parent,
    ObjectCloneType clone_type = ObjectCloneType::Snapshot,
    Args &&... args) const
  {
    auto * cloned = clone_raw_ptr (clone_type, std::forward<Args> (args)...);
    if (cloned)
      {
        cloned->setParent (parent);
      }
    return cloned;
  }

protected:
  ICloneable () = default;

private:
  /**
   * @brief Initializes the cloned object.
   *
   * @note Only final classes should implement this.
   *
   * @throw ZrythmException If the object could not be cloned.
   */
  virtual void init_after_cloning (
    const Derived  &other,
    ObjectCloneType clone_type = ObjectCloneType::Snapshot) = 0;
};

/**
 * @brief Concept that checks if a type is cloneable (i.e., inherits from
 * ICloneable).
 * @tparam T The type to check.
 */
template <typename T>
concept Cloneable =
  std::derived_from<T, ICloneable<T>> && std::is_final_v<T>
  && std::default_initializable<T>;

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
 * Clone a unique pointer using a variant
 * @tparam Variant The variant type containing possible derived classes
 * @tparam Base The base class type
 * @param base_ptr Pointer to the base class object
 * @return A unique pointer to the cloned object
 */
template <typename Variant, typename Base>
  requires AllInheritFromBase<Variant, Base>
auto
clone_unique_with_variant (
  const Base *    base_ptr,
  ObjectCloneType clone_type = ObjectCloneType::Snapshot)
  -> std::unique_ptr<Base>
{
  using VariantPtr = to_pointer_variant<Variant>;
  auto variant_ptr = convert_to_variant<VariantPtr> (base_ptr);

  return std::visit (
    [clone_type] (auto * ptr) -> std::unique_ptr<Base> {
      using T = base_type<decltype (ptr)>;
      if constexpr (!std::is_same_v<T, std::nullptr_t>)
        {
          if (ptr)
            {
              if constexpr (std::is_base_of_v<Base, T>)
                {
                  return std::unique_ptr<Base> (ptr->clone_unique (clone_type));
                }
            }
        }
      return nullptr;
    },
    variant_ptr);
}

template <typename Variant, typename Base>
  requires AllInheritFromBase<Variant, Base>
auto
clone_shared_with_variant (
  const Base *    base_ptr,
  ObjectCloneType clone_type = ObjectCloneType::Snapshot)
  -> std::shared_ptr<Base>
{
  using VariantPtr = to_pointer_variant<Variant>;
  auto variant_ptr = convert_to_variant<VariantPtr> (base_ptr);

  return std::visit (
    [clone_type] (auto * ptr) -> std::shared_ptr<Base> {
      using T = base_type<decltype (ptr)>;
      if constexpr (!std::is_same_v<T, std::nullptr_t>)
        {
          if (ptr)
            {
              if constexpr (std::is_base_of_v<Base, T>)
                {
                  return std::shared_ptr<Base> (ptr->clone_shared (clone_type));
                }
            }
        }
      return nullptr;
    },
    variant_ptr);
}

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
          if constexpr (Cloneable<T>)
            {
              dest[i] =
                src[i]->clone_unique (clone_type = ObjectCloneType::Snapshot);
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
          if constexpr (Cloneable<T>)
            {
              if constexpr (std::is_same_v<Ptr<T>, std::unique_ptr<T>>)
                {
                  dest.push_back (
                    ptr->clone_unique (clone_type = ObjectCloneType::Snapshot));
                }
              else if constexpr (std::is_same_v<Ptr<T>, std::shared_ptr<T>>)
                {
                  dest.push_back (
                    ptr->clone_shared (clone_type = ObjectCloneType::Snapshot));
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

#endif
