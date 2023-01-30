// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "actions/tracklist_selections.h"
#include "audio/engine.h"
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

G_DEFINE_TYPE (
  PluginBrowserWidget,
  plugin_browser_widget,
  GTK_TYPE_WIDGET)

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
#define AUTHORS_ICON "user"

/**
 * Updates the label below the list of plugins with
 * the plugin info.
 */
static void
update_plugin_info_label (PluginBrowserWidget * self)
{
  GObject * gobj = gtk_single_selection_get_selected_item (
    GTK_SINGLE_SELECTION (self->plugin_selection_model));
  if (!gobj)
    return;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  PluginDescriptor * descr =
    (PluginDescriptor *) wrapped_obj->obj;

  char * label = g_strdup_printf (
    "%s\n%s, %s%s\nAudio: %d, %d\nMidi: %d, "
    "%d\nControls: %d, %d\nCV: %d, %d",
    descr->author, descr->category_str,
    plugin_protocol_to_str (descr->protocol),
    descr->arch == ARCH_32 ? " (32-bit)" : "",
    descr->num_audio_ins, descr->num_audio_outs,
    descr->num_midi_ins, descr->num_midi_outs,
    descr->num_ctrl_ins, descr->num_ctrl_outs,
    descr->num_cv_ins, descr->num_cv_outs);
  gtk_label_set_text (self->plugin_info, label);
  g_free (label);
}

static PluginBrowserSortStyle
get_sort_style (void)
{
  return g_settings_get_enum (
    S_UI_PLUGIN_BROWSER, "plugin-browser-sort-style");
}

static void
update_stack_switcher_emblems (PluginBrowserWidget * self)
{
  int num_collections = gtk_tree_selection_count_selected_rows (
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->collection_tree_view)));
  int num_authors = gtk_tree_selection_count_selected_rows (
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->author_tree_view)));
  int num_categories = gtk_tree_selection_count_selected_rows (
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->category_tree_view)));
  int num_protocols = gtk_tree_selection_count_selected_rows (
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->protocol_tree_view)));

  AdwViewStackPage * page;

