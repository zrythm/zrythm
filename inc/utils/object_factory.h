// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_OBJECT_FACTORY_H__
#define __UTILS_OBJECT_FACTORY_H__

#include <memory>
#include <optional>

/// @brief A factory class for creating objects that require initialization.
///
/// This class provides a static `create()` method that constructs an object of
/// type `Derived` and initializes it. If the initialization is successful, the
/// method returns the constructed object wrapped in a `std::optional`. If the
/// initialization fails, the method returns `std::nullopt`.
///
/// The `Derived` class must implement the pure virtual `initialize()` method,
/// which performs the necessary initialization logic.
template <typename Derived> class InitializableObjectFactory
{
public:
  /// @brief Constructs and initializes a shared pointer to an object of type
  /// `Derived`.
  ///
  /// @tparam Args The types of the arguments to be forwarded to the constructor
  /// of `Derived`.
  /// @param args The arguments to be forwarded to the constructor of `Derived`.
  /// @return A `std::optional` containing a shared pointer to the constructed
  /// and initialized object,
  ///         or `std::nullopt` if the initialization failed.
  template <typename... Args>
  static std::optional<std::shared_ptr<Derived>> create_shared (Args &&... args)
  {
    static_assert (
      !std::is_base_of_v<std::enable_shared_from_this<Derived>, Derived>,
      "InitializableObjectFactory is not compatible with enable_shared_from_this");

    auto obj =
      std::shared_ptr<Derived> (new Derived (std::forward<Args> (args)...));
    if (obj->initialize ())
      {
        return obj;
      }
    return std::nullopt;
  }

  /// @brief Constructs and initializes a unique pointer to an object of type
  /// `Derived`.
  ///
  /// @tparam Args The types of the arguments to be forwarded to the constructor
  /// of `Derived`.
  /// @param args The arguments to be forwarded to the constructor of `Derived`.
  /// @return A `std::optional` containing a unique pointer to the constructed
  /// and initialized object,
  ///         or `std::nullopt` if the initialization failed.
  template <typename... Args>
  static std::optional<std::unique_ptr<Derived>> create_unique (Args &&... args)
  {
    auto obj =
      std::unique_ptr<Derived> (new Derived (std::forward<Args> (args)...));
    if (obj->initialize ())
      {
        return obj;
      }
    return std::nullopt;
  }

protected:
  /// @brief Default constructor.
  InitializableObjectFactory () = default;

  /// @brief Default destructor.
  virtual ~InitializableObjectFactory () = default;

private:
  /// @brief Performs the initialization logic for the derived class.
  ///
  /// This method must be implemented by the derived class to perform the
  /// necessary initialization logic. If the initialization is successful, the
  /// method should return `true`, otherwise it should return `false`.
  virtual bool initialize () = 0;
};

template <typename T>
concept FactoryCreatable = std::derived_from<T, InitializableObjectFactory<T>>;

#endif // __UTILS_OBJECT_FACTORY_H__