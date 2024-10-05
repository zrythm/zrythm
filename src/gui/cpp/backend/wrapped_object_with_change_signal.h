// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_H__
#define __GUI_BACKEND_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_H__

#include "common/dsp/track.h"
#include "common/utils/types.h"
#include "gui/cpp/backend/project_info.h"

#include <giomm/listmodel.h>
#include <glib-object.h>

class PluginDescriptor;
class ChordDescriptor;
class ChordPresetPack;
class ChordPreset;
class FileDescriptor;
class MidiMapping;
class Port;
struct ChannelSendTarget;
class PluginCollection;
class ExtPort;
struct FileBrowserLocation;
class ArrangerObject;
class Track;
class Plugin;

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

enum class WrappedObjectType
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
};

/**
 * A GObject-ified normal C object with a signal that interested
 * parties can listen to for changes.
 *
 * To be used in list view models and other APIs that require
 * using a GObject.
 */
using WrappedObjectWithChangeSignal = struct _WrappedObjectWithChangeSignal
{
  GObject parent_instance;

  using ObjVariant = merge_variants_t<
    TrackVariant,
    PluginVariant,
    ArrangerObjectVariant,
    PortVariant,
    std::variant<
      PluginDescriptor,
      ChordDescriptor,
      ChordPreset,
      ChordPresetPack,
      FileDescriptor,
      MidiMapping,
      ProjectInfo,
      ChannelSendTarget,
      PluginCollection,
      ExtPort,
      FileBrowserLocation>>;
  using ObjPtrVariant =
    merge_variants_t<std::variant<std::nullptr_t>, to_pointer_variant<ObjVariant>>;

  WrappedObjectType type;
  ObjPtrVariant     obj = nullptr;

  /** Parent model, if using tree model. */
  GListModel * parent_model;

  /** Model containing the children for this object (if using
   * tree model). */
  GListModel * child_model;

  ObjectFreeFunc free_func;
};

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
wrapped_object_with_change_signal_new (
  WrappedObjectWithChangeSignal::ObjPtrVariant obj,
  WrappedObjectType                            type);

/**
 * If this function is not used, the internal object will not be free'd.
 */
WrappedObjectWithChangeSignal *
wrapped_object_with_change_signal_new_with_free_func (
  WrappedObjectWithChangeSignal::ObjPtrVariant obj,
  WrappedObjectType                            type,
  ObjectFreeFunc                               free_func);

ArrangerObject *
wrapped_object_with_change_signal_get_arranger_object (
  WrappedObjectWithChangeSignal * self);

Track *
wrapped_object_with_change_signal_get_track (
  WrappedObjectWithChangeSignal * self);

Port *
wrapped_object_with_change_signal_get_port (
  WrappedObjectWithChangeSignal * self);

Plugin *
wrapped_object_with_change_signal_get_plugin (
  WrappedObjectWithChangeSignal * self);

/**
 * @}
 */

#endif
