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

static void
add_from_object (
  GPtrArray *      arr,
  ArrangerObject * obj)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    wrapped_object_with_change_signal_new (
      obj, WRAPPED_OBJECT_TYPE_ARRANGER_OBJECT);
  g_ptr_array_add (arr, wrapped_obj);
}

#define ADD_FOREACH_IN_ARRANGER(arranger) \
  { \
    GPtrArray * objs_arr = \
      g_ptr_array_new_full (200, NULL); \
    arranger_widget_get_all_objects ( \
      arranger, objs_arr); \
    for (size_t i = 0; i < objs_arr->len; i++) \
      { \
        ArrangerObject * obj = (ArrangerObject *) \
          g_ptr_array_index (objs_arr, i); \
        add_from_object (ptr_array, obj); \
      } \
  }

static void
refresh_timeline_model (EventViewerWidget * self)
{
  GListStore * store =
    z_gtk_column_view_get_list_store (
      self->column_view);

  GPtrArray * ptr_array = g_ptr_array_new ();
  ADD_FOREACH_IN_ARRANGER (MW_TIMELINE);
  ADD_FOREACH_IN_ARRANGER (MW_PINNED_TIMELINE);

  z_gtk_list_store_splice (store, ptr_array);
}

static void
refresh_midi_model (EventViewerWidget * self)
{
  GListStore * store =
    z_gtk_column_view_get_list_store (
      self->column_view);

  GPtrArray * ptr_array = g_ptr_array_new ();
  ADD_FOREACH_IN_ARRANGER (MW_MIDI_ARRANGER);

  z_gtk_list_store_splice (store, ptr_array);
}

static void
refresh_chord_model (EventViewerWidget * self)
{
  GListStore * store =
    z_gtk_column_view_get_list_store (
      self->column_view);

  GPtrArray * ptr_array = g_ptr_array_new ();
  ADD_FOREACH_IN_ARRANGER (MW_CHORD_ARRANGER);

  z_gtk_list_store_splice (store, ptr_array);
}

static void
refresh_automation_model (EventViewerWidget * self)
{
  GListStore * store =
    z_gtk_column_view_get_list_store (
      self->column_view);

  GPtrArray * ptr_array = g_ptr_array_new ();
  ADD_FOREACH_IN_ARRANGER (MW_AUTOMATION_ARRANGER);

  z_gtk_list_store_splice (store, ptr_array);
}

#undef ADD_FOREACH_IN_ARRANGER

static void
refresh_audio_model (EventViewerWidget * self)
{
  GListStore * store =
    z_gtk_column_view_get_list_store (
      self->column_view);

  GPtrArray * ptr_array = g_ptr_array_new ();

  z_gtk_list_store_splice (store, ptr_array);
}

static void
refresh_editor_model (EventViewerWidget * self)
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

static ArrangerSelections *
get_arranger_selections (EventViewerWidget * self)
{
  switch (self->type)
    {
    case EVENT_VIEWER_TYPE_TIMELINE:
      return (ArrangerSelections *) TL_SELECTIONS;
    case EVENT_VIEWER_TYPE_MIDI:
      return (ArrangerSelections *) MA_SELECTIONS;
    case EVENT_VIEWER_TYPE_AUDIO:
      return (
        ArrangerSelections *) AUDIO_SELECTIONS;
    case EVENT_VIEWER_TYPE_CHORD:
      return (
        ArrangerSelections *) CHORD_SELECTIONS;
    case EVENT_VIEWER_TYPE_AUTOMATION:
      return (ArrangerSelections *)
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
  GPtrArray * objs_arr = g_ptr_array_new ();
  arranger_selections_get_all_objects (
    sel, objs_arr);
  GListModel * list = G_LIST_MODEL (sel_model);
  guint num_items = g_list_model_get_n_items (list);
  for (guint i = 0; i < num_items; i++)
    {
      WrappedObjectWithChangeSignal * wrapped_obj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
          g_list_model_get_object (list, i));
      ArrangerObject * iter_obj =
        (ArrangerObject *) wrapped_obj->obj;

      for (size_t j = 0; j < objs_arr->len; j++)
        {
          ArrangerObject * obj = (ArrangerObject *)
            g_ptr_array_index (objs_arr, j);

          if (obj != iter_obj)
            continue;

          gtk_selection_model_select_item (
            sel_model, i, false);
          break;
        }
    }
  g_ptr_array_unref (objs_arr);

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
          GPtrArray * objs_arr = g_ptr_array_new ();
          arranger_selections_get_all_objects (
            sel, objs_arr);
          for (size_t i = 0; i < objs_arr->len; i++)
            {
              arranger_selections_add_object (
                self->last_selections,
                (ArrangerObject *)
                  g_ptr_array_index (objs_arr, i));
            }
          g_ptr_array_unref (objs_arr);
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
          GPtrArray * objs_arr = g_ptr_array_new ();
          arranger_selections_get_all_objects (
            sel, objs_arr);
          GPtrArray * cached_objs_arr =
            g_ptr_array_new ();
          arranger_selections_get_all_objects (
            self->last_selections, cached_objs_arr);
          for (size_t i = 0; i < objs_arr->len; i++)
            {
              ArrangerObject * a = (ArrangerObject *)
                g_ptr_array_index (objs_arr, i);
              ArrangerObject * b = (ArrangerObject *)
                g_ptr_array_index (
                  cached_objs_arr, i);
              if (a != b)
                {
                  need_model_refresh = true;
                  break;
                }
            }
          g_ptr_array_unref (objs_arr);
          g_ptr_array_unref (cached_objs_arr);
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
        MW_TIMELINE_EVENT_VIEWER, selections_only);
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

