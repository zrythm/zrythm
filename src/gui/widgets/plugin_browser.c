/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/create_tracks_action.h"
#include "audio/engine.h"
#include "audio/mixer.h"
#include "gui/widgets/expander_box.h"
#include "gui/widgets/plugin_browser.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/right_dock_edge.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/ui.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (PluginBrowserWidget,
               plugin_browser_widget,
               GTK_TYPE_PANED)

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

/**
 * Called when row is double clicked.
 */
static void
on_row_activated (GtkTreeView       *tree_view,
               GtkTreePath       *tp,
               GtkTreeViewColumn *column,
               gpointer           user_data)
{
  GtkTreeModel * model = GTK_TREE_MODEL (user_data);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    model,
    &iter,
    tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    model,
    &iter,
    PL_COLUMN_DESCR,
    &value);
  PluginDescriptor * descr =
    g_value_get_pointer (&value);

  TrackType tt;
  if (plugin_is_instrument (descr))
    tt = TRACK_TYPE_INSTRUMENT;
  else
    tt = TRACK_TYPE_BUS;

  UndoableAction * ua =
    create_tracks_action_new (
      tt,
      descr,
      NULL,
      TRACKLIST->num_tracks,
      1);

  undo_manager_perform (UNDO_MANAGER, ua);
}

/**
 * Visible function for plugin tree model.
 *
 * Used for filtering based on selected category.
 */
static gboolean
visible_func (GtkTreeModel *model,
              GtkTreeIter  *iter,
              gpointer      data)
{
  PluginBrowserWidget * self = Z_PLUGIN_BROWSER_WIDGET (data);

  // Visible if row is non-empty and category matches selected
  PluginDescriptor *descr;
  gboolean visible = FALSE;

  gtk_tree_model_get (
    model, iter, PL_COLUMN_DESCR, &descr, -1);
  if (!self->selected_category)
    {
      visible = TRUE;
    }
  else if (descr->category &&
           strcmp (descr->category,
                   self->selected_category) == 0)
    {
      visible = TRUE;
    }

  return visible;
}

static void
show_context_menu (
  PluginBrowserWidget * self,
  PluginDescriptor *    descr)
{
  GtkWidget *menu;
  /*GtkMenuItem *menuitem;*/
  menu = gtk_menu_new();

#define APPEND(mi) \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL (menu), \
    GTK_WIDGET (menuitem));


#undef APPEND

  gtk_menu_attach_to_widget (
    GTK_MENU (menu),
    GTK_WIDGET (self), NULL);
  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static int
do_after_init (
  PluginBrowserWidget * self)
{
  if (!GTK_IS_PANED (self))
    return G_SOURCE_REMOVE;

  /* set divider position */
  int divider_pos =
    g_settings_get_int (
      S_UI,
      "browser-divider-position");
  gtk_paned_set_position (
    GTK_PANED (self),
    divider_pos);

  return G_SOURCE_REMOVE;
}

static void
on_plugin_right_click (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  PluginBrowserWidget * self)
{
  if (n_press != 1)
    return;

  GtkTreePath *path;
  GtkTreeViewColumn *column;

  GtkTreeSelection * selection =
    gtk_tree_view_get_selection (
      (self->plugin_tree_view));
  if (!gtk_tree_view_get_path_at_pos
      (GTK_TREE_VIEW(self->plugin_tree_view),
       x, y,
       &path, &column, NULL, NULL))

      // if we can't find path at pos, we surely don't
      // want to pop up the menu
      return;

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

  show_context_menu (self, descr);
}


static int
update_plugin_info_label (PluginBrowserWidget * self,
                          gpointer user_data)
{
  char * label = (char *) user_data;

  gtk_label_set_text (self->plugin_info, label);

  return G_SOURCE_REMOVE;
}