#define SET_EMBLEM(scroll_name, num) \
  page = adw_view_stack_get_page ( \
    self->stack, GTK_WIDGET (self->scroll_name##_scroll)); \
  adw_view_stack_page_set_badge_number ( \
    page, (unsigned int) num); \
  adw_view_stack_page_set_needs_attention (page, num > 0)

  SET_EMBLEM (collection, num_collections);
  SET_EMBLEM (author, num_authors);
  SET_EMBLEM (category, num_categories);
  SET_EMBLEM (protocol, num_protocols);

#undef SET_EMBLEM
}

static void
save_tree_view_selection (
  PluginBrowserWidget * self,
  GtkTreeView *         tree_view,
  const char *          key)
{
  GtkTreeSelection * selection;
  GList *            selected_rows;
  char *             path_strings[60000];
  int                num_path_strings = 0;

  selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  selected_rows =
    gtk_tree_selection_get_selected_rows (selection, NULL);
  for (GList * l = selected_rows; l != NULL;
       l = g_list_next (l))
    {
      GtkTreePath * tp = (GtkTreePath *) l->data;
      char *        path_str = gtk_tree_path_to_string (tp);
      path_strings[num_path_strings++] = path_str;
    }
  path_strings[num_path_strings] = NULL;
  g_settings_set_strv (
    S_UI_PLUGIN_BROWSER, key,
    (const char * const *) path_strings);
  for (int i = 0; i < num_path_strings; i++)
    {
      g_free (path_strings[i]);
      path_strings[i] = NULL;
    }
}

/**
 * Save (remember) the current selections.
 */
static void
save_tree_view_selections (PluginBrowserWidget * self)
{
  save_tree_view_selection (
    self, self->collection_tree_view,
    "plugin-browser-collections");
  save_tree_view_selection (
    self, self->author_tree_view, "plugin-browser-authors");
  save_tree_view_selection (
    self, self->category_tree_view,
    "plugin-browser-categories");
  save_tree_view_selection (
    self, self->protocol_tree_view,
    "plugin-browser-protocols");
}

/**
 * Restores a selection from the gsettings.
 */
static void
restore_tree_view_selection (
  PluginBrowserWidget * self,
  GtkTreeView *         tree_view,
  const char *          key)
{
  char ** selection_paths =
    g_settings_get_strv (S_UI_PLUGIN_BROWSER, key);
  if (!selection_paths)
    return;

  GtkTreeSelection * selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  for (int i = 0; selection_paths[i] != NULL; i++)
    {
      char *        selection_path = selection_paths[i];
      GtkTreePath * tp =
        gtk_tree_path_new_from_string (selection_path);
      gtk_tree_selection_select_path (selection, tp);
    }

  g_strfreev (selection_paths);
}

static void
restore_tree_view_selections (PluginBrowserWidget * self)
{
  restore_tree_view_selection (
    self, self->collection_tree_view,
    "plugin-browser-collections");
  restore_tree_view_selection (
    self, self->author_tree_view, "plugin-browser-authors");
  restore_tree_view_selection (
    self, self->category_tree_view,
    "plugin-browser-categories");
  restore_tree_view_selection (
    self, self->protocol_tree_view,
    "plugin-browser-protocols");
  update_plugin_info_label (self);
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
  PluginBrowserWidget * self =
    Z_PLUGIN_BROWSER_WIDGET (user_data);

  GObject * gobj = g_list_model_get_object (
    G_LIST_MODEL (self->plugin_selection_model), position);
  if (!gobj)
    return;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  PluginDescriptor * descr =
    (PluginDescriptor *) wrapped_obj->obj;
  self->current_descriptors[0] = descr;
  self->num_current_descriptors = 1;

  char tmp[600];
  sprintf (tmp, "%p", descr);
  GVariant * variant = g_variant_new_string (tmp);
  g_action_group_activate_action (
    G_ACTION_GROUP (zrythm_app),
    "plugin-browser-add-to-project", variant);
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
  PluginBrowserWidget * self =
    Z_PLUGIN_BROWSER_WIDGET (user_data);
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (item);
  PluginDescriptor * descr =
    (PluginDescriptor *) wrapped_obj->obj;

  int instruments_active, effects_active, modulators_active,
    midi_modifiers_active;
  instruments_active =
    gtk_toggle_button_get_active (self->toggle_instruments);
  effects_active =
    gtk_toggle_button_get_active (self->toggle_effects);
  modulators_active =
    gtk_toggle_button_get_active (self->toggle_modulators);
  midi_modifiers_active = gtk_toggle_button_get_active (
    self->toggle_midi_modifiers);

  /* filter by name */
  const char * text = gtk_editable_get_text (
    GTK_EDITABLE (self->plugin_search_entry));
  if (
    text && strlen (text) > 0
    && !string_contains_substr_case_insensitive (
      descr->name, text))
    {
      return false;
    }

  /* no filter, all visible */
  if (
    self->num_selected_categories == 0
    && self->num_selected_authors == 0
    && self->num_selected_protocols == 0
    && !self->selected_collection && !self->current_search
    && !instruments_active && !effects_active
    && !modulators_active && !midi_modifiers_active)
    {
      return true;
    }

  int visible = false;

  /* not visible if category selected and plugin
   * doesn't match */
  if (self->num_selected_categories > 0)
    {
      visible = false;
      for (int i = 0; i < self->num_selected_categories; i++)
        {
          if (descr->category == self->selected_categories[i])
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

  /* not visible if author selected and plugin
   * doesn't match */
  if (self->num_selected_authors > 0)
    {
      visible = false;
      if (descr->author)
        {
          for (int i = 0; i < self->num_selected_authors; i++)
            {
              if (
                symap_map (self->symap, descr->author)
                == self->selected_authors[i])
                {
                  visible = true;
                  break;
                }
            }
        }
      else
        {
          visible = false;
        }

      /* not visible if the author is not one
       * of the selected authors */
      if (!visible)
        return false;
    }

  /* not visible if plugin is not part of
   * selected collection */
  if (self->selected_collection)
    {
      if (plugin_collection_contains_descriptor (
            self->selected_collection, descr, false))
        {
          visible = true;
        }
      else
        {
          return false;
        }
    }

  /* not visible if plugin protocol is not one of
   * selected protocols */
  if (self->num_selected_protocols > 0)
    {
      visible = false;
      for (int i = 0; i < self->num_selected_protocols; i++)
        {
          if (descr->protocol == self->selected_protocols[i])
            {
              visible = true;
              break;
            }
        }

      if (!visible)
        return false;
    }

  /* not visible if plugin type doesn't match */
  if (
    instruments_active
    && !plugin_descriptor_is_instrument (descr))
    return false;
  if (effects_active && !plugin_descriptor_is_effect (descr))
    return false;
  if (modulators_active && !plugin_descriptor_is_modulator (descr))
    return false;
  if (
    midi_modifiers_active
    && !plugin_descriptor_is_midi_modifier (descr))
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
plugin_sort_func (
  const void * _a,
  const void * _b,
  gpointer     user_data)
{
  WrappedObjectWithChangeSignal * wrapped_obj_a =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL ((void *) _a);
  PluginDescriptor * descr_a =
    (PluginDescriptor *) wrapped_obj_a->obj;
  WrappedObjectWithChangeSignal * wrapped_obj_b =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL ((void *) _b);
  PluginDescriptor * descr_b =
    (PluginDescriptor *) wrapped_obj_b->obj;

  PluginBrowserSortStyle sort_style = get_sort_style ();
  if (sort_style == PLUGIN_BROWSER_SORT_ALPHA)
    {
      return string_utf8_strcasecmp (
        descr_a->name, descr_b->name);
    }
  else
    {
      PluginSetting * setting_a =
        plugin_setting_new_default (descr_a);
      PluginSetting * setting_b =
        plugin_setting_new_default (descr_b);

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
          ret =
            setting_b->num_instantiations
            - setting_a->num_instantiations;
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
show_collection_context_menu (
  PluginBrowserWidget * self,
  PluginCollection *    collection,
  double                x,
  double                y)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  if (collection)
    {
      self->current_collections[0] = collection;
      self->num_current_collections = 1;

      menuitem = z_gtk_create_menu_item (
        _ ("Rename"), "edit-rename",
        "app.plugin-collection-rename");
      g_menu_append_item (menu, menuitem);

      menuitem = z_gtk_create_menu_item (
        _ ("Delete"), "edit-delete",
        "app.plugin-collection-remove");
      g_menu_append_item (menu, menuitem);
    }
  else
    {
      self->num_current_collections = 0;

      menuitem = z_gtk_create_menu_item (
        _ ("Add"), "add", "app.plugin-collection-add");
      g_menu_append_item (menu, menuitem);
    }

  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x, y, menu);
}

static void
on_collection_right_click (
  GtkGestureClick *     gesture,
  gint                  n_press,
  gdouble               x_dbl,
  gdouble               y_dbl,
  PluginBrowserWidget * self)
{
  if (n_press != 1)
    return;

  GtkTreePath *       path;
  GtkTreeViewColumn * column;

  int x, y;
  gtk_tree_view_convert_widget_to_bin_window_coords (
    GTK_TREE_VIEW (self->collection_tree_view), (int) x_dbl,
    (int) y_dbl, &x, &y);

  GtkTreeSelection * selection = gtk_tree_view_get_selection (
    (self->collection_tree_view));
  bool have_selection = true;
  if (!gtk_tree_view_get_path_at_pos (
        GTK_TREE_VIEW (self->collection_tree_view), x, y,
        &path, &column, NULL, NULL))
    {
      /* no collection selected */
      have_selection = false;
    }

  PluginCollection * collection = NULL;

  gtk_tree_selection_unselect_all (selection);
  if (have_selection)
    {
      gtk_tree_selection_select_path (selection, path);
      GtkTreeIter iter;
      gtk_tree_model_get_iter (
        GTK_TREE_MODEL (self->collection_tree_model), &iter,
        path);
      int collection_idx = 0;
      gtk_tree_model_get (
        GTK_TREE_MODEL (self->collection_tree_model), &iter,
        1, &collection_idx, -1);
      gtk_tree_path_free (path);

      collection =
        PLUGIN_MANAGER->collections
          ->collections[collection_idx];
    }

  show_collection_context_menu (self, collection, x, y);
}

static void
show_category_context_menu (
  PluginBrowserWidget * self,
  double                x,
  double                y)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem = z_gtk_create_menu_item (
    _ ("Reset"), NULL, "app.plugin-browser-reset::category");
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x, y, menu);
}

static void
on_category_right_click (
  GtkGestureClick *     gesture,
  gint                  n_press,
  gdouble               x_dbl,
  gdouble               y_dbl,
  PluginBrowserWidget * self)
{
  if (n_press != 1)
    return;

  GtkTreePath *       path;
  GtkTreeViewColumn * column;

  int x, y;
  gtk_tree_view_convert_widget_to_bin_window_coords (
    GTK_TREE_VIEW (self->category_tree_view), (int) x_dbl,
    (int) y_dbl, &x, &y);

  if (!gtk_tree_view_get_path_at_pos (
        GTK_TREE_VIEW (self->category_tree_view), x, y, &path,
        &column, NULL, NULL))
    {
      g_message ("no path at position %d %d", x, y);
      return;
    }

  show_category_context_menu (self, x, y);
}

static void
show_author_context_menu (
  PluginBrowserWidget * self,
  double                x,
  double                y)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem = z_gtk_create_menu_item (
    _ ("Reset"), NULL, "app.plugin-browser-reset::author");
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x, y, menu);
}

