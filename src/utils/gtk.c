// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * ---
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "gui/accel.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/right_dock_edge.h"
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
#  include <gdk/wayland/gdkwayland.h>
#endif

#include <glib/gi18n.h>

#include <gsk/gskrenderer.h>

typedef enum
{
  Z_UTILS_GTK_ERROR_FAILED,
} ZUtilsGtkError;

#define Z_UTILS_GTK_ERROR z_utils_gtk_error_quark ()
GQuark
z_utils_gtk_error_quark (void);
G_DEFINE_QUARK (
  z - utils - gtk - error - quark,
  z_utils_gtk_error)

GdkMonitor *
z_gtk_get_primary_monitor (void)
{
  GdkDisplay * display;
  GdkMonitor * monitor;
  GListModel * monitor_list;
  display = gdk_display_get_default ();
  if (!display)
    {
      g_message ("no display");
      return NULL;
    }
  monitor_list = gdk_display_get_monitors (display);
  monitor = g_list_model_get_item (monitor_list, 0);
  if (!monitor)
    {
      g_message ("no primary monitor");
      return NULL;
    }

  return monitor;
}

int
z_gtk_get_primary_monitor_scale_factor (void)
{
  GdkMonitor * monitor;
  int          scale_factor;

  if (ZRYTHM_TESTING || !ZRYTHM_HAVE_UI)
    {
      return 1;
    }

  monitor = z_gtk_get_primary_monitor ();
  if (!monitor)
    {
      g_message ("no primary monitor");
      goto return_default_scale_factor;
    }
  scale_factor = gdk_monitor_get_scale_factor (monitor);
  if (scale_factor < 1)
    {
      g_message ("invalid scale factor: %d", scale_factor);
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
  GdkMonitor * monitor;
  int          refresh_rate;

  if (ZRYTHM_TESTING || !ZRYTHM_HAVE_UI)
    {
      return 30;
    }

  monitor = z_gtk_get_primary_monitor ();
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
      g_warning ("invalid refresh rate: %d", refresh_rate);
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

  GdkDisplay * display = gdk_display_get_default ();
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

void
z_gtk_widget_remove_all_children (GtkWidget * widget)
{
  if (GTK_IS_BUTTON (widget))
    {
      gtk_button_set_child (GTK_BUTTON (widget), NULL);
      return;
    }
  if (GTK_IS_MENU_BUTTON (widget))
    {
      gtk_menu_button_set_child (
        GTK_MENU_BUTTON (widget), NULL);
      return;
    }

  z_gtk_widget_remove_children_of_type (
    widget, GTK_TYPE_WIDGET);
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
  GtkWidget * box = gtk_message_dialog_get_message_area (self);

  GtkLabel * label = NULL;
  for (GtkWidget * child = gtk_widget_get_first_child (box);
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      label = GTK_LABEL (child);
      const char * name = gtk_widget_class_get_css_name (
        GTK_WIDGET_GET_CLASS (label));
      if (string_is_equal (name, "label") && !secondary)
        {
          break;
        }
      else if (
        string_is_equal (name, "secondary_label") && secondary)
        {
          break;
        }
    }

  return label;
}

void
z_gtk_overlay_add_if_not_exists (
  GtkOverlay * overlay,
  GtkWidget *  widget)
{
  for (GtkWidget * child =
         gtk_widget_get_first_child (GTK_WIDGET (overlay));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (child == widget)
        {
          g_message ("exists");
          return;
        }
    }

  g_message ("not exists, adding");
  gtk_overlay_add_overlay (overlay, widget);
}

void
z_gtk_widget_destroy_all_children (GtkWidget * widget)
{
  /* children are destroyed if there is no extra
   * reference when they are removed from their
   * parent */
  z_gtk_widget_remove_all_children (widget);
}

void
z_gtk_widget_remove_children_of_type (
  GtkWidget * widget,
  GType       type)
{
  for (GtkWidget * child = gtk_widget_get_last_child (widget);
       child != NULL;)
    {
      GtkWidget * next_child =
        gtk_widget_get_prev_sibling (child);
      if (G_TYPE_CHECK_INSTANCE_TYPE (child, type))
        {
          if (GTK_IS_BOX (widget))
            {
#if 0
              g_debug (
                "removing %s (%p) from box %s (%p)",
                gtk_widget_get_name (child), child,
                gtk_widget_get_name (widget), widget);
#endif
              gtk_box_remove (GTK_BOX (widget), child);
            }
          else
            {
#if 0
              g_debug (
                "unparenting %s (%p) from "
                "parent %s (%p)",
                gtk_widget_get_name (child), child,
                gtk_widget_get_name (widget), widget);
#endif
              gtk_widget_unparent (child);
            }
        }
      child = next_child;
    }
}

void
z_gtk_tree_view_remove_all_columns (GtkTreeView * treeview)
{
  g_return_if_fail (treeview && GTK_IS_TREE_VIEW (treeview));

  GtkTreeViewColumn * column;
  GList *             list, *iter;
  list = gtk_tree_view_get_columns (treeview);
  for (iter = list; iter != NULL; iter = g_list_next (iter))
    {
      column = GTK_TREE_VIEW_COLUMN (iter->data);
      gtk_tree_view_remove_column (treeview, column);
    }
  g_list_free (list);
}

void
z_gtk_column_view_remove_all_columnes (
  GtkColumnView * column_view)
{
  g_return_if_fail (
    column_view && GTK_IS_COLUMN_VIEW (column_view));

  GListModel * list =
    gtk_column_view_get_columns (column_view);
  gpointer ptr;
  while ((ptr = g_list_model_get_item (list, 0)))
    {
      GtkColumnViewColumn * col = GTK_COLUMN_VIEW_COLUMN (ptr);
      gtk_column_view_remove_column (column_view, col);
    }
}

GListStore *
z_gtk_column_view_get_list_store (GtkColumnView * column_view)
{
  GtkSelectionModel * sel_model =
    gtk_column_view_get_model (column_view);
  GListModel * model;
  if (GTK_IS_MULTI_SELECTION (sel_model))
    {
      model = gtk_multi_selection_get_model (
        GTK_MULTI_SELECTION (sel_model));
    }
  else
    {
      model = gtk_single_selection_get_model (
        GTK_SINGLE_SELECTION (sel_model));
    }
  if (GTK_IS_SORT_LIST_MODEL (model))
    {
      model = gtk_sort_list_model_get_model (
        GTK_SORT_LIST_MODEL (model));
    }
  return G_LIST_STORE (model);
}

/**
 * Removes all items and re-populates the list
 * store.
 */
void
z_gtk_list_store_splice (
  GListStore * store,
  GPtrArray *  ptr_array)
{
  size_t     num_objs;
  gpointer * objs = g_ptr_array_steal (ptr_array, &num_objs);
  g_list_store_splice (
    store, 0, g_list_model_get_n_items (G_LIST_MODEL (store)),
    objs, num_objs);
  g_free (objs);
}

/**
 * Configures a simple value-text combo box using
 * the given model.
 */
void
z_gtk_configure_simple_combo_box (
  GtkComboBox *  cb,
  GtkTreeModel * model)
{
  enum
  {
    VALUE_COL,
    TEXT_COL,
    ID_COL,
  };

  GtkCellRenderer * renderer;
  gtk_combo_box_set_model (cb, model);
  gtk_combo_box_set_id_column (cb, ID_COL);
  gtk_cell_layout_clear (GTK_CELL_LAYOUT (cb));
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (
    GTK_CELL_LAYOUT (cb), renderer, TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (cb), renderer, "text", TEXT_COL, NULL);
}

/**
 * Sets the icon name and optionally text.
 */
void
z_gtk_button_set_icon_name_and_text (
  GtkButton *    btn,
  const char *   name,
  const char *   text,
  bool           icon_first,
  GtkOrientation orientation,
  int            spacing)
{
  GtkWidget * img = gtk_image_new_from_icon_name (name);
  z_gtk_widget_remove_all_children (GTK_WIDGET (btn));
  GtkWidget * box = gtk_box_new (orientation, spacing);
  GtkWidget * label = gtk_label_new (text);
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
      gtk_box_append (GTK_BOX (box), img);
      gtk_box_append (GTK_BOX (box), label);
    }
  else
    {
      gtk_box_append (GTK_BOX (box), label);
      gtk_box_append (GTK_BOX (box), img);
    }
  gtk_button_set_child (GTK_BUTTON (btn), box);
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
  /* TODO */
#if 0
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
              gtk_widget_get_first_child (
                GTK_WIDGET (inner_child)));
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
          gtk_widget_get_first_child (
            GTK_WIDGET (btn_child)));
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
#endif
}

