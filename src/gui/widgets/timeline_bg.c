/*
 * gui/widgets/timeline_bg.c - Timeline background
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

#include "zrythm.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TimelineBgWidget,
               timeline_bg_widget,
               ARRANGER_BG_WIDGET_TYPE)


static gboolean
draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr,
                                &rect);

  /* handle horizontal drawing for tracks */
  GtkWidget * tw_widget;
  gint wx, wy;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (!track->visible)
        continue;

      /* draw line below widget */
      TrackWidget * tw = track->widget;
      tw_widget = GTK_WIDGET (tw);
      gtk_widget_translate_coordinates(
                tw_widget,
                GTK_WIDGET (MW_TRACKLIST),
                0,
                0,
                &wx,
                &wy);
      int line_y =
        wy + gtk_widget_get_allocated_height (
          GTK_WIDGET (tw_widget));
      if (line_y > rect.y &&
          line_y < (rect.y + rect.height))
        z_cairo_draw_horizontal_line (cr,
                                      line_y,
                                      rect.x,
                                      rect.x + rect.width,
                                      1.0);
    }

  /* draw automation related stuff */
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];

      AutomationTracklist * automation_tracklist =
        track_get_automation_tracklist (track);
      if (automation_tracklist)
        {
          for (int j = 0;
               j < automation_tracklist->num_automation_tracks;
               j++)
            {
              AutomationTrack * at = automation_tracklist->automation_tracks[j];

              if (at->widget &&
                  track->bot_paned_visible &&
                  at->visible)
                {
                  /* horizontal automation track lines */
                  gint wx, wy;
                  gtk_widget_translate_coordinates(
                            GTK_WIDGET (at->widget),
                            GTK_WIDGET (MW_TRACKLIST),
                            0,
                            0,
                            &wx,
                            &wy);
                  if (wy > rect.y &&
                      wy < (rect.y + rect.height))
                    z_cairo_draw_horizontal_line (
                      cr,
                      wy,
                      rect.x,
                      rect.x + rect.width,
                      0.2);

                }
            }
        }
    }



  return 0;
}

TimelineBgWidget *
timeline_bg_widget_new (RulerWidget *    ruler,
                        ArrangerWidget * arranger)
{
  TimelineBgWidget * self =
    g_object_new (TIMELINE_BG_WIDGET_TYPE,
                  NULL);

  ARRANGER_BG_WIDGET_GET_PRIVATE (self);
  ab_prv->ruler = ruler;
  ab_prv->arranger = arranger;

  return self;
}

static void
timeline_bg_widget_class_init (TimelineBgWidgetClass * _klass)
{
}

static void
timeline_bg_widget_init (TimelineBgWidget *self )
{
  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);
}
