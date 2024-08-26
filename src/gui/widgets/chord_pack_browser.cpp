// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/chord_pack_browser.h"
#include "gui/widgets/file_auditioner_controls.h"
#include "gui/widgets/item_factory.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/right_dock_edge.h"
#include "project.h"
#include "settings/chord_preset_pack_manager.h"
#include "settings/g_settings_manager.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (ChordPackBrowserWidget, chord_pack_browser_widget, GTK_TYPE_BOX)

/**
 * Visible function for file tree model.
 */
static gboolean
psets_filter_func (GObject * item, gpointer user_data)
{
  ChordPackBrowserWidget * self = Z_CHORD_PACK_BROWSER_WIDGET (user_data);
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (item);
  ChordPreset * pset = std::get<ChordPreset *> (wrapped_obj->obj);

  GObject * pack_gobj = G_OBJECT (
    gtk_single_selection_get_selected_item (self->packs_selection_model));
  if (!pack_gobj)
    return true;

  WrappedObjectWithChangeSignal * wrapped_pack =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (pack_gobj);
  ChordPresetPack * pack = std::get<ChordPresetPack *> (wrapped_pack->obj);

  if (pack->contains_preset (*pset))
    {
      return true;
    }

  return false;
}

static GListModel *
create_model_for_packs (ChordPackBrowserWidget * self)
{
  GListStore * store = g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  for (auto &pack : CHORD_PRESET_PACK_MANAGER->packs_)
    {
      WrappedObjectWithChangeSignal * wrapped_pack =
        wrapped_object_with_change_signal_new (
          pack.get (), WrappedObjectType::WRAPPED_OBJECT_TYPE_CHORD_PSET_PACK);

      g_list_store_append (store, wrapped_pack);
    }

  self->packs_selection_model = gtk_single_selection_new (G_LIST_MODEL (store));

  return G_LIST_MODEL (self->packs_selection_model);
}

static GListModel *
create_model_for_psets (ChordPackBrowserWidget * self)
{
  /* file name, index */
  GListStore * store = g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  for (auto &pack : CHORD_PRESET_PACK_MANAGER->packs_)
    {
      for (auto &pset : pack->presets_)
        {
          WrappedObjectWithChangeSignal * wrapped_pset =
            wrapped_object_with_change_signal_new (
              &pset, WrappedObjectType::WRAPPED_OBJECT_TYPE_CHORD_PSET);

          g_list_store_append (store, wrapped_pset);
        }
    }

  self->psets_filter = gtk_custom_filter_new (
    (GtkCustomFilterFunc) psets_filter_func, self, nullptr);
  self->psets_filter_model = gtk_filter_list_model_new (
    G_LIST_MODEL (store), GTK_FILTER (self->psets_filter));
  self->psets_selection_model =
    gtk_single_selection_new (G_LIST_MODEL (self->psets_filter_model));

  return G_LIST_MODEL (self->psets_selection_model);
}

static void
refilter_presets (ChordPackBrowserWidget * self)
{
  gtk_filter_changed (
    GTK_FILTER (self->psets_filter), GTK_FILTER_CHANGE_DIFFERENT);
}

void
chord_pack_browser_widget_refresh_presets (ChordPackBrowserWidget * self)
{
  self->psets_selection_model =
    GTK_SINGLE_SELECTION (create_model_for_psets (self));
  gtk_list_view_set_model (
    self->psets_list_view, GTK_SELECTION_MODEL (self->psets_selection_model));
  refilter_presets (self);
}

void
chord_pack_browser_widget_refresh_packs (ChordPackBrowserWidget * self)
{
  self->packs_selection_model =
    GTK_SINGLE_SELECTION (create_model_for_packs (self));
  gtk_list_view_set_model (
    self->packs_list_view, GTK_SELECTION_MODEL (self->packs_selection_model));
}

static int
update_pset_info_label (ChordPackBrowserWidget * self, const char * label)
{
  gtk_label_set_markup (
    self->pset_info, label ? label : _ ("No preset selected"));

  return G_SOURCE_REMOVE;
}

