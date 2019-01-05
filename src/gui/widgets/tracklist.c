/*
 * gui/widgets/tracks.c - TrackList
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

/** \file
 */

#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/mixer.h"
#include "audio/scale.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/chord_track.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/track.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
#include "utils/ui.h"

G_DEFINE_TYPE (TracklistWidget,
               tracklist_widget,
               DZL_TYPE_MULTI_PANED)

static void
on_resize_end (TracklistWidget * self,
               GtkWidget *       child)
{
  if (!IS_TRACK_WIDGET (child))
    return;

  TrackWidget * tw = TRACK_WIDGET (child);
  TRACK_WIDGET_GET_PRIVATE (tw);
  Track * track = tw_prv->track;
  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_INT);
  gtk_container_child_get_property (
    GTK_CONTAINER (self),
    GTK_WIDGET (tw),
    "position",
    &a);
  track->handle_pos = g_value_get_int (&a);
}

static TrackWidget *
get_hit_track (TracklistWidget *  self,
               double            x,
               double            y)
{
  /* go through each child */
  for(int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (!track->visible)
        continue;

      TrackWidget * tw = track->widget;

      GtkAllocation allocation;
      gtk_widget_get_allocation (GTK_WIDGET (tw),
                                 &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (self),
                GTK_WIDGET (tw),
                x,
                y,
                &wx,
                &wy);

      /* if hit */
      if (wx >= 0 &&
          wx <= allocation.width &&
          wy >= 0 &&
          wy <= allocation.height)
        {
          return tw;
        }
    }
  return NULL;
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{

}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{

}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{


}

/**
 * Delete track(s) action handler.
 */
static void
on_delete_tracks ()
{
  TracklistWidget * self = MW_TRACKLIST;

  GET_SELECTED_TRACKS;

  for (int i = 0; i < num_selected; i++)
    {
      Track * track = selected_tracks[i];
      switch (track->type)
        {
        case TRACK_TYPE_CHORD:
          break;
        case TRACK_TYPE_INSTRUMENT:
        case TRACK_TYPE_MASTER:
        case TRACK_TYPE_BUS:
        case TRACK_TYPE_AUDIO:
            {
              ChannelTrack * ct = (ChannelTrack *) track;
              mixer_remove_channel (MIXER,
                                    ct->channel);
              tracklist_remove_track (self->tracklist,
                                      track);
              break;
            }
        }
    }
}

static void
on_add_ins_track (GtkMenuItem * menu_item,
                  TracklistWidget * self)
{
  Channel * chan = channel_create (CT_MIDI,
                                   "Instrument Track");
  mixer_add_channel (MIXER, chan);
  mixer_widget_refresh (MW_MIXER);
  tracklist_append_track (TRACKLIST,
                          chan->track);
  tracklist_widget_refresh (MW_TRACKLIST);
}

static void
show_context_menu (TracklistWidget * self)
{
  GtkWidget *menu, *menuitem;
  menu = gtk_menu_new();

  GET_SELECTED_TRACKS;

  if (num_selected > 0)
    {
      char * str;
      if (num_selected == 1)
        str = g_strdup_printf ("Delete Track");
      else
        str = g_strdup_printf ("Delete %d Tracks",
                               num_selected);
      menuitem = gtk_menu_item_new_with_label(str);
      g_free (str);
      g_signal_connect(menuitem, "activate",
                       (GCallback) on_delete_tracks, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }
  else
    {
      menuitem = gtk_menu_item_new_with_label("Add Instrument Track");
      g_signal_connect(menuitem, "activate",
                       (GCallback) on_add_ins_track, self);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
      menuitem = gtk_menu_item_new_with_label("Add Audio Track");
      /*g_signal_connect(menuitem, "activate",*/
                       /*(GCallback) on_delete_tracks, NULL);*/
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
      menuitem = gtk_menu_item_new_with_label("Add Bus");
      /*g_signal_connect(menuitem, "activate",*/
                       /*(GCallback) on_delete_tracks, NULL);*/
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }

  gtk_widget_show_all(menu);
  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

static void
on_right_click (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  TracklistWidget * self = (TracklistWidget *) user_data;

  if (!gtk_widget_has_focus (GTK_WIDGET (self)))
    gtk_widget_grab_focus (GTK_WIDGET (self));

  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
  /*guint button =*/
    /*gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));*/
  const GdkEvent * event =
    gtk_gesture_get_last_event (GTK_GESTURE (gesture), sequence);
  GdkModifierType state_mask;
  gdk_event_get_state (event, &state_mask);

  GET_SELECTED_TRACKS

  TrackWidget * hit_tw = get_hit_track (self, x, y);
  if (hit_tw)
    {
      TRACK_WIDGET_GET_PRIVATE (hit_tw);
      Track * track = tw_prv->track;

      if (!array_contains ((void **)selected_tracks,
                           num_selected,
                           track))
        {
          if (state_mask & GDK_SHIFT_MASK ||
              state_mask & GDK_CONTROL_MASK)
            {
              tracklist_widget_toggle_select_track (self,
                                                    track,
                                                    1);
            }
          else
            {
              tracklist_widget_toggle_select_track (self,
                                                    track,
                                                    0);
            }
        }
    }
  else
    {
      tracklist_widget_toggle_select_all_tracks (self, 0);
    }

  if (n_press == 1)
    {
      show_context_menu (self);
    }
}

static void
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  TracklistWidget * self = (TracklistWidget *) user_data;

  if (!gtk_widget_has_focus (GTK_WIDGET (self)))
    gtk_widget_grab_focus (GTK_WIDGET (self));

  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
  /*guint button =*/
    /*gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));*/
  const GdkEvent * event =
    gtk_gesture_get_last_event (GTK_GESTURE (gesture), sequence);
  GdkModifierType state_mask;
  gdk_event_get_state (event, &state_mask);

  TrackWidget * hit_tw = get_hit_track (self, x, y);
  if (hit_tw)
    {
      TRACK_WIDGET_GET_PRIVATE (hit_tw);
      Track * track = tw_prv->track;

      if (state_mask & GDK_SHIFT_MASK ||
          state_mask & GDK_CONTROL_MASK)
        {
          tracklist_widget_toggle_select_track (self,
                                                track,
                                                1);
        }
      else
        {
          tracklist_widget_toggle_select_track (self,
                                                track,
                                                0);
        }
    }
  else
    {
      if (!(state_mask & GDK_SHIFT_MASK ||
          state_mask & GDK_CONTROL_MASK))
        tracklist_widget_toggle_select_all_tracks (self, 0);
    }
}

static gboolean
on_key_action (GtkWidget *widget,
               GdkEventKey  *event,
               gpointer   user_data)
{
  TracklistWidget * self = (TracklistWidget *) user_data;

  if (event->state & GDK_CONTROL_MASK &&
      event->type == GDK_KEY_PRESS &&
      event->keyval == GDK_KEY_a)
    {
      tracklist_widget_toggle_select_all_tracks (self, 1);
    }

  return FALSE;
}

/**
 * Selects or deselects all tracks.
 */
void
tracklist_widget_toggle_select_all_tracks (
  TracklistWidget *self,
  int              select)
{
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (!track->visible)
        continue;

      TrackWidget *tw = track->widget;

      track_widget_select (tw, select);
    }

  GET_SELECTED_TRACKS;

  /* show the selected tracks in the inspector */
  inspector_widget_show_selections (INSPECTOR_CHILD_TRACK,
                                    (void **) selected_tracks,
                                    num_selected);
}

void
tracklist_widget_toggle_select_track (TracklistWidget * self,
                               Track *           track,
                               int               append) ///< append to selections
{
  GET_SELECTED_TRACKS;

  if (!append)
    {
      /* deselect existing selections */
      for (int i = 0; i < num_selected; i++)
        {
          Track * t = selected_tracks[i];
          track_widget_select (t->widget, 0);
        }
      num_selected = 0;
    }

  /* if already selected */
  if (array_contains ((void **) selected_tracks,
                       num_selected,
                       track))
    {
      /* deselect track */
      track_widget_select (track->widget, 0);
    }
  else /* not selected */
    {
      /* select track */
      track_widget_select (track->widget, 1);
    }

  /* show the selected tracks in the inspector */
  inspector_widget_show_selections (INSPECTOR_CHILD_TRACK,
                                    (void **) selected_tracks,
                                    num_selected);
}

void
tracklist_widget_refresh (TracklistWidget * self)
{
  /* remove ddbox */
  g_object_ref (self->ddbox);
  gtk_container_remove (
    GTK_CONTAINER (self),
    GTK_WIDGET (self->ddbox));

  /* remove all tracks */
  z_gtk_container_remove_all_children (
    GTK_CONTAINER (self));

  /* add tracks */
  for (int i = 0; i < self->tracklist->num_tracks; i++)
    {
      Track * track = self->tracklist->tracks[i];
      if (track->visible)
        {
          /* create widget */
          track->widget = track_widget_new (track);

          g_assert (track->widget);
          track_widget_refresh (track->widget);

          /* add to tracklist widget */
          gtk_container_add (GTK_CONTAINER (self),
                             GTK_WIDGET (track->widget));
        }
    }

  /* re-add ddbox */
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->ddbox));
  g_object_unref (self->ddbox);

  /* set handle position.
   * this is done because the position resets to -1 every
   * time a child is added or deleted */
  GList *children, *iter;
  children =
    gtk_container_get_children (GTK_CONTAINER (self));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      if (IS_TRACK_WIDGET (iter->data))
        {
          TrackWidget * tw = TRACK_WIDGET (iter->data);
          TRACK_WIDGET_GET_PRIVATE (tw);
          Track * track = tw_prv->track;
          GValue a = G_VALUE_INIT;
          g_value_init (&a, G_TYPE_INT);
          g_value_set_int (&a, track->handle_pos);
          gtk_container_child_set_property (
            GTK_CONTAINER (self),
            GTK_WIDGET (tw),
            "position",
            &a);
        }
    }
  g_list_free(children);
}