static void
on_author_right_click (
  GtkGestureClick *     gesture,
  gint                  n_press,
  gdouble               x_dbl,
  gdouble               y_dbl,
  PluginBrowserWidget * self)
{
  if (n_press != 1)
    return;

  GtkTreePath *       path;
  GtkTreeViewColumn * column;

  int x, y;
  gtk_tree_view_convert_widget_to_bin_window_coords (
    GTK_TREE_VIEW (self->author_tree_view), (int) x_dbl,
    (int) y_dbl, &x, &y);

  if (!gtk_tree_view_get_path_at_pos (
        GTK_TREE_VIEW (self->author_tree_view), x, y, &path,
        &column, NULL, NULL))
    {
      g_message ("no path at position %d %d", x, y);
      return;
    }

  show_author_context_menu (self, x, y);
}

static void
cat_selected_foreach (
  GtkTreeModel *        model,
  GtkTreePath *         path,
  GtkTreeIter *         iter,
  PluginBrowserWidget * self)
{
  char * str;
  gtk_tree_model_get (model, iter, 0, &str, -1);

  self->selected_categories[self->num_selected_categories++] =
    plugin_descriptor_string_to_category (str);

  g_free (str);
}

static void
author_selected_foreach (
  GtkTreeModel *        model,
  GtkTreePath *         path,
  GtkTreeIter *         iter,
  PluginBrowserWidget * self)
{
  char * str;
  gtk_tree_model_get (model, iter, 0, &str, -1);

  self->selected_authors[self->num_selected_authors++] =
    symap_map (self->symap, str);

  g_free (str);
}

