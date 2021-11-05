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

#include "gui/backend/wrapped_object_with_change_signal.h"
#include "utils/error.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  WrappedObjectWithChangeSignal,
  wrapped_object_with_change_signal,
  G_TYPE_OBJECT)

enum
{
  SIGNAL_CHANGED,
  N_SIGNALS
};

static guint obj_signals[N_SIGNALS] = { 0 };

/**
 * Fires the signal.
 */
void
wrapped_object_with_change_signal_fire (
  WrappedObjectWithChangeSignal * self)
{
  g_signal_emit (self, obj_signals[SIGNAL_CHANGED], 0);
}

WrappedObjectWithChangeSignal *
wrapped_object_with_change_signal_new (
  void *            obj,
  WrappedObjectType type)
{
  WrappedObjectWithChangeSignal * self =
    g_object_new (
      WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE, NULL);

  self->type = type;
  self->obj = obj;

  return self;
}

static void
wrapped_object_with_change_signal_class_init (
  WrappedObjectWithChangeSignalClass * klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (klass);

  obj_signals[SIGNAL_CHANGED] =
  g_signal_newv (
    "changed",
    G_TYPE_FROM_CLASS (oklass),
    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE
      | G_SIGNAL_NO_HOOKS,
    NULL /* closure */,
    NULL /* accumulator */,
    NULL /* accumulator data */,
    NULL /* C marshaller */,
    G_TYPE_NONE /* return_type */,
    0     /* n_params */,
    NULL  /* param_types */);
}

static void
wrapped_object_with_change_signal_init (
  WrappedObjectWithChangeSignal * self)
{
}
