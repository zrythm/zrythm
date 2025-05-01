// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef UTILS_INITIALIZABLE_OBJECT_H
#define UTILS_INITIALIZABLE_OBJECT_H

#include <memory>

#include "utils/traits.h"
#include "utils/types.h"

namespace zrythm::utils
{

namespace InitializableObjectDetail
{
/// @brief Concept to ensure a type has an initialize() method returning bool.
template <typename T>
concept Initializable = requires (T t) {
  { t.initialize () } -> std::convertible_to<bool>;
};
}

/// @brief A factory class for creating initializable objects using static
/// polymorphism.
template <typename Derived> class InitializableObject
{
protected:
  /// @brief Protected constructor to prevent instantiation.
  InitializableObject () = default;

public:
  /// @brief Creates a shared pointer to initialized object.
  template <typename... Args>
  static std::shared_ptr<Derived> create_shared (Args &&... args)
  {
    auto obj =
      std::shared_ptr<Derived> (new Derived (std::forward<Args> (args)...));
    return obj->initialize () ? obj : nullptr;
  }

  /// @brief Creates a unique pointer to initialized object.
  template <typename... Args>
  static std::unique_ptr<Derived> create_unique (Args &&... args)
  {
    auto obj =
      std::unique_ptr<Derived> (new Derived (std::forward<Args> (args)...));
    return obj->initialize () ? std::move (obj) : nullptr;
  }

  Z_DISABLE_COPY_MOVE (InitializableObject);
};

template <typename T>
concept Initializable = DerivedFromCRTPBase<T, InitializableObject>;

} // namespace zrythm::utils

#endif // UTILS_OBJECT_FACTORY_H
