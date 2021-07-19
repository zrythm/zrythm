/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include <time.h>
#include <sys/time.h>

#include "actions/tracklist_selections.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/master_track.h"
#include "audio/meter.h"
#include "audio/track.h"
#include "gui/widgets/balance_control.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/channel_sends_expander.h"
#include "gui/widgets/channel_slot.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/fader_buttons.h"
#include "gui/widgets/editable_label.h"
#include "gui/widgets/expander_box.h"
#include "gui/widgets/meter.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/fader.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/plugin_strip_expander.h"
#include "gui/widgets/route_target_selector.h"
#include "gui/widgets/track.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (ChannelWidget,
               channel_widget,
               GTK_TYPE_EVENT_BOX)

/**
 * Tick function.
 *
 * usually, the way to do that is via g_idle_add() or g_main_context_invoke()
 * the other option is to use polling and have the GTK thread check if the
 * value changed every monitor refresh
 * that would be done via gtk_widget_set_tick_functions()
 * gtk_widget_set_tick_function()
 */
gboolean
channel_widget_update_meter_reading (
  ChannelWidget * self,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  double prev = self->meter_reading_val;
  Channel * channel = self->channel;

  if (!gtk_widget_get_mapped (
        GTK_WIDGET (self)))
    {
      return G_SOURCE_CONTINUE;
    }

  /* TODO */
  Track * track =
    channel_get_track (channel);
  if (track->out_signal_type == TYPE_EVENT)
    {
      gtk_label_set_text (
        self->meter_reading, "-∞");
      return G_SOURCE_CONTINUE;
    }

  float amp =
    MAX (
      self->meter_l->meter->prev_max,
      self->meter_r->meter->prev_max);
  double val = (double) math_amp_to_dbfs (amp);
  if (math_doubles_equal (val, prev))
    return G_SOURCE_CONTINUE;
  if (val < -100.)
    gtk_label_set_text (self->meter_reading, "-∞");
  else
    {
      char string[40];
      if (val < -10.)
        {
          sprintf (string, "%.0f", val);
        }
      else
        {
          sprintf (string, "%.1f", val);
        }
      char formatted_str[80];
      if (val > 0)
        {
          sprintf (
            formatted_str,
            "<span foreground=\"#FF0A05\">%s</span>",
            string);
        }
      else
        {
          strcpy (formatted_str, string);
        }
      gtk_label_set_markup (
        self->meter_reading, formatted_str);
    }

  self->meter_reading_val = val;

  return G_SOURCE_CONTINUE;
}

static void
on_drag_data_received (
  GtkWidget        *widget,
  GdkDragContext   *context,
  gint              x,
  gint              y,
  GtkSelectionData *data,
  guint             info,
  guint             time,
  ChannelWidget * self)
{
  g_debug ("channel widget: drag data received");
  Track * this =
    channel_get_track (self->channel);

  /* determine if moving or copying */
  GdkDragAction action =
    gdk_drag_context_get_selected_action (
      context);

  int w =
    gtk_widget_get_allocated_width (widget);

  TrackWidgetHighlight location;
  if (track_type_is_foldable (this->type) &&
      x < w - 12 && x > 12)
    {
      location = TRACK_WIDGET_HIGHLIGHT_INSIDE;
    }
  else if (x < w / 2)
    {
      location = TRACK_WIDGET_HIGHLIGHT_TOP;
    }
  else
    {
      location = TRACK_WIDGET_HIGHLIGHT_BOTTOM;
    }
  tracklist_handle_move_or_copy (
    TRACKLIST, this, location, action);
}

static void
on_drag_data_get (
  GtkWidget        *widget,
  GdkDragContext   *context,
  GtkSelectionData *data,
  guint             info,
  guint             time,
  ChannelWidget * self)
{
  g_debug ("channel widget: drag data get");

  /* Not really needed since the selections are
   * used. just send master */
  gtk_selection_data_set (
    data,
    gdk_atom_intern_static_string (
      TARGET_ENTRY_TRACK),
    32,
    (const guchar *) &P_MASTER_TRACK,
    sizeof (P_MASTER_TRACK));
}

/**
 * For drag n drop.
 */
