/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * File browser.
 */

#include "actions/create_tracks_action.h"
#include "audio/encoder.h"
#include "gui/backend/file_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/panel_file_browser.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/tracklist.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include <sndfile.h>
#include <samplerate.h>

G_DEFINE_TYPE (
  PanelFileBrowserWidget,
  panel_file_browser_widget,
  GTK_TYPE_PANED)

enum
{
  COLUMN_ICON,
  COLUMN_NAME,
  COLUMN_DESCR,
  NUM_COLUMNS
};

/**
 * Visible function for file tree model.
 *
 * Used for filtering based on selected category.
 */
static gboolean
visible_func (GtkTreeModel *model,
              GtkTreeIter  *iter,
              gpointer      data)
{
  /*PanelFileBrowserWidget * self = Z_PANEL_FILE_BROWSER_WIDGET (data);*/

  return TRUE;
}

static int
update_file_info_label (PanelFileBrowserWidget * self,
                          gpointer user_data)
{
  char * label = (char *) user_data;

  gtk_label_set_text (self->file_info, label);

  return G_SOURCE_REMOVE;
}

static void
on_selection_changed (GtkTreeSelection * ts,
                      gpointer         user_data)
{
  PanelFileBrowserWidget * self =
    Z_PANEL_FILE_BROWSER_WIDGET (user_data);
  GtkTreeView * tv =
    gtk_tree_selection_get_tree_view (ts);
  GtkTreeModel * model =
    gtk_tree_view_get_model (tv);
  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (ts,
                                          NULL);
  if (selected_rows)
    {
      GtkTreePath * tp =
        (GtkTreePath *) g_list_first (selected_rows)->data;
      GtkTreeIter iter;
      gtk_tree_model_get_iter (model,
                               &iter,
                               tp);
      GValue value = G_VALUE_INIT;

      if (model == self->type_tree_model)
        {
          gtk_tree_model_get_value (
            model, &iter, 0, &value);
          self->selected_type =
            g_value_get_int (&value);
          gtk_tree_model_filter_refilter (
            self->files_tree_model);
        }
      else if (model ==
               GTK_TREE_MODEL (self->files_tree_model))
        {
          gtk_tree_model_get_value (model,
                                    &iter,
                                    COLUMN_DESCR,
                                    &value);
          SupportedFile * descr =
            g_value_get_pointer (&value);
          char * file_type_label =
            supported_file_type_get_description (
              descr->type);

          self->selected_file_descr = descr;

          char * label;
          if (
#ifdef HAVE_FFMPEG
              descr->type == FILE_TYPE_MP3 ||
#endif
              descr->type == FILE_TYPE_FLAC ||
              descr->type == FILE_TYPE_OGG ||
              descr->type == FILE_TYPE_WAV)
            {
              AudioEncoder * enc =
                audio_encoder_new_from_file (
                  descr->abs_path);
              label =
                g_strdup_printf (
                "%s\nFormat: TODO\nSample rate: %d\n"
                "Channels:%d Bitrate: %d\nBit depth: %d",
                descr->label,
                enc->nfo.sample_rate,
                enc->nfo.channels,
                enc->nfo.bit_rate,
                enc->nfo.bit_depth);
              audio_encoder_free (enc);
            }
          else
            label =
              g_strdup_printf (
              "%s\nType: %s",
              descr->label, file_type_label);
          update_file_info_label (self, label);
        }
    }
}

static void
on_drag_data_get (
  GtkWidget        *widget,
  GdkDragContext   *context,
  GtkSelectionData *data,
  guint             info,
  guint             time,
  gpointer          user_data)
{
  GtkTreeSelection * ts =
    (GtkTreeSelection *) user_data;
  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (ts, NULL);
  GtkTreePath * tp =
    (GtkTreePath *)
    g_list_first (selected_rows)->data;
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    GTK_TREE_MODEL (
      MW_PANEL_FILE_BROWSER->files_tree_model),
    &iter, tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (
      MW_PANEL_FILE_BROWSER->files_tree_model),
    &iter,
    COLUMN_DESCR,
    &value);
  SupportedFile * descr =
    g_value_get_pointer (&value);

  gtk_selection_data_set (
    data,
    gdk_atom_intern_static_string (
      TARGET_ENTRY_SUPPORTED_FILE),
    32,
    (const guchar *)&descr,
    sizeof (SupportedFile));
}

