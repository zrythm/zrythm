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

#include "gui/accel.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/visibility.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/strv_builder.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

int
z_gtk_widget_destroy_idle (
  GtkWidget * widget)
{
  gtk_widget_destroy (widget);

  return G_SOURCE_REMOVE;
}

int
z_gtk_get_primary_monitor_scale_factor (void)
{
  GdkDisplay * display;
  GdkMonitor * monitor;
  int scale_factor;

  if (ZRYTHM_TESTING || !ZRYTHM_HAVE_UI)
    {
      return 1;
    }

  display =
    gdk_display_get_default ();
  if (!display)
    {
      g_message ("no display");
      goto return_default_scale_factor;
    }
  monitor =
    gdk_display_get_primary_monitor (display);
  if (!monitor)
    {
      g_message ("no primary monitor");
      goto return_default_scale_factor;
    }
  scale_factor =
    gdk_monitor_get_scale_factor (monitor);
  if (scale_factor < 1)
    {
      g_message (
        "invalid scale factor: %d", scale_factor);
      goto return_default_scale_factor;
    }

  return scale_factor;

return_default_scale_factor:
  g_message (
    "failed to get refresh rate from device, "
    "returning default");
  return 1;
}

/**
 * Returns the refresh rate in Hz.
 */
int
z_gtk_get_primary_monitor_refresh_rate (void)
{
  GdkDisplay * display;
  GdkMonitor * monitor;
  int refresh_rate;

  if (ZRYTHM_TESTING || !ZRYTHM_HAVE_UI)
    {
      return 30;
    }

  display =
    gdk_display_get_default ();
  if (!display)
    {
      g_warning ("no display");
      goto return_default_refresh_rate;
    }
  monitor =
    gdk_display_get_primary_monitor (display);
  if (!monitor)
    {
      g_warning ("no primary monitor");
      goto return_default_refresh_rate;
    }
  refresh_rate =
    /* divide by 1000 because gdk returns the
     * value in milli-Hz */
    gdk_monitor_get_refresh_rate (monitor) / 1000;
  if (refresh_rate == 0)
    {
      g_warning (
        "invalid refresh rate: %d", refresh_rate);
      goto return_default_refresh_rate;
    }

  return refresh_rate;

return_default_refresh_rate:
  g_warning (
    "failed to get refresh rate from device, "
    "returning default");
  return 30;
}

bool
z_gtk_is_wayland (void)
{
  if (ZRYTHM_TESTING || !ZRYTHM_HAVE_UI)
    {
      return false;
    }

  GdkDisplay * display =
    gdk_display_get_default ();
  g_return_val_if_fail (display, false);

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (display))
    {
      /*g_debug ("wayland");*/
      return true;
    }
  else
    {
      /*g_debug ("not wayland");*/
    }
#endif

  return false;
}

/**
 * @note Bumps reference, must be decremented after
 * calling.
 */
void
z_gtk_container_remove_all_children (
  GtkContainer * container)
{
  GList *children, *iter;

  children =
    gtk_container_get_children (
      GTK_CONTAINER (container));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      /*g_object_ref (GTK_WIDGET (iter->data));*/
      gtk_container_remove (
        container,
        GTK_WIDGET (iter->data));
    }
  g_list_free (children);
}

/**
 * Returns the primary or secondary label of the
 * given GtkMessageDialog.
 *
 * @param 0 for primary, 1 for secondary.
 */
GtkLabel *
z_gtk_message_dialog_get_label (
  GtkMessageDialog * self,
  const int          secondary)
{
  GList *children, *iter;

  GtkWidget * box =
    gtk_message_dialog_get_message_area (self);

  children =
    gtk_container_get_children (
      GTK_CONTAINER (box));
  GtkLabel * label = NULL;
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      label =
        GTK_LABEL (iter->data);
      const char * name =
      gtk_widget_class_get_css_name (
        GTK_WIDGET_GET_CLASS (label));
      if (string_is_equal (name, "label") &&
          !secondary)
        {
          break;
        }
      else if (
        string_is_equal (name, "secondary_label") &&
        secondary)
        {
          break;
        }
    }
  g_list_free (children);

  return label;
}

