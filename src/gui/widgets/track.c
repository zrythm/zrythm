/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/bus_track.h"
#include "audio/group_track.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/region.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/audio_track.h"
#include "gui/widgets/automation_lane.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bus_track.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/chord_track.h"
#include "gui/widgets/group_track.h"
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker_track.h"
#include "gui/widgets/master_track.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/track.h"
#include "gui/widgets/track_top_grid.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE_WITH_PRIVATE (TrackWidget,
                            track_widget,
                            GTK_TYPE_GRID)

void
track_widget_select (
  TrackWidget * self,
  int           select) ///< select or not
{
  TRACK_WIDGET_GET_PRIVATE (self);
  Track * track = tw_prv->track;

  if (select)
    {
      /*gtk_widget_set_state_flags (*/
        /*GTK_WIDGET (self),*/
        /*GTK_STATE_FLAG_SELECTED,*/
        /*0);*/
      tracklist_selections_add_track (
        TRACKLIST_SELECTIONS,
        track);
    }
  else
    {
      /*gtk_widget_unset_state_flags (*/
        /*GTK_WIDGET (self),*/
        /*GTK_STATE_FLAG_SELECTED);*/
      tracklist_selections_remove_track (
        TRACKLIST_SELECTIONS,
        track);
    }

  /* auto-set recording mode */
  Channel * chan;
  switch (track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_BUS:
    case TRACK_TYPE_GROUP:
      chan = track->channel;
      g_message (
        "%sselecting track %s, recording %d sa %d",
        select ? "" : "de",
        track->name,
        track->recording,
        chan->record_set_automatically);
      /* if selecting the track and recording is not already
       * on, turn these on */
      if (select && !track->recording)
        {
          track_set_recording (track, 1);
          chan->record_set_automatically = 1;
        }
      /* if deselecting and record mode was automatically
       * set when the track was selected, turn these off */
      else if (!select && chan->record_set_automatically)
        {
          track_set_recording (track, 0);
          chan->record_set_automatically = 0;
        }
      track_widget_refresh (self);
      break;
    case TRACK_TYPE_CHORD:
      break;
    default:
      break;
    }

  EVENTS_PUSH (ET_TRACK_CHANGED,
               track);
}

static gboolean
on_motion (GtkWidget * widget,
           GdkEventMotion *event,
           gpointer        user_data)
{
  TrackWidget * self = Z_TRACK_WIDGET (user_data);

  if (event->type == GDK_ENTER_NOTIFY ||
      (event->type == GDK_MOTION_NOTIFY &&
       !bot_bar_status_contains (_("Record")) &&
       !bot_bar_status_contains (_("Solo")) &&
       !bot_bar_status_contains (_("Mute")) &&
       !bot_bar_status_contains (_("Freeze")) &&
       !bot_bar_status_contains (_("Lock")) &&
       !bot_bar_status_contains (_("Show UI")) &&
       !bot_bar_status_contains (_("Show Automation Lanes")) &&
       !bot_bar_status_contains (_("Freeze"))))
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT, 0);
      bot_bar_change_status (
        _("Track - Change track parameters like \
Solo/Mute... - Click the icon on the bottom right \
to bring up the automation lanes - Double click to \
show corresponding channel in Mixer (if applicable)"));
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);
      bot_bar_change_status ("");
    }

  return FALSE;
}

/**
 * Wrapper.
 */
void
track_widget_refresh (TrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);

#define REFRESH_TW(caps,sc) \
  case TRACK_TYPE_##caps: \
    sc##_track_widget_refresh ( \
      Z_##caps##_TRACK_WIDGET (self)); \
    break

  switch (tw_prv->track->type)
    {
      REFRESH_TW (INSTRUMENT, instrument);
      REFRESH_TW (MASTER, master);
      REFRESH_TW (AUDIO, audio);
      REFRESH_TW (CHORD, chord);
      REFRESH_TW (BUS, bus);
      REFRESH_TW (GROUP, group);
      REFRESH_TW (MARKER, marker);
    default:
      break;
    }
#undef REFRESH_TW
}

/**
 * Wrapper to refresh buttons only.
 */
void
track_widget_refresh_buttons (
  TrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);

#define REFRESH_TW_BUTTONS(caps,sc) \
  case TRACK_TYPE_##caps: \
    sc##_track_widget_refresh_buttons ( \
      Z_##caps##_TRACK_WIDGET (self)); \
    break

  switch (tw_prv->track->type)
    {
      REFRESH_TW_BUTTONS (INSTRUMENT, instrument);
      REFRESH_TW_BUTTONS (MASTER, master);
      REFRESH_TW_BUTTONS (AUDIO, audio);
      REFRESH_TW_BUTTONS (CHORD, chord);
      REFRESH_TW_BUTTONS (BUS, bus);
      REFRESH_TW_BUTTONS (GROUP, group);
      REFRESH_TW_BUTTONS (MARKER, marker);
    default:
      break;
    }
