// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_CHANNEL_H__
#define __GUI_WIDGETS_CHANNEL_H__

#include "dsp/channel.h"
#include "gui/widgets/meter.h"

#include <gtk/gtk.h>

typedef struct _PluginStripExpanderWidget
  PluginStripExpanderWidget;

#define CHANNEL_WIDGET_TYPE (channel_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChannelWidget,
  channel_widget,
  Z,
  CHANNEL_WIDGET,
  GtkWidget)

typedef struct _ColorAreaWidget   ColorAreaWidget;
typedef struct _KnobWidget        KnobWidget;
typedef struct _FaderWidget       FaderWidget;
typedef struct Channel            Channel;
typedef struct _ChannelSlotWidget ChannelSlotWidget;
typedef struct _RouteTargetSelectorWidget
  RouteTargetSelectorWidget;
typedef struct _BalanceControlWidget BalanceControlWidget;
typedef struct _EditableLabelWidget  EditableLabelWidget;
typedef struct _FaderButtonsWidget   FaderButtonsWidget;
typedef struct _ChannelSendsExpanderWidget
  ChannelSendsExpanderWidget;

typedef struct _ChannelWidget
{
  GtkWidget                   parent_instance;
  GtkBox *                    color_box;
  GtkGrid *                   grid;
  RouteTargetSelectorWidget * output;
  ColorAreaWidget *           color;
  GtkBox *                    icon_and_name_event_box;
  EditableLabelWidget *       name;
  GtkBox *                    phase_controls;
  GtkButton *                 phase_invert;
  GtkLabel *                  phase_reading;
  KnobWidget *                phase_knob;

  /** Instrument slot. */
  GtkBox *            instrument_box;
  ChannelSlotWidget * instrument_slot;

  /* ----- mid box ------ */

  GtkBox *                    mid_box;
  PluginStripExpanderWidget * inserts;
  PluginStripExpanderWidget * midi_fx;

  /** Sends. */
  ChannelSendsExpanderWidget * sends;

  /* -------- end mid box --------- */

  FaderButtonsWidget * fader_buttons;

  /** Meter area including reading. */
  GtkBox *               meter_area;
  GtkBox *               balance_control_box;
  BalanceControlWidget * balance_control;
  FaderWidget *          fader;
  MeterWidget *          meter_l;
  MeterWidget *          meter_r;
  GtkLabel *             meter_reading;
  GtkImage *             icon;

  /** Cache. */
  double meter_reading_val;

  /** Used for highlighting. */
  GtkBox * highlight_left_box;
  GtkBox * highlight_right_box;

  /** Box for auxiliary buttons near the top of the
   * widget. */
  GtkBox * aux_buttons_box;

  /** Mono compatibility button. */
  GtkToggleButton * mono_compat_btn;

  /** Number of clicks, used when selecting/moving/
   * dragging channels. */
  int n_press;

  /** Control held down on drag begin. */
  int ctrl_held_at_start;

  /** If drag update was called at least once. */
  int dragged;

  /** The track selection processing was done in
   * the dnd callbacks, so no need to do it in
   * drag_end. */
  int selected_in_dnd;

  /** Pointer to owner Channel. */
  Channel * channel;

  /**
   * Last time a plugin was pressed.
   *
   * This is to detect when a channel was selected
   * without clicking a plugin.
   */
  gint64 last_plugin_press;

  /** Last MIDI event trigger time, for MIDI
   * output. */
  gint64 last_midi_trigger_time;

  /** Whole channel press. */
  GtkGestureClick * mp;

  GtkGestureClick * right_mouse_mp;

  /** Drag on the icon and name event box. */
  GtkGestureDrag * drag;

  bool setup;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
} ChannelWidget;

/**
 * Updates the inserts.
 */
void
channel_widget_update_midi_fx_and_inserts (
  ChannelWidget * self);

void
channel_widget_redraw_fader (ChannelWidget * self);

/**
 * Creates a channel widget using the given channel
 * data.
 */
ChannelWidget *
channel_widget_new (Channel * channel);

void
channel_widget_tear_down (ChannelWidget * self);

/**
 * Updates everything on the widget.
 *
 * It is redundant but keeps code organized. Should
 * fix if it causes lags.
 */
void
channel_widget_refresh (ChannelWidget * self);

void
channel_widget_refresh_buttons (ChannelWidget * self);

/**
 * Displays the widget.
 */
void
channel_widget_show (ChannelWidget * self);

#endif
