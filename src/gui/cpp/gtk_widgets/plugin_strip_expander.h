// SPDX-FileCopyrightText: © 2020, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Plugin strip expander widget.
 */

#ifndef __GUI_WIDGETS_PLUGIN_STRIP_EXPANDER_H__
#define __GUI_WIDGETS_PLUGIN_STRIP_EXPANDER_H__

#include "gui/cpp/gtk_widgets/expander_box.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include "common/plugins/plugin_identifier.h"
#include "common/utils/audio.h"
#include "common/utils/types.h"

#define PLUGIN_STRIP_EXPANDER_WIDGET_TYPE \
  (plugin_strip_expander_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PluginStripExpanderWidget,
  plugin_strip_expander_widget,
  Z,
  PLUGIN_STRIP_EXPANDER_WIDGET,
  ExpanderBoxWidget);

class ChannelTrack;
TYPEDEF_STRUCT_UNDERSCORED (ChannelSlotWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

enum class PluginStripExpanderPosition
{
  PSE_POSITION_CHANNEL,
  PSE_POSITION_INSPECTOR,
};

/**
 * A TwoColExpanderBoxWidget for showing the ports
 * in the InspectorWidget.
 */
using PluginStripExpanderWidget = struct _PluginStripExpanderWidget
{
  ExpanderBoxWidget parent_instance;

  PluginSlotType              slot_type;
  PluginStripExpanderPosition position;

  /** Scrolled window for the vbox inside. */
  GtkScrolledWindow * scroll;
  GtkViewport *       viewport;

  /** VBox containing each slot. */
  GtkBox * box;

  /** 1 box for each item. */
  GtkBox * strip_boxes[STRIP_SIZE];

  /** Channel slots, if type is inserts. */
  ChannelSlotWidget * slots[STRIP_SIZE];

  /** Owner track. */
  ChannelTrack * track;
};

/**
 * Queues a redraw of the given slot.
 */
void
plugin_strip_expander_widget_redraw_slot (
  PluginStripExpanderWidget * self,
  int                         slot);

/**
 * Refreshes each field.
 */
void
plugin_strip_expander_widget_refresh (PluginStripExpanderWidget * self);

/**
 * Sets up the PluginStripExpanderWidget.
 */
void
plugin_strip_expander_widget_setup (
  PluginStripExpanderWidget * self,
  PluginSlotType              type,
  PluginStripExpanderPosition position,
  ChannelTrack *              track);

/**
 * @}
 */

#endif