#undef REFRESH_TW_BUTTONS
}

/**
 * Returns if cursor is in top half of the track.
 *
 * Used by timeline to determine if it will select
 * objects or range.
 */
int
track_widget_is_cursor_in_top_half (
  TrackWidget * self,
  double        y)
{
  TRACK_WIDGET_GET_PRIVATE (self);
  GtkWidget * top_grid =
    GTK_WIDGET (tw_prv->top_grid);

  /* determine selection type based on click
   * position */
  GtkAllocation allocation;
  gtk_widget_get_allocation (
    GTK_WIDGET (top_grid),
    &allocation);

  gint wx, wy;
  gtk_widget_translate_coordinates (
    GTK_WIDGET (MW_TIMELINE),
    GTK_WIDGET (top_grid),
    0,
    y,
    &wx,
    &wy);

  /* if bot half */
  if (wy >= allocation.height / 2 &&
      wy <= allocation.height)
    {
      return 0;
    }
  else /* if top half */
    {
      return 1;
    }
}

/**
 * Blocks all signal handlers.
 */
void
track_widget_block_all_signal_handlers (
  TrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);
  if (Z_IS_INSTRUMENT_TRACK_WIDGET (self))
    {
      InstrumentTrackWidget * itw =
        Z_INSTRUMENT_TRACK_WIDGET (self);
      g_signal_handler_block (
        itw->solo,
        tw_prv->solo_toggled_handler_id);
      g_signal_handler_block (
        itw->mute,
        tw_prv->mute_toggled_handler_id);
    }
  else if (Z_IS_AUDIO_TRACK_WIDGET (self))
    {
      AudioTrackWidget * atw =
        Z_AUDIO_TRACK_WIDGET (self);
      g_signal_handler_block (
        atw->solo,
        tw_prv->solo_toggled_handler_id);
      g_signal_handler_block (
        atw->mute,
        tw_prv->mute_toggled_handler_id);
    }
  else if (Z_IS_MASTER_TRACK_WIDGET (self))
    {
      MasterTrackWidget * mtw =
        Z_MASTER_TRACK_WIDGET (self);
      g_signal_handler_block (
        mtw->solo,
        tw_prv->solo_toggled_handler_id);
      g_signal_handler_block (
        mtw->mute,
        tw_prv->mute_toggled_handler_id);
    }
  else if (Z_IS_CHORD_TRACK_WIDGET (self))
    {
      ChordTrackWidget * mtw =
        Z_CHORD_TRACK_WIDGET (self);
      g_signal_handler_block (
        mtw->solo,
        tw_prv->solo_toggled_handler_id);
      g_signal_handler_block (
        mtw->mute,
        tw_prv->mute_toggled_handler_id);
    }
  else if (Z_IS_BUS_TRACK_WIDGET (self))
    {
      BusTrackWidget * mtw =
        Z_BUS_TRACK_WIDGET (self);
      g_signal_handler_block (
        mtw->solo,
        tw_prv->solo_toggled_handler_id);
      g_signal_handler_block (
        mtw->mute,
        tw_prv->mute_toggled_handler_id);
    }
}

/**
 * Unblocks all signal handlers.
 */
void
track_widget_unblock_all_signal_handlers (
  TrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);
  if (Z_IS_INSTRUMENT_TRACK_WIDGET (self))
    {
      InstrumentTrackWidget * itw =
        Z_INSTRUMENT_TRACK_WIDGET (self);
      g_signal_handler_unblock (
        itw->solo,
        tw_prv->solo_toggled_handler_id);
      g_signal_handler_unblock (
        itw->mute,
        tw_prv->mute_toggled_handler_id);
    }
  else if (Z_IS_AUDIO_TRACK_WIDGET (self))
    {
      AudioTrackWidget * atw =
        Z_AUDIO_TRACK_WIDGET (self);
      g_signal_handler_unblock (
        atw->solo,
        tw_prv->solo_toggled_handler_id);
      g_signal_handler_unblock (
        atw->mute,
        tw_prv->mute_toggled_handler_id);
    }
  else if (Z_IS_MASTER_TRACK_WIDGET (self))
    {
      MasterTrackWidget * mtw =
        Z_MASTER_TRACK_WIDGET (self);
      g_signal_handler_unblock (
        mtw->solo,
        tw_prv->solo_toggled_handler_id);
      g_signal_handler_unblock (
        mtw->mute,
        tw_prv->mute_toggled_handler_id);
    }
  else if (Z_IS_CHORD_TRACK_WIDGET (self))
    {
      ChordTrackWidget * mtw =
        Z_CHORD_TRACK_WIDGET (self);
      g_signal_handler_unblock (
        mtw->solo,
        tw_prv->solo_toggled_handler_id);
      g_signal_handler_unblock (
        mtw->mute,
        tw_prv->mute_toggled_handler_id);
    }
  else if (Z_IS_BUS_TRACK_WIDGET (self))
    {
      BusTrackWidget * mtw =
        Z_BUS_TRACK_WIDGET (self);
      g_signal_handler_unblock (
        mtw->solo,
        tw_prv->solo_toggled_handler_id);
      g_signal_handler_unblock (
        mtw->mute,
        tw_prv->mute_toggled_handler_id);
    }
}