static void
on_dnd_drag_begin (
  GtkWidget      *widget,
  GdkDragContext *context,
  ChannelWidget * self)
{
  Track * track =
    channel_get_track (self->channel);
  self->selected_in_dnd = 1;
  MW_MIXER->start_drag_track = track;

  if (self->n_press == 1)
    {
      int ctrl = 0, selected = 0;

      ctrl = self->ctrl_held_at_start;

      if (tracklist_selections_contains_track (
            TRACKLIST_SELECTIONS,
            track))
        selected = 1;

      /* no control & not selected */
      if (!ctrl && !selected)
        {
          tracklist_selections_select_single (
            TRACKLIST_SELECTIONS,
            track, F_PUBLISH_EVENTS);
        }
      else if (!ctrl && selected)
        { }
      else if (ctrl && !selected)
        tracklist_selections_add_track (
          TRACKLIST_SELECTIONS, track, 1);
    }
}

/**
 * Highlights/unhighlights the channels
 * appropriately.
 */
static void
do_highlight (
  ChannelWidget * self,
  gint x,
  gint y)
{
  /* if we are closer to the start of selection than
   * the end */
  int w =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (self));
  if (x < w / 2)
    {
      /* highlight left */
      gtk_drag_highlight (
        GTK_WIDGET (
          self->highlight_left_box));
      gtk_widget_set_size_request (
        GTK_WIDGET (
          self->highlight_left_box),
        2, -1);

      /* unhilight right */
      gtk_drag_unhighlight (
        GTK_WIDGET (
          self->highlight_right_box));
      gtk_widget_set_size_request (
        GTK_WIDGET (
          self->highlight_right_box),
        -1, -1);
    }
  else
    {
      /* highlight right */
      gtk_drag_highlight (
        GTK_WIDGET (
          self->highlight_right_box));
      gtk_widget_set_size_request (
        GTK_WIDGET (
          self->highlight_right_box),
        2, -1);

      /* unhilight left */
      gtk_drag_unhighlight (
        GTK_WIDGET (
          self->highlight_left_box));
      gtk_widget_set_size_request (
        GTK_WIDGET (
          self->highlight_left_box),
        -1, -1);
    }
}

static void
on_drag_motion (
  GtkWidget *widget,
  GdkDragContext *context,
  gint x,
  gint y,
  guint time,
  ChannelWidget * self)
{
  GdkModifierType mask;
  z_gtk_widget_get_mask (
    widget, &mask);
  if (mask & GDK_CONTROL_MASK)
    gdk_drag_status (context, GDK_ACTION_COPY, time);
  else
    gdk_drag_status (context, GDK_ACTION_MOVE, time);

  do_highlight (self, x, y);
}

static void
on_drag_leave (
  GtkWidget *      widget,
  GdkDragContext * context,
  guint            time,
  ChannelWidget *  self)
{
  g_message ("on_drag_leave");

  /*do_highlight (self);*/
  gtk_drag_unhighlight (
    GTK_WIDGET (self->highlight_left_box));
  gtk_widget_set_size_request (
    GTK_WIDGET (self->highlight_left_box),
    -1, -1);
  gtk_drag_unhighlight (
    GTK_WIDGET (self->highlight_right_box));
  gtk_widget_set_size_request (
    GTK_WIDGET (self->highlight_right_box),
    -1, -1);
}

/**
 * Callback when somewhere in the channel is
 * pressed.
 *
 * Only responsible for setting the tracklist
 * selection and should not do anything else.
 */
static void
on_whole_channel_press (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  ChannelWidget * self)
{
  self->n_press = n_press;

  GdkModifierType state_mask =
    ui_get_state_mask (
      GTK_GESTURE (gesture));
  self->ctrl_held_at_start =
    state_mask & GDK_CONTROL_MASK;
}

void
channel_widget_redraw_fader (
  ChannelWidget * self)
{
  g_return_if_fail (self->fader);
  gtk_widget_queue_draw (
    GTK_WIDGET (self->fader));
}

static void
on_drag_begin (
  GtkGestureDrag * gesture,
  gdouble          start_x,
  gdouble          start_y,
  ChannelWidget *  self)
{
  self->selected_in_dnd = 0;
  self->dragged = 0;
}

static void
on_drag_update (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  ChannelWidget *  self)
{
  self->dragged = 1;
}

static gboolean
on_btn_release (
  GtkWidget *      widget,
  GdkEventButton * event,
  ChannelWidget *  self)
{
  if (self->dragged || self->selected_in_dnd)
    return FALSE;

  Track * track =
    channel_get_track (self->channel);
  if (self->n_press == 1)
    {
      gint64 cur_time = g_get_monotonic_time ();
      if (cur_time - self->last_plugin_press >
            1000)
        {
          PROJECT->last_selection =
            SELECTION_TYPE_TRACKLIST;
        }

      bool ctrl = event->state & GDK_CONTROL_MASK;
      bool shift = event->state & GDK_SHIFT_MASK;
      tracklist_selections_handle_click (
        track, ctrl, shift, self->dragged);
    }

  return FALSE;
}

