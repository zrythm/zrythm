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
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
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

enum AudioColumns
{
  AUDIO_COLUMN_START_POS,
  AUDIO_COLUMN_END_POS,
  NUM_AUDIO_COLUMNS,
};

#define SET_SORT_POSITION_FUNC(col) \
  { \
    FuncData * data = object_new (FuncData); \
    data->column = col; \
    gtk_tree_sortable_set_sort_func ( \
      GTK_TREE_SORTABLE (store), \
      data->column, sort_position_func, data, \
      free); \
  }

#define SET_POSITION_PRINT_FUNC(col) \
  { \
    FuncData * data = object_new (FuncData); \
    data->column = col; \
    gtk_tree_view_column_set_cell_data_func ( \
      column, renderer, \
      print_position_cell_data_func, data, free); \
  }

static void
get_event_type_as_string (
  ArrangerObjectType type,
  char *             buf)
{
  g_return_if_fail (
    buf || type > ARRANGER_OBJECT_TYPE_ALL);

  const char * untranslated_type =
    arranger_object_stringize_type (type);
  strcpy (buf, _(untranslated_type));
}

typedef struct FuncData
{
  /** Column ID. */
  int                 column;
} FuncData;

static int
sort_position_func (
  GtkTreeModel * model,
  GtkTreeIter *  a,
  GtkTreeIter *  b,
  gpointer       user_data)
{
  FuncData * sort_data = (FuncData *) user_data;
  Position * pos1 = NULL;
  gtk_tree_model_get (
    model, a, sort_data->column, &pos1, -1);
  Position * pos2 = NULL;
  gtk_tree_model_get (
    model, b, sort_data->column, &pos2, -1);
  if (!pos1 && !pos2)
    return 0;
  else if (pos1 && !pos2)
    return 1;
  else if (!pos1 && pos2)
    return -1;
  else
    return (pos1->ticks < pos2->ticks) ? -1 : 1;
}

static void
print_position_cell_data_func (
  GtkTreeViewColumn * tree_column,
  GtkCellRenderer *   cell,
  GtkTreeModel *      tree_model,
  GtkTreeIter *       iter,
  gpointer            data)
{
  FuncData * print_data = (FuncData *) data;
  int col_id = print_data->column;
  Position * pos = NULL;
  gtk_tree_model_get (
    tree_model, iter, col_id, &pos, -1);
  char pos_str[50];
  if (pos)
    position_to_string_full (pos, pos_str, 1);
  else
    strcpy (pos_str, "");
  g_object_set (
    cell, "text", pos_str, NULL);
}