TrackWidgetPrivate *
track_widget_get_private (TrackWidget * self)
{
  return track_widget_get_instance_private (self);
}

static void
show_context_menu (TrackWidget * self)
{
  GtkWidget *menu;
  GtkMenuItem *menuitem;
  menu = gtk_menu_new();
  TRACK_WIDGET_GET_PRIVATE (self);
  Track * track = tw_prv->track;

#define APPEND(mi) \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL (menu), \
    GTK_WIDGET (menuitem));

  int num_selected =
    TRACKLIST_SELECTIONS->num_tracks;

  if (num_selected > 0)
    {
      char * str;

      if (track->type != TRACK_TYPE_MASTER)
        {
          /* delete track */
          if (num_selected == 1)
            str =
              g_strdup_printf (_("_Delete Track"));
          else
            str =
              g_strdup_printf (_("_Delete %d Tracks"),
                               num_selected);
          menuitem =
            z_gtk_create_menu_item (
              str,
              "z-delete",
              0,
              NULL,
              0,
              "win.delete-selected-tracks");
          g_free (str);
          APPEND (menuitem);

          /* duplicate track */
          if (num_selected == 1)
            str =
              g_strdup_printf (_("_Duplicate Track"));
          else
            str =
              g_strdup_printf (
                _("_Duplicate %d Tracks"),
                num_selected);
          menuitem =
            z_gtk_create_menu_item (
              str,
              "z-edit-duplicate",
              0,
              NULL,
              0,
              "win.duplicate-selected-tracks");
          g_free (str);
          APPEND (menuitem);
        }

      /* add regions */
      if (track->type == TRACK_TYPE_INSTRUMENT)
        {
          menuitem =
            z_gtk_create_menu_item (
              _("Add Region"),
              "z-gtk-add",
              0,
              NULL,
              0,
              "win.duplicate-selected-tracks");
          APPEND (menuitem);
        }
    }

#undef APPEND

  gtk_menu_attach_to_widget (
    GTK_MENU (menu),
    GTK_WIDGET (self), NULL);
  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static void
on_right_click (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  TrackWidget * tw = Z_TRACK_WIDGET (user_data);
  TRACK_WIDGET_GET_PRIVATE (tw);

  UI_GET_STATE_MASK (gesture);

  Track * track = tw_prv->track;
  if (!track_is_selected (track))
    {
      if (state_mask & GDK_SHIFT_MASK ||
          state_mask & GDK_CONTROL_MASK)
        {
          tracklist_widget_select_track (
            MW_TRACKLIST,
            track,
            F_SELECT, F_APPEND);
        }
      else
        {
          tracklist_widget_select_track (
            MW_TRACKLIST,
            track,
            F_SELECT, F_NO_APPEND);
        }
    }
  if (n_press == 1)
    {
      show_context_menu (tw);
    }
}

static void
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  TrackWidget * self =
    Z_TRACK_WIDGET (user_data);

  /* FIXME should do this via focus on click property */
  /*if (!gtk_widget_has_focus (GTK_WIDGET (self)))*/
    /*gtk_widget_grab_focus (GTK_WIDGET (self));*/

  UI_GET_STATE_MASK (gesture);

  TRACK_WIDGET_GET_PRIVATE (self);
  Track * track = tw_prv->track;

  PROJECT->last_selection =
    SELECTION_TYPE_TRACK;

  tracklist_widget_select_track (
    MW_TRACKLIST, track,
    track_is_selected (track) &&
    state_mask & GDK_CONTROL_MASK ?
      F_NO_SELECT: F_SELECT,
    (state_mask & GDK_SHIFT_MASK ||
      state_mask & GDK_CONTROL_MASK) ?
      F_APPEND : F_NO_APPEND);
}

/**
 * Wrapper for child track widget.
 *
 * Sets color, draw callback, etc.
 */
