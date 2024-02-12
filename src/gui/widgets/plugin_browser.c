// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2024 Miró Allard <miro.allard@pm.me>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "actions/tracklist_selections.h"
#include "dsp/engine.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/dialogs/string_entry_dialog.h"
#include "gui/widgets/expander_box.h"
#include "gui/widgets/item_factory.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/plugin_browser.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/string_list_item_factory.h"
#include "plugins/carla/carla_discovery.h"
#include "plugins/collections.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "enum-types.h"

G_DEFINE_TYPE (PluginBrowserWidget, plugin_browser_widget, GTK_TYPE_WIDGET)

enum
{
  CAT_COLUMN_NAME,
  CAT_NUM_COLUMNS,
};

enum
{
  PL_COLUMN_ICON,
  PL_COLUMN_NAME,
  PL_COLUMN_DESCR,
  PL_NUM_COLUMNS
};

#define PROTOCOLS_ICON "protocol"
#define COLLECTIONS_ICON "favorite"
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
  GObject * gobj = gtk_single_selection_get_selected_item (
    GTK_SINGLE_SELECTION (gtk_list_view_get_model (self->plugin_list_view)));
  if (!gobj)
    return;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  PluginDescriptor * descr = (PluginDescriptor *) wrapped_obj->obj;

  char * type_label = g_strdup_printf (
    "%s • %s%s", descr->category_str, plugin_protocol_to_str (descr->protocol),
    descr->arch == ARCH_32 ? " (32-bit)" : "");
  char * audio_label =
    g_strdup_printf ("%d, %d", descr->num_audio_ins, descr->num_audio_outs);
  char * midi_label =
    g_strdup_printf ("%d, %d", descr->num_midi_ins, descr->num_midi_outs);
  char * ctrl_label =
    g_strdup_printf ("%d, %d", descr->num_ctrl_ins, descr->num_ctrl_outs);
  char * cv_label =
    g_strdup_printf ("%d, %d", descr->num_cv_ins, descr->num_cv_outs);

  gtk_label_set_text (self->plugin_author_label, descr->author);
  gtk_label_set_text (self->plugin_type_label, type_label);
  gtk_label_set_text (self->plugin_audio_label, audio_label);
  gtk_label_set_text (self->plugin_midi_label, midi_label);
  gtk_label_set_text (self->plugin_ctrl_label, ctrl_label);
  gtk_label_set_text (self->plugin_cv_label, cv_label);

  g_free (type_label);
  g_free (audio_label);
  g_free (midi_label);
  g_free (ctrl_label);
  g_free (cv_label);
}

