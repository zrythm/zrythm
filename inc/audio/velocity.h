/*
 * audio/velocity.h - velocity for MIDI notes
 *
 * Copyright (C) 2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_VELOCITY_H__
#define __AUDIO_VELOCITY_H__

#include <cyaml/cyaml.h>

/**
 * @addtogroup audio
 *
 * @{
 */

typedef struct MidiNote MidiNote;
typedef struct _VelocityWidget VelocityWidget;

typedef struct Velocity
{
  int              vel; ///< velocity value (0-127)

  /**
   * Owner.
   *
   * For convenience only.
   */
  MidiNote *       midi_note; ///< cache

  VelocityWidget * widget; ///< widget
} Velocity;

static const cyaml_schema_field_t
  velocity_fields_schema[] =
{
	CYAML_FIELD_INT (
			"vel", CYAML_FLAG_DEFAULT,
			Velocity, vel),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
velocity_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
			Velocity, velocity_fields_schema),
};

Velocity *
velocity_new (int        vel);

Velocity *
velocity_clone (Velocity * src);

Velocity *
velocity_default ();

/**
 * Destroys the velocity instance.
 */
void
velocity_free (Velocity * self);

/**
 * @}
 */

#endif
