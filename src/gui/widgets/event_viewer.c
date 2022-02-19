/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/backend/wrapped_object_with_change_signal.h"
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
#include "gui/widgets/item_factory.h"
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
  TIMELINE_COLUMN_FADE_IN_POS,
  TIMELINE_COLUMN_FADE_OUT_POS,
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

#if 0
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
#endif

typedef struct FuncData
{
  /** Column ID. */
  int                 column;
} FuncData;

#if 0
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
#endif

#if 0
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
#endif

static void
add_from_object (
  GPtrArray *      arr,
  ArrangerObject * obj)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    wrapped_object_with_change_signal_new (
      obj, WRAPPED_OBJECT_TYPE_ARRANGER_OBJECT);
  g_ptr_array_add (arr, wrapped_obj);
#if 0
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
          TIMELINE_COLUMN_FADE_IN_POS,
          &obj->fade_in_pos,
          TIMELINE_COLUMN_FADE_OUT_POS,
          &obj->fade_out_pos,
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
          TIMELINE_COLUMN_FADE_IN_POS, NULL,
          TIMELINE_COLUMN_FADE_OUT_POS, NULL,
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
#endif
}

/**
 * Removes all items and re-populates the list
 * store.
 */
static void
splice_list_store (
  GListStore * store,
  GPtrArray *  ptr_array)
{
  size_t num_objs;
  gpointer * objs =
    g_ptr_array_steal (ptr_array, &num_objs);
  g_list_store_splice (
    store, 0,
    g_list_model_get_n_items (
      G_LIST_MODEL (store)),
    objs, num_objs);
  g_free (objs);
}

#define ADD_FOREACH_IN_ARRANGER(arranger) \
  arranger_widget_get_all_objects ( \
    arranger, objs, &num_objs); \
  for (int i = 0; i < num_objs; i++) \
    { \
      ArrangerObject * obj = objs[i]; \
      add_from_object (ptr_array, obj); \
    }

static void
refresh_timeline_model (
  EventViewerWidget * self)
{
  GListStore * store =
    z_gtk_column_view_get_list_store (
      self->column_view);

  GPtrArray * ptr_array = g_ptr_array_new ();

  /* add data to the list store (cheat by using
   * the timeline arranger children) */
  ArrangerObject * objs[2000];
  int              num_objs;
  ADD_FOREACH_IN_ARRANGER (MW_TIMELINE);
  ADD_FOREACH_IN_ARRANGER (MW_PINNED_TIMELINE);

  splice_list_store (store, ptr_array);

#if 0
  SET_SORT_POSITION_FUNC (
    TIMELINE_COLUMN_START_POS);
  SET_SORT_POSITION_FUNC (
    TIMELINE_COLUMN_CLIP_START_POS);
  SET_SORT_POSITION_FUNC (
    TIMELINE_COLUMN_LOOP_START_POS);
  SET_SORT_POSITION_FUNC (
    TIMELINE_COLUMN_LOOP_END_POS);
  SET_SORT_POSITION_FUNC (
    TIMELINE_COLUMN_FADE_IN_POS);
  SET_SORT_POSITION_FUNC (
    TIMELINE_COLUMN_FADE_OUT_POS);
  SET_SORT_POSITION_FUNC (
    TIMELINE_COLUMN_END_POS);
#endif
}

static void
refresh_midi_model (
  EventViewerWidget * self)
{
  GListStore * store =
    z_gtk_column_view_get_list_store (
      self->column_view);

  GPtrArray * ptr_array = g_ptr_array_new ();

  /* add data to the list */
  ArrangerObject * objs[2000];
  int              num_objs;
  ADD_FOREACH_IN_ARRANGER (
    MW_MIDI_ARRANGER);

  splice_list_store (store, ptr_array);

#if 0
  SET_SORT_POSITION_FUNC (
    MIDI_COLUMN_START_POS);
  SET_SORT_POSITION_FUNC (
    MIDI_COLUMN_END_POS);
#endif
}

static void
refresh_chord_model (
  EventViewerWidget * self)
{
  GListStore * store =
    z_gtk_column_view_get_list_store (
      self->column_view);

  GPtrArray * ptr_array = g_ptr_array_new ();

  /* add data to the list */
  ArrangerObject * objs[2000];
  int              num_objs;
  ADD_FOREACH_IN_ARRANGER (
    MW_CHORD_ARRANGER);

  splice_list_store (store, ptr_array);

#if 0
  SET_SORT_POSITION_FUNC (
    CHORD_COLUMN_START_POS);
#endif
}

