// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
#pragma once

#include <QObject>
#include <QUuid>

#include <boost/describe/class.hpp>
#include <nlohmann/json_fwd.hpp>

namespace zrythm::utils
{

/**
 * @brief QObject-based base for all UUID-identifiable objects.
 *
 * Inherits QObject so that every registry object is a QObject automatically.
 * This enables qobject_cast from the base pointer, QObject parent-child
 * ownership in the registry, and eliminates the need for derived types to
 * inherit QObject separately.
 *
 * Every object in the project that needs to be referenced by UUID inherits
 * from this, either directly (via UuidIdentifiableObject<T>) or indirectly
 * (through a typed base like Track).
 */
class UuidIdentifiableBase : public QObject
{
  Q_OBJECT

public:
  explicit UuidIdentifiableBase (QObject * parent = nullptr)
      : QObject (parent), uuid_ (QUuid::createUuid ())
  {
  }
  UuidIdentifiableBase (const QUuid &id, QObject * parent = nullptr)
      : QObject (parent), uuid_ (id)
  {
  }

  Q_DISABLE_COPY_MOVE (UuidIdentifiableBase)
  ~UuidIdentifiableBase () override = default;

  QUuid raw_uuid () const { return uuid_; }

  friend bool
  operator== (const UuidIdentifiableBase &lhs, const UuidIdentifiableBase &rhs)
  {
    return lhs.uuid_ == rhs.uuid_;
  }

  friend void to_json (nlohmann::json &j, const UuidIdentifiableBase &obj);
  friend void from_json (const nlohmann::json &j, UuidIdentifiableBase &obj);

protected:
  void set_raw_uuid (const QUuid &id) { uuid_ = id; }

private:
  QUuid uuid_;

  BOOST_DESCRIBE_CLASS (UuidIdentifiableBase, (), (), (), (uuid_))
};

} // namespace zrythm::utils