static void
protocol_selected_foreach (
  GtkTreeModel *        model,
  GtkTreePath *         path,
  GtkTreeIter *         iter,
  PluginBrowserWidget * self)
{
  PluginProtocol protocol;
  gtk_tree_model_get (model, iter, 2, &protocol, -1);

  self->selected_protocols[self->num_selected_protocols++] =
    protocol;
}

static void
on_tree_selection_changed (
  GtkTreeSelection *    ts,
  PluginBrowserWidget * self)
{
  GtkTreeView *  tv = gtk_tree_selection_get_tree_view (ts);
  GtkTreeModel * model = gtk_tree_view_get_model (tv);
  GList *        selected_rows =
    gtk_tree_selection_get_selected_rows (ts, NULL);

  if (model == self->category_tree_model)
    {
      self->num_selected_categories = 0;

      gtk_tree_selection_selected_foreach (
        ts, (GtkTreeSelectionForeachFunc) cat_selected_foreach,
        self);

      gtk_filter_changed (
        GTK_FILTER (self->plugin_filter),
        GTK_FILTER_CHANGE_DIFFERENT);
    }
  else if (model == self->author_tree_model)
    {
      self->num_selected_authors = 0;

      gtk_tree_selection_selected_foreach (
        ts,
        (GtkTreeSelectionForeachFunc) author_selected_foreach,
        self);

      gtk_filter_changed (
        GTK_FILTER (self->plugin_filter),
        GTK_FILTER_CHANGE_DIFFERENT);
    }
  else if (model == GTK_TREE_MODEL (self->protocol_tree_model))
    {
      self->num_selected_protocols = 0;

      gtk_tree_selection_selected_foreach (
        ts,
        (GtkTreeSelectionForeachFunc) protocol_selected_foreach,
        self);

      gtk_filter_changed (
        GTK_FILTER (self->plugin_filter),
        GTK_FILTER_CHANGE_DIFFERENT);
    }
  else if (model == GTK_TREE_MODEL (self->collection_tree_model))
    {
      if (selected_rows)
        {
          GtkTreeIter iter;
          gtk_tree_selection_get_selected (ts, NULL, &iter);
          int collection_idx = 0;
          gtk_tree_model_get (
            GTK_TREE_MODEL (self->collection_tree_model),
            &iter, 1, &collection_idx, -1);

          self->selected_collection =
            PLUGIN_MANAGER->collections
              ->collections[collection_idx];
        }
      else
        {
          self->selected_collection = NULL;
        }
      g_message (
        "selected collection: %s",
        self->selected_collection
          ? self->selected_collection->name
          : "none");

      gtk_filter_changed (
        GTK_FILTER (self->plugin_filter),
        GTK_FILTER_CHANGE_DIFFERENT);
    }

  g_list_free_full (
    selected_rows, (GDestroyNotify) gtk_tree_path_free);

  save_tree_view_selections (self);
  update_stack_switcher_emblems (self);
  update_plugin_info_label (self);
}

static void
on_plugin_selection_changed (
  GtkSelectionModel * selection_model,
  guint               position,
  guint               n_items,
  gpointer            user_data)
{
  PluginBrowserWidget * self =
    Z_PLUGIN_BROWSER_WIDGET (user_data);

  GObject * gobj = gtk_single_selection_get_selected_item (
    GTK_SINGLE_SELECTION (self->plugin_selection_model));
  if (!gobj)
    return;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  PluginDescriptor * descr =
    (PluginDescriptor *) wrapped_obj->obj;

  /* set as current descriptor */
  self->current_descriptors[0] = descr;
  self->num_current_descriptors = 1;

  /* update status label */
  update_plugin_info_label (self);
}

