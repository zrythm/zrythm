/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"

#include "actions/tracklist_selections.h"
#include "audio/engine.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/dialogs/string_entry_dialog.h"
#include "gui/widgets/expander_box.h"
#include "gui/widgets/plugin_browser.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  PluginBrowserWidget,
  plugin_browser_widget,
  GTK_TYPE_BOX)

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

void
plugin_browser_widget_refresh_collections (
  PluginBrowserWidget * self);

static void
update_stack_switcher_emblems (
  PluginBrowserWidget * self)
{
  /* TODO */
#if 0
  bool has_collections =
    gtk_tree_selection_count_selected_rows (
      gtk_tree_view_get_selection (
        GTK_TREE_VIEW (
          self->collection_tree_view)));
  bool has_authors =
    gtk_tree_selection_count_selected_rows (
      gtk_tree_view_get_selection (
        GTK_TREE_VIEW (
          self->author_tree_view)));
  bool has_categories =
    gtk_tree_selection_count_selected_rows (
      gtk_tree_view_get_selection (
        GTK_TREE_VIEW (
          self->category_tree_view)));
  bool has_protocols =
    gtk_tree_selection_count_selected_rows (
      gtk_tree_view_get_selection (
        GTK_TREE_VIEW (
          self->protocol_tree_view)));

  GList *children, *iter;
  children =
    gtk_container_get_children (
      GTK_CONTAINER (self->stack_switcher));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      if (!GTK_IS_RADIO_BUTTON (iter->data))
        continue;

      GtkButton * button = GTK_BUTTON (iter->data);

      GtkWidget * btn_child =
        gtk_bin_get_child (GTK_BIN (button));
      GtkImage * prev_img = NULL;
      if (GTK_IS_BIN (btn_child))
        {
          GtkWidget * inner_child =
            gtk_bin_get_child (
              GTK_BIN (btn_child));
          if (GTK_IS_CONTAINER (inner_child))
            {
              prev_img =
                GTK_IMAGE (
                  z_gtk_container_get_single_child (
                    GTK_CONTAINER (inner_child)));
            }
          else if (GTK_IS_IMAGE (inner_child))
            {
              prev_img = GTK_IMAGE (inner_child);
            }
          else
            {
              g_critical (
                "unknown type %s",
                G_OBJECT_TYPE_NAME (inner_child));
            }
        }
      else
        {
          prev_img =
            GTK_IMAGE (
              gtk_bin_get_child (GTK_BIN (button)));
        }

      GtkIconSize icon_size;
      const char * icon_name = NULL;
      GtkImageType image_type =
        gtk_image_get_storage_type (prev_img);
      if (image_type == GTK_IMAGE_ICON_NAME)
        {
          gtk_image_get_icon_name (
            prev_img, &icon_name, &icon_size);
        }
      else if (image_type == GTK_IMAGE_GICON)
        {
          GIcon * emblemed_icon = NULL;
          gtk_image_get_gicon (
            prev_img, &emblemed_icon, &icon_size);
          g_return_if_fail (emblemed_icon);
          GIcon * prev_icon = NULL;
          if (G_IS_EMBLEMED_ICON (emblemed_icon))
            {
              prev_icon =
                g_emblemed_icon_get_icon (
                  G_EMBLEMED_ICON (emblemed_icon));
            }
          else if (G_IS_THEMED_ICON (emblemed_icon))
            {
              prev_icon = emblemed_icon;
            }
          else
            {
              g_return_if_reached ();
            }

          const char * const * icon_names =
            g_themed_icon_get_names (
              G_THEMED_ICON (prev_icon));
          icon_name =  icon_names[0];
        }
      g_return_if_fail (icon_name);

      /* add emblem if needed */
      bool needs_emblem =
        (has_collections &&
         string_is_equal (
           icon_name, COLLECTIONS_ICON)) ||
        (has_protocols &&
         string_is_equal (
           icon_name, PROTOCOLS_ICON)) ||
        (has_authors &&
         string_is_equal (
           icon_name, AUTHORS_ICON)) ||
        (has_categories &&
         string_is_equal (
           icon_name, CATEGORIES_ICON));

      z_gtk_button_set_emblem (
        button,
        needs_emblem ? "media-record" : NULL);
    }
  g_list_free (children);
#endif
}

static void
save_tree_view_selection (
  PluginBrowserWidget * self,
  GtkTreeView *         tree_view,
  const char *          key)
{
  GtkTreeSelection * selection;
  GList * selected_rows;
  char * path_strings[60000];
  int num_path_strings = 0;

  selection =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (tree_view));
  selected_rows =
    gtk_tree_selection_get_selected_rows (
      selection, NULL);
  for (GList * l = selected_rows; l != NULL;
       l = g_list_next (l))
    {
      GtkTreePath * tp =
        (GtkTreePath *) l->data;
      char * path_str =
        gtk_tree_path_to_string (tp);
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
save_tree_view_selections (
  PluginBrowserWidget * self)
{
  save_tree_view_selection (
    self, self->collection_tree_view,
    "plugin-browser-collections");
  save_tree_view_selection (
    self, self->author_tree_view,
    "plugin-browser-authors");
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
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (tree_view));
  for (int i = 0; selection_paths[i] != NULL; i++)
    {
      char * selection_path = selection_paths[i];
      GtkTreePath * tp =
        gtk_tree_path_new_from_string (
          selection_path);
      gtk_tree_selection_select_path (
        selection, tp);
    }

  g_strfreev (selection_paths);
}

static void
restore_tree_view_selections (
  PluginBrowserWidget * self)
{
  restore_tree_view_selection (
    self, self->collection_tree_view,
    "plugin-browser-collections");
  restore_tree_view_selection (
    self, self->author_tree_view,
    "plugin-browser-authors");
  restore_tree_view_selection (
    self, self->category_tree_view,
    "plugin-browser-categories");
  restore_tree_view_selection (
    self, self->protocol_tree_view,
    "plugin-browser-protocols");
}

