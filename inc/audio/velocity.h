/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Velocities for MidiNote's.
 */

#ifndef __AUDIO_VELOCITY_H__
#define __AUDIO_VELOCITY_H__

#include "gui/backend/arranger_object.h"
#include "gui/backend/arranger_object_info.h"

#include <cyaml/cyaml.h>

typedef struct MidiNote MidiNote;
typedef struct _VelocityWidget VelocityWidget;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Default velocity.
 */
#define VELOCITY_DEFAULT 90

typedef enum VelocityCloneFlag
{
  /** Create a new Velocity to be added to a
   * MidiNote as a main Velocity. */
  VELOCITY_CLONE_COPY_MAIN,

  /** Create a new Velocity that will not be used
   * as a main Velocity. */
  VELOCITY_CLONE_COPY,
} VelocityCloneFlag;

/**
 * The MidiNote velocity.
 */
typedef struct Velocity
{
  /** Velocity value (0-127). */
  int              vel;

  /**
   * Owner.
   *
   * For convenience only.
   */
  MidiNote *       midi_note;

  /** The widget. */
  VelocityWidget * widget;

  ArrangerObjectInfo obj_info;
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
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    Velocity, velocity_fields_schema),
};

#define velocity_is_main(r) \
  arranger_object_info_is_main ( \
    &r->obj_info)

#define velocity_is_transient(r) \
  arranger_object_info_is_transient ( \
    &r->obj_info)

/** Gets the transient counterpart of the
 * Velocity. */
#define velocity_get_main_trans_velocity(r) \
  ((Velocity *) r->obj_info.main_trans)

/** Gets the main counterpart of the
 * Velocity. */
#define velocity_get_main_velocity(r) \
  ((Velocity *) r->obj_info.main)

/**
 * Init after loading a Project.
 */
void
velocity_init_loaded (
  Velocity * self);

/**
 * Creates a new Velocity with the given value.
 */
Velocity *
velocity_new (
  MidiNote * midi_note,
  int        vel,
  int        is_main);

/**
 * Sets the MidiNote the Velocity belongs to.
 */
void
velocity_set_midi_note (
  Velocity * velocity,
  MidiNote * midi_note);

/**
 * Finds the actual Velocity in the project from the
 * given clone.
 */
Velocity *
velocity_find (
  Velocity * clone);

/**
 * Clones the Velocity.
 */
Velocity *
velocity_clone (
  Velocity * src,
  VelocityCloneFlag flag);

/**
 * Returns 1 if the Velocity's match, 0 if not.
 */
int
velocity_is_equal (
  Velocity * src,
  Velocity * dest);

/**
 * Wrapper that calls midi_note_is_selected().
 */
#define velocity_is_selected(vel) \
  midi_note_is_selected ( \
    velocity_get_main_velocity (vel)->midi_note)

/**
 * Returns if Velocity is (should be) visible.
 */
#define velocity_should_be_visible(vel) \
  arranger_object_info_should_be_visible ( \
    vel->obj_info)

ARRANGER_OBJ_DECLARE_GEN_WIDGET (
  Velocity, velocity);

/**
 * Changes the Velocity by the given amount of
 * values (delta).
 */
void
velocity_shift (
  Velocity * self,
  int        delta);

/**
 * Sets the velocity to the given value.
 */
void
velocity_set_val (
  Velocity * self,
  int        val);

ARRANGER_OBJ_DECLARE_FREE_ALL_LANELESS (
  Velocity, velocity);

/**
 * Destroys the velocity instance.
 */
void
velocity_free (Velocity * self);

/**
 * @}
 */

#endif
