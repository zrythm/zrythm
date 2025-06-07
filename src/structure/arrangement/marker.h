// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/bounded_object.h"
#include "structure/arrangement/named_object.h"
#include "structure/arrangement/timeline_object.h"
#include "utils/icloneable.h"

namespace zrythm::structure::arrangement
{

/**
 * Marker for the MarkerTrack.
 */
class Marker final : public QObject, public TimelineObject, public NamedObject
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

  Marker (
    ArrangerObjectRegistry &obj_registry,
    TrackResolver           track_resolver,
    NameValidator           name_validator,
    QObject *               parent = nullptr);

  bool is_start () const { return marker_type_ == Type::Start; }
  bool is_end () const { return marker_type_ == Type::End; }

  bool is_deletable () const override
  {
    return marker_type_ != Type::Start && marker_type_ != Type::End;
  }

  friend void
  init_from (Marker &obj, const Marker &other, utils::ObjectCloneType clone_type);

  ArrangerObjectPtrVariant
  add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtrVariant insert_clone_to_project () const override;

  bool
  validate (bool is_project, dsp::FramesPerTick frames_per_tick) const override;

private:
  static constexpr std::string_view kMarkerTypeKey = "markerType";
  friend void                       to_json (nlohmann::json &j, const Marker &m)
  {
    to_json (j, static_cast<const ArrangerObject &> (m));
    to_json (j, static_cast<const NamedObject &> (m));
    j[kMarkerTypeKey] = m.marker_type_;
  }
  friend void from_json (const nlohmann::json &j, Marker &m)
  {
    from_json (j, static_cast<ArrangerObject &> (m));
    from_json (j, static_cast<NamedObject &> (m));
    j.at (kMarkerTypeKey).get_to (m.marker_type_);
  }

public:
  /** Marker type. */
  Type marker_type_ = Type::Custom;

  BOOST_DESCRIBE_CLASS (
    Marker,
    (NamedObject, TimelineObject),
    (marker_type_),
    (),
    ())
};

} // namespace zrythm::structure::arrangement