void
z_gtk_overlay_add_if_not_exists (
  GtkOverlay * overlay,
  GtkWidget *  widget)
{
  GList *children, *iter;

  children =
    gtk_container_get_children (
      GTK_CONTAINER (overlay));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      if (iter->data == widget)
        {
          g_message ("exists");
          g_list_free (children);
          return;
        }
    }
  g_list_free (children);

  g_message ("not exists, adding");
  gtk_overlay_add_overlay (overlay, widget);
}

void
z_gtk_container_destroy_all_children (
  GtkContainer * container)
{
  GList *children, *iter;

  children =
    gtk_container_get_children (GTK_CONTAINER (container));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    gtk_widget_destroy (GTK_WIDGET (iter->data));
  g_list_free (children);
}

void
z_gtk_container_remove_children_of_type (
  GtkContainer * container,
  GType          type)
{
  GList *children, *iter;

  children =
    gtk_container_get_children (GTK_CONTAINER (container));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      GtkWidget * widget = iter->data;
      if (G_TYPE_CHECK_INSTANCE_TYPE (
            widget, type))
        {
          /*g_object_ref (widget);*/
          gtk_container_remove (
            container,
            widget);
        }
    }
  g_list_free (children);
}

void
z_gtk_tree_view_remove_all_columns (
  GtkTreeView * treeview)
{
  g_return_if_fail (
    treeview && GTK_IS_TREE_VIEW (treeview));

  GtkTreeViewColumn *column;
  GList * list, * iter;
  list =
    gtk_tree_view_get_columns (treeview);
  for (iter = list; iter != NULL;
       iter = g_list_next (iter))
    {
      column =
        GTK_TREE_VIEW_COLUMN (iter->data);
      gtk_tree_view_remove_column (
        treeview, column);
    }
  g_list_free (list);
}

/**
 * Configures a simple value-text combo box using
 * the given model.
 */
void
z_gtk_configure_simple_combo_box (
  GtkComboBox * cb,
  GtkTreeModel * model)
{
  enum
  {
    VALUE_COL,
    TEXT_COL,
    ID_COL,
  };

  GtkCellRenderer *renderer;
  gtk_combo_box_set_model (
    cb, model);
  gtk_combo_box_set_id_column (
    cb, ID_COL);
  gtk_cell_layout_clear (
    GTK_CELL_LAYOUT (cb));
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (
    GTK_CELL_LAYOUT (cb),
    renderer, TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (cb), renderer,
    "text", TEXT_COL,
    NULL);
}

/**
 * Sets the icon name and optionally text.
 */
void
z_gtk_button_set_icon_name_and_text (
  GtkButton *  btn,
  const char * name,
  const char * text,
  bool         icon_first,
  GtkOrientation orientation,
  int          spacing)
{
  GtkWidget * img =
    gtk_image_new_from_icon_name (
      name, GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_visible (img, 1);
  z_gtk_container_remove_all_children (
    GTK_CONTAINER (btn));
  GtkWidget * box =
    gtk_box_new (orientation, spacing);
  gtk_widget_set_visible (box, true);
  GtkWidget * label =
    gtk_label_new (text);
  gtk_widget_set_visible (label, true);
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      gtk_widget_set_hexpand (label, true);
    }
  else
    {
      gtk_widget_set_vexpand (label, true);
    }

  if (icon_first)
    {
      gtk_container_add (GTK_CONTAINER (box), img);
      gtk_container_add (
        GTK_CONTAINER (box), label);
    }
  else
    {
      gtk_container_add (
        GTK_CONTAINER (box), label);
      gtk_container_add (GTK_CONTAINER (box), img);
    }
  gtk_container_add (
    GTK_CONTAINER (btn), box);
}

/**
 * Sets the icon name and optionally text.
 */
