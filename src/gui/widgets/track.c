/*
 * gui/widgets/track.c - Track
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

#include "audio/automatable.h"
#include "audio/bus_track.h"
#include "audio/automation_track.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "audio/region.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/audio_track.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/bus_track.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/chord_track.h"
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/master_track.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "utils/gtk.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TrackWidget, track_widget, GTK_TYPE_GRID)

static void
size_allocate_cb (GtkWidget * widget, GtkAllocation * allocation, void * data)
{
  gtk_widget_queue_draw (GTK_WIDGET (
    MW_CENTER_DOCK->timeline));
  gtk_widget_queue_allocate (GTK_WIDGET (
    MW_CENTER_DOCK->timeline));
}

static void
on_show_automation (GtkWidget * widget, void * data)
{
  TrackWidget * self = TRACK_WIDGET (data);

  /* toggle visibility flag */
  self->track->bot_paned_visible = self->track->bot_paned_visible ? 0 : 1;

  tracklist_widget_show (MW_CENTER_DOCK->tracklist);
}

gboolean
on_draw (GtkWidget    * widget,
         cairo_t        *cr,
         gpointer      user_data)
{
  TrackWidget * self = TRACK_WIDGET (user_data);

  guint width, height;
  GtkStyleContext *context;
  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  if (self->selected)
    {
      cairo_set_source_rgba (cr, 1, 1, 1, 0.1);
      cairo_rectangle (cr, 0, 0, width, height);
      cairo_fill (cr);
    }

  return FALSE;
}

/**
 * Sets up the track widget.
 *
 * Sets color, draw callback, etc.
 */
TrackWidget *
track_widget_new (Track * track)
{
  g_assert (track);

  TrackWidget * self =
    g_object_new (TRACK_WIDGET_TYPE,
                  NULL);

  self->track = track;

  /* set color */
  GdkRGBA * color;
  switch (track->type)
    {
    case TRACK_TYPE_CHORD:
      color = calloc (1, sizeof (GdkRGBA));
      gdk_rgba_parse (color, "blue");
      break;
    case TRACK_TYPE_BUS:
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_AUDIO:
        {
          Channel * chan = track_get_channel (track);
          color = &chan->color;
          break;
        }
    }
  g_assert (color->alpha == 1.0);
  self->color = color_area_widget_new (color,
                                       5,
                                       -1);
  gtk_box_pack_start (self->color_box,
                      GTK_WIDGET (self->color),
                      1,
                      1,
                      0);

  g_signal_connect (self, "size-allocate",
                    G_CALLBACK (size_allocate_cb), NULL);
  g_signal_connect (self->track_box, "draw",
                    G_CALLBACK (on_draw), self);

  gtk_widget_show_all (GTK_WIDGET (self));

  switch (self->track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
      self->ins_tw = instrument_track_widget_new (self);
      gtk_box_pack_start (self->track_box,
                          GTK_WIDGET (self->ins_tw),
                          1,
                          1,
                          0);
      break;
    case TRACK_TYPE_MASTER:
      self->master_tw = master_track_widget_new (self);
      gtk_box_pack_start (self->track_box,
                          GTK_WIDGET (self->master_tw),
                          1,
                          1,
                          0);
      break;
    case TRACK_TYPE_AUDIO:
      self->audio_tw = audio_track_widget_new (self);
      gtk_box_pack_start (self->track_box,
                          GTK_WIDGET (self->audio_tw),
                          1,
                          1,
                          0);
      break;
    case TRACK_TYPE_CHORD:
      self->chord_tw = chord_track_widget_new (self);
      gtk_box_pack_start (self->track_box,
                          GTK_WIDGET (self->chord_tw),
                          1,
                          1,
                          0);
      break;
    case TRACK_TYPE_BUS:
      self->bus_tw = bus_track_widget_new (self);
      gtk_box_pack_start (self->track_box,
                          GTK_WIDGET (self->bus_tw),
                          1,
                          1,
                          0);
      break;
    }

  return self;
}

static void
track_widget_init (TrackWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
track_widget_class_init (TrackWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                               "/org/zrythm/ui/track.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                TrackWidget, color_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        TrackWidget,
                                        track_box);
}

void
track_widget_select (TrackWidget * self,
                     int           select) ///< 1 = select, 0 = unselect
{
  self->selected = select;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * Wrapper.
 */
void
track_widget_refresh (TrackWidget * self)
{
  switch (self->track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
      instrument_track_widget_refresh (
        self->ins_tw);
      break;
    case TRACK_TYPE_MASTER:
      master_track_widget_refresh (
        self->master_tw);
      break;
    case TRACK_TYPE_AUDIO:
      audio_track_widget_refresh (
        self->audio_tw);
      break;
    case TRACK_TYPE_CHORD:
      chord_track_widget_refresh (
        self->chord_tw);
      break;
    case TRACK_TYPE_BUS:
      bus_track_widget_refresh (
        self->bus_tw);
      break;
    }
}