static WrappedObjectWithChangeSignal *
get_selected_preset (GtkWidget * widget)
{
  ChordPackBrowserWidget * self = Z_CHORD_PACK_BROWSER_WIDGET (widget);
  GObject *                gobj = G_OBJECT (
    gtk_single_selection_get_selected_item (self->psets_selection_model));
  if (!gobj)
    return NULL;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);

  return wrapped_obj;
}

static void
on_pack_activated (GtkListView * list_view, guint position, gpointer user_data)
{
  ChordPackBrowserWidget * self = Z_CHORD_PACK_BROWSER_WIDGET (user_data);

  GObject * gobj = g_list_model_get_object (
    G_LIST_MODEL (self->packs_selection_model), position);
  if (!gobj)
    return;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  ChordPresetPack * pack = std::get<ChordPresetPack *> (wrapped_obj->obj);

  /* TODO */
  (void) pack;
}

static void
on_pset_activated (GtkListView * list_view, guint position, gpointer user_data)
{
  ChordPackBrowserWidget * self = Z_CHORD_PACK_BROWSER_WIDGET (user_data);

  GObject * gobj = g_list_model_get_object (
    G_LIST_MODEL (self->psets_selection_model), position);
  if (!gobj)
    return;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  auto * pset = std::get<ChordPreset *> (wrapped_obj->obj);

  auto pack = CHORD_PRESET_PACK_MANAGER->get_pack_for_preset (*pset);
  z_return_if_fail (pack);

  /* set preset to chord pads */
  int pack_idx = CHORD_PRESET_PACK_MANAGER->get_pack_index (*pack);
  int pset_idx = CHORD_PRESET_PACK_MANAGER->get_pset_index (*pset);
  gtk_widget_activate_action (
    GTK_WIDGET (self), "app.load-chord-preset", "s",
    fmt::format ("{},{}", pack_idx, pset_idx).c_str ());
}

static void
on_pack_selection_changed (
  GtkSelectionModel * selection_model,
  guint               position,
  guint               n_items,
  gpointer            user_data)
{
  ChordPackBrowserWidget * self = Z_CHORD_PACK_BROWSER_WIDGET (user_data);

  SAMPLE_PROCESSOR->stop_file_playback ();

  GObject * gobj = G_OBJECT (gtk_single_selection_get_selected_item (
    GTK_SINGLE_SELECTION (self->packs_selection_model)));
  if (!gobj)
    {
      self->cur_pack = NULL;
      return;
    }

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  ChordPresetPack * pack = std::get<ChordPresetPack *> (wrapped_obj->obj);
  self->cur_pack = pack;

  self->selected_packs->clear ();
  self->selected_packs->push_back (pack);

  refilter_presets (self);
}

static void
on_pset_selection_changed (
  GtkSelectionModel * selection_model,
  guint               position,
  guint               n_items,
  gpointer            user_data)
{
  ChordPackBrowserWidget * self = Z_CHORD_PACK_BROWSER_WIDGET (user_data);

  SAMPLE_PROCESSOR->stop_file_playback ();

  GObject * gobj = G_OBJECT (gtk_single_selection_get_selected_item (
    GTK_SINGLE_SELECTION (self->psets_selection_model)));
  if (!gobj)
    {
      self->cur_pset = NULL;
      return;
    }

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  ChordPreset * pset = std::get<ChordPreset *> (wrapped_obj->obj);
  self->cur_pset = pset;

  self->selected_psets->clear ();
  self->selected_psets->push_back (pset);

  auto label = pset->get_info_text ();
  z_debug ("selected preset: {}", pset->name_);
  update_pset_info_label (self, label.c_str ());

  if (g_settings_get_boolean (S_UI_FILE_BROWSER, "autoplay"))
    {
      SAMPLE_PROCESSOR->queue_chord_preset (*pset);
    }
}

static void
packs_list_view_setup (
  ChordPackBrowserWidget * self,
  GtkListView *            list_view,
  GtkSelectionModel *      selection_model)
{
  gtk_list_view_set_model (list_view, selection_model);
  *self->packs_item_factory =
    std::make_unique<ItemFactory> (ItemFactory::Type::IconAndText, false, "");
  gtk_list_view_set_factory (
    list_view, (*self->packs_item_factory)->list_item_factory_);

  g_signal_connect (
    G_OBJECT (selection_model), "selection-changed",
    G_CALLBACK (on_pack_selection_changed), self);
}

