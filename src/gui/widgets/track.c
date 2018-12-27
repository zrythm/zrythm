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
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE_WITH_PRIVATE (TrackWidget,
                            track_widget,
                            GTK_TYPE_GRID)

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

  TRACK_WIDGET_GET_PRIVATE (self);

  /* toggle visibility flag */
  tw_prv->track->bot_paned_visible =
    tw_prv->track->bot_paned_visible ? 0 : 1;

  tracklist_widget_show (MW_CENTER_DOCK->tracklist);
}

gboolean
on_draw (GtkWidget    * widget,
         cairo_t        *cr,
         gpointer      user_data)
{
  TrackWidget * self = TRACK_WIDGET (user_data);
  TRACK_WIDGET_GET_PRIVATE (self);

  guint width, height;
  GtkStyleContext *context;
  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);

  if (tw_prv->selected)
    {
      cairo_set_source_rgba (cr, 1, 1, 1, 0.1);
      cairo_rectangle (cr, 0, 0, width, height);
      cairo_fill (cr);
    }

  return FALSE;
}

void
track_widget_select (TrackWidget * self,
                     int           select) ///< 1 = select, 0 = unselect
{
  TRACK_WIDGET_GET_PRIVATE (self);
  tw_prv->selected = select;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * Wrapper.
 */
void
track_widget_refresh (TrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);

  switch (tw_prv->track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
      instrument_track_widget_refresh (
        INSTRUMENT_TRACK_WIDGET (self));
      break;
    case TRACK_TYPE_MASTER:
      master_track_widget_refresh (
        MASTER_TRACK_WIDGET (self));
      break;
    case TRACK_TYPE_AUDIO:
      audio_track_widget_refresh (
        AUDIO_TRACK_WIDGET (self));
      break;
    case TRACK_TYPE_CHORD:
      chord_track_widget_refresh (
        CHORD_TRACK_WIDGET (self));
      break;
    case TRACK_TYPE_BUS:
      bus_track_widget_refresh (
        BUS_TRACK_WIDGET (self));
      break;
    }
}

TrackWidgetPrivate *
track_widget_get_private (TrackWidget * self)
{
  return track_widget_get_instance_private (self);
}

/**
 * Wrapper for child track widget.
 *
 * Sets color, draw callback, etc.
 */
TrackWidget *
track_widget_new (Track * track)
{
  g_assert (track);

  TrackWidget * self;

  switch (track->type)
    {
    case TRACK_TYPE_CHORD:
      self = TRACK_WIDGET (
        chord_track_widget_new (track));
      break;
    case TRACK_TYPE_BUS:
      self = TRACK_WIDGET (
        bus_track_widget_new (track));
      break;
    case TRACK_TYPE_INSTRUMENT:
      self = TRACK_WIDGET (
        instrument_track_widget_new (track));
      break;
    case TRACK_TYPE_MASTER:
      self = TRACK_WIDGET (
        master_track_widget_new (track));
      break;
    case TRACK_TYPE_AUDIO:
      self = TRACK_WIDGET (
        audio_track_widget_new (track));
      break;
    }

  g_assert (IS_TRACK_WIDGET (self));

  TRACK_WIDGET_GET_PRIVATE (self);
  tw_prv->track = track;

  g_signal_connect (self, "size-allocate",
                    G_CALLBACK (size_allocate_cb), NULL);
  g_signal_connect (self, "draw",
                    G_CALLBACK (on_draw), self);

  return self;
}

void
track_widget_on_show_automation (GtkWidget * widget,
                                 void *      data)
{
  TrackWidget * self =
    TRACK_WIDGET (data);

  TRACK_WIDGET_GET_PRIVATE (self);

  /* toggle visibility flag */
  tw_prv->track->bot_paned_visible =
    tw_prv->track->bot_paned_visible ? 0 : 1;

  /* FIXME rename to refresh */
  tracklist_widget_show (MW_TRACKLIST);
}

static void
track_widget_init (TrackWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  TRACK_WIDGET_GET_PRIVATE (self);
}

static void
track_widget_class_init (TrackWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "track.ui");

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
    name);
  gtk_widget_class_bind_template_child_private (
    klass,
    TrackWidget,
    icon);
  gtk_widget_class_bind_template_child_private (
    klass,
    TrackWidget,
    upper_controls);
  gtk_widget_class_bind_template_child_private (
    klass,
    TrackWidget,
    right_activity_box);
  gtk_widget_class_bind_template_child_private (
    klass,
    TrackWidget,
    mid_controls);
  gtk_widget_class_bind_template_child_private (
    klass,
    TrackWidget,
    bot_controls);
}