/**
 * Called when row is double clicked.
 */
static void
on_row_activated (
  GtkTreeView       *tree_view,
  GtkTreePath       *tp,
  GtkTreeViewColumn *column,
  gpointer           user_data)
{
  PluginBrowserWidget * self =
    Z_PLUGIN_BROWSER_WIDGET (user_data);
  GtkTreeModel * model = GTK_TREE_MODEL (user_data);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (model, &iter, tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    model, &iter, PL_COLUMN_DESCR, &value);
  PluginDescriptor * descr =
    g_value_get_pointer (&value);
  self->current_descriptors[0] = descr;
  self->num_current_descriptors = 1;

  g_action_group_activate_action (
    G_ACTION_GROUP (zrythm_app),
    "plugin-browser-add-to-project", NULL);
}

/**
 * Visible function for plugin tree model.
 *
 * Used for filtering based on selected category.
 */
static gboolean
visible_func (
  GtkTreeModel *model,
  GtkTreeIter  *iter,
  PluginBrowserWidget * self)
{
  // Visible if row is non-empty and category
  // matches selected
  PluginDescriptor *descr;

  gtk_tree_model_get (
    model, iter, PL_COLUMN_DESCR, &descr, -1);

  int instruments_active, effects_active,
      modulators_active, midi_modifiers_active;
  instruments_active =
    gtk_toggle_button_get_active (
      self->toggle_instruments);
  effects_active =
    gtk_toggle_button_get_active (
      self->toggle_effects);
  modulators_active =
    gtk_toggle_button_get_active (
      self->toggle_modulators);
  midi_modifiers_active =
    gtk_toggle_button_get_active (
      self->toggle_midi_modifiers);

  /* no filter, all visible */
  if (self->num_selected_categories == 0 &&
      self->num_selected_authors == 0 &&
      self->num_selected_protocols == 0 &&
      !self->selected_collection &&
      !self->current_search &&
      !instruments_active &&
      !effects_active &&
      !modulators_active &&
      !midi_modifiers_active)
    {
      return true;
    }

  int visible = false;

  /* not visible if category selected and plugin
   * doesn't match */
  if (self->num_selected_categories > 0)
    {
      visible = false;
      for (int i = 0;
           i < self->num_selected_categories; i++)
        {
          if (descr->category ==
                self->selected_categories[i])
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
          for (int i = 0;
               i < self->num_selected_authors; i++)
            {
              if (symap_map (
                    self->symap, descr->author) ==
                      self->selected_authors[i])
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
            self->selected_collection, descr,
            false))
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
      for (int i = 0;
           i < self->num_selected_protocols; i++)
        {
          if (descr->protocol ==
                self->selected_protocols[i])
            {
              visible = true;
              break;
            }
        }

      if (!visible)
        return false;
    }

  /* not visibile if plugin type doesn't match */
  if (instruments_active &&
      !plugin_descriptor_is_instrument (descr))
    return false;
  if (effects_active &&
      !plugin_descriptor_is_effect (descr))
    return false;
  if (modulators_active &&
      !plugin_descriptor_is_modulator (descr))
    return false;
  if (midi_modifiers_active &&
      !plugin_descriptor_is_midi_modifier (descr))
    return false;

  if (self->current_search &&
      !string_contains_substr_case_insensitive (
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

static void
show_plugin_context_menu (
  PluginBrowserWidget * self,
  PluginDescriptor *    descr,
  double                x,
  double                y)
{
  g_return_if_fail (self && descr);
  self->current_descriptors[0] = descr;
  self->num_current_descriptors = 1;

  GMenu * menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem =
    z_gtk_create_menu_item (
      _("Add to project"), NULL,
      "app.plugin-browser-add-to-project");
  g_menu_append_item (menu, menuitem);

#if 0
  if (descr->protocol == PROT_LV2 &&
      lv2_plugin_pick_most_preferable_ui (
        descr->uri, NULL, NULL,
        false))
    {
      char * uis[40];
      int num_uis = 0;
      lv2_plugin_get_uis (
        descr->uri, uis, &num_uis);

      GtkMenu * submenu = NULL;
      if (num_uis > 1)
        {
          menuitem =
            GTK_MENU_ITEM (
              gtk_menu_item_new_with_label (
                /* TRANSLATORS: Add plugin to project
                 * using the selected UI. This opens
                 * up a submenu to choose a UI */
                _("Add to project with UI")));
          APPEND;
          submenu =
            GTK_MENU (gtk_menu_new ());
          gtk_widget_set_visible (
            GTK_WIDGET (submenu), true);
        }

      for (int i = 0; i < num_uis; i++)
        {
          char * ui = uis[i];
          if (lv2_plugin_is_ui_supported (
                descr->uri, ui) &&
              num_uis > 1)
            {
              GtkMenuItem * submenu_item =
                GTK_MENU_ITEM (
                  gtk_menu_item_new_with_label (
                    g_strdup (ui)));
              gtk_widget_set_visible (
                GTK_WIDGET (submenu_item), true);
              gtk_menu_shell_append (
                GTK_MENU_SHELL (submenu),
                GTK_WIDGET (submenu_item));
              new_setting =
                plugin_setting_new_default (descr);
              new_setting->open_with_carla = false;
              new_setting->bridge_mode =
                CARLA_BRIDGE_NONE;
              g_free_and_null (new_setting->ui_uri);
              new_setting->ui_uri = g_strdup (ui);
              CONNECT_SIGNAL (submenu_item);
            }
          g_free (ui);
        }
      if (num_uis > 1)
        {
          gtk_menu_item_set_submenu (
            menuitem, GTK_WIDGET (submenu));
        }
    }
#endif

#ifdef HAVE_CARLA
  menuitem =
    z_gtk_create_menu_item (
      _("Add to project (carla)"), NULL,
      "app.plugin-browser-add-to-project-carla");
  g_menu_append_item (menu, menuitem);

  PluginSetting * new_setting =
    plugin_setting_new_default (descr);
  if (descr->has_custom_ui &&
      descr->min_bridge_mode ==
        CARLA_BRIDGE_NONE &&
      !new_setting->force_generic_ui)
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Add to project (carla)"), NULL,
          "app.plugin-browser-add-to-project-bridged-ui");
      g_menu_append_item (menu, menuitem);
    }
  plugin_setting_free (new_setting);

  menuitem =
    z_gtk_create_menu_item (
      _("Add to project (carla)"), NULL,
      "app.plugin-browser-add-to-project-bridged-full");
  g_menu_append_item (menu, menuitem);
#endif

#if 0
  menuitem =
    GTK_MENU_ITEM (
      gtk_check_menu_item_new_with_mnemonic (
        _("Use _Generic UI")));
  APPEND;
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (menuitem),
    new_setting->force_generic_ui);
  g_signal_connect (
    G_OBJECT (menuitem), "toggled",
    G_CALLBACK (on_use_generic_ui_toggled), descr);
#endif

  /* add to collection */
  GMenu * add_collections_submenu = g_menu_new ();
  int num_added = 0;
  for (int i = 0;
       i < PLUGIN_MANAGER->collections->
         num_collections;
       i++)
    {
      PluginCollection * coll =
        PLUGIN_MANAGER->collections->collections[i];
      if (plugin_collection_contains_descriptor (
            coll, descr, false))
        {
          continue;
        }

      char tmp[600];
      sprintf (
        tmp,
        "app.plugin-browser-add-to-collection::%p",
        coll);
      menuitem =
        z_gtk_create_menu_item (
          coll->name, NULL, tmp);
      g_menu_append_item (
        add_collections_submenu, menuitem);
      num_added++;
    }
  if (num_added > 0)
    {
      g_menu_append_section (
        menu, _("Add to collection"),
        G_MENU_MODEL (add_collections_submenu));
    }
  else
    {
      g_object_unref (add_collections_submenu);
    }

  /* remove from collection */
  GMenu * remove_collections_submenu = g_menu_new ();
  num_added = 0;
  for (int i = 0;
       i < PLUGIN_MANAGER->collections->
         num_collections;
       i++)
    {
      PluginCollection * coll =
        PLUGIN_MANAGER->collections->collections[i];
      if (!plugin_collection_contains_descriptor (
            coll, descr, false))
        {
          continue;
        }

      char tmp[600];
      sprintf (
        tmp,
        "app.plugin-browser-remove-from-collection::%p",
        coll);
      menuitem =
        z_gtk_create_menu_item (
          coll->name, NULL, tmp);
      g_menu_append_item (
        remove_collections_submenu, menuitem);
      num_added++;
    }
  if (num_added > 0)
    {
      g_menu_append_section (
        menu, _("Remove from collection"),
        G_MENU_MODEL (remove_collections_submenu));
    }
  else
    {
      g_object_unref (remove_collections_submenu);
    }

  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x, y, menu);
}