static PluginBrowserSortStyle
get_sort_style (void)
{
  return g_settings_get_enum (S_UI_PLUGIN_BROWSER, "plugin-browser-sort-style");
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

  SET_EMBLEM (collection, self->selected_collections->len);
  SET_EMBLEM (author, self->selected_authors->len);
  SET_EMBLEM (category, self->selected_categories->len);
  SET_EMBLEM (protocol, self->selected_protocols->len);

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
      g_debug ("no selection paths for key %s", key);
      return;
    }

  GtkSelectionModel * selection = gtk_list_view_get_model (list_view);
  gtk_selection_model_unselect_all (selection);
  int i = 0;
  for (i = 0; selection_paths[i] != NULL; i++)
    {
      char *  selection_path = selection_paths[i];
      guint64 val64 = g_ascii_strtoull (selection_path, NULL, 10);
      guint   val = (guint) val64;
      g_signal_handlers_block_by_func (selection, on_selection_changed, self);
      gtk_selection_model_select_item (selection, val, false);
      g_signal_handlers_unblock_by_func (selection, on_selection_changed, self);
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
  PluginDescriptor * descr = (PluginDescriptor *) wrapped_obj->obj;

  char tmp[600];
  sprintf (tmp, "%p", descr);
  GVariant * variant = g_variant_new_string (tmp);
  g_action_group_activate_action (
    G_ACTION_GROUP (zrythm_app), "plugin-browser-add-to-project", variant);
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
  PluginDescriptor * descr = (PluginDescriptor *) wrapped_obj->obj;

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
    && !string_contains_substr_case_insensitive (descr->name, text))
    {
      return false;
    }

  /* no filter, all visible */
  if (
    self->selected_categories->len == 0 && self->selected_authors->len == 0
    && self->selected_protocols->len == 0 && self->selected_collections->len == 0
    && !self->current_search && !instruments_active && !effects_active
    && !modulators_active && !midi_modifiers_active)
    {
      return true;
    }

  bool visible = false;

  /* not visible if plugin protocol is not one of
   * selected protocols */
  if (self->selected_protocols->len > 0)
    {
      visible = false;
      for (guint i = 0; i < self->selected_protocols->len; i++)
        {
          ZPluginProtocol * prot =
            &g_array_index (self->selected_protocols, ZPluginProtocol, i);
          if (descr->protocol == *prot)
            {
              visible = true;
              break;
            }
        }

      if (!visible)
        return false;
    }

  /* not visible if category selected and plugin doesn't
   * match */
  if (self->selected_categories->len > 0)
    {
      visible = false;
      for (guint i = 0; i < self->selected_categories->len; i++)
        {
          ZPluginCategory * cat =
            &g_array_index (self->selected_categories, ZPluginCategory, i);
          if (descr->category == *cat)
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

  /* not visible if author selected and plugin doesn't
   * match */
  if (self->selected_authors->len > 0)
    {
      if (!descr->author)
        return false;

      visible = false;
      uint32_t author_sym = symap_map (self->symap, descr->author);
      for (guint i = 0; i < self->selected_authors->len; i++)
        {
          uint32_t * sym = &g_array_index (self->selected_authors, uint32_t, i);
          if (*sym == author_sym)
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

  /* not visible if plugin is not part of
   * selected collection */
  if (self->selected_collections->len > 0)
    {
      visible = false;
      for (guint i = 0; i < self->selected_collections->len; i++)
        {
          PluginCollection * coll = (PluginCollection *) g_ptr_array_index (
            self->selected_collections, i);

          if (plugin_collection_contains_descriptor (coll, descr, false))
            {
              visible = true;
              break;
            }
        }

      if (!visible)
        return false;
    }

  /* not visible if plugin type doesn't match */
  if (instruments_active && !plugin_descriptor_is_instrument (descr))
    return false;
  if (effects_active && !plugin_descriptor_is_effect (descr))
    return false;
  if (modulators_active && !plugin_descriptor_is_modulator (descr))
    return false;
  if (midi_modifiers_active && !plugin_descriptor_is_midi_modifier (descr))
    return false;

  if (
    self->current_search
    && !string_contains_substr_case_insensitive (
      descr->name, self->current_search))
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
  PluginDescriptor * descr_a = (PluginDescriptor *) wrapped_obj_a->obj;
  WrappedObjectWithChangeSignal * wrapped_obj_b =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL ((void *) _b);
  PluginDescriptor * descr_b = (PluginDescriptor *) wrapped_obj_b->obj;

  PluginBrowserSortStyle sort_style = get_sort_style ();
  if (sort_style == PLUGIN_BROWSER_SORT_ALPHA)
    {
      return string_utf8_strcasecmp (descr_a->name, descr_b->name);
    }
  else
    {
      PluginSetting * setting_a = plugin_setting_new_default (descr_a);
      PluginSetting * setting_b = plugin_setting_new_default (descr_b);

      int ret = -1;

      /* higher values are sorted higher */
      if (sort_style == PLUGIN_BROWSER_SORT_LAST_USED)
        {
          gint64 ret64 =
            setting_b->last_instantiated_time
            - setting_a->last_instantiated_time;
          if (ret64 == 0)
            ret = 0;
          else if (ret64 > 0)
            ret = 1;
          else
            ret = -1;
        }
      else if (sort_style == PLUGIN_BROWSER_SORT_MOST_USED)
        {
          ret = setting_b->num_instantiations - setting_a->num_instantiations;
        }
      else
        {
          g_return_val_if_reached (-1);
        }

      plugin_setting_free (setting_a);
      plugin_setting_free (setting_b);

      return ret;
    }

  g_return_val_if_reached (-1);
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
      g_array_remove_range (
        self->selected_categories, 0, self->selected_categories->len);
      for (guint64 i = 0; i < num_selected; i++)
        {
          guint             idx = gtk_bitset_get_nth (bitset, i);
          AdwEnumListItem * item = ADW_ENUM_LIST_ITEM (
            g_list_model_get_item (G_LIST_MODEL (selection_model), idx));
          ZPluginCategory cat = adw_enum_list_item_get_value (item);
          g_array_append_val (self->selected_categories, cat);
        }

      if (update_ui)
        {
          gtk_filter_changed (
            GTK_FILTER (self->plugin_filter), GTK_FILTER_CHANGE_DIFFERENT);
        }
    }
  else if (selection_model == gtk_list_view_get_model (self->author_list_view))
    {
      g_array_remove_range (
        self->selected_authors, 0, self->selected_authors->len);
      for (guint64 i = 0; i < num_selected; i++)
        {
          guint             idx = gtk_bitset_get_nth (bitset, i);
          GtkStringObject * str_obj = GTK_STRING_OBJECT (
            g_list_model_get_item (G_LIST_MODEL (selection_model), idx));
          uint32_t sym =
            symap_map (self->symap, gtk_string_object_get_string (str_obj));
          g_array_append_val (self->selected_authors, sym);
        }

      if (update_ui)
        {
          gtk_filter_changed (
            GTK_FILTER (self->plugin_filter), GTK_FILTER_CHANGE_DIFFERENT);
        }
    }
  else if (selection_model == gtk_list_view_get_model (self->protocol_list_view))
    {
      g_array_remove_range (
        self->selected_protocols, 0, self->selected_protocols->len);
      for (guint64 i = 0; i < num_selected; i++)
        {
          guint             idx = gtk_bitset_get_nth (bitset, i);
          AdwEnumListItem * item = ADW_ENUM_LIST_ITEM (
            g_list_model_get_item (G_LIST_MODEL (selection_model), idx));
          ZPluginProtocol prot = adw_enum_list_item_get_value (item);
          g_array_append_val (self->selected_protocols, prot);
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
      g_ptr_array_remove_range (
        self->selected_collections, 0, self->selected_collections->len);
      for (guint64 i = 0; i < num_selected; i++)
        {
          guint                           idx = gtk_bitset_get_nth (bitset, i);
          WrappedObjectWithChangeSignal * wobj =
            Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
              g_list_model_get_item (G_LIST_MODEL (selection_model), idx));
          const PluginCollection * coll = (const PluginCollection *) wobj->obj;
          if (coll)
            {
              g_ptr_array_add (self->selected_collections, (gpointer) coll);
            }
          else
            {
              g_critical ("collection not found");
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
      GObject * gobj = gtk_single_selection_get_selected_item (
        GTK_SINGLE_SELECTION (selection_model));
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

  PluginCollections * collections = PLUGIN_MANAGER->collections;
  g_return_val_if_fail (collections, NULL);
  for (int i = 0; i < collections->num_collections; i++)
    {
      PluginCollection * collection = collections->collections[i];

      /* add row to model */
      WrappedObjectWithChangeSignal * wobj =
        wrapped_object_with_change_signal_new (
          collection, WRAPPED_OBJECT_TYPE_PLUGIN_COLLECTION);
      g_list_store_append (list_store, wobj);
    }

  GtkMultiSelection * sel = gtk_multi_selection_new (G_LIST_MODEL (list_store));

  return GTK_SELECTION_MODEL (sel);
}

static GtkSelectionModel *
create_model_for_categories (void)
{
  AdwEnumListModel *  model = adw_enum_list_model_new (Z_TYPE_PLUGIN_CATEGORY);
  GtkMultiSelection * sel = gtk_multi_selection_new (G_LIST_MODEL (model));

  return GTK_SELECTION_MODEL (sel);
}

static GtkSelectionModel *
create_model_for_authors (void)
{
  GtkStringList * string_list = gtk_string_list_new (NULL);
  for (int i = 0; i < PLUGIN_MANAGER->num_plugin_authors; i++)
    {
      const gchar * name = PLUGIN_MANAGER->plugin_authors[i];
      gtk_string_list_append (string_list, name);
    }

  GtkMultiSelection * sel = gtk_multi_selection_new (G_LIST_MODEL (string_list));

  return GTK_SELECTION_MODEL (sel);
}

static GtkSelectionModel *
create_model_for_protocols (void)
{
  AdwEnumListModel *  model = adw_enum_list_model_new (Z_TYPE_PLUGIN_PROTOCOL);
  GtkMultiSelection * sel = gtk_multi_selection_new (G_LIST_MODEL (model));

  return GTK_SELECTION_MODEL (sel);
}

static GtkSelectionModel *
create_model_for_plugins (PluginBrowserWidget * self)
{
  GListStore * store = g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  for (size_t i = 0; i < PLUGIN_MANAGER->plugin_descriptors->len; i++)
    {
      PluginDescriptor * descr =
        g_ptr_array_index (PLUGIN_MANAGER->plugin_descriptors, i);

      WrappedObjectWithChangeSignal * wrapped_descr =
        wrapped_object_with_change_signal_new (
          descr, WRAPPED_OBJECT_TYPE_PLUGIN_DESCR);

      g_list_store_append (store, wrapped_descr);
    }

  /* create sorter */
  self->plugin_sorter = gtk_custom_sorter_new (plugin_sort_func, self, NULL);
  self->plugin_sort_model = gtk_sort_list_model_new (
    G_LIST_MODEL (store), GTK_SORTER (self->plugin_sorter));

  /* create filter */
  self->plugin_filter = gtk_custom_filter_new (
    (GtkCustomFilterFunc) plugin_filter_func, self, NULL);
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
          ItemFactory * factory = item_factory_new (
            self->plugin_list_view == list_view
              ? ITEM_FACTORY_ICON_AND_TEXT
              : ITEM_FACTORY_TEXT,
            false, NULL);
          gtk_list_view_set_factory (list_view, factory->list_item_factory);
          g_ptr_array_add (self->item_factories, factory);
        }
      else if (list_view == self->author_list_view)
        {
          gtk_list_view_set_factory (
            list_view, string_list_item_factory_new (NULL));
        }
      else if (list_view == self->category_list_view)
        {
          gtk_list_view_set_factory (
            list_view,
            string_list_item_factory_new ((
              StringListItemFactoryEnumStringGetter) plugin_category_to_string));
        }
      else if (list_view == self->protocol_list_view)
        {
          gtk_list_view_set_factory (
            list_view,
            string_list_item_factory_new (
              (StringListItemFactoryEnumStringGetter) plugin_protocol_to_str));
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
  g_ptr_array_remove_range (
    self->selected_collections, 0, self->selected_collections->len);
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
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER, "plugin-browser-tab",
        PLUGIN_BROWSER_TAB_COLLECTION);
    }
  else if (child == GTK_WIDGET (self->author_box))
    {
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER, "plugin-browser-tab", PLUGIN_BROWSER_TAB_AUTHOR);
    }
  else if (child == GTK_WIDGET (self->category_box))
    {
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER, "plugin-browser-tab", PLUGIN_BROWSER_TAB_CATEGORY);
    }
  else if (child == GTK_WIDGET (self->protocol_box))
    {
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER, "plugin-browser-tab", PLUGIN_BROWSER_TAB_PROTOCOL);
    }
}

