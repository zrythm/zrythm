/*
 * gui/widgets/plugin_browser.c - The plugin, etc., plugin_browser on the right
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "zrythm.h"
#include "audio/engine.h"
#include "audio/mixer.h"
#include "gui/widgets/plugin_browser.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/right_dock_edge.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (PluginBrowserWidget,
               plugin_browser_widget,
               GTK_TYPE_PANED)

enum
{
  COLUMN_ICON,
  COLUMN_NAME,
  COLUMN_DESCR,
  NUM_COLUMNS
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
    COLUMN_DESCR,
    &value);
  PluginDescriptor * descr =
    g_value_get_pointer (&value);
  mixer_add_channel_from_plugin_descr (descr);
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

  gtk_tree_model_get (model, iter, 1, &descr, -1);
  if (!self->selected_category)
    {
      visible = TRUE;
    }
  else if (descr->category &&
      strcmp (descr->category, self->selected_category) == 0)
    {
      visible = TRUE;
    }

  return visible;
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
        /*gtk_tree_row_reference_new (MAIN_WINDOW->plugin_browser->plugins_tree_model,*/
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
                                    COLUMN_NAME,
                                    &value);
          self->selected_category =
            g_value_get_string (&value);
          gtk_tree_model_filter_refilter (
            self->plugins_tree_model);
        }
      else if (model ==
               GTK_TREE_MODEL (self->plugins_tree_model))
        {
          gtk_tree_model_get_value (model,
                                    &iter,
                                    COLUMN_DESCR,
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
  GtkTreeSelection * ts = GTK_TREE_SELECTION (user_data);
  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (ts,
                                                                NULL);
  GtkTreePath * tp = (GtkTreePath *)g_list_first (selected_rows)->data;
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    GTK_TREE_MODEL (MW_PLUGIN_BROWSER->plugins_tree_model),
    &iter,
    tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (MW_PLUGIN_BROWSER->plugins_tree_model),
    &iter,
    COLUMN_DESCR,
    &value);
  PluginDescriptor * descr = g_value_get_pointer (&value);

  gtk_selection_data_set (data,
        gdk_atom_intern_static_string ("PLUGIN_DESCR"),
        32,
        (const guchar *)&descr,
        sizeof (PluginDescriptor));
}

static GtkTreeModel *
create_model_for_types ()
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
  list_store = gtk_list_store_new (NUM_COLUMNS,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_POINTER);

  for (i = 0; i < PLUGIN_MANAGER->num_plugins; i++)
    {
      PluginDescriptor * descr =
        PLUGIN_MANAGER->plugin_descriptors[i];
      gchar * icon_name = "plugins";
      if (!strcmp (descr->category, "Instrument"))
        icon_name = "audio-midi";
      else if (!strcmp (descr->category, "Distortion"))
        icon_name = "distortionfx";

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter,
        COLUMN_ICON, icon_name,
        COLUMN_NAME, descr->name,
        COLUMN_DESCR, descr,
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

static void
expander_callback (GObject    *object,
                   GParamSpec *param_spec,
                   gpointer    user_data)
{
  GtkExpander *expander;

  GtkWidget * scrolled_window = GTK_WIDGET (user_data);
  expander = GTK_EXPANDER (object);

  if (gtk_expander_get_expanded (expander))
    {
      gtk_widget_set_vexpand (GTK_WIDGET (scrolled_window),
                             TRUE);
    }
  else
    {
      gtk_widget_set_vexpand (GTK_WIDGET (scrolled_window),
                             FALSE);
    }
}

static GtkWidget *
tree_view_create (PluginBrowserWidget * self,
                  GtkTreeModel * model,
                  int          allow_multi,
                  int          dnd)
{
  /* instantiate tree view using model */
  GtkWidget * tree_view = gtk_tree_view_new_with_model (
      GTK_TREE_MODEL (model));

  /* init tree view */
  GtkCellRenderer * renderer;
  GtkTreeViewColumn * column;
  if (GTK_TREE_MODEL (self->plugins_tree_model) == model)
    {
      /* column for icon */
      renderer =
        gtk_cell_renderer_pixbuf_new ();
      column =
        gtk_tree_view_column_new_with_attributes (
          "icon", renderer,
          "icon-name", COLUMN_ICON,
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
          "text", COLUMN_NAME,
          NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view),
        column);
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
            GTK_TREE_VIEW (tree_view),
            FALSE);

  if (allow_multi)
    gtk_tree_selection_set_mode (
        gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)),
        GTK_SELECTION_MULTIPLE);

  if (dnd)
    {
      GtkTargetEntry entries[1];
      entries[0].target = TARGET_ENTRY_PLUGIN_DESCR;
      entries[0].flags = GTK_TARGET_SAME_APP;
      entries[0].info = TARGET_ENTRY_ID_PLUGIN_DESCR;
      gtk_tree_view_enable_model_drag_source (
        GTK_TREE_VIEW (tree_view),
        GDK_BUTTON1_MASK,
        entries,
        1,
        GDK_ACTION_COPY);
      g_signal_connect (GTK_WIDGET (tree_view),
                        "drag-data-get",
                        G_CALLBACK (on_drag_data_get),
                        gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)));
    }

  g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view))),
                    "changed",
                    G_CALLBACK (on_selection_changed),
                    self);

  return tree_view;
}