static void
on_plugin_right_click (
  GtkGestureClick * gesture,
  gint                   n_press,
  gdouble                x_dbl,
  gdouble                y_dbl,
  PluginBrowserWidget *  self)
{
  if (n_press != 1)
    return;

  GtkTreePath *path;
  GtkTreeViewColumn *column;

  int x, y;
  gtk_tree_view_convert_widget_to_bin_window_coords (
    GTK_TREE_VIEW (self->plugin_tree_view),
    (int) x_dbl, (int) y_dbl, &x, &y);

  GtkTreeSelection * selection =
    gtk_tree_view_get_selection (
      (self->plugin_tree_view));
  if (!gtk_tree_view_get_path_at_pos (
        GTK_TREE_VIEW (self->plugin_tree_view),
        x, y,
        &path, &column, NULL, NULL))
    {
      g_message ("no path at position %d %d", x, y);
      // if we can't find path at pos, we surely don't
      // want to pop up the menu
      return;
    }

  gtk_tree_selection_unselect_all(selection);
  gtk_tree_selection_select_path(selection, path);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    GTK_TREE_MODEL (self->plugin_tree_model),
    &iter, path);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (self->plugin_tree_model),
    &iter,
    PL_COLUMN_DESCR,
    &value);
  gtk_tree_path_free(path);

  PluginDescriptor * descr =
    g_value_get_pointer (&value);

  show_plugin_context_menu (self, descr, x, y);
}

static void
show_collection_context_menu (
  PluginBrowserWidget * self,
  PluginCollection *    collection,
  double                x,
  double                y)
{
  GMenu * menu = g_menu_new ();
  GMenuItem * menuitem;

  if (collection)
    {
      self->current_collections[0] = collection;
      self->num_current_collections = 1;

      menuitem =
        z_gtk_create_menu_item (
          _("Rename"), "edit-rename",
          "app.plugin-collection-rename");
      g_menu_append_item (menu, menuitem);

      menuitem =
        z_gtk_create_menu_item (
          _("Delete"), "edit-delete",
          "app.plugin-collection-remove");
      g_menu_append_item (menu, menuitem);
    }
  else
    {
      self->num_current_collections = 0;

      menuitem =
        z_gtk_create_menu_item (
          _("Add"), "add",
          "app.plugin-collection-add");
      g_menu_append_item (menu, menuitem);
    }

  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x, y, menu);
}