static void
refresh_color (ChannelWidget * self)
{
  Track * track =
    channel_get_track (self->channel);
  color_area_widget_set_color (
    self->color, &track->color);
}

#if 0
static void
setup_phase_panel (ChannelWidget * self)
{
  self->phase_knob =
    knob_widget_new_simple (
      channel_get_phase,
      channel_set_phase,
      self->channel,
      0, 180, 20, 0.0f);
  gtk_box_pack_end (self->phase_controls,
                       GTK_WIDGET (self->phase_knob),
                       0, 1, 0);
  char * str =
    g_strdup_printf (
      "%.1f",
      (double) self->channel->fader.phase);
  gtk_label_set_text (
    self->phase_reading, str);
  g_free (str);
}
#endif

static void
setup_meter (ChannelWidget * self)
{
  Track * track =
    channel_get_track (self->channel);
  switch (track->out_signal_type)
    {
    case TYPE_EVENT:
      meter_widget_setup (
        self->meter_l,
        self->channel->midi_out, 14);
      gtk_widget_set_margin_start (
        GTK_WIDGET (self->meter_l), 5);
      gtk_widget_set_margin_end (
        GTK_WIDGET (self->meter_l), 5);
      gtk_widget_set_visible (
        GTK_WIDGET (self->meter_r), 0);
      break;
    case TYPE_AUDIO:
      meter_widget_setup (
        self->meter_l,
        self->channel->stereo_out->l, 12);
      meter_widget_setup (
        self->meter_r,
        self->channel->stereo_out->r, 12);
      break;
    default:
      break;
    }
}

/**
 * Updates the inserts.
 */
void
channel_widget_update_inserts (ChannelWidget * self)
{
  plugin_strip_expander_widget_refresh (
    self->inserts);
}

static void
setup_channel_icon (ChannelWidget * self)
{
  Track * track =
    channel_get_track (self->channel);
  gtk_image_set_from_icon_name (
    self->icon, track->icon_name,
    GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->icon),
    track_is_enabled (track));
}

static void
refresh_output (ChannelWidget * self)
{
  route_target_selector_widget_refresh (
    self->output, self->channel);
}

static void
refresh_name (ChannelWidget * self)
{
  Track * track =
    channel_get_track (self->channel);
  g_warn_if_fail (track->name);
  if (track_is_enabled (track))
    {
      gtk_label_set_text (
        GTK_LABEL (self->name->label), track->name);
    }
  else
    {
      char * markup =
        g_strdup_printf (
          "<span foreground=\"grey\">%s</span>",
          track->name);
      gtk_label_set_markup (
        GTK_LABEL (self->name->label), markup);
    }
}

/**
 * Sets up the icon and name box for drag & move.
 */
/*static void*/
/*setup_drag_move (*/
  /*ChannelWidget * self)*/
/*{*/

/*}*/

static void
setup_balance_control (ChannelWidget * self)
{
  self->balance_control =
    balance_control_widget_new (
      channel_get_balance_control,
      channel_set_balance_control,
      self->channel, self->channel->fader->balance,
      12);
  gtk_box_pack_start (
    self->balance_control_box,
    GTK_WIDGET (self->balance_control),
    F_NO_EXPAND, F_FILL, 0);
}

static void
setup_aux_buttons (
  ChannelWidget * self)
{
}

void
channel_widget_refresh_buttons (
  ChannelWidget * self)
{
  fader_buttons_widget_refresh (
    self->fader_buttons,
    channel_get_track (self->channel));
}

static void
update_reveal_status (
  ChannelWidget * self)
{
  expander_box_widget_set_reveal (
    Z_EXPANDER_BOX_WIDGET (self->inserts),
    g_settings_get_boolean (
      S_UI_MIXER, "inserts-expanded"));
  expander_box_widget_set_reveal (
    Z_EXPANDER_BOX_WIDGET (self->sends),
    g_settings_get_boolean (
      S_UI_MIXER, "sends-expanded"));
}

/**
 * Updates everything on the widget.
 *
 * It is reduntant but keeps code organized. Should fix if it causes lags.
 */