static void
psets_list_view_setup (
  ChordPackBrowserWidget * self,
  GtkListView *            list_view,
  GtkSelectionModel *      selection_model)
{
  gtk_list_view_set_model (list_view, selection_model);
  *self->psets_item_factory =
    std::make_unique<ItemFactory> (ItemFactory::Type::IconAndText, false, "");
  gtk_list_view_set_factory (
    list_view, (*self->psets_item_factory)->list_item_factory_);

  g_signal_connect (
    G_OBJECT (selection_model), "selection-changed",
    G_CALLBACK (on_pset_selection_changed), self);
}

static void
on_pack_right_click (
  GtkGestureClick * self,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  gpointer          user_data)
{
  z_debug ("right click");
}

ChordPackBrowserWidget *
chord_pack_browser_widget_new (void)
{
  ChordPackBrowserWidget * self = static_cast<ChordPackBrowserWidget *> (
    g_object_new (CHORD_PACK_BROWSER_WIDGET_TYPE, nullptr));

  z_info ("Instantiating chord_pack_browser widget...");

  gtk_label_set_xalign (self->pset_info, 0);

  file_auditioner_controls_widget_setup (
    self->auditioner_controls, GTK_WIDGET (self), false, get_selected_preset,
    [self] () { refilter_presets (self); });

  /* populate packs */
  self->packs_selection_model =
    GTK_SINGLE_SELECTION (create_model_for_packs (self));
  packs_list_view_setup (
    self, self->packs_list_view,
    GTK_SELECTION_MODEL (self->packs_selection_model));
  g_signal_connect (
    self->packs_list_view, "activate", G_CALLBACK (on_pack_activated), self);

  /* populate presets */
  self->psets_selection_model =
    GTK_SINGLE_SELECTION (create_model_for_psets (self));
  psets_list_view_setup (
    self, self->psets_list_view,
    GTK_SELECTION_MODEL (self->psets_selection_model));
  g_signal_connect (
    self->psets_list_view, "activate", G_CALLBACK (on_pset_activated), self);

  /* connect right click handlers */
  GtkGestureClick * mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "released", G_CALLBACK (on_pack_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->packs_list_view), GTK_EVENT_CONTROLLER (mp));

  return self;
}

static void
chord_pack_browser_widget_finalize (GObject * obj)
{
  ChordPackBrowserWidget * self = Z_CHORD_PACK_BROWSER_WIDGET (obj);

  delete self->packs_item_factory;
  delete self->psets_item_factory;
  delete self->selected_packs;
  delete self->selected_psets;

  G_OBJECT_CLASS (chord_pack_browser_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
chord_pack_browser_widget_class_init (ChordPackBrowserWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "chord_pack_browser.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, ChordPackBrowserWidget, x)

  BIND_CHILD (paned);
  BIND_CHILD (browser_top);
  BIND_CHILD (browser_bot);
  BIND_CHILD (pset_info);
  BIND_CHILD (psets_list_view);
  BIND_CHILD (packs_list_view);
  BIND_CHILD (auditioner_controls);

#undef BIND_CHILD

  GObjectClass * gobject_class = G_OBJECT_CLASS (_klass);
  gobject_class->finalize = chord_pack_browser_widget_finalize;
}

static void
chord_pack_browser_widget_init (ChordPackBrowserWidget * self)
{
  self->packs_item_factory = new std::unique_ptr<ItemFactory> ();
  self->psets_item_factory = new std::unique_ptr<ItemFactory> ();
  self->selected_packs = new std::vector<ChordPresetPack *> ();
  self->selected_psets = new std::vector<ChordPreset *> ();

  g_type_ensure (FILE_AUDITIONER_CONTROLS_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (nullptr));
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->popover_menu));

  gtk_widget_set_hexpand (GTK_WIDGET (self->paned), true);

  gtk_label_set_wrap (self->pset_info, true);

  gtk_widget_add_css_class (GTK_WIDGET (self), "chord-pack-browser");
}
