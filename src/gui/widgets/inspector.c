/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/inspector_editor.h"
#include "gui/widgets/inspector_plugin.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/two_col_expander_box.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (InspectorWidget,
               inspector_widget,
               GTK_TYPE_STACK)

/**
 * Refreshes the inspector widget (shows current
 * selections.
 *
 * Uses Project->last_selection to decide which
 * stack to show.
 */
void
inspector_widget_refresh (
  InspectorWidget * self)
{
  if (PROJECT->last_selection == SELECTION_TYPE_TRACK)
    {
      inspector_track_widget_show_tracks (
        self->track,
        TRACKLIST_SELECTIONS);
      gtk_stack_set_visible_child (
        GTK_STACK (self),
        GTK_WIDGET (self->track));
    }
  else if (PROJECT->last_selection ==
           SELECTION_TYPE_PLUGIN)
    {
      inspector_plugin_widget_show (
        self->plugin,
        MIXER_SELECTIONS);
      gtk_stack_set_visible_child (
        GTK_STACK (self),
        GTK_WIDGET (self->plugin));
    }
}

InspectorWidget *
inspector_widget_new ()
{
  InspectorWidget * self =
    g_object_new (INSPECTOR_WIDGET_TYPE, NULL);

  gtk_widget_set_visible (GTK_WIDGET (self), 1);

  inspector_track_widget_show_tracks (
    self->track,
    TRACKLIST_SELECTIONS);
  gtk_stack_set_visible_child (
    GTK_STACK (self),
    GTK_WIDGET (self->track));

  return self;
}

static void
inspector_widget_class_init (
  InspectorWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "inspector.ui");

  gtk_widget_class_set_css_name (
    klass, "inspector");

  gtk_widget_class_bind_template_child (
    klass,
    InspectorWidget,
    track);
  gtk_widget_class_bind_template_child (
    klass,
    InspectorWidget,
    editor);
  gtk_widget_class_bind_template_child (
    klass,
    InspectorWidget,
    plugin);
}

static void
inspector_widget_init (InspectorWidget * self)
{
  gtk_widget_destroy (
    GTK_WIDGET (g_object_new (
      TWO_COL_EXPANDER_BOX_WIDGET_TYPE, NULL)));

  gtk_widget_init_template (GTK_WIDGET (self));

  /*self->size_group =*/
    /*gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);*/
  /*gtk_size_group_add_widget (self->size_group,*/
                             /*GTK_WIDGET (self->region));*/
  /* TODO add the rest */

  GtkRequisition min_size, nat_size;
  gtk_widget_get_preferred_size (
    GTK_WIDGET (self->track),
    &min_size,
    &nat_size);
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    min_size.width,
    -1);
}
