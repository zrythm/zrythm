// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/region.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/arranger_wrapper.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
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
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (EventViewerWidget, event_viewer_widget, GTK_TYPE_BOX)

static void
add_from_object (
  std::vector<WrappedObjectWithChangeSignal *> &arr,
  ArrangerObject *                              obj)
{
  auto * wrapped_obj = wrapped_object_with_change_signal_new (
    obj, WrappedObjectType::WRAPPED_OBJECT_TYPE_ARRANGER_OBJECT);
  arr.push_back (wrapped_obj);
}

static void
add_foreach_in_arranger (
  ArrangerWidget *                              arranger,
  std::vector<WrappedObjectWithChangeSignal *> &wrapped_objs)
{
  std::vector<ArrangerObject *> objs;
  arranger_widget_get_all_objects (arranger, objs);
  for (auto obj : objs)
    {
      add_from_object (wrapped_objs, obj);
    }
}

static void
refresh_timeline_model (EventViewerWidget * self)
{
  GListStore * store = z_gtk_column_view_get_list_store (self->column_view);

  std::vector<WrappedObjectWithChangeSignal *> objs;
  add_foreach_in_arranger (MW_TIMELINE, objs);
  add_foreach_in_arranger (MW_PINNED_TIMELINE, objs);

  z_gtk_list_store_splice (store, objs);
}

static void
refresh_midi_model (EventViewerWidget * self)
{
  GListStore * store = z_gtk_column_view_get_list_store (self->column_view);

  std::vector<WrappedObjectWithChangeSignal *> objs;
  add_foreach_in_arranger (MW_MIDI_ARRANGER, objs);

  z_gtk_list_store_splice (store, objs);
}

static void
refresh_chord_model (EventViewerWidget * self)
{
  GListStore * store = z_gtk_column_view_get_list_store (self->column_view);

  std::vector<WrappedObjectWithChangeSignal *> objs;
  add_foreach_in_arranger (MW_CHORD_ARRANGER, objs);

  z_gtk_list_store_splice (store, objs);
}

static void
refresh_automation_model (EventViewerWidget * self)
{
  GListStore * store = z_gtk_column_view_get_list_store (self->column_view);

  std::vector<WrappedObjectWithChangeSignal *> objs;
  add_foreach_in_arranger (MW_AUTOMATION_ARRANGER, objs);

  z_gtk_list_store_splice (store, objs);
}

#undef ADD_FOREACH_IN_ARRANGER

static void
refresh_audio_model (EventViewerWidget * self)
{
  GListStore * store = z_gtk_column_view_get_list_store (self->column_view);

  std::vector<WrappedObjectWithChangeSignal *> objs;

  z_gtk_list_store_splice (store, objs);
}

static void
refresh_editor_model (EventViewerWidget * self)
{
  switch (self->type)
    {
    case EventViewerType::EVENT_VIEWER_TYPE_MIDI:
      refresh_midi_model (self);
      break;
    case EventViewerType::EVENT_VIEWER_TYPE_AUDIO:
      refresh_audio_model (self);
      break;
    case EventViewerType::EVENT_VIEWER_TYPE_AUTOMATION:
      refresh_automation_model (self);
      break;
    case EventViewerType::EVENT_VIEWER_TYPE_CHORD:
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
    case EventViewerType::EVENT_VIEWER_TYPE_TIMELINE:
      return TL_SELECTIONS.get ();
    case EventViewerType::EVENT_VIEWER_TYPE_MIDI:
      return MIDI_SELECTIONS.get ();
    case EventViewerType::EVENT_VIEWER_TYPE_AUDIO:
      return AUDIO_SELECTIONS.get ();
    case EventViewerType::EVENT_VIEWER_TYPE_CHORD:
      return CHORD_SELECTIONS.get ();
    case EventViewerType::EVENT_VIEWER_TYPE_AUTOMATION:
      return AUTOMATION_SELECTIONS.get ();
    }

  z_return_val_if_reached (nullptr);
}