static void
on_collection_right_click (
  GtkGestureClick * gesture,
  gint                   n_press,
  gdouble                x_dbl,
  gdouble                y_dbl,
  PluginBrowserWidget *  self)
{
  if (n_press != 1)
    return;

  GtkTreePath *path;
  GtkTreeViewColumn *column;

  int x, y;
  gtk_tree_view_convert_widget_to_bin_window_coords (
    GTK_TREE_VIEW (self->collection_tree_view),
    (int) x_dbl, (int) y_dbl, &x, &y);

  GtkTreeSelection * selection =
    gtk_tree_view_get_selection (
      (self->collection_tree_view));
  bool have_selection = true;
  if (!gtk_tree_view_get_path_at_pos (
        GTK_TREE_VIEW (self->collection_tree_view),
        x, y,
        &path, &column, NULL, NULL))
    {
      /* no collection selected */
      have_selection = false;
    }

  PluginCollection * collection = NULL;

  gtk_tree_selection_unselect_all (selection);
  if (have_selection)
    {
      gtk_tree_selection_select_path (
        selection, path);
      GtkTreeIter iter;
      gtk_tree_model_get_iter (
        GTK_TREE_MODEL (
          self->collection_tree_model),
        &iter, path);
      int collection_idx = 0;
      gtk_tree_model_get (
        GTK_TREE_MODEL (
          self->collection_tree_model),
        &iter, 1, &collection_idx, -1);
      gtk_tree_path_free (path);

      collection =
        PLUGIN_MANAGER->collections->collections[
          collection_idx];
    }

  show_collection_context_menu (
    self, collection, x, y);
}

static void
show_category_context_menu (
  PluginBrowserWidget * self,
  double                x,
  double                y)
{
  GMenu * menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem =
    z_gtk_create_menu_item (
      _("Reset"), NULL,
      "app.plugin-browser-reset::category");
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x, y, menu);
}

static void
on_category_right_click (
  GtkGestureClick * gesture,
  gint                   n_press,
  gdouble                x_dbl,
  gdouble                y_dbl,
  PluginBrowserWidget *  self)
{
  if (n_press != 1)
    return;

  GtkTreePath *path;
  GtkTreeViewColumn *column;

  int x, y;
  gtk_tree_view_convert_widget_to_bin_window_coords (
    GTK_TREE_VIEW (self->category_tree_view),
    (int) x_dbl, (int) y_dbl, &x, &y);

  if (!gtk_tree_view_get_path_at_pos (
        GTK_TREE_VIEW (self->category_tree_view),
        x, y,
        &path, &column, NULL, NULL))
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
  GMenu * menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem =
    z_gtk_create_menu_item (
      _("Reset"), NULL,
      "app.plugin-browser-reset::author");
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x, y, menu);
}

static void
on_author_right_click (
  GtkGestureClick * gesture,
  gint                   n_press,
  gdouble                x_dbl,
  gdouble                y_dbl,
  PluginBrowserWidget *  self)
{
  if (n_press != 1)
    return;

  GtkTreePath *path;
  GtkTreeViewColumn *column;

  int x, y;
  gtk_tree_view_convert_widget_to_bin_window_coords (
    GTK_TREE_VIEW (self->author_tree_view),
    (int) x_dbl, (int) y_dbl, &x, &y);

  if (!gtk_tree_view_get_path_at_pos (
        GTK_TREE_VIEW (self->author_tree_view),
        x, y,
        &path, &column, NULL, NULL))
    {
      g_message ("no path at position %d %d", x, y);
      return;
    }

  show_author_context_menu (self, x, y);
}

/**
 * Updates the label below the list of plugins with
 * the plugin info.
 */
static int
update_plugin_info_label (
  PluginBrowserWidget * self,
  gpointer user_data)
{
  char * label = (char *) user_data;

  gtk_label_set_text (self->plugin_info, label);

  return G_SOURCE_REMOVE;
}

static void
cat_selected_foreach (
  GtkTreeModel *model,
  GtkTreePath *path,
  GtkTreeIter *iter,
  PluginBrowserWidget * self)
{
  char * str;
  gtk_tree_model_get (
    model, iter, 0, &str, -1);

  self->selected_categories[
    self->num_selected_categories++] =
      plugin_descriptor_string_to_category (
        str);

  g_free (str);
}

static void
author_selected_foreach (
  GtkTreeModel *model,
  GtkTreePath *path,
  GtkTreeIter *iter,
  PluginBrowserWidget * self)
{
  char * str;
  gtk_tree_model_get (
    model, iter, 0, &str, -1);

  self->selected_authors[
    self->num_selected_authors++] =
      symap_map (self->symap, str);

  g_free (str);
}

static void
protocol_selected_foreach (
  GtkTreeModel *model,
  GtkTreePath *path,
  GtkTreeIter *iter,
  PluginBrowserWidget * self)
{
  PluginProtocol protocol;
  gtk_tree_model_get (
    model, iter, 2, &protocol, -1);

  self->selected_protocols[
    self->num_selected_protocols++] =
      protocol;
}

