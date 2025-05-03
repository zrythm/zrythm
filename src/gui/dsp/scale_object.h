// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_SCALE_OBJECT_H__
#define __AUDIO_SCALE_OBJECT_H__

#include "dsp/musical_scale.h"
#include "gui/dsp/bounded_object.h"
#include "gui/dsp/muteable_object.h"
#include "gui/dsp/timeline_object.h"
#include "utils/icloneable.h"
#include "utils/iserializable.h"

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
      public ICloneable<ScaleObject>,
      public zrythm::utils::serialization::ISerializable<ScaleObject>
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

  QString getName () const
  {
    return utils::std_string_to_qstring (gen_human_friendly_name ());
  }
  Q_SIGNAL void nameChanged (const QString &name);

  // =========================================================

  void init_after_cloning (const ScaleObject &other, ObjectCloneType clone_type)
    override;

  void set_scale (const MusicalScale &scale) { scale_ = scale; }

  std::string gen_human_friendly_name () const override;

  ArrangerObjectPtrVariant
  add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtrVariant insert_clone_to_project () const override;

  bool
  validate (bool is_project, dsp::FramesPerTick frames_per_tick) const override;

  DECLARE_DEFINE_FIELDS_METHOD ();

  friend bool operator== (const ScaleObject &a, const ScaleObject &b);

public:
  /** The scale descriptor. */
  MusicalScale scale_;
};

inline bool
operator== (const ScaleObject &a, const ScaleObject &b)
{
  return static_cast<const TimelineObject &> (a)
           == static_cast<const TimelineObject &> (b)
         && static_cast<const MuteableObject &> (a)
              == static_cast<const MuteableObject &> (b)
         && static_cast<const ArrangerObject &> (a)
              == static_cast<const ArrangerObject &> (b)
         && a.scale_ == b.scale_;
}

DEFINE_OBJECT_FORMATTER (ScaleObject, ScaleObject, [] (const ScaleObject &so) {
  return fmt::format ("ScaleObject[position: {}]", so.get_position ());
})

/**
 * @}
 */

#endif
