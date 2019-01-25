/*
 * audio/piano_roll.h - piano roll back end
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_PIANO_ROLL_H__
#define __AUDIO_PIANO_ROLL_H__

typedef enum MidiModifier
{
  MIDI_MODIFIER_VELOCITY,
  MIDI_MODIFIER_PITCH_WHEEL,
  MIDI_MODIFIER_MOD_WHEEL,
  MIDI_MODIFIER_AFTERTOUCH,
} MidiModifier;

/**
 * Piano roll serializable backend.
 *
 * The actual widgets should reflect the information here.
 */
typedef struct PianoRoll
{
  int                    notes_zoom; ///< notes zoom level
  MidiModifier           midi_modifier; ///< selected midi modifier
} PianoRoll;

void
piano_roll_init (PianoRoll * self);

#endif
