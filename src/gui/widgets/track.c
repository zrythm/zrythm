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
#include "audio/tracklist.h"
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

void
track_widget_select (TrackWidget * self,
                     int           select) ///< 1 = select, 0 = unselect
{
  TRACK_WIDGET_GET_PRIVATE (self);
  tw_prv->track->selected = select;
  if (select)
    {
      gtk_widget_set_state_flags (GTK_WIDGET (self),
                                  GTK_STATE_FLAG_SELECTED,
                                  0);
    }
  else
    {
      gtk_widget_unset_state_flags (GTK_WIDGET (self),
                                    GTK_STATE_FLAG_SELECTED);
    }
}

static void
on_motion (GtkWidget * widget,
           GdkEventMotion *event,
           gpointer        user_data)
{
  TrackWidget * self = TRACK_WIDGET (user_data);

  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (GTK_WIDGET (self),
                                  GTK_STATE_FLAG_PRELIGHT,
                                  0);
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (GTK_WIDGET (self),
                                    GTK_STATE_FLAG_PRELIGHT);
    }
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

GtkWidget *
track_widget_get_bottom_paned (TrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);

  return gtk_paned_get_child2 (tw_prv->paned);
}

static void
track_widget_init (TrackWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  TRACK_WIDGET_GET_PRIVATE (self);

  /* make widget able to notify */
  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  g_signal_connect (G_OBJECT (self), "size-allocate",
                    G_CALLBACK (size_allocate_cb), NULL);
  g_signal_connect (G_OBJECT (tw_prv->event_box), "enter-notify-event",
                    G_CALLBACK (on_motion),  self);
  g_signal_connect (G_OBJECT(tw_prv->event_box), "leave-notify-event",
                    G_CALLBACK (on_motion),  self);
  g_signal_connect (G_OBJECT(tw_prv->event_box), "motion-notify-event",
                    G_CALLBACK (on_motion),  self);
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
  gtk_widget_class_bind_template_child_private (
    klass,
    TrackWidget,
    event_box);
}