void
channel_widget_refresh (ChannelWidget * self)
{
  refresh_name (self);
  refresh_output (self);
  channel_widget_update_meter_reading (
    self, NULL, NULL);
  channel_widget_refresh_buttons (self);
  refresh_color (self);
  update_reveal_status (self);
  setup_channel_icon (self);

  Track * track =
    channel_get_track (self->channel);
  if (track_is_selected (track))
    {
      /* set selected or not */
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_SELECTED, 0);
    }
  else
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_SELECTED);
    }
}

static void
show_context_menu (
  ChannelWidget * self)
{
  GtkWidget *menu;
  GtkMenuItem *menuitem;
  menu = gtk_menu_new();
  Track * track = self->channel->track;

#define APPEND(mi) \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL (menu), \
    GTK_WIDGET (menuitem));

#define ADD_SEPARATOR \
  menuitem = \
    GTK_MENU_ITEM ( \
      gtk_separator_menu_item_new ()); \
  gtk_widget_set_visible ( \
    GTK_WIDGET (menuitem), true); \
  APPEND (menuitem)

  int num_selected =
    TRACKLIST_SELECTIONS->num_tracks;

  if (num_selected > 0)
    {
      char * str;

      if (track->type != TRACK_TYPE_MASTER &&
          track->type != TRACK_TYPE_CHORD &&
          track->type != TRACK_TYPE_MARKER &&
          track->type != TRACK_TYPE_TEMPO)
        {
          /* delete track */
          if (num_selected == 1)
            str =
              g_strdup (_("_Delete Track"));
          else
            str =
              g_strdup (_("_Delete Tracks"));
          menuitem =
            z_gtk_create_menu_item (
              str, "edit-delete", F_NO_TOGGLE,
              "win.delete-selected-tracks");
          g_free (str);
          APPEND (menuitem);

          /* duplicate track */
          if (num_selected == 1)
            str =
              g_strdup (_("_Duplicate Track"));
          else
            str =
              g_strdup (_("_Duplicate Tracks"));
          menuitem =
            z_gtk_create_menu_item (
              str, "edit-copy", F_NO_TOGGLE,
              "win.duplicate-selected-tracks");
          g_free (str);
          APPEND (menuitem);
        }

      menuitem =
        z_gtk_create_menu_item (
          num_selected == 1 ?
            _("Hide Track") :
            _("Hide Tracks"),
          "view-hidden", F_NO_TOGGLE,
          "win.hide-selected-tracks");
      APPEND (menuitem);

      menuitem =
        z_gtk_create_menu_item (
          num_selected == 1 ?
            _("Pin/Unpin Track") :
            _("Pin/Unpin Tracks"),
          "window-pin", F_NO_TOGGLE,
          "win.pin-selected-tracks");
      APPEND (menuitem);
    }

  /* add solo/mute/listen */
  if (track_type_has_channel (track->type))
    {
      ADD_SEPARATOR;

      if (tracklist_selections_contains_soloed_track (
            TRACKLIST_SELECTIONS, F_NO_SOLO))
        {
          menuitem =
            z_gtk_create_menu_item (
              _("Solo"), "solo", F_NO_TOGGLE,
              "win.solo-selected-tracks");
          APPEND (menuitem);
        }
      if (tracklist_selections_contains_soloed_track (
            TRACKLIST_SELECTIONS, F_SOLO))
        {
          menuitem =
            z_gtk_create_menu_item (
              _("Unsolo"), "unsolo", F_NO_TOGGLE,
              "win.unsolo-selected-tracks");
          APPEND (menuitem);
        }

      if (tracklist_selections_contains_muted_track (
            TRACKLIST_SELECTIONS, F_NO_MUTE))
        {
          menuitem =
            z_gtk_create_menu_item (
              _("Mute"), "mute", F_NO_TOGGLE,
              "win.mute-selected-tracks");
          APPEND (menuitem);
        }
      if (tracklist_selections_contains_muted_track (
            TRACKLIST_SELECTIONS, F_MUTE))
        {
          menuitem =
            z_gtk_create_menu_item (
              _("Unmute"), "unmute", F_NO_TOGGLE,
              "win.unmute-selected-tracks");
          APPEND (menuitem);
        }

      if (tracklist_selections_contains_listened_track (
            TRACKLIST_SELECTIONS, F_NO_LISTEN))
        {
          menuitem =
            z_gtk_create_menu_item (
              _("Listen"), "listen",
              F_NO_TOGGLE,
              "win.listen-selected-tracks");
          APPEND (menuitem);
        }
      if (tracklist_selections_contains_listened_track (
            TRACKLIST_SELECTIONS, F_LISTEN))
        {
          menuitem =
            z_gtk_create_menu_item (
              _("Unlisten"), "unlisten",
              F_NO_TOGGLE,
              "win.unlisten-selected-tracks");
          APPEND (menuitem);
        }
    }

  /* add enable/disable */
  if (tracklist_selections_contains_enabled_track (
        TRACKLIST_SELECTIONS, F_ENABLED))
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Disable"), "offline",
          F_NO_TOGGLE,
          "win.disable-selected-tracks");
      APPEND (menuitem);
    }
  else
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Enable"), "online",
          F_NO_TOGGLE,
          "win.enable-selected-tracks");
      APPEND (menuitem);
    }

  ADD_SEPARATOR;
  menuitem =
    z_gtk_create_menu_item (
      _("Change color..."), "color-fill",
      F_NO_TOGGLE, "win.change-track-color");
  APPEND (menuitem);