void
z_gtk_button_set_icon_name (
  GtkButton *  btn,
  const char * name)
{
  GtkWidget * img =
    gtk_image_new_from_icon_name (
      name, GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_visible (img, 1);
  z_gtk_container_remove_all_children (
    GTK_CONTAINER (btn));
  gtk_container_add (GTK_CONTAINER (btn), img);
}

/**
 * Sets the given emblem to the button, or unsets
 * the emblem if \ref emblem_icon is NULL.
 */
void
z_gtk_button_set_emblem (
  GtkButton *  btn,
  const char * emblem_icon_name)
{
  GtkWidget * btn_child =
    gtk_bin_get_child (GTK_BIN (btn));
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
  else if (GTK_IS_IMAGE (btn_child))
    {
      prev_img = GTK_IMAGE (btn_child);
    }
  else if (GTK_IS_CONTAINER (btn_child))
    {
      prev_img =
        GTK_IMAGE (
          z_gtk_container_get_single_child (
            GTK_CONTAINER (btn_child)));
    }
  else
    {
      g_return_if_reached ();
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
  else
    {
      g_return_if_reached ();
    }

  GIcon * icon = g_themed_icon_new (icon_name);

  if (emblem_icon_name)
    {
      GIcon * dot_icon =
        g_themed_icon_new ("media-record");
      GEmblem * emblem =
        g_emblem_new (dot_icon);
      icon =
        g_emblemed_icon_new (icon, emblem);
    }

  /* set new icon */
  GtkWidget * img =
    gtk_image_new_from_gicon (
      icon, icon_size);
  gtk_widget_set_visible (img, true);
  gtk_button_set_image (btn, img);
}

/**
 * Creates a button with the given icon name.
 */
GtkButton *
z_gtk_button_new_with_icon (
  const char * name)
{
  GtkButton * btn = GTK_BUTTON (gtk_button_new ());
  z_gtk_button_set_icon_name (btn, name);
  gtk_widget_set_visible (GTK_WIDGET (btn), true);
  return btn;
}

/**
 * Creates a toggle button with the given icon name.
 */
GtkToggleButton *
z_gtk_toggle_button_new_with_icon (
  const char * name)
{
  GtkToggleButton * btn =
    GTK_TOGGLE_BUTTON (gtk_toggle_button_new ());
  z_gtk_button_set_icon_name (
    GTK_BUTTON (btn), name);
  gtk_widget_set_visible (GTK_WIDGET (btn), true);

  return btn;
}

/**
 * Creates a toggle button with the given icon name.
 */
GtkToggleButton *
z_gtk_toggle_button_new_with_icon_and_text (
  const char * name,
  const char * text,
  bool         icon_first,
  GtkOrientation orientation,
  int          spacing)
{
  GtkToggleButton * btn =
    GTK_TOGGLE_BUTTON (gtk_toggle_button_new ());
  z_gtk_button_set_icon_name_and_text (
    GTK_BUTTON (btn), name, text, icon_first,
    orientation, spacing);
  gtk_widget_set_visible (GTK_WIDGET (btn), true);

  return btn;
}

/**
 * Creates a button with the given icon name and
 * text.
 */
GtkButton *
z_gtk_button_new_with_icon_and_text (
  const char * name,
  const char * text,
  bool         icon_first,
  GtkOrientation orientation,
  int          spacing)
{
  GtkButton * btn =
    GTK_BUTTON (gtk_button_new ());
  z_gtk_button_set_icon_name_and_text (
    GTK_BUTTON (btn), name, text, icon_first,
    orientation, spacing);
  gtk_widget_set_visible (GTK_WIDGET (btn), true);

  return btn;
}

/**
 * Creates a button with the given resource name as icon.
 */
GtkButton *
z_gtk_button_new_with_resource (IconType  icon_type,
                                const char * name)
{
  GtkButton * btn = GTK_BUTTON (gtk_button_new ());
  resources_add_icon_to_button (btn,
                                icon_type,
                                name);
  gtk_widget_set_visible (GTK_WIDGET (btn),
                          1);
  return btn;
}

/**
 * Creates a toggle button with the given resource name as
 * icon.
 */
GtkToggleButton *
z_gtk_toggle_button_new_with_resource (
  IconType  icon_type,
  const char * name)
{
  GtkToggleButton * btn =
    GTK_TOGGLE_BUTTON (gtk_toggle_button_new ());
  resources_add_icon_to_button (GTK_BUTTON (btn),
                                icon_type,
                                name);
  gtk_widget_set_visible (GTK_WIDGET (btn),
                          1);
  return btn;
}

/**
 * Creates a menu item.
 */
GtkMenuItem *
z_gtk_create_menu_item_full (
  const gchar *   label_name,
  const gchar *   icon_name,
  IconType        resource_icon_type,
  const gchar *   resource,
  bool            is_toggle,
  const char *    action_name)
{
  GtkWidget *box =
    gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *icon = NULL;
  if (icon_name)
    {
      icon =
        gtk_image_new_from_icon_name (
          icon_name, GTK_ICON_SIZE_MENU);
    }
  else if (resource)
    {
      icon =
        resources_get_icon (
          resource_icon_type, resource);
    }
  GtkWidget * label =
    gtk_accel_label_new (label_name);
  GtkWidget * menu_item;
  if (is_toggle)
    menu_item = gtk_check_menu_item_new ();
  else
    menu_item = gtk_menu_item_new ();

  if (icon)
    gtk_container_add (GTK_CONTAINER (box), icon);

  gtk_label_set_use_underline (
    GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);

  if (action_name)
    {
      gtk_actionable_set_action_name (
        GTK_ACTIONABLE (menu_item),
        action_name);
      accel_set_accel_label_from_action (
        GTK_ACCEL_LABEL (label),
        action_name);
    }

  gtk_box_pack_end (
    GTK_BOX (box), label, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (menu_item), box);

  gtk_widget_show_all (menu_item);

  return GTK_MENU_ITEM (menu_item);
}

/**
 * Returns a pointer stored at the given selection.
 */
void *
z_gtk_get_single_selection_pointer (
  GtkTreeView * tv,
  int           column)
{
  GtkTreeSelection * ts =
    gtk_tree_view_get_selection (tv);
  GtkTreeModel * model =
    gtk_tree_view_get_model (tv);
  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (
      ts, NULL);
  GtkTreePath * tp =
    (GtkTreePath *)
    g_list_first (selected_rows)->data;
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    model, &iter, tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    model, &iter, column, &value);
  return g_value_get_pointer (&value);
}

/**
 * Returns the label from a given GtkMenuItem.
 *
 * The menu item must have a box with an optional
 * icon and a label inside.
 */
GtkLabel *
z_gtk_get_label_from_menu_item (
  GtkMenuItem * mi)
{
  GList *children, *iter;

  /* get box */
  GtkWidget * box = NULL;
  children =
    gtk_container_get_children (
      GTK_CONTAINER (mi));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      if (GTK_IS_BOX (iter->data))
        {
          box = iter->data;
          g_list_free (children);
          break;
        }
    }
  g_warn_if_fail (box);

  children =
    gtk_container_get_children (
      GTK_CONTAINER (box));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      if (GTK_IS_LABEL (iter->data))
        {
          GtkLabel * label = GTK_LABEL (iter->data);
          g_list_free (children);
          return label;
        }
    }

  g_warn_if_reached ();
  g_list_free (children);

  return NULL;
}