static void
refresh_automation_model (
  EventViewerWidget * self)
{
  GListStore * store =
    z_gtk_column_view_get_list_store (
      self->column_view);

  GPtrArray * ptr_array = g_ptr_array_new ();

  /* add data to the list */
  ArrangerObject * objs[2000];
  int              num_objs;
  ADD_FOREACH_IN_ARRANGER (
    MW_AUTOMATION_ARRANGER);

  splice_list_store (store, ptr_array);

#if 0
  SET_SORT_POSITION_FUNC (
    AUTOMATION_COLUMN_POS);
#endif
}

static void
refresh_audio_model (
  EventViewerWidget * self)
{
  GListStore * store =
    z_gtk_column_view_get_list_store (
      self->column_view);

  GPtrArray * ptr_array = g_ptr_array_new ();

#if 0
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (
    store, &iter,
    AUDIO_COLUMN_START_POS,
    &AUDIO_SELECTIONS->sel_start,
    AUDIO_COLUMN_END_POS,
    &AUDIO_SELECTIONS->sel_end,
    -1);
#endif

  splice_list_store (store, ptr_array);

#if 0
  SET_SORT_POSITION_FUNC (
    AUDIO_COLUMN_START_POS);
  SET_SORT_POSITION_FUNC (
    AUDIO_COLUMN_END_POS);
#endif
}

static void
refresh_editor_model (
  EventViewerWidget * self)
{
  switch (self->type)
    {
    case EVENT_VIEWER_TYPE_MIDI:
      refresh_midi_model (self);
      break;
    case EVENT_VIEWER_TYPE_AUDIO:
      refresh_audio_model (self);
      break;
    case EVENT_VIEWER_TYPE_AUTOMATION:
      refresh_automation_model (self);
      break;
    case EVENT_VIEWER_TYPE_CHORD:
      refresh_chord_model (self);
      break;
    default:
      g_return_if_reached ();
    }
}

#undef ADD_FOREACH_IN_ARRANGER

static ArrangerSelections *
get_arranger_selections (
  EventViewerWidget * self)
{
  switch (self->type)
    {
    case EVENT_VIEWER_TYPE_TIMELINE:
      return (ArrangerSelections *) TL_SELECTIONS;
    case EVENT_VIEWER_TYPE_MIDI:
      return (ArrangerSelections *) MA_SELECTIONS;
    case EVENT_VIEWER_TYPE_AUDIO:
      return
        (ArrangerSelections *) AUDIO_SELECTIONS;
    case EVENT_VIEWER_TYPE_CHORD:
      return
        (ArrangerSelections *) CHORD_SELECTIONS;
    case EVENT_VIEWER_TYPE_AUTOMATION:
      return
        (ArrangerSelections *)
        AUTOMATION_SELECTIONS;
    }

  g_return_val_if_reached (NULL);
}

static void
mark_selected_objects_as_selected (
  EventViewerWidget * self)
{
  ArrangerSelections * sel =
    get_arranger_selections (self);
  if (!sel)
    return;

  self->marking_selected_objs = true;

  GtkSelectionModel * sel_model =
    gtk_column_view_get_model (self->column_view);
  gtk_selection_model_unselect_all (sel_model);
  int num_objs = 0;
  ArrangerObject ** objs =
    arranger_selections_get_all_objects (
      sel, &num_objs);
  GListModel * list = G_LIST_MODEL (sel_model);
  guint num_items =
    g_list_model_get_n_items (list);
  for (guint i = 0; i < num_items; i++)
    {
      WrappedObjectWithChangeSignal * wrapped_obj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
          g_list_model_get_object (list, i));
      ArrangerObject * iter_obj =
        (ArrangerObject *) wrapped_obj->obj;

      for (int j = 0; j < num_objs; j++)
        {
          ArrangerObject * obj = objs[j];
          if (obj != iter_obj)
            continue;

          gtk_selection_model_select_item (
            sel_model, i, false);
          break;
        }
    }
  free (objs);

  self->marking_selected_objs = false;
}

/**
 * Called to update the models/selections.
 *
 * @param selections_only Only update the selection
 *   status of each item without repopulating the
 *   model.
 */