static GtkTreeModel *
create_model_for_favorites (void)
{
  /* list name, list index */
  GtkListStore * list_store =
    gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

  PluginCollections * collections =
    PLUGIN_MANAGER->collections;
  g_return_val_if_fail (collections, NULL);

  for (int i = 0; i < collections->num_collections; i++)
    {
      PluginCollection * collection =
        collections->collections[i];

      /* add row to model */
      GtkTreeIter iter;
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, collection->name, 1, i, -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_categories (void)
{
  /* plugin category */
  GtkListStore * list_store =
    gtk_list_store_new (1, G_TYPE_STRING);

  GtkTreeIter iter;

  for (int i = 0; i < PLUGIN_MANAGER->num_plugin_categories;
       i++)
    {
      const gchar * name =
        PLUGIN_MANAGER->plugin_categories[i];

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter, 0, name, -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_authors (void)
{
  /* plugin author */
  GtkListStore * list_store =
    gtk_list_store_new (1, G_TYPE_STRING);

  GtkTreeIter iter;

  for (int i = 0; i < PLUGIN_MANAGER->num_plugin_authors; i++)
    {
      const gchar * name = PLUGIN_MANAGER->plugin_authors[i];

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter, 0, name, -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_protocols (void)
{
  /* protocol icon, string, enum */
  GtkListStore * list_store = gtk_list_store_new (
    3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

  GtkTreeIter iter;

  for (PluginProtocol prot = PROT_LV2; prot <= PROT_JSFX;
       prot++)
    {
      const char * name = plugin_protocol_to_str (prot);

      const char * icon = NULL;
      switch (prot)
        {
        case PROT_LV2:
          icon = "logo-lv2";
          break;
        case PROT_LADSPA:
          icon = "logo-ladspa";
          break;
        case PROT_AU:
          icon = "logo-au";
          break;
        case PROT_VST:
        case PROT_VST3:
          icon = "logo-vst";
          break;
        case PROT_SFZ:
        case PROT_SF2:
          icon = "file-music-line";
          break;
        default:
          icon = "plug";
          break;
        }

      /* Add a new row to the model */
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, icon, 1, name, 2, prot, -1);
    }

  GtkTreeModel * model = gtk_tree_model_sort_new_with_model (
    GTK_TREE_MODEL (list_store));

  return model;
}

static GListModel *
create_model_for_plugins (PluginBrowserWidget * self)
{
  GListStore * store =
    g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  for (size_t i = 0;
       i < PLUGIN_MANAGER->plugin_descriptors->len; i++)
    {
      PluginDescriptor * descr = g_ptr_array_index (
        PLUGIN_MANAGER->plugin_descriptors, i);

      WrappedObjectWithChangeSignal * wrapped_descr =
        wrapped_object_with_change_signal_new (
          descr, WRAPPED_OBJECT_TYPE_PLUGIN_DESCR);

      g_list_store_append (store, wrapped_descr);
    }

  /* create sorter */
  self->plugin_sorter =
    gtk_custom_sorter_new (plugin_sort_func, self, NULL);
  self->plugin_sort_model = gtk_sort_list_model_new (
    G_LIST_MODEL (store), GTK_SORTER (self->plugin_sorter));

  /* create filter */
  self->plugin_filter = gtk_custom_filter_new (
    (GtkCustomFilterFunc) plugin_filter_func, self, NULL);
  self->plugin_filter_model = gtk_filter_list_model_new (
    G_LIST_MODEL (self->plugin_sort_model),
    GTK_FILTER (self->plugin_filter));

  /* create single selection */
  self->plugin_selection_model = gtk_single_selection_new (
    G_LIST_MODEL (self->plugin_filter_model));

  return G_LIST_MODEL (self->plugin_selection_model);
}

/* TODO */
#if 0
static bool
refilter_source (
  PluginBrowserWidget * self)
{
  gtk_filter_changed (
    GTK_FILTER (self->plugin_filter_model),
    GTK_FILTER_CHANGE_DIFFERENT);

  return G_SOURCE_REMOVE;
}

static gboolean
plugin_search_equal_func (
  GtkTreeModel *model,
  gint column,
  const gchar *key,
  GtkTreeIter *iter,
  PluginBrowserWidget * self)
{
  char * str;
  gtk_tree_model_get (
    model, iter, column, &str, -1);

  char * down_key =
    g_utf8_strdown (key, -1);
  char * down_str =
    g_utf8_strdown (str, -1);
  /*g_debug ("key '%s' str '%s'", key, str);*/

  g_free_and_null (self->current_search);
  self->current_search = g_strdup (key);

  bool match =
    string_contains_substr_case_insensitive (
      down_str, down_key);

  /* refilter treeview if this is the last
   * iteration of this func */
  GtkTreeIter next_iter = *iter;
  bool has_next =
    gtk_tree_model_iter_next (model, &next_iter);
  if (match || !has_next)
    {
      g_idle_add (
        (GSourceFunc) refilter_source, self);
    }

  g_free (str);
  g_free (down_key);
  g_free (down_str);

  return !match;
}
#endif

static void
plugin_list_view_setup (
  PluginBrowserWidget * self,
  GtkListView *         list_view,
  GtkSelectionModel *   selection_model)
{
  gtk_list_view_set_model (list_view, selection_model);
  self->plugin_item_factory = item_factory_new (
    ITEM_FACTORY_ICON_AND_TEXT, false, NULL);
  gtk_list_view_set_factory (
    list_view, self->plugin_item_factory->list_item_factory);

  g_signal_connect (
    G_OBJECT (selection_model), "selection-changed",
    G_CALLBACK (on_plugin_selection_changed), self);
}

/**
 * Sets up the given treeview.
 */
static void
tree_view_setup (
  PluginBrowserWidget * self,
  GtkTreeView *         tree_view,
  GtkTreeModel *        model,
  bool                  allow_multi)
{
  gtk_tree_view_set_model (tree_view, model);
  z_gtk_tree_view_remove_all_columns (tree_view);

  /* init tree view */
  GtkCellRenderer *   renderer;
  GtkTreeViewColumn * column;
  if (model == GTK_TREE_MODEL (self->protocol_tree_model))
    {
      /* column for icon */
      renderer = gtk_cell_renderer_pixbuf_new ();
      column = gtk_tree_view_column_new_with_attributes (
        "icon", renderer, "icon-name", 0, NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view), column);

      /* column for name */
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes (
        "name", renderer, "text", 1, NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view), column);

      /* set search column */
      gtk_tree_view_set_search_column (
        GTK_TREE_VIEW (tree_view), 1);

      gtk_tree_sortable_set_sort_column_id (
        GTK_TREE_SORTABLE (model), 1, GTK_SORT_ASCENDING);
    }
  else
    {
      /* column for name */
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes (
        "name", renderer, "text", 0, NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view), column);
    }

  /* hide headers and allow multi-selection */
  gtk_tree_view_set_headers_visible (
    GTK_TREE_VIEW (tree_view), false);

  if (allow_multi)
    {
      gtk_tree_selection_set_mode (
        gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)),
        GTK_SELECTION_MULTIPLE);
    }

  GtkTreeSelection * sel =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  g_signal_connect (
    G_OBJECT (sel), "changed",
    G_CALLBACK (on_tree_selection_changed), self);
}

void
plugin_browser_widget_refresh_collections (
  PluginBrowserWidget * self)
{
  self->collection_tree_model = create_model_for_favorites ();
  tree_view_setup (
    self, self->collection_tree_view,
    self->collection_tree_model, F_NO_MULTI_SELECT);
}

static void
on_visible_child_changed (
  AdwViewStack *        stack,
  GParamSpec *          pspec,
  PluginBrowserWidget * self)
{
  GtkWidget * child = adw_view_stack_get_visible_child (stack);

  if (child == GTK_WIDGET (self->collection_scroll))
    {
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER, "plugin-browser-tab",
        PLUGIN_BROWSER_TAB_COLLECTION);
    }
  else if (child == GTK_WIDGET (self->author_scroll))
    {
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER, "plugin-browser-tab",
        PLUGIN_BROWSER_TAB_AUTHOR);
    }
  else if (child == GTK_WIDGET (self->category_scroll))
    {
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER, "plugin-browser-tab",
        PLUGIN_BROWSER_TAB_CATEGORY);
    }
  else if (child == GTK_WIDGET (self->protocol_scroll))
    {
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER, "plugin-browser-tab",
        PLUGIN_BROWSER_TAB_PROTOCOL);
    }
}