/**
 * Creates a toggle button with the given icon name.
 */
GtkToggleButton *
z_gtk_toggle_button_new_with_icon (const char * name)
{
  GtkToggleButton * btn =
    GTK_TOGGLE_BUTTON (gtk_toggle_button_new ());
  gtk_button_set_icon_name (GTK_BUTTON (btn), name);
  return btn;
}

/**
 * Creates a toggle button with the given icon name.
 */
GtkToggleButton *
z_gtk_toggle_button_new_with_icon_and_text (
  const char *   name,
  const char *   text,
  bool           icon_first,
  GtkOrientation orientation,
  int            spacing)
{
  GtkToggleButton * btn =
    GTK_TOGGLE_BUTTON (gtk_toggle_button_new ());
  z_gtk_button_set_icon_name_and_text (
    GTK_BUTTON (btn), name, text, icon_first, orientation,
    spacing);

  return btn;
}

/**
 * Creates a button with the given icon name and
 * text.
 */
GtkButton *
z_gtk_button_new_with_icon_and_text (
  const char *   name,
  const char *   text,
  bool           icon_first,
  GtkOrientation orientation,
  int            spacing)
{
  GtkButton * btn = GTK_BUTTON (gtk_button_new ());
  z_gtk_button_set_icon_name_and_text (
    GTK_BUTTON (btn), name, text, icon_first, orientation,
    spacing);
  gtk_widget_set_visible (GTK_WIDGET (btn), true);

  return btn;
}

/**
 * Creates a button with the given resource name as icon.
 */
GtkButton *
z_gtk_button_new_with_resource (
  IconType     icon_type,
  const char * name)
{
  GtkButton * btn = GTK_BUTTON (gtk_button_new ());
  resources_add_icon_to_button (btn, icon_type, name);
  gtk_widget_set_visible (GTK_WIDGET (btn), 1);
  return btn;
}

/**
 * Creates a toggle button with the given resource name as
 * icon.
 */
GtkToggleButton *
z_gtk_toggle_button_new_with_resource (
  IconType     icon_type,
  const char * name)
{
  GtkToggleButton * btn =
    GTK_TOGGLE_BUTTON (gtk_toggle_button_new ());
  resources_add_icon_to_button (
    GTK_BUTTON (btn), icon_type, name);
  gtk_widget_set_visible (GTK_WIDGET (btn), 1);
  return btn;
}

/**
 * Creates a menu item.
 */
GMenuItem *
z_gtk_create_menu_item_full (
  const gchar * label_name,
  const gchar * icon_name,
  const char *  detailed_action)
{
  g_return_val_if_fail (label_name, NULL);

  GMenuItem * menuitem =
    g_menu_item_new (label_name, detailed_action);

  /* note: setting an icon causes the accelerators
   * to not be right-aligned - maybe it's because
   * this is an icon name and not a GIcon. FIXME
   * pass a GIcon */
#if 0
  if (icon_name)
    {
      g_menu_item_set_attribute (
        menuitem, G_MENU_ATTRIBUTE_ICON,
        "s", icon_name, NULL);
    }
#endif

  return menuitem;
}

/**
 * Returns a pointer stored at the given selection.
 */