static GtkWidget *
add_scroll_window (GtkTreeView * tree_view)
{
  /* add treeview to scroll window */
  GtkWidget * scrolled_window =
    gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
                     GTK_WIDGET (tree_view));
  return scrolled_window;
}


/**
 * Creates a GtkTreeView using the given model,
 * and puts it in the given expander in a GtkScrolledWindow
 *
 * @return the scroll window
 * TODO FIXME
 */
static GtkScrolledWindow *
create_tree_view_add_to_expander (PluginBrowserWidget * self,
                                  GtkTreeModel * model,
                  GtkExpander  * expander)
{
  GtkWidget * scrolled_window = add_scroll_window
                  (GTK_TREE_VIEW (tree_view_create (self,
                                                    model,
                                                    1,0)));

  /* add scroll window to expander */
  gtk_container_add (
      GTK_CONTAINER (expander),
      scrolled_window);

  /* connect signal to expand/hide */
  g_signal_connect (expander, "notify::expanded",
                  G_CALLBACK (expander_callback),
                  scrolled_window);

  return GTK_SCROLLED_WINDOW (scrolled_window);
}

PluginBrowserWidget *
plugin_browser_widget_new ()
{
  PluginBrowserWidget * self = g_object_new (PLUGIN_BROWSER_WIDGET_TYPE, NULL);

  g_message ("Instantiating plugin_browser widget...");

  gtk_label_set_xalign (self->plugin_info, 0);

  /* set divider position */
  int divider_pos =
    g_settings_get_int (SETTINGS->root,
                      "browser-divider-position");
  gtk_paned_set_position (GTK_PANED (self),
                          divider_pos);

  /* create each tree */
  create_tree_view_add_to_expander (
    self,
    create_model_for_types (),
    GTK_EXPANDER (self->collections_exp));
  create_tree_view_add_to_expander (
    self,
    create_model_for_types (),
    GTK_EXPANDER (self->types_exp));
  self->category_tree_model = create_model_for_categories ();
  GtkScrolledWindow * scrolled_window =
  create_tree_view_add_to_expander (
    self,
    self->category_tree_model,
    GTK_EXPANDER (self->cat_exp));

  /* expand category by default */
  gtk_expander_set_expanded (GTK_EXPANDER (self->cat_exp),
                             TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (scrolled_window),
                          TRUE);

  /* populate plugins */
  self->plugins_tree_model = GTK_TREE_MODEL_FILTER (create_model_for_plugins (self));
  self->plugins_tree_view =
    GTK_TREE_VIEW (tree_view_create (
      self,
      GTK_TREE_MODEL (
        self->plugins_tree_model),
      0,
      1));
  g_signal_connect (G_OBJECT (self->plugins_tree_view),
                    "row-activated",
                    G_CALLBACK (on_row_activated),
                    self->plugins_tree_model);
  GtkWidget * plugin_scroll_window =
    add_scroll_window (self->plugins_tree_view);
  gtk_box_pack_start (GTK_BOX (self->browser_bot),
                      plugin_scroll_window,
                      1, 1, 0);

  return self;
}

static void
plugin_browser_widget_class_init (PluginBrowserWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "plugin_browser.ui");

  gtk_widget_class_set_css_name (klass,
                                 "browser");

  gtk_widget_class_bind_template_child (
    klass,
    PluginBrowserWidget,
    browser_top);
  gtk_widget_class_bind_template_child (
    klass,
    PluginBrowserWidget,
    browser_search);
  gtk_widget_class_bind_template_child (
    klass,
    PluginBrowserWidget,
    collections_exp);
  gtk_widget_class_bind_template_child (
    klass,
    PluginBrowserWidget,
    types_exp);
  gtk_widget_class_bind_template_child (
    klass,
    PluginBrowserWidget,
    cat_exp);
  gtk_widget_class_bind_template_child (
    klass,
    PluginBrowserWidget,
    browser_bot);
  gtk_widget_class_bind_template_child (
    klass,
    PluginBrowserWidget,
    plugin_info);
}

static void
plugin_browser_widget_init (PluginBrowserWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