static void
on_selection_changed (
  GtkTreeSelection * ts,
  PluginBrowserWidget * self)
{
  GtkTreeView * tv =
    gtk_tree_selection_get_tree_view (ts);
  GtkTreeModel * model =
    gtk_tree_view_get_model (tv);
  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (
      ts, NULL);

  if (model == self->category_tree_model)
    {
      self->num_selected_categories = 0;

      gtk_tree_selection_selected_foreach (
        ts,
        (GtkTreeSelectionForeachFunc)
          cat_selected_foreach,
        self);

      gtk_tree_model_filter_refilter (
        self->plugin_tree_model);
    }
  else if (model == self->author_tree_model)
    {
      self->num_selected_authors = 0;

      gtk_tree_selection_selected_foreach (
        ts,
        (GtkTreeSelectionForeachFunc)
          author_selected_foreach,
        self);

      gtk_tree_model_filter_refilter (
        self->plugin_tree_model);
    }
  else if (model ==
             GTK_TREE_MODEL (
               self->protocol_tree_model))
    {
      self->num_selected_protocols = 0;

      gtk_tree_selection_selected_foreach (
        ts,
        (GtkTreeSelectionForeachFunc)
          protocol_selected_foreach,
        self);

      gtk_tree_model_filter_refilter (
        self->plugin_tree_model);
    }
  else if (model ==
             GTK_TREE_MODEL (
               self->plugin_tree_model))
    {
      if (selected_rows)
        {
          GtkTreePath * tp =
            (GtkTreePath *)
            g_list_first (selected_rows)->data;
          GtkTreeIter iter;
          gtk_tree_model_get_iter (
            model, &iter, tp);
          PluginDescriptor * descr;
          gtk_tree_model_get (
            model, &iter, PL_COLUMN_DESCR, &descr, -1);
          char * label = g_strdup_printf (
            "%s\n%s, %s%s\nAudio: %d, %d\nMidi: %d, "
            "%d\nControls: %d, %d\nCV: %d, %d",
            descr->author,
            descr->category_str,
            plugin_protocol_to_str (
              descr->protocol),
            descr->arch == ARCH_32 ? " (32-bit)" : "",
            descr->num_audio_ins,
            descr->num_audio_outs,
            descr->num_midi_ins,
            descr->num_midi_outs,
            descr->num_ctrl_ins,
            descr->num_ctrl_outs,
            descr->num_cv_ins,
            descr->num_cv_outs);
          update_plugin_info_label (
            self, label);
        }
    }
  else if (model ==
             GTK_TREE_MODEL (
               self->collection_tree_model))
    {
      if (selected_rows)
        {
          GtkTreeIter iter;
          gtk_tree_selection_get_selected (
            ts, NULL, &iter);
          int collection_idx = 0;
          gtk_tree_model_get (
            GTK_TREE_MODEL (
              self->collection_tree_model),
            &iter, 1, &collection_idx, -1);

          self->selected_collection =
            PLUGIN_MANAGER->collections->
              collections[collection_idx];
        }
      else
        {
          self->selected_collection = NULL;
        }
      g_message (
        "selected collection: %s",
        self->selected_collection ?
          self->selected_collection->name :
          "none");

      gtk_tree_model_filter_refilter (
        self->plugin_tree_model);
    }

  g_list_free_full (
    selected_rows,
    (GDestroyNotify) gtk_tree_path_free);

  save_tree_view_selections (self);
  update_stack_switcher_emblems (self);
}

static GdkContentProvider *
on_dnd_drag_prepare (
  GtkDragSource * source,
  double          x,
  double          y,
  gpointer        user_data)
{
  GtkTreeView * tv = GTK_TREE_VIEW (user_data);
  PluginDescriptor * descr =
    z_gtk_get_single_selection_pointer (
      tv, PL_COLUMN_DESCR);
  char descr_str[600];
  sprintf (
    descr_str, PLUGIN_DESCRIPTOR_DND_PREFIX "%p",
    descr);

  GdkContentProvider * content_providers[] = {
    gdk_content_provider_new_typed (
      G_TYPE_STRING, descr_str),
  };

  return
    gdk_content_provider_new_union (
      content_providers,
      G_N_ELEMENTS (content_providers));
}