static void
toggles_changed (
  GtkToggleButton *     btn,
  PluginBrowserWidget * self)
{
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
          gtk_toggle_button_set_active (
            self->toggle_effects, 0);
          gtk_toggle_button_set_active (
            self->toggle_modulators, 0);
          gtk_toggle_button_set_active (
            self->toggle_midi_modifiers, 0);
        }
      else if (btn == self->toggle_effects)
        {
          S_SET_ENUM (
            S_UI_PLUGIN_BROWSER, "plugin-browser-filter",
            PLUGIN_BROWSER_FILTER_EFFECT);
          gtk_toggle_button_set_active (
            self->toggle_instruments, 0);
          gtk_toggle_button_set_active (
            self->toggle_modulators, 0);
          gtk_toggle_button_set_active (
            self->toggle_midi_modifiers, 0);
        }
      else if (btn == self->toggle_modulators)
        {
          S_SET_ENUM (
            S_UI_PLUGIN_BROWSER, "plugin-browser-filter",
            PLUGIN_BROWSER_FILTER_MODULATOR);
          gtk_toggle_button_set_active (
            self->toggle_instruments, 0);
          gtk_toggle_button_set_active (
            self->toggle_effects, 0);
          gtk_toggle_button_set_active (
            self->toggle_midi_modifiers, 0);
        }
      else if (btn == self->toggle_midi_modifiers)
        {
          S_SET_ENUM (
            S_UI_PLUGIN_BROWSER, "plugin-browser-filter",
            PLUGIN_BROWSER_FILTER_MIDI_EFFECT);
          gtk_toggle_button_set_active (
            self->toggle_instruments, 0);
          gtk_toggle_button_set_active (
            self->toggle_effects, 0);
          gtk_toggle_button_set_active (
            self->toggle_modulators, 0);
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
    GTK_FILTER (self->plugin_filter),
    GTK_FILTER_CHANGE_DIFFERENT);
  update_plugin_info_label (self);
}

static void
on_map (GtkWidget * widget, PluginBrowserWidget * self)
{
  /*g_message ("PLUGIN MAP EVENT");*/
  if (gtk_widget_get_mapped (GTK_WIDGET (self)))
    {
      self->start_saving_pos = 1;
      self->first_time_position_set_time =
        g_get_monotonic_time ();

      /* set divider position */
      int divider_pos =
        g_settings_get_int (S_UI, "browser-divider-position");
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
      divider_pos =
        g_settings_get_int (S_UI, "browser-divider-position");
      gtk_paned_set_position (self->paned, divider_pos);

      self->first_time_position_set = 0;
      /*g_message ("***************************got plugin position %d",*/
      /*divider_pos);*/
    }
  else
    {
      /* save the divide position */
      divider_pos = gtk_paned_get_position (self->paned);
      g_settings_set_int (
        S_UI, "browser-divider-position", divider_pos);
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
    GTK_FILTER (self->plugin_filter),
    GTK_FILTER_CHANGE_DIFFERENT);
  update_plugin_info_label (self);

  g_message ("key release");
}

