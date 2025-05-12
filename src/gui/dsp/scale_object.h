// SPDX-FileCopyrightText: © 2018-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_SCALE_OBJECT_H__
#define __AUDIO_SCALE_OBJECT_H__

#include "dsp/musical_scale.h"
#include "gui/dsp/bounded_object.h"
#include "gui/dsp/muteable_object.h"
#include "gui/dsp/timeline_object.h"
#include "utils/icloneable.h"
#include "utils/serialization.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

using namespace zrythm;

class ScaleObject final
    : public QObject,
      public TimelineObject,
      public MuteableObject,
      public ICloneable<ScaleObject>
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

  void init_after_cloning (const ScaleObject &other, ObjectCloneType clone_type)
    override;

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
};

DEFINE_OBJECT_FORMATTER (ScaleObject, ScaleObject, [] (const ScaleObject &so) {
  return fmt::format ("ScaleObject[position: {}]", so.get_position ());
})

/**
 * @}
 */

#endif
