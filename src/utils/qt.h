// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include "utils/types.h"

#include <QPointer>

namespace zrythm::utils
{

/**
 * @brief Helper that checks if 2 values are equal.
 *
 * This is intended used in QObject property setters.
 */
template <class T>
constexpr bool
values_equal_for_qproperty_type (const T &a, const T &b)
{
  if constexpr (std::is_floating_point_v<T>)
    return qFuzzyCompare (a, b);
  else
    return a == b;
}

template <typename T>
concept QObjectDerived = std::is_base_of_v<QObject, T>;

/**
 * @brief A unique pointer for QObject objects that also works with
 * QObject-based ownership.
 *
 * @tparam T
 */
template <QObjectDerived T> class QObjectUniquePtr
{
public:
  QObjectUniquePtr (T * ptr = nullptr) : ptr_ (ptr) { }

  ~QObjectUniquePtr () { reset (); }

  Z_DISABLE_COPY (QObjectUniquePtr)

  // Allow moving
  QObjectUniquePtr (QObjectUniquePtr &&other) noexcept : ptr_ (other.release ())
  {
  }
  QObjectUniquePtr &operator= (QObjectUniquePtr &&other) noexcept
  {
    if (this != &other)
      {
        reset (other.release ());
      }
    return *this;
  }

  void reset (T * ptr = nullptr)
  {
    if (ptr_ != ptr)
      {
        if (ptr_ != nullptr)
          {
            delete ptr_.get ();
          }
        ptr_ = ptr;
      }
  }

  T * release ()
  {
    auto ptr = ptr_;
    ptr_ = nullptr;
    return ptr.get ();
  }

  T *      get () const { return ptr_; }
  T *      operator->() const { return ptr_; }
  T       &operator* () const { return *ptr_; }
  explicit operator bool () const { return !ptr_.isNull (); }

  // Conversion to QPointer
  explicit operator QPointer<T> () const { return ptr_; }

private:
  QPointer<T> ptr_;
};

template <typename T, typename... Args>
QObjectUniquePtr<T>
make_qobject_unique (Args &&... args)
{
  return QObjectUniquePtr<T> (new T (std::forward<Args> (args)...));
}
} // namespace zrythm::utils