static void
mark_selected_objects_as_selected (EventViewerWidget * self)
{
  ArrangerSelections * sel = get_arranger_selections (self);
  if (!sel)
    return;

  self->marking_selected_objs = true;

  GtkSelectionModel * sel_model = gtk_column_view_get_model (self->column_view);
  gtk_selection_model_unselect_all (sel_model);
  GListModel * list = G_LIST_MODEL (sel_model);
  guint        num_items = g_list_model_get_n_items (list);
  for (guint i = 0; i < num_items; i++)
    {
      WrappedObjectWithChangeSignal * wrapped_obj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (g_list_model_get_object (list, i));
      auto * iter_obj = (ArrangerObject *) wrapped_obj->obj;

      for (auto &obj : sel->objects_)
        {
          if (obj.get () != iter_obj)
            continue;

          gtk_selection_model_select_item (sel_model, i, false);
          break;
        }
    }

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
event_viewer_widget_refresh (EventViewerWidget * self, bool selections_only)
{
  if (!selections_only)
    {
      switch (self->type)
        {
        case EventViewerType::EVENT_VIEWER_TYPE_TIMELINE:
          if (!g_settings_get_boolean (S_UI, "timeline-event-viewer-visible"))
            return;

          refresh_timeline_model (self);
          break;
        case EventViewerType::EVENT_VIEWER_TYPE_MIDI:
        case EventViewerType::EVENT_VIEWER_TYPE_AUDIO:
        case EventViewerType::EVENT_VIEWER_TYPE_CHORD:
        case EventViewerType::EVENT_VIEWER_TYPE_AUTOMATION:
          if (!g_settings_get_boolean (S_UI, "editor-event-viewer-visible"))
            return;

          refresh_editor_model (self);
          break;
        }

      ArrangerSelections * sel = get_arranger_selections (self);
      if (sel)
        {
          self->last_selections = ArrangerSelections::new_from_type (sel->type_);
          for (auto &obj : sel->objects_)
            {
              self->last_selections->add_object_ref (obj);
            }
        }
    }

  mark_selected_objects_as_selected (self);
}

/**
 * Convenience function.
 */
void
event_viewer_widget_refresh_for_selections (ArrangerSelections * sel)
{
  EventViewerWidget * self;
  switch (sel->type_)
    {
    case ArrangerSelections::Type::Timeline:
      self = MW_TIMELINE_EVENT_VIEWER;
      break;
    case ArrangerSelections::Type::Chord:
      self = MW_BOT_DOCK_EDGE->event_viewer_chord;
      break;
    case ArrangerSelections::Type::Automation:
      self = MW_BOT_DOCK_EDGE->event_viewer_automation;
      break;
    case ArrangerSelections::Type::Audio:
      self = MW_BOT_DOCK_EDGE->event_viewer_audio;
      break;
    case ArrangerSelections::Type::Midi:
      self = MW_BOT_DOCK_EDGE->event_viewer_midi;
      break;
    default:
      g_return_if_reached ();
    }

  bool need_model_refresh = false;
  if (self->last_selections)
    {
      if (sel->objects_.size () != self->last_selections->objects_.size ())
        need_model_refresh = true;
      else
        {
          for (size_t i = 0; i < sel->objects_.size (); i++)
            {
              ArrangerObject * a = sel->objects_[i].get ();
              ArrangerObject * b = self->last_selections->objects_[i].get ();
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

  event_viewer_widget_refresh (self, !need_model_refresh);
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
    case ArrangerWidgetType::ARRANGER_WIDGET_TYPE_TIMELINE:
      event_viewer_widget_refresh (MW_TIMELINE_EVENT_VIEWER, selections_only);
      break;
    case ArrangerWidgetType::ARRANGER_WIDGET_TYPE_MIDI:
    case ArrangerWidgetType::ARRANGER_WIDGET_TYPE_MIDI_MODIFIER:
      event_viewer_widget_refresh (
        MW_BOT_DOCK_EDGE->event_viewer_midi, selections_only);
      break;
    case ArrangerWidgetType::ARRANGER_WIDGET_TYPE_CHORD:
      event_viewer_widget_refresh (
        MW_BOT_DOCK_EDGE->event_viewer_chord, selections_only);
      break;
    case ArrangerWidgetType::ARRANGER_WIDGET_TYPE_AUTOMATION:
      event_viewer_widget_refresh (
        MW_BOT_DOCK_EDGE->event_viewer_automation, selections_only);
      break;
    case ArrangerWidgetType::ARRANGER_WIDGET_TYPE_AUDIO:
      event_viewer_widget_refresh (
        MW_BOT_DOCK_EDGE->event_viewer_audio, selections_only);
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
          obj->select ( !path_currently_selected, F_APPEND,
            F_PUBLISH_EVENTS);
        }
    }

  /* allow toggle */
  return true;
}
#endif

static char *
get_obj_type (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  auto * obj = (ArrangerObject *) wrapped_obj->obj;

  const char * untranslated_type = obj->get_type_as_string (obj->type_);
  return g_strdup (_ (untranslated_type));
}

static double
get_obj_pos_dbl (void * data, void * param)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  auto * obj = (ArrangerObject *) wrapped_obj->obj;

  ArrangerObject::PositionType pos_type =
    ENUM_INT_TO_VALUE (ArrangerObject::PositionType, GPOINTER_TO_UINT (param));

  Position pos;
  obj->get_position_from_type (&pos, pos_type);

  return pos.ticks_;
}

static guint
get_midi_note_pitch (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  auto * mn = (MidiNote *) wrapped_obj->obj;

  return mn->val_;
}

static guint
get_midi_note_velocity (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  auto * mn = (MidiNote *) wrapped_obj->obj;

  return mn->vel_->vel_;
}

static int
get_automation_point_idx (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  auto * ap = (AutomationPoint *) wrapped_obj->obj;

  return ap->index_;
}

static float
get_automation_point_value (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  auto * ap = (AutomationPoint *) wrapped_obj->obj;

  return ap->fvalue_;
}

static double
get_automation_point_curviness (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  auto * ap = (AutomationPoint *) wrapped_obj->obj;

  return ap->curve_opts_.curviness_;
}

static char *
get_automation_point_curve_type_str (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  auto * ap = (AutomationPoint *) wrapped_obj->obj;
  return g_strdup (
    CurveOptions_Algorithm_to_string (ap->curve_opts_.algo_, true).c_str ());
}

static void
add_timeline_columns (EventViewerWidget * self)
{
  /* remove existing columns and factories */
  z_gtk_column_view_remove_all_columns (self->column_view);
  self->item_factories.clear ();

  GtkSorter *     sorter;
  GtkExpression * expression;

  /* column for name */
  expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, nullptr, 0, nullptr,
    G_CALLBACK (wrapped_object_with_change_signal_get_display_name), nullptr,
    nullptr);
  sorter = GTK_SORTER (gtk_string_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Text,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Name"));

  /* column for type */
  expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, nullptr, 0, nullptr, G_CALLBACK (get_obj_type), nullptr,
    nullptr);
  sorter = GTK_SORTER (gtk_string_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Text,
    Z_F_NOT_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Type"));

  /* column for start pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, nullptr, 0, nullptr, G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (ArrangerObject::PositionType::Start), nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Position,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Start"));

  /* column for clip start pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, nullptr, 0, nullptr, G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (ArrangerObject::PositionType::ClipStart), nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Position,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Clip start"));

  /* column for loop start pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, nullptr, 0, nullptr, G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (ArrangerObject::PositionType::LoopStart), nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Position,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Loop start"));

  /* column for loop end pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, nullptr, 0, nullptr, G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (ArrangerObject::PositionType::LoopEnd), nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Position,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Loop end"));

  /* column for fade in pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, nullptr, 0, nullptr, G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (ArrangerObject::PositionType::FadeIn), nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Position,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Fade in"));

  /* column for fade out pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, nullptr, 0, nullptr, G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (ArrangerObject::PositionType::FadeOut), nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Position,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Fade out"));

  /* column for end pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, nullptr, 0, nullptr, G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (ArrangerObject::PositionType::End), nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Position,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("End"));
}

static void
append_midi_columns (EventViewerWidget * self)
{
  GtkSorter *     sorter;
  GtkExpression * expression;

  /* column for note name */
  expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, nullptr, 0, nullptr,
    G_CALLBACK (wrapped_object_with_change_signal_get_display_name), nullptr,
    nullptr);
  sorter = GTK_SORTER (gtk_string_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Text,
    Z_F_NOT_EDITABLE, Z_F_RESIZABLE, nullptr, _ ("Note"));

  /* column for pitch */
  expression = gtk_cclosure_expression_new (
    G_TYPE_UINT, nullptr, 0, nullptr, G_CALLBACK (get_midi_note_pitch), nullptr,
    nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Integer,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Pitch"));

  /* column for velocity */
  expression = gtk_cclosure_expression_new (
    G_TYPE_UINT, nullptr, 0, nullptr, G_CALLBACK (get_midi_note_velocity),
    nullptr, nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Integer,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Velocity"));

  /* column for start pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, nullptr, 0, nullptr, G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (ArrangerObject::PositionType::Start), nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Position,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Start"));

  /* column for end pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, nullptr, 0, nullptr, G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (ArrangerObject::PositionType::End), nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Position,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("End"));
}

static void
append_chord_columns (EventViewerWidget * self)
{
  GtkSorter *     sorter;
  GtkExpression * expression;

  /* column for name */
  expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, nullptr, 0, nullptr,
    G_CALLBACK (wrapped_object_with_change_signal_get_display_name), nullptr,
    nullptr);
  sorter = GTK_SORTER (gtk_string_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Text,
    Z_F_NOT_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Name"));

  /* column for start pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, nullptr, 0, nullptr, G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (ArrangerObject::PositionType::Start), nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Position,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Position"));
}

static void
append_automation_columns (EventViewerWidget * self)
{
  GtkSorter *     sorter;
  GtkExpression * expression;

  /* column for index */
  expression = gtk_cclosure_expression_new (
    G_TYPE_INT, nullptr, 0, nullptr, G_CALLBACK (get_automation_point_idx),
    nullptr, nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Integer,
    Z_F_NOT_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Index"));

  /* column for start pos */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, nullptr, 0, nullptr, G_CALLBACK (get_obj_pos_dbl),
    GUINT_TO_POINTER (ArrangerObject::PositionType::Start), nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Position,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Position"));

  /* column for value */
  expression = gtk_cclosure_expression_new (
    G_TYPE_FLOAT, nullptr, 0, nullptr, G_CALLBACK (get_automation_point_value),
    nullptr, nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Text,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Value"));

  /* column for curve type */
  expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, nullptr, 0, nullptr,
    G_CALLBACK (get_automation_point_curve_type_str), nullptr, nullptr);
  sorter = GTK_SORTER (gtk_string_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Text,
    Z_F_NOT_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Curve type"));

  /* column for curviness */
  expression = gtk_cclosure_expression_new (
    G_TYPE_DOUBLE, nullptr, 0, nullptr,
    G_CALLBACK (get_automation_point_curviness), nullptr, nullptr);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Text,
    Z_F_EDITABLE, Z_F_RESIZABLE, sorter, _ ("Curviness"));
}

static void
append_audio_columns (EventViewerWidget * self)
{
  /* column for start pos */
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Position,
    Z_F_EDITABLE, Z_F_RESIZABLE, nullptr, _ ("Start"));

  /* column for end pos */
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Position,
    Z_F_EDITABLE, Z_F_RESIZABLE, nullptr, _ ("End"));
}

/**
 * Sets up the event viewer.
 */
void
event_viewer_widget_setup (EventViewerWidget * self, EventViewerType type)
{
  self->type = type;

  /* TODO */
#if 0
  GtkTreeSelection * sel =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->treeview));
  gtk_tree_selection_set_select_function (
    sel, selection_func, self, nullptr);
