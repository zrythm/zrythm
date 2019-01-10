/*
 * audio/velocity.h - velocity for MIDI notes
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __AUDIO_VELOCITY_H__
#define __AUDIO_VELOCITY_H__

typedef struct MidiNote MidiNote;
typedef struct _VelocityWidget VelocityWidget;

typedef struct Velocity
{
  int              vel; ///< velocity value (0-127)
  MidiNote *       midi_note; ///< pointer back to MIDI note
  VelocityWidget * widget; ///< widget
} Velocity;

Velocity *
velocity_new (int        vel);

Velocity *
velocity_default ();

#endif