/**
 * Sets the tooltip and finds the accel keys and
 * appends them to the tooltip in small text.
 */
void
z_gtk_set_tooltip_for_actionable (
  GtkActionable * actionable,
  const char *    tooltip)
{
  const char * action_name =
    gtk_actionable_get_action_name (
      actionable);
  char detailed_action[600];
  strcpy (detailed_action, action_name);
  GVariant * target_value =
    gtk_actionable_get_action_target_value (
      actionable);
  if (target_value &&
      string_is_equal (
        g_variant_get_type_string (target_value),
        "s"))
    {
      sprintf (
        detailed_action, "%s::%s",
        action_name,
        g_variant_get_string (target_value, 0));
    }
  char * accel =
    accel_get_primary_accel_for_action (
      detailed_action);
  char * tt;
  if (accel)
    tt =
      g_strdup_printf (
        "%s <span size=\"x-small\" foreground=\"#F79616\">%s</span>",
        tooltip, accel);
  else
    tt = g_strdup (tooltip);
  gtk_widget_set_tooltip_markup (
    GTK_WIDGET (actionable),
    tt);
  g_free (accel);
  g_free (tt);
}

/**
 * Changes the size of the icon inside tool buttons.
 */
void
z_gtk_tool_button_set_icon_size (
  GtkToolButton * toolbutton,
  GtkIconSize     icon_size)
{
  GtkImage * img =
    GTK_IMAGE (
      z_gtk_container_get_single_child (
        GTK_CONTAINER (
          z_gtk_container_get_single_child (
            GTK_CONTAINER (
              z_gtk_container_get_single_child (
                GTK_CONTAINER (toolbutton)))))));
  GtkImageType type =
    gtk_image_get_storage_type (GTK_IMAGE (img));
  g_return_if_fail (type == GTK_IMAGE_ICON_NAME);
  const char * _icon_name;
  gtk_image_get_icon_name (
    GTK_IMAGE (img), &_icon_name, NULL);
  char * icon_name = g_strdup (_icon_name);
  gtk_image_set_from_icon_name (
    GTK_IMAGE (img), icon_name,
    GTK_ICON_SIZE_SMALL_TOOLBAR);
  g_free (icon_name);
}

