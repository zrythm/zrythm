/*
 * gui/widgets/inspector_region.c - A inspector_region widget
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

#include "gui/widgets/inspector_region.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (InspectorRegionWidget, inspector_region_widget, GTK_TYPE_GRID)


static void
inspector_region_widget_class_init (InspectorRegionWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                               "/online/alextee/zrythm/ui/inspector_region.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        InspectorRegionWidget,
                                        position_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        InspectorRegionWidget,
                                        length_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        InspectorRegionWidget,
                                        color);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        InspectorRegionWidget,
                                        muted_toggle);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        InspectorRegionWidget,
                                        header);
}

static void
inspector_region_widget_init (InspectorRegionWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

InspectorRegionWidget *
inspector_region_widget_new ()
{
  InspectorRegionWidget * self = g_object_new (INSPECTOR_REGION_WIDGET_TYPE, NULL);
  gtk_widget_show_all (GTK_WIDGET (self));

  return self;
}

void
inspector_region_widget_show_regions (InspectorRegionWidget * self,
                                      Region **               regions,
                                      int                     num_regions)
{
  if (num_regions == 1)
    {
      gtk_label_set_text (self->header, "Region");
    }
  else
    {
      char * string = g_strdup_printf ("Regions (%d)", num_regions);
      gtk_label_set_text (self->header, string);
      g_free (string);

      for (int i = 0; i < num_regions; i++)
      {

      }
    }
}