#endif

  switch (self->type)
    {
    case EventViewerType::EVENT_VIEWER_TYPE_TIMELINE:
      add_timeline_columns (self);
      break;
    case EventViewerType::EVENT_VIEWER_TYPE_MIDI:
      append_midi_columns (self);
      break;
    case EventViewerType::EVENT_VIEWER_TYPE_AUDIO:
      append_audio_columns (self);
      break;
    case EventViewerType::EVENT_VIEWER_TYPE_CHORD:
      append_chord_columns (self);
      break;
    case EventViewerType::EVENT_VIEWER_TYPE_AUTOMATION:
      append_automation_columns (self);
      break;
    }

  event_viewer_widget_refresh (self, false);
}

EventViewerWidget *
event_viewer_widget_new ()
{
  auto * self = static_cast<EventViewerWidget *> (
    g_object_new (EVENT_VIEWER_WIDGET_TYPE, nullptr));

  return self;
}

static void
event_viewer_finalize (EventViewerWidget * self)
{
  self->last_selections.~unique_ptr<ArrangerSelections> ();
  self->item_factories.~ItemFactoryPtrVector ();

  G_OBJECT_CLASS (event_viewer_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
event_viewer_widget_init (EventViewerWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  new (&self->last_selections) std::unique_ptr<ArrangerSelections> ();
  new (&self->item_factories) ItemFactoryPtrVector ();

  GListStore * store = g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  /* make sortable */
  GtkSorter * sorter = gtk_column_view_get_sorter (self->column_view);
  sorter = g_object_ref (sorter);
  GtkSortListModel * sort_list_model =
    gtk_sort_list_model_new (G_LIST_MODEL (store), sorter);

  GtkMultiSelection * sel = GTK_MULTI_SELECTION (
    gtk_multi_selection_new (G_LIST_MODEL (sort_list_model)));
  gtk_column_view_set_model (self->column_view, GTK_SELECTION_MODEL (sel));
}

static void
event_viewer_widget_class_init (EventViewerWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "event_viewer.ui");
  gtk_widget_class_set_css_name (klass, "event-viewer");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, EventViewerWidget, x)

  BIND_CHILD (column_view);

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) event_viewer_finalize;
}
