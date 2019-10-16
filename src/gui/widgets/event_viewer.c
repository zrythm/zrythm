/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/region.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  EventViewerWidget,
  event_viewer_widget,
  GTK_TYPE_BOX)

enum TimelineColumns
{
  TIMELINE_COLUMN_NAME,
  TIMELINE_COLUMN_TYPE,
  TIMELINE_COLUMN_START_POS,
  TIMELINE_COLUMN_CLIP_START_POS,
  TIMELINE_COLUMN_LOOP_START_POS,
  TIMELINE_COLUMN_LOOP_END_POS,
  TIMELINE_COLUMN_END_POS,
  TIMELINE_COLUMN_OBJ,
  NUM_TIMELINE_COLUMNS,
};

static void
get_event_type_as_string (
  EventViewerEventType type,
  char *               buf)
{
  g_return_if_fail (buf || type >= 0);
  switch (type)
    {
    case EVENT_VIEWER_ET_REGION:
      strcpy (buf, _("Region"));
      break;
    case EVENT_VIEWER_ET_MARKER:
      strcpy (buf, _("Marker"));
      break;
    case EVENT_VIEWER_ET_SCALE_OBJECT:
      strcpy (buf, _("Scale"));
      break;
    }
}

static void
add_from_widget (
  GtkListStore * store,
  GtkTreeIter *  iter,
  GtkWidget *    widget)
{
  char type[500];
  char start_pos[50];
  char clip_start[50];
  char loop_start[50];
  char loop_end[50];
  char end_pos[50];
  if (Z_IS_REGION_WIDGET (widget))
    {
      REGION_WIDGET_GET_PRIVATE (widget);
      g_return_if_fail (rw_prv->region);
      Region * r = rw_prv->region;

      /* skip non-mains */
      if (!region_is_main (r) ||
          region_is_transient (r))
        return;

      region_get_type_as_string (
        r->type, type);
      position_stringize (
        &r->start_pos, start_pos);
      position_stringize (
        &r->end_pos, end_pos);
      position_stringize (
        &r->clip_start_pos, clip_start);
      position_stringize (
        &r->loop_start_pos, loop_start);
      position_stringize (
        &r->loop_end_pos, loop_end);
      gtk_list_store_append (store, iter);
      gtk_list_store_set (
        store, iter,
        TIMELINE_COLUMN_NAME, r->name,
        TIMELINE_COLUMN_TYPE, type,
        TIMELINE_COLUMN_START_POS, start_pos,
        TIMELINE_COLUMN_CLIP_START_POS, clip_start,
        TIMELINE_COLUMN_LOOP_START_POS, loop_start,
        TIMELINE_COLUMN_LOOP_END_POS, loop_end,
        TIMELINE_COLUMN_END_POS, end_pos,
        TIMELINE_COLUMN_OBJ, r,
        -1);
    }
  else if (Z_IS_MARKER_WIDGET (widget))
    {
      MarkerWidget * mw =
        Z_MARKER_WIDGET (widget);
      g_return_if_fail (mw->marker);
      Marker * m = mw->marker;

      /* skip non-mains */
      if (!marker_is_main (m) ||
          marker_is_transient (m))
        return;

      get_event_type_as_string (
        EVENT_VIEWER_ET_MARKER, type);
      position_stringize (
        &m->pos, start_pos);
      gtk_list_store_append (store, iter);
      gtk_list_store_set (
        store, iter,
        TIMELINE_COLUMN_NAME, m->name,
        TIMELINE_COLUMN_TYPE, type,
        TIMELINE_COLUMN_START_POS, start_pos,
        TIMELINE_COLUMN_CLIP_START_POS, "",
        TIMELINE_COLUMN_LOOP_START_POS, "",
        TIMELINE_COLUMN_LOOP_END_POS, "",
        TIMELINE_COLUMN_END_POS, "",
        TIMELINE_COLUMN_OBJ, m,
        -1);
    }
}