static GtkTreeModel *
create_model_for_favorites ()
{
  /* list name, list index */
  GtkListStore * list_store =
    gtk_list_store_new (
      2, G_TYPE_STRING, G_TYPE_INT);

  PluginCollections * collections =
    PLUGIN_MANAGER->collections;
  g_return_val_if_fail (collections, NULL);

  for (int i = 0;
       i < collections->num_collections; i++)
    {
      PluginCollection * collection =
        collections->collections[i];

      /* add row to model */
      GtkTreeIter iter;
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter,
        0, collection->name,
        1, i,
        -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_categories ()
{
  /* plugin category */
  GtkListStore * list_store =
    gtk_list_store_new (
      1, G_TYPE_STRING);

  GtkTreeIter iter;

  for (int i = 0;
       i < PLUGIN_MANAGER->num_plugin_categories;
       i++)
    {
      const gchar * name =
        PLUGIN_MANAGER->plugin_categories[i];

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, name, -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_authors ()
{
  /* plugin author */
  GtkListStore * list_store =
    gtk_list_store_new (
      1, G_TYPE_STRING);

  GtkTreeIter iter;

  for (int i = 0;
       i < PLUGIN_MANAGER->num_plugin_authors;
       i++)
    {
      const gchar * name =
        PLUGIN_MANAGER->plugin_authors[i];

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, name, -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_protocols ()
{
  /* protocol icon, string, enum */
  GtkListStore * list_store =
    gtk_list_store_new (
      3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

  GtkTreeIter iter;

  for (PluginProtocol prot = PROT_LV2;
       prot <= PROT_SF2; prot++)
    {
      const char * name =
        plugin_protocol_to_str (prot);

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
        list_store, &iter,
        0, icon,
        1, name,
        2, prot,
        -1);
    }

  GtkTreeModel * model =
    gtk_tree_model_sort_new_with_model (
      GTK_TREE_MODEL (list_store));

  return model;
}

static GtkTreeModel *
create_model_for_plugins (
  PluginBrowserWidget * self)
{
  GtkListStore *list_store;
  /*GtkTreePath *path;*/
  GtkTreeIter iter;

  /* plugin name, index */
  list_store =
    gtk_list_store_new (
      PL_NUM_COLUMNS, G_TYPE_STRING,
      G_TYPE_STRING, G_TYPE_POINTER);

  for (size_t i = 0;
       i < PLUGIN_MANAGER->plugin_descriptors->len;
       i++)
    {
      PluginDescriptor * descr =
        g_ptr_array_index (
          PLUGIN_MANAGER->plugin_descriptors, i);
      const char * icon_name = NULL;
      if (plugin_descriptor_is_instrument (descr))
        {
          icon_name = "instrument";
        }
      else if (plugin_descriptor_is_modulator (
                 descr))
        {
          icon_name = "modulator";
        }
      else if (plugin_descriptor_is_midi_modifier (
                 descr))
        {
          icon_name = "signal-midi";
        }
      else if (plugin_descriptor_is_effect (
                 descr))
        {
          icon_name = "bars";
        }
      else
        {
          icon_name = "plug";
        }

      /*else if (!strcmp (descr->category, "Distortion"))*/
        /*icon_name = "distortionfx";*/

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter,
        PL_COLUMN_ICON, icon_name,
        PL_COLUMN_NAME, descr->name,
        PL_COLUMN_DESCR, descr,
        -1);
    }

  GtkTreeModel * model =
    gtk_tree_model_filter_new (
      GTK_TREE_MODEL (list_store),
      NULL);
  gtk_tree_model_filter_set_visible_func (
    GTK_TREE_MODEL_FILTER (model),
    (GtkTreeModelFilterVisibleFunc) visible_func,
    self, NULL);

  return model;
}

static bool
refilter_source (
  PluginBrowserWidget * self)
{
  gtk_tree_model_filter_refilter (
    self->plugin_tree_model);

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

/**
 * Sets up the given treeview.
 */
static void
tree_view_setup (
  PluginBrowserWidget * self,
  GtkTreeView *         tree_view,
  GtkTreeModel *        model,
  bool                  allow_multi,
  bool                  dnd)
{
  gtk_tree_view_set_model (tree_view, model);

  z_gtk_tree_view_remove_all_columns (tree_view);

  /* init tree view */
  GtkCellRenderer * renderer;
  GtkTreeViewColumn * column;
  if (model ==
        GTK_TREE_MODEL (self->plugin_tree_model))
    {
      /* column for icon */
      renderer =
        gtk_cell_renderer_pixbuf_new ();
      column =
        gtk_tree_view_column_new_with_attributes (
          "icon", renderer,
          "icon-name", PL_COLUMN_ICON,
          NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view),
        column);

      /* column for name */
      renderer =
        gtk_cell_renderer_text_new ();
      column =
        gtk_tree_view_column_new_with_attributes (
          "name", renderer,
          "text", PL_COLUMN_NAME,
          NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view),
        column);

      /* set search column */
      gtk_tree_view_set_search_column (
        GTK_TREE_VIEW (tree_view),
        PL_COLUMN_NAME);

      /* set search func */
      gtk_tree_view_set_search_equal_func (
        GTK_TREE_VIEW (tree_view),
        (GtkTreeViewSearchEqualFunc)
          plugin_search_equal_func,
        self, NULL);
    }
  else if (model ==
             GTK_TREE_MODEL (
               self->protocol_tree_model))
    {
      /* column for icon */
      renderer =
        gtk_cell_renderer_pixbuf_new ();
      column =
        gtk_tree_view_column_new_with_attributes (
          "icon", renderer,
          "icon-name", 0,
          NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view), column);

      /* column for name */
      renderer =
        gtk_cell_renderer_text_new ();
      column =
        gtk_tree_view_column_new_with_attributes (
          "name", renderer,
          "text", 1,
          NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view),
        column);

      /* set search column */
      gtk_tree_view_set_search_column (
        GTK_TREE_VIEW (tree_view), 1);

      gtk_tree_sortable_set_sort_column_id (
        GTK_TREE_SORTABLE (model), 1,
        GTK_SORT_ASCENDING);
    }
  else
    {
      /* column for name */
      renderer =
        gtk_cell_renderer_text_new ();
      column =
        gtk_tree_view_column_new_with_attributes (
          "name", renderer,
          "text", 0,
          NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view),
        column);
    }

  /* hide headers and allow multi-selection */
  gtk_tree_view_set_headers_visible (
    GTK_TREE_VIEW (tree_view), false);

  if (allow_multi)
    {
      gtk_tree_selection_set_mode (
        gtk_tree_view_get_selection (
          GTK_TREE_VIEW (tree_view)),
        GTK_SELECTION_MULTIPLE);
    }

  if (dnd)
    {
      GtkDragSource * drag_source =
        gtk_drag_source_new ();
      g_signal_connect (
        drag_source, "prepare",
        G_CALLBACK (on_dnd_drag_prepare),
        tree_view);
      gtk_widget_add_controller (
        GTK_WIDGET (tree_view),
        GTK_EVENT_CONTROLLER (drag_source));
    }

  GtkTreeSelection * sel =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (tree_view));
  g_signal_connect (
    G_OBJECT (sel), "changed",
    G_CALLBACK (on_selection_changed), self);
}

void
plugin_browser_widget_refresh_collections (
  PluginBrowserWidget * self)
{
  self->collection_tree_model =
    create_model_for_favorites ();
  tree_view_setup (
    self, self->collection_tree_view,
    self->collection_tree_model,
    F_NO_MULTI_SELECT, F_NO_DND);
}

static void
on_visible_child_changed (
  GtkStack * stack,
  GParamSpec * pspec,
  PluginBrowserWidget * self)
{
  GtkWidget * child =
    gtk_stack_get_visible_child (stack);

  if (child == GTK_WIDGET (self->collection_scroll))
    {
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER,
        "plugin-browser-tab",
        PLUGIN_BROWSER_TAB_COLLECTION);
    }
  else if (child ==
             GTK_WIDGET (self->author_scroll))
    {
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER,
        "plugin-browser-tab",
        PLUGIN_BROWSER_TAB_AUTHOR);
    }
  else if (child ==
           GTK_WIDGET (self->category_scroll))
    {
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER,
        "plugin-browser-tab",
        PLUGIN_BROWSER_TAB_CATEGORY);
    }
  else if (child ==
           GTK_WIDGET (self->protocol_scroll))
    {
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER,
        "plugin-browser-tab",
        PLUGIN_BROWSER_TAB_PROTOCOL);
    }
}

