// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MARKER_H__
#define __AUDIO_MARKER_H__

/**
 * @addtogroup dsp
 *
 * @{
 */

#include "gui/dsp/bounded_object.h"
#include "gui/dsp/named_object.h"
#include "gui/dsp/timeline_object.h"
#include "utils/icloneable.h"

/**
 * Marker for the MarkerTrack.
 */
class Marker final
    : public QObject,
      public TimelineObject,
      public NamedObject,
      public ICloneable<Marker>,
      public zrythm::utils::serialization::ISerializable<Marker>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (Marker)
  DEFINE_NAMEABLE_OBJECT_QML_PROPERTIES (Marker)

public:
  /**
   * Marker type.
   */
  enum class Type
  {
    /** Song start Marker that cannot be deleted. */
    Start,
    /** Song end Marker that cannot be deleted. */
    End,
    /** Custom Marker. */
    Custom,
  };

  Marker (const DeserializationDependencyHolder &dh)
      : Marker (
          dh.get<std::reference_wrapper<ArrangerObjectRegistry>> ().get (),
          dh.get<NameValidator> ())
  {
  }
  Marker (
    ArrangerObjectRegistry &obj_registry,
    NameValidator           name_validator,
    QObject *               parent = nullptr);

  bool is_start () const { return marker_type_ == Type::Start; }
  bool is_end () const { return marker_type_ == Type::End; }

  bool is_deletable () const override
  {
    return marker_type_ != Type::Start && marker_type_ != Type::End;
  }

  void init_loaded () override;

  void
  init_after_cloning (const Marker &other, ObjectCloneType clone_type) override;

  std::optional<ArrangerObjectPtrVariant> find_in_project () const override;

  ArrangerObjectPtrVariant
  add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtrVariant insert_clone_to_project () const override;

  bool validate (bool is_project, double frames_per_tick) const override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Marker type. */
  Type marker_type_ = Type::Custom;
};

inline bool
operator== (const Marker &lhs, const Marker &rhs)
{
  return static_cast<const TimelineObject &> (lhs)
           == static_cast<const TimelineObject &> (rhs)
         && static_cast<const NamedObject &> (lhs)
              == static_cast<const NamedObject &> (rhs)
         && static_cast<const ArrangerObject &> (lhs)
              == static_cast<const ArrangerObject &> (rhs)
         && lhs.marker_type_ == rhs.marker_type_;
}

DEFINE_OBJECT_FORMATTER (Marker, Marker, [] (const Marker &m) {
  return fmt::format (
    "Marker[name: {}, type: {}, position: {}]", m.get_name (),
    ENUM_NAME (m.marker_type_), *m.pos_);
})

/**
 * @}
 */

#endif
