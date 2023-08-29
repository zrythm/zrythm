// SPDX-FileCopyrightText: © 2018-2019, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/pinned_tracklist.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/gtk.h"
#include "zrythm.h"

G_DEFINE_TYPE (
  PinnedTracklistWidget,
  pinned_tracklist_widget,
  GTK_TYPE_BOX)

/**
 * Gets TrackWidget hit at the given coordinates.
 */
TrackWidget *
pinned_tracklist_widget_get_hit_track (
  PinnedTracklistWidget * self,
  double                  x,
  double                  y)
{
  /* go through each child */
  for (int i = 0; i < self->tracklist->num_tracks; i++)
    {
      Track *       track;
      TrackWidget * tw;
      gint          wx, wy;
      track = self->tracklist->tracks[i];
      if (!track->visible || !track->pinned)
        continue;

      tw = track->widget;

      int width = gtk_widget_get_width (GTK_WIDGET (tw));
      int height = gtk_widget_get_height (GTK_WIDGET (tw));

      gtk_widget_compute_point (
        GTK_WIDGET (self), GTK_WIDGET (tw), (int) x, (int) y,
        &wx, &wy);

      /* if hit */
      if (wx >= 0 && wx <= width && wy >= 0 && wy <= height)
        {
          return tw;
        }
    }
  return NULL;
}

/**
 * Removes and readds the tracks.
 */
void
pinned_tracklist_widget_hard_refresh (
  PinnedTracklistWidget * self)
{
  /* remove all tracks */
  z_gtk_container_remove_all_children (GTK_CONTAINER (self));

  /* add tracks */
  Track * track;
  for (int i = 0; i < self->tracklist->num_tracks; i++)
    {
      track = self->tracklist->tracks[i];
      if (!track->visible || !track->pinned)
        continue;

      /* create widget */
      if (!GTK_IS_WIDGET (track->widget))
        track->widget = track_widget_new (track);

      track_widget_refresh (track->widget);

      /* add to tracklist widget */
      gtk_container_add (
        GTK_CONTAINER (self), GTK_WIDGET (track->widget));
    }
  /*GtkWidget * sep =*/
  /*gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);*/
  /*gtk_widget_set_visible (sep, 1);*/
  /*gtk_container_add (*/
  /*GTK_CONTAINER (self),*/
  /*sep);*/

  /* set handle position.
   * this is done because the position resets to
   * -1 every time a child is added or deleted */
  GList *children, *iter;
  children = gtk_container_get_children (GTK_CONTAINER (self));
  for (iter = children; iter != NULL; iter = g_list_next (iter))
    {
      if (Z_IS_TRACK_WIDGET (iter->data))
        {
          TrackWidget * tw = Z_TRACK_WIDGET (iter->data);
          TRACK_WIDGET_GET_PRIVATE (tw);
          track = tw_prv->track;
          GValue a = G_VALUE_INIT;
          g_value_init (&a, G_TYPE_INT);
          g_value_set_int (&a, track->handle_pos);
          gtk_container_child_set_property (
            GTK_CONTAINER (self), GTK_WIDGET (tw), "position",
            &a);
        }
    }
  g_list_free (children);
}

static void
pinned_tracklist_widget_on_size_allocate (
  GtkWidget *             widget,
  GdkRectangle *          allocation,
  PinnedTracklistWidget * self)
{
  /*gtk_widget_set_size_request (*/
  /*GTK_WIDGET (*/
  /*MW_CENTER_DOCK->pinned_timeline_scroll),*/
  /*-1, allocation->height);*/
  if (
    gtk_paned_get_position (
      MW_TIMELINE_PANEL->timeline_divider_pane)
    != allocation->height)
    {
      gtk_paned_set_position (
        MW_TIMELINE_PANEL->timeline_divider_pane,
        allocation->height);
    }
}

/**
 * Sets up the PinnedTracklistWidget.
 */
void
pinned_tracklist_widget_setup (
  PinnedTracklistWidget * self,
  Tracklist *             tracklist)
{
  g_warn_if_fail (tracklist);
  self->tracklist = tracklist;
  tracklist->pinned_widget = self;

  pinned_tracklist_widget_hard_refresh (self);

  g_signal_connect (
    G_OBJECT (self), "size-allocate",
    G_CALLBACK (pinned_tracklist_widget_on_size_allocate),
    self);
}

static void
pinned_tracklist_widget_class_init (
  PinnedTracklistWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);

  gtk_widget_class_set_css_name (klass, "ruler-tracklist");
}

static void
pinned_tracklist_widget_init (PinnedTracklistWidget * self)
{
  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
  gtk_box_set_spacing (GTK_BOX (self), 1);
}
