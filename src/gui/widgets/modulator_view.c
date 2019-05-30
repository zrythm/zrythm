/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/modulator.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/modulator_view.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

G_DEFINE_TYPE (ModulatorViewWidget,
               modulator_view_widget,
               GTK_TYPE_BOX)

void
modulator_view_widget_refresh (
  ModulatorViewWidget * self,
  Track *               track)
{
  self->track = track;

  z_gtk_container_remove_all_children (
    GTK_CONTAINER (self->modulators_box));

  for (int i = 0;
       i < self->track->num_modulators;
       i++)
    {
      Modulator * modulator =
        track->modulators[i];
      gtk_container_add (
        GTK_CONTAINER (self->modulators_box),
        GTK_WIDGET (modulator->widget));
    }

  DragDestBoxWidget * drag_dest =
    drag_dest_box_widget_new (
      GTK_ORIENTATION_HORIZONTAL,
      0,
      DRAG_DEST_BOX_TYPE_MODULATORS);
  gtk_box_pack_end (
    GTK_BOX (self->modulators_box),
    GTK_WIDGET (drag_dest),
    1, 1, 0);
}

static void
modulator_view_widget_init (
  ModulatorViewWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  GdkRGBA * color = calloc (1, sizeof (GdkRGBA));
  gdk_rgba_parse (color, "gray");
  color_area_widget_set_color (
    self->color, color);

  DragDestBoxWidget * drag_dest =
    drag_dest_box_widget_new (
      GTK_ORIENTATION_HORIZONTAL,
      0,
      DRAG_DEST_BOX_TYPE_MODULATORS);
  gtk_box_pack_end (
    GTK_BOX (self->modulators_box),
    GTK_WIDGET (drag_dest),
    1, 1, 0);
}

static void
modulator_view_widget_class_init (
  ModulatorViewWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "modulator_view.ui");

  gtk_widget_class_set_css_name (
    klass, "modulator-view");

  gtk_widget_class_bind_template_child (
    klass,
    ModulatorViewWidget,
    color);
  gtk_widget_class_bind_template_child (
    klass,
    ModulatorViewWidget,
    track_name);
  gtk_widget_class_bind_template_child (
    klass,
    ModulatorViewWidget,
    modulators_box);
}
