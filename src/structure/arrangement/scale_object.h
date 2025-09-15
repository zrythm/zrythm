// SPDX-FileCopyrightText: © 2018-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/musical_scale.h"
#include "structure/arrangement/arranger_object.h"
#include "utils/icloneable.h"

namespace zrythm::structure::arrangement
{

class ScaleObject final : public ArrangerObject
{
  Q_OBJECT
  Q_PROPERTY (MusicalScale * scale READ scale WRITE setScale NOTIFY scaleChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using MusicalScale = dsp::MusicalScale;

public:
  ScaleObject (const dsp::TempoMap &tempo_map, QObject * parent = nullptr);

  // =========================================================
  // QML Interface
  // =========================================================

  MusicalScale * scale () const { return scale_.get (); }
  void           setScale (MusicalScale * scale)
  {
    if (scale == nullptr || scale == scale_.get ())
      return;

    scale->setParent (this);
    scale_ = scale;
    Q_EMIT scaleChanged (scale);
  }
  Q_SIGNAL void scaleChanged (MusicalScale * scale);

  // =========================================================

private:
  friend void init_from (
    ScaleObject           &obj,
    const ScaleObject     &other,
    utils::ObjectCloneType clone_type);

  static constexpr auto kScaleKey = "scale"sv;
  friend void           to_json (nlohmann::json &j, const ScaleObject &so)
  {
    to_json (j, static_cast<const ArrangerObject &> (so));
    j[kScaleKey] = so.scale_;
  }
  friend void from_json (const nlohmann::json &j, ScaleObject &so)
  {
    from_json (j, static_cast<ArrangerObject &> (so));
    j.at (kScaleKey).get_to (*so.scale_);
  }

private:
  /** The scale descriptor. */
  utils::QObjectUniquePtr<MusicalScale> scale_;

  BOOST_DESCRIBE_CLASS (ScaleObject, (ArrangerObject), (), (), (scale_))
};

} // namespace zrythm::structure::arrangement