/**
 * Sets the ellipsize mode of each text cell
 * renderer in the combo box.
 */
void
z_gtk_combo_box_set_ellipsize_mode (
  GtkComboBox * self,
  PangoEllipsizeMode ellipsize)
{
  GList *children, *iter;
  children =
    gtk_cell_layout_get_cells  (
      GTK_CELL_LAYOUT (self));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      if (!GTK_IS_CELL_RENDERER_TEXT (iter->data))
        continue;

      g_object_set (
        iter->data, "ellipsize", ellipsize,
        NULL);
    }
  g_list_free (children);
}

/**
 * Adds the given style class to the widget.
 */
void
z_gtk_widget_add_style_class (
  GtkWidget   *widget,
  const gchar *class_name)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (class_name != NULL);

  gtk_style_context_add_class (
    gtk_widget_get_style_context (widget),
    class_name);
}

/**
 * Removes the given style class from the widget.
 */
void
z_gtk_widget_remove_style_class (
  GtkWidget   *widget,
  const gchar *class_name)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (class_name != NULL);

  gtk_style_context_remove_class (
    gtk_widget_get_style_context (widget),
    class_name);
}

/**
 * Returns the nth child of a container.
 */
GtkWidget *
z_gtk_container_get_nth_child (
  GtkContainer * container,
  int            index)
{
  GList *children, *iter;
  children =
    gtk_container_get_children (container);
  int i = 0;
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      GtkWidget * widget =
        GTK_WIDGET (iter->data);
      if (i++ == index)
        {
          g_list_free (children);
          return widget;
        }
    }
  g_list_free (children);
  g_return_val_if_reached (NULL);
}

/**
 * Sets the margin on all 4 sides on the widget.
 */
void
z_gtk_widget_set_margin (
  GtkWidget * widget,
  int         margin)
{
  gtk_widget_set_margin_start (widget, margin);
  gtk_widget_set_margin_end (widget, margin);
  gtk_widget_set_margin_top (widget, margin);
  gtk_widget_set_margin_bottom (widget, margin);
}

GtkFlowBoxChild *
z_gtk_flow_box_get_selected_child (
  GtkFlowBox * self)
{
  GList * list =
    gtk_flow_box_get_selected_children (self);
  GtkFlowBoxChild * sel_child = NULL;
  for (GList * l = list; l != NULL; l = l->next)
    {
      sel_child = (GtkFlowBoxChild *) l->data;
      break;
    }
  g_list_free (list);

  return sel_child;
}

/**
 * Callback to use for simple directory links.
 */
bool
z_gtk_activate_dir_link_func (
  GtkLabel * label,
  char *     uri,
  void *     data)
{
  io_open_directory (uri);

  return TRUE;
}

