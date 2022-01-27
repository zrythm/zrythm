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

/**
 * \file
 *
 * File browser.
 */

#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/file_auditioner_controls.h"
#include "gui/widgets/item_factory.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/chord_pack_browser.h"
#include "gui/widgets/right_dock_edge.h"
#include "project.h"
#include "settings/chord_preset_pack_manager.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ChordPackBrowserWidget, chord_pack_browser_widget,
  GTK_TYPE_BOX)

/**
 * Visible function for file tree model.
 */
static gboolean
psets_filter_func (
  GObject * item,
  gpointer  user_data)
{
  ChordPackBrowserWidget * self =
    Z_CHORD_PACK_BROWSER_WIDGET (user_data);
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (item);
  ChordPreset * pset =
    (ChordPreset *) wrapped_obj->obj;
  (void) pset;
  (void) self;

  return true;
}

static GListModel *
create_model_for_packs (
  ChordPackBrowserWidget * self)
{
  GListStore * store =
    g_list_store_new (
      WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  int num_packs =
    chord_preset_pack_manager_get_num_packs (
      CHORD_PRESET_PACK_MANAGER);
  for (int i = 0; i < num_packs; i++)
    {
      ChordPresetPack * pack =
        chord_preset_pack_manager_get_pack_at (
          CHORD_PRESET_PACK_MANAGER, i);

      WrappedObjectWithChangeSignal * wrapped_pack =
        wrapped_object_with_change_signal_new (
          pack,
          WRAPPED_OBJECT_TYPE_CHORD_PSET_PACK);

      g_list_store_append (store, wrapped_pack);
    }

  self->packs_selection_model =
    gtk_single_selection_new (
      G_LIST_MODEL (store));

  return
    G_LIST_MODEL (self->packs_selection_model);
}

static GListModel *
create_model_for_psets (
  ChordPackBrowserWidget * self)
{
  /* file name, index */
  GListStore * store =
    g_list_store_new (
      WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  int num_packs =
    chord_preset_pack_manager_get_num_packs (
      CHORD_PRESET_PACK_MANAGER);
  for (int i = 0; i < num_packs; i++)
    {
      const ChordPresetPack * pack =
        chord_preset_pack_manager_get_pack_at (
          CHORD_PRESET_PACK_MANAGER, i);

      for (int j = 0; j < pack->num_presets; j++)
        {
          ChordPreset * pset = pack->presets[j];

          WrappedObjectWithChangeSignal * wrapped_pset =
            wrapped_object_with_change_signal_new (
              pset,
              WRAPPED_OBJECT_TYPE_CHORD_PSET);

          g_list_store_append (store, wrapped_pset);
        }
    }

  self->psets_filter =
    gtk_custom_filter_new (
      (GtkCustomFilterFunc) psets_filter_func,
      self, NULL);
  self->psets_filter_model =
    gtk_filter_list_model_new (
      G_LIST_MODEL (store),
      GTK_FILTER (self->psets_filter));
  self->psets_selection_model =
    gtk_single_selection_new (
      G_LIST_MODEL (self->psets_filter_model));

  return
    G_LIST_MODEL (self->psets_selection_model);
}

static void
refilter_presets (
  ChordPackBrowserWidget * self)
{
  gtk_filter_changed (
    GTK_FILTER (self->psets_filter),
    GTK_FILTER_CHANGE_DIFFERENT);
}

void
chord_pack_browser_widget_refresh_presets (
  ChordPackBrowserWidget * self)
{
  self->psets_selection_model =
    GTK_SINGLE_SELECTION (
      create_model_for_psets (self));
  gtk_list_view_set_model (
    self->psets_list_view,
    GTK_SELECTION_MODEL (self->psets_selection_model));
  refilter_presets (self);
}

void
chord_pack_browser_widget_refresh_packs (
  ChordPackBrowserWidget * self)
{
  self->packs_selection_model =
    GTK_SINGLE_SELECTION (
      create_model_for_packs (self));
  gtk_list_view_set_model (
    self->packs_list_view,
    GTK_SELECTION_MODEL (
      self->packs_selection_model));
}

static int
update_pset_info_label (
  ChordPackBrowserWidget * self,
  const char *             label)
{
  gtk_label_set_markup (
    self->pset_info,
    label ? label : _("No preset selected"));

  return G_SOURCE_REMOVE;
}

static ChordPreset *
get_selected_preset (
  ChordPackBrowserWidget * self)
{
  GObject * gobj =
    gtk_single_selection_get_selected_item (
      self->psets_selection_model);
  if (!gobj)
    return NULL;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  ChordPreset * pset =
    (ChordPreset *) wrapped_obj->obj;

  return pset;
}

static void
on_pack_activated (
  GtkListView * list_view,
  guint         position,
  gpointer      user_data)
{
  ChordPackBrowserWidget * self =
    Z_CHORD_PACK_BROWSER_WIDGET (user_data);

  GObject * gobj =
    g_list_model_get_object (
      G_LIST_MODEL (self->packs_selection_model),
      position);
  if (!gobj)
    return;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  ChordPresetPack * pack =
    (ChordPresetPack *) wrapped_obj->obj;

  /* TODO */
  (void) pack;
}

static void
on_pset_activated (
  GtkListView * list_view,
  guint         position,
  gpointer      user_data)
{
  ChordPackBrowserWidget * self =
    Z_CHORD_PACK_BROWSER_WIDGET (user_data);

  GObject * gobj =
    g_list_model_get_object (
      G_LIST_MODEL (self->psets_selection_model),
      position);
  if (!gobj)
    return;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  ChordPreset * pset =
    (ChordPreset *) wrapped_obj->obj;

  /* TODO set preset to chord pads */
  (void) pset;
}

static void
on_pack_selection_changed (
  GtkSelectionModel *   selection_model,
  guint                 position,
  guint                 n_items,
  gpointer              user_data)
{
  ChordPackBrowserWidget * self =
    Z_CHORD_PACK_BROWSER_WIDGET (user_data);

  sample_processor_stop_file_playback (
    SAMPLE_PROCESSOR);

  GObject * gobj =
    gtk_single_selection_get_selected_item (
      GTK_SINGLE_SELECTION (
        self->packs_selection_model));
  if (!gobj)
    {
      self->cur_pack = NULL;
      return;
    }

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  ChordPresetPack * pack =
    (ChordPresetPack *) wrapped_obj->obj;
  self->cur_pack = pack;

  g_ptr_array_remove_range (
    self->selected_packs, 0,
    self->selected_packs->len);
  g_ptr_array_add (
    self->selected_packs, pack);

  refilter_presets (self);
}

static void
on_pset_selection_changed (
  GtkSelectionModel *   selection_model,
  guint                 position,
  guint                 n_items,
  gpointer              user_data)
{
  ChordPackBrowserWidget * self =
    Z_CHORD_PACK_BROWSER_WIDGET (user_data);

  sample_processor_stop_file_playback (
    SAMPLE_PROCESSOR);

  GObject * gobj =
    gtk_single_selection_get_selected_item (
      GTK_SINGLE_SELECTION (
        self->psets_selection_model));
  if (!gobj)
    {
      self->cur_pset = NULL;
      return;
    }

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  ChordPreset * pset =
    (ChordPreset *) wrapped_obj->obj;
  self->cur_pset = pset;

  g_ptr_array_remove_range (
    self->selected_psets, 0,
    self->selected_psets->len);
  g_ptr_array_add (
    self->selected_psets, pset);

  char * label = chord_preset_get_info_text (pset);
  g_message (
    "selected preset: %s", pset->name);
  update_pset_info_label (self, label);

  if (g_settings_get_boolean (
        S_UI_FILE_BROWSER, "autoplay"))
    {
      sample_processor_queue_chord_preset (
        SAMPLE_PROCESSOR, pset);
    }
}

static void
packs_list_view_setup (
  ChordPackBrowserWidget * self,
  GtkListView *            list_view,
  GtkSelectionModel *      selection_model)
{
  gtk_list_view_set_model (
    list_view, selection_model);
  self->packs_item_factory =
    item_factory_new (
      ITEM_FACTORY_ICON_AND_TEXT, false, NULL);
  gtk_list_view_set_factory (
    list_view,
    self->packs_item_factory->list_item_factory);

  g_signal_connect (
    G_OBJECT (selection_model), "selection-changed",
    G_CALLBACK (on_pack_selection_changed),
    self);
}

static void
psets_list_view_setup (
  ChordPackBrowserWidget * self,
  GtkListView *            list_view,
  GtkSelectionModel *      selection_model)
{
  gtk_list_view_set_model (
    list_view, selection_model);
  self->psets_item_factory =
    item_factory_new (
      ITEM_FACTORY_ICON_AND_TEXT, false, NULL);
  gtk_list_view_set_factory (
    list_view,
    self->psets_item_factory->list_item_factory);

  g_signal_connect (
    G_OBJECT (selection_model), "selection-changed",
    G_CALLBACK (on_pset_selection_changed),
    self);
}

static void
on_pack_right_click (
  GtkGestureClick * self,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  gpointer          user_data)
{
  g_debug ("right click");
}

ChordPackBrowserWidget *
chord_pack_browser_widget_new ()
{
  ChordPackBrowserWidget * self =
    g_object_new (
      CHORD_PACK_BROWSER_WIDGET_TYPE, NULL);

  g_message (
    "Instantiating chord_pack_browser widget...");

  gtk_label_set_xalign (self->pset_info, 0);

  file_auditioner_controls_widget_setup (
    self->auditioner_controls,
    GTK_WIDGET (self),
    (SelectedFileGetter) get_selected_preset,
    (GenericCallback) refilter_presets);

  /* populate packs */
  self->packs_selection_model =
    GTK_SINGLE_SELECTION (
      create_model_for_packs (self));
  packs_list_view_setup (
    self, self->packs_list_view,
    GTK_SELECTION_MODEL (self->packs_selection_model));
  g_signal_connect (
    self->packs_list_view, "activate",
    G_CALLBACK (on_pack_activated), self);

  /* populate presets */
  self->psets_selection_model =
    GTK_SINGLE_SELECTION (
      create_model_for_psets (self));
  psets_list_view_setup (
    self, self->psets_list_view,
    GTK_SELECTION_MODEL (self->psets_selection_model));
  g_signal_connect (
    self->psets_list_view, "activate",
    G_CALLBACK (on_pset_activated), self);

  /* connect right click handlers */
  GtkGestureClick * mp =
    GTK_GESTURE_CLICK (
      gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "released",
    G_CALLBACK (on_pack_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->packs_list_view),
    GTK_EVENT_CONTROLLER (mp));

  return self;
}

static void
chord_pack_browser_widget_class_init (
  ChordPackBrowserWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "chord_pack_browser.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ChordPackBrowserWidget, x)

  BIND_CHILD (paned);
  BIND_CHILD (browser_top);
  BIND_CHILD (browser_bot);
  BIND_CHILD (pset_info);
  BIND_CHILD (psets_list_view);
  BIND_CHILD (packs_list_view);
  BIND_CHILD (auditioner_controls);

#undef BIND_CHILD
}

static void
chord_pack_browser_widget_init (
  ChordPackBrowserWidget * self)
{
  g_type_ensure (
    FILE_AUDITIONER_CONTROLS_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->popover_menu =
    GTK_POPOVER_MENU (
      gtk_popover_menu_new_from_model (NULL));
  gtk_box_append (
    GTK_BOX (self),
    GTK_WIDGET (self->popover_menu));

  gtk_widget_set_hexpand (
    GTK_WIDGET (self->paned), true);

  self->selected_psets = g_ptr_array_new ();

  gtk_widget_add_css_class (
    GTK_WIDGET (self), "chord-pack-browser");
}
