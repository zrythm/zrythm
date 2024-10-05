// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2024 Miró Allard <miro.allard@pm.me>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/plugins/carla_discovery.h"
#include "common/plugins/collections.h"
#include "common/plugins/plugin.h"
#include "common/plugins/plugin_manager.h"
#include "common/utils/objects.h"
#include "common/utils/resources.h"
#include "common/utils/string.h"
#include "gui/cpp/backend/settings/g_settings_manager.h"
#include "gui/cpp/backend/wrapped_object_with_change_signal.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/item_factory.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/plugin_browser.h"
#include "gui/cpp/gtk_widgets/right_dock_edge.h"
#include "gui/cpp/gtk_widgets/string_list_item_factory.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include <fmt/printf.h>

G_DEFINE_TYPE (PluginBrowserWidget, plugin_browser_widget, GTK_TYPE_WIDGET)

#define PROTOCOLS_ICON "protocol"
#define COLLECTIONS_ICON "gnome-icon-library-starred-symbolic"
#define CATEGORIES_ICON "category"
#define AUTHORS_ICON "gnome-icon-library-person-symbolic"

static void
on_selection_changed (
  GtkSelectionModel * selection_model,
  guint               position,
  guint               n_items,
  gpointer            user_data);

static void
update_internal_selections (
  PluginBrowserWidget * self,
  GtkSelectionModel *   selection_model,
  bool                  update_ui);

/**
 * Updates the label below the list of plugins with
 * the plugin info.
 */
static void
update_plugin_info_label (PluginBrowserWidget * self)
{
  GObject * gobj = G_OBJECT (gtk_single_selection_get_selected_item (
    GTK_SINGLE_SELECTION (gtk_list_view_get_model (self->plugin_list_view))));
  if (!gobj)
    return;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  PluginDescriptor * descr = std::get<PluginDescriptor *> (wrapped_obj->obj);

  auto type_label = fmt::format (
    "{} • {}{}", descr->category_str_,
    PluginDescriptor::plugin_protocol_to_str (descr->protocol_),
    descr->arch_ == PluginArchitecture::ARCH_32_BIT ? " (32-bit)" : "");
  auto audio_label =
    fmt::format ("{}, {}", descr->num_audio_ins_, descr->num_audio_outs_);
  auto midi_label =
    fmt::format ("{}, {}", descr->num_midi_ins_, descr->num_midi_outs_);
  auto ctrl_label =
    fmt::format ("{}, {}", descr->num_ctrl_ins_, descr->num_ctrl_outs_);
  auto cv_label =
    fmt::format ("{}, {}", descr->num_cv_ins_, descr->num_cv_outs_);

  gtk_label_set_text (self->plugin_author_label, descr->author_.c_str ());
  gtk_label_set_text (self->plugin_type_label, type_label.c_str ());
  gtk_label_set_text (self->plugin_audio_label, audio_label.c_str ());
  gtk_label_set_text (self->plugin_midi_label, midi_label.c_str ());
  gtk_label_set_text (self->plugin_ctrl_label, ctrl_label.c_str ());
  gtk_label_set_text (self->plugin_cv_label, cv_label.c_str ());
}

static PluginBrowserSortStyle
get_sort_style (void)
{
  return ENUM_INT_TO_VALUE (
    PluginBrowserSortStyle,
    g_settings_get_enum (S_UI_PLUGIN_BROWSER, "plugin-browser-sort-style"));
}

