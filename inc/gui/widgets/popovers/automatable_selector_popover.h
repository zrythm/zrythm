// SPDX-FileCopyrightText: © 2019, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_AUTOMATABLE_SELECTOR_POPOVER_H__
#define __GUI_WIDGETS_AUTOMATABLE_SELECTOR_POPOVER_H__

#include "utils/types.h"

#include <gtk/gtk.h>

#define AUTOMATABLE_SELECTOR_POPOVER_WIDGET_TYPE \
  (automatable_selector_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AutomatableSelectorPopoverWidget,
  automatable_selector_popover_widget,
  Z,
  AUTOMATABLE_SELECTOR_POPOVER_WIDGET,
  GtkPopover);

TYPEDEF_STRUCT (ItemFactory);

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Automatable type.
 *
 * These are shown on the left side of the popover.
 */
enum class AutomatableSelectorType
{
  /** Midi automatable (modwheel etc.). */
  AS_TYPE_MIDI_CH1,
  AS_TYPE_MIDI_CH2,
  AS_TYPE_MIDI_CH3,
  AS_TYPE_MIDI_CH4,
  AS_TYPE_MIDI_CH5,
  AS_TYPE_MIDI_CH6,
  AS_TYPE_MIDI_CH7,
  AS_TYPE_MIDI_CH8,
  AS_TYPE_MIDI_CH9,
  AS_TYPE_MIDI_CH10,
  AS_TYPE_MIDI_CH11,
  AS_TYPE_MIDI_CH12,
  AS_TYPE_MIDI_CH13,
  AS_TYPE_MIDI_CH14,
  AS_TYPE_MIDI_CH15,
  AS_TYPE_MIDI_CH16,

  /** Channel. */
  AS_TYPE_CHANNEL,

  /** Plugin at Track MIDI fx slot. */
  AS_TYPE_MIDI_FX,

  /** Instrument plugin. */
  AS_TYPE_INSTRUMENT,

  /** Insert plugin. */
  AS_TYPE_INSERT,

  /** Modulator plugin. */
  AS_TYPE_MODULATOR,

  /** Tempo track ports. */
  AS_TYPE_TEMPO,

  /** Modulator macros. */
  AS_TYPE_MACRO,
};

/**
 * A popover for selecting the automation track
 * to automate.
 */
typedef struct _AutomatableSelectorPopoverWidget
{
  GtkPopover parent_instance;

  /** The owner button. */
  AutomationTrack * owner;

  GtkListView * type_listview;
  GtkListView * port_listview;

  ItemFactory * port_factory;

  GtkLabel * info;

  GtkSearchEntry * port_search_entry;

  /**
   * The selected Port will be stored here and passed to the button when
   * closing so that it can hide the current AutomationTrack and create/show
   * the one corresponding to this Automatable.
   */
  Port * selected_port;
} AutomatableSelectorPopoverWidget;

/**
 * Creates the popover.
 */
AutomatableSelectorPopoverWidget *
automatable_selector_popover_widget_new (AutomationTrack * owner);

/**
 * @}
 */

#endif
