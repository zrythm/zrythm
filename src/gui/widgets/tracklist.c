/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/tracklist_selections.h"
#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/scale.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/chord_track.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  TracklistWidget, tracklist_widget, GTK_TYPE_BOX)

static void
on_drag_leave (
  GtkWidget      *widget,
  GdkDragContext *context,
  guint           time,
  TracklistWidget * self)
{
  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
      if (track->visible &&
          track->widget)
        {
          track_widget_do_highlight (
            track->widget, 0, 0, 0);
        }

    }
}

static void
on_dnd_motion (
  GtkWidget *widget,
  GdkDragContext *context,
  gint x,
  gint y,
  guint time,
  TracklistWidget * self)
{
  GdkModifierType mask;
  z_gtk_widget_get_mask (widget, &mask);
  if (mask & GDK_CONTROL_MASK)
    {
      gdk_drag_status (
        context, GDK_ACTION_COPY, time);
    }
  else
    {
      gdk_drag_status (
        context, GDK_ACTION_MOVE, time);
    }

  TrackWidget * hit_tw = NULL;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];

      if (!track->visible)
        continue;

      if (ui_is_child_hit (
            GTK_WIDGET (self),
            GTK_WIDGET (track->widget),
            1, 1, x, y, 0, 1))
        {
          hit_tw = track->widget;
        }
      else
        {
          track_widget_do_highlight (
            track->widget, x, y, 0);
        }
    }

  if (hit_tw)
    {
      GtkAllocation allocation;
      gtk_widget_get_allocation (
        GTK_WIDGET (hit_tw),
        &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates (
        GTK_WIDGET (self),
        GTK_WIDGET (hit_tw),
        (int) x, (int) y, &wx, &wy);
      track_widget_do_highlight (
        hit_tw, wx, wy, 1);
    }
}

static void
on_dnd_drag_data_received (
  GtkWidget        * widget,
  GdkDragContext   * context,
  gint               x,
  gint               y,
  GtkSelectionData * data,
  guint              info,
  guint              time,
  TracklistWidget *  self)
{
  g_message ("dnd data received on tracklist");

  /* get track widget at the x,y point */
  TrackWidget * hit_tw = NULL;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];

      if (!track->visible)
        continue;

      if (ui_is_child_hit (
            GTK_WIDGET (self),
            GTK_WIDGET (track->widget),
            1, 1, x, y, 0, 1))
        {
          hit_tw = track->widget;
        }
      else
        {
          track_widget_do_highlight (
            track->widget, x, y, 0);
        }
    }

  g_return_if_fail (hit_tw);

  Track * this = hit_tw->track;

  /* determine if moving or copying */
  GdkDragAction action =
    gdk_drag_context_get_selected_action (
      context);

  GtkAllocation allocation;
  gtk_widget_get_allocation (
    GTK_WIDGET (hit_tw),
    &allocation);

  gint wx, wy;
  gtk_widget_translate_coordinates (
    GTK_WIDGET (self),
    GTK_WIDGET (hit_tw),
    (int) x, (int) y, &wx, &wy);

  int h =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (hit_tw));

  /* determine position to move to */
  int pos;
  if (wy < h / 2)
    {
      if (this->pos <=
          MW_MIXER->start_drag_track->pos)
        pos = this->pos;
      else
        {
          Track * prev =
            tracklist_get_prev_visible_track (
              TRACKLIST, this);
          pos =
            prev ? prev->pos : this->pos;
        }
    }
  else
    {
      if (this->pos >=
          MW_MIXER->start_drag_track->pos)
        pos = this->pos;
      else
        {
          Track * next =
            tracklist_get_next_visible_track (
              TRACKLIST, this);
          pos =
            next ? next->pos : this->pos;
        }
    }

  UndoableAction * ua = NULL;
  if (action == GDK_ACTION_COPY)
    {
      ua =
        tracklist_selections_action_new_copy (
          TRACKLIST_SELECTIONS, pos);
    }
  else if (action == GDK_ACTION_MOVE)
    {
      ua =
        tracklist_selections_action_new_move (
          TRACKLIST_SELECTIONS, pos);
    }

  g_warn_if_fail (ua);

  undo_manager_perform (
    UNDO_MANAGER, ua);
}

