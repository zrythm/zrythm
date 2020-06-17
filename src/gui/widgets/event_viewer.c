/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

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

enum MidiColumns
{
  MIDI_COLUMN_NAME,
  MIDI_COLUMN_PITCH,
  MIDI_COLUMN_VELOCITY,
  MIDI_COLUMN_START_POS,
  MIDI_COLUMN_END_POS,
  MIDI_COLUMN_OBJ,
  NUM_MIDI_COLUMNS,
};

enum ChordColumns
{
  CHORD_COLUMN_NAME,
  CHORD_COLUMN_START_POS,
  CHORD_COLUMN_OBJ,
  NUM_CHORD_COLUMNS,
};

enum AutomationColumns
{
  AUTOMATION_COLUMN_INDEX,
  AUTOMATION_COLUMN_POS,
  AUTOMATION_COLUMN_VALUE,
  AUTOMATION_COLUMN_CURVINESS,
  AUTOMATION_COLUMN_OBJ,
  NUM_AUTOMATION_COLUMNS,
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
    case EVENT_VIEWER_ET_MIDI_NOTE:
      strcpy (buf, _("MIDI note"));
      break;
    case EVENT_VIEWER_ET_CHORD_OBJECT:
      strcpy (buf, _("Chord"));
      break;
    case EVENT_VIEWER_ET_AUTOMATION_POINT:
      strcpy (buf, _("Automation point"));
      break;
    }
}