static void
on_plugin_search_changed (
  GtkSearchEntry *      search_entry,
  PluginBrowserWidget * self)
{
  gtk_filter_changed (
    GTK_FILTER (self->plugin_filter),
    GTK_FILTER_CHANGE_DIFFERENT);
  update_plugin_info_label (self);
}

static void
on_sort_style_changed (
  GtkToggleButton *     btn,
  PluginBrowserWidget * self)
{
  PluginBrowserSortStyle sort_style =
    PLUGIN_BROWSER_SORT_ALPHA;
  if (gtk_toggle_button_get_active (self->alpha_sort_btn))
    {
      sort_style = PLUGIN_BROWSER_SORT_ALPHA;
    }
  else if (gtk_toggle_button_get_active (
             self->last_used_sort_btn))
    {
      sort_style = PLUGIN_BROWSER_SORT_LAST_USED;
    }
  else if (gtk_toggle_button_get_active (
             self->most_used_sort_btn))
    {
      sort_style = PLUGIN_BROWSER_SORT_MOST_USED;
    }

  g_settings_set_enum (
    S_UI_PLUGIN_BROWSER, "plugin-browser-sort-style",
    sort_style);

  gtk_sorter_changed (
    GTK_SORTER (self->plugin_sorter),
    GTK_SORTER_CHANGE_DIFFERENT);
}

static void
dispose (PluginBrowserWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  G_OBJECT_CLASS (plugin_browser_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
finalize (PluginBrowserWidget * self)
{
  symap_free (self->symap);

  G_OBJECT_CLASS (plugin_browser_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

PluginBrowserWidget *
plugin_browser_widget_new (void)
{
  PluginBrowserWidget * self =
    g_object_new (PLUGIN_BROWSER_WIDGET_TYPE, NULL);

  self->symap = symap_new ();

  gtk_label_set_xalign (self->plugin_info, 0);

  /* setup collections */
  plugin_browser_widget_refresh_collections (self);

  /* connect right click handler */
  GtkGestureClick * mp =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_collection_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->collection_tree_view),
    GTK_EVENT_CONTROLLER (mp));

  /* setup protocols */
  self->protocol_tree_model =
    GTK_TREE_MODEL_SORT (create_model_for_protocols ());
  tree_view_setup (
    self, self->protocol_tree_view,
    GTK_TREE_MODEL (self->protocol_tree_model),
    F_MULTI_SELECT);

  /* setup authors */
  self->author_tree_model = create_model_for_authors ();
  tree_view_setup (
    self, self->author_tree_view, self->author_tree_model,
    F_MULTI_SELECT);

  /* connect right click handler */
  mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_author_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->author_tree_view),
    GTK_EVENT_CONTROLLER (mp));

  /* setup categories */
  self->category_tree_model = create_model_for_categories ();
  tree_view_setup (
    self, self->category_tree_view, self->category_tree_model,
    F_MULTI_SELECT);

  /* connect right click handler */
  mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_category_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->category_tree_view),
    GTK_EVENT_CONTROLLER (mp));

  /* populate plugins */
  self->plugin_selection_model =
    GTK_SINGLE_SELECTION (create_model_for_plugins (self));
  plugin_list_view_setup (
    self, self->plugin_list_view,
    GTK_SELECTION_MODEL (self->plugin_selection_model));
  g_signal_connect (
    G_OBJECT (self->plugin_list_view), "activate",
    G_CALLBACK (on_plugin_row_activated), self);

  /* setup plugin search entry */
  gtk_search_entry_set_key_capture_widget (
    self->plugin_search_entry,
    GTK_WIDGET (self->plugin_list_view));
  g_object_set (
    G_OBJECT (self->plugin_search_entry), "placeholder-text",
    _ ("Search..."), NULL);
  g_signal_connect (
    G_OBJECT (self->plugin_search_entry), "search-changed",
    G_CALLBACK (on_plugin_search_changed), self);

  /* set the selected values */
  PluginBrowserTab tab =
    S_GET_ENUM (S_UI_PLUGIN_BROWSER, "plugin-browser-tab");
  PluginBrowserFilter filter =
    S_GET_ENUM (S_UI_PLUGIN_BROWSER, "plugin-browser-filter");
  restore_tree_view_selections (self);

  switch (tab)
    {
    case PLUGIN_BROWSER_TAB_COLLECTION:
      adw_view_stack_set_visible_child (
        self->stack, GTK_WIDGET (self->collection_scroll));
      break;
    case PLUGIN_BROWSER_TAB_AUTHOR:
      adw_view_stack_set_visible_child (
        self->stack, GTK_WIDGET (self->author_scroll));
      break;
    case PLUGIN_BROWSER_TAB_CATEGORY:
      adw_view_stack_set_visible_child (
        self->stack, GTK_WIDGET (self->category_scroll));
      break;
    case PLUGIN_BROWSER_TAB_PROTOCOL:
      adw_view_stack_set_visible_child (
        self->stack, GTK_WIDGET (self->protocol_scroll));
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
      gtk_toggle_button_set_active (
        self->toggle_instruments, 1);
      break;
    case PLUGIN_BROWSER_FILTER_EFFECT:
      gtk_toggle_button_set_active (self->toggle_effects, 1);
      break;
    case PLUGIN_BROWSER_FILTER_MODULATOR:
      gtk_toggle_button_set_active (
        self->toggle_modulators, 1);
      break;
    case PLUGIN_BROWSER_FILTER_MIDI_EFFECT:
      gtk_toggle_button_set_active (
        self->toggle_midi_modifiers, 1);
      break;
    }

  /* set divider position */
  int divider_pos =
    g_settings_get_int (S_UI, "browser-divider-position");
  gtk_paned_set_position (self->paned, divider_pos);
  self->first_time_position_set = 1;
  g_message (
    "setting plugin browser divider pos to %d", divider_pos);

  /* re-sort */
  gtk_sorter_changed (
    GTK_SORTER (self->plugin_sorter),
    GTK_SORTER_CHANGE_DIFFERENT);

  /* notify when tab changes */
  g_signal_connect (
    G_OBJECT (self->stack), "notify::visible-child",
    G_CALLBACK (on_visible_child_changed), self);
  g_signal_connect (
    G_OBJECT (self), "notify::position",
    G_CALLBACK (on_position_change), self);
  g_signal_connect (
    G_OBJECT (self), "map", G_CALLBACK (on_map), self);

  GtkEventController * key_controller =
    gtk_event_controller_key_new ();
  g_signal_connect (
    G_OBJECT (key_controller), "key-released",
    G_CALLBACK (on_key_release), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), key_controller);

  return self;
}

