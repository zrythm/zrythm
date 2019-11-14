/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/audio_bus_track.h"
#include "audio/audio_group_track.h"
#include "audio/master_track.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/region.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/audio_track.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/audio_bus_track.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/chord_track.h"
#include "gui/widgets/editable_label.h"
#include "gui/widgets/audio_group_track.h"
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker_track.h"
#include "gui/widgets/master_track.h"
#include "gui/widgets/midi_activity_bar.h"
#include "gui/widgets/midi_bus_track.h"
#include "gui/widgets/midi_group_track.h"
#include "gui/widgets/midi_track.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/track.h"
#include "gui/widgets/track_top_grid.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE_WITH_PRIVATE (
  TrackWidget, track_widget, GTK_TYPE_EVENT_BOX)

static gboolean
on_motion (
  GtkWidget *      widget,
  GdkEventMotion * event,
  TrackWidget *    self)
{
  TRACK_WIDGET_GET_PRIVATE (self);

  int height =
    gtk_widget_get_allocated_height (widget);

  /* show resize cursor */
  g_message ("y %f height %d",
             event->y, height);
  if (tw_prv->bg_hovered)
    {
      if (height - event->y < 12)
        {
          tw_prv->resize = 1;
          ui_set_cursor_from_name (widget, "s-resize");
        }
      else
        {
          tw_prv->resize = 0;
          ui_set_pointer_cursor (widget);
        }
    }

  if (event->type == GDK_ENTER_NOTIFY)
    {
      g_message ("enter");
      gtk_widget_set_state_flags (
        GTK_WIDGET (tw_prv->main_grid),
        GTK_STATE_FLAG_PRELIGHT, 0);
      tw_prv->bg_hovered = 1;
      tw_prv->resize = 0;
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      g_message ("leave");
      ui_set_pointer_cursor (widget);
      gtk_widget_unset_state_flags (
        GTK_WIDGET (tw_prv->main_grid),
        GTK_STATE_FLAG_PRELIGHT);
      tw_prv->bg_hovered = 0;
      tw_prv->resize = 0;
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

  g_return_if_fail (tw_prv && tw_prv->track);

  switch (tw_prv->track->type)
    {
      REFRESH_TW (INSTRUMENT, instrument);
      REFRESH_TW (MASTER, master);
      REFRESH_TW (AUDIO, audio);
      REFRESH_TW (CHORD, chord);
      REFRESH_TW (MIDI, midi);
      REFRESH_TW (AUDIO_BUS, audio_bus);
      REFRESH_TW (MIDI_BUS, midi_bus);
      REFRESH_TW (AUDIO_GROUP, audio_group);
      REFRESH_TW (MIDI_GROUP, midi_group);
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
      REFRESH_TW_BUTTONS (AUDIO_BUS, audio_bus);
      REFRESH_TW_BUTTONS (AUDIO_GROUP, audio_group);
      REFRESH_TW_BUTTONS (MIDI_BUS, midi_bus);
      REFRESH_TW_BUTTONS (MIDI_GROUP, midi_group);
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
    (int) y,
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
  else if (Z_IS_AUDIO_BUS_TRACK_WIDGET (self))
    {
      AudioBusTrackWidget * mtw =
        Z_AUDIO_BUS_TRACK_WIDGET (self);
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
  else if (Z_IS_AUDIO_BUS_TRACK_WIDGET (self))
    {
      AudioBusTrackWidget * mtw =
        Z_AUDIO_BUS_TRACK_WIDGET (self);
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

static TrackLane *
get_lane_at_y (
  TrackWidget * self,
  double        y)
{
  TRACK_WIDGET_GET_PRIVATE (self);
  Track * track = tw_prv->track;

  if (!track->lanes_visible)
    return NULL;

  TrackLane * lane = NULL;
  for (int i = 0; i < track->num_lanes; i++)
    {
      lane = track->lanes[i];

      g_message ("checking %d", i);
      if (lane->widget &&
          ui_is_child_hit (
            GTK_WIDGET (self),
            GTK_WIDGET (lane->widget),
            0, 1, 0, y, 0, 0))
        {
          return lane;
        }
    }

  return NULL;
}

/**
 * Info to pass when selecting a MIDI channel from
 * the context menu.
 */
typedef struct MidiChSelectionInfo
{
  /** Either one of these should be set. */
  Track *     track;
  TrackLane * lane;

  /** MIDI channel (1-16), or 0 for lane to
   * inherit. */
  midi_byte_t ch;
} MidiChSelectionInfo;

static void
on_midi_ch_selected (
  GtkMenuItem *         menu_item,
  MidiChSelectionInfo * info)
{
  if (info->lane)
    {
      info->lane->midi_ch = info->ch;
    }
  if (info->track)
    {
      info->track->midi_ch = info->ch;
    }
  free (info);
}

static void
show_context_menu (
  TrackWidget * self,
  double        y)
{
  GtkWidget *menu;
  GtkMenuItem *menuitem;
  menu = gtk_menu_new();
  TRACK_WIDGET_GET_PRIVATE (self);
  Track * track = tw_prv->track;
  TrackLane * lane =
    get_lane_at_y (self, y);

#define APPEND(mi) \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL (menu), \
    GTK_WIDGET (menuitem));

  int num_selected =
    TRACKLIST_SELECTIONS->num_tracks;

  if (num_selected > 0)
    {
      char * str;

      if (track->type != TRACK_TYPE_MASTER &&
          track->type != TRACK_TYPE_CHORD &&
          track->type != TRACK_TYPE_MARKER)
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
              g_strdup (_("_Duplicate Track"));
          else
            str =
              g_strdup (_("_Duplicate Tracks"));
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

      menuitem =
        z_gtk_create_menu_item (
          num_selected == 1 ?
            _("Hide Track") :
            _("Hide Tracks"),
          "z-gnumeric-column-hide",
          0,
          NULL,
          0,
          "win.hide-selected-tracks");
      APPEND (menuitem);

      menuitem =
        z_gtk_create_menu_item (
          num_selected == 1 ?
            _("Pin/Unpin Track") :
            _("Pin/Unpin Tracks"),
          "z-window-pin",
          0,
          NULL,
          0,
          "win.pin-selected-tracks");
      APPEND (menuitem);
    }

  /* add midi channel selectors */
  if (track_has_piano_roll (track))
    {
      menuitem =
        GTK_MENU_ITEM (
          gtk_menu_item_new_with_label (
            _("Track MIDI Ch")));

      GtkMenu * submenu =
        GTK_MENU (gtk_menu_new ());
      gtk_widget_set_visible (
        GTK_WIDGET (submenu), 1);
      GtkMenuItem * submenu_item;
      for (int i = 1; i <= 16; i++)
        {
          char * lbl =
            g_strdup_printf (
              _("%sMIDI Channel %d"),
              i == track->midi_ch ? "* " : "",
              i);
          submenu_item =
            GTK_MENU_ITEM (
              gtk_menu_item_new_with_label (lbl));
          g_free (lbl);

          MidiChSelectionInfo * info =
            calloc (
              1, sizeof (MidiChSelectionInfo));
          info->track = track;
          info->ch = (midi_byte_t) i;
          g_signal_connect (
            G_OBJECT (submenu_item), "activate",
            G_CALLBACK (on_midi_ch_selected),
            info);

          gtk_menu_shell_append (
            GTK_MENU_SHELL (submenu),
            GTK_WIDGET (submenu_item));
          gtk_widget_set_visible (
            GTK_WIDGET (submenu_item), 1);
        }

      gtk_menu_item_set_submenu (
        menuitem, GTK_WIDGET (submenu));
      gtk_widget_set_visible (
        GTK_WIDGET (menuitem), 1);

      APPEND (menuitem);

      if (lane)
        {
          char * lbl =
            g_strdup_printf (
              _("Lane %d MIDI Ch"),
              lane->pos);
          menuitem =
            GTK_MENU_ITEM (
              gtk_menu_item_new_with_label (
                lbl));
          g_free (lbl);

          submenu =
            GTK_MENU (gtk_menu_new ());
          gtk_widget_set_visible (
            GTK_WIDGET (submenu), 1);
          for (int i = 0; i <= 16; i++)
            {
              if (i == 0)
                lbl =
                  g_strdup_printf (
                    _("%sInherit"),
                    lane->midi_ch == i ? "* " : "");
              else
                lbl =
                  g_strdup_printf (
                    _("%sMIDI Channel %d"),
                    lane->midi_ch == i ? "* " : "",
                    i);
              submenu_item =
                GTK_MENU_ITEM (
                  gtk_menu_item_new_with_label (
                    lbl));
              g_free (lbl);

              MidiChSelectionInfo * info =
                calloc (
                  1, sizeof (MidiChSelectionInfo));
              info->lane = lane;
              info->ch = (midi_byte_t) i;
              g_signal_connect (
                G_OBJECT (submenu_item), "activate",
                G_CALLBACK (on_midi_ch_selected),
                info);

              gtk_menu_shell_append (
                GTK_MENU_SHELL (submenu),
                GTK_WIDGET (submenu_item));
              gtk_widget_set_visible (
                GTK_WIDGET (submenu_item), 1);
            }

          gtk_menu_item_set_submenu (
            menuitem, GTK_WIDGET (submenu));
          gtk_widget_set_visible (
            GTK_WIDGET (menuitem), 1);

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

  GdkModifierType state_mask =
    ui_get_state_mask (GTK_GESTURE (gesture));

  Track * track = tw_prv->track;
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
      show_context_menu (tw, y);
    }
}

static void
multipress_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  gpointer              user_data)
{
  TrackWidget * self =
    Z_TRACK_WIDGET (user_data);

  /* FIXME should do this via focus on click
   * property */
  /*if (!gtk_widget_has_focus (GTK_WIDGET (self)))*/
    /*gtk_widget_grab_focus (GTK_WIDGET (self));*/

  GdkModifierType state_mask =
    ui_get_state_mask (GTK_GESTURE (gesture));

  TRACK_WIDGET_GET_PRIVATE (self);
  Track * track = tw_prv->track;

  PROJECT->last_selection =
    SELECTION_TYPE_TRACK;

  track_select (
    track,
    track_is_selected (track) &&
    state_mask & GDK_CONTROL_MASK ?
      F_NO_SELECT: F_SELECT,
    (state_mask & GDK_SHIFT_MASK ||
      state_mask & GDK_CONTROL_MASK) ?
      0 : 1,
    1);
}

static void
on_drag_begin (GtkGestureDrag *gesture,
               gdouble         start_x,
               gdouble         start_y,
               TrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);

  tw_prv->selected_in_dnd = 0;
  tw_prv->dragged = 0;
}

static void
on_drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               TrackWidget * self)
{
  g_message ("drag_update");
  TRACK_WIDGET_GET_PRIVATE (self);

  tw_prv->dragged = 1;
}

static void
on_drag_data_get (
  GtkWidget        *widget,
  GdkDragContext   *context,
  GtkSelectionData *data,
  guint             info,
  guint             time,
  TrackWidget * self)
{
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
  TrackWidget * self)
{
  g_message ("dnd drag begin");
  TRACK_WIDGET_GET_PRIVATE (self);

  Track * track = tw_prv->track;
  tw_prv->selected_in_dnd = 1;
  MW_MIXER->start_drag_track = track;

  if (tw_prv->n_press == 1)
    {
      int ctrl = 0, selected = 0;

      ctrl = tw_prv->ctrl_held_at_start;

      if (tracklist_selections_contains_track (
            TRACKLIST_SELECTIONS,
            track))
        selected = 1;

      /* no control & not selected */
      if (!ctrl && !selected)
        {
          tracklist_selections_select_single (
            TRACKLIST_SELECTIONS,
            track);
        }
      else if (!ctrl && selected)
        { }
      else if (ctrl && !selected)
        tracklist_selections_add_track (
          TRACKLIST_SELECTIONS,
          track);
    }

  if (tw_prv->resize)
    {
      tw_prv->selected_in_dnd = 0;
      gdk_drag_abort (context, 0);
      g_message ("aborting drag");
    }
      g_message ("aaaaaa");
}

/**
 * Highlights/unhighlights the Tracks
 * appropriately.
 *
 * @param highlight 1 to highlight top or bottom,
 *   0 to unhighlight all.
 */
void
track_widget_do_highlight (
  TrackWidget * self,
  gint          x,
  gint          y,
  const int     highlight)
{
  TRACK_WIDGET_GET_PRIVATE (self);

  if (highlight)
    {
      /* if we are closer to the start of selection
       * than the end */
      int h =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (self));
      if (y < h / 2)
        {
          /* highlight top */
          gtk_drag_highlight (
            GTK_WIDGET (
              tw_prv->highlight_top_box));
          gtk_widget_set_size_request (
            GTK_WIDGET (
              tw_prv->highlight_top_box),
            -1, 2);

          /* unhilight bot */
          gtk_drag_unhighlight (
            GTK_WIDGET (
              tw_prv->highlight_bot_box));
          gtk_widget_set_size_request (
            GTK_WIDGET (
              tw_prv->highlight_bot_box),
            -1, -1);
        }
      else
        {
          /* highlight bot */
          gtk_drag_highlight (
            GTK_WIDGET (
              tw_prv->highlight_bot_box));
          gtk_widget_set_size_request (
            GTK_WIDGET (
              tw_prv->highlight_bot_box),
            -1, 2);

          /* unhilight top */
          gtk_drag_unhighlight (
            GTK_WIDGET (
              tw_prv->highlight_top_box));
          gtk_widget_set_size_request (
            GTK_WIDGET (
              tw_prv->highlight_top_box),
            -1, -1);
        }
    }
  else
    {
      gtk_drag_unhighlight (
        GTK_WIDGET (
          tw_prv->highlight_top_box));
      gtk_widget_set_size_request (
        GTK_WIDGET (tw_prv->highlight_top_box),
        -1, -1);
      gtk_drag_unhighlight (
        GTK_WIDGET (
          tw_prv->highlight_bot_box));
      gtk_widget_set_size_request (
        GTK_WIDGET (tw_prv->highlight_bot_box),
        -1, -1);
    }
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
    NEW_TW (AUDIO_BUS, audio_bus);
    NEW_TW (AUDIO_GROUP, audio_group);
    NEW_TW (MIDI_BUS, midi_bus);
    NEW_TW (MIDI_GROUP, midi_group);
    NEW_TW (INSTRUMENT, instrument);
    NEW_TW (MASTER, master);
    NEW_TW (AUDIO, audio);
    NEW_TW (MIDI, midi);
    NEW_TW (MARKER, marker);
    default:
      break;
    }

#undef NEW_TW

  g_warn_if_fail (Z_IS_TRACK_WIDGET (self));

  TRACK_WIDGET_GET_PRIVATE (self);
  color_area_widget_setup_track (
    tw_prv->color, track);
  midi_activity_bar_widget_setup_track (
    tw_prv->top_grid->midi_activity,
    track);
  editable_label_widget_setup (
    tw_prv->top_grid->name, track,
    track_get_name, track_set_name);

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
  TrackWidget *     data)
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

