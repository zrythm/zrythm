// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"

#define DEFINE_NAMEABLE_OBJECT_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* name */ \
  /* ================================================================ */ \
  Q_PROPERTY (QString name READ getName WRITE setName NOTIFY nameChanged) \
  QString getName () const \
  { \
    return name_.to_qstring (); \
  } \
  void setName (const QString &name) \
  { \
    const auto name_str = utils::Utf8String::from_qstring (name); \
    if (name_ == name_str) \
      return; \
    set_name_with_action (name_str); \
    Q_EMIT nameChanged (name); \
  } \
  Q_SIGNAL void nameChanged (const QString &name);

namespace zrythm::structure::arrangement
{

/**
 * @class NamedObject
 * @brief Base class for objects that have a name.
 *
 * This class provides a common interface for objects that have a name. It
 * includes methods for getting, setting, and validating the name, as well as
 * generating an escaped version of the name for drawing purposes.
 *
 * Derived classes that require name validation must pass a validator callable
 * to the constructor.
 */
class NamedObject : virtual public ArrangerObject
{
public:
  using NameValidator = std::function<bool (const utils::Utf8String &)>;

  NamedObject (
    NameValidator validator = [] (const utils::Utf8String &) { return true; })
      : name_validator_ (std::move (validator))
  {
  }

  Q_DISABLE_COPY_MOVE (NamedObject)
  ~NamedObject () override = default;

  /**
   * Returns the name of the object.
   */
  utils::Utf8String get_name () const { return name_; }

  /**
   * Generates the escaped name for the object.
   */
  void gen_escaped_name ();

  /**
   * @brief Sets the name of the object.
   *
   * @param name The new name for the object.
   * @param fire_events Whether to fire events when the name is changed.
   */
  void set_name (this auto &&self, const utils::Utf8String &name)
  {
    self.name_ = name;
    self.gen_escaped_name ();
    Q_EMIT self.nameChanged (name.to_qstring ());
  }

  void generate_name_from_automation_track (
    this auto  &self,
    const auto &track,
    const auto &at)
  {
    self.set_name (utils::Utf8String::from_utf8_encoded_string (
      fmt::format ("{} - {}", track.get_name (), at.getLabel ())));
  }
  void generate_name_from_track (this auto &self, const auto &track)
  {
    self.set_name (track.get_name ());
  }

  void generate_name (
    this auto                       &self,
    std::optional<utils::Utf8String> base_name,
    const auto *                     at,
    const auto *                     track)
  {
    if (base_name)
      {
        self.set_name (*base_name);
      }
    else if (at)
      {
        self.generate_name_from_automation_track (*track, *at);
        return;
      }
    else
      {
        self.generate_name_from_track (*track);
      }
  }

  /**
   * Changes the name and adds an action to the undo stack.
   *
   * Calls @ref set_name() internally.
   */
  void set_name_with_action (const utils::Utf8String &name);

  utils::Utf8String gen_human_friendly_name () const final { return name_; }

protected:
  void copy_members_from (const NamedObject &other, ObjectCloneType clone_type)
  {
    name_ = other.name_;
    escaped_name_ = other.escaped_name_;
  }

private:
  static constexpr std::string_view kNameKey = "name";
  friend void to_json (nlohmann::json &j, const NamedObject &named_object)
  {
    j[kNameKey] = named_object.name_;
  }
  friend void from_json (const nlohmann::json &j, NamedObject &named_object)
  {
    j.at (kNameKey).get_to (named_object.name_);
    named_object.gen_escaped_name ();
  }

protected:
  /** Name to be shown on the widget. */
  utils::Utf8String name_;

  /** Escaped name for drawing. */
  utils::Utf8String escaped_name_;

  NameValidator name_validator_;

  BOOST_DESCRIBE_CLASS (NamedObject, (ArrangerObject), (), (name_), ())
};

using NamedObjectVariant =
  std::variant<MidiRegion, AudioRegion, ChordRegion, AutomationRegion, Marker>;
using NamedObjectPtrVariant = to_pointer_variant<NamedObjectVariant>;

} // namespace zrythm::structure::arrangement
