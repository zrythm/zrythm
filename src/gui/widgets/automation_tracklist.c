/*
 * gui/widgets/automation_tracklist.c - Track list for automations
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
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/track.h"
#include "utils/gtk.h"
#include "utils/arrays.h"

G_DEFINE_TYPE (AutomationTracklistWidget, automation_tracklist_widget, GTK_TYPE_BOX)

/**
 * Creates and returns an automation tracklist widget.
 */
AutomationTracklistWidget *
automation_tracklist_widget_new (TrackWidget * track_widget)
{
  AutomationTracklistWidget * self = g_object_new (
                            AUTOMATION_TRACKLIST_WIDGET_TYPE,
                            "orientation",
                            GTK_ORIENTATION_VERTICAL,
                            NULL);
  self->track_widget = track_widget;

  return self;
}

/**
 * Show or hide all automation track widgets.
 */
void
automation_tracklist_widget_show (AutomationTracklistWidget *self)
{
  /* if automation tracklist should be shown */
  if (self->track_widget->track->automations_visible)
    {
      /* if automation tracklist is hidden */
      if (gtk_paned_get_child2 (GTK_PANED (
            self->track_widget->track_automation_paned)) != self)
        {
          /* pack the automation tracklist in the track */
          gtk_paned_pack2 (self->track_widget->track_automation_paned,
                           GTK_WIDGET (self),
                           Z_GTK_NO_RESIZE,
                           Z_GTK_NO_SHRINK);
          if (self->has_g_object_ref)
            {
              g_object_unref (self);
              self->has_g_object_ref = 1;
            }

          /* add volume automation if no automations */
          if (self->num_automation_track_widgets == 0)
            {
              automation_tracklist_widget_add_automation_track (
                self,
                self->track_widget->track->automation_tracks[0],
                0);
            }
        }
      gtk_widget_show_all (GTK_WIDGET (self));
    }
  else /* if automation tracklist should be hidden */
    {
      /* if automation tracklist is visible */
      if (gtk_paned_get_child2 (GTK_PANED (
            self->track_widget->track_automation_paned)) == self)
        {
          /* remove the automation tracklist from the track */
          g_object_ref (self);
          self->has_g_object_ref = 1;
          gtk_container_remove (GTK_CONTAINER (self->track_widget->track_automation_paned),
                                GTK_WIDGET (self));
        }
    }
}

/**
 * Adds an automation track to the automation tracklist widget at pos.
 */
void
automation_tracklist_widget_add_automation_track (AutomationTracklistWidget * self,
                                                  AutomationTrack *           at,
                                                  int                         pos)
{
  if (pos > self->num_automation_track_widgets)
    {
      g_error ("Invalid position %d to add automation track in automation tracklist", pos);
      return;
    }

  g_message ("num automation track widgets %d", self->num_automation_track_widgets);

  /* create automation track */
  AutomationTrackWidget * at_widget =
    automation_track_widget_new (at);


  /* if first automation track */
  if (self->num_automation_track_widgets == 0)
    {
      /* pack the paned in the automations box */
      gtk_box_pack_start (GTK_BOX (self),
                          GTK_WIDGET (at_widget),
                          Z_GTK_EXPAND,
                          Z_GTK_FILL,
                          0);
    }
  else /* not first automation track */
    {
        if (pos == 0) /* if going to be placed first */
          {
            /* remove current automation track widget from its parent
             * and add it to new widget */
            AutomationTrackWidget * current_widget = self->automation_track_widgets[pos];
            g_object_ref (current_widget);
            gtk_container_remove (GTK_CONTAINER (self),
                                  GTK_WIDGET (current_widget));
            gtk_paned_pack2 (GTK_PANED (at_widget),
                             GTK_WIDGET (current_widget),
                             Z_GTK_RESIZE,
                             Z_GTK_NO_SHRINK);
            g_object_unref (current_widget);

            /* put new at widget where the prev at widget was in the parent */
            gtk_paned_pack2 (GTK_PANED (self),
                             GTK_WIDGET (at_widget),
                             Z_GTK_NO_RESIZE,
                             Z_GTK_NO_SHRINK);
          }
        else if (pos == self->num_automation_track_widgets) /* if last */
          {
            /* get parent */
            AutomationTrackWidget * parent_at_widget;
            parent_at_widget = self->automation_track_widgets[pos - 1];

            /* put new at widget in the parent */
            gtk_paned_pack2 (GTK_PANED (parent_at_widget),
                             GTK_WIDGET (at_widget),
                             Z_GTK_NO_RESIZE,
                             Z_GTK_NO_SHRINK);

          }
        else /* if not first or last position */
          {
            /* get parent */
            AutomationTrackWidget * parent_at_widget;
            parent_at_widget = self->automation_track_widgets[pos - 1];

            /* remove current automation track widget from its parent
             * and add it to new widget */
            AutomationTrackWidget * current_widget = self->automation_track_widgets[pos];
            g_object_ref (current_widget);
            gtk_container_remove (GTK_CONTAINER (parent_at_widget),
                                  GTK_WIDGET (current_widget));
            gtk_paned_pack2 (GTK_PANED (at_widget),
                             GTK_WIDGET (current_widget),
                             Z_GTK_NO_RESIZE,
                             Z_GTK_NO_SHRINK);
            g_object_unref (current_widget);

            /* put new at widget where the prev at widget was in the parent */
            gtk_paned_pack2 (GTK_PANED (parent_at_widget),
                             GTK_WIDGET (at_widget),
                             Z_GTK_NO_RESIZE,
                             Z_GTK_NO_SHRINK);
          }
    }

  /* insert into array */
  arrays_insert ((void **) self->automation_track_widgets,
                 &self->num_automation_track_widgets,
                 pos,
                 at_widget);
}

int
automation_tracklist_widget_get_automation_track_widget_index (
  AutomationTracklistWidget * self,
  AutomationTrackWidget * at)
{
  for (int i = 0; i < self->num_automation_track_widgets; i++)
    {
      if (self->automation_track_widgets[i] == at)
        return i;
    }
  g_error ("automation track widget not found in atuomation tracklist widget");
  return -1;
}

/**
 * GTK boilerplate.
 */
static void
automation_tracklist_widget_init (AutomationTracklistWidget * self)
{
}
static void
automation_tracklist_widget_class_init (AutomationTracklistWidgetClass * klass)
{
}