static void
add_from_object (
  GtkListStore *   store,
  GtkTreeIter *    iter,
  ArrangerObject * obj)
{
  char name[200];
  char type[200];
  switch (obj->type)
    {
    case ARRANGER_OBJECT_TYPE_REGION:
      {
        ZRegion * r = (ZRegion *) obj;

        region_get_type_as_string (
          r->id.type, type);
        gtk_list_store_append (store, iter);
        gtk_list_store_set (
          store, iter,
          TIMELINE_COLUMN_NAME, r->name,
          TIMELINE_COLUMN_TYPE, type,
          TIMELINE_COLUMN_START_POS, &obj->pos,
          TIMELINE_COLUMN_CLIP_START_POS,
          &obj->clip_start_pos,
          TIMELINE_COLUMN_LOOP_START_POS,
          &obj->loop_start_pos,
          TIMELINE_COLUMN_LOOP_END_POS,
          &obj->loop_end_pos,
          TIMELINE_COLUMN_END_POS,
          &obj->end_pos,
          TIMELINE_COLUMN_OBJ, r,
          -1);
      }
      break;
    case ARRANGER_OBJECT_TYPE_MARKER:
      {
        Marker * m = (Marker *) obj;

        get_event_type_as_string (obj->type, type);
        gtk_list_store_append (store, iter);
        gtk_list_store_set (
          store, iter,
          TIMELINE_COLUMN_NAME, m->name,
          TIMELINE_COLUMN_TYPE, type,
          TIMELINE_COLUMN_START_POS, &obj->pos,
          TIMELINE_COLUMN_CLIP_START_POS, NULL,
          TIMELINE_COLUMN_LOOP_START_POS, NULL,
          TIMELINE_COLUMN_LOOP_END_POS, NULL,
          TIMELINE_COLUMN_END_POS, NULL,
          TIMELINE_COLUMN_OBJ, m,
          -1);
      }
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      {
        MidiNote * mn = (MidiNote *) obj;

        midi_note_get_val_as_string (
          mn, name, 0);
        get_event_type_as_string (obj->type, type);
        gtk_list_store_append (store, iter);
        gtk_list_store_set (
          store, iter,
          MIDI_COLUMN_NAME, name,
          MIDI_COLUMN_PITCH, mn->val,
          MIDI_COLUMN_VELOCITY, mn->vel->vel,
          MIDI_COLUMN_START_POS, &obj->pos,
          MIDI_COLUMN_END_POS, &obj->end_pos,
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
        get_event_type_as_string (obj->type, type);
        gtk_list_store_append (store, iter);
        gtk_list_store_set (
          store, iter,
          CHORD_COLUMN_NAME, name,
          CHORD_COLUMN_START_POS, &obj->pos,
          CHORD_COLUMN_OBJ, c,
          -1);
      }
      break;
    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
      {
        AutomationPoint * ap =
          (AutomationPoint*) obj;

        get_event_type_as_string (obj->type, type);
        gtk_list_store_append (store, iter);
        gtk_list_store_set (
          store, iter,
          AUTOMATION_COLUMN_INDEX, ap->index,
          AUTOMATION_COLUMN_POS, &obj->pos,
          AUTOMATION_COLUMN_VALUE, ap->fvalue,
          AUTOMATION_COLUMN_CURVINESS,
          ap->curve_opts.curviness,
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
      /* name */
      G_TYPE_STRING,
      /* type */
      G_TYPE_STRING,
      /* start pos */
      G_TYPE_POINTER,
      /* clip start pos */
      G_TYPE_POINTER,
      /* loop start pos */
      G_TYPE_POINTER,
      /* loop end pos */
      G_TYPE_POINTER,
      /* end pos */
      G_TYPE_POINTER,
      /* object */
      G_TYPE_POINTER);

  /* add data to the list store (cheat by using
   * the timeline arranger children) */
  ArrangerObject * objs[2000];
  int              num_objs;
  ADD_FOREACH_IN_ARRANGER (MW_TIMELINE);
  ADD_FOREACH_IN_ARRANGER (MW_PINNED_TIMELINE);

  SET_SORT_POSITION_FUNC (
    TIMELINE_COLUMN_START_POS);
  SET_SORT_POSITION_FUNC (
    TIMELINE_COLUMN_CLIP_START_POS);
  SET_SORT_POSITION_FUNC (
    TIMELINE_COLUMN_LOOP_START_POS);
  SET_SORT_POSITION_FUNC (
    TIMELINE_COLUMN_LOOP_END_POS);
  SET_SORT_POSITION_FUNC (
    TIMELINE_COLUMN_END_POS);

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
      /* name */
      G_TYPE_STRING,
      /* pitch */
      G_TYPE_INT,
      /* velocity */
      G_TYPE_INT,
      /* start pos */
      G_TYPE_POINTER,
      /* end pos */
      G_TYPE_POINTER,
      /* object ptr */
      G_TYPE_POINTER);

  /* add data to the list */
  ArrangerObject * objs[2000];
  int              num_objs;
  ADD_FOREACH_IN_ARRANGER (
    MW_MIDI_ARRANGER);

  SET_SORT_POSITION_FUNC (
    MIDI_COLUMN_START_POS);
  SET_SORT_POSITION_FUNC (
    MIDI_COLUMN_END_POS);

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
      /* name */
      G_TYPE_STRING,
      /* position */
      G_TYPE_POINTER,
      G_TYPE_POINTER);

  /* add data to the list */
  ArrangerObject * objs[2000];
  int              num_objs;
  ADD_FOREACH_IN_ARRANGER (
    MW_CHORD_ARRANGER);

  SET_SORT_POSITION_FUNC (
    CHORD_COLUMN_START_POS);

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
      /* index */
      G_TYPE_INT,
      /* position */
      G_TYPE_POINTER,
      /* value */
      G_TYPE_FLOAT,
      /* curviness */
      G_TYPE_DOUBLE,
      /* object */
      G_TYPE_POINTER);

  /* add data to the list */
  ArrangerObject * objs[2000];
  int              num_objs;
  ADD_FOREACH_IN_ARRANGER (
    MW_AUTOMATION_ARRANGER);

  SET_SORT_POSITION_FUNC (
    AUTOMATION_COLUMN_POS);

  return GTK_TREE_MODEL (store);
}