static void
update_stack_switcher_emblems (PluginBrowserWidget * self)
{
  AdwViewStackPage * page;

#define SET_EMBLEM(scroll_name, num) \
  page = adw_view_stack_get_page ( \
    self->stack, GTK_WIDGET (self->scroll_name##_box)); \
  adw_view_stack_page_set_badge_number (page, num); \
  adw_view_stack_page_set_needs_attention (page, num > 0)

  SET_EMBLEM (collection, self->selected_collections.size ());
  SET_EMBLEM (author, self->selected_authors.size ());
  SET_EMBLEM (category, self->selected_categories.size ());
  SET_EMBLEM (protocol, self->selected_protocols.size ());

#undef SET_EMBLEM
}

static void
save_selection (
  PluginBrowserWidget * self,
  GtkListView *         list_view,
  const char *          key)
{
  GStrvBuilder *      path_builder = g_strv_builder_new ();
  GtkSelectionModel * selection = gtk_list_view_get_model (list_view);
  GtkBitset *         bitset = gtk_selection_model_get_selection (selection);
  guint64             num_selected = gtk_bitset_get_size (bitset);
  for (guint64 i = 0; i < num_selected; i++)
    {
      guint idx = gtk_bitset_get_nth (bitset, i);
      char  buf[20];
      g_snprintf (buf, 20, "%u", idx);
      g_strv_builder_add (path_builder, buf);
    }
  GStrv strings = g_strv_builder_end (path_builder);
  g_settings_set_strv (S_UI_PLUGIN_BROWSER, key, (const char * const *) strings);
  g_strfreev (strings);
}

/**
 * Save (remember) the current selections.
 */
static void
save_selections (PluginBrowserWidget * self)
{
  save_selection (
    self, self->collection_list_view, "plugin-browser-collections");
  save_selection (self, self->author_list_view, "plugin-browser-authors");
  save_selection (self, self->category_list_view, "plugin-browser-categories");
  save_selection (self, self->protocol_list_view, "plugin-browser-protocols");
}

/**
 * Restores a selection from the gsettings.
 */
static void
restore_selection (
  PluginBrowserWidget * self,
  GtkListView *         list_view,
  const char *          key)
{
  char ** selection_paths = g_settings_get_strv (S_UI_PLUGIN_BROWSER, key);
  if (!selection_paths)
    {
      z_debug ("no selection paths for key {}", key);
      return;
    }

  GtkSelectionModel * selection = gtk_list_view_get_model (list_view);
  gtk_selection_model_unselect_all (selection);
  int i = 0;
  for (i = 0; selection_paths[i] != NULL; i++)
    {
      char *  selection_path = selection_paths[i];
      guint64 val64 = g_ascii_strtoull (selection_path, nullptr, 10);
      guint   val = (guint) val64;
      g_signal_handlers_block_by_func (
        selection, (gpointer) on_selection_changed, self);
      gtk_selection_model_select_item (selection, val, false);
      g_signal_handlers_unblock_by_func (
        selection, (gpointer) on_selection_changed, self);
    }

  g_strfreev (selection_paths);
}

static void
restore_selections (PluginBrowserWidget * self)
{
  restore_selection (
    self, self->collection_list_view, "plugin-browser-collections");
  restore_selection (
    self, self->category_list_view, "plugin-browser-categories");
  restore_selection (self, self->protocol_list_view, "plugin-browser-protocols");
  restore_selection (self, self->author_list_view, "plugin-browser-authors");
  update_internal_selections (
    self, gtk_list_view_get_model (self->collection_list_view), false);
  update_internal_selections (
    self, gtk_list_view_get_model (self->author_list_view), false);
  update_internal_selections (
    self, gtk_list_view_get_model (self->protocol_list_view), false);
  update_internal_selections (
    self, gtk_list_view_get_model (self->category_list_view), true);
}

/**
 * Called when row is double clicked.
 */
static void
on_plugin_row_activated (
  GtkListView * list_view,
  guint         position,
  gpointer      user_data)
{
  PluginBrowserWidget * self = Z_PLUGIN_BROWSER_WIDGET (user_data);

  /* FIXME this is checking the position on the selection
   * model - probably wrong, check */
  GObject * gobj = g_list_model_get_object (
    G_LIST_MODEL (gtk_list_view_get_model (self->plugin_list_view)), position);
  if (!gobj)
    return;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  PluginDescriptor * descr = std::get<PluginDescriptor *> (wrapped_obj->obj);

  char tmp[600];
  sprintf (tmp, "%p", descr);
  auto var = Glib::Variant<Glib::ustring>::create (tmp);
  zrythm_app->activate_action ("plugin-browser-add-to-project", var);
}

/**
 * Visible function for plugin tree model.
 *
 * Used for filtering based on selected category.
 *
 * Visible if row is non-empty and category
 * matches selected.
 */
static gboolean
plugin_filter_func (GObject * item, gpointer user_data)
{
  PluginBrowserWidget *           self = Z_PLUGIN_BROWSER_WIDGET (user_data);
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (item);
  PluginDescriptor * descr = std::get<PluginDescriptor *> (wrapped_obj->obj);

  int instruments_active, effects_active, modulators_active,
    midi_modifiers_active;
  instruments_active = gtk_toggle_button_get_active (self->toggle_instruments);
  effects_active = gtk_toggle_button_get_active (self->toggle_effects);
  modulators_active = gtk_toggle_button_get_active (self->toggle_modulators);
  midi_modifiers_active =
    gtk_toggle_button_get_active (self->toggle_midi_modifiers);

  /* filter by name */
  const char * text =
    gtk_editable_get_text (GTK_EDITABLE (self->plugin_search_entry));
  if (
    text && strlen (text) > 0
    && !string_contains_substr_case_insensitive (descr->name_.c_str (), text))
    {
      return false;
    }

  /* no filter, all visible */
  if (
    self->selected_categories.empty () && self->selected_authors.empty ()
    && self->selected_protocols.empty () && self->selected_collections.empty ()
    && !self->current_search && !instruments_active && !effects_active
    && !modulators_active && !midi_modifiers_active)
    {
      return true;
    }

  bool visible = false;

  /* not visible if plugin protocol is not one of selected protocols */
  if (!self->selected_protocols.empty ())
    {
      visible = false;
      for (auto prot : self->selected_protocols)
        {
          if (descr->protocol_ == prot)
            {
              visible = true;
              break;
            }
        }

      if (!visible)
        return false;
    }

  /* not visible if category selected and plugin doesn't match */
  if (!self->selected_categories.empty ())
    {
      visible = false;
      for (auto &cat : self->selected_categories)
        {
          if (descr->category_ == cat)
            {
              visible = true;
              break;
            }
        }

      /* not visible if the category is not one
       * of the selected categories */
      if (!visible)
        return false;
    }

  /* not visible if author selected and plugin doesn't match */
  if (!self->selected_authors.empty ())
    {
      if (descr->author_.empty ())
        return false;

      visible = false;
      uint32_t author_sym = self->symap->map (descr->author_.c_str ());
      for (auto &sym : self->selected_authors)
        {
          if (sym == author_sym)
            {
              visible = true;
              break;
            }
        }

      /* not visible if the author is not one of the selected
       * authors */
      if (!visible)
        return false;
    }

  /* not visible if plugin is not part of selected collection */
  if (!self->selected_collections.empty ())
    {
      visible = false;
      for (auto &coll : self->selected_collections)
        {
          if (coll->contains_descriptor (*descr))
            {
              visible = true;
              break;
            }
        }

      if (!visible)
        return false;
    }

  /* not visible if plugin type doesn't match */
  if (instruments_active && !descr->is_instrument ())
    return false;
  if (effects_active && !descr->is_effect ())
    return false;
  if (modulators_active && !descr->is_modulator ())
    return false;
  if (midi_modifiers_active && !descr->is_midi_modifier ())
    return false;

  if (
    self->current_search
    && !string_contains_substr_case_insensitive (
      descr->name_.c_str (), self->current_search))
    {
      return false;
    }

  return true;
}

#if 0
static void
delete_plugin_setting (
  PluginSetting * setting,
  GClosure *      closure)
{
  plugin_setting_free (setting);
}

static void
on_use_generic_ui_toggled (
  GtkCheckMenuItem * menuitem,
  PluginDescriptor * descr)
{
  bool is_active =
    gtk_check_menu_item_get_active (menuitem);

  PluginSetting * setting =
    plugin_setting_new_default (descr);
  setting->force_generic_ui = is_active;
  plugin_settings_set (
    S_PLUGIN_SETTINGS, setting, F_SERIALIZE);
  plugin_setting_free (setting);
}
#endif

static gint
plugin_sort_func (const void * _a, const void * _b, gpointer user_data)
{
  WrappedObjectWithChangeSignal * wrapped_obj_a =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL ((void *) _a);
  PluginDescriptor * descr_a = std::get<PluginDescriptor *> (wrapped_obj_a->obj);
  WrappedObjectWithChangeSignal * wrapped_obj_b =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL ((void *) _b);
  PluginDescriptor * descr_b = std::get<PluginDescriptor *> (wrapped_obj_b->obj);

  PluginBrowserSortStyle sort_style = get_sort_style ();
  if (sort_style == PluginBrowserSortStyle::PLUGIN_BROWSER_SORT_ALPHA)
    {
      return string_utf8_strcasecmp (
        descr_a->name_.c_str (), descr_b->name_.c_str ());
    }
  else
    {
      PluginSetting setting_a (*descr_a);
      PluginSetting setting_b (*descr_b);

      int ret = -1;

      /* higher values are sorted higher */
      if (sort_style == PluginBrowserSortStyle::PLUGIN_BROWSER_SORT_LAST_USED)
        {
          gint64 ret64 =
            setting_b.last_instantiated_time_
            - setting_a.last_instantiated_time_;
          if (ret64 == 0)
            ret = 0;
          else if (ret64 > 0)
            ret = 1;
          else
            ret = -1;
        }
      else if (
        sort_style == PluginBrowserSortStyle::PLUGIN_BROWSER_SORT_MOST_USED)
        {
          ret = setting_b.num_instantiations_ - setting_a.num_instantiations_;
        }
      else
        {
          z_return_val_if_reached (-1);
        }

      return ret;
    }

  z_return_val_if_reached (-1);
}

static void
update_internal_selections (
  PluginBrowserWidget * self,
  GtkSelectionModel *   selection_model,
  bool                  update_ui)
{
  GtkBitset * bitset = gtk_selection_model_get_selection (selection_model);
  guint64     num_selected = gtk_bitset_get_size (bitset);

  if (selection_model == gtk_list_view_get_model (self->category_list_view))
    {
      self->selected_categories.clear ();
      for (guint64 i = 0; i < num_selected; i++)
        {
          guint           idx = gtk_bitset_get_nth (bitset, i);
          ZPluginCategory cat = ENUM_INT_TO_VALUE (ZPluginCategory, idx);
          self->selected_categories.push_back (cat);
        }

      if (update_ui)
        {
          gtk_filter_changed (
            GTK_FILTER (self->plugin_filter), GTK_FILTER_CHANGE_DIFFERENT);
        }
    }
  else if (selection_model == gtk_list_view_get_model (self->author_list_view))
    {
      self->selected_authors.clear ();
      for (guint64 i = 0; i < num_selected; i++)
        {
          guint             idx = gtk_bitset_get_nth (bitset, i);
          GtkStringObject * str_obj = GTK_STRING_OBJECT (
            g_list_model_get_item (G_LIST_MODEL (selection_model), idx));
          uint32_t sym =
            self->symap->map (gtk_string_object_get_string (str_obj));
          self->selected_authors.push_back (sym);
        }

      if (update_ui)
        {
          gtk_filter_changed (
            GTK_FILTER (self->plugin_filter), GTK_FILTER_CHANGE_DIFFERENT);
        }
    }
  else if (selection_model == gtk_list_view_get_model (self->protocol_list_view))
    {
      self->selected_protocols.clear ();
      for (guint64 i = 0; i < num_selected; i++)
        {
          guint          idx = gtk_bitset_get_nth (bitset, i);
          PluginProtocol prot = ENUM_INT_TO_VALUE (PluginProtocol, idx);
          self->selected_protocols.push_back (prot);
        }

      if (update_ui)
        {
          gtk_filter_changed (
            GTK_FILTER (self->plugin_filter), GTK_FILTER_CHANGE_DIFFERENT);
        }
    }
  else if (
    selection_model == gtk_list_view_get_model (self->collection_list_view))
    {
      self->selected_collections.clear ();
      for (guint64 i = 0; i < num_selected; i++)
        {
          guint                           idx = gtk_bitset_get_nth (bitset, i);
          WrappedObjectWithChangeSignal * wobj =
            Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
              g_list_model_get_item (G_LIST_MODEL (selection_model), idx));
          auto * coll = std::get<PluginCollection *> (wobj->obj);
          if (coll)
            {
              self->selected_collections.push_back (coll);
            }
          else
            {
              z_error ("collection not found");
            }
        }

      if (update_ui)
        {
          gtk_filter_changed (
            GTK_FILTER (self->plugin_filter), GTK_FILTER_CHANGE_DIFFERENT);
        }
    }
  else if (selection_model == gtk_list_view_get_model (self->plugin_list_view))
    {
      GObject * gobj = G_OBJECT (gtk_single_selection_get_selected_item (
        GTK_SINGLE_SELECTION (selection_model)));
      if (!gobj)
        return;

      if (update_ui)
        {
          /* update status label */
          update_plugin_info_label (self);
        }
    }

  if (update_ui)
    {
      save_selections (self);
      update_stack_switcher_emblems (self);
      update_plugin_info_label (self);
    }
}

static void
on_selection_changed (
  GtkSelectionModel * selection_model,
  guint               position,
  guint               n_items,
  gpointer            user_data)
{
  PluginBrowserWidget * self = Z_PLUGIN_BROWSER_WIDGET (user_data);

  update_internal_selections (self, selection_model, true);
}

static GtkSelectionModel *
create_model_for_collections (void)
{
  GListStore * list_store =
    g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  auto &collections = PLUGIN_MANAGER->collections_;
  z_return_val_if_fail (collections, nullptr);
  for (auto &collection : collections->collections_)
    {
      /* add row to model */
      WrappedObjectWithChangeSignal * wobj =
        wrapped_object_with_change_signal_new (
          &collection, WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_COLLECTION);
      g_list_store_append (list_store, wobj);
    }

  GtkMultiSelection * sel = gtk_multi_selection_new (G_LIST_MODEL (list_store));

  return GTK_SELECTION_MODEL (sel);
}

static GtkSelectionModel *
create_model_for_categories (void)
{
  GtkStringList * string_list = gtk_string_list_new (nullptr);
  for (size_t i = 0; i < ENUM_COUNT (ZPluginCategory); i++)
    {
      std::string name = PluginDescriptor::category_to_string (
        ENUM_INT_TO_VALUE (ZPluginCategory, i));
      gtk_string_list_append (string_list, name.c_str ());
    }

  GtkMultiSelection * sel = gtk_multi_selection_new (G_LIST_MODEL (string_list));

  return GTK_SELECTION_MODEL (sel);
}

static GtkSelectionModel *
create_model_for_authors (void)
{
  GtkStringList * string_list = gtk_string_list_new (nullptr);
  for (auto &author : PLUGIN_MANAGER->plugin_authors_)
    {
      gtk_string_list_append (string_list, author.c_str ());
    }

  GtkMultiSelection * sel = gtk_multi_selection_new (G_LIST_MODEL (string_list));

  return GTK_SELECTION_MODEL (sel);
}

static GtkSelectionModel *
create_model_for_protocols (void)
{
  GtkStringList * string_list = gtk_string_list_new (nullptr);
  for (size_t i = 0; i < ENUM_COUNT (PluginProtocol); i++)
    {
      std::string name = PluginDescriptor::plugin_protocol_to_str (
        ENUM_INT_TO_VALUE (PluginProtocol, i));
      gtk_string_list_append (string_list, name.c_str ());
    }

  GtkMultiSelection * sel = gtk_multi_selection_new (G_LIST_MODEL (string_list));

  return GTK_SELECTION_MODEL (sel);
}

static GtkSelectionModel *
create_model_for_plugins (PluginBrowserWidget * self)
{
  GListStore * store = g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  for (auto &descr : PLUGIN_MANAGER->plugin_descriptors_)
    {
      WrappedObjectWithChangeSignal * wrapped_descr =
        wrapped_object_with_change_signal_new_with_free_func (
          new PluginDescriptor (descr),
          WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_DESCR,
          [] (void * ptr) { delete static_cast<PluginDescriptor *> (ptr); });

      g_list_store_append (store, wrapped_descr);
    }

  /* create sorter */
  self->plugin_sorter = gtk_custom_sorter_new (plugin_sort_func, self, nullptr);
  self->plugin_sort_model = gtk_sort_list_model_new (
    G_LIST_MODEL (store), GTK_SORTER (self->plugin_sorter));

  /* create filter */
  self->plugin_filter = gtk_custom_filter_new (
    (GtkCustomFilterFunc) plugin_filter_func, self, nullptr);
  self->plugin_filter_model = gtk_filter_list_model_new (
    G_LIST_MODEL (self->plugin_sort_model), GTK_FILTER (self->plugin_filter));

  /* create single selection */
  GtkSingleSelection * single_sel =
    gtk_single_selection_new (G_LIST_MODEL (self->plugin_filter_model));

  return GTK_SELECTION_MODEL (single_sel);
}

static void
list_view_setup (
  PluginBrowserWidget * self,
  GtkListView *         list_view,
  GtkSelectionModel *   selection_model,
  bool                  first_time)
{
  gtk_list_view_set_model (list_view, selection_model);

  if (first_time)
    {
      if (
        list_view == self->plugin_list_view
        || list_view == self->collection_list_view)
        {
          auto factory = std::make_unique<ItemFactory> (
            self->plugin_list_view == list_view
              ? ItemFactory::Type::IconAndText
              : ItemFactory::Type::Text,
            false, "");
          gtk_list_view_set_factory (list_view, factory->list_item_factory_);
          self->item_factories.emplace_back (std::move (factory));
        }
      else if (list_view == self->author_list_view)
        {
          gtk_list_view_set_factory (
            list_view, string_list_item_factory_new (nullptr));
        }
      else if (list_view == self->category_list_view)
        {
          gtk_list_view_set_factory (
            list_view, string_list_item_factory_new ([] (int value) {
              return PluginDescriptor::category_to_string (
                static_cast<ZPluginCategory> (value));
            }));
        }
      else if (list_view == self->protocol_list_view)
        {
          gtk_list_view_set_factory (
            list_view, string_list_item_factory_new ([] (int value) {
              return PluginDescriptor::plugin_protocol_to_str (
                static_cast<PluginProtocol> (value));
            }));
        }
    }

  g_signal_connect (
    G_OBJECT (selection_model), "selection-changed",
    G_CALLBACK (on_selection_changed), self);
}

void
plugin_browser_widget_refresh_collections (PluginBrowserWidget * self)
{
  GtkSelectionModel * model = create_model_for_collections ();
  self->selected_collections.clear ();
  list_view_setup (self, self->collection_list_view, model, false);
}

static void
on_visible_child_changed (
  AdwViewStack *        stack,
  GParamSpec *          pspec,
  PluginBrowserWidget * self)
{
  GtkWidget * child = adw_view_stack_get_visible_child (stack);

  if (child == GTK_WIDGET (self->collection_box))
    {
      g_settings_set_enum (
        S_UI_PLUGIN_BROWSER, "plugin-browser-tab",
        ENUM_VALUE_TO_INT (PluginBrowserTab::PLUGIN_BROWSER_TAB_COLLECTION));
    }
  else if (child == GTK_WIDGET (self->author_box))
    {
      g_settings_set_enum (
        S_UI_PLUGIN_BROWSER, "plugin-browser-tab",
        ENUM_VALUE_TO_INT (PluginBrowserTab::PLUGIN_BROWSER_TAB_AUTHOR));
    }
  else if (child == GTK_WIDGET (self->category_box))
    {
      g_settings_set_enum (
        S_UI_PLUGIN_BROWSER, "plugin-browser-tab",
        ENUM_VALUE_TO_INT (PluginBrowserTab::PLUGIN_BROWSER_TAB_CATEGORY));
    }
  else if (child == GTK_WIDGET (self->protocol_box))
    {
      g_settings_set_enum (
        S_UI_PLUGIN_BROWSER, "plugin-browser-tab",
        ENUM_VALUE_TO_INT (PluginBrowserTab::PLUGIN_BROWSER_TAB_PROTOCOL));
    }
}

static void
toggles_changed (GtkToggleButton * btn, PluginBrowserWidget * self)
{
  z_return_if_fail (GTK_IS_TOGGLE_BUTTON (btn));
  z_return_if_fail (Z_IS_PLUGIN_BROWSER_WIDGET (self));
  if (gtk_toggle_button_get_active (btn))
    {
      /* block signals, unset all, unblock */
      g_signal_handlers_block_by_func (
        self->toggle_instruments, (gpointer) toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_effects, (gpointer) toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_modulators, (gpointer) toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_midi_modifiers, (gpointer) toggles_changed, self);

      if (btn == self->toggle_instruments)
        {
          g_settings_set_enum (
            S_UI_PLUGIN_BROWSER, "plugin-browser-filter",
            ENUM_VALUE_TO_INT (
              PluginBrowserFilter::PLUGIN_BROWSER_FILTER_INSTRUMENT));
          gtk_toggle_button_set_active (self->toggle_effects, 0);
          gtk_toggle_button_set_active (self->toggle_modulators, 0);
          gtk_toggle_button_set_active (self->toggle_midi_modifiers, 0);
        }
      else if (btn == self->toggle_effects)
        {
          g_settings_set_enum (
            S_UI_PLUGIN_BROWSER, "plugin-browser-filter",
            ENUM_VALUE_TO_INT (
              PluginBrowserFilter::PLUGIN_BROWSER_FILTER_EFFECT));
          gtk_toggle_button_set_active (self->toggle_instruments, 0);
          gtk_toggle_button_set_active (self->toggle_modulators, 0);
          gtk_toggle_button_set_active (self->toggle_midi_modifiers, 0);
        }
      else if (btn == self->toggle_modulators)
        {
          g_settings_set_enum (
            S_UI_PLUGIN_BROWSER, "plugin-browser-filter",
            ENUM_VALUE_TO_INT (
              PluginBrowserFilter::PLUGIN_BROWSER_FILTER_MODULATOR));
          gtk_toggle_button_set_active (self->toggle_instruments, 0);
          gtk_toggle_button_set_active (self->toggle_effects, 0);
          gtk_toggle_button_set_active (self->toggle_midi_modifiers, 0);
        }
      else if (btn == self->toggle_midi_modifiers)
        {
          g_settings_set_enum (
            S_UI_PLUGIN_BROWSER, "plugin-browser-filter",
            ENUM_VALUE_TO_INT (
              PluginBrowserFilter::PLUGIN_BROWSER_FILTER_MIDI_EFFECT));
          gtk_toggle_button_set_active (self->toggle_instruments, 0);
          gtk_toggle_button_set_active (self->toggle_effects, 0);
          gtk_toggle_button_set_active (self->toggle_modulators, 0);
        }

      g_signal_handlers_unblock_by_func (
        self->toggle_instruments, (gpointer) toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_effects, (gpointer) toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_modulators, (gpointer) toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_midi_modifiers, (gpointer) toggles_changed, self);
    }
  else
    {
      g_settings_set_enum (
        S_UI_PLUGIN_BROWSER, "plugin-browser-filter",
        ENUM_VALUE_TO_INT (PluginBrowserFilter::PLUGIN_BROWSER_FILTER_NONE));
    }
  gtk_filter_changed (
    GTK_FILTER (self->plugin_filter), GTK_FILTER_CHANGE_DIFFERENT);
  update_plugin_info_label (self);
}

static void
on_map (GtkWidget * widget, PluginBrowserWidget * self)
{
  /*z_info ("PLUGIN MAP EVENT");*/
  if (gtk_widget_get_mapped (GTK_WIDGET (self)))
    {
      self->start_saving_pos = 1;
      self->first_time_position_set_time = g_get_monotonic_time ();

      /* set divider position */
      int divider_pos = g_settings_get_int (S_UI, "browser-divider-position");
      gtk_paned_set_position (self->paned, divider_pos);
      self->first_time_position_set = 1;
    }
}

static void
on_position_change (
  GtkStack *            stack,
  GParamSpec *          pspec,
  PluginBrowserWidget * self)
{
  int divider_pos;
  if (!self->start_saving_pos)
    return;

  gint64 curr_time = g_get_monotonic_time ();

  if (
    self->first_time_position_set
    || curr_time - self->first_time_position_set_time < 400000)
    {
      /* get divider position */
      divider_pos = g_settings_get_int (S_UI, "browser-divider-position");
      gtk_paned_set_position (self->paned, divider_pos);

      self->first_time_position_set = 0;
      /*z_info ("***************************got plugin position {}",*/
      /*divider_pos);*/
    }
  else
    {
      /* save the divide position */
      divider_pos = gtk_paned_get_position (self->paned);
      g_settings_set_int (S_UI, "browser-divider-position", divider_pos);
      /*z_info ("***************************set plugin position to {}",*/
      /*divider_pos);*/
    }
}

static void
on_key_release (
  GtkEventControllerKey * key_controller,
  guint                   keyval,
  guint                   keycode,
  GdkModifierType         state,
  PluginBrowserWidget *   self)
{
  g_free_and_null (self->current_search);

  gtk_filter_changed (
    GTK_FILTER (self->plugin_filter), GTK_FILTER_CHANGE_DIFFERENT);
  update_plugin_info_label (self);

  z_info ("key release");
}

static void
on_plugin_search_changed (
  GtkSearchEntry *      search_entry,
  PluginBrowserWidget * self)
{
  gtk_filter_changed (
    GTK_FILTER (self->plugin_filter), GTK_FILTER_CHANGE_DIFFERENT);
  update_plugin_info_label (self);
}

static void
on_sort_style_changed (GtkToggleButton * btn, PluginBrowserWidget * self)
{
  PluginBrowserSortStyle sort_style =
    PluginBrowserSortStyle::PLUGIN_BROWSER_SORT_ALPHA;
  if (gtk_toggle_button_get_active (self->alpha_sort_btn))
    {
      sort_style = PluginBrowserSortStyle::PLUGIN_BROWSER_SORT_ALPHA;
    }
  else if (gtk_toggle_button_get_active (self->last_used_sort_btn))
    {
      sort_style = PluginBrowserSortStyle::PLUGIN_BROWSER_SORT_LAST_USED;
    }
  else if (gtk_toggle_button_get_active (self->most_used_sort_btn))
    {
      sort_style = PluginBrowserSortStyle::PLUGIN_BROWSER_SORT_MOST_USED;
    }

  g_settings_set_enum (
    S_UI_PLUGIN_BROWSER, "plugin-browser-sort-style",
    ENUM_VALUE_TO_INT (sort_style));

  gtk_sorter_changed (
    GTK_SORTER (self->plugin_sorter), GTK_SORTER_CHANGE_DIFFERENT);
}

PluginBrowserWidget *
plugin_browser_widget_new (void)
{
  PluginBrowserWidget * self = static_cast<PluginBrowserWidget *> (
    g_object_new (PLUGIN_BROWSER_WIDGET_TYPE, nullptr));

  self->symap = new Symap ();

  /* setup collections */
  GtkSelectionModel * model = create_model_for_collections ();
  list_view_setup (self, self->collection_list_view, model, true);

  /* setup protocols */
  model = create_model_for_protocols ();
  list_view_setup (self, self->protocol_list_view, model, true);

  /* setup authors */
  model = create_model_for_authors ();
  list_view_setup (self, self->author_list_view, model, true);

  /* setup categories */
  model = create_model_for_categories ();
  list_view_setup (self, self->category_list_view, model, true);

  /* populate plugins */
  model = create_model_for_plugins (self);
  list_view_setup (self, self->plugin_list_view, model, true);
  g_signal_connect (
    G_OBJECT (self->plugin_list_view), "activate",
    G_CALLBACK (on_plugin_row_activated), self);

  /* setup plugin search entry */
  gtk_search_entry_set_key_capture_widget (
    self->plugin_search_entry, GTK_WIDGET (self->plugin_list_view));
  g_object_set (
    G_OBJECT (self->plugin_search_entry), "placeholder-text", _ ("Search..."),
    nullptr);
  g_signal_connect (
    G_OBJECT (self->plugin_search_entry), "search-changed",
    G_CALLBACK (on_plugin_search_changed), self);

  /* set the selected values */
  PluginBrowserTab tab = ENUM_INT_TO_VALUE (
    PluginBrowserTab,
    g_settings_get_enum (S_UI_PLUGIN_BROWSER, "plugin-browser-tab"));
  PluginBrowserFilter filter = ENUM_INT_TO_VALUE (
    PluginBrowserFilter,
    g_settings_get_enum (S_UI_PLUGIN_BROWSER, "plugin-browser-filter"));
  restore_selections (self);

  switch (tab)
    {
    case PluginBrowserTab::PLUGIN_BROWSER_TAB_COLLECTION:
      adw_view_stack_set_visible_child (
        self->stack, GTK_WIDGET (self->collection_box));
      break;
    case PluginBrowserTab::PLUGIN_BROWSER_TAB_AUTHOR:
      adw_view_stack_set_visible_child (
        self->stack, GTK_WIDGET (self->author_box));
      break;
    case PluginBrowserTab::PLUGIN_BROWSER_TAB_CATEGORY:
      adw_view_stack_set_visible_child (
        self->stack, GTK_WIDGET (self->category_box));
      break;
    case PluginBrowserTab::PLUGIN_BROWSER_TAB_PROTOCOL:
      adw_view_stack_set_visible_child (
        self->stack, GTK_WIDGET (self->protocol_box));
      break;
    default:
      z_warn_if_reached ();
      break;
    }

  switch (filter)
    {
    case PluginBrowserFilter::PLUGIN_BROWSER_FILTER_NONE:
      break;
    case PluginBrowserFilter::PLUGIN_BROWSER_FILTER_INSTRUMENT:
      gtk_toggle_button_set_active (self->toggle_instruments, 1);
      break;
    case PluginBrowserFilter::PLUGIN_BROWSER_FILTER_EFFECT:
      gtk_toggle_button_set_active (self->toggle_effects, 1);
      break;
    case PluginBrowserFilter::PLUGIN_BROWSER_FILTER_MODULATOR:
      gtk_toggle_button_set_active (self->toggle_modulators, 1);
      break;
    case PluginBrowserFilter::PLUGIN_BROWSER_FILTER_MIDI_EFFECT:
      gtk_toggle_button_set_active (self->toggle_midi_modifiers, 1);
      break;
    }

  /* set divider position */
  int divider_pos = g_settings_get_int (S_UI, "browser-divider-position");
  gtk_paned_set_position (self->paned, divider_pos);
  self->first_time_position_set = 1;
  z_info ("setting plugin browser divider pos to {}", divider_pos);

  /* re-sort */
  gtk_sorter_changed (
    GTK_SORTER (self->plugin_sorter), GTK_SORTER_CHANGE_DIFFERENT);

  /* notify when tab changes */
  g_signal_connect (
    G_OBJECT (self->stack), "notify::visible-child",
    G_CALLBACK (on_visible_child_changed), self);
  g_signal_connect (
    G_OBJECT (self), "notify::position", G_CALLBACK (on_position_change), self);
  g_signal_connect (G_OBJECT (self), "map", G_CALLBACK (on_map), self);

  GtkEventController * key_controller = gtk_event_controller_key_new ();
  g_signal_connect (
    G_OBJECT (key_controller), "key-released", G_CALLBACK (on_key_release),
    self);
  gtk_widget_add_controller (GTK_WIDGET (self), key_controller);

  return self;
}

static void
dispose (PluginBrowserWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  G_OBJECT_CLASS (plugin_browser_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
finalize (PluginBrowserWidget * self)
{
  std::construct_at (&self->selected_authors);
  std::construct_at (&self->selected_categories);
  std::construct_at (&self->selected_protocols);
  std::construct_at (&self->selected_collections);
  std::construct_at (&self->item_factories);

  object_delete_and_null (self->symap);

  G_OBJECT_CLASS (plugin_browser_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
plugin_browser_widget_class_init (PluginBrowserWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "plugin_browser.ui");

  /*gtk_widget_class_add_shortcut (*/

#define BIND_CHILD(name) \
  gtk_widget_class_bind_template_child (klass, PluginBrowserWidget, name)

  BIND_CHILD (paned);
  BIND_CHILD (collection_box);
  BIND_CHILD (protocol_box);
  BIND_CHILD (category_box);
  BIND_CHILD (author_box);
  BIND_CHILD (plugin_scroll);
  BIND_CHILD (collection_list_view);
  BIND_CHILD (protocol_list_view);
  BIND_CHILD (category_list_view);
  BIND_CHILD (author_list_view);
  BIND_CHILD (plugin_search_entry);
  BIND_CHILD (plugin_list_view);
  BIND_CHILD (browser_bot);
  BIND_CHILD (plugin_author_label);
  BIND_CHILD (plugin_type_label);
  BIND_CHILD (plugin_audio_label);
  BIND_CHILD (plugin_midi_label);
  BIND_CHILD (plugin_ctrl_label);
  BIND_CHILD (plugin_cv_label);
  BIND_CHILD (stack_switcher_box);
  BIND_CHILD (stack);
  BIND_CHILD (plugin_toolbar);
  BIND_CHILD (toggle_instruments);
  BIND_CHILD (toggle_effects);
  BIND_CHILD (toggle_modulators);
  BIND_CHILD (toggle_midi_modifiers);
  BIND_CHILD (alpha_sort_btn);
  BIND_CHILD (last_used_sort_btn);
  BIND_CHILD (most_used_sort_btn);

#undef BIND_CHILD

#define BIND_SIGNAL(sig) gtk_widget_class_bind_template_callback (klass, sig)

  BIND_SIGNAL (toggles_changed);

#undef BIND_SIGNAL

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * goklass = G_OBJECT_CLASS (_klass);
  goklass->dispose = (GObjectFinalizeFunc) dispose;
  goklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
plugin_browser_widget_init (PluginBrowserWidget * self)
{
  std::construct_at (&self->selected_authors);
  std::construct_at (&self->selected_categories);
  std::construct_at (&self->selected_protocols);
  std::construct_at (&self->selected_collections);
  std::construct_at (&self->item_factories);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (nullptr));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  self->stack_switcher = ADW_VIEW_SWITCHER (adw_view_switcher_new ());
  adw_view_switcher_set_stack (self->stack_switcher, self->stack);
  gtk_box_append (self->stack_switcher_box, GTK_WIDGET (self->stack_switcher));
  gtk_widget_set_size_request (GTK_WIDGET (self->stack_switcher), -1, 52);

  gtk_widget_set_hexpand (GTK_WIDGET (self->paned), true);

  gtk_widget_add_css_class (GTK_WIDGET (self), "plugin-browser");

  /* add toggle group for sort buttons */
  gtk_toggle_button_set_group (self->last_used_sort_btn, self->alpha_sort_btn);
  gtk_toggle_button_set_group (self->most_used_sort_btn, self->alpha_sort_btn);
  gtk_toggle_button_set_active (self->alpha_sort_btn, true);

  /* use sort style from last time */
  PluginBrowserSortStyle sort_style = get_sort_style ();
  switch (sort_style)
    {
    case PluginBrowserSortStyle::PLUGIN_BROWSER_SORT_ALPHA:
      gtk_toggle_button_set_active (self->alpha_sort_btn, true);
      break;
    case PluginBrowserSortStyle::PLUGIN_BROWSER_SORT_LAST_USED:
      gtk_toggle_button_set_active (self->last_used_sort_btn, true);
      break;
    case PluginBrowserSortStyle::PLUGIN_BROWSER_SORT_MOST_USED:
      gtk_toggle_button_set_active (self->most_used_sort_btn, true);
      break;
    }

  /* add callback when sort style changed */
  g_signal_connect (
    G_OBJECT (self->alpha_sort_btn), "toggled",
    G_CALLBACK (on_sort_style_changed), self);
  g_signal_connect (
    G_OBJECT (self->last_used_sort_btn), "toggled",
    G_CALLBACK (on_sort_style_changed), self);
  g_signal_connect (
    G_OBJECT (self->most_used_sort_btn), "toggled",
    G_CALLBACK (on_sort_style_changed), self);
}
