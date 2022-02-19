/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Object with change signal.
 */

#ifndef __GUI_BACKEND_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_H__
#define __GUI_BACKEND_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_H__

#include <glib-object.h>

#define WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE \
  (wrapped_object_with_change_signal_get_type ())
G_DECLARE_FINAL_TYPE (
  WrappedObjectWithChangeSignal,
  wrapped_object_with_change_signal,
  Z, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL,
  GObject)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef enum WrappedObjectType
{
  WRAPPED_OBJECT_TYPE_TRACK,
  WRAPPED_OBJECT_TYPE_PLUGIN,
  WRAPPED_OBJECT_TYPE_PLUGIN_DESCR,
  WRAPPED_OBJECT_TYPE_CHORD_DESCR,
  WRAPPED_OBJECT_TYPE_CHORD_PSET,
  WRAPPED_OBJECT_TYPE_CHORD_PSET_PACK,
  WRAPPED_OBJECT_TYPE_SUPPORTED_FILE,
  WRAPPED_OBJECT_TYPE_MIDI_MAPPING,
  WRAPPED_OBJECT_TYPE_ARRANGER_OBJECT,
} WrappedObjectType;

typedef struct _WrappedObjectWithChangeSignal
{
  GObject           parent_instance;

  WrappedObjectType type;
  void *            obj;
} WrappedObjectWithChangeSignal;

/**
 * Fires the signal.
 */
void
wrapped_object_with_change_signal_fire (
  WrappedObjectWithChangeSignal * self);

/**
 * Returns a display name for the given object,
 * intended to be used where the object should be
 * displayed (eg, a dropdown).
 *
 * This can be used with GtkCclosureExpression.
 */
char *
wrapped_object_with_change_signal_get_display_name (
  void * data);

/**
 * Instantiates a new WrappedObjectWithChangeSignal.
 */
WrappedObjectWithChangeSignal *
wrapped_object_with_change_signal_new (
  void *            obj,
  WrappedObjectType type);

/**
 * @}
 */

#endif