static GtkTreeModel *
create_audio_model (
  EventViewerWidget * self)
{
  GtkListStore *store;
  GtkTreeIter iter;

  /* create list store */
  store =
    gtk_list_store_new (
      NUM_AUDIO_COLUMNS,
      /* start position */
      G_TYPE_POINTER,
      /* end position */
      G_TYPE_POINTER);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (
    store, &iter,
    AUDIO_COLUMN_START_POS,
    &AUDIO_SELECTIONS->sel_start,
    AUDIO_COLUMN_END_POS,
    &AUDIO_SELECTIONS->sel_end,
    -1);

  SET_SORT_POSITION_FUNC (
    AUDIO_COLUMN_START_POS);
  SET_SORT_POSITION_FUNC (
    AUDIO_COLUMN_END_POS);

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
      return create_audio_model (self);
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
  gtk_tree_view_column_set_reorderable (
    column, true);
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
  gtk_tree_view_column_set_reorderable (
    column, true);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for start pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Start"), renderer, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, TIMELINE_COLUMN_START_POS);
  SET_POSITION_PRINT_FUNC (
    TIMELINE_COLUMN_START_POS);
  gtk_tree_view_column_set_reorderable (
    column, true);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for clip start pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Clip start"), renderer, NULL);
  SET_POSITION_PRINT_FUNC (
    TIMELINE_COLUMN_CLIP_START_POS);
  gtk_tree_view_column_set_sort_column_id (
    column, TIMELINE_COLUMN_CLIP_START_POS);
  gtk_tree_view_column_set_reorderable (
    column, true);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for loop start pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Loop start"), renderer, NULL);
  SET_POSITION_PRINT_FUNC (
    TIMELINE_COLUMN_LOOP_START_POS);
  gtk_tree_view_column_set_sort_column_id (
    column, TIMELINE_COLUMN_LOOP_START_POS);
  gtk_tree_view_column_set_reorderable (
    column, true);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for loop end pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Loop end"), renderer, NULL);
  SET_POSITION_PRINT_FUNC (
    TIMELINE_COLUMN_LOOP_END_POS);
  gtk_tree_view_column_set_sort_column_id (
    column, TIMELINE_COLUMN_LOOP_END_POS);
  gtk_tree_view_column_set_reorderable (
    column, true);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for end pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("End"), renderer, NULL);
  SET_POSITION_PRINT_FUNC (
    TIMELINE_COLUMN_END_POS);
  gtk_tree_view_column_set_sort_column_id (
    column, TIMELINE_COLUMN_END_POS);
  gtk_tree_view_column_set_reorderable (
    column, true);
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
  gtk_tree_view_column_set_reorderable (
    column, true);
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
  gtk_tree_view_column_set_reorderable (
    column, true);
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
  gtk_tree_view_column_set_reorderable (
    column, true);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for start pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Start"), renderer, NULL);
  SET_POSITION_PRINT_FUNC (
    MIDI_COLUMN_START_POS);
  gtk_tree_view_column_set_sort_column_id (
    column, MIDI_COLUMN_START_POS);
  gtk_tree_view_column_set_reorderable (
    column, true);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for end pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("End"), renderer, NULL);
  SET_POSITION_PRINT_FUNC (
    MIDI_COLUMN_END_POS);
  gtk_tree_view_column_set_sort_column_id (
    column, MIDI_COLUMN_END_POS);
  gtk_tree_view_column_set_reorderable (
    column, true);
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
  gtk_tree_view_column_set_reorderable (
    column, true);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for start pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Position"), renderer, NULL);
  SET_POSITION_PRINT_FUNC (
    CHORD_COLUMN_START_POS);
  gtk_tree_view_column_set_sort_column_id (
    column, CHORD_COLUMN_START_POS);
  gtk_tree_view_column_set_reorderable (
    column, true);
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
  gtk_tree_view_column_set_reorderable (
    column, true);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for start pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Position"), renderer, NULL);
  SET_POSITION_PRINT_FUNC (
    AUTOMATION_COLUMN_POS);
  gtk_tree_view_column_set_sort_column_id (
    column, AUTOMATION_COLUMN_POS);
  gtk_tree_view_column_set_reorderable (
    column, true);
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
  gtk_tree_view_column_set_reorderable (
    column, true);
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
  gtk_tree_view_column_set_reorderable (
    column, true);
  gtk_tree_view_append_column (
    self->treeview, column);
}

