// SPDX-FileCopyrightText: © 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/arranger_object_all.h"
#include "common/dsp/channel_send.h"
#include "common/dsp/ext_port.h"
#include "common/dsp/track_all.h"
#include "common/io/file_descriptor.h"
#include "common/plugins/collections.h"
#include "common/plugins/plugin_descriptor.h"
#include "common/utils/objects.h"
#include "gui/cpp/backend/file_manager.h"
#include "gui/cpp/backend/settings/chord_preset_pack.h"
#include "gui/cpp/backend/wrapped_object_with_change_signal.h"
#include "gui/cpp/gtk_widgets/greeter.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

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
  z_return_val_if_fail (Z_IS_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data), nullptr);
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);

  return std::visit (
    [&] (auto &&obj) -> char * {
      using ObjT = base_type<decltype (obj)>;
      if constexpr (
        std::is_same_v<ChordPresetPack, ObjT>
        || std::is_same_v<PluginDescriptor, ObjT>)
        {
          return g_strdup (obj->name_.c_str ());
        }
      else if constexpr (std::is_same_v<ChannelSendTarget, ObjT>)
        {
          return g_strdup (obj->describe ().c_str ());
        }
      else if constexpr (std::is_same_v<ExtPort, ObjT>)
        {
          return g_strdup (obj->get_friendly_name ().c_str ());
        }
      else if constexpr (std::derived_from<ObjT, ArrangerObject>)
        {
          return g_strdup (obj->gen_human_friendly_name ().c_str ());
        }
      else
        {
          z_return_val_if_reached (nullptr);
        }
    },
    wrapped_obj->obj);
}

ArrangerObject *
wrapped_object_with_change_signal_get_arranger_object (
  WrappedObjectWithChangeSignal * self)
{
  z_return_val_if_fail (
    self->type == WrappedObjectType::WRAPPED_OBJECT_TYPE_ARRANGER_OBJECT,
    nullptr);
  return get_ptr_variant_as_base_ptr<ArrangerObject> (self->obj);
}

Track *
wrapped_object_with_change_signal_get_track (
  WrappedObjectWithChangeSignal * self)
{
  z_return_val_if_fail (
    self->type == WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK, nullptr);
  return get_ptr_variant_as_base_ptr<Track> (self->obj);
}

Port *
wrapped_object_with_change_signal_get_port (WrappedObjectWithChangeSignal * self)
{
  z_return_val_if_fail (
    self->type == WrappedObjectType::WRAPPED_OBJECT_TYPE_PORT, nullptr);
  return get_ptr_variant_as_base_ptr<Port> (self->obj);
}

Plugin *
wrapped_object_with_change_signal_get_plugin (
  WrappedObjectWithChangeSignal * self)
{
  z_return_val_if_fail (
    self->type == WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN, nullptr);
  return get_ptr_variant_as_base_ptr<Plugin> (self->obj);
}

/**
 * If this function is not used, the internal object will
 * not be free'd.
 */
WrappedObjectWithChangeSignal *
wrapped_object_with_change_signal_new_with_free_func (
  WrappedObjectWithChangeSignal::ObjPtrVariant obj,
  WrappedObjectType                            type,
  ObjectFreeFunc                               free_func)
{
  WrappedObjectWithChangeSignal * self =
    wrapped_object_with_change_signal_new (obj, type);
  self->free_func = free_func;

  return self;
}

WrappedObjectWithChangeSignal *
wrapped_object_with_change_signal_new (
  WrappedObjectWithChangeSignal::ObjPtrVariant obj,
  WrappedObjectType                            type)
{
  z_return_val_if_fail (std::get_if<std::nullptr_t> (&obj) == nullptr, nullptr);

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
  if (self->free_func)
    {
      std::visit (
        [&] (auto &&obj) {
          if constexpr (
            !std::is_same_v<std::nullptr_t, base_type<decltype (obj)>>)
            {
              self->free_func (obj);
            }
        },
        self->obj);
    }

  std::destroy_at (&self->obj);

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
  std::construct_at (&self->obj);
}
