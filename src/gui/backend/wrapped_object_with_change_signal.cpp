// SPDX-FileCopyrightText: © 2021-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/arranger_object.h"
#include "dsp/channel_send.h"
#include "dsp/ext_port.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "plugins/plugin_descriptor.h"
#include "settings/chord_preset_pack.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (
  WrappedObjectWithChangeSignal,
  wrapped_object_with_change_signal,
  G_TYPE_OBJECT)

enum
{
  SIGNAL_CHANGED,
  N_SIGNALS
};

static guint obj_signals[N_SIGNALS] = {};

/**
 * Fires the signal.
 */
void
wrapped_object_with_change_signal_fire (WrappedObjectWithChangeSignal * self)
{
  g_signal_emit (self, obj_signals[SIGNAL_CHANGED], 0);
}

/**
 * Returns a display name for the given object,
 * intended to be used where the object should be
 * displayed (eg, a dropdown).
 *
 * This can be used with GtkCclosureExpression.
 */
char *
wrapped_object_with_change_signal_get_display_name (void * data)
{
  g_return_val_if_fail (Z_IS_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data), nullptr);
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);

  switch (wrapped_obj->type)
    {
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_CHORD_PSET_PACK:
      {
        auto * pack = (ChordPresetPack *) wrapped_obj->obj;
        return g_strdup (pack->name_.c_str ());
      }
      break;
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_DESCR:
      {
        auto * descr = (PluginDescriptor *) wrapped_obj->obj;
        return g_strdup (descr->name_.c_str ());
      }
      break;
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_CHANNEL_SEND_TARGET:
      {
        auto * target = (ChannelSend::Target *) wrapped_obj->obj;
        return g_strdup (target->describe ().c_str ());
      }
      break;
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_EXT_PORT:
      {
        auto * port = (ExtPort *) wrapped_obj->obj;
        return g_strdup (port->get_friendly_name ().c_str ());
      }
      break;
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_ARRANGER_OBJECT:
      {
        auto * obj = (ArrangerObject *) wrapped_obj->obj;
        return g_strdup (obj->gen_human_friendly_name ().c_str ());
      }
      break;
    default:
      g_return_val_if_reached (nullptr);
      break;
    }

  g_return_val_if_reached (nullptr);
}

/**
 * If this function is not used, the internal object will
 * not be free'd.
 */
WrappedObjectWithChangeSignal *
wrapped_object_with_change_signal_new_with_free_func (
  void *            obj,
  WrappedObjectType type,
  ObjectFreeFunc    free_func)
{
  WrappedObjectWithChangeSignal * self =
    wrapped_object_with_change_signal_new (obj, type);
  self->free_func = free_func;

  return self;
}

WrappedObjectWithChangeSignal *
wrapped_object_with_change_signal_new (void * obj, WrappedObjectType type)
{
  WrappedObjectWithChangeSignal * self = Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
    g_object_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE, nullptr));

  self->type = type;
  self->obj = obj;

  if (G_IS_INITIALLY_UNOWNED (self))
    g_object_ref_sink (G_OBJECT (self));

  return self;
}

static void
dispose (WrappedObjectWithChangeSignal * self)
{
  object_free_w_func_and_null (g_object_unref, self->child_model);
}

static void
finalize (WrappedObjectWithChangeSignal * self)
{
  if (self->free_func && self->obj)
    {
      self->free_func (self->obj);
    }

  G_OBJECT_CLASS (wrapped_object_with_change_signal_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
wrapped_object_with_change_signal_class_init (
  WrappedObjectWithChangeSignalClass * klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (klass);

  obj_signals[SIGNAL_CHANGED] = g_signal_newv (
    "changed", G_TYPE_FROM_CLASS (oklass),
    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
    NULL /* closure */, NULL /* accumulator */, NULL /* accumulator data */,
    NULL /* C marshaller */, G_TYPE_NONE /* return_type */, 0 /* n_params */,
    NULL /* param_types */);

  oklass->dispose = (GObjectFinalizeFunc) dispose;
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
wrapped_object_with_change_signal_init (WrappedObjectWithChangeSignal * self)
{
}