static void
plugin_browser_widget_class_init (
  PluginBrowserWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "plugin_browser.ui");

  /*gtk_widget_class_add_shortcut (*/

#define BIND_CHILD(name) \
  gtk_widget_class_bind_template_child ( \
    klass, PluginBrowserWidget, name)

  BIND_CHILD (paned);
  BIND_CHILD (collection_scroll);
  BIND_CHILD (protocol_scroll);
  BIND_CHILD (category_scroll);
  BIND_CHILD (author_scroll);
  BIND_CHILD (plugin_scroll);
  BIND_CHILD (collection_tree_view);
  BIND_CHILD (protocol_tree_view);
  BIND_CHILD (category_tree_view);
  BIND_CHILD (author_tree_view);
  BIND_CHILD (plugin_search_entry);
  BIND_CHILD (plugin_list_view);
  BIND_CHILD (browser_bot);
  BIND_CHILD (plugin_info);
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

#define BIND_SIGNAL(sig) \
  gtk_widget_class_bind_template_callback (klass, sig)

  BIND_SIGNAL (toggles_changed);

#undef BIND_SIGNAL

  gtk_widget_class_set_layout_manager_type (
    klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * goklass = G_OBJECT_CLASS (_klass);
  goklass->dispose = (GObjectFinalizeFunc) dispose;
  goklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
plugin_browser_widget_init (PluginBrowserWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (
    GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  self->stack_switcher =
    ADW_VIEW_SWITCHER (adw_view_switcher_new ());
  adw_view_switcher_set_stack (
    self->stack_switcher, self->stack);
  gtk_box_append (
    self->stack_switcher_box,
    GTK_WIDGET (self->stack_switcher));

  gtk_widget_set_hexpand (GTK_WIDGET (self->paned), true);

  self->current_collections =
    calloc (40000, sizeof (PluginCollection *));
  self->current_descriptors =
    calloc (40000, sizeof (PluginDescriptor *));

  gtk_widget_add_css_class (
    GTK_WIDGET (self), "plugin-browser");

  /* add toggle group for sort buttons */
  gtk_toggle_button_set_group (
    self->last_used_sort_btn, self->alpha_sort_btn);
  gtk_toggle_button_set_group (
    self->most_used_sort_btn, self->alpha_sort_btn);
  gtk_toggle_button_set_active (self->alpha_sort_btn, true);

  /* use sort style from last time */
  PluginBrowserSortStyle sort_style = get_sort_style ();
  switch (sort_style)
    {
    case PLUGIN_BROWSER_SORT_ALPHA:
      gtk_toggle_button_set_active (
        self->alpha_sort_btn, true);
      break;
    case PLUGIN_BROWSER_SORT_LAST_USED:
      gtk_toggle_button_set_active (
        self->last_used_sort_btn, true);
      break;
    case PLUGIN_BROWSER_SORT_MOST_USED:
      gtk_toggle_button_set_active (
        self->most_used_sort_btn, true);
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