static char *
get_obj_name (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  ArrangerObject * obj =
    (ArrangerObject *) wrapped_obj->obj;

  return arranger_object_gen_human_readable_name (
    obj);
}

static char *
get_obj_type (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  ArrangerObject * obj =
    (ArrangerObject *) wrapped_obj->obj;

  const char * untranslated_type =
    arranger_object_stringize_type (obj->type);
  return g_strdup (_ (untranslated_type));
}

static double
get_obj_pos_dbl (void * data, void * param)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  ArrangerObject * obj =
    (ArrangerObject *) wrapped_obj->obj;

  ArrangerObjectPositionType pos_type =
    GPOINTER_TO_UINT (param);

  Position pos;
  arranger_object_get_position_from_type (
    obj, &pos, pos_type);

  return pos.ticks;
}

static guint
get_midi_note_pitch (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  MidiNote * mn = (MidiNote *) wrapped_obj->obj;

  return mn->val;
}

static guint
get_midi_note_velocity (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  MidiNote * mn = (MidiNote *) wrapped_obj->obj;

  return mn->vel->vel;
}

static int
get_automation_point_idx (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  AutomationPoint * ap =
    (AutomationPoint *) wrapped_obj->obj;

  return ap->index;
}

static float
get_automation_point_value (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  AutomationPoint * ap =
    (AutomationPoint *) wrapped_obj->obj;

  return ap->fvalue;
}

static double
get_automation_point_curviness (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  AutomationPoint * ap =
    (AutomationPoint *) wrapped_obj->obj;

  return ap->curve_opts.curviness;
}

static char *
get_automation_point_curve_type_str (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  AutomationPoint * ap =
    (AutomationPoint *) wrapped_obj->obj;

  char str[600];
  curve_algorithm_get_localized_name (
    ap->curve_opts.algo, str);

  return g_strdup (str);
}