static GtkTreeModel *
create_timeline_model (
  EventViewerWidget * self)
{
  GtkListStore *store;
  GtkTreeIter iter;

  /* create list store */
  store =
    gtk_list_store_new (
      NUM_TIMELINE_COLUMNS,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_POINTER);

  /* add data to the list store (cheat by using
   * the timeline arranger children) */
  GList *children, * children_iter;
  children =
    gtk_container_get_children (
      GTK_CONTAINER (MW_TIMELINE));
  for (children_iter = children;
       children_iter != NULL;
       children_iter = g_list_next (children_iter))
    {
      GtkWidget * widget =
        GTK_WIDGET (children_iter->data);
      add_from_widget (
        store, &iter, widget);
    }
  g_list_free (children);
  children =
    gtk_container_get_children (
      GTK_CONTAINER (MW_PINNED_TIMELINE));
  for (children_iter = children;
       children_iter != NULL;
       children_iter = g_list_next (children_iter))
    {
      GtkWidget * widget =
        GTK_WIDGET (children_iter->data);
      add_from_widget (
        store, &iter, widget);
    }
  g_list_free (children);

  return GTK_TREE_MODEL (store);
}

static void
refresh_timeline_events (
  EventViewerWidget * self)
{
  self->model =
    create_timeline_model (self);
  gtk_tree_view_set_model (
    GTK_TREE_VIEW (self->treeview),
    self->model);
}

static void
add_timeline_columns (
  EventViewerWidget * self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Name"), renderer, "text",
      TIMELINE_COLUMN_NAME, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, TIMELINE_COLUMN_NAME);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for type */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Type"), renderer, "text",
      TIMELINE_COLUMN_TYPE, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, TIMELINE_COLUMN_TYPE);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for start pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Start"), renderer, "text",
      TIMELINE_COLUMN_START_POS, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, TIMELINE_COLUMN_START_POS);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for clip start pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Clip start"), renderer, "text",
      TIMELINE_COLUMN_CLIP_START_POS, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, TIMELINE_COLUMN_CLIP_START_POS);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for loop start pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Loop start"), renderer, "text",
      TIMELINE_COLUMN_LOOP_START_POS, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, TIMELINE_COLUMN_LOOP_START_POS);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for loop end pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Loop end"), renderer, "text",
      TIMELINE_COLUMN_LOOP_END_POS, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, TIMELINE_COLUMN_LOOP_END_POS);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for end pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("End"), renderer, "text",
      TIMELINE_COLUMN_END_POS, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, TIMELINE_COLUMN_END_POS);
  gtk_tree_view_append_column (
    self->treeview, column);
}

/**
 * Called to update the models.
 */
void
event_viewer_widget_refresh (
  EventViewerWidget * self)
{
  switch (self->type)
    {
    case EVENT_VIEWER_TYPE_TIMELINE:
      refresh_timeline_events (self);
      break;
    case EVENT_VIEWER_TYPE_EDITOR:
      /*refresh_editor_events (self);*/
      break;
    }
}

void
event_viewer_widget_refresh_for_arranger (
  ArrangerWidget *    arranger)
{
  if (Z_IS_TIMELINE_ARRANGER_WIDGET (arranger))
    {
      event_viewer_widget_refresh (
        MW_TIMELINE_EVENT_VIEWER);
    }
  else if (Z_IS_MIDI_ARRANGER_WIDGET (arranger))
    {
      /*event_viewer_widget_refresh (*/
        /*MW_TIMELINE_EVENT_VIEWER);*/
    }
}

/**
 * Sets up the event viewer.
 */
void
event_viewer_widget_setup (
  EventViewerWidget * self,
  EventViewerType     type)
{
  self->type = type;

  switch (type)
    {
    case EVENT_VIEWER_TYPE_TIMELINE:
      add_timeline_columns (self);
      refresh_timeline_events (self);
      break;
    case EVENT_VIEWER_TYPE_EDITOR:
      break;
    }
}

static void
event_viewer_widget_init (
  EventViewerWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


static void
event_viewer_widget_class_init (EventViewerWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "event_viewer.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, EventViewerWidget, x)

  BIND_CHILD (treeview);
}
