/*
 * gui/widgets/inspector_track.c - A inspector_track widget
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

#include "gui/widgets/inspector_track.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (InspectorTrackWidget, inspector_track_widget, GTK_TYPE_GRID)


static void
inspector_track_widget_class_init (InspectorTrackWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                               "/online/alextee/zrythm/ui/inspector_track.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        InspectorTrackWidget,
                                        position_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        InspectorTrackWidget,
                                        length_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        InspectorTrackWidget,
                                        color);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        InspectorTrackWidget,
                                        muted_toggle);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        InspectorTrackWidget,
                                        header);
}

static void
inspector_track_widget_init (InspectorTrackWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

InspectorTrackWidget *
inspector_track_widget_new ()
{
  InspectorTrackWidget * self = g_object_new (INSPECTOR_TRACK_WIDGET_TYPE, NULL);
  gtk_widget_show_all (GTK_WIDGET (self));

  return self;
}

void
inspector_track_widget_show_tracks (InspectorTrackWidget * self,
                                      Track **               tracks,
                                      int                     num_tracks)
{
  if (num_tracks == 1)
    {
      gtk_label_set_text (self->header, "Track");
    }
  else
    {
      char * string = g_strdup_printf ("Tracks (%d)", num_tracks);
      gtk_label_set_text (self->header, string);
      g_free (string);

      for (int i = 0; i < num_tracks; i++)
      {

      }
    }
}