static void
append_audio_columns (
  EventViewerWidget * self)
{
  GtkCellRenderer * renderer;
  GtkTreeViewColumn * column;

  /* column for start pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Selection start"), renderer, NULL);
  SET_POSITION_PRINT_FUNC (
    AUDIO_COLUMN_START_POS);
  gtk_tree_view_column_set_sort_column_id (
    column, AUDIO_COLUMN_START_POS);
  gtk_tree_view_column_set_reorderable (
    column, true);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for end pos */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Selection end"), renderer, NULL);
  SET_POSITION_PRINT_FUNC (
    AUDIO_COLUMN_END_POS);
  gtk_tree_view_column_set_sort_column_id (
    column, AUDIO_COLUMN_END_POS);
  gtk_tree_view_column_set_reorderable (
    column, true);
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
      append_audio_columns (self);
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

static ArrangerSelections *
get_arranger_selections (
  EventViewerWidget * self)
{
  if (self->type == EVENT_VIEWER_TYPE_TIMELINE)
    {
      return (ArrangerSelections *) TL_SELECTIONS;
    }
  else
    {
      ZRegion * r =
        clip_editor_get_region (CLIP_EDITOR);
      if (!r)
        return NULL;

      switch (r->id.type)
        {
        case REGION_TYPE_MIDI:
          return (ArrangerSelections *) MA_SELECTIONS;
        case REGION_TYPE_AUDIO:
          return
            (ArrangerSelections *) AUDIO_SELECTIONS;
        case REGION_TYPE_AUTOMATION:
          return
            (ArrangerSelections *)
            AUTOMATION_SELECTIONS;
        case REGION_TYPE_CHORD:
          return
            (ArrangerSelections *) CHORD_SELECTIONS;
        }
    }

  g_return_val_if_reached (NULL);
}

static int
get_obj_column (
  EventViewerWidget * self)
{
  if (self->type == EVENT_VIEWER_TYPE_TIMELINE)
    {
      return TIMELINE_COLUMN_OBJ;
    }
  else
    {
      ZRegion * r =
        clip_editor_get_region (CLIP_EDITOR);
      if (!r)
        return -1;

      switch (r->id.type)
        {
        case REGION_TYPE_MIDI:
          return MIDI_COLUMN_OBJ;
        case REGION_TYPE_AUDIO:
          return -1;
        case REGION_TYPE_AUTOMATION:
          return AUTOMATION_COLUMN_OBJ;
        case REGION_TYPE_CHORD:
          return CHORD_COLUMN_OBJ;
        }
    }

  g_return_val_if_reached (-1);
}

static void
mark_selected_objects_as_selected (
  EventViewerWidget * self)
{
  ArrangerSelections * sel =
    get_arranger_selections (self);;
  int obj_column = get_obj_column (self);;
  if (!sel || obj_column < 0)
    return;

  self->marking_selected_objs = true;

  GtkTreeSelection * selection =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->treeview));
  gtk_tree_selection_unselect_all (selection);
  int num_objs = 0;
  ArrangerObject ** objs =
    arranger_selections_get_all_objects (
      sel, &num_objs);
  GtkTreeIter iter;
  bool has_iter =
    gtk_tree_model_get_iter_first (
      GTK_TREE_MODEL (self->model), &iter);
  while (has_iter)
    {
      ArrangerObject * iter_obj = NULL;
      gtk_tree_model_get (
        GTK_TREE_MODEL (self->model), &iter,
        obj_column, &iter_obj, -1);
      g_return_if_fail (iter_obj);

      for (int i = 0; i < num_objs; i++)
        {
          ArrangerObject * obj = objs[i];
          if (obj != iter_obj)
            continue;

          gtk_tree_selection_select_iter (
            selection, &iter);
        }

      has_iter =
        gtk_tree_model_iter_next (
          GTK_TREE_MODEL (self->model), &iter);
    }
  free (objs);

  self->marking_selected_objs = false;
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
  if (self->type == EVENT_VIEWER_TYPE_TIMELINE
      &&
      !g_settings_get_boolean (
         S_UI, "timeline-event-viewer-visible"))
    return;
  if (self->type == EVENT_VIEWER_TYPE_EDITOR
      &&
      !g_settings_get_boolean (
         S_UI, "editor-event-viewer-visible"))
    return;

  switch (self->type)
    {
    case EVENT_VIEWER_TYPE_TIMELINE:
      refresh_timeline_events (self);
      break;
    case EVENT_VIEWER_TYPE_EDITOR:
      refresh_editor_events (self);
      break;
    }

  GtkTreeSelection * selection =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->treeview));
  gtk_tree_selection_set_mode (
    selection, GTK_SELECTION_MULTIPLE);

  mark_selected_objects_as_selected (self);
}

