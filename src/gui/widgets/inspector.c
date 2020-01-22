/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include <glib/gi18n.h>

G_DEFINE_TYPE (InspectorWidget,
               inspector_widget,
               GTK_TYPE_BOX)

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
  if (PROJECT->last_selection ==
        SELECTION_TYPE_TRACK)
    {
      inspector_track_widget_show_tracks (
        self->track,
        TRACKLIST_SELECTIONS);
      gtk_stack_set_visible_child (
        self->stack,
        GTK_WIDGET (self->track));
    }
  else if (PROJECT->last_selection ==
           SELECTION_TYPE_PLUGIN)
    {
      inspector_plugin_widget_show (
        self->plugin, MIXER_SELECTIONS);
      gtk_stack_set_visible_child (
        self->stack, GTK_WIDGET (self->plugin));
    }
}

/**
 * Sets up the inspector widget.
 */
void
inspector_widget_setup (
  InspectorWidget * self)
{
  inspector_track_widget_setup (
    self->track,
    TRACKLIST_SELECTIONS);
}

InspectorWidget *
inspector_widget_new ()
{
  InspectorWidget * self =
    g_object_new (INSPECTOR_WIDGET_TYPE, NULL);

  gtk_stack_set_visible_child (
    self->stack,
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

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    InspectorWidget, \
    child)

  BIND_CHILD (stack);
  BIND_CHILD (stack_switcher_box);
  BIND_CHILD (track);
  BIND_CHILD (editor);
  BIND_CHILD (plugin);

#undef BIND_CHILD
}

static void
inspector_widget_init (InspectorWidget * self)
{
  g_type_ensure (TWO_COL_EXPANDER_BOX_WIDGET_TYPE);
  g_type_ensure (INSPECTOR_TRACK_WIDGET_TYPE);
  g_type_ensure (INSPECTOR_EDITOR_WIDGET_TYPE);
  g_type_ensure (INSPECTOR_PLUGIN_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->stack_switcher =
    GTK_STACK_SWITCHER (gtk_stack_switcher_new ());
  gtk_stack_switcher_set_stack (
    self->stack_switcher,
    self->stack);
  gtk_widget_show_all (
    GTK_WIDGET (self->stack_switcher));
  gtk_box_pack_start (
    self->stack_switcher_box,
    GTK_WIDGET (self->stack_switcher),
    TRUE, TRUE, 0);

  /* set stackswitcher icons */
  GValue iconval1 = G_VALUE_INIT;
  GValue iconval2 = G_VALUE_INIT;
  GValue iconval3 = G_VALUE_INIT;
  g_value_init (&iconval1, G_TYPE_STRING);
  g_value_init (&iconval2, G_TYPE_STRING);
  g_value_init (&iconval3, G_TYPE_STRING);
  g_value_set_string (
    &iconval1, "z-media-album-track");
  g_value_set_string(
    &iconval2, "piano_roll");
  g_value_set_string(
    &iconval3, "z-plugins");

  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->track),
    "icon-name", &iconval1);
  g_value_set_string (
    &iconval1, _("Track properties"));
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->track),
    "title", &iconval1);
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->editor),
    "icon-name", &iconval2);
  g_value_set_string (
    &iconval2, _("Editor properties"));
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->editor),
    "title", &iconval2);
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->plugin),
    "icon-name", &iconval3);
  g_value_set_string (
    &iconval3, _("Plugin properties"));
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->plugin),
    "title", &iconval3);
  g_value_unset (&iconval1);
  g_value_unset (&iconval2);
  g_value_unset (&iconval3);

  GList *children, *iter;
  children =
    gtk_container_get_children (
      GTK_CONTAINER (self->stack_switcher));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      if (!GTK_IS_RADIO_BUTTON (iter->data))
        continue;

      GtkWidget * radio = GTK_WIDGET (iter->data);
      g_object_set (
        radio, "hexpand", TRUE, NULL);
    }

  g_list_free (children);


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