TrackWidget *
tracklist_widget_get_hit_track (
  TracklistWidget *  self,
  double            x,
  double            y)
{
  /* go through each child */
  for(int i = 0;
      i < self->tracklist->num_tracks; i++)
    {
      Track * track = self->tracklist->tracks[i];
      if (!track->visible || track_is_pinned (track))
        continue;

      TrackWidget * tw = track->widget;

      /* return it if hit */
      if (ui_is_child_hit (
            GTK_WIDGET (self),
            GTK_WIDGET (tw),
            0, 1, x, y, 0, 0))
        return tw;
    }
  return NULL;
}

static gboolean
on_key_action (
  GtkWidget *       widget,
  GdkEventKey  *    event,
  TracklistWidget * self)
{
  if (event->state & GDK_CONTROL_MASK &&
      event->type == GDK_KEY_PRESS &&
      event->keyval == GDK_KEY_a)
    {
      tracklist_selections_select_all (
        TRACKLIST_SELECTIONS, 1);
    }

  return FALSE;
}

static void
tracklist_widget_on_size_allocate (
  GtkWidget *       widget,
  GdkRectangle *    allocation,
  TracklistWidget * self)
{
  if (!PROJECT || !TRACKLIST)
    return;

  EVENTS_PUSH (
    ET_TRACKS_RESIZED, self);
}

/**
 * Handle ctrl+shift+scroll.
 */
void
tracklist_widget_handle_vertical_zoom_scroll (
  TracklistWidget * self,
  GdkEventScroll *  event)
{
  if (!(event->state & GDK_CONTROL_MASK &&
        event->state & GDK_SHIFT_MASK))
    return;

  GtkScrolledWindow * scroll =
    self->unpinned_scroll;

  /* get current adjustment so we can get the
   * difference from the cursor */
  GtkAdjustment * adj =
    gtk_scrolled_window_get_vadjustment (scroll);
  double adj_val = gtk_adjustment_get_value (adj);
  double size_before =
    gtk_adjustment_get_upper (adj);
  double adj_perc = event->y / size_before;

  /* get px diff so we can calculate the new
   * adjustment later */
  double diff = event->y - adj_val;

  /* scroll down, zoom out */
  double size_after;
  double multiplier = 1.08;
  if (event->delta_y > 0)
    {
      size_after = size_before / multiplier;
    }
  else /* scroll up, zoom in */
    {
      size_after = size_before * multiplier;
    }

  bool can_resize =
    tracklist_multiply_track_heights (
      self->tracklist,
      event->delta_y > 0 ?
        1 / multiplier : multiplier,
      false, true, false);
  g_debug ("can resize: %d", can_resize);
  if (can_resize)
    {
      tracklist_multiply_track_heights (
        self->tracklist,
        event->delta_y > 0 ?
          1 / multiplier : multiplier,
        false, false, true);

      /* get updated adjustment and set its value
       at the same offset as before */
      adj =
        gtk_scrolled_window_get_vadjustment (scroll);
      gtk_adjustment_set_value (
        adj, adj_perc * size_after - diff);

      EVENTS_PUSH (
        ET_TRACKS_RESIZED, self);
    }
}

static gboolean
on_scroll (
  GtkWidget *widget,
  GdkEventScroll  *event,
  TracklistWidget * self)
{
  double x = event->x,
         y = event->y;

  g_debug ("scrolled to %f, %f", x, y);

  if (!(event->state & GDK_CONTROL_MASK &&
        event->state & GDK_SHIFT_MASK))
    return FALSE;

  tracklist_widget_handle_vertical_zoom_scroll (
    self, event);

  return TRUE;
}