static void
on_selection_changed (GtkTreeSelection * ts,
                      gpointer         user_data)
{
  PluginBrowserWidget * self = Z_PLUGIN_BROWSER_WIDGET (user_data);
  GtkTreeView * tv = gtk_tree_selection_get_tree_view (ts);
  GtkTreeModel * model = gtk_tree_view_get_model (tv);
  GList * selected_rows = gtk_tree_selection_get_selected_rows (ts,
                                                                NULL);
  if (selected_rows)
    {
      GtkTreePath * tp = (GtkTreePath *)g_list_first (selected_rows)->data;
      /*gint * indices = gtk_tree_path_get_indices (tp);*/
      /*GtkTreeRowReference *rr =*/
        /*gtk_tree_row_reference_new (MAIN_WINDOW->plugin_browser->plugin_tree_model,*/
                                    /*tp);*/
      GtkTreeIter iter;
      gtk_tree_model_get_iter (model,
                               &iter,
                               tp);
      GValue value = G_VALUE_INIT;

      if (model == self->category_tree_model)
        {
          gtk_tree_model_get_value (model,
                                    &iter,
                                    0,
                                    &value);
          self->selected_category =
            g_value_get_string (&value);
          gtk_tree_model_filter_refilter (
            self->plugin_tree_model);
        }
      else if (model ==
               GTK_TREE_MODEL (self->plugin_tree_model))
        {
          gtk_tree_model_get_value (model,
                                    &iter,
                                    PL_COLUMN_DESCR,
                                    &value);
          PluginDescriptor * descr =
            g_value_get_pointer (&value);
          char * label = g_strdup_printf (
            "%s\n%s, %d\nAudio: %d, %d\nMidi: %d, "
            "%d\nControls: %d, %d",
            descr->author,
            descr->category,
            descr->protocol,
            descr->num_audio_ins,
            descr->num_audio_outs,
            descr->num_midi_ins,
            descr->num_midi_outs,
            descr->num_ctrl_ins,
            descr->num_ctrl_outs);
          update_plugin_info_label (self,
                                    label);
        }
    }
}

static void
on_drag_data_get (GtkWidget        *widget,
               GdkDragContext   *context,
               GtkSelectionData *data,
               guint             info,
               guint             time,
               gpointer          user_data)
{
  GtkTreeView * tv = GTK_TREE_VIEW (user_data);
  PluginDescriptor * descr =
    z_gtk_get_single_selection_pointer (
      tv, PL_COLUMN_DESCR);

  gtk_selection_data_set (data,
    gdk_atom_intern_static_string (
      TARGET_ENTRY_PLUGIN_DESCR),
    32,
    (const guchar *)&descr,
    sizeof (PluginDescriptor));
}

