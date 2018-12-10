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

G_DEFINE_TYPE (TracklistWidget, tracklist_widget, GTK_TYPE_BOX)


static TrackWidget *
get_hit_track (TracklistWidget *  self,
               double            x,
               double            y)
{
  /* go through each child */
  for(int i = 0; i < self->num_visible; i++)
    {
      TrackWidget * tw;
      tw = self->visible_tw[i];

      GtkAllocation allocation;
      GtkWidget * inner_tw;
      switch (tw->track->type)
        {
        case TRACK_TYPE_INSTRUMENT:
          inner_tw = GTK_WIDGET (tw->ins_tw);
          break;
        case TRACK_TYPE_MASTER:
          inner_tw = GTK_WIDGET (tw->master_tw);
          break;
        case TRACK_TYPE_AUDIO:
          inner_tw = GTK_WIDGET (tw->audio_tw);
          break;
        case TRACK_TYPE_CHORD:
          inner_tw = GTK_WIDGET (tw->chord_tw);
          break;
        case TRACK_TYPE_BUS:
          inner_tw = GTK_WIDGET (tw->bus_tw);
          break;
        }
      gtk_widget_get_allocation (inner_tw,
                                 &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (self),
                inner_tw,
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
 * Removes the given track from the tracklist.
 */
void
tracklist_widget_remove_track (Track * track)
{
  TracklistWidget * self = MW_TRACKLIST;

  FOREACH_TW
    {
      Track * t = self->visible_tw[i]->track;
      /*if (t == track)*/
        /*{*/
          /*int pos = i;*/
          /*TrackWidget * tw = t->widget;*/

          /*[> get parent track widget <]*/
          /*TrackWidget * parent_tw;*/
          /*if (pos != 0)*/
            /*{*/
              /*parent_tw = self->visible_tw[pos - 1];*/
            /*}*/

          /*[> if track to delete is last widget <]*/
          /*if (pos == self->num_visible - 1)*/
            /*{*/
              /* remove ddbox from track, remove track from parent
               * and add ddbox to parent track widget */
              /*g_object_ref (self->ddbox);*/
              /*gtk_container_remove (GTK_CONTAINER (tw),*/
                                    /*GTK_WIDGET (self->ddbox));*/
              /*if (pos == 0)*/
                /*{*/
                  /*gtk_container_remove (GTK_CONTAINER (tw->chord_tw),*/
                                        /*GTK_WIDGET (tw));*/
                  /*gtk_paned_pack2 (GTK_PANED (self->chord_tw),*/
                                   /*GTK_WIDGET (self->ddbox),*/
                                   /*Z_GTK_RESIZE,*/
                                   /*Z_GTK_SHRINK);*/
                /*}*/
              /*else*/
                /*{*/
                  /*gtk_container_remove (GTK_CONTAINER (parent_tw),*/
                                        /*GTK_WIDGET (tw));*/
                  /*gtk_paned_pack2 (GTK_PANED (parent_tw),*/
                                   /*GTK_WIDGET (self->ddbox),*/
                                   /*Z_GTK_RESIZE,*/
                                   /*Z_GTK_SHRINK);*/
                /*}*/
              /*g_object_unref (self->ddbox);*/
            /*}*/
          /*else [> if parent is not last widget <]*/
            /*{*/
              /* remove child track from track, remove track from parent
               * and add child track to parent */
              /*TrackWidget * child_tw;*/
              /*if (pos != 0)*/
                /*{*/
                  /*child_tw = self->visible_tw[pos];*/
                  /*g_object_ref (child_tw);*/
                /*}*/
              /*else*/
                /*{*/
                  /*g_object_ref (self->chord_tw);*/
                /*}*/

              /*gtk_container_remove (GTK_CONTAINER (tw),*/
                                    /*GTK_WIDGET (child_tw));*/
              /*if (pos == 0)*/
                /*{*/
                  /*gtk_container_remove (GTK_CONTAINER (parent_tw),*/
                                        /*GTK_WIDGET (tw));*/
                  /*gtk_paned_pack2 (GTK_PANED (parent_tw),*/
                                   /*GTK_WIDGET (child_tw),*/
                                   /*Z_GTK_RESIZE,*/
                                   /*Z_GTK_SHRINK);*/
                  /*g_object_unref (child_tw);*/
                /*}*/
              /*else*/
                /*{*/
                  /*gtk_container_remove (GTK_CONTAINER (parent_tw),*/
                                        /*GTK_WIDGET (tw));*/
                  /*gtk_paned_pack2 (GTK_PANED (parent_tw),*/
                                   /*GTK_WIDGET (child_tw),*/
                                   /*Z_GTK_RESIZE,*/
                                   /*Z_GTK_SHRINK);*/
                  /*g_object_unref (child_tw);*/
                /*}*/
            /*}*/

          /*[> delete from array <]*/
          /*array_delete ((void **) self->visible_tw,*/
                         /*&self->num_visible,*/
                         /*tw);*/

        /*}*/
    }
  gtk_widget_destroy (GTK_WIDGET (track->widget));
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
      InstrumentTrack * it;
      switch (track->type)
        {
        case TRACK_TYPE_MASTER:
        case TRACK_TYPE_CHORD:
          break;
        case TRACK_TYPE_INSTRUMENT:
          it = (InstrumentTrack *) track;
          mixer_remove_channel (it->channel);
          break;
          /* TODO below */
        case TRACK_TYPE_AUDIO:
          break;
        case TRACK_TYPE_BUS:
          break;
        }
    }
}

static void
on_add_ins_track ()
{
  TracklistWidget * self = MW_TRACKLIST;

  Channel * chan = channel_create (CT_MIDI, "Instrument Track");
  mixer_add_channel_and_init_track (chan);
  mixer_widget_add_channel (MW_MIXER, chan);
  tracklist_widget_add_track (self, chan->track, self->num_visible);
}

static void
show_context_menu ()
{
  TracklistWidget * self = MW_TRACKLIST;

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
                       (GCallback) on_add_ins_track, NULL);
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
      if (!array_contains ((void **)selected_tracks,
                           num_selected,
                           hit_tw->track))
        {
          if (state_mask & GDK_SHIFT_MASK ||
              state_mask & GDK_CONTROL_MASK)
            {
              tracklist_widget_toggle_select_track (self, hit_tw->track, 1);
            }
          else
            {
              tracklist_widget_toggle_select_track (self, hit_tw->track, 0);
            }
        }
    }
  else
    {
      tracklist_widget_toggle_select_all_tracks (self, 0);
    }

  if (n_press == 1)
    {
      show_context_menu ();
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
      if (state_mask & GDK_SHIFT_MASK ||
          state_mask & GDK_CONTROL_MASK)
        {
          tracklist_widget_toggle_select_track (self, hit_tw->track, 1);
        }
      else
        {
          tracklist_widget_toggle_select_track (self, hit_tw->track, 0);
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
tracklist_widget_toggle_select_all_tracks (TracklistWidget *self,
                                           int              select)
{
  FOREACH_TW
    {
      TrackWidget *tw;
      tw = self->visible_tw[i];

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

Track *
tracklist_widget_get_prev_visible_track (Track * track)
{
  TracklistWidget * self = MW_TRACKLIST;
  FOREACH_TW
    {
      TrackWidget * tw = self->visible_tw[i];
      if (tw == track->widget &&
          i != 0)
        {
          return self->visible_tw[i - 1]->track;
        }
    }
  return NULL;
}

Track *
tracklist_widget_get_next_visible_track (Track * track)
{
  TracklistWidget * self = MW_TRACKLIST;
  FOREACH_TW
    {
      TrackWidget * tw = self->visible_tw[i];
      if (tw == track->widget &&
          i != self->num_visible - 1)
        {
          return self->visible_tw[i + 1]->track;
        }
    }
  return NULL;
}


Track *
tracklist_widget_get_top_track ()
{
  TracklistWidget * self = MW_TRACKLIST;
  if (self->num_visible > 0)
    return self->visible_tw[0]->track;
  else
    return NULL;
}

/**
 * Adds given track to tracklist widget.
 */
void
tracklist_widget_add_track (TracklistWidget * self,
                            Track *           track,
                            int               pos)
{
  /* check if given arguments are valid */
  if (pos > self->num_visible ||
      pos < 0)
    {
      g_error ("Invalid position %d to add track in tracklist",
               pos);
      return;
    }
  if (track->type == TRACK_TYPE_MASTER &&
      tracklist_contains_master_track (self->tracklist))
    {
      g_error ("Master track already exists");
      return;
    }
  if (track->type == TRACK_TYPE_CHORD &&
      tracklist_contains_chord_track (self->tracklist))
    {
      g_error ("Chord track already exists");
      return;
    }

  /* create track */
  track->widget = track_widget_new (track);

  /* if first track to be added */
  if (pos == 0)
    {
      /* if no other tracks exist */
      if (self->num_visible == 0)
        {
          /* add box to track widget */
          self->ddbox =
            drag_dest_box_widget_new (
              GTK_ORIENTATION_VERTICAL,
              0,
              DRAG_DEST_BOX_TYPE_TRACKLIST);
          gtk_paned_pack2 (GTK_PANED (track->widget),
                           GTK_WIDGET (self->ddbox),
                           Z_GTK_NO_RESIZE,
                           Z_GTK_NO_SHRINK);
        }
      /* other first track exists */
      else
        {
          TrackWidget * first_tw = tracklist_widget_get_first_visible_track (self)->widget;

          /* remove first track from tracklist */
          g_object_ref (first_tw);
          gtk_container_remove (GTK_CONTAINER (self),
                                GTK_WIDGET (first_tw));

          /* pack previous first track to new track */
          gtk_paned_pack2 (GTK_PANED (track->widget),
                           GTK_WIDGET (first_tw),
                           Z_GTK_RESIZE,
                           Z_GTK_SHRINK);

          g_object_unref (first_tw);
        }

      /* add new track to tracklist */
      gtk_box_pack_start (GTK_BOX (self),
                          GTK_WIDGET (track->widget),
                          Z_GTK_EXPAND,
                          Z_GTK_FILL,
                          0);
    }
  /* if last track to be added */
  else if (pos == self->num_visible)
    {
      /* remove ddbox from last track and add to this
       * track */
      g_object_ref (self->ddbox);
      gtk_container_remove (
        GTK_CONTAINER (self->visible_tw
          [self->num_visible - 1]),
        gtk_paned_get_child2 (
          GTK_PANED (self->visible_tw
            [self->num_visible - 1])));
      gtk_paned_pack2 (GTK_PANED (track->widget),
                       GTK_WIDGET (self->ddbox),
                       Z_GTK_RESIZE,
                       Z_GTK_SHRINK);
      g_object_unref (self->ddbox);

      /* put new track widget where the box/prev track widget was in the parent */
      gtk_paned_pack2 (
        GTK_PANED (self->visible_tw
          [self->num_visible - 1]),
        GTK_WIDGET (track->widget),
        Z_GTK_RESIZE,
        Z_GTK_NO_SHRINK);
    }
  /* if adding track to any position other than first or
   * last */
  else
    {
      /* remove current track widget and add to new track_widget */
      TrackWidget * current_widget = self->visible_tw[pos];
      g_object_ref (current_widget);
      gtk_container_remove (
        GTK_CONTAINER (self->visible_tw[pos - 1]),
        GTK_WIDGET (current_widget));
      gtk_paned_pack2 (GTK_PANED (track->widget),
                       GTK_WIDGET (current_widget),
                       Z_GTK_RESIZE,
                       Z_GTK_SHRINK);
      g_object_unref (current_widget);

      /* put new track where the prev track was
       * in the parent */
      gtk_paned_pack2 (
        GTK_PANED (self->visible_tw[pos - 1]),
        GTK_WIDGET (track->widget),
        Z_GTK_RESIZE,
        Z_GTK_NO_SHRINK);
    }

  /* insert into array */
  array_insert ((void **) self->visible_tw,
                 &self->num_visible,
                 pos,
                 track->widget);
}

/**
 * Creates a new tracklist widget and sets it to main window.
 */
TracklistWidget *
tracklist_widget_new (Tracklist * tracklist)
{
  g_message ("Creating tracklist widget...");

  /* create widget */
  TracklistWidget * self = g_object_new (
                            TRACKLIST_WIDGET_TYPE,
                            "orientation",
                            GTK_ORIENTATION_VERTICAL,
                            NULL);
  self->tracklist = tracklist;
  tracklist->widget = self;

  /* set size */
  gtk_widget_set_size_request (GTK_WIDGET (self),
                               -1,
                               6000);

  /* add tracks */
  for (int i = 0; i < self->tracklist->num_tracks; i++)
    {
      Track * track = self->tracklist->tracks[i];
      if (track->visible)
        tracklist_widget_add_track (self,
                                    track,
                                    self->num_visible);
    }

  /* make widget able to notify */
  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  /* make widget focusable */
  gtk_widget_set_can_focus (GTK_WIDGET (self),
                           1);
  gtk_widget_set_focus_on_click (GTK_WIDGET (self),
                                 1);

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

  return self; /* cosmetic */
}

/**
 * Makes sure all the tracks for channels marked as visible are visible.
 */
void
tracklist_widget_show (TracklistWidget *self)
{
  gtk_widget_show (GTK_WIDGET (self));
  track_widget_show (MIXER->master->track->widget);
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      track_widget_show (MIXER->channels[i]->track->widget);
    }
  if (self->ddbox)
    gtk_widget_show (GTK_WIDGET (self->ddbox));
}

Track *
tracklist_widget_get_first_visible_track (TracklistWidget * self)
{
  if (self->num_visible > 0)
    {
      return self->visible_tw[0]->track;
    }
  else
    return NULL;
}

Track *
tracklist_widget_get_last_visible_track (TracklistWidget * self)
{
  if (self->num_visible > 0)
    {
      return self->visible_tw[self->num_visible - 1]->track;
    }
  else
    return NULL;
}

static void
tracklist_widget_init (TracklistWidget * self)
{
  self->drag = GTK_GESTURE_DRAG (
                gtk_gesture_drag_new (GTK_WIDGET (self)));
  self->multipress = GTK_GESTURE_MULTI_PRESS (
                gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  self->right_mouse_mp = GTK_GESTURE_MULTI_PRESS (
                gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->right_mouse_mp),
                                 GDK_BUTTON_SECONDARY);
}

static void
tracklist_widget_class_init (TracklistWidgetClass * klass)
{
}