/**
 * Convenience function.
 */
void
event_viewer_widget_refresh_for_selections (
  ArrangerSelections * sel)
{
  if (sel->type == ARRANGER_SELECTIONS_TYPE_TIMELINE)
    event_viewer_widget_refresh (
      MW_TIMELINE_EVENT_VIEWER);
  else
    event_viewer_widget_refresh (
      MW_EDITOR_EVENT_VIEWER);
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
    case ARRANGER_WIDGET_TYPE_MIDI_MODIFIER:
    case ARRANGER_WIDGET_TYPE_CHORD:
    case ARRANGER_WIDGET_TYPE_AUTOMATION:
    case ARRANGER_WIDGET_TYPE_AUDIO:
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}

static int
selection_func (
  GtkTreeSelection * selection,
  GtkTreeModel *     model,
  GtkTreePath *      path,
  int                path_currently_selected,
  void *             data)
{
  EventViewerWidget * self =
    (EventViewerWidget *) data;

  GtkTreeIter iter;
  bool has_any =
    gtk_tree_model_get_iter (model, &iter, path);
  g_return_val_if_fail (has_any, false);

  /* select object if selection is not made
   * programmatically */
  if (!self->marking_selected_objs)
    {
      int obj_column = get_obj_column (self);
      if (obj_column >= 0)
        {
          ArrangerObject * obj = NULL;
          gtk_tree_model_get (
            model, &iter, obj_column, &obj, -1);
          arranger_object_select (
            obj, !path_currently_selected, F_APPEND,
            F_PUBLISH_EVENTS);
        }
    }

  /* allow toggle */
  return true;
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

  GtkTreeSelection * sel =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->treeview));
  gtk_tree_selection_set_select_function (
    sel, selection_func, self, NULL);

  event_viewer_widget_refresh (self);
}

EventViewerWidget *
event_viewer_widget_new (void)
{
  EventViewerWidget * self =
    g_object_new (
      EVENT_VIEWER_WIDGET_TYPE, NULL);

  return self;
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
