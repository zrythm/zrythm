// SPDX-FileCopyrightText: © 2018-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/musical_scale.h"
#include "structure/arrangement/bounded_object.h"
#include "structure/arrangement/muteable_object.h"
#include "structure/arrangement/timeline_object.h"
#include "utils/icloneable.h"
#include "utils/serialization.h"

namespace zrythm::structure::arrangement
{

class ScaleObject final : public QObject, public TimelineObject, public MuteableObject
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (ScaleObject)
  Q_PROPERTY (QString name READ getName NOTIFY nameChanged)

public:
  using MusicalScale = dsp::MusicalScale;

public:
  DECLARE_FINAL_ARRANGER_OBJECT_CONSTRUCTORS (ScaleObject)

  // =========================================================
  // QML Interface
  // =========================================================

  QString getName () const { return gen_human_friendly_name ().to_qstring (); }
  Q_SIGNAL void nameChanged (const QString &name);

  // =========================================================

  friend void init_from (
    ScaleObject           &obj,
    const ScaleObject     &other,
    utils::ObjectCloneType clone_type);

  void set_scale (const MusicalScale &scale) { scale_ = scale; }

  utils::Utf8String gen_human_friendly_name () const override;

  ArrangerObjectPtrVariant
  add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtrVariant insert_clone_to_project () const override;

  bool
  validate (bool is_project, dsp::FramesPerTick frames_per_tick) const override;

private:
  static constexpr std::string_view kScaleKey = "scale";
  friend void to_json (nlohmann::json &j, const ScaleObject &so)
  {
    to_json (j, static_cast<const ArrangerObject &> (so));
    to_json (j, static_cast<const MuteableObject &> (so));
    j[kScaleKey] = so.scale_;
  }
  friend void from_json (const nlohmann::json &j, ScaleObject &so)
  {
    from_json (j, static_cast<ArrangerObject &> (so));
    from_json (j, static_cast<MuteableObject &> (so));
    j.at (kScaleKey).get_to (so.scale_);
  }

public:
  /** The scale descriptor. */
  MusicalScale scale_;

  BOOST_DESCRIBE_CLASS (
    ScaleObject,
    (TimelineObject, MuteableObject),
    (scale_),
    (),
    ())
};

} // namespace zrythm::structure::arrangement