#undef APPEND
#undef ADD_SEPARATOR

  gtk_menu_attach_to_widget (
    GTK_MENU (menu),
    GTK_WIDGET (self), NULL);
  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static void
on_right_click (
  GtkGestureMultiPress * gesture,
  gint                    n_press,
  gdouble                x,
  gdouble                y,
  ChannelWidget *        self)
{
  GdkModifierType state_mask =
    ui_get_state_mask (GTK_GESTURE (gesture));

  Track * track = self->channel->track;
  if (!track_is_selected (track))
    {
      if (state_mask & GDK_SHIFT_MASK ||
          state_mask & GDK_CONTROL_MASK)
        {
          track_select (
            track, F_SELECT, 0, 1);
        }
      else
        {
          track_select (
            track, F_SELECT, 1, 1);
        }
    }
  if (n_press == 1)
    {
      show_context_menu (self);
    }
}

static void
on_destroy (
  ChannelWidget * self)
{
  channel_widget_tear_down (self);
}

ChannelWidget *
channel_widget_new (Channel * channel)
{
  ChannelWidget * self =
    g_object_new (CHANNEL_WIDGET_TYPE, NULL);
  self->channel = channel;

  Track * track =
    channel_get_track (self->channel);

  plugin_strip_expander_widget_setup (
    self->inserts, PLUGIN_SLOT_INSERT,
    PSE_POSITION_CHANNEL, track);
  channel_sends_expander_widget_setup (
    self->sends, CSE_POSITION_CHANNEL, track);
  setup_aux_buttons (self);
  fader_widget_setup (
    self->fader, channel->fader, 38, -1);
  setup_meter (self);
  setup_balance_control (self);
  setup_channel_icon (self);
  editable_label_widget_setup (
    self->name, track,
    (GenericStringGetter) track_get_name,
    (GenericStringSetter)
    track_set_name_with_action);
  route_target_selector_widget_setup (
    self->output, self->channel);
  color_area_widget_setup_track (
    self->color, track);

#if 0
  /*if (self->channel->track->type ==*/
        /*TRACK_TYPE_INSTRUMENT)*/
    /*{*/
      self->instrument_slot =
        channel_slot_widget_new (
          -1, self->channel, PLUGIN_SLOT_INSTRUMENT,
          true);
      gtk_widget_set_visible (
        GTK_WIDGET (self->instrument_slot), true);
      gtk_widget_set_hexpand (
        GTK_WIDGET (self->instrument_slot), true);
      gtk_container_add (
        GTK_CONTAINER (self->instrument_box),
        GTK_WIDGET (self->instrument_slot));
    /*}*/
#endif

  channel_widget_refresh (self);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self),
    (GtkTickCallback)
      channel_widget_update_meter_reading,
    self, NULL);

  g_signal_connect (
    self, "destroy",
    G_CALLBACK (on_destroy), NULL);

  g_object_ref (self);

  self->setup = true;

  return self;
}

/**
 * Prepare for finalization.
 */
void
channel_widget_tear_down (
  ChannelWidget * self)
{
  g_debug ("tearing down %p...", self);

  if (self->setup)
    {
      g_object_unref (self);
      self->channel->widget = NULL;
      self->setup = false;
    }

  g_debug ("done");
}

