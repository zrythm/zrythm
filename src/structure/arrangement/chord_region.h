// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/chord_object.h"
#include "structure/arrangement/region.h"

namespace zrythm::structure::arrangement
{
class ChordRegion final
    : public QObject,
      public ArrangerObject,
      public ArrangerObjectOwner<ChordObject>
{
  Q_OBJECT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (ChordRegion)
  Q_PROPERTY (RegionMixin * regionMixin READ regionMixin CONSTANT)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    ChordRegion,
    chordObjects,
    ChordObject)
  QML_ELEMENT

public:
  ChordRegion (
    const dsp::TempoMap          &tempo_map,
    ArrangerObjectRegistry       &object_registry,
    dsp::FileAudioSourceRegistry &file_audio_source_registry,
    QObject *                     parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  RegionMixin * regionMixin () const { return region_mixin_.get (); }

  // ========================================================================

  std::string
  get_field_name_for_serialization (const ChordObject *) const override
  {
    return "chordObjects";
  }

private:
  friend void init_from (
    ChordRegion           &obj,
    const ChordRegion     &other,
    utils::ObjectCloneType clone_type);

  static constexpr auto kRegionMixinKey = "regionMixin"sv;
  friend void           to_json (nlohmann::json &j, const ChordRegion &region)
  {
    to_json (j, static_cast<const ArrangerObject &> (region));
    j[kRegionMixinKey] = region.region_mixin_;
    to_json (j, static_cast<const ArrangerObjectOwner &> (region));
  }
  friend void from_json (const nlohmann::json &j, ChordRegion &region)
  {
    from_json (j, static_cast<ArrangerObject &> (region));
    j.at (kRegionMixinKey).get_to (*region.region_mixin_);
    from_json (j, static_cast<ArrangerObjectOwner &> (region));
  }

private:
  utils::QObjectUniquePtr<RegionMixin> region_mixin_;

  BOOST_DESCRIBE_CLASS (
    ChordRegion,
    (ArrangerObject, ArrangerObjectOwner<ChordObject>),
    (),
    (),
    (region_mixin_))
};

}