static void
toggles_changed (
  GtkToggleButton * btn,
  PluginBrowserWidget * self)
{
  if (gtk_toggle_button_get_active (btn))
    {
      /* block signals, unset all, unblock */
      g_signal_handlers_block_by_func (
        self->toggle_instruments,
        toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_effects,
        toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_modulators,
        toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_midi_modifiers,
        toggles_changed, self);

      if (btn == self->toggle_instruments)
        {
          S_SET_ENUM (
            S_UI_PLUGIN_BROWSER,
            "plugin-browser-filter",
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
            S_UI_PLUGIN_BROWSER,
            "plugin-browser-filter",
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
            S_UI_PLUGIN_BROWSER,
            "plugin-browser-filter",
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
            S_UI_PLUGIN_BROWSER,
            "plugin-browser-filter",
            PLUGIN_BROWSER_FILTER_MIDI_EFFECT);
          gtk_toggle_button_set_active (
            self->toggle_instruments, 0);
          gtk_toggle_button_set_active (
            self->toggle_effects, 0);
          gtk_toggle_button_set_active (
            self->toggle_modulators, 0);
        }

      g_signal_handlers_unblock_by_func (
        self->toggle_instruments,
        toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_effects,
        toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_modulators,
        toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_midi_modifiers,
        toggles_changed, self);
    }
  else
    {
      S_SET_ENUM (
        S_UI_PLUGIN_BROWSER,
        "plugin-browser-filter",
        PLUGIN_BROWSER_FILTER_NONE);
    }
  gtk_tree_model_filter_refilter (
    self->plugin_tree_model);
}

static int
on_map_event (
  GtkStack * stack,
  GdkEvent * event,
  PluginBrowserWidget * self)
{
  /*g_message ("PLUGIN MAP EVENT");*/
  if (gtk_widget_get_mapped (
        GTK_WIDGET (self)))
    {
      self->start_saving_pos = 1;
      self->first_time_position_set_time =
        g_get_monotonic_time ();

      /* set divider position */
      int divider_pos =
        g_settings_get_int (
          S_UI,
          "browser-divider-position");
      gtk_paned_set_position (
        self->paned, divider_pos);
      self->first_time_position_set = 1;
    }

  return false;
}

static void
on_position_change (
  GtkStack * stack,
  GParamSpec * pspec,
  PluginBrowserWidget * self)
{
  int divider_pos;
  if (!self->start_saving_pos)
    return;

  gint64 curr_time = g_get_monotonic_time ();

  if (self->first_time_position_set ||
      curr_time -
        self->first_time_position_set_time < 400000)
    {
      /* get divider position */
      divider_pos =
        g_settings_get_int (
          S_UI,
          "browser-divider-position");
      gtk_paned_set_position (
        self->paned, divider_pos);

      self->first_time_position_set = 0;
      /*g_message ("***************************got plugin position %d",*/
                 /*divider_pos);*/
    }
  else
    {
      /* save the divide position */
      divider_pos =
        gtk_paned_get_position (self->paned);
      g_settings_set_int (
        S_UI,
        "browser-divider-position",
        divider_pos);
      /*g_message ("***************************set plugin position to %d",*/
                 /*divider_pos);*/
    }
}

static bool
on_key_release (
  GtkWidget *           widget,
  GdkEvent *            event,
  PluginBrowserWidget * self)
{
  g_free_and_null (self->current_search);

  gtk_tree_model_filter_refilter (
    self->plugin_tree_model);

  g_message ("key release");

  return false;
}

static void
finalize (
  PluginBrowserWidget * self)
{
  symap_free (self->symap);

  G_OBJECT_CLASS (
    plugin_browser_widget_parent_class)->
      finalize (G_OBJECT (self));
}

PluginBrowserWidget *
plugin_browser_widget_new ()
{
  PluginBrowserWidget * self =
    g_object_new (
      PLUGIN_BROWSER_WIDGET_TYPE, NULL);

  self->symap = symap_new ();

  gtk_label_set_xalign (self->plugin_info, 0);

  /* setup collections */
  plugin_browser_widget_refresh_collections (self);

  /* connect right click handler */
  GtkGestureClick * mp =
    GTK_GESTURE_CLICK (
      gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_collection_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->collection_tree_view),
    GTK_EVENT_CONTROLLER (mp));

  /* setup protocols */
  self->protocol_tree_model =
   GTK_TREE_MODEL_SORT (
     create_model_for_protocols ());
  tree_view_setup (
    self, self->protocol_tree_view,
    GTK_TREE_MODEL (self->protocol_tree_model),
    F_MULTI_SELECT, F_NO_DND);

  /* setup authors */
  self->author_tree_model =
    create_model_for_authors ();
  tree_view_setup (
    self, self->author_tree_view,
    self->author_tree_model,
    F_MULTI_SELECT, F_NO_DND);

  /* connect right click handler */
  mp =
    GTK_GESTURE_CLICK (
      gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_author_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->author_tree_view),
    GTK_EVENT_CONTROLLER (mp));

  /* setup categories */
  self->category_tree_model =
    create_model_for_categories ();
  tree_view_setup (
    self, self->category_tree_view,
    self->category_tree_model,
    F_MULTI_SELECT, F_NO_DND);

  /* connect right click handler */
  mp =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_category_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->category_tree_view),
    GTK_EVENT_CONTROLLER (mp));

  /* populate plugins */
  self->plugin_tree_model =
    GTK_TREE_MODEL_FILTER (
      create_model_for_plugins (self));
  tree_view_setup (
    self, self->plugin_tree_view,
    GTK_TREE_MODEL (
      self->plugin_tree_model),
    F_NO_MULTI_SELECT, F_DND);
  g_signal_connect (
    G_OBJECT (self->plugin_tree_view),
    "row-activated",
    G_CALLBACK (on_row_activated),
    self->plugin_tree_model);

  /* connect right click handler */
  mp =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_plugin_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->plugin_tree_view),
    GTK_EVENT_CONTROLLER (mp));

  /* set the selected values */
  PluginBrowserTab tab =
    S_GET_ENUM (
      S_UI_PLUGIN_BROWSER, "plugin-browser-tab");
  PluginBrowserFilter filter =
    S_GET_ENUM (
      S_UI_PLUGIN_BROWSER, "plugin-browser-filter");
  restore_tree_view_selections (self);

  switch (tab)
    {
    case PLUGIN_BROWSER_TAB_COLLECTION:
      gtk_stack_set_visible_child (
        self->stack,
        GTK_WIDGET (self->collection_scroll));
      break;
    case PLUGIN_BROWSER_TAB_AUTHOR:
      gtk_stack_set_visible_child (
        self->stack,
        GTK_WIDGET (self->author_scroll));
      break;
    case PLUGIN_BROWSER_TAB_CATEGORY:
      gtk_stack_set_visible_child (
        self->stack,
        GTK_WIDGET (self->category_scroll));
      break;
    case PLUGIN_BROWSER_TAB_PROTOCOL:
      gtk_stack_set_visible_child (
        self->stack,
        GTK_WIDGET (self->protocol_scroll));
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
      gtk_toggle_button_set_active (
        self->toggle_effects, 1);
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
    g_settings_get_int (
      S_UI,
      "browser-divider-position");
  gtk_paned_set_position (self->paned, divider_pos);
  self->first_time_position_set = 1;
  g_message (
    "setting plugin browser divider pos to %d",
    divider_pos);

  /* notify when tab changes */
  g_signal_connect (
    G_OBJECT (self->stack), "notify::visible-child",
    G_CALLBACK (on_visible_child_changed), self);
  g_signal_connect (
    G_OBJECT (self), "notify::position",
    G_CALLBACK (on_position_change), self);
  g_signal_connect (
    G_OBJECT (self), "map-event",
    G_CALLBACK (on_map_event), self);
  g_signal_connect (
    G_OBJECT (self), "key-release-event",
    G_CALLBACK (on_key_release), self);

  return self;
}

