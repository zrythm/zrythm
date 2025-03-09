// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef UTILS_OBJECT_FACTORY_H
#define UTILS_OBJECT_FACTORY_H

#include <memory>
#include <optional>

namespace zrythm::utils {

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
class InitializableObject
{
public:
  /// @brief Creates a shared pointer to initialized object.
  template <typename Derived, typename... Args>
    requires InitializableObjectDetail::Initializable<Derived>
  static std::shared_ptr<Derived> create_shared (Args &&... args)
  {
    auto obj =
      std::shared_ptr<Derived> (new Derived (std::forward<Args> (args)...));
    return obj->initialize () ? obj  : nullptr;
  }

  /// @brief Creates a unique pointer to initialized object.
  template <typename Derived, typename... Args>
    requires InitializableObjectDetail::Initializable<Derived>
  static std::unique_ptr<Derived> create_unique (Args &&... args)
  {
    auto obj = std::unique_ptr<Derived> (new Derived (std::forward<Args> (args)...));
    return obj->initialize () ? std::move (obj)  : nullptr;
  }
};

template <typename T>
concept Initializable = std::derived_from<T, InitializableObject>;

} // namespace zrythm::utils

#endif // UTILS_OBJECT_FACTORY_H