static void
channel_widget_class_init (
  ChannelWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "channel.ui");
  gtk_widget_class_set_css_name (
    klass, "channel");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ChannelWidget, x)

  BIND_CHILD (color);
  BIND_CHILD (output);
  BIND_CHILD (grid);
  BIND_CHILD (icon_and_name_event_box);
  BIND_CHILD (name);
  BIND_CHILD (mid_box);
  BIND_CHILD (inserts);
  BIND_CHILD (sends);
  BIND_CHILD (instrument_box);
  BIND_CHILD (fader_buttons);
  BIND_CHILD (fader);
  BIND_CHILD (meter_area);
  BIND_CHILD (meter_l);
  BIND_CHILD (meter_r);
  BIND_CHILD (meter_reading);
  BIND_CHILD (icon);
  BIND_CHILD (balance_control_box);
  BIND_CHILD (highlight_left_box);
  BIND_CHILD (highlight_right_box);
  BIND_CHILD (aux_buttons_box);

#undef BIND_CHILD
}

static void
channel_widget_init (ChannelWidget * self)
{
  g_type_ensure (ROUTE_TARGET_SELECTOR_WIDGET_TYPE);
  g_type_ensure (FADER_WIDGET_TYPE);
  g_type_ensure (FADER_BUTTONS_WIDGET_TYPE);
  g_type_ensure (METER_WIDGET_TYPE);
  g_type_ensure (COLOR_AREA_WIDGET_TYPE);
  g_type_ensure (PLUGIN_STRIP_EXPANDER_WIDGET_TYPE);
  g_type_ensure (
    CHANNEL_SENDS_EXPANDER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->last_midi_trigger_time = 0;

  /* set font sizes */
  GtkStyleContext * context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self->name->label));
  gtk_style_context_add_class (
    context, "channel_label");
  gtk_label_set_max_width_chars (
    self->name->label, 10);
  gtk_label_set_max_width_chars (
    self->output->label, 9);

  char * entry_track = g_strdup (TARGET_ENTRY_TRACK);
  GtkTargetEntry entries[] = {
    {
      entry_track, GTK_TARGET_SAME_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_TRACK),
    },
  };

  /* set as drag source for track */
  gtk_drag_source_set (
    GTK_WIDGET (self->icon_and_name_event_box),
    GDK_BUTTON1_MASK,
    entries, G_N_ELEMENTS (entries),
    GDK_ACTION_MOVE | GDK_ACTION_COPY);

  /* set as drag dest for channel (the channel will
   * be moved based on which half it was dropped in,
   * left or right) */
  gtk_drag_dest_set (
    GTK_WIDGET (self),
    GTK_DEST_DEFAULT_MOTION |
      GTK_DEST_DEFAULT_DROP,
    entries, G_N_ELEMENTS (entries),
    GDK_ACTION_MOVE | GDK_ACTION_COPY);
  g_free (entry_track);

  self->mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  self->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (
        GTK_WIDGET (
          self)));

  gtk_widget_set_hexpand (
    GTK_WIDGET (self), 0);

  self->right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self->icon_and_name_event_box)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_mouse_mp),
    GDK_BUTTON_SECONDARY);

  g_signal_connect (
    G_OBJECT (self->mp), "pressed",
    G_CALLBACK (on_whole_channel_press), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin",
    G_CALLBACK (on_drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update",
    G_CALLBACK (on_drag_update), self);
  /*g_signal_connect (*/
    /*G_OBJECT (self->drag), "drag-end",*/
    /*G_CALLBACK (on_drag_end), self);*/
  g_signal_connect_after (
    GTK_WIDGET (self->icon_and_name_event_box),
    "drag-begin",
    G_CALLBACK(on_dnd_drag_begin), self);
  /*g_signal_connect (*/
    /*GTK_WIDGET (self->icon_and_name_event_box),*/
    /*"drag-end",*/
    /*G_CALLBACK(on_dnd_drag_end), self);*/
  g_signal_connect (
    GTK_WIDGET (self), "drag-data-received",
    G_CALLBACK(on_drag_data_received), self);
  g_signal_connect (
    GTK_WIDGET (self->icon_and_name_event_box),
    "drag-data-get",
    G_CALLBACK (on_drag_data_get), self);
  g_signal_connect (
    GTK_WIDGET (self), "drag-motion",
    G_CALLBACK (on_drag_motion), self);
  g_signal_connect (
    GTK_WIDGET (self), "drag-leave",
    G_CALLBACK (on_drag_leave), self);
  /*g_signal_connect (*/
    /*GTK_WIDGET (self), "drag-failed",*/
    /*G_CALLBACK (on_drag_failed), self);*/
  g_signal_connect (
    GTK_WIDGET (self), "button-release-event",
    G_CALLBACK (on_btn_release), self);
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
}
