// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CHORD_OBJECT_H__
#define __AUDIO_CHORD_OBJECT_H__

#include "dsp/chord_descriptor.h"
#include "gui/dsp/arranger_object.h"
#include "gui/dsp/bounded_object.h"
#include "gui/dsp/muteable_object.h"
#include "gui/dsp/region_owned_object.h"
#include "utils/icloneable.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

using namespace zrythm;

namespace zrythm::dsp
{
class ChordDescriptor;
}
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
class ChordObject final
    : public QObject,
      public MuteableObject,
      public RegionOwnedObject,
      public ICloneable<ChordObject>,
      public zrythm::utils::serialization::ISerializable<ChordObject>
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
  ChordObject (const DeserializationDependencyHolder &dh)
      : ChordObject (
          dh.get<std::reference_wrapper<ArrangerObjectRegistry>> ().get ())
  {
  }
  ChordObject (ArrangerObjectRegistry &obj_registry, QObject * parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  ChordDescriptor * getChordDescriptor () const;

  void          setChordDescriptor (ChordDescriptor * descr);
  Q_SIGNAL void chordDescriptorChanged (ChordDescriptor *);

  // ========================================================================

  void set_chord_descriptor (int index);

  std::optional<ArrangerObjectPtrVariant> find_in_project () const override;

  ArrangerObjectPtrVariant
  add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtrVariant insert_clone_to_project () const override;

  std::string gen_human_friendly_name () const override;

  friend bool operator== (const ChordObject &lhs, const ChordObject &rhs);

  bool
  validate (bool is_project, dsp::FramesPerTick frames_per_tick) const override;

  void init_after_cloning (const ChordObject &other, ObjectCloneType clone_type)
    override;

  void init_loaded () override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** The index of the chord it belongs to (0 topmost). */
  int chord_index_ = 0;
};

inline bool
operator== (const ChordObject &lhs, const ChordObject &rhs)
{
  return static_cast<const ArrangerObject &> (lhs)
           == static_cast<const ArrangerObject &> (rhs)
         && lhs.chord_index_ == rhs.chord_index_
         && static_cast<const RegionOwnedObject &> (lhs)
              == static_cast<const RegionOwnedObject &> (rhs)
         && static_cast<const MuteableObject &> (lhs)
              == static_cast<const MuteableObject &> (rhs);
}

DEFINE_OBJECT_FORMATTER (ChordObject, ChordObject, [] (const ChordObject &co) {
  return fmt::format (
    "ChordObject [{}]: chord ID {}", *co.pos_, co.chord_index_);
});

/**
 * @}
 */

#endif