void
event_viewer_widget_refresh (
  EventViewerWidget * self,
  bool                selections_only)
{
  if (!selections_only)
    {
      switch (self->type)
        {
        case EVENT_VIEWER_TYPE_TIMELINE:
          if (!g_settings_get_boolean (
                 S_UI,
                 "timeline-event-viewer-visible"))
            return;

          refresh_timeline_model (self);
          break;
        case EVENT_VIEWER_TYPE_MIDI:
        case EVENT_VIEWER_TYPE_AUDIO:
        case EVENT_VIEWER_TYPE_CHORD:
        case EVENT_VIEWER_TYPE_AUTOMATION:
          if (!g_settings_get_boolean (
                 S_UI, "editor-event-viewer-visible"))
            return;

          refresh_editor_model (self);
          break;
        }

      ArrangerSelections * sel =
        get_arranger_selections (self);
      if (sel)
        {
          object_free_w_func_and_null (
            arranger_selections_free,
            self->last_selections);
          self->last_selections =
            arranger_selections_new (sel->type);
          int num_objs;
          ArrangerObject ** objs =
            arranger_selections_get_all_objects (
              sel, &num_objs);
          for (int i = 0; i < num_objs; i++)
            {
              arranger_selections_add_object (
                self->last_selections, objs[i]);
            }
        }
    }

  mark_selected_objects_as_selected (self);
}

/**
 * Convenience function.
 */
void
event_viewer_widget_refresh_for_selections (
  ArrangerSelections * sel)
{
  EventViewerWidget * self;
  switch (sel->type)
    {
    case ARRANGER_SELECTIONS_TYPE_TIMELINE:
      self = MW_TIMELINE_EVENT_VIEWER;
      break;
    case ARRANGER_SELECTIONS_TYPE_CHORD:
      self = MW_BOT_DOCK_EDGE->event_viewer_chord;
      break;
    case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
      self =
        MW_BOT_DOCK_EDGE->event_viewer_automation;
      break;
    case ARRANGER_SELECTIONS_TYPE_AUDIO:
      self = MW_BOT_DOCK_EDGE->event_viewer_audio;
      break;
    case ARRANGER_SELECTIONS_TYPE_MIDI:
      self = MW_BOT_DOCK_EDGE->event_viewer_midi;
      break;
    default:
      g_return_if_reached ();
    }

  bool need_model_refresh = false;
  if (self->last_selections)
    {
      int num_objs =
        arranger_selections_get_num_objects (sel);
      int num_cached_objs =
        arranger_selections_get_num_objects (
          self->last_selections);
      if (num_objs != num_cached_objs)
        need_model_refresh = true;
      else
        {
          ArrangerObject ** objs =
            arranger_selections_get_all_objects (
              sel, &num_objs);
          ArrangerObject ** cached_objs =
            arranger_selections_get_all_objects (
              self->last_selections,
              &num_cached_objs);
          for (int i = 0; i < num_objs; i++)
            {
              ArrangerObject * a = objs[i];
              ArrangerObject * b = cached_objs[i];
              if (a != b)
                {
                  need_model_refresh = true;
                  break;
                }
            }
        }
    }
  else
    need_model_refresh = true;

  event_viewer_widget_refresh (
    self, !need_model_refresh);
}

/**
 * Convenience function.
 *
 * @param selections_only Only update the selection
 *   status of each item without repopulating the
 *   model.
 */
void
event_viewer_widget_refresh_for_arranger (
  const ArrangerWidget * arranger,
  bool                   selections_only)
{
  switch (arranger->type)
    {
    case ARRANGER_WIDGET_TYPE_TIMELINE:
      event_viewer_widget_refresh (
        MW_TIMELINE_EVENT_VIEWER,
        selections_only);
      break;
    case ARRANGER_WIDGET_TYPE_MIDI:
    case ARRANGER_WIDGET_TYPE_MIDI_MODIFIER:
      event_viewer_widget_refresh (
        MW_BOT_DOCK_EDGE->event_viewer_midi,
        selections_only);
      break;
    case ARRANGER_WIDGET_TYPE_CHORD:
      event_viewer_widget_refresh (
        MW_BOT_DOCK_EDGE->event_viewer_chord,
        selections_only);
      break;
    case ARRANGER_WIDGET_TYPE_AUTOMATION:
      event_viewer_widget_refresh (
        MW_BOT_DOCK_EDGE->event_viewer_automation,
        selections_only);
      break;
    case ARRANGER_WIDGET_TYPE_AUDIO:
      event_viewer_widget_refresh (
        MW_BOT_DOCK_EDGE->event_viewer_audio,
        selections_only);
      break;
    default:
      g_return_if_reached ();
      break;
    }
}