static void
add_from_object (
  GtkListStore *   store,
  GtkTreeIter *    iter,
  ArrangerObject * obj)
{
  char name[200];
  char type[200];
  char start_pos[50];
  char clip_start[50];
  char loop_start[50];
  char loop_end[50];
  char end_pos[50];
  switch (obj->type)
    {
    case ARRANGER_OBJECT_TYPE_REGION:
      {
        ZRegion * r = (ZRegion *) obj;

        region_get_type_as_string (
          r->id.type, type);
        position_stringize (
          &obj->pos, start_pos);
        position_stringize (
          &obj->end_pos, end_pos);
        position_stringize (
          &obj->clip_start_pos, clip_start);
        position_stringize (
          &obj->loop_start_pos, loop_start);
        position_stringize (
          &obj->loop_end_pos, loop_end);
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
      break;
    case ARRANGER_OBJECT_TYPE_MARKER:
      {
        Marker * m = (Marker *) obj;

        get_event_type_as_string (
          EVENT_VIEWER_ET_MARKER, type);
        position_stringize (
          &obj->pos, start_pos);
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
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      {
        MidiNote * mn = (MidiNote *) obj;

        midi_note_get_val_as_string (
          mn, name, 0);
        get_event_type_as_string (
          EVENT_VIEWER_ET_MIDI_NOTE, type);
        position_stringize (
          &obj->pos, start_pos);
        position_stringize (
          &obj->end_pos, end_pos);
        char pitch[10];
        char vel[10];
        sprintf (pitch, "%d", mn->val);
        sprintf (vel, "%d", mn->vel->vel);
        gtk_list_store_append (store, iter);
        gtk_list_store_set (
          store, iter,
          MIDI_COLUMN_NAME, name,
          MIDI_COLUMN_PITCH, pitch,
          MIDI_COLUMN_VELOCITY, vel,
          MIDI_COLUMN_START_POS, start_pos,
          MIDI_COLUMN_END_POS, end_pos,
          MIDI_COLUMN_OBJ, mn,
          -1);
      }
      break;
    case ARRANGER_OBJECT_TYPE_CHORD_OBJECT:
      {
        ChordObject * c = (ChordObject *) obj;

        ChordDescriptor * descr =
          chord_object_get_chord_descriptor (c);
        chord_descriptor_to_string (
          descr, name);
        get_event_type_as_string (
          EVENT_VIEWER_ET_CHORD_OBJECT, type);
        position_stringize (
          &obj->pos, start_pos);
        gtk_list_store_append (store, iter);
        gtk_list_store_set (
          store, iter,
          CHORD_COLUMN_NAME, name,
          CHORD_COLUMN_START_POS, start_pos,
          CHORD_COLUMN_OBJ, c,
          -1);
      }
      break;
    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
      {
        AutomationPoint * ap =
          (AutomationPoint*) obj;

        get_event_type_as_string (
          EVENT_VIEWER_ET_AUTOMATION_POINT,
          type);
        position_stringize (
          &obj->pos, start_pos);
        char index[50];
        sprintf (index, "%d", ap->index);
        char value[50];
        sprintf (value, "%f", (double) ap->fvalue);
        char curviness[50];
        sprintf (
          curviness, "%f",
          ap->curve_opts.curviness);
        gtk_list_store_append (store, iter);
        gtk_list_store_set (
          store, iter,
          AUTOMATION_COLUMN_INDEX, index,
          AUTOMATION_COLUMN_POS, start_pos,
          AUTOMATION_COLUMN_VALUE, value,
          AUTOMATION_COLUMN_CURVINESS, curviness,
          AUTOMATION_COLUMN_OBJ, ap,
          -1);
      }
      break;
    default:
      break;
    }
}

#define ADD_FOREACH_IN_ARRANGER(arranger) \
  arranger_widget_get_all_objects ( \
    arranger, objs, &num_objs); \
  for (int i = 0; i < num_objs; i++) \
    { \
      ArrangerObject * obj = objs[i]; \
      add_from_object ( \
        store, &iter, obj); \
    } \

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
  ArrangerObject * objs[2000];
  int              num_objs;
  ADD_FOREACH_IN_ARRANGER (MW_TIMELINE);
  ADD_FOREACH_IN_ARRANGER (MW_PINNED_TIMELINE);

  return GTK_TREE_MODEL (store);
}

static GtkTreeModel *
create_midi_model (
  EventViewerWidget * self)
{
  GtkListStore *store;
  GtkTreeIter iter;

  /* create list store */
  store =
    gtk_list_store_new (
      NUM_MIDI_COLUMNS,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_POINTER);

  /* add data to the list */
  ArrangerObject * objs[2000];
  int              num_objs;
  ADD_FOREACH_IN_ARRANGER (
    MW_MIDI_ARRANGER);

  return GTK_TREE_MODEL (store);
}

static GtkTreeModel *
create_chord_model (
  EventViewerWidget * self)
{
  GtkListStore *store;
  GtkTreeIter iter;

  /* create list store */
  store =
    gtk_list_store_new (
      NUM_CHORD_COLUMNS,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_POINTER);

  /* add data to the list */
  ArrangerObject * objs[2000];
  int              num_objs;
  ADD_FOREACH_IN_ARRANGER (
    MW_CHORD_ARRANGER);

  return GTK_TREE_MODEL (store);
}

static GtkTreeModel *
create_automation_model (
  EventViewerWidget * self)
{
  GtkListStore *store;
  GtkTreeIter iter;

  /* create list store */
  store =
    gtk_list_store_new (
      NUM_AUTOMATION_COLUMNS,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_POINTER);

  /* add data to the list */
  ArrangerObject * objs[2000];
  int              num_objs;
  ADD_FOREACH_IN_ARRANGER (
    MW_AUTOMATION_ARRANGER);

  return GTK_TREE_MODEL (store);
}

static GtkTreeModel *
create_editor_model (
  EventViewerWidget * self)
{
  /* Check what to add based on the selected
   * region */
  ZRegion * r =
    clip_editor_get_region (CLIP_EDITOR);
  g_return_val_if_fail (r, NULL);

  /* add data to the list */
  switch (r->id.type)
    {
    case REGION_TYPE_MIDI:
      return create_midi_model (self);
      break;
    case REGION_TYPE_AUDIO:
      return NULL;
      /* TODO */
      break;
    case REGION_TYPE_AUTOMATION:
      return create_automation_model (self);
      break;
    case REGION_TYPE_CHORD:
      return create_chord_model (self);
      break;
    }

  g_return_val_if_reached (NULL);
}

#undef ADD_FOREACH_IN_ARRANGER

static void
add_timeline_columns (
  EventViewerWidget * self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* first remove existing columns */
  z_gtk_tree_view_remove_all_columns (
    self->treeview);

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

static void
append_midi_columns (
  EventViewerWidget * self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Note"), renderer, "text",
      MIDI_COLUMN_NAME, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, MIDI_COLUMN_NAME);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for pitch */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Pitch"), renderer, "text",
      MIDI_COLUMN_PITCH, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, MIDI_COLUMN_PITCH);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for velocity */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Velocity"), renderer, "text",
      MIDI_COLUMN_VELOCITY, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, MIDI_COLUMN_VELOCITY);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for start pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Start"), renderer, "text",
      MIDI_COLUMN_START_POS, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, MIDI_COLUMN_START_POS);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for end pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("End"), renderer, "text",
      MIDI_COLUMN_END_POS, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, MIDI_COLUMN_END_POS);
  gtk_tree_view_append_column (
    self->treeview, column);
}