TrackWidget *
track_widget_new (Track * track)
{
  g_return_val_if_fail (track, NULL);

  TrackWidget * self = NULL;

#define NEW_TW(caps,sc) \
  case TRACK_TYPE_##caps: \
    self = Z_TRACK_WIDGET ( \
      sc##_track_widget_new (track)); \
    break

  switch (track->type)
    {
    NEW_TW (CHORD, chord);
    NEW_TW (BUS, bus);
    NEW_TW (GROUP, group);
    NEW_TW (INSTRUMENT, instrument);
    NEW_TW (MASTER, master);
    NEW_TW (AUDIO, audio);
    NEW_TW (MARKER, marker);
    default:
      break;
    }

#undef NEW_TW

  g_warn_if_fail (Z_IS_TRACK_WIDGET (self));

  TRACK_WIDGET_GET_PRIVATE (self);
  color_area_widget_setup_track (
    tw_prv->color, track);

  tw_prv->track = track;

  return self;
}

/**
 * Callback when lanes button is toggled.
 */
void
track_widget_on_show_lanes_toggled (
  GtkWidget * widget,
  TrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);
  Track * track = tw_prv->track;

  /* set visibility flag */
  track->lanes_visible =
    gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (widget));

  EVENTS_PUSH (ET_TRACK_LANES_VISIBILITY_CHANGED,
               track);
}

void
track_widget_on_show_automation_toggled (
  GtkWidget *   widget,
  TrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);
  Track * track = tw_prv->track;

  /* set visibility flag */
  track->bot_paned_visible =
    gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (widget));

  EVENTS_PUSH (ET_TRACK_BOT_PANED_VISIBILITY_CHANGED,
               track);
}

void
track_widget_on_solo_toggled (
  GtkToggleButton * btn,
  void *            data)
{
  TRACK_WIDGET_GET_PRIVATE (data);
  track_set_soloed (
    tw_prv->track,
    gtk_toggle_button_get_active (btn),
    1);
}

/**
 * General handler for tracks that have mute
 * buttons.
 */
void
track_widget_on_mute_toggled (
  GtkToggleButton * btn,
  void *            data)
{
  TRACK_WIDGET_GET_PRIVATE (data);
  track_set_muted (
    tw_prv->track,
    gtk_toggle_button_get_active (btn),
    1);
}

void
track_widget_on_record_toggled (
  GtkWidget * widget,
  void *      data)
{
  TrackWidget * self =
    Z_TRACK_WIDGET (data);
  TRACK_WIDGET_GET_PRIVATE (self);
  Track * track = tw_prv->track;
  ChannelTrack * ct = (ChannelTrack *) track;
  Channel * chan = ct->channel;

  /* toggle record flag */
  track_set_recording (track, !track->recording);
  chan->record_set_automatically = 0;
  g_message ("recording %d, %s",
             track->recording,
             track->name);

  EVENTS_PUSH (ET_TRACK_STATE_CHANGED,
               track);
}

GtkWidget *
track_widget_get_bottom_paned (TrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);

  return dzl_multi_paned_get_nth_child (
    DZL_MULTI_PANED (tw_prv->paned), 1);
}

static void
on_destroy (
  TrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);

  Track * track = tw_prv->track;

  g_object_unref (self);

  track->widget = NULL;
}

static void
track_widget_init (TrackWidget * self)
{
  g_type_ensure (COLOR_AREA_WIDGET_TYPE);
  g_type_ensure (TRACK_TOP_GRID_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  TRACK_WIDGET_GET_PRIVATE (self);

  tw_prv->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  tw_prv->right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (tw_prv->right_mouse_mp),
    GDK_BUTTON_SECONDARY);

  /* make widget able to notify */
  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_ALL_EVENTS_MASK);

  g_signal_connect (
    G_OBJECT (tw_prv->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (tw_prv->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
  g_signal_connect (
    G_OBJECT (tw_prv->event_box),
    "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(tw_prv->event_box),
    "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(tw_prv->event_box),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "destroy",
    G_CALLBACK (on_destroy),  NULL);

  g_object_ref (self);
}

static void
track_widget_class_init (TrackWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "track.ui");

  gtk_widget_class_set_css_name (klass,
                                 "track");

  gtk_widget_class_bind_template_child_private (
    klass,
    TrackWidget,
    color);
  gtk_widget_class_bind_template_child_private (
    klass,
    TrackWidget,
    paned);
  gtk_widget_class_bind_template_child_private (
    klass,
    TrackWidget,
    top_grid);
  gtk_widget_class_bind_template_child_private (
    klass,
    TrackWidget,
    event_box);
  /*gtk_widget_class_bind_template_child_private (*/
    /*klass,*/
    /*TrackWidget,*/
    /*lanes_box);*/
}
