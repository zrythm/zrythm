// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_NAMEABLE_OBJECT_H__
#define __DSP_NAMEABLE_OBJECT_H__

#include "gui/dsp/arranger_object.h"

#define DEFINE_NAMEABLE_OBJECT_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* name */ \
  /* ================================================================ */ \
  Q_PROPERTY (QString name READ getName WRITE setName NOTIFY nameChanged) \
  QString getName () const \
  { \
    return QString::fromStdString (name_); \
  } \
  void setName (const QString &name) \
  { \
    if (name_ == name.toStdString ()) \
      return; \
    set_name_with_action (name.toStdString ()); \
    Q_EMIT nameChanged (name); \
  } \
  Q_SIGNAL void nameChanged (const QString &name);

/**
 * @class NamedObject
 * @brief Base class for objects that have a name.
 *
 * This class provides a common interface for objects that have a name. It
 * includes methods for getting, setting, and validating the name, as well as
 * generating an escaped version of the name for drawing purposes.
 *
 * Derived classes must implement the pure virtual methods defined in this class.
 */
class NamedObject
    : virtual public ArrangerObject,
      public zrythm::utils::serialization::ISerializable<NamedObject>
{
public:
  NamedObject () noexcept = default;
  NamedObject (std::string name) : name_ (std::move (name))
  {
    gen_escaped_name ();
  }
  Q_DISABLE_COPY_MOVE (NamedObject)
  ~NamedObject () override = default;

  void init_loaded_base ();

  /**
   * Returns the name of the object.
   */
  virtual std::string get_name () const final { return name_; }

  /**
   * Generates the escaped name for the object.
   */
  virtual void gen_escaped_name () final;

  /**
   * Validates the given name.
   *
   * @return True if valid, false otherwise.
   */
  virtual bool validate_name (const std::string &name)
  {
    /* by default, all names are valid unless the derived class overrides this */
    return true;
  };

  /**
   * @brief Sets the name of the object.
   *
   * @param name The new name for the object.
   * @param fire_events Whether to fire events when the name is changed.
   */
  virtual void set_name (const std::string &name, bool fire_events) final;

  /**
   * Changes the name and adds an action to the undo stack.
   *
   * Calls @ref set_name() internally.
   */
  virtual void set_name_with_action (const std::string &name) final;

  std::string gen_human_friendly_name () const final { return name_; }

protected:
  void copy_members_from (const NamedObject &other, ObjectCloneType clone_type)
  {
    name_ = other.name_;
    escaped_name_ = other.escaped_name_;
  }

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  /** Name to be shown on the widget. */
  std::string name_;

  /** Escaped name for drawing. */
  std::string escaped_name_;
};

inline bool
operator== (const NamedObject &lhs, const NamedObject &rhs)
{
  return lhs.name_ == rhs.name_;
}

using NamedObjectVariant =
  std::variant<MidiRegion, AudioRegion, ChordRegion, AutomationRegion, Marker>;
using NamedObjectPtrVariant = to_pointer_variant<NamedObjectVariant>;

#endif // __DSP_NAMEABLE_OBJECT_H__