static void
toggles_changed (GtkToggleButton * btn, PluginBrowserWidget * self)
{
  g_return_if_fail (GTK_IS_TOGGLE_BUTTON (btn));
  g_return_if_fail (Z_IS_PLUGIN_BROWSER_WIDGET (self));
  if (gtk_toggle_button_get_active (btn))
    {
      /* block signals, unset all, unblock */
      g_signal_handlers_block_by_func (
        self->toggle_instruments, toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_effects, toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_modulators, toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_midi_modifiers, toggles_changed, self);

      if (btn == self->toggle_instruments)
        {
          S_SET_ENUM (
            S_UI_PLUGIN_BROWSER, "plugin-browser-filter",
            PLUGIN_BROWSER_FILTER_INSTRUMENT);
          gtk_toggle_button_set_active (self->toggle_effects, 0);
          gtk_toggle_button_set_active (self->toggle_modulators, 0);
          gtk_toggle_button_set_active (self->toggle_midi_modifiers, 0);
        }
      else if (btn == self->toggle_effects)
        {
          S_SET_ENUM (
            S_UI_PLUGIN_BROWSER, "plugin-browser-filter",
            PLUGIN_BROWSER_FILTER_EFFECT);
          gtk_toggle_button_set_active (self->toggle_instruments, 0);
          gtk_toggle_button_set_active (self->toggle_modulators, 0);
          gtk_toggle_button_set_active (self->toggle_midi_modifiers, 0);
        }
      else if (btn == self->toggle_modulators)
        {
          S_SET_ENUM (
            S_UI_PLUGIN_BROWSER, "plugin-browser-filter",
            PLUGIN_BROWSER_FILTER_MODULATOR);
          gtk_toggle_button_set_active (self->toggle_instruments, 0);
          gtk_toggle_button_set_active (self->toggle_effects, 0);
          gtk_toggle_button_set_active (self->toggle_midi_modifiers, 0);
        }
      else if (btn == self->toggle_midi_modifiers)
        {
          S_SET_ENUM (
            S_UI_PLUGIN_BROWSER, "plugin-browser-filter",
            PLUGIN_BROWSER_FILTER_MIDI_EFFECT);
          gtk_toggle_button_set_active (self->toggle_instruments, 0);
          gtk_toggle_button_set_active (self->toggle_effects, 0);
          gtk_toggle_button_set_active (self->toggle_modulators, 0);
        }

      g_signal_handlers_unblock_by_func (
        self->toggle_instruments, toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_effects, toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_modulators, toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_midi_modifiers, toggles_changed, self);
    }
  else
    {
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER, "plugin-browser-filter",
        PLUGIN_BROWSER_FILTER_NONE);
    }
  gtk_filter_changed (
    GTK_FILTER (self->plugin_filter), GTK_FILTER_CHANGE_DIFFERENT);
  update_plugin_info_label (self);
}

