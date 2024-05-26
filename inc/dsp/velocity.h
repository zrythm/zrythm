// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Velocities for MidiNote's.
 */

#ifndef __AUDIO_VELOCITY_H__
#define __AUDIO_VELOCITY_H__

#include <cstdint>

#include "dsp/region_identifier.h"
#include "gui/backend/arranger_object.h"

typedef struct MidiNote        MidiNote;
typedef struct _VelocityWidget VelocityWidget;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define velocity_is_selected(r) \
  arranger_object_is_selected ((ArrangerObject *) r)

/**
 * Default velocity.
 */
#define VELOCITY_DEFAULT 90

/**
 * The MidiNote velocity.
 *
 * @extends ArrangerObject
 */
typedef struct Velocity
{
  /** Base struct. */
  ArrangerObject base;

  /** Velocity value (0-127). */
  uint8_t vel;

  /** Velocity at drag begin - used for ramp
   * actions only. */
  uint8_t vel_at_start;

  /** Pointer back to the MIDI note. */
  MidiNote * midi_note;
} Velocity;

/**
 * Creates a new Velocity with the given value.
 */
Velocity *
velocity_new (MidiNote * midi_note, const uint8_t vel);

/**
 * Sets the MidiNote the Velocity belongs to.
 */
void
velocity_set_midi_note (Velocity * velocity, MidiNote * midi_note);

/**
 * Returns 1 if the Velocity's match, 0 if not.
 */
int
velocity_is_equal (Velocity * src, Velocity * dest);

/**
 * Changes the Velocity by the given amount of
 * values (delta).
 */
void
velocity_shift (Velocity * self, const int delta);

/**
 * Sets the velocity to the given value.
 *
 * The given value may exceed the bounds 0-127,
 * and will be clamped.
 */
void
velocity_set_val (Velocity * self, const int val);

/**
 * Returns the owner MidiNote.
 */
MidiNote *
velocity_get_midi_note (const Velocity * const self);

const char *
velocity_setting_enum_to_str (guint index);

guint
velocity_setting_str_to_enum (const char * str);

/**
 * @}
 */

#endif