GtkSourceLanguageManager *
z_gtk_source_language_manager_get (void)
{
  GtkSourceLanguageManager * manager =
    gtk_source_language_manager_get_default ();

  static bool already_set = false;

  if (already_set)
    {
      return manager;
    }

  /* get the default search paths */
  const char * const * before_paths =
    gtk_source_language_manager_get_search_path (
      manager);

  /* build the new paths */
  StrvBuilder * after_paths_builder =
    strv_builder_new ();
  StrvBuilder * after_paths_builder_tmp =
    strv_builder_new ();
  int i = 0;
  while (before_paths[i])
    {
      g_debug (
        "language specs dir %d: %s",
        i, before_paths[i]);
      strv_builder_add (
        after_paths_builder, before_paths[i]);
      strv_builder_add (
        after_paths_builder_tmp, before_paths[i]);
      i++;
    }

  /* add the new path if not already in the list */
  char * language_specs_dir =
    zrythm_get_dir (
      ZRYTHM_DIR_SYSTEM_SOURCEVIEW_LANGUAGE_SPECS_DIR);
  g_return_val_if_fail (language_specs_dir, NULL);
  char ** tmp_dirs =
    strv_builder_end (after_paths_builder_tmp);
  if (!g_strv_contains (
         (const char * const *) tmp_dirs,
         language_specs_dir))
    {
      strv_builder_add (
        after_paths_builder, language_specs_dir);
    }
  g_strfreev (tmp_dirs);

  char ** dirs =
    strv_builder_end (after_paths_builder);

  i = 0;
  while (dirs[i])
    {
      const char * dir = dirs[i];
      g_message ("%d: %s", i, dir);
      i++;
    }

  gtk_source_language_manager_set_search_path (
    manager, dirs);

  g_free (language_specs_dir);
  g_strfreev (dirs);

  already_set = true;

  return manager;
}

typedef struct DetachableNotebookData
{
  /** Parent window of original notebook. */
  GtkWindow *     parent_window;

  /** Original notebook. */
  GtkNotebook *   notebook;

  /** Windows for dropping detached tabs. */
  GPtrArray *     new_windows;

  /** New notebooks inside new windows. */
  GPtrArray *     new_notebooks;

  /** Hashtable of settings schema keys => widget
   * pointers. */
  GHashTable *    ht;
} DetachableNotebookData;

static void
on_new_notebook_page_removed (
  GtkNotebook *            notebook,
  GtkWidget *              child,
  guint                    page_num,
  GtkWindow *              new_window)
{
  /* destroy the sub window after the notebook is
   * empty */
  if (gtk_notebook_get_n_pages (notebook) == 0)
    {
      gtk_widget_destroy (GTK_WIDGET (new_window));
    }
}

static void
on_new_window_destroyed (
  GtkWidget *              widget,
  DetachableNotebookData * data)
{
  guint idx;
  bool found =
    g_ptr_array_find (
      data->new_windows, widget, &idx);
  g_return_if_fail (found);

  GtkNotebook * new_notebook =
    g_ptr_array_index (data->new_notebooks, idx);
  g_ptr_array_remove_index (data->new_windows, idx);
  g_ptr_array_remove_index (
    data->new_notebooks, idx);

  /* if the sub window gets destroyed, push pages
   * back to the main window */
  /* detach the notebook pages in reverse sequence
   * to avoid index errors */
  int n_pages =
    gtk_notebook_get_n_pages (new_notebook);
  for (int i = n_pages - 1; i >= 0; i--)
    {
      GtkWidget * page =
        gtk_notebook_get_nth_page (new_notebook, i);
      GtkWidget * tab_label =
        gtk_notebook_get_tab_label (
          new_notebook, page);
      g_object_ref (page);
      g_object_ref (tab_label);
      gtk_notebook_detach_tab (
        new_notebook, page);
      gtk_notebook_append_page (
        data->notebook, page, tab_label);
      g_object_unref (page);
      g_object_unref (tab_label);
      gtk_notebook_set_tab_detachable (
        data->notebook, page, true);
      gtk_notebook_set_tab_reorderable (
        data->notebook, page, true);
    }
}

typedef struct DetachableNotebookDeleteEventData
{
  DetachableNotebookData * data;
  GtkWidget *              page;
} DetachableNotebookDeleteEventData;