static void
on_map (GtkWidget * widget, PluginBrowserWidget * self)
{
  /*g_message ("PLUGIN MAP EVENT");*/
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
      /*g_message ("***************************got plugin position %d",*/
      /*divider_pos);*/
    }
  else
    {
      /* save the divide position */
      divider_pos = gtk_paned_get_position (self->paned);
      g_settings_set_int (S_UI, "browser-divider-position", divider_pos);
      /*g_message ("***************************set plugin position to %d",*/
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

  g_message ("key release");
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
  PluginBrowserSortStyle sort_style = PLUGIN_BROWSER_SORT_ALPHA;
  if (gtk_toggle_button_get_active (self->alpha_sort_btn))
    {
      sort_style = PLUGIN_BROWSER_SORT_ALPHA;
    }
  else if (gtk_toggle_button_get_active (self->last_used_sort_btn))
    {
      sort_style = PLUGIN_BROWSER_SORT_LAST_USED;
    }
  else if (gtk_toggle_button_get_active (self->most_used_sort_btn))
    {
      sort_style = PLUGIN_BROWSER_SORT_MOST_USED;
    }

  g_settings_set_enum (
    S_UI_PLUGIN_BROWSER, "plugin-browser-sort-style", sort_style);

  gtk_sorter_changed (
    GTK_SORTER (self->plugin_sorter), GTK_SORTER_CHANGE_DIFFERENT);
}

