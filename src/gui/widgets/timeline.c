/*
 * gui/widgets/timeline.c - The timeline containing regions
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

#include "zrythm_app.h"
#include "project.h"
#include "settings_manager.h"
#include "gui/widgets/region.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/tracks.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TimelineWidget, timeline_widget, GTK_TYPE_OVERLAY)

#define MW_RULER MAIN_WINDOW->ruler

/**
 * Gets called to set the position/size of each overlayed widget.
 */
gboolean
get_child_position (GtkOverlay   *overlay,
               GtkWidget    *widget,
               GdkRectangle *allocation,
               gpointer      user_data)
{
  if (IS_REGION_WIDGET (widget))
    {
      allocation->x = 3;
      allocation->y = 3;
      allocation->width = 300;
      allocation->height = 100;
    }
}

TimelineWidget *
timeline_widget_new ()
{
  g_message ("Creating timeline...");
  TimelineWidget * self = g_object_new (TIMELINE_WIDGET_TYPE, NULL);

  self->bg = timeline_bg_widget_new ();

  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->bg));

  g_signal_connect (G_OBJECT (self), "get-child-position",
                    G_CALLBACK (get_child_position), NULL);

  gtk_overlay_add_overlay (GTK_OVERLAY (self),
                           MIXER->channels[0]->track->regions[0]->widget);

  return self;
}

static void
timeline_widget_class_init (TimelineWidgetClass * klass)
{
}

static void
timeline_widget_init (TimelineWidget *self )
{
}