static GtkTreeModel *
create_model_for_types ()
{
  GtkListStore *list_store;
  GtkTreeIter iter;
  gint i;

  /* file name, index */
  list_store =
    gtk_list_store_new (
      2, G_TYPE_STRING, G_TYPE_INT);

  for (i = 0; i < NUM_FILE_TYPES; i++)
    {
      ZFileType ft = i;
      char * label =
        supported_file_type_get_description (ft);

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter,
        0, label,
        1, ft,
        -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_locations ()
{
  GtkListStore *list_store;
  /*GtkTreePath *path;*/
  GtkTreeIter iter;
  gint i;

  /* file name, index */
  list_store = gtk_list_store_new (2,
                                   G_TYPE_STRING,
                                   G_TYPE_POINTER);

  for (i = 0; i < FILE_MANAGER->num_locations; i++)
    {
      FileBrowserLocation * loc =
        FILE_MANAGER->locations[i];

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter,
        0, loc->label,
        1, loc,
        -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_files (PanelFileBrowserWidget * self)
{
  GtkListStore *list_store;
  /*GtkTreePath *path;*/
  GtkTreeIter iter;
  gint i;

  /* file name, index */
  list_store =
    gtk_list_store_new (NUM_COLUMNS,
                        G_TYPE_STRING,
                        G_TYPE_STRING,
                        G_TYPE_POINTER);

  for (i = 0; i < FILE_MANAGER->num_files; i++)
    {
      SupportedFile * descr =
        FILE_MANAGER->files[i];

      char icon_name[400];
      switch (descr->type)
        {
        case FILE_TYPE_MIDI:
          strcpy (
            icon_name,
            "audio-midi");
          break;
        case FILE_TYPE_MP3:
          strcpy (
            icon_name,
            "audio-x-mpeg");
          break;
        case FILE_TYPE_FLAC:
          strcpy (
            icon_name,
            "audio-x-flac");
          break;
        case FILE_TYPE_OGG:
          strcpy (
            icon_name,
            "application-ogg");
          break;
        case FILE_TYPE_WAV:
          strcpy (
            icon_name,
            "audio-x-wav");
          break;
        case FILE_TYPE_DIR:
          strcpy (
            icon_name,
            "folder");
          break;
        case FILE_TYPE_PARENT_DIR:
          strcpy (
            icon_name,
            "folder");
          break;
        case FILE_TYPE_OTHER:
          strcpy (
            icon_name,
            "application-x-zerosize");
          break;
        default:
          strcpy (icon_name, "");
          break;
        }

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter,
        COLUMN_ICON, icon_name,
        COLUMN_NAME, descr->label,
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
on_row_activated (GtkTreeView       *tree_view,
               GtkTreePath       *tp,
               GtkTreeViewColumn *column,
               gpointer           user_data)
{
  PanelFileBrowserWidget * self =
    MW_PANEL_FILE_BROWSER;
  GtkTreeModel * model =
    GTK_TREE_MODEL (self->files_tree_model);
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
  SupportedFile * descr =
    g_value_get_pointer (&value);

  /*g_message ("activated file type %d, abs path %s",*/
             /*descr->type,*/
             /*descr->abs_path);*/
  if (descr->type == FILE_TYPE_DIR ||
      descr->type == FILE_TYPE_PARENT_DIR)
    {
      /* FIXME free unnecessary stuff */
      FileBrowserLocation * loc =
        malloc (sizeof (FileBrowserLocation));
      loc->path = descr->abs_path;
      loc->label = g_path_get_basename (loc->path);
      file_manager_set_selection (
        FILE_MANAGER, loc,
        FB_SELECTION_TYPE_LOCATIONS, true);
      self->files_tree_model =
        GTK_TREE_MODEL_FILTER (
          create_model_for_files (self));
      gtk_tree_view_set_model (
        self->files_tree_view,
        GTK_TREE_MODEL (self->files_tree_model));
    }
  else if (descr->type == FILE_TYPE_WAV ||
           descr->type == FILE_TYPE_OGG ||
           descr->type == FILE_TYPE_FLAC ||
           descr->type == FILE_TYPE_MP3)
    {
      UndoableAction * action =
        create_tracks_action_new (
          TRACK_TYPE_AUDIO, NULL, descr,
          TRACKLIST->num_tracks, PLAYHEAD, 1);
      undo_manager_perform (UNDO_MANAGER, action);
    }
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
tree_view_create (PanelFileBrowserWidget * self,
                  GtkTreeModel * model,
                  int          allow_multi,
                  int          dnd)
{
  /* instantiate tree view using model */
  GtkWidget * tree_view =
    gtk_tree_view_new_with_model (
      GTK_TREE_MODEL (model));

  /* init tree view */
  GtkCellRenderer * renderer;
  GtkTreeViewColumn * column;
  if (GTK_TREE_MODEL (self->files_tree_model) ==
        model)
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

      /* set search column */
      gtk_tree_view_set_search_column (
        GTK_TREE_VIEW (tree_view),
        COLUMN_NAME);
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
        gtk_tree_view_get_selection (
          GTK_TREE_VIEW (tree_view)),
        GTK_SELECTION_MULTIPLE);

  if (dnd)
    {
      char * entry_uri_list =
        g_strdup (TARGET_ENTRY_SUPPORTED_FILE);
      GtkTargetEntry entries[] = {
        {
          entry_uri_list, GTK_TARGET_SAME_APP,
          symap_map (ZSYMAP, TARGET_ENTRY_SUPPORTED_FILE),
        },
      };
      gtk_tree_view_enable_model_drag_source (
        GTK_TREE_VIEW (tree_view),
        GDK_BUTTON1_MASK, entries,
        G_N_ELEMENTS (entries),
        GDK_ACTION_COPY);
      g_free (entry_uri_list);

      g_signal_connect (
        GTK_WIDGET (tree_view), "drag-data-get",
        G_CALLBACK (on_drag_data_get),
        gtk_tree_view_get_selection (
          GTK_TREE_VIEW (tree_view)));
    }

  GtkTreeSelection * sel =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (tree_view));
  g_signal_connect (
    G_OBJECT (sel), "changed",
    G_CALLBACK (on_selection_changed), self);

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

static int
on_realize (
  GtkWidget * widget,
  PanelFileBrowserWidget * self)
{
  /*g_message ("file browser realized");*/

  /* set divider position */
  int divider_pos =
    g_settings_get_int (
      S_UI,
      "browser-divider-position");
  gtk_paned_set_position (
    GTK_PANED (self), divider_pos);
  /*self->first_time_position_set = 1;*/

  return FALSE;
}


/**
 * Creates a GtkTreeView using the given model,
 * and puts it in the given expander in a GtkScrolledWindow
 *
 * @return the scroll window
 * TODO FIXME
 */
static GtkScrolledWindow *
create_tree_view_add_to_expander (PanelFileBrowserWidget * self,
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

PanelFileBrowserWidget *
panel_file_browser_widget_new ()
{
  PanelFileBrowserWidget * self = g_object_new (PANEL_FILE_BROWSER_WIDGET_TYPE, NULL);

  g_message ("Instantiating panel_file_browser widget...");

  gtk_label_set_xalign (self->file_info, 0);

  /* create each tree */
  create_tree_view_add_to_expander (
    self,
    create_model_for_types (),
    GTK_EXPANDER (self->collections_exp));
  create_tree_view_add_to_expander (
    self,
    create_model_for_types (),
    GTK_EXPANDER (self->types_exp));
  self->locations_tree_model =
    create_model_for_locations ();
  GtkScrolledWindow * scrolled_window =
    create_tree_view_add_to_expander (
      self,
      self->locations_tree_model,
      GTK_EXPANDER (self->locations_exp));

  /* expand locations by default */
  gtk_expander_set_expanded (
    GTK_EXPANDER (self->locations_exp),
    TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (scrolled_window),
                          TRUE);

  /* populate files */
  self->files_tree_model =
    GTK_TREE_MODEL_FILTER (create_model_for_files (self));
  self->files_tree_view =
    GTK_TREE_VIEW (tree_view_create (
      self,
      GTK_TREE_MODEL (
        self->files_tree_model),
      0,
      1));
  g_signal_connect (G_OBJECT (self->files_tree_view),
                    "row-activated",
                    G_CALLBACK (on_row_activated),
                    NULL);
  GtkWidget * file_scroll_window =
    add_scroll_window (self->files_tree_view);
  gtk_box_pack_start (GTK_BOX (self->browser_bot),
                      file_scroll_window,
                      1, 1, 0);
  gtk_widget_show_all (file_scroll_window);

  /* set divider position */
  /*int divider_pos =*/
    /*g_settings_get_int (*/
      /*S_UI,*/
      /*"browser-divider-position");*/
  gtk_paned_set_position (GTK_PANED (self),
                          40);

  g_signal_connect (
    G_OBJECT (self), "realize",
    G_CALLBACK (on_realize), self);

  return self;
}

static void
panel_file_browser_widget_class_init (PanelFileBrowserWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "panel_file_browser.ui");

  gtk_widget_class_set_css_name (
    klass, "browser");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, PanelFileBrowserWidget, x)

  BIND_CHILD (browser_top);
  BIND_CHILD (browser_search);
  BIND_CHILD (collections_exp);
  BIND_CHILD (types_exp);
  BIND_CHILD (locations_exp);
  BIND_CHILD (browser_bot);
  BIND_CHILD (file_info);

#undef BIND_CHILD
}

static void
panel_file_browser_widget_init (PanelFileBrowserWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