void *
z_gtk_get_single_selection_pointer (GtkTreeView * tv, int column)
{
  g_return_val_if_fail (GTK_IS_TREE_VIEW (tv), NULL);
  GtkTreeSelection * ts = gtk_tree_view_get_selection (tv);
  g_return_val_if_fail (GTK_IS_TREE_SELECTION (ts), NULL);
  GtkTreeModel * model = gtk_tree_view_get_model (tv);
  g_return_val_if_fail (GTK_IS_TREE_MODEL (model), NULL);
  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (ts, NULL);
  GtkTreePath * tp =
    (GtkTreePath *) g_list_first (selected_rows)->data;
  g_return_val_if_fail (tp, NULL);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (model, &iter, tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (model, &iter, column, &value);
  return g_value_get_pointer (&value);
}

#if 0
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
#endif

/**
 * Gets the tooltip for the given action on the
 * given widget.
 *
 * If the action is valid, an orange text showing
 * the accelerator will be added to the tooltip.
 *
 * @return A new string that must be free'd with
 * g_free().
 */
char *
z_gtk_get_tooltip_for_action (
  const char * detailed_action,
  const char * tooltip)
{
  char * tmp =
    accel_get_primary_accel_for_action (detailed_action);
  if (tmp)
    {
      char * accel = g_markup_escape_text (tmp, -1);
      g_free (tmp);
      char accel_color_hex[90];
      ui_gdk_rgba_to_hex (
        &UI_COLORS->bright_orange, accel_color_hex);
      char edited_tooltip[800];
      sprintf (
        edited_tooltip,
        "%s <span size=\"x-small\" "
        "foreground=\"%s\">%s</span>",
        tooltip, accel_color_hex, accel);
      g_free (accel);
      return g_strdup (edited_tooltip);
    }
  else
    {
      return g_strdup (tooltip);
    }
}

/**
 * Sets the tooltip for the given action on the
 * given widget.
 *
 * If the action is valid, an orange text showing
 * the accelerator will be added to the tooltip.
 */
void
z_gtk_widget_set_tooltip_for_action (
  GtkWidget *  widget,
  const char * detailed_action,
  const char * tooltip)
{
  char * edited_tooltip =
    z_gtk_get_tooltip_for_action (detailed_action, tooltip);
  gtk_widget_set_tooltip_markup (widget, edited_tooltip);
  g_free (edited_tooltip);
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
    gtk_actionable_get_action_name (actionable);
  char *     detailed_action = g_strdup (action_name);
  GVariant * target_value =
    gtk_actionable_get_action_target_value (actionable);
  if (target_value)
    {
      g_free (detailed_action);
      detailed_action = g_action_print_detailed_name (
        action_name, target_value);
    }
  z_gtk_widget_set_tooltip_for_action (
    GTK_WIDGET (actionable), detailed_action, tooltip);
  g_free (detailed_action);
}

#if 0
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
#endif

/**
 * Sets the ellipsize mode of each text cell
 * renderer in the combo box.
 */
void
z_gtk_combo_box_set_ellipsize_mode (
  GtkComboBox *      self,
  PangoEllipsizeMode ellipsize)
{
  GList *children, *iter;
  children =
    gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (self));
  for (iter = children; iter != NULL; iter = g_list_next (iter))
    {
      if (!GTK_IS_CELL_RENDERER_TEXT (iter->data))
        continue;

      g_object_set (iter->data, "ellipsize", ellipsize, NULL);
    }
  g_list_free (children);
}

/**
 * Removes the given style class from the widget.
 */
void
z_gtk_widget_remove_style_class (
  GtkWidget *   widget,
  const gchar * class_name)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (class_name != NULL);

  gtk_style_context_remove_class (
    gtk_widget_get_style_context (widget), class_name);
}

/**
 * Returns the nth child of a container.
 */