static bool
on_new_window_delete_event (
  GtkWidget *                         widget,
  GdkEvent *                          event,
  DetachableNotebookDeleteEventData * delete_data)
{
  GtkWidget * page = delete_data->page;

  char * val =
    g_hash_table_lookup (
      delete_data->data->ht, page);
  g_return_val_if_fail (val, false);
  char key_detached[600];
  sprintf (key_detached, "%s-detached", val);
  char key_size[600];
  sprintf (key_size, "%s-size", val);

  int w, h;
  gtk_window_get_size (
    GTK_WINDOW (widget), &w, &h);
  g_settings_set_boolean (
    S_UI_PANELS, key_detached, false);
  g_settings_set (
    S_UI_PANELS, key_size, "(ii)", w, h);
  g_debug ("saving %s size %d %d", val, w, w);

  return false;
}

static void
free_delete_data (
  DetachableNotebookDeleteEventData * data,
  GClosure *                          closure)
{
  free (data);
}

static GtkNotebook *
on_create_window (
  GtkNotebook *            notebook,
  GtkWidget *              page,
  int                      x,
  int                      y,
  DetachableNotebookData * data)
{
  GtkWindow * new_window =
    GTK_WINDOW (
      gtk_window_new (GTK_WINDOW_TOPLEVEL));
  GtkNotebook * new_notebook =
    GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_container_add (
    GTK_CONTAINER (new_window),
    GTK_WIDGET (new_notebook));
  /* very important for DND */
  gtk_notebook_set_group_name (
    new_notebook, "foldable-notebook-group");

  g_signal_connect (
    G_OBJECT (new_notebook), "page-removed",
    G_CALLBACK (on_new_notebook_page_removed),
    new_window);
  g_signal_connect (
    G_OBJECT (new_window), "destroy",
    G_CALLBACK (on_new_window_destroyed),
    data);

  DetachableNotebookDeleteEventData * delete_data =
    object_new (
      DetachableNotebookDeleteEventData);
  delete_data->data = data;
  delete_data->page = page;
  g_signal_connect_data (
    G_OBJECT (new_window), "delete-event",
    G_CALLBACK (on_new_window_delete_event),
    delete_data,
    (GClosureNotify) free_delete_data, 0);
  gtk_window_set_icon_name (
    GTK_WINDOW (new_window), "zrythm");
  gtk_window_set_transient_for (
    GTK_WINDOW (new_window),
    GTK_WINDOW (data->parent_window));
  gtk_window_set_destroy_with_parent (
    new_window, true);

  /* set application so that actions are connected
   * properly */
  gtk_window_set_application (
    GTK_WINDOW (new_window),
    GTK_APPLICATION (zrythm_app));

  gtk_widget_show_all (
    GTK_WIDGET (new_window));
  gtk_widget_set_visible (page, true);

  g_ptr_array_add (
    data->new_windows, new_window);
  g_ptr_array_add (
    data->new_notebooks, new_notebook);

  char * val =
    g_hash_table_lookup (data->ht, page);
  g_return_val_if_fail (val, NULL);
  char key_detached[600];
  sprintf (key_detached, "%s-detached", val);
  char key_size[600];
  sprintf (key_size, "%s-size", val);

  /* save/load settings */
  g_settings_set_boolean (
    S_UI_PANELS, key_detached, true);
  GVariant * size_val =
    g_settings_get_value (
      S_UI_PANELS, key_size);
  int width =
    (int)
    g_variant_get_int32 (
      g_variant_get_child_value (size_val, 0));
  int height =
    (int)
    g_variant_get_int32 (
      g_variant_get_child_value (size_val, 1));
  g_debug (
    "loading %s size %d %d", val, width, height);
  gtk_window_resize (
    GTK_WINDOW (new_window), width, height);
  g_variant_unref (size_val);

  return new_notebook;
}

static void
on_detachable_notebook_destroyed (
  GtkWidget * widget,
  DetachableNotebookData * data)
{
  g_ptr_array_free (data->new_windows, false);
  g_ptr_array_free (data->new_notebooks, false);
  g_hash_table_destroy (data->ht);
  free (data);
}

/**
 * Detach eligible pages programmatically into a
 * new window.
 *
 * Used during startup.
 *
 * @param data Data received from
 *   z_gtk_notebook_make_detachable.
 */
