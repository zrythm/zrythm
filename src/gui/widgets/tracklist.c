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
#include "audio/mixer.h"
#include "audio/track.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/track.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
#include "utils/ui.h"

/* FIXME find an algorithm to name new channels */
static int counter = 0;

G_DEFINE_TYPE (TracklistWidget, tracklist_widget, GTK_TYPE_BOX)

/**
 * Adds master track.
 */
void
tracklist_widget_add_master_track (TracklistWidget * self)
{
  if (self->master_track_widget)
    {
      g_error ("Master track already added to tracklist");
      return;
    }

  Track * track = MIXER->master->track;

  /* create track widget */
  TrackWidget * track_widget = track_widget_new (track);
  track->widget = track_widget;


  /* add last box to new track widget */
  self->ddbox = drag_dest_box_widget_new (GTK_ORIENTATION_VERTICAL,
                                          0,
                                          DRAG_DEST_BOX_TYPE_TRACKLIST);
  gtk_paned_pack2 (GTK_PANED (track_widget),
                   GTK_WIDGET (self->ddbox),
                   Z_GTK_NO_RESIZE,
                   Z_GTK_NO_SHRINK);

  /* add master track widget to tracklist */
  gtk_box_pack_start (GTK_BOX (self),
                      GTK_WIDGET (track_widget),
                      Z_GTK_EXPAND,
                      Z_GTK_FILL,
                      0);


  self->master_track_widget = track_widget;
}

/**
 * Adds a track to the tracklist widget.
 *
 * Must NOT be used with master track (see tracklist_widget_add_master_track).
 */
void
tracklist_widget_add_track (TracklistWidget * self,
                            Track *           track,
                            int               pos) ///< position to insert at,
                                                  ///< starting from 0 after master
{
  g_message ("adding track %s to tracklist widget", track->channel->name);
  if (pos > self->num_track_widgets)
    {
      g_error ("Invalid position %d to add track in tracklist", pos);
      return;
    }

  /* create track */
  TrackWidget * track_widget = track_widget_new (track);
  track->widget = track_widget;

  /* get parent track widget */
  TrackWidget * parent_track_widget = pos == 0 ?
                                      self->master_track_widget :
                                      self->track_widgets[pos - 1];

  /* if parent is last widget */
  if (pos == self->num_track_widgets)
    {
      /* remove ddbox and add to new track_widget */
      g_object_ref (self->ddbox);
      gtk_container_remove (GTK_CONTAINER (parent_track_widget),
                            gtk_paned_get_child2 (
                                    GTK_PANED (parent_track_widget)));
      gtk_paned_pack2 (GTK_PANED (track_widget),
                       GTK_WIDGET (self->ddbox),
                       Z_GTK_RESIZE,
                       Z_GTK_SHRINK);
      g_object_unref (self->ddbox);
    }
  else /* if parent is not last widget */
    {
      /* remove current track widget and add to new track_widget */
      TrackWidget * current_widget = self->track_widgets[pos];
      g_object_ref (current_widget);
      gtk_container_remove (GTK_CONTAINER (parent_track_widget),
                            GTK_WIDGET (current_widget));
      gtk_paned_pack2 (GTK_PANED (track_widget),
                       GTK_WIDGET (current_widget),
                       Z_GTK_RESIZE,
                       Z_GTK_SHRINK);
      g_object_unref (current_widget);
    }

  /* put new track widget where the box/prev track widget was in the parent */
  gtk_paned_pack2 (GTK_PANED (parent_track_widget),
                   GTK_WIDGET (track_widget),
                   Z_GTK_RESIZE,
                   Z_GTK_NO_SHRINK);

  /* insert into array */
  arrays_insert ((void **) self->track_widgets,
                 &self->num_track_widgets,
                 pos,
                 track_widget);
}

/**
 * Creates a new tracklist widget and sets it to main window.
 */
TracklistWidget *
tracklist_widget_new ()
{
  g_message ("Creating tracklist widget...");

  /* create widget */
  TracklistWidget * self = g_object_new (
                            TRACKLIST_WIDGET_TYPE,
                            "orientation",
                            GTK_ORIENTATION_VERTICAL,
                            NULL);
  MAIN_WINDOW->tracklist = self;

  /* set size */
  gtk_widget_set_size_request (GTK_WIDGET (self),
                               -1,
                               6000);

  /* add each channel */
  tracklist_widget_add_master_track (self);
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * channel = MIXER->channels[i];
      tracklist_widget_add_track (self, channel->track, i);
    }

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
  gtk_widget_show (GTK_WIDGET (self->ddbox));
}



static void
tracklist_widget_init (TracklistWidget * self)
{
}

static void
tracklist_widget_class_init (TracklistWidgetClass * klass)
{
}
