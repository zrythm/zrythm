// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/dsp/arranger_object_owner.h"
#include "gui/dsp/chord_object.h"
#include "gui/dsp/region.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

class ChordRegion final
    : public QObject,
      public RegionImpl<ChordRegion>,
      public ArrangerObjectOwner<ChordObject>,
      public ICloneable<ChordRegion>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (ChordRegion)
  DEFINE_REGION_QML_PROPERTIES (ChordRegion)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    ChordRegion,
    chordObjects,
    ChordObject)

  friend class RegionImpl<ChordRegion>;

public:
  DECLARE_FINAL_ARRANGER_OBJECT_CONSTRUCTORS (ChordRegion)

  using RegionT = RegionImpl<ChordRegion>;

  bool
  validate (bool is_project, dsp::FramesPerTick frames_per_tick) const override;

  Location get_location (const ChordObject &) const override
  {
    return { .track_id_ = track_id_, .owner_ = get_uuid () };
  }

  std::string
  get_field_name_for_serialization (const ChordObject *) const override
  {
    return "chordObjects";
  }

  void init_after_cloning (const ChordRegion &other, ObjectCloneType clone_type)
    override;

private:
  friend void to_json (nlohmann::json &j, const ChordRegion &cr)
  {
    to_json (j, static_cast<const ArrangerObject &> (cr));
    to_json (j, static_cast<const BoundedObject &> (cr));
    to_json (j, static_cast<const LoopableObject &> (cr));
    to_json (j, static_cast<const MuteableObject &> (cr));
    to_json (j, static_cast<const NamedObject &> (cr));
    to_json (j, static_cast<const ColoredObject &> (cr));
    to_json (j, static_cast<const Region &> (cr));
    to_json (j, static_cast<const ArrangerObjectOwner &> (cr));
  }
  friend void from_json (const nlohmann::json &j, ChordRegion &cr)
  {
    from_json (j, static_cast<ArrangerObject &> (cr));
    from_json (j, static_cast<BoundedObject &> (cr));
    from_json (j, static_cast<LoopableObject &> (cr));
    from_json (j, static_cast<MuteableObject &> (cr));
    from_json (j, static_cast<NamedObject &> (cr));
    from_json (j, static_cast<ColoredObject &> (cr));
    from_json (j, static_cast<Region &> (cr));
    from_json (j, static_cast<ArrangerObjectOwner &> (cr));
  }
};

DEFINE_OBJECT_FORMATTER (ChordRegion, ChordRegion, [] (const ChordRegion &cr) {
  return fmt::format (
    "ChordRegion[id: {}, {}]", cr.get_uuid (), static_cast<const Region &> (cr));
})

/**
 * @}
 */