static void
detach_pages_programmatically (
  GtkNotebook *            old_notebook,
  DetachableNotebookData * data)
{
  int n_pages =
    gtk_notebook_get_n_pages (old_notebook);
  for (int i = n_pages - 1; i >= 0; i--)
    {
      GtkWidget * page =
        gtk_notebook_get_nth_page (
          old_notebook, i);
      GtkWidget * tab_label =
        gtk_notebook_get_tab_label (
          old_notebook, page);

      char * val =
        g_hash_table_lookup (data->ht, page);
      g_return_if_fail (val);
      char key_detached[600];
      sprintf (key_detached, "%s-detached", val);
      bool needs_detach =
        g_settings_get_boolean (
          S_UI_PANELS, key_detached);

      if (needs_detach)
        {
          g_object_ref (page);
          g_object_ref (tab_label);
          GtkNotebook * new_notebook =
            on_create_window (
              old_notebook, page, 12, 12, data);
          gtk_notebook_detach_tab (
            old_notebook, page);
          gtk_notebook_append_page (
            new_notebook, page, tab_label);
          gtk_notebook_set_tab_detachable (
            new_notebook, page, true);
          gtk_notebook_set_tab_reorderable (
            new_notebook, page, true);
          g_object_unref (page);
          g_object_unref (tab_label);
        }
    }
}

/**
 * Makes the given GtkNotebook detachable to
 * a new window.
 */
void
z_gtk_notebook_make_detachable (
  GtkNotebook * notebook,
  GtkWindow *   parent_window)
{
  DetachableNotebookData * data =
    object_new (DetachableNotebookData);
  data->notebook = notebook;
  data->new_windows = g_ptr_array_new ();
  data->new_notebooks = g_ptr_array_new ();
  data->parent_window = parent_window;

  /* prepare hashtable */
#define ADD_PAIR(key,w) \
  g_return_if_fail (key); \
  g_return_if_fail (w); \
  g_hash_table_insert ( \
    data->ht, w, g_strdup (key))

  data->ht =
    g_hash_table_new_full (
      NULL, NULL, NULL, g_free);
  ADD_PAIR ("track-visibility",
    MW_LEFT_DOCK_EDGE->visibility_box);
  ADD_PAIR (
    "track-inspector",
    MW_LEFT_DOCK_EDGE->track_inspector_scroll);
  ADD_PAIR (
    "plugin-inspector",
    MW_LEFT_DOCK_EDGE->plugin_inspector_scroll);
  ADD_PAIR (
    "plugin-browser",
    MW_RIGHT_DOCK_EDGE->plugin_browser_box);
  ADD_PAIR (
    "file-browser",
    MW_RIGHT_DOCK_EDGE->file_browser_box);
  ADD_PAIR (
    "monitor-section",
    MW_RIGHT_DOCK_EDGE->monitor_section_box);
  ADD_PAIR (
    "modulator-view",
    MW_BOT_DOCK_EDGE->modulator_view_box);
  ADD_PAIR (
    "mixer",
    MW_BOT_DOCK_EDGE->mixer_box);
  ADD_PAIR (
    "clip-editor",
    MW_BOT_DOCK_EDGE->clip_editor_box);
  ADD_PAIR (
    "chord-pad",
    MW_BOT_DOCK_EDGE->chord_pad_box);
  ADD_PAIR (
    "timeline",
    MW_MAIN_NOTEBOOK->
      timeline_plus_event_viewer_paned);
  ADD_PAIR (
    "cc-bindings",
    MW_MAIN_NOTEBOOK->cc_bindings_box);
  ADD_PAIR (
    "port-connections",
    MW_MAIN_NOTEBOOK->port_connections_box);
  ADD_PAIR (
    "scenes",
    MW_MAIN_NOTEBOOK->scenes_box);

#undef ADD_PAIR

  g_signal_connect (
    G_OBJECT (notebook), "create-window",
    G_CALLBACK (on_create_window), data);
  g_signal_connect_after (
    G_OBJECT (notebook), "destroy",
    G_CALLBACK (on_detachable_notebook_destroyed),
    data);

  detach_pages_programmatically (notebook, data);
}
