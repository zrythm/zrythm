// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MIDI_NOTE_H__
#define __AUDIO_MIDI_NOTE_H__

#include "zrythm-config.h"

#include <cstdint>
#include <memory>

#include "dsp/lengthable_object.h"
#include "dsp/muteable_object.h"
#include "dsp/position.h"
#include "dsp/velocity.h"
#include "gui/cpp/backend/piano_roll.h"
#include "utils/pango.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

constexpr int MIDI_NOTE_MAGIC = 3588791;
#define IS_MIDI_NOTE(tr) \
  ((tr) != nullptr && static_cast<MidiNote *> (tr)->magic_ == MIDI_NOTE_MAGIC)

/**
 * A MIDI note inside a Region shown in the piano roll.
 */
class MidiNote final
    : public MuteableObject,
      public RegionOwnedObjectImpl<MidiRegion>,
      public LengthableObject,
      public ICloneable<MidiNote>,
      public ISerializable<MidiNote>
{
public:
  MidiNote ();

  /**
   * Creates a new MidiNote.
   */
  MidiNote (
    const RegionIdentifier &region_id,
    Position                start_pos,
    Position                end_pos,
    uint8_t                 val,
    uint8_t                 vel);

  void init_loaded () override;

  void set_cache_val (const uint8_t val) { cache_val_ = val; }

  /**
   * Gets the MIDI note's value as a string (eg "C#4").
   *
   * @param use_markup Use markup to show the octave as a superscript.
   */
  std::string
  get_val_as_string (PianoRoll::NoteNotation notation, bool use_markup) const;

  /**
   * Listen to the given MidiNote.
   *
   * @param listen Turn note on if 1, or turn it
   *   off if 0.
   */
  void listen (bool listen);

  /**
   * Shifts MidiNote's position and/or value.
   *
   * @param delta Y (0-127)
   */
  void shift_pitch (const int delta);

  /**
   * Returns if the MIDI note is hit at given pos (in the timeline).
   */
  bool is_hit (const signed_frame_t gframes) const;

  /**
   * Sends a note off if currently playing and sets
   * the pitch of the MidiNote.
   */
  void set_val (const uint8_t val);

  ArrangerWidget * get_arranger () const override;

  ArrangerObjectPtr find_in_project () const override;

  std::string print_to_str () const override;

  ArrangerObjectPtr add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtr insert_clone_to_project () const override;

  // friend bool operator== (const MidiNote &lhs, const MidiNote &rhs);

  std::string gen_human_friendly_name () const override;

  bool validate (bool is_project, double frames_per_tick) const override;

  void init_after_cloning (const MidiNote &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Velocity. */
  std::shared_ptr<Velocity> vel_;

  /** The note/pitch, (0-127). */
  uint8_t val_ = 0;

  /** Cached note, for live operations. */
  uint8_t cache_val_ = 0;

  /** Whether or not this note is currently listened to */
  bool currently_listened_ = false;

  /** The note/pitch that is currently playing, if @ref
   * currently_listened_ is true. */
  uint8_t last_listened_val_ = 0;

  int magic_ = MIDI_NOTE_MAGIC;

  /** Cache layout for drawing the name. */
  PangoLayoutUniquePtr layout_;
};

inline bool
operator== (const MidiNote &lhs, const MidiNote &rhs)
{
  return lhs.val_ == rhs.val_ && *lhs.vel_ == *rhs.vel_
         && static_cast<const MuteableObject &> (lhs)
              == static_cast<const MuteableObject &> (rhs)
         && static_cast<const RegionOwnedObject &> (lhs)
              == static_cast<const RegionOwnedObject &> (rhs)
         && static_cast<const LengthableObject &> (lhs)
              == static_cast<const LengthableObject &> (rhs)
         && static_cast<const ArrangerObject &> (lhs)
              == static_cast<const ArrangerObject &> (rhs);
}

DEFINE_OBJECT_FORMATTER (MidiNote, [] (const MidiNote &mn) {
  return fmt::format (
    "MidiNote [{} ~ {}]: note {}, vel {}", mn.pos_, mn.end_pos_, mn.val_,
    mn.vel_->vel_);
});

/**
 * @}
 */

#endif // __AUDIO_MIDI_NOTE_H__
