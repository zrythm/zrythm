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
 * Action for changing scales in a ScaleObject.
 */

#ifndef __UNDO_EDIT_SCALE_ACTION_H__
#define __UNDO_EDIT_SCALE_ACTION_H__

#include "actions/undoable_action.h"

typedef struct MusicalScale MusicalScale;
typedef struct ScaleObject ScaleObject;

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * An action for changing the scale stored in a
 * ScaleObject.
 */
typedef struct EditScaleAction
{
  UndoableAction    parent_instance;

  /** ScaleObject clone. */
  ScaleObject *     scale;

  /** Clone of new MusicalScale */
  MusicalScale * descr;
} EditScaleAction;

UndoableAction *
edit_scale_action_new (
  ScaleObject *  scale,
  MusicalScale * descr);

int
edit_scale_action_do (
	EditScaleAction * self);

int
edit_scale_action_undo (
	EditScaleAction * self);

char *
edit_scale_action_stringize (
	EditScaleAction * self);

void
edit_scale_action_free (
	EditScaleAction * self);

/**
 * @}
 */

#endif