void
tracklist_widget_hard_refresh (
  TracklistWidget * self)
{
  /* remove all children */
  z_gtk_container_remove_all_children (
    (GtkContainer *) self->unpinned_box);
  z_gtk_container_remove_all_children (
    (GtkContainer *) self->pinned_box);

  /** add pinned tracks */
  for (int i = 0; i < self->tracklist->num_tracks;
       i++)
    {
      Track * track = self->tracklist->tracks[i];

      if (!track_is_pinned (track))
        continue;

      /* create widget */
      if (!GTK_IS_WIDGET (track->widget))
        track->widget = track_widget_new (track);

      gtk_widget_set_visible (
        (GtkWidget *) track->widget,
        track->visible);

      /*track_widget_refresh (track->widget);*/

      track_widget_update_size (track->widget);

      gtk_container_add (
        (GtkContainer *) self->pinned_box,
        (GtkWidget *) track->widget);
    }

  /* readd all visible unpinned tracks to
   * scrolled window */
  for (int i = 0; i < self->tracklist->num_tracks;
       i++)
    {
      Track * track = self->tracklist->tracks[i];

      if (track_is_pinned (track))
        continue;

      /* create widget */
      if (!GTK_IS_WIDGET (track->widget))
        track->widget = track_widget_new (track);

      gtk_widget_set_visible (
        (GtkWidget *) track->widget,
        track->visible);

      /*track_widget_refresh (track->widget);*/

      track_widget_update_size (track->widget);

      gtk_container_add (
        (GtkContainer *) self->unpinned_box,
        (GtkWidget *) track->widget);
    }

  /* re-add ddbox */
  gtk_container_add (
    GTK_CONTAINER (self->unpinned_box),
    GTK_WIDGET (self->ddbox));

  /*g_object_unref (self->ddbox);*/

  /* set handle position.
   * this is done because the position resets to -1
   * every time a child is added or deleted */
  /*GList *children, *iter;*/
  /*children =*/
    /*gtk_container_get_children (GTK_CONTAINER (self));*/
  /*for (iter = children;*/
       /*iter != NULL;*/
       /*iter = g_list_next (iter))*/
    /*{*/
      /*if (Z_IS_TRACK_WIDGET (iter->data))*/
        /*{*/
          /*TrackWidget * tw = Z_TRACK_WIDGET (iter->data);*/
          /*TRACK_WIDGET_GET_PRIVATE (tw);*/
          /*Track * track = tw_prv->track;*/
          /*GValue a = G_VALUE_INIT;*/
          /*g_value_init (&a, G_TYPE_INT);*/
          /*g_value_set_int (&a, track->handle_pos);*/
          /*gtk_container_child_set_property (*/
            /*GTK_CONTAINER (self),*/
            /*GTK_WIDGET (tw),*/
            /*"position",*/
            /*&a);*/
        /*}*/
    /*}*/
  /*g_list_free(children);*/
}

/**
 * Makes sure all the tracks for channels marked as
 * visible are visible.
 */
void
tracklist_widget_update_track_visibility (
  TracklistWidget *self)
{
  gtk_widget_show (GTK_WIDGET (self));
  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (!GTK_IS_WIDGET (track->widget))
        track->widget = track_widget_new (track);

      gtk_widget_set_visible (
        GTK_WIDGET (track->widget),
        track->visible);

      track_widget_update_size (
        track->widget);
    }
}

void
tracklist_widget_setup (
  TracklistWidget * self,
  Tracklist * tracklist)
{
  g_return_if_fail (tracklist);

  self->tracklist = tracklist;
  tracklist->widget = self;

  tracklist_widget_hard_refresh (self);

  self->pinned_size_group =
    gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (
    self->pinned_size_group,
    GTK_WIDGET (self->pinned_box));
  gtk_size_group_add_widget (
    self->pinned_size_group,
    GTK_WIDGET (
      MW_TIMELINE_PANEL->pinned_timeline_scroll));

  self->unpinned_size_group =
    gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (
    self->unpinned_size_group,
    GTK_WIDGET (self->unpinned_box));
  gtk_size_group_add_widget (
    self->unpinned_size_group,
    GTK_WIDGET (
      MW_TIMELINE_PANEL->timeline));

  self->setup = true;
}