void
tracklist_widget_setup (TracklistWidget * self,
                        Tracklist * tracklist)
{
  g_assert (tracklist);
  self->tracklist = tracklist;
  tracklist->widget = self;

  tracklist_widget_refresh (self);
}

/**
 * Makes sure all the tracks for channels marked as visible are visible.
 */
void
tracklist_widget_show (TracklistWidget *self)
{
  gtk_widget_show (GTK_WIDGET (self));
  track_widget_refresh (MIXER->master->track->widget);
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      track_widget_refresh (MIXER->channels[i]->track->widget);
    }
  if (self->ddbox)
    gtk_widget_show (GTK_WIDGET (self->ddbox));
}

static void
tracklist_widget_init (TracklistWidget * self)
{
  self->ddbox =
    drag_dest_box_widget_new (
      GTK_ORIENTATION_VERTICAL,
      0,
      DRAG_DEST_BOX_TYPE_TRACKLIST);
  gtk_container_add (GTK_CONTAINER (self),
                   GTK_WIDGET (self->ddbox));

  self->drag = GTK_GESTURE_DRAG (
                gtk_gesture_drag_new (GTK_WIDGET (self)));
  self->multipress = GTK_GESTURE_MULTI_PRESS (
                gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  self->right_mouse_mp = GTK_GESTURE_MULTI_PRESS (
                gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->right_mouse_mp),
                                 GDK_BUTTON_SECONDARY);

  /* make widget able to notify */
  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  g_signal_connect (G_OBJECT(self->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT (self->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (G_OBJECT (self->right_mouse_mp), "pressed",
                    G_CALLBACK (on_right_click), self);
  g_signal_connect (G_OBJECT (self), "key-press-event",
                    G_CALLBACK (on_key_action), self);
  g_signal_connect (G_OBJECT (self), "key-release-event",
                    G_CALLBACK (on_key_action), self);
  g_signal_connect (G_OBJECT (self), "resize-drag-end",
                    G_CALLBACK (on_resize_end), NULL);
}

static void
tracklist_widget_class_init (TracklistWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "tracklist");
}
