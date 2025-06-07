// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/chord_descriptor.h"
#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/bounded_object.h"
#include "structure/arrangement/muteable_object.h"
#include "structure/arrangement/region_owned_object.h"
#include "utils/icloneable.h"

namespace zrythm::structure::arrangement
{
class ChordRegion;

/**
 * The ChordObject class represents a chord inside a ChordRegion. It inherits
 * from MuteableObject and RegionOwnedObject.
 *
 * The class provides methods to set the region and index of the chord, get the
 * associated ChordDescriptor, and find the ChordObject corresponding to a given
 * position.
 *
 * The chord_index_ member variable stores the index of the chord in the chord
 * pad (0 being the topmost chord). The magic_ member variable is used to
 * identify valid ChordObject instances. The layout member variable is a cache
 * for the Pango layout used to draw the chord name.
 */
class ChordObject final : public QObject, public MuteableObject, public RegionOwnedObject
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (ChordObject)
  Q_PROPERTY (
    ChordDescriptor * chordDescriptor READ getChordDescriptor WRITE
      setChordDescriptor NOTIFY chordDescriptorChanged)

public:
  using RegionT = ChordRegion;
  using ChordDescriptor = dsp::ChordDescriptor;

  static constexpr int WIDGET_TRIANGLE_WIDTH = 10;

public:
  DECLARE_FINAL_ARRANGER_OBJECT_CONSTRUCTORS (ChordObject)

  // ========================================================================
  // QML Interface
  // ========================================================================

  ChordDescriptor * getChordDescriptor () const;

  void          setChordDescriptor (ChordDescriptor * descr);
  Q_SIGNAL void chordDescriptorChanged (ChordDescriptor *);

  // ========================================================================

  void set_chord_descriptor (int index);

  ArrangerObjectPtrVariant
  add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtrVariant insert_clone_to_project () const override;

  utils::Utf8String gen_human_friendly_name () const override;

  friend bool operator== (const ChordObject &lhs, const ChordObject &rhs)
  {
    return static_cast<const ArrangerObject &> (lhs)
             == static_cast<const ArrangerObject &> (rhs)
           && lhs.chord_index_ == rhs.chord_index_
           && static_cast<const RegionOwnedObject &> (lhs)
                == static_cast<const RegionOwnedObject &> (rhs)
           && static_cast<const MuteableObject &> (lhs)
                == static_cast<const MuteableObject &> (rhs);
  }

  bool
  validate (bool is_project, dsp::FramesPerTick frames_per_tick) const override;

  friend void init_from (
    ChordObject           &obj,
    const ChordObject     &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr std::string_view kChordIndexKey = "chordIndex";
  friend void to_json (nlohmann::json &j, const ChordObject &co)
  {
    to_json (j, static_cast<const ArrangerObject &> (co));
    to_json (j, static_cast<const MuteableObject &> (co));
    to_json (j, static_cast<const RegionOwnedObject &> (co));
    j[kChordIndexKey] = co.chord_index_;
  }
  friend void from_json (const nlohmann::json &j, ChordObject &co)
  {
    from_json (j, static_cast<ArrangerObject &> (co));
    from_json (j, static_cast<MuteableObject &> (co));
    from_json (j, static_cast<RegionOwnedObject &> (co));
    j.at (kChordIndexKey).get_to (co.chord_index_);
  }

public:
  /** The index of the chord it belongs to (0 topmost). */
  int chord_index_ = 0;

  BOOST_DESCRIBE_CLASS (
    ChordObject,
    (MuteableObject, RegionOwnedObject),
    (chord_index_),
    (),
    ())
};

}
