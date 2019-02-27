/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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

#include "audio/automation_lane.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/automation_lane.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/track.h"
#include "utils/gtk.h"
#include "utils/arrays.h"

G_DEFINE_TYPE (AutomationTracklistWidget,
               automation_tracklist_widget,
               DZL_TYPE_MULTI_PANED)

#define GET_TRACK(self) Track * track = \
  self->automation_tracklist->track

/**
 * Creates and returns an automation tracklist widget.
 */
AutomationTracklistWidget *
automation_tracklist_widget_new (
  AutomationTracklist * automation_tracklist)
{
  AutomationTracklistWidget * self =
    g_object_new (
      AUTOMATION_TRACKLIST_WIDGET_TYPE,
      NULL);

  self->automation_tracklist = automation_tracklist;

  return self;
}

/**
 * Show or hide all automation lane widgets.
 */
void
automation_tracklist_widget_refresh (
  AutomationTracklistWidget *self)
{
  GET_TRACK (self);

  if (!track->bot_paned_visible)
    {
      gtk_widget_set_visible (GTK_WIDGET (self),
                              0);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self),
                              1);
    }

  /* remove all automation lanes */
  z_gtk_container_remove_all_children (
    GTK_CONTAINER (self));

  /* add automation lanes */
  g_message ("num automation lanes %d",
             self->automation_tracklist->num_automation_lanes);
  for (int i = 0;
       i < self->automation_tracklist->
         num_automation_lanes;
       i++)
    {
      AutomationLane * al =
        self->automation_tracklist->
          automation_lanes[i];

      /* show automation track */
      if (al->visible)
        {
          g_assert (GTK_IS_WIDGET (al->widget));
          /*g_message ("self %p, al %p, al widget %p",*/
                     /*self,*/
                     /*al,*/
                     /*al->widget);*/

          automation_lane_widget_refresh (
            al->widget);

          /* add to automation tracklist widget */
          gtk_container_add (
            GTK_CONTAINER (self),
            GTK_WIDGET (al->widget));
        }

      /* show/hide automation points/curves */
      /*for (int j = 0; j < at->num_automation_points; j++)*/
        /*{*/
          /*AutomationPoint * ap = at->automation_points[j];*/
          /*gtk_widget_set_visible (*/
            /*GTK_WIDGET (ap->widget),*/
            /*at->visible && track->bot_paned_visible);*/
        /*}*/
      /*for (int j = 0; j < at->num_automation_curves; j++)*/
        /*{*/
          /*AutomationCurve * ac = at->automation_curves[j];*/
          /*gtk_widget_set_visible (*/
            /*GTK_WIDGET (ac->widget),*/
            /*at->visible && track->bot_paned_visible);*/
        /*}*/
    }

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
      if (Z_IS_AUTOMATION_LANE_WIDGET (iter->data))
        {
          AutomationLaneWidget * alw =
            Z_AUTOMATION_LANE_WIDGET (iter->data);
          AutomationLane * al = alw->al;
          GValue a = G_VALUE_INIT;
          g_value_init (&a, G_TYPE_INT);
          g_value_set_int (&a, al->handle_pos);
          gtk_container_child_set_property (
            GTK_CONTAINER (self),
            GTK_WIDGET (alw),
            "position",
            &a);
        }
    }
  g_list_free(children);
}

static void
automation_tracklist_widget_init (
  AutomationTracklistWidget * self)
{
}

static void
automation_tracklist_widget_class_init (
  AutomationTracklistWidgetClass * klass)
{
}
