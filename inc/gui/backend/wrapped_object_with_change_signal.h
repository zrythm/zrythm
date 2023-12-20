// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Object with change signal.
 */

#ifndef __GUI_BACKEND_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_H__
#define __GUI_BACKEND_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_H__

#include "utils/types.h"

#include <glib-object.h>

#define WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE \
  (wrapped_object_with_change_signal_get_type ())
G_DECLARE_FINAL_TYPE (
  WrappedObjectWithChangeSignal,
  wrapped_object_with_change_signal,
  Z,
  WRAPPED_OBJECT_WITH_CHANGE_SIGNAL,
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
  WRAPPED_OBJECT_TYPE_PROJECT_INFO,
  WRAPPED_OBJECT_TYPE_PORT,
  WRAPPED_OBJECT_TYPE_CHANNEL_SEND_TARGET,
  WRAPPED_OBJECT_TYPE_PLUGIN_COLLECTION,
  WRAPPED_OBJECT_TYPE_EXT_PORT,
  WRAPPED_OBJECT_TYPE_FILE_BROWSER_LOCATION,
} WrappedObjectType;

/**
 * A GObject-ified normal C object with a signal that interested
 * parties can listen to for changes.
 *
 * To be used in list view models and other APIs that require
 * using a GObject.
 */
typedef struct _WrappedObjectWithChangeSignal
{
  GObject parent_instance;

  WrappedObjectType type;
  void *            obj;

  /** Parent model, if using tree model. */
  GListModel * parent_model;

  /** Model containing the children for this object (if using
   * tree model). */
  GListModel * child_model;

  ObjectFreeFunc free_func;
} WrappedObjectWithChangeSignal;

/**
 * Fires the signal.
 *
 * @memberof WrappedObjectWithChangeSignal
 */
void
wrapped_object_with_change_signal_fire (WrappedObjectWithChangeSignal * self);

/**
 * Returns a display name for the given object, intended to be used where the
 * object should be displayed (eg, a dropdown).
 *
 * This can be used with GtkCclosureExpression.
 */
char *
wrapped_object_with_change_signal_get_display_name (void * data);

/**
 * Instantiates a new WrappedObjectWithChangeSignal.
 */
WrappedObjectWithChangeSignal *
wrapped_object_with_change_signal_new (void * obj, WrappedObjectType type);

/**
 * If this function is not used, the internal object will not be free'd.
 */
WrappedObjectWithChangeSignal *
wrapped_object_with_change_signal_new_with_free_func (
  void *            obj,
  WrappedObjectType type,
  ObjectFreeFunc    free_func);

/**
 * @}
 */

#endif
