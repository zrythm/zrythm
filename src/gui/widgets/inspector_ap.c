/*
 * gui/widgets/inspector_ap.c - A inspector_ap widget
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dsp/automation_point.h"
#include "gui/widgets/inspector_ap.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (InspectorApWidget, inspector_ap_widget, GTK_TYPE_GRID)

static void
inspector_ap_widget_class_init (InspectorApWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (
    GTK_WIDGET_CLASS (klass), "/org/zrythm/ui/inspector_ap.ui");

  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass), InspectorApWidget, position_box);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass), InspectorApWidget, length_box);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass), InspectorApWidget, color);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass), InspectorApWidget, mute_toggle);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass), InspectorApWidget, header);
}

static void
inspector_ap_widget_init (InspectorApWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

void
inspector_ap_widget_show_aps (
  InspectorApWidget * self,
  AutomationPoint **  aps,
  int                 num_aps)
{
  if (num_aps == 1)
    {
      gtk_label_set_text (self->header, "Ap");
    }
  else
    {
      char * string = g_strdup_printf ("Aps (%d)", num_aps);
      gtk_label_set_text (self->header, string);
      g_free (string);

      for (int i = 0; i < num_aps; i++)
        {
        }
    }
}