/**
 * Sets the Track name on the TrackWidget.
 */
void
track_widget_set_name (
  TrackWidget * self,
  const char * name)
{
  TRACK_WIDGET_GET_PRIVATE (self);

  gtk_label_set_text (
    tw_prv->top_grid->name->label, name);
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

  GtkWidgetClass * main_grid_class =
    GTK_WIDGET_GET_CLASS (tw_prv->main_grid);
  gtk_widget_class_set_css_name (
    main_grid_class, "track-grid");

  /*gtk_paned_set_wide_handle (*/
    /*(GtkPaned *) self, 1);*/
  /*gtk_orientable_set_orientation (*/
    /*GTK_ORIENTABLE (self),*/
    /*GTK_ORIENTATION_VERTICAL);*/
  gtk_widget_set_vexpand_set (
    (GtkWidget *) self, 1);

  /* set font sizes */
  gtk_label_set_max_width_chars (
    tw_prv->top_grid->name->label, 14);
  gtk_label_set_width_chars (
    tw_prv->top_grid->name->label, 14);
  gtk_label_set_ellipsize (
    tw_prv->top_grid->name->label, PANGO_ELLIPSIZE_END);
  gtk_label_set_xalign (
    tw_prv->top_grid->name->label, 0);

  tw_prv->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (
        GTK_WIDGET (self)));

  tw_prv->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (tw_prv->main_grid)));
  tw_prv->right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (tw_prv->main_grid)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (tw_prv->right_mouse_mp),
    GDK_BUTTON_SECONDARY);

  /* make widget able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  char * entry_track = g_strdup (TARGET_ENTRY_TRACK);
  GtkTargetEntry entries[] = {
    {
      entry_track, GTK_TARGET_SAME_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_TRACK),
    },
  };

  /* set as drag source for track */
  gtk_drag_source_set (
    GTK_WIDGET (tw_prv->event_box),
    GDK_BUTTON1_MASK,
    entries, G_N_ELEMENTS (entries),
    GDK_ACTION_MOVE | GDK_ACTION_COPY);
  g_free (entry_track);

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
    G_OBJECT (tw_prv->event_box),
    "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (tw_prv->event_box),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (tw_prv->drag), "drag-begin",
    G_CALLBACK (on_drag_begin), self);
  g_signal_connect (
    G_OBJECT (tw_prv->drag), "drag-update",
    G_CALLBACK (on_drag_update), self);
  g_signal_connect_after (
    GTK_WIDGET (tw_prv->event_box),
    "drag-begin",
    G_CALLBACK(on_dnd_drag_begin), self);
  g_signal_connect (
    GTK_WIDGET (tw_prv->event_box),
    "drag-data-get",
    G_CALLBACK (on_drag_data_get), self);
  g_signal_connect (
    G_OBJECT(self), "destroy",
    G_CALLBACK (on_destroy),  NULL);

  g_object_ref (self);
}

static void
track_widget_class_init (TrackWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "track.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child_private ( \
    klass, TrackWidget, x)

  BIND_CHILD (main_grid);
  BIND_CHILD (color);
  BIND_CHILD (paned);
  BIND_CHILD (top_grid);
  BIND_CHILD (event_box);
  BIND_CHILD (highlight_top_box);
  BIND_CHILD (highlight_bot_box);
}