static void
add_timeline_columns (EventViewerWidget * self)
{
  /* remove existing columns and factories */
  z_gtk_column_view_remove_all_columnes (
    self->column_view);
  g_ptr_array_remove_range (
    self->item_factories, 0,
    self->item_factories->len);

  GtkSorter *     sorter;
  GtkExpression * expression;

  /* column for name */
  expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, NULL, 0, NULL,
    G_CALLBACK (get_obj_name), NULL, NULL);
  sorter = GTK_SORTER (
    gtk_string_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_EDITABLE, Z_F_RESIZABLE,
    sorter, _ ("Name"));

  /* column for type */
  expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, NULL, 0, NULL,
    G_CALLBACK (get_obj_type), NULL, NULL);
  sorter = GTK_SORTER (
    gtk_string_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_NOT_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Type"));

  /* column for start pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, NULL, 0, NULL,
    G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (
      ARRANGER_OBJECT_POSITION_TYPE_START),
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Start"));

  /* column for clip start pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, NULL, 0, NULL,
    G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (
      ARRANGER_OBJECT_POSITION_TYPE_CLIP_START),
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Clip start"));

  /* column for loop start pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, NULL, 0, NULL,
    G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (
      ARRANGER_OBJECT_POSITION_TYPE_LOOP_START),
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Loop start"));

  /* column for loop end pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, NULL, 0, NULL,
    G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (
      ARRANGER_OBJECT_POSITION_TYPE_LOOP_END),
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Loop end"));

  /* column for fade in pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, NULL, 0, NULL,
    G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (
      ARRANGER_OBJECT_POSITION_TYPE_FADE_IN),
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Fade in"));

  /* column for fade out pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, NULL, 0, NULL,
    G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (
      ARRANGER_OBJECT_POSITION_TYPE_FADE_OUT),
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Fade out"));

  /* column for end pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, NULL, 0, NULL,
    G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (
      ARRANGER_OBJECT_POSITION_TYPE_END),
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("End"));
}

static void
append_midi_columns (EventViewerWidget * self)
{
  GtkSorter *     sorter;
  GtkExpression * expression;

  /* column for note name */
  expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, NULL, 0, NULL,
    G_CALLBACK (get_obj_name), NULL, NULL);
  sorter = GTK_SORTER (
    gtk_string_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_NOT_EDITABLE,
    Z_F_RESIZABLE, NULL, _ ("Note"));

  /* column for pitch */
  expression = gtk_cclosure_expression_new (
    G_TYPE_UINT, NULL, 0, NULL,
    G_CALLBACK (get_midi_note_pitch), NULL, NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_INTEGER, Z_F_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Pitch"));

  /* column for velocity */
  expression = gtk_cclosure_expression_new (
    G_TYPE_UINT, NULL, 0, NULL,
    G_CALLBACK (get_midi_note_velocity), NULL,
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_INTEGER, Z_F_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Velocity"));

  /* column for start pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, NULL, 0, NULL,
    G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (
      ARRANGER_OBJECT_POSITION_TYPE_START),
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Start"));

  /* column for end pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, NULL, 0, NULL,
    G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (
      ARRANGER_OBJECT_POSITION_TYPE_END),
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("End"));
}

static void
append_chord_columns (EventViewerWidget * self)
{
  GtkSorter *     sorter;
  GtkExpression * expression;

  /* column for name */
  expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, NULL, 0, NULL,
    G_CALLBACK (get_obj_name), NULL, NULL);
  sorter = GTK_SORTER (
    gtk_string_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_NOT_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Name"));

  /* column for start pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, NULL, 0, NULL,
    G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (
      ARRANGER_OBJECT_POSITION_TYPE_START),
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Position"));
}

static void
append_automation_columns (EventViewerWidget * self)
{
  GtkSorter *     sorter;
  GtkExpression * expression;

  /* column for index */
  expression = gtk_cclosure_expression_new (
    G_TYPE_INT, NULL, 0, NULL,
    G_CALLBACK (get_automation_point_idx), NULL,
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_INTEGER, Z_F_NOT_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Index"));

  /* column for start pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, NULL, 0, NULL,
    G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (
      ARRANGER_OBJECT_POSITION_TYPE_START),
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Position"));

  /* column for value */
  expression = gtk_cclosure_expression_new (
    G_TYPE_FLOAT, NULL, 0, NULL,
    G_CALLBACK (get_automation_point_value), NULL,
    NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_EDITABLE, Z_F_RESIZABLE,
    sorter, _ ("Value"));

  /* column for curve type */
  expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, NULL, 0, NULL,
    G_CALLBACK (get_automation_point_curve_type_str),
    NULL, NULL);
  sorter = GTK_SORTER (
    gtk_string_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_NOT_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Curve type"));

  /* column for curviness */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, NULL, 0, NULL,
    G_CALLBACK (get_automation_point_curviness),
    NULL, NULL);
  sorter = GTK_SORTER (
    gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_EDITABLE, Z_F_RESIZABLE,
    sorter, _ ("Curviness"));
}

static void
append_audio_columns (EventViewerWidget * self)
{
  /* column for start pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    Z_F_RESIZABLE, NULL, _ ("Start"));

  /* column for end pos */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_POSITION, Z_F_EDITABLE,
    Z_F_RESIZABLE, NULL, _ ("End"));
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
    g_object_new (EVENT_VIEWER_WIDGET_TYPE, NULL);

  return self;
}

static void
event_viewer_finalize (EventViewerWidget * self)
{
  g_ptr_array_unref (self->item_factories);

  object_free_w_func_and_null (
    arranger_selections_free,
    self->last_selections);

  G_OBJECT_CLASS (event_viewer_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
event_viewer_widget_init (EventViewerWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->region_type = -1;

  self->item_factories =
    g_ptr_array_new_with_free_func (
      item_factory_free_func);

  GListStore * store = g_list_store_new (
    WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  /* make sortable */
  GtkSorter * sorter =
    gtk_column_view_get_sorter (self->column_view);
  sorter = g_object_ref (sorter);
  GtkSortListModel * sort_list_model =
    gtk_sort_list_model_new (
      G_LIST_MODEL (store), sorter);

  GtkMultiSelection * sel =
    GTK_MULTI_SELECTION (gtk_multi_selection_new (
      G_LIST_MODEL (sort_list_model)));
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

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) event_viewer_finalize;
}