static void
plugin_browser_widget_class_init (
  PluginBrowserWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "plugin_browser.ui");

#define BIND_CHILD(name) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    PluginBrowserWidget, \
    name)

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
  BIND_CHILD (plugin_tree_view);
  BIND_CHILD (browser_bot);
  BIND_CHILD (plugin_info);
  BIND_CHILD (stack_switcher_box);
  BIND_CHILD (stack);
  BIND_CHILD (plugin_toolbar);
  BIND_CHILD (toggle_instruments);
  BIND_CHILD (toggle_effects);
  BIND_CHILD (toggle_modulators);
  BIND_CHILD (toggle_midi_modifiers);

#undef BIND_CHILD

#define BIND_SIGNAL(sig) \
  gtk_widget_class_bind_template_callback ( \
    klass, sig)

  BIND_SIGNAL (toggles_changed);

#undef BIND_SIGNAL

  GObjectClass * goklass = G_OBJECT_CLASS (_klass);
  goklass->finalize =
    (GObjectFinalizeFunc) finalize;
}

static void
plugin_browser_widget_init (
  PluginBrowserWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->popover_menu =
    GTK_POPOVER_MENU (
      gtk_popover_menu_new_from_model (NULL));
  gtk_box_append (
    GTK_BOX (self),
    GTK_WIDGET (self->popover_menu));

  self->stack_switcher =
    GTK_STACK_SWITCHER (gtk_stack_switcher_new ());
  gtk_stack_switcher_set_stack (
    self->stack_switcher,
    self->stack);
  gtk_box_append (
    self->stack_switcher_box,
    GTK_WIDGET (self->stack_switcher));

  /* set stackswitcher icons */
#if 0
  GValue iconval1 = G_VALUE_INIT;
  GValue iconval2 = G_VALUE_INIT;
  GValue iconval3 = G_VALUE_INIT;
  GValue iconval4 = G_VALUE_INIT;
  g_value_init (&iconval1, G_TYPE_STRING);
  g_value_init (&iconval2, G_TYPE_STRING);
  g_value_init (&iconval3, G_TYPE_STRING);
  g_value_init (&iconval4, G_TYPE_STRING);
  g_value_set_string (
    &iconval1, COLLECTIONS_ICON);
  g_value_set_string(
    &iconval2, AUTHORS_ICON);
  g_value_set_string(
    &iconval3, CATEGORIES_ICON);
  g_value_set_string(
    &iconval4, PROTOCOLS_ICON);

  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->collection_scroll),
    "icon-name", &iconval1);
  g_value_set_string (
    &iconval1, _("Collection"));
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->collection_scroll),
    "title", &iconval1);
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->author_scroll),
    "icon-name", &iconval2);
  g_value_set_string (
    &iconval2, _("Author"));
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->author_scroll),
    "title", &iconval2);
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->category_scroll),
    "icon-name", &iconval3);
  g_value_set_string (
    &iconval3, _("Category"));
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->category_scroll),
    "title", &iconval3);
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->protocol_scroll),
    "icon-name", &iconval4);
  g_value_set_string (
    &iconval4, _("Protocol"));
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->protocol_scroll),
    "title", &iconval4);
  g_value_unset (&iconval1);
  g_value_unset (&iconval2);
  g_value_unset (&iconval3);
  g_value_unset (&iconval4);
#endif

#if 0
  GList *children, *iter;
  children =
    gtk_container_get_children (
      GTK_CONTAINER (self->stack_switcher));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      if (!GTK_IS_RADIO_BUTTON (iter->data))
        continue;

      GtkWidget * radio = GTK_WIDGET (iter->data);
      g_object_set (
        radio, "hexpand", true, NULL);
    }
  g_list_free (children);
#endif

  self->current_collections =
    calloc (40000, sizeof (PluginCollection *));
  self->current_descriptors =
    calloc (40000, sizeof (PluginDescriptor *));

  gtk_widget_add_css_class (
    GTK_WIDGET (self), "plugin-browser");
}