PluginBrowserWidget *
plugin_browser_widget_new (void)
{
  PluginBrowserWidget * self = g_object_new (PLUGIN_BROWSER_WIDGET_TYPE, NULL);

  self->symap = symap_new ();

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
    NULL);
  g_signal_connect (
    G_OBJECT (self->plugin_search_entry), "search-changed",
    G_CALLBACK (on_plugin_search_changed), self);

  /* set the selected values */
  PluginBrowserTab tab = S_GET_ENUM (S_UI_PLUGIN_BROWSER, "plugin-browser-tab");
  PluginBrowserFilter filter =
    S_GET_ENUM (S_UI_PLUGIN_BROWSER, "plugin-browser-filter");
  restore_selections (self);

  switch (tab)
    {
    case PLUGIN_BROWSER_TAB_COLLECTION:
      adw_view_stack_set_visible_child (
        self->stack, GTK_WIDGET (self->collection_box));
      break;
    case PLUGIN_BROWSER_TAB_AUTHOR:
      adw_view_stack_set_visible_child (
        self->stack, GTK_WIDGET (self->author_box));
      break;
    case PLUGIN_BROWSER_TAB_CATEGORY:
      adw_view_stack_set_visible_child (
        self->stack, GTK_WIDGET (self->category_box));
      break;
    case PLUGIN_BROWSER_TAB_PROTOCOL:
      adw_view_stack_set_visible_child (
        self->stack, GTK_WIDGET (self->protocol_box));
      break;
    default:
      g_warn_if_reached ();
      break;
    }

  switch (filter)
    {
    case PLUGIN_BROWSER_FILTER_NONE:
      break;
    case PLUGIN_BROWSER_FILTER_INSTRUMENT:
      gtk_toggle_button_set_active (self->toggle_instruments, 1);
      break;
    case PLUGIN_BROWSER_FILTER_EFFECT:
      gtk_toggle_button_set_active (self->toggle_effects, 1);
      break;
    case PLUGIN_BROWSER_FILTER_MODULATOR:
      gtk_toggle_button_set_active (self->toggle_modulators, 1);
      break;
    case PLUGIN_BROWSER_FILTER_MIDI_EFFECT:
      gtk_toggle_button_set_active (self->toggle_midi_modifiers, 1);
      break;
    }

  /* set divider position */
  int divider_pos = g_settings_get_int (S_UI, "browser-divider-position");
  gtk_paned_set_position (self->paned, divider_pos);
  self->first_time_position_set = 1;
  g_message ("setting plugin browser divider pos to %d", divider_pos);

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
  object_free_w_func_and_null (symap_free, self->symap);
  object_free_w_func_and_null (g_array_unref, self->selected_authors);
  object_free_w_func_and_null (g_array_unref, self->selected_categories);
  object_free_w_func_and_null (g_array_unref, self->selected_protocols);
  object_free_w_func_and_null (g_ptr_array_unref, self->selected_collections);
  object_free_w_func_and_null (g_ptr_array_unref, self->item_factories);

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
  gtk_widget_init_template (GTK_WIDGET (self));

  self->popover_menu = GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
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
    case PLUGIN_BROWSER_SORT_ALPHA:
      gtk_toggle_button_set_active (self->alpha_sort_btn, true);
      break;
    case PLUGIN_BROWSER_SORT_LAST_USED:
      gtk_toggle_button_set_active (self->last_used_sort_btn, true);
      break;
    case PLUGIN_BROWSER_SORT_MOST_USED:
      gtk_toggle_button_set_active (self->most_used_sort_btn, true);
      break;
    }

  self->selected_authors = g_array_new (false, true, sizeof (uint32_t));
  self->selected_categories =
    g_array_new (false, true, sizeof (ZPluginCategory));
  self->selected_protocols = g_array_new (false, true, sizeof (ZPluginProtocol));
  self->selected_collections = g_ptr_array_new ();

  self->item_factories = g_ptr_array_new_with_free_func (item_factory_free_func);

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
