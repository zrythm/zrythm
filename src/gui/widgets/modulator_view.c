/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/modulator.h"
#include "gui/widgets/modulator_macro.h"
#include "gui/widgets/modulator_view.h"
#include "gui/widgets/rotated_label.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

G_DEFINE_TYPE (
  ModulatorViewWidget,
  modulator_view_widget,
  GTK_TYPE_BOX)

void
modulator_view_widget_refresh (
  ModulatorViewWidget * self,
  Track *               track)
{
  self->track = track;
  rotated_label_widget_set_markup (
    self->track_name, track->name);
  color_area_widget_set_color (
    self->color, &track->color);

  z_gtk_widget_remove_all_children (
    GTK_WIDGET (self->modulators_box));

  for (int i = 0; i < self->track->num_modulators;
       i++)
    {
      Plugin * modulator = track->modulators[i];
      if (!modulator->modulator_widget)
        {
          modulator->modulator_widget =
            modulator_widget_new (modulator);
        }
      gtk_box_append (
        GTK_BOX (self->modulators_box),
        GTK_WIDGET (modulator->modulator_widget));
    }

  DragDestBoxWidget * drag_dest =
    drag_dest_box_widget_new (
      GTK_ORIENTATION_HORIZONTAL, 0,
      DRAG_DEST_BOX_TYPE_MODULATORS);
  gtk_box_append (
    GTK_BOX (self->modulators_box),
    GTK_WIDGET (drag_dest));
}

ModulatorViewWidget *
modulator_view_widget_new (void)
{
  ModulatorViewWidget * self =
    g_object_new (MODULATOR_VIEW_WIDGET_TYPE, NULL);

  return self;
}

static void
modulator_view_widget_init (
  ModulatorViewWidget * self)
{
  g_type_ensure (ROTATED_LABEL_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  rotated_label_widget_setup (self->track_name, -90);

  GdkRGBA color;
  gdk_rgba_parse (&color, "gray");
  color_area_widget_set_color (self->color, &color);

  DragDestBoxWidget * drag_dest =
    drag_dest_box_widget_new (
      GTK_ORIENTATION_HORIZONTAL, 0,
      DRAG_DEST_BOX_TYPE_MODULATORS);
  gtk_box_append (
    GTK_BOX (self->modulators_box),
    GTK_WIDGET (drag_dest));

  for (int i = 0; i < 8; i++)
    {
      self->macros[i] =
        modulator_macro_widget_new (i);
      gtk_widget_set_visible (
        GTK_WIDGET (self->macros[i]), true);
      gtk_box_append (
        GTK_BOX (self->macros_box),
        GTK_WIDGET (self->macros[i]));
    }
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

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ModulatorViewWidget, x)

  BIND_CHILD (color);
  BIND_CHILD (track_name);
  BIND_CHILD (modulators_box);
  BIND_CHILD (macros_box);
}