#if 0
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
#endif

static void
add_timeline_columns (
  EventViewerWidget * self)
{
  /* remove existing columns and factories */
  z_gtk_column_view_remove_all_columnes (
    self->column_view);
  g_ptr_array_remove_range (
    self->item_factories, 0,
    self->item_factories->len);

  /* column for name */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_EDITABLE, _("Name"));

  /* column for type */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_NOT_EDITABLE,
    _("Type"));

  /* column for start pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    _("Start"));

  /* column for clip start pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    _("Clip start"));

  /* column for loop start pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    _("Loop start"));

  /* column for loop end pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    _("Loop end"));

  /* column for fade in pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    _("Fade in"));

  /* column for fade out pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    _("Fade out"));

  /* column for end pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    _("End"));
}

static void
append_midi_columns (
  EventViewerWidget * self)
{
  /* column for note name */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_NOT_EDITABLE,
    _("Note"));

  /* column for pitch */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_INTEGER, Z_F_EDITABLE,
    _("Pitch"));

  /* column for velocity */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_INTEGER, Z_F_EDITABLE,
    _("Velocity"));

  /* column for start pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    _("Start"));

  /* column for end pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    _("End"));
}

static void
append_chord_columns (
  EventViewerWidget * self)
{
  /* column for name */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_NOT_EDITABLE,
    _("Name"));

  /* column for start pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    _("Position"));
}

static void
append_automation_columns (
  EventViewerWidget * self)
{
  /* column for index */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_INTEGER, Z_F_NOT_EDITABLE,
    _("Index"));

  /* column for start pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    _("Position"));

  /* column for value */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_EDITABLE,
    _("Value"));

  /* column for curviness */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_NOT_EDITABLE,
    _("Curve type"));

  /* column for curviness */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_EDITABLE,
    _("Curviness"));
}

static void
append_audio_columns (
  EventViewerWidget * self)
{
  /* column for start pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    _("Start"));

  /* column for end pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    _("End"));
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

  /* TODO */
#if 0
  GtkTreeSelection * sel =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->treeview));
  gtk_tree_selection_set_select_function (
    sel, selection_func, self, NULL);
#endif

  switch (self->type)
    {
    case EVENT_VIEWER_TYPE_TIMELINE:
      add_timeline_columns (self);
      break;
    case EVENT_VIEWER_TYPE_MIDI:
      append_midi_columns (self);
      break;
    case EVENT_VIEWER_TYPE_AUDIO:
      append_audio_columns (self);
      break;
    case EVENT_VIEWER_TYPE_CHORD:
      append_chord_columns (self);
      break;
    case EVENT_VIEWER_TYPE_AUTOMATION:
      append_automation_columns (self);
      break;
    }

  event_viewer_widget_refresh (self, false);
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
event_viewer_finalize (
  EventViewerWidget * self)
{
  g_ptr_array_unref (self->item_factories);

  object_free_w_func_and_null (
    arranger_selections_free,
    self->last_selections);

  G_OBJECT_CLASS (
    event_viewer_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
event_viewer_widget_init (
  EventViewerWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->region_type = -1;

  self->item_factories =
    g_ptr_array_new_with_free_func (
      item_factory_free_func);

  GListStore * store =
    g_list_store_new (
      WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  GtkMultiSelection * sel =
    GTK_MULTI_SELECTION (
      gtk_multi_selection_new (
        G_LIST_MODEL (store)));
  gtk_column_view_set_model (
    self->column_view, GTK_SELECTION_MODEL (sel));
}


static void
event_viewer_widget_class_init (
  EventViewerWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "event_viewer.ui");
  gtk_widget_class_set_css_name (
    klass, "event-viewer");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, EventViewerWidget, x)

  BIND_CHILD (column_view);

#undef BIND_CHILD

  GObjectClass * oklass =
    G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) event_viewer_finalize;
}