static GtkTreeModel *
create_model_for_collections ()
{
  GtkListStore *list_store;
  /*GtkTreePath *path;*/
  GtkTreeIter iter;
  gint i;

  /* plugin name, index */
  list_store = gtk_list_store_new (2,
                                   G_TYPE_STRING, G_TYPE_INT);

  for (i = 0; i < PLUGIN_MANAGER->num_plugins; i++)
    {
      const gchar * name =
        PLUGIN_MANAGER->plugin_descriptors[i]->name;

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter,
                          0, name,
                          -1);
      gtk_list_store_set (list_store, &iter,
                          1, i,
                          -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_categories ()
{
  GtkListStore *list_store;
  /*GtkTreePath *path;*/
  GtkTreeIter iter;
  gint i;

  /* plugin name, index */
  list_store = gtk_list_store_new (1,
                                   G_TYPE_STRING);

  for (i = 0; i < PLUGIN_MANAGER->num_plugin_categories; i++)
    {
      const gchar * name = PLUGIN_MANAGER->plugin_categories[i];

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter,
                          0, name,
                          -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_plugins (PluginBrowserWidget * self)
{
  GtkListStore *list_store;
  /*GtkTreePath *path;*/
  GtkTreeIter iter;
  gint i;

  /* plugin name, index */
  list_store = gtk_list_store_new (PL_NUM_COLUMNS,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_POINTER);

  for (i = 0; i < PLUGIN_MANAGER->num_plugins; i++)
    {
      PluginDescriptor * descr =
        PLUGIN_MANAGER->plugin_descriptors[i];
      gchar * icon_name = "z-plugins";
      if (!strcmp (descr->category, "Instrument"))
        icon_name = "z-audio-midi";
      else if (!strcmp (descr->category, "Distortion"))
        icon_name = "z-distortionfx";

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
    visible_func,
    self,
    NULL);

  return model;
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

  int match =
    g_strrstr (down_str, down_key) != NULL;

  g_free (str);
  g_free (down_key);
  g_free (down_str);

  return !match;
}

static void
tree_view_setup (
  PluginBrowserWidget * self,
  GtkTreeView *         tree_view,
  GtkTreeModel *        model,
  int                   allow_multi,
  int                   dnd)
{
  gtk_tree_view_set_model (tree_view, model);

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

      /* connect right click handler */
      GtkGestureMultiPress * mp =
        GTK_GESTURE_MULTI_PRESS (
          gtk_gesture_multi_press_new (
            GTK_WIDGET (tree_view)));
      gtk_gesture_single_set_button (
        GTK_GESTURE_SINGLE (mp),
        GDK_BUTTON_SECONDARY);
      g_signal_connect (
        G_OBJECT (mp), "pressed",
        G_CALLBACK (on_plugin_right_click), self);
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
    GTK_TREE_VIEW (tree_view), FALSE);

  if (allow_multi)
    gtk_tree_selection_set_mode (
      gtk_tree_view_get_selection (
        GTK_TREE_VIEW (tree_view)),
      GTK_SELECTION_MULTIPLE);

  if (dnd)
    {
      GtkTargetEntry entries[1];
      entries[0].target = TARGET_ENTRY_PLUGIN_DESCR;
      entries[0].flags = GTK_TARGET_SAME_APP;
      entries[0].info =
        symap_map (
          ZSYMAP, TARGET_ENTRY_PLUGIN_DESCR);
      gtk_tree_view_enable_model_drag_source (
        GTK_TREE_VIEW (tree_view),
        GDK_BUTTON1_MASK,
        entries,
        1,
        GDK_ACTION_COPY);
      g_signal_connect (
        GTK_WIDGET (tree_view), "drag-data-get",
        G_CALLBACK (on_drag_data_get), tree_view);
    }

  g_signal_connect (
    G_OBJECT (
      gtk_tree_view_get_selection (
        GTK_TREE_VIEW (tree_view))), "changed",
     G_CALLBACK (on_selection_changed), self);
}

static void
toggles_changed (
  GtkToggleToolButton * btn,
  PluginBrowserWidget * self)
{
  gtk_tree_model_filter_refilter (
    self->plugin_tree_model);
}

PluginBrowserWidget *
plugin_browser_widget_new ()
{
  PluginBrowserWidget * self =
    g_object_new (
      PLUGIN_BROWSER_WIDGET_TYPE, NULL);

  gtk_label_set_xalign (self->plugin_info, 0);

  /* setup collections */
  self->collection_tree_model =
    create_model_for_collections ();
  tree_view_setup (
    self, self->collection_tree_view,
    self->collection_tree_model, 1,0);

  /* setup categories */
  self->category_tree_model =
    create_model_for_categories ();
  tree_view_setup (
    self, self->category_tree_view,
    self->category_tree_model, 1,0);

  /* populate plugins */
  self->plugin_tree_model =
    GTK_TREE_MODEL_FILTER (
      create_model_for_plugins (self));
  tree_view_setup (
    self, self->plugin_tree_view,
    GTK_TREE_MODEL (
      self->plugin_tree_model), 0, 1);
  g_signal_connect (
    G_OBJECT (self->plugin_tree_view),
    "row-activated",
    G_CALLBACK (on_row_activated),
    self->plugin_tree_model);

  /* for some reason setting the position here
   * gets ignored, so set it after 1 sec */
  g_timeout_add (
    500,(GSourceFunc) do_after_init,
    GTK_PANED (self));

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

  gtk_widget_class_set_css_name (
    klass, "browser");

#define BIND_CHILD(name) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    PluginBrowserWidget, \
    name)

  BIND_CHILD (collection_scroll);
  BIND_CHILD (protocol_scroll);
  BIND_CHILD (category_scroll);
  BIND_CHILD (plugin_scroll);
  BIND_CHILD (collection_tree_view);
  BIND_CHILD (protocol_tree_view);
  BIND_CHILD (category_tree_view);
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
}

static void
plugin_browser_widget_init (
  PluginBrowserWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->stack_switcher =
    GTK_STACK_SWITCHER (gtk_stack_switcher_new ());
  gtk_stack_switcher_set_stack (
    self->stack_switcher,
    self->stack);
  gtk_widget_show_all (
    GTK_WIDGET (self->stack_switcher));
  gtk_box_pack_start (
    self->stack_switcher_box,
    GTK_WIDGET (self->stack_switcher),
    TRUE, TRUE, 0);

  /* set stackswitcher icons */
  GValue iconval1 = G_VALUE_INIT;
  GValue iconval2 = G_VALUE_INIT;
  GValue iconval3 = G_VALUE_INIT;
  g_value_init (&iconval1, G_TYPE_STRING);
  g_value_init (&iconval2, G_TYPE_STRING);
  g_value_init (&iconval3, G_TYPE_STRING);
  g_value_set_string (
    &iconval1, "z-favorites");
  g_value_set_string(
    &iconval2, "iconfinder_category_103432_edited");
  g_value_set_string(
    &iconval3, "plug-solid");

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
    GTK_WIDGET (self->category_scroll),
    "icon-name", &iconval2);
  g_value_set_string (
    &iconval2, _("Category"));
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->category_scroll),
    "title", &iconval2);
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->protocol_scroll),
    "icon-name", &iconval3);
  g_value_set_string (
    &iconval3, _("Protocol"));
  gtk_container_child_set_property (
    GTK_CONTAINER (self->stack),
    GTK_WIDGET (self->protocol_scroll),
    "title", &iconval3);

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
        radio, "hexpand", TRUE, NULL);
    }
  g_list_free (children);
}