/**
 * Prepare for finalization.
 */
void
tracklist_widget_tear_down (
  TracklistWidget * self)
{
  g_message ("tearing down %p...", self);

  if (self->setup)
    {
      g_object_unref (self->pinned_box);
      g_object_unref (self->unpinned_scroll);
      g_object_unref (self->unpinned_box);
      g_object_unref (self->ddbox);

      self->setup = false;
    }

  g_message ("done");
}

static void
tracklist_widget_init (TracklistWidget * self)
{
  gtk_box_set_spacing (
    GTK_BOX (self), 1);

  self->pinned_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 1));
  g_object_ref (self->pinned_box);
  gtk_widget_set_visible (
    GTK_WIDGET (self->pinned_box), 1);

  /** add pinned box */
  gtk_container_add (
    (GtkContainer *) self,
    (GtkWidget *) self->pinned_box);

  self->unpinned_scroll =
    GTK_SCROLLED_WINDOW (
      gtk_scrolled_window_new (NULL, NULL));
  gtk_scrolled_window_set_policy (
    self->unpinned_scroll,
    GTK_POLICY_NEVER,
    GTK_POLICY_EXTERNAL);
  g_object_ref (self->unpinned_scroll);
  gtk_widget_set_visible (
    GTK_WIDGET (self->unpinned_scroll), 1);
  self->unpinned_box =
    GTK_BOX (
      gtk_box_new (
        GTK_ORIENTATION_VERTICAL, 1));
  g_object_ref (self->unpinned_box);
  gtk_widget_set_visible (
    GTK_WIDGET (self->unpinned_box), 1);

  /* add scrolled window */
  gtk_container_add (
    (GtkContainer *) self,
    (GtkWidget *) self->unpinned_scroll);
  gtk_container_add (
    (GtkContainer *) self->unpinned_scroll,
    (GtkWidget *) self->unpinned_box);

  /* create the drag dest box and bump its reference
   * so it doesn't get deleted. */
  self->ddbox =
    drag_dest_box_widget_new (
      GTK_ORIENTATION_VERTICAL,
      0,
      DRAG_DEST_BOX_TYPE_TRACKLIST);
  g_object_ref (self->ddbox);
  gtk_widget_set_visible (
    GTK_WIDGET (self->ddbox), 1);

  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (self),
    GTK_ORIENTATION_VERTICAL);

  /* make widget able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self),
    GDK_ALL_EVENTS_MASK);

  GtkTargetEntry entries[] = {
    {
      (char *) TARGET_ENTRY_TRACK,
      GTK_TARGET_SAME_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_TRACK),
    },
  };

  /* set as drag dest for track (the track will
   * be moved based on which half it was dropped in,
   * top or bot) */
  gtk_drag_dest_set (
    GTK_WIDGET (self),
    GTK_DEST_DEFAULT_MOTION |
      GTK_DEST_DEFAULT_DROP,
    entries, G_N_ELEMENTS (entries),
    GDK_ACTION_MOVE | GDK_ACTION_COPY);

  g_signal_connect (
    GTK_WIDGET (self), "drag-data-received",
    G_CALLBACK (on_dnd_drag_data_received), self);
  g_signal_connect (
    GTK_WIDGET (self), "drag-motion",
    G_CALLBACK (on_dnd_motion), self);
  g_signal_connect (
    GTK_WIDGET (self), "drag-leave",
    G_CALLBACK (on_drag_leave), self);
  g_signal_connect (
    G_OBJECT (self), "key-press-event",
    G_CALLBACK (on_key_action), self);
  g_signal_connect (
    G_OBJECT (self), "key-release-event",
    G_CALLBACK (on_key_action), self);
  g_signal_connect (
    G_OBJECT (self), "size-allocate",
    G_CALLBACK (
      tracklist_widget_on_size_allocate), self);
  g_signal_connect (
    G_OBJECT (self), "scroll-event",
    G_CALLBACK (on_scroll), self);
}

static void
tracklist_widget_class_init (
  TracklistWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "tracklist");
}
