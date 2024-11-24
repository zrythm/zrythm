// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * ICloneable interface.
 */

#ifndef __UTILS_ICLONEABLE_H__
#define __UTILS_ICLONEABLE_H__

#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

#include "utils/object_factory.h"
#include "utils/traits.h"
#include "utils/types.h"

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * @brief Enum to specify cloning type.
 */
enum class CloneType
{
  Unique,
  Shared,
  Both
};

#define DEFINE_ICLONEABLE_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* helpers */ \
  /* ================================================================ */ \
  Q_INVOKABLE ClassType * clone##ClassType () const \
  { \
    return clone_raw_ptr (); \
  }

/**
 * Interface for objects that can be cloned.
 *
 * Clones are mainly used for serialization, so inheriting classes are only
 * expected to copy serializable members.
 *
 * @note Throws ZrythmException if cloning fails.
 */
template <typename Derived, CloneType Type = CloneType::Both> class ICloneable
{
public:
  virtual ~ICloneable () = default;

  friend Derived;

  constexpr Derived * get_derived () { return static_cast<Derived *> (this); }
  constexpr const Derived * get_derived () const
  {
    return static_cast<const Derived *> (this);
  }

  std::unique_ptr<Derived> clone_unique () const
    requires (Type == CloneType::Unique || Type == CloneType::Both)
  {
    std::unique_ptr<Derived> cloned;
    if constexpr (FactoryCreatable<Derived>)
      {
        cloned = *cloned->create_unique ();
      }
    else
      {
        cloned = std::make_unique<Derived> ();
      }
    cloned->init_after_cloning (*get_derived ());
    return cloned;
  }

  std::shared_ptr<Derived> clone_shared () const
    requires (Type == CloneType::Shared || Type == CloneType::Both)
  {
    std::shared_ptr<Derived> cloned;
    if constexpr (FactoryCreatable<Derived>)
      {
        cloned = *cloned->create_shared ();
      }
    else
      {
        cloned = std::make_shared<Derived> ();
      }
    cloned->init_after_cloning (*get_derived ());
    return cloned;
  }

  Derived * clone_raw_ptr () const
    requires (Type == CloneType::Unique || Type == CloneType::Both)
  {
    Derived * cloned = nullptr;
    if constexpr (FactoryCreatable<Derived>)
      {
        cloned = (*cloned->create_unique ()).release ();
      }
    else
      {
        cloned = new Derived ();
      }
    cloned->init_after_cloning (*get_derived ());
    return cloned;
  }

  Derived * clone_qobject (QObject * parent) const
    requires (Type == CloneType::Unique || Type == CloneType::Both)
  {
    Derived * cloned = clone_raw_ptr ();
    cloned->setParent (parent);
    return cloned;
  }

#if 0
  template <typename T>
  std::unique_ptr<T> clone_unique_as () const
    requires (Type == CloneType::Unique || Type == CloneType::Both)
  {
    return std::unique_ptr<T> (dynamic_cast<T *> (clone_unique ().release ()));
  }

  template <typename T>
  std::shared_ptr<T> clone_shared_as () const
    requires (Type == CloneType::Shared || Type == CloneType::Both)
  {
    return std::dynamic_pointer_cast<T> (clone_shared ());
  }
#endif

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
  virtual void init_after_cloning (const Derived &other) = 0;
};

template <typename T>
concept CloneableUnique =
  std::is_base_of_v<ICloneable<T, CloneType::Unique>, T>
  || std::is_base_of_v<ICloneable<T, CloneType::Both>, T>;

template <typename T>
concept CloneableShared =
  std::is_base_of_v<ICloneable<T, CloneType::Shared>, T>
  || std::is_base_of_v<ICloneable<T, CloneType::Both>, T>;

/**
 * @brief Concept that checks if a type is cloneable (i.e., inherits from
 * ICloneable).
 * @tparam T The type to check.
 */
template <typename T>
concept Cloneable =
  (CloneableUnique<T> || CloneableShared<T>)
  && std::is_final_v<T> && std::default_initializable<T>;

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
clone_unique_with_variant (const Base * base_ptr) -> std::unique_ptr<Base>
{
  using VariantPtr = to_pointer_variant<Variant>;
  auto variant_ptr = convert_to_variant<VariantPtr> (base_ptr);

  return std::visit (
    [] (auto * ptr) -> std::unique_ptr<Base> {
      using T = base_type<decltype (ptr)>;
      if constexpr (!std::is_same_v<T, std::nullptr_t>)
        {
          if (ptr)
            {
              if constexpr (std::is_base_of_v<Base, T> && CloneableUnique<T>)
                {
                  return std::unique_ptr<Base> (ptr->clone_unique ());
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
clone_shared_with_variant (const Base * base_ptr) -> std::shared_ptr<Base>
{
  using VariantPtr = to_pointer_variant<Variant>;
  auto variant_ptr = convert_to_variant<VariantPtr> (base_ptr);

  return std::visit (
    [] (auto * ptr) -> std::shared_ptr<Base> {
      using T = base_type<decltype (ptr)>;
      if constexpr (!std::is_same_v<T, std::nullptr_t>)
        {
          if (ptr)
            {
              if constexpr (std::is_base_of_v<Base, T> && CloneableShared<T>)
                {
                  return std::shared_ptr<Base> (ptr->clone_shared ());
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
  const std::array<std::unique_ptr<T>, N> &src)
{
  for (size_t i = 0; i < N; ++i)
    {
      if (src[i])
        {
          if constexpr (Cloneable<T>)
            {
              dest[i] = src[i]->clone_unique ();
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
clone_ptr_vector (std::vector<Ptr<T>> &dest, const std::vector<Ptr<T>> &src)
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
                  dest.push_back (ptr->clone_unique ());
                }
              else if constexpr (std::is_same_v<Ptr<T>, std::shared_ptr<T>>)
                {
                  dest.push_back (ptr->clone_shared ());
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
clone_unique_ptr_container (Container &dest, const Container &src)
{
  if constexpr (StdArray<Container>)
    {
      clone_unique_ptr_array (dest, src);
    }
  else
    {
      clone_ptr_vector (dest, src);
    }
}

template <typename Container, typename Variant, typename Base>
  requires AllInheritFromBase<Variant, Base>
void
clone_variant_container (Container &dest, const Container &src)
{
  if constexpr (StdArray<Container>)
    {
      for (size_t i = 0; i < src.size (); ++i)
        {
          if (src[i])
            {
              dest[i] = clone_unique_with_variant<Variant, Base> (src[i].get ());
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
              dest.push_back (
                clone_unique_with_variant<Variant, Base> (ptr.get ()));
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
clone_variant_container (Container &dest, const Container &src)
{
  using Base = typename Container::value_type::element_type;

  if constexpr (StdArray<Container>)
    {
      // Handle std::array
      for (size_t i = 0; i < src.size (); ++i)
        {
          if (src[i])
            {
              dest[i] = clone_unique_with_variant<Variant, Base> (src[i].get ());
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
              dest.push_back (
                clone_unique_with_variant<Variant, Base> (ptr.get ()));
            }
          else
            {
              dest.push_back (nullptr);
            }
        }
    }
}

/**
 * @}
 */

#endif