static void
append_chord_columns (
  EventViewerWidget * self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Chord"), renderer, "text",
      CHORD_COLUMN_NAME, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, CHORD_COLUMN_NAME);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for start pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Position"), renderer, "text",
      CHORD_COLUMN_START_POS, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, CHORD_COLUMN_START_POS);
  gtk_tree_view_append_column (
    self->treeview, column);
}

static void
append_automation_columns (
  EventViewerWidget * self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Index"), renderer, "text",
      AUTOMATION_COLUMN_INDEX, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, AUTOMATION_COLUMN_INDEX);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for start pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Position"), renderer, "text",
      AUTOMATION_COLUMN_POS, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, AUTOMATION_COLUMN_POS);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for value */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Value"), renderer, "text",
      AUTOMATION_COLUMN_VALUE, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, AUTOMATION_COLUMN_VALUE);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for curviness */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Curviness"), renderer, "text",
      AUTOMATION_COLUMN_CURVINESS, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, AUTOMATION_COLUMN_CURVINESS);
  gtk_tree_view_append_column (
    self->treeview, column);
}

/**
 * @return If columns added.
 */
static int
add_editor_columns (
  EventViewerWidget * self)
{
  ZRegion * r =
    clip_editor_get_region (CLIP_EDITOR);
  if (!r)
    return 0;

  /* if the region type is the same no need to
   * remove/readd columns */
  if (self->region_type == r->id.type)
    return 1;
  else
    self->region_type = r->id.type;

  /* first remove existing columns */
  z_gtk_tree_view_remove_all_columns (
    self->treeview);

  switch (r->id.type)
    {
    case REGION_TYPE_MIDI:
      append_midi_columns (self);
      break;
    case REGION_TYPE_AUDIO:
      break;
    case REGION_TYPE_CHORD:
      append_chord_columns (self);
      break;
    case REGION_TYPE_AUTOMATION:
      append_automation_columns (self);
      break;
    default:
      break;
    }

  return 1;
}

static void
refresh_timeline_events (
  EventViewerWidget * self)
{
  add_timeline_columns (self);
  self->model =
    create_timeline_model (self);
  gtk_tree_view_set_model (
    GTK_TREE_VIEW (self->treeview),
    self->model);
}

static void
refresh_editor_events (
  EventViewerWidget * self)
{
  if (add_editor_columns (self))
    {
      self->model =
        create_editor_model (self);
      gtk_tree_view_set_model (
        GTK_TREE_VIEW (self->treeview),
        self->model);
    }
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
      refresh_editor_events (self);
      break;
    }
}

void
event_viewer_widget_refresh_for_arranger (
  ArrangerWidget * arranger)
{
  switch (arranger->type)
    {
    case ARRANGER_WIDGET_TYPE_TIMELINE:
      event_viewer_widget_refresh (
        MW_TIMELINE_EVENT_VIEWER);
      break;
    case ARRANGER_WIDGET_TYPE_MIDI:
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    case ARRANGER_WIDGET_TYPE_MIDI_MODIFIER:
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    case ARRANGER_WIDGET_TYPE_CHORD:
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    case ARRANGER_WIDGET_TYPE_AUTOMATION:
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    default:
      g_warn_if_reached ();
      break;
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
      refresh_timeline_events (self);
      break;
    case EVENT_VIEWER_TYPE_EDITOR:
      refresh_editor_events (self);
      break;
    }
}

static void
event_viewer_widget_init (
  EventViewerWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->region_type = -1;
}


static void
event_viewer_widget_class_init (
  EventViewerWidgetClass * _klass)
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
