// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/icloneable.h"
#include "utils/utf8_string.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include <boost/describe.hpp>

namespace zrythm::structure::arrangement
{

/**
 * @brief Name functionality for arranger objects.
 */
class ArrangerObjectName : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString name READ name WRITE setName NOTIFY nameChanged)
  QML_ELEMENT

public:
  ArrangerObjectName (QObject * parent = nullptr) : QObject (parent) { }
  Z_DISABLE_COPY_MOVE (ArrangerObjectName)
  ~ArrangerObjectName () override = default;

  // ========================================================================
  // QML Interface
  // ========================================================================

  QString name () const { return name_.to_qstring (); }
  void    setName (const QString &name)
  {
    const auto name_str = utils::Utf8String::from_qstring (name);
    if (name_ == name_str)
      return;
    name_ = name_str;
    gen_escaped_name ();
    Q_EMIT nameChanged (name_str.to_qstring ());
  }
  Q_SIGNAL void nameChanged (const QString &name);

  // ========================================================================

  /**
   * Returns the name of the object.
   */
  utils::Utf8String get_name () const { return name_; }

private:
  friend void init_from (
    ArrangerObjectName       &obj,
    const ArrangerObjectName &other,
    utils::ObjectCloneType    clone_type)
  {
    obj.name_ = other.name_;
    obj.escaped_name_ = other.escaped_name_;
  }

  static constexpr std::string_view kNameKey = "name";
  friend void
  to_json (nlohmann::json &j, const ArrangerObjectName &named_object)
  {
    j[kNameKey] = named_object.name_;
  }
  friend void
  from_json (const nlohmann::json &j, ArrangerObjectName &named_object)
  {
    j.at (kNameKey).get_to (named_object.name_);
    named_object.gen_escaped_name ();
  }

  /**
   * Generates the escaped name for the object.
   */
  void gen_escaped_name ();

private:
  /** Name to be shown on the widget. */
  utils::Utf8String name_;

  /** Escaped name for drawing. */
  utils::Utf8String escaped_name_;

  BOOST_DESCRIBE_CLASS (ArrangerObjectName, (), (), (), (name_))
};

} // namespace zrythm::structure::arrangement