GtkWidget *
z_gtk_widget_get_nth_child (GtkWidget * widget, int index)
{
  int i = 0;
  for (GtkWidget * child = gtk_widget_get_first_child (widget);
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (i != index)
        {
          i++;
          continue;
        }
      return child;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Sets the margin on all 4 sides on the widget.
 */
void
z_gtk_widget_set_margin (GtkWidget * widget, int margin)
{
  gtk_widget_set_margin_start (widget, margin);
  gtk_widget_set_margin_end (widget, margin);
  gtk_widget_set_margin_top (widget, margin);
  gtk_widget_set_margin_bottom (widget, margin);
}

GtkFlowBoxChild *
z_gtk_flow_box_get_selected_child (GtkFlowBox * self)
{
  GList * list = gtk_flow_box_get_selected_children (self);
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
    gtk_source_language_manager_get_search_path (manager);

  /* build the new paths */
  StrvBuilder * after_paths_builder = strv_builder_new ();
  StrvBuilder * after_paths_builder_tmp = strv_builder_new ();
  int           i = 0;
  while (before_paths[i])
    {
      g_debug (
        "language specs dir %d: %s", i, before_paths[i]);
      strv_builder_add (after_paths_builder, before_paths[i]);
      strv_builder_add (
        after_paths_builder_tmp, before_paths[i]);
      i++;
    }

  /* add the new path if not already in the list */
  char * language_specs_dir = zrythm_get_dir (
    ZRYTHM_DIR_SYSTEM_SOURCEVIEW_LANGUAGE_SPECS_DIR);
  g_return_val_if_fail (language_specs_dir, NULL);
  char ** tmp_dirs =
    strv_builder_end (after_paths_builder_tmp);
  if (!g_strv_contains (
        (const char * const *) tmp_dirs, language_specs_dir))
    {
      strv_builder_add (
        after_paths_builder, language_specs_dir);
    }
  g_strfreev (tmp_dirs);
  g_free (language_specs_dir);

  /* add bundled dir for GNU/Linux packages */
  language_specs_dir = zrythm_get_dir (
    ZRYTHM_DIR_SYSTEM_BUNDLED_SOURCEVIEW_LANGUAGE_SPECS_DIR);
  strv_builder_add (after_paths_builder, language_specs_dir);
  g_free (language_specs_dir);

  char ** dirs = strv_builder_end (after_paths_builder);

  i = 0;
  while (dirs[i])
    {
      const char * dir = dirs[i];
      g_message ("%d: %s", i, dir);
      i++;
    }

  gtk_source_language_manager_set_search_path (
    manager, (const char * const *) dirs);

  g_strfreev (dirs);

  already_set = true;

#if 0
  /* print found language specs */
  const char * const * lang_ids =
    gtk_source_language_manager_get_language_ids (
      manager);
  const char * lang_id = NULL;
  i = 0;
  while ((lang_id = lang_ids[i++]) != NULL)
    {
      g_debug ("[%d] %s", i, lang_id);
    }
#endif

  return manager;
}

typedef struct DetachableNotebookData
{
  /** Parent window of original notebook. */
  GtkWindow * parent_window;

  /** Original notebook. */
  GtkNotebook * notebook;

  /** Windows for dropping detached tabs. */
  GPtrArray * new_windows;

  /** New notebooks inside new windows. */
  GPtrArray * new_notebooks;

  /** Hashtable of settings schema keys => widget
   * pointers. */
  GHashTable * ht;

  /** Window title. */
  const char * title;

  /** Window role. */
  const char * role;
} DetachableNotebookData;

static void
on_new_notebook_page_removed (
  GtkNotebook * notebook,
  GtkWidget *   child,
  guint         page_num,
  GtkWindow *   new_window)
{
  /* destroy the sub window after the notebook is
   * empty */
  if (gtk_notebook_get_n_pages (notebook) == 0)
    {
      gtk_window_destroy (new_window);
    }
}

static void
on_new_window_destroyed (
  GtkWidget *              widget,
  DetachableNotebookData * data)
{
  guint idx;
  bool  found =
    g_ptr_array_find (data->new_windows, widget, &idx);
  g_return_if_fail (found);

  GtkNotebook * new_notebook =
    g_ptr_array_index (data->new_notebooks, idx);
  const char * name =
    gtk_widget_get_name (GTK_WIDGET (new_notebook));
  g_debug ("widget %s (%p)", name, new_notebook);
  g_return_if_fail (GTK_IS_NOTEBOOK (new_notebook));
  g_ptr_array_remove_index (data->new_windows, idx);
  g_ptr_array_remove_index (data->new_notebooks, idx);

  /* if the sub window gets destroyed, push pages
   * back to the main window */
  /* detach the notebook pages in reverse sequence
   * to avoid index errors */
  int n_pages = gtk_notebook_get_n_pages (new_notebook);
  for (int i = n_pages - 1; i >= 0; i--)
    {
      GtkWidget * page =
        gtk_notebook_get_nth_page (new_notebook, i);
      GtkWidget * tab_label =
        gtk_notebook_get_tab_label (new_notebook, page);
      g_object_ref (page);
      g_object_ref (tab_label);
      gtk_notebook_detach_tab (new_notebook, page);
      gtk_notebook_append_page (
        data->notebook, page, tab_label);
      g_object_unref (page);
      g_object_unref (tab_label);
      gtk_notebook_set_tab_detachable (
        data->notebook, page, true);
      gtk_notebook_set_tab_reorderable (
        data->notebook, page, true);
    }
  g_object_unref (new_notebook);
}

typedef struct DetachableNotebookDeleteEventData
{
  DetachableNotebookData * data;
  GtkWidget *              page;
} DetachableNotebookDeleteEventData;

static bool
on_new_window_close_request (
  GtkWidget *                         widget,
  DetachableNotebookDeleteEventData * delete_data)
{
  GtkWidget * page = delete_data->page;

  char * val =
    g_hash_table_lookup (delete_data->data->ht, page);
  g_return_val_if_fail (val, false);
  char key_detached[600];
  sprintf (key_detached, "%s-detached", val);
  char key_size[600];
  sprintf (key_size, "%s-size", val);

  int w, h;
  gtk_window_get_default_size (GTK_WINDOW (widget), &w, &h);
  g_settings_set_boolean (S_UI_PANELS, key_detached, false);
  g_settings_set (S_UI_PANELS, key_size, "(ii)", w, h);
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
  DetachableNotebookData * data)
{
  g_return_val_if_fail (data, NULL);

  GtkWindow *   new_window = GTK_WINDOW (gtk_window_new ());
  GtkNotebook * new_notebook =
    GTK_NOTEBOOK (gtk_notebook_new ());
  g_object_ref_sink (new_notebook);
  gtk_window_set_child (
    GTK_WINDOW (new_window), GTK_WIDGET (new_notebook));

  const char * title = "Zrythm";
  /*const char * role = "zrythm-panel";*/

#define SET_TITLE_AND_ROLE(w, t, r) \
  if (page == GTK_WIDGET (w)) \
    { \
      title = t; \
    }

  /* FIXME the names should be fetched from the
   * tab labels instead of repeating the names
   * here */
  SET_TITLE_AND_ROLE (
    MW_BOT_DOCK_EDGE->mixer_box, _ ("Mixer"), "mixer");
  SET_TITLE_AND_ROLE (
    MW_BOT_DOCK_EDGE->modulator_view_box, _ ("Modulators"),
    "modulators");
  SET_TITLE_AND_ROLE (
    MW_BOT_DOCK_EDGE->chord_pad_panel_box, _ ("Chord Pad"),
    "chord-pad");
  SET_TITLE_AND_ROLE (
    MW_BOT_DOCK_EDGE->clip_editor_box, _ ("Editor"), "editor");
  SET_TITLE_AND_ROLE (
    MW_LEFT_DOCK_EDGE->track_inspector_scroll,
    _ ("Track Inspector"), "track-inspector");
  SET_TITLE_AND_ROLE (
    MW_LEFT_DOCK_EDGE->plugin_inspector_scroll,
    _ ("Plugin Inspector"), "plugin-inspector");
  SET_TITLE_AND_ROLE (
    MW_RIGHT_DOCK_EDGE->plugin_browser_box,
    _ ("Plugin Browser"), "plugin-browser");
  SET_TITLE_AND_ROLE (
    MW_RIGHT_DOCK_EDGE->file_browser_box, _ ("File Browser"),
    "file-browser");
  SET_TITLE_AND_ROLE (
    MW_RIGHT_DOCK_EDGE->chord_pack_browser_box,
    _ ("Chord Preset Browser"), "chord-pack-browser");
  SET_TITLE_AND_ROLE (
    MW_RIGHT_DOCK_EDGE->monitor_section_box, _ ("Monitor"),
    "monitor");
  SET_TITLE_AND_ROLE (
    MW_MAIN_NOTEBOOK->timeline_plus_event_viewer_paned,
    _ ("Timeline"), "timeline");
  SET_TITLE_AND_ROLE (
    MW_MAIN_NOTEBOOK->cc_bindings_box, _ ("MIDI CC Bindings"),
    "midi-cc-bindings");
  SET_TITLE_AND_ROLE (
    MW_MAIN_NOTEBOOK->port_connections_box,
    _ ("Port Connections"), "port-connections");
  SET_TITLE_AND_ROLE (
    MW_MAIN_NOTEBOOK->scenes_box, _ ("Scenes"), "scenes");

#undef SET_TITLE_AND_ROLE

  gtk_window_set_title (new_window, title);
  /*gtk_window_set_role (*/
  /*new_window, role);*/

  /* very important for DND */
  gtk_notebook_set_group_name (
    new_notebook, "foldable-notebook-group");

  g_signal_connect (
    G_OBJECT (new_notebook), "page-removed",
    G_CALLBACK (on_new_notebook_page_removed), new_window);
  g_signal_connect (
    G_OBJECT (new_window), "destroy",
    G_CALLBACK (on_new_window_destroyed), data);

  DetachableNotebookDeleteEventData * delete_data =
    object_new (DetachableNotebookDeleteEventData);
  delete_data->data = data;
  delete_data->page = page;
  g_signal_connect_data (
    G_OBJECT (new_window), "close-request",
    G_CALLBACK (on_new_window_close_request), delete_data,
    (GClosureNotify) free_delete_data, 0);
  gtk_window_set_icon_name (GTK_WINDOW (new_window), "zrythm");
  gtk_window_set_transient_for (
    GTK_WINDOW (new_window), GTK_WINDOW (data->parent_window));
  gtk_window_set_destroy_with_parent (new_window, true);

  /* set application so that actions are connected
   * properly */
  gtk_window_set_application (
    GTK_WINDOW (new_window), GTK_APPLICATION (zrythm_app));

  gtk_window_present (GTK_WINDOW (new_window));
  gtk_widget_set_visible (page, true);

  g_ptr_array_add (data->new_windows, new_window);
  g_ptr_array_add (data->new_notebooks, new_notebook);

  char * val = g_hash_table_lookup (data->ht, page);
  g_return_val_if_fail (val, NULL);
  char key_detached[600];
  sprintf (key_detached, "%s-detached", val);
  char key_size[600];
  sprintf (key_size, "%s-size", val);

  /* save/load settings */
  g_settings_set_boolean (S_UI_PANELS, key_detached, true);
  GVariant * size_val =
    g_settings_get_value (S_UI_PANELS, key_size);
  int width = (int) g_variant_get_int32 (
    g_variant_get_child_value (size_val, 0));
  int height = (int) g_variant_get_int32 (
    g_variant_get_child_value (size_val, 1));
  g_debug ("loading %s size %d %d", val, width, height);
  gtk_window_set_default_size (
    GTK_WINDOW (new_window), width, height);
  g_variant_unref (size_val);

  return new_notebook;
}

static void
on_detachable_notebook_destroyed (
  GtkWidget *              widget,
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
  g_return_if_fail (GTK_IS_NOTEBOOK (old_notebook));
  int n_pages = gtk_notebook_get_n_pages (old_notebook);
  for (int i = n_pages - 1; i >= 0; i--)
    {
      GtkWidget * page =
        gtk_notebook_get_nth_page (old_notebook, i);
      GtkWidget * tab_label =
        gtk_notebook_get_tab_label (old_notebook, page);

      char * val = g_hash_table_lookup (data->ht, page);
      g_return_if_fail (val);
      char key_detached[600];
      sprintf (key_detached, "%s-detached", val);
      bool needs_detach =
        g_settings_get_boolean (S_UI_PANELS, key_detached);

      if (needs_detach)
        {
          g_object_ref (page);
          g_object_ref (tab_label);
          GtkNotebook * new_notebook =
            on_create_window (old_notebook, page, data);
          gtk_notebook_detach_tab (old_notebook, page);
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
  g_return_if_fail (GTK_IS_NOTEBOOK (notebook));
  DetachableNotebookData * data =
    object_new (DetachableNotebookData);
  data->notebook = notebook;
  data->new_windows = g_ptr_array_new ();
  data->new_notebooks = g_ptr_array_new ();
  data->parent_window = parent_window;

  /* prepare hashtable */
#define ADD_PAIR(key, w) \
  g_return_if_fail (key); \
  g_return_if_fail (w); \
  g_hash_table_insert (data->ht, w, g_strdup (key))

  data->ht = g_hash_table_new_full (NULL, NULL, NULL, g_free);
  ADD_PAIR (
    "track-inspector",
    MW_LEFT_DOCK_EDGE->track_inspector_scroll);
  ADD_PAIR (
    "plugin-inspector",
    MW_LEFT_DOCK_EDGE->plugin_inspector_scroll);
  ADD_PAIR (
    "plugin-browser", MW_RIGHT_DOCK_EDGE->plugin_browser_box);
  ADD_PAIR (
    "file-browser", MW_RIGHT_DOCK_EDGE->file_browser_box);
  ADD_PAIR (
    "monitor-section",
    MW_RIGHT_DOCK_EDGE->monitor_section_box);
  ADD_PAIR (
    "chord-pack-browser",
    MW_RIGHT_DOCK_EDGE->chord_pack_browser_box);
  ADD_PAIR (
    "modulator-view", MW_BOT_DOCK_EDGE->modulator_view_box);
  ADD_PAIR ("mixer", MW_BOT_DOCK_EDGE->mixer_box);
  ADD_PAIR ("clip-editor", MW_BOT_DOCK_EDGE->clip_editor_box);
  ADD_PAIR (
    "chord-pad", MW_BOT_DOCK_EDGE->chord_pad_panel_box);
  ADD_PAIR (
    "timeline",
    MW_MAIN_NOTEBOOK->timeline_plus_event_viewer_paned);
  ADD_PAIR ("cc-bindings", MW_MAIN_NOTEBOOK->cc_bindings_box);
  ADD_PAIR (
    "port-connections",
    MW_MAIN_NOTEBOOK->port_connections_box);
  ADD_PAIR ("scenes", MW_MAIN_NOTEBOOK->scenes_box);

#undef ADD_PAIR

  g_signal_connect (
    G_OBJECT (notebook), "create-window",
    G_CALLBACK (on_create_window), data);
  g_signal_connect_after (
    G_OBJECT (notebook), "destroy",
    G_CALLBACK (on_detachable_notebook_destroyed), data);

  detach_pages_programmatically (notebook, data);
}

/**
 * Wraps the message area in a scrolled window.
 */
void
z_gtk_message_dialog_wrap_message_area_in_scroll (
  GtkMessageDialog * dialog,
  int                min_width,
  int                min_height)
{
  GtkBox * box = GTK_BOX (gtk_message_dialog_get_message_area (
    GTK_MESSAGE_DIALOG (dialog)));
  GtkWidget * secondary_area =
    z_gtk_widget_get_nth_child (GTK_WIDGET (box), 1);
  gtk_box_remove (GTK_BOX (box), secondary_area);
  GtkWidget * scrolled_window = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_min_content_width (
    GTK_SCROLLED_WINDOW (scrolled_window), min_width);
  gtk_scrolled_window_set_min_content_height (
    GTK_SCROLLED_WINDOW (scrolled_window), min_height);
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (scrolled_window), secondary_area);
  gtk_box_append (GTK_BOX (box), scrolled_window);
}

/**
 * Returns the full text contained in the text
 * buffer.
 *
 * Must be free'd using g_free().
 */
char *
z_gtk_text_buffer_get_full_text (GtkTextBuffer * buffer)
{
  GtkTextIter start_iter, end_iter;
  gtk_text_buffer_get_start_iter (buffer, &start_iter);
  gtk_text_buffer_get_end_iter (buffer, &end_iter);
  return gtk_text_buffer_get_text (
    GTK_TEXT_BUFFER (buffer), &start_iter, &end_iter, false);
}

/**
 * Generates a screenshot image for the given
 * widget.
 *
 * See gdk_pixbuf_savev() for the parameters.
 *
 * @param accept_fallback Whether to accept a
 *   fallback "no image" pixbuf.
 * @param[out] ret_dir Placeholder for directory to
 *   be deleted after using the screenshot.
 * @param[out] ret_path Placeholder for absolute
 *   path to the screenshot.
 */
void
z_gtk_generate_screenshot_image (
  GtkWidget *  widget,
  const char * type,
  char **      option_keys,
  char **      option_values,
  char **      ret_dir,
  char **      ret_path,
  bool         accept_fallback)
{
  g_return_if_fail (*ret_dir == NULL && *ret_path == NULL);

  GdkPaintable * paintable = gtk_widget_paintable_new (widget);
  GtkSnapshot *  snapshot = gtk_snapshot_new ();
  gdk_paintable_snapshot (
    paintable, GDK_SNAPSHOT (snapshot),
    gtk_widget_get_allocated_width (GTK_WIDGET (widget)),
    gtk_widget_get_allocated_height (GTK_WIDGET (widget)));
  GskRenderNode * node = gtk_snapshot_free_to_node (snapshot);
  GskRenderer *   renderer = gsk_renderer_new_for_surface (
      z_gtk_widget_get_surface (GTK_WIDGET (MAIN_WINDOW)));
  g_return_if_fail (renderer);
  GdkTexture * texture =
    gsk_renderer_render_texture (renderer, node, NULL);

  GError * err = NULL;
  *ret_dir = g_dir_make_tmp ("zrythm-widget-XXXXXX", &err);
  if (*ret_dir == NULL)
    {
      g_warning (
        "failed creating temporary dir: %s", err->message);
      return;
    }
  char * abs_path =
    g_build_filename (*ret_dir, "screenshot.png", NULL);

  /* note: can also use
   * gdk_pixbuf_get_from_texture() for other
   * formats */
  bool ret = gdk_texture_save_to_png (texture, abs_path);

  g_object_unref (texture);
  gsk_render_node_unref (node);
  gsk_renderer_unrealize (renderer);
  g_object_unref (renderer);
  /* SEGFAULT */
  /*g_object_unref (snapshot);*/
  g_object_unref (paintable);

  if (!ret)
    {
      g_warning ("screenshot save failed");
      return;
    }

  *ret_path = abs_path;
  g_message ("saved widget screenshot to %s", *ret_path);
}

/**
 * Sets the action target of the given GtkActionable
 * to be binded to the given setting.
 *
 * Mainly used for binding GSettings keys to toggle
 * buttons.
 */
void
z_gtk_actionable_set_action_from_setting (
  GtkActionable * actionable,
  GSettings *     settings,
  const char *    key)
{
  GSimpleActionGroup * action_group =
    g_simple_action_group_new ();
  GAction * action = g_settings_create_action (settings, key);
  g_action_map_add_action (
    G_ACTION_MAP (action_group), action);
  char * group_prefix =
    g_strdup_printf ("%s-action-group", key);
  gtk_widget_insert_action_group (
    GTK_WIDGET (actionable), group_prefix,
    G_ACTION_GROUP (action_group));
  char * action_name =
    g_strdup_printf ("%s.%s", group_prefix, key);
  gtk_actionable_set_action_name (actionable, action_name);
  g_free (group_prefix);
  g_free (action_name);
}

/**
 * Returns column number or -1 if not found or on
 * error.
 */
int
z_gtk_tree_view_column_get_column_id (GtkTreeViewColumn * col)
{
  GtkTreeView * tree_view =
    GTK_TREE_VIEW (gtk_tree_view_column_get_tree_view (col));
  g_return_val_if_fail (tree_view != NULL, -1);

  GList * cols = gtk_tree_view_get_columns (tree_view);

  int num = g_list_index (cols, (gpointer) col);

  g_list_free (cols);

  return num;
}

bool
z_gtk_is_event_button (GdkEvent * ev)
{
  g_return_val_if_fail (ev != NULL, false);

  return GDK_IS_EVENT_TYPE (ev, GDK_BUTTON_PRESS)
         || GDK_IS_EVENT_TYPE (ev, GDK_BUTTON_RELEASE);
}

/**
 * Gets the visible rectangle from the scrolled
 * window's adjustments.
 */
void
z_gtk_scrolled_window_get_visible_rect (
  GtkScrolledWindow * scroll,
  graphene_rect_t *   rect)
{
  GtkAdjustment * hadj =
    gtk_scrolled_window_get_hadjustment (scroll);
  GtkAdjustment * vadj =
    gtk_scrolled_window_get_vadjustment (scroll);
  rect->origin.x = (float) gtk_adjustment_get_value (hadj);
  rect->origin.y = (float) gtk_adjustment_get_value (vadj);
  rect->size.width =
    (float) gtk_adjustment_get_page_size (hadj);
  rect->size.height =
    (float) gtk_adjustment_get_page_size (vadj);
}

void
z_gtk_graphene_rect_t_to_gdk_rectangle (
  GdkRectangle *    rect,
  graphene_rect_t * grect)
{
  rect->x = (int) grect->origin.x;
  rect->y = (int) grect->origin.y;
  rect->width = (int) grect->size.width;
  rect->height = (int) grect->size.height;
}

typedef struct
{
  GtkDialog * dialog;
  gint        response_id;
  GMainLoop * loop;
  gboolean    destroyed;
} RunInfo;

static void
shutdown_loop (RunInfo * ri)
{
  if (g_main_loop_is_running (ri->loop))
    g_main_loop_quit (ri->loop);
}

static void
run_unmap_handler (GtkDialog * dialog, gpointer data)
{
  RunInfo * ri = data;

  shutdown_loop (ri);
}

static void
run_response_handler (
  GtkDialog * dialog,
  gint        response_id,
  gpointer    data)
{
  RunInfo * ri;

  ri = data;

  ri->response_id = response_id;

  shutdown_loop (ri);
}

static gboolean
run_delete_handler (GtkDialog * dialog, gpointer data)
{
  RunInfo * ri = data;

  shutdown_loop (ri);

  return TRUE; /* Do not destroy */
}

static void
run_destroy_handler (GtkDialog * dialog, gpointer data)
{
  RunInfo * ri = data;

  /* shutdown_loop will be called by run_unmap_handler */

  ri->destroyed = TRUE;
}

#if 0
static void
dialog_response_cb (
  GtkDialog * dialog,
  gint        response_id,
  DialogResponseData * data)
{
  data->response = response_id;
  g_main_loop_quit (data->main_loop);
}
#endif

/**
 * Mimics the blocking behavior.
 *
 * @return The response ID.
 */
int
z_gtk_dialog_run (GtkDialog * dialog, bool destroy_on_close)
{
  RunInfo  ri = { NULL, GTK_RESPONSE_NONE, NULL, FALSE };
  gboolean was_modal;
  gulong   response_handler;
  gulong   unmap_handler;
  gulong   destroy_handler;
  gulong   delete_handler;

  g_return_val_if_fail (GTK_IS_DIALOG (dialog), -1);

  g_object_ref (dialog);

  was_modal = gtk_window_get_modal (GTK_WINDOW (dialog));
  if (!was_modal)
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  if (!gtk_widget_get_visible (GTK_WIDGET (dialog)))
    gtk_widget_show (GTK_WIDGET (dialog));

  response_handler = g_signal_connect (
    dialog, "response", G_CALLBACK (run_response_handler),
    &ri);

  unmap_handler = g_signal_connect (
    dialog, "unmap", G_CALLBACK (run_unmap_handler), &ri);

  delete_handler = g_signal_connect (
    dialog, "close-request", G_CALLBACK (run_delete_handler),
    &ri);

  destroy_handler = g_signal_connect (
    dialog, "destroy", G_CALLBACK (run_destroy_handler), &ri);

  ri.loop = g_main_loop_new (NULL, FALSE);

  /*gdk_threads_leave ();*/
  g_main_loop_run (ri.loop);
  /*gdk_threads_enter ();*/

  g_main_loop_unref (ri.loop);

  ri.loop = NULL;

  if (!ri.destroyed)
    {
      if (!was_modal)
        gtk_window_set_modal (GTK_WINDOW (dialog), FALSE);

      g_signal_handler_disconnect (dialog, response_handler);
      g_signal_handler_disconnect (dialog, unmap_handler);
      g_signal_handler_disconnect (dialog, delete_handler);
      g_signal_handler_disconnect (dialog, destroy_handler);
    }

  g_object_unref (dialog);

  if (destroy_on_close && GTK_IS_WINDOW (dialog))
    gtk_window_destroy (GTK_WINDOW (dialog));

  return ri.response_id;
}

/**
 * The popover must already exist as a children
 * of its intended widget (or a common parent).
 *
 * This function will set the new menu and show it.
 */
void
z_gtk_show_context_menu_from_g_menu (
  GtkPopoverMenu * popover_menu,
  double           x,
  double           y,
  GMenu *          menu)
{
  g_return_if_fail (GTK_IS_POPOVER_MENU (popover_menu));
  gtk_popover_menu_set_menu_model (
    popover_menu, G_MENU_MODEL (menu));
  gtk_popover_set_pointing_to (
    GTK_POPOVER (popover_menu),
    &Z_GDK_RECTANGLE_INIT_UNIT ((int) x, (int) y));
  /*gtk_popover_set_has_arrow (*/
  /*GTK_POPOVER (popover_menu), false);*/
  gtk_popover_popup (GTK_POPOVER (popover_menu));
  g_debug ("popup %p", popover_menu);
}

/**
 * Returns the bitmask of the selected action
 * during a drop (eg, GDK_ACTION_COPY).
 */
GdkDragAction
z_gtk_drop_target_get_selected_action (
  GtkDropTarget * drop_target)
{
  GdkDrop * drop =
    gtk_drop_target_get_current_drop (drop_target);
  GdkDrag * drag = gdk_drop_get_drag (drop);

  /* if not in-app drag, only copy is applicable */
  if (!drag)
    return GDK_ACTION_COPY;

  GdkDragAction action = gdk_drag_get_selected_action (drag);

  return action;
}

GtkIconTheme *
z_gtk_icon_theme_get_default (void)
{
  GdkDisplay *   display = gdk_display_get_default ();
  GtkIconTheme * icon_theme =
    gtk_icon_theme_get_for_display (display);
  return icon_theme;
}

char *
z_gtk_file_chooser_get_filename (GtkFileChooser * file_chooser)
{
  GFile * file = gtk_file_chooser_get_file (file_chooser);
  if (!file)
    {
      g_debug ("file is NULL");
      return NULL;
    }

  char * path = g_file_get_path (file);
  g_object_unref (file);

  return path;
}

void
z_gtk_file_chooser_set_file_from_path (
  GtkFileChooser * file_chooser,
  const char *     filename)
{
  GFile * file = g_file_new_for_path (filename);
  g_return_if_fail (file);
  gtk_file_chooser_set_file (file_chooser, file, NULL);
  g_object_unref (file);
}

/**
 * Returns the text on the clipboard, or NULL if
 * there is nothing or the content is not text.
 */
char *
z_gdk_clipboard_get_text (GdkClipboard * clipboard)
{
  /* initialize a GValue to receive text */
  GValue value = G_VALUE_INIT;
  g_value_init (&value, G_TYPE_STRING);

  /* Get the content provider for the clipboard,
   * and ask it for text */
  GdkContentProvider * provider =
    gdk_clipboard_get_content (clipboard);

  /* if the clipboard data was not set by this
   * process or clipboard is empty, return NULL */
  if (!provider)
    {
      g_debug ("clipboard content provider is NULL");
      return NULL;
    }

  /* if the content provider does not contain text,
   * we are not interested */
  if (!gdk_content_provider_get_value (provider, &value, NULL))
    {
      g_debug (
        "clipboard content provider does not "
        "contain text");
      return NULL;
    }

  char * str = g_strdup (g_value_get_string (&value));

  g_value_unset (&value);

  return str;
}

#ifdef HAVE_X11
Window
z_gtk_window_get_x11_xid (GtkWindow * window)
{
#  ifdef GDK_WINDOWING_WAYLAND
  GdkDisplay * display = gdk_display_get_default ();
  g_return_val_if_fail (display, false);
  if (GDK_IS_WAYLAND_DISPLAY (display))
    return 0;
#  endif

  GtkNative *  native = GTK_NATIVE (window);
  GdkSurface * surface = gtk_native_get_surface (native);
  Window       xid = gdk_x11_surface_get_xid (surface);
  return xid;
}
#endif

#ifdef _WOE32
HWND
z_gtk_window_get_windows_hwnd (GtkWindow * window)
{
  GtkNative *  native = GTK_NATIVE (window);
  GdkSurface * surface = gtk_native_get_surface (native);
  HWND         hwnd = GDK_SURFACE_HWND (surface);
  g_return_val_if_fail (hwnd, NULL);
  return hwnd;
}
#endif

#ifdef __APPLE__
void *
z_gtk_window_get_nsview (GtkWindow * window)
{
  GtkNative *  native = GTK_NATIVE (window);
  GdkSurface * surface = gtk_native_get_surface (native);
  void *       nsview = NULL;
  /* FIXME enable when patch is merged */
#  if 0
    gdk_macos_surface_get_view (surface);
#  endif
  g_return_val_if_fail (nsview, NULL);
  return nsview;
}
#endif

/**
 * Creates a new pixbuf for the given icon scaled
 * at the given width/height.
 *
 * Pass -1 for either width/height to maintain
 * aspect ratio.
 */
GdkPixbuf *
z_gdk_pixbuf_new_from_icon_name (
  const char * icon_name,
  int          width,
  int          height,
  int          scale,
  GError **    error)
{
  GtkIconTheme * icon_theme = z_gtk_icon_theme_get_default ();
  GtkIconPaintable * paintable = gtk_icon_theme_lookup_icon (
    icon_theme, icon_name, NULL, width, scale,
    GTK_TEXT_DIR_NONE, GTK_ICON_LOOKUP_PRELOAD);
  GFile *     file = gtk_icon_paintable_get_file (paintable);
  char *      path = g_file_get_path (file);
  GdkPixbuf * pixbuf = NULL;
  GError *    err = NULL;
  if (path)
    {
      pixbuf = gdk_pixbuf_new_from_file_at_scale (
        path, width, height, true, &err);
      g_free (path);
    }
  else
    {
      char *       uri = g_file_get_uri (file);
      const char * resource_prefix = "resource://";
      const char * resource_path =
        uri + strlen (resource_prefix);
      pixbuf = gdk_pixbuf_new_from_resource_at_scale (
        resource_path, width, height, true, &err);
      g_free (uri);
    }
  g_object_unref (file);
  g_object_unref (paintable);

  if (!pixbuf)
    {
      g_set_error (
        error, Z_UTILS_GTK_ERROR, Z_UTILS_GTK_ERROR_FAILED,
        "Failed to load pixbuf for icon %s", icon_name);
      return NULL;
    }

  return pixbuf;
}

/**
 * Creates a new texture for the given icon scaled
 * at the given width/height.
 *
 * Pass -1 for either width/height to maintain
 * aspect ratio.
 */
GdkTexture *
z_gdk_texture_new_from_icon_name (
  const char * icon_name,
  int          width,
  int          height,
  int          scale)
{
  /* FIXME pass GError and handle gracefully */
  GdkPixbuf * pixbuf = z_gdk_pixbuf_new_from_icon_name (
    icon_name, width, height, scale, NULL);
  g_return_val_if_fail (pixbuf, NULL);

  return gdk_texture_new_for_pixbuf (pixbuf);
}

void
z_gtk_print_graphene_rect (graphene_rect_t * rect)
{
  g_message (
    "x: %f | y: %f | width: %f | height: %f", rect->origin.x,
    rect->origin.y, rect->size.width, rect->size.height);
}

/**
 * Prints the widget's hierarchy (parents).
 */
void
z_gtk_widget_print_hierarchy (GtkWidget * widget)
{
  GQueue *    queue = g_queue_new ();
  GtkWidget * parent = widget;
  while ((parent = gtk_widget_get_parent (parent)))
    {
      g_queue_push_tail (queue, parent);
    }

  const int spaces_per_child = 2;
  int       cur_spaces = 0;
  GString * gstr = g_string_new (NULL);
  while ((parent = (GtkWidget *) g_queue_pop_tail (queue)))
    {
      /* TODO print parent class in () */
      g_string_append_printf (
        gstr, "%s\n", gtk_widget_get_name (parent));
      cur_spaces += spaces_per_child;
      for (int j = 0; j < cur_spaces; j++)
        {
          g_string_append_c (gstr, ' ');
        }
    }

  g_string_append (gstr, gtk_widget_get_name (widget));
  char * str = g_string_free (gstr, false);
  g_message ("%s", str);
  g_queue_free (queue);
}

const char *
z_gtk_get_gsk_renderer_type (void)
{
  static const char * renderer_type = NULL;
  if (renderer_type)
    return renderer_type;

  GdkSurface * surface =
    gdk_surface_new_toplevel (gdk_display_get_default ());
  GskRenderer * renderer =
    gsk_renderer_new_for_surface (surface);
  if (string_is_equal (
        G_OBJECT_TYPE_NAME (renderer), "GskGLRenderer"))
    {
      renderer_type = "GL";
    }
  else if (string_is_equal (
             G_OBJECT_TYPE_NAME (renderer), "GskCairoRenderer"))
    {
      renderer_type = "Cairo";
    }
  else
    {
      g_warning ("unknown renderer");
      renderer_type = "Unknown";
    }

  gsk_renderer_unrealize (renderer);
  g_object_unref (renderer);
  gdk_surface_destroy (surface);

  return renderer_type;
}

/**
 * A shortcut callback to use for simple actions.
 *
 * A single parameter must be passed: action name
 * under "app.".
 */
gboolean
z_gtk_simple_action_shortcut_func (
  GtkWidget * widget,
  GVariant *  args,
  gpointer    user_data)
{
  gsize        size;
  const char * action_name =
    g_variant_get_string (args, &size);
  char **    strs = g_strsplit (action_name, "::", -1);
  char *     param = strs[1];
  GVariant * variant = NULL;
  if (param)
    variant = g_variant_new_string (param);
  g_action_group_activate_action (
    G_ACTION_GROUP (zrythm_app), strs[0], variant);
  g_message ("activating %s::%s", action_name, param);
  g_strfreev (strs);

  return true;
}

/**
 * Recursively searches the children of \ref widget
 * for a child of type \ref type.
 */
GtkWidget *
z_gtk_widget_find_child_of_type (GtkWidget * widget, GType type)
{
  for (GtkWidget * child = gtk_widget_get_first_child (widget);
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (G_OBJECT_TYPE (child) == type)
        {
          return child;
        }
      else
        {
          GtkWidget * inner_match =
            z_gtk_widget_find_child_of_type (child, type);
          if (inner_match)
            return inner_match;
        }
    }

  return NULL;
}

void
z_gtk_list_box_remove_all_children (GtkListBox * list_box)
{
  GtkListBoxRow * row = NULL;
  while (
    (row = gtk_list_box_get_row_at_index (list_box, 0))
    != NULL)
    {
      gtk_list_box_remove (list_box, GTK_WIDGET (row));
    }
}

void
z_graphene_rect_print (const graphene_rect_t * rect)
{
  g_return_if_fail (rect->size.width >= 0);
  g_return_if_fail (rect->size.height >= 0);
  g_message (
    "graphene rect: x %f y %f w %f h %f", rect->origin.x,
    rect->origin.y, rect->size.width, rect->size.height);
}
