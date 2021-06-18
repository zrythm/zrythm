/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/tracklist_selections.h"
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
#include "gui/widgets/volume.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
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

static GtkTreeModel *
create_model_for_locations ()
{
  GtkListStore *list_store;
  /*GtkTreePath *path;*/
  GtkTreeIter iter;

  /* icon, file name, index */
  list_store =
    gtk_list_store_new (
      3, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_POINTER);

  for (guint i = 0;
       i < FILE_MANAGER->locations->len; i++)
    {
      FileBrowserLocation * loc =
        g_ptr_array_index (
          FILE_MANAGER->locations, i);

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter,
        0, "folder",
        1, loc->label,
        2, loc,
        -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static void
on_bookmark_remove_activate (
  GtkMenuItem *            menuitem,
  PanelFileBrowserWidget * self)
{
  g_return_if_fail (self->cur_loc);

  if (self->cur_loc->standard)
    {
      ui_show_error_message (
        MAIN_WINDOW,
        _("Cannot delete standard bookmark"));
      return;
    }

  GtkWidget * dialog =
    gtk_message_dialog_new (
      GTK_WINDOW (MAIN_WINDOW),
      GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_QUESTION,
      GTK_BUTTONS_YES_NO,
      "%s",
      _("Are you sure you want to remove this "
      "bookmark?"));
  gtk_widget_show_all (GTK_WIDGET (dialog));
  int result =
    gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  if (result == GTK_RESPONSE_YES)
    {
      file_manager_remove_location_and_save (
        FILE_MANAGER, self->cur_loc->path,
        true);

      /* refresh treeview */
      self->bookmarks_tree_model =
        GTK_TREE_MODEL (
          create_model_for_locations (self));
      gtk_tree_view_set_model (
        self->bookmarks_tree_view,
        GTK_TREE_MODEL (
          self->bookmarks_tree_model));
    }
}

static void
show_bookmarks_context_menu (
  PanelFileBrowserWidget *    self,
  const FileBrowserLocation * loc)
{
  GtkMenuItem * menuitem;
  GtkWidget * menu = gtk_menu_new();

#define APPEND \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL (menu), \
    GTK_WIDGET (menuitem));

  self->cur_loc = loc;

  menuitem =
    z_gtk_create_menu_item (
      _("Delete"), "edit-delete", false, NULL);
  gtk_widget_set_visible (
    GTK_WIDGET (menuitem), true);
  APPEND;
  g_signal_connect (
    G_OBJECT (menuitem), "activate",
    G_CALLBACK (on_bookmark_remove_activate),
    self);

#undef APPEND

  gtk_menu_attach_to_widget (
    GTK_MENU (menu),
    GTK_WIDGET (self), NULL);
  gtk_menu_popup_at_pointer (
    GTK_MENU (menu), NULL);
}

static void
on_bookmark_right_click (
  GtkGestureMultiPress *   gesture,
  gint                     n_press,
  gdouble                  x_dbl,
  gdouble                  y_dbl,
  PanelFileBrowserWidget * self)
{
  if (n_press != 1)
    return;

  GtkTreePath *path;
  GtkTreeViewColumn *column;

  int x, y;
  gtk_tree_view_convert_widget_to_bin_window_coords (
    GTK_TREE_VIEW (self->bookmarks_tree_view),
    (int) x_dbl, (int) y_dbl, &x, &y);

  GtkTreeSelection * selection =
    gtk_tree_view_get_selection (
      (self->bookmarks_tree_view));
  if (!gtk_tree_view_get_path_at_pos (
        GTK_TREE_VIEW (self->bookmarks_tree_view),
        x, y,
        &path, &column, NULL, NULL))
    {
      g_message ("no path at position %d %d", x, y);
      // if we can't find path at pos, we surely don't
      // want to pop up the menu
      return;
    }

  gtk_tree_selection_unselect_all (selection);
  gtk_tree_selection_select_path (selection, path);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    GTK_TREE_MODEL (self->bookmarks_tree_model),
    &iter, path);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (self->bookmarks_tree_model),
    &iter, COLUMN_DESCR, &value);
  gtk_tree_path_free (path);

  FileBrowserLocation * loc =
    g_value_get_pointer (&value);

  show_bookmarks_context_menu (self, loc);
}

static void
on_dir_add_bookmark (
  GtkMenuItem *            menuitem,
  PanelFileBrowserWidget * self)
{
  g_return_if_fail (self->cur_file);

  file_manager_add_location_and_save (
    FILE_MANAGER, self->cur_file->abs_path);

  /* refresh treeview */
  self->bookmarks_tree_model =
    GTK_TREE_MODEL (
      create_model_for_locations (self));
  gtk_tree_view_set_model (
    self->bookmarks_tree_view,
    GTK_TREE_MODEL (self->bookmarks_tree_model));
}

static void
show_files_context_menu (
  PanelFileBrowserWidget * self,
  const SupportedFile *    file)
{
  GtkMenuItem * menuitem;
  GtkWidget * menu = gtk_menu_new();

#define APPEND \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL (menu), \
    GTK_WIDGET (menuitem));

  self->cur_file = file;

  if (file->type == FILE_TYPE_DIR)
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Add Bookmark"), "favorite", false,
          NULL);
      gtk_widget_set_visible (
        GTK_WIDGET (menuitem), true);
      APPEND;
      g_signal_connect (
        G_OBJECT (menuitem), "activate",
        G_CALLBACK (on_dir_add_bookmark), self);
    }

#undef APPEND

  gtk_menu_attach_to_widget (
    GTK_MENU (menu),
    GTK_WIDGET (self), NULL);
  gtk_menu_popup_at_pointer (
    GTK_MENU (menu), NULL);
}

static void
on_file_right_click (
  GtkGestureMultiPress *   gesture,
  gint                     n_press,
  gdouble                  x_dbl,
  gdouble                  y_dbl,
  PanelFileBrowserWidget * self)
{
  if (n_press != 1)
    return;

  GtkTreePath *path;
  GtkTreeViewColumn *column;

  int x, y;
  gtk_tree_view_convert_widget_to_bin_window_coords (
    GTK_TREE_VIEW (self->files_tree_view),
    (int) x_dbl, (int) y_dbl, &x, &y);

  GtkTreeSelection * selection =
    gtk_tree_view_get_selection (
      (self->files_tree_view));
  if (!gtk_tree_view_get_path_at_pos (
        GTK_TREE_VIEW (self->files_tree_view),
        x, y,
        &path, &column, NULL, NULL))
    {
      g_message ("no path at position %d %d", x, y);
      // if we can't find path at pos, we surely don't
      // want to pop up the menu
      return;
    }

  gtk_tree_selection_unselect_all (selection);
  gtk_tree_selection_select_path (selection, path);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    GTK_TREE_MODEL (self->files_tree_model),
    &iter, path);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (self->files_tree_model),
    &iter, COLUMN_DESCR, &value);
  gtk_tree_path_free (path);

  SupportedFile * descr =
    g_value_get_pointer (&value);

  show_files_context_menu (self, descr);
}

/**
 * Visible function for file tree model.
 */
static gboolean
visible_func (
  GtkTreeModel *           model,
  GtkTreeIter *            iter,
  PanelFileBrowserWidget * self)
{
  SupportedFile * descr;
  gtk_tree_model_get (
    model, iter, COLUMN_DESCR, &descr, -1);

  bool show_audio =
    gtk_toggle_tool_button_get_active (
      self->toggle_audio);
  bool show_midi =
    gtk_toggle_tool_button_get_active (
      self->toggle_midi);
  bool show_presets =
    gtk_toggle_tool_button_get_active (
      self->toggle_presets);
  bool all_toggles_off =
    !show_audio && !show_midi && !show_presets;

  bool visible = false;
  switch (descr->type)
    {
    case FILE_TYPE_MIDI:
      visible = show_midi || all_toggles_off;
      break;
    case FILE_TYPE_DIR:
    case FILE_TYPE_PARENT_DIR:
      return true;
    case FILE_TYPE_MP3:
    case FILE_TYPE_FLAC:
    case FILE_TYPE_OGG:
    case FILE_TYPE_WAV:
      visible = show_audio || all_toggles_off;
      break;
    case FILE_TYPE_OTHER:
      visible =
        all_toggles_off &&
        g_settings_get_boolean (
          S_UI_FILE_BROWSER,
          "show-unsupported-files");
      break;
    default:
      break;
    }

  if (!visible)
    return false;

  if (!g_settings_get_boolean (
         S_UI_FILE_BROWSER, "show-hidden-files") &&
      descr->hidden)
    return false;

  return visible;
}

static int
update_file_info_label (
  PanelFileBrowserWidget * self,
  gpointer user_data)
{
  char * label = (char *) user_data;

  gtk_label_set_text (self->file_info, label);

  return G_SOURCE_REMOVE;
}

static void
on_selection_changed (
  GtkTreeSelection *       ts,
  PanelFileBrowserWidget * self)
{
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
        (GtkTreePath *)
        g_list_first (selected_rows)->data;
      GtkTreeIter iter;
      gtk_tree_model_get_iter (
        model, &iter, tp);
      GValue value = G_VALUE_INIT;

      if (model ==
            GTK_TREE_MODEL (self->files_tree_model))
        {
          gtk_tree_model_get_value (
            model, &iter, COLUMN_DESCR, &value);
          SupportedFile * descr =
            g_value_get_pointer (&value);
          char * file_type_label =
            supported_file_type_get_description (
              descr->type);

          g_ptr_array_remove_range (
            self->selected_files, 0,
            self->selected_files->len);
          g_ptr_array_add (
            self->selected_files, descr);

          char * label;
          if (supported_file_type_is_audio (
                descr->type))
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

          if (g_settings_get_boolean (
                S_UI_FILE_BROWSER, "autoplay") &&
              (supported_file_type_is_audio (
                 descr->type) ||
               supported_file_type_is_midi (
                 descr->type)))
            {
              sample_processor_queue_file (
                SAMPLE_PROCESSOR, descr);
            }
        }
    }
}

static SupportedFile *
get_selected_file (
  PanelFileBrowserWidget * self)
{
  GtkTreeSelection * ts =
    gtk_tree_view_get_selection (
      self->files_tree_view);
  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (
      ts, NULL);
  if (!selected_rows)
    return  NULL;

  GtkTreePath * tp =
    (GtkTreePath *)
    g_list_first (selected_rows)->data;
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    GTK_TREE_MODEL (self->files_tree_model),
    &iter, tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (self->files_tree_model),
    &iter, COLUMN_DESCR, &value);
  SupportedFile * descr =
    g_value_get_pointer (&value);

  g_list_free_full (
    selected_rows,
    (GDestroyNotify) gtk_tree_path_free);

  return descr;
}

static void
on_play_clicked (
  GtkToolButton *          toolbutton,
  PanelFileBrowserWidget * self)
{
  SupportedFile * descr = get_selected_file (self);
  if (!descr)
    return;

  sample_processor_queue_file (
    SAMPLE_PROCESSOR, descr);
}

static void
on_stop_clicked (
  GtkToolButton *          toolbutton,
  PanelFileBrowserWidget * self)
{
  sample_processor_stop_file_playback (
    SAMPLE_PROCESSOR);
}

static void
on_drag_data_get (
  GtkWidget *              widget,
  GdkDragContext *         context,
  GtkSelectionData *       data,
  guint                    info,
  guint                    time,
  PanelFileBrowserWidget * self)
{
  if (GTK_WIDGET (self->files_tree_view) ==
        widget)
    {
      SupportedFile * descr =
        get_selected_file (self);

      gtk_selection_data_set (
        data,
        gdk_atom_intern_static_string (
          TARGET_ENTRY_SUPPORTED_FILE),
        32,
        (const guchar *)&descr,
        sizeof (SupportedFile));
    }
}

static GtkTreeModel *
create_model_for_files (
  PanelFileBrowserWidget * self)
{
  GtkListStore *list_store;
  GtkTreeIter iter;
  gint i;

  /* file name, index */
  list_store =
    gtk_list_store_new (
      NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING,
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
    (GtkTreeModelFilterVisibleFunc) visible_func,
    self, NULL);

  return model;
}

static void
on_bookmark_row_activated (
  GtkTreeView *            tree_view,
  GtkTreePath *            tp,
  GtkTreeViewColumn *      column,
  PanelFileBrowserWidget * self)
{
  GtkTreeModel * model =
    GTK_TREE_MODEL (
      gtk_tree_view_get_model (tree_view));
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    model, &iter, tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    model, &iter, 2, &value);
  FileBrowserLocation * loc =
    g_value_get_pointer (&value);

  file_manager_set_selection (
    FILE_MANAGER, loc, true, true);
  self->files_tree_model =
    GTK_TREE_MODEL_FILTER (
      create_model_for_files (self));
  gtk_tree_view_set_model (
    self->files_tree_view,
    GTK_TREE_MODEL (self->files_tree_model));
}

static void
on_file_row_activated (
  GtkTreeView *            tree_view,
  GtkTreePath *            tp,
  GtkTreeViewColumn *      column,
  PanelFileBrowserWidget * self)
{
  GtkTreeModel * model =
    GTK_TREE_MODEL (self->files_tree_model);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    model, &iter, tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    model, &iter, COLUMN_DESCR, &value);
  SupportedFile * descr =
    g_value_get_pointer (&value);

  if (descr->type == FILE_TYPE_DIR ||
      descr->type == FILE_TYPE_PARENT_DIR)
    {
      /* FIXME free unnecessary stuff */
      FileBrowserLocation * loc =
        object_new (FileBrowserLocation);
      loc->path = descr->abs_path;
      loc->label = g_path_get_basename (loc->path);
      file_manager_set_selection (
        FILE_MANAGER, loc, true, true);
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
        tracklist_selections_action_new_create (
          TRACK_TYPE_AUDIO, NULL, descr,
          TRACKLIST->num_tracks, PLAYHEAD, 1, -1);
      undo_manager_perform (UNDO_MANAGER, action);
    }
}

static void
toggles_changed (
  GtkToggleToolButton *    btn,
  PanelFileBrowserWidget * self)
{
  if (gtk_toggle_tool_button_get_active (btn))
    {
      /* block signals, unset all, unblock */
      g_signal_handlers_block_by_func (
        self->toggle_audio,
        toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_midi,
        toggles_changed, self);
      g_signal_handlers_block_by_func (
        self->toggle_presets,
        toggles_changed, self);

      if (btn == self->toggle_audio)
        {
          S_SET_ENUM (
            S_UI_FILE_BROWSER, "filter",
            FILE_BROWSER_FILTER_AUDIO);
          gtk_toggle_tool_button_set_active (
            self->toggle_midi, 0);
          gtk_toggle_tool_button_set_active (
            self->toggle_presets, 0);
        }
      else if (btn == self->toggle_midi)
        {
          S_SET_ENUM (
            S_UI_FILE_BROWSER, "filter",
            FILE_BROWSER_FILTER_MIDI);
          gtk_toggle_tool_button_set_active (
            self->toggle_audio, 0);
          gtk_toggle_tool_button_set_active (
            self->toggle_presets, 0);
        }
      else if (btn == self->toggle_presets)
        {
          S_SET_ENUM (
            S_UI_FILE_BROWSER, "filter",
            FILE_BROWSER_FILTER_PRESET);
          gtk_toggle_tool_button_set_active (
            self->toggle_midi, 0);
          gtk_toggle_tool_button_set_active (
            self->toggle_audio, 0);
        }

      g_signal_handlers_unblock_by_func (
        self->toggle_audio,
        toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_midi,
        toggles_changed, self);
      g_signal_handlers_unblock_by_func (
        self->toggle_presets,
        toggles_changed, self);
    }
  else
    {
      S_SET_ENUM (
        S_UI_FILE_BROWSER, "filter",
        FILE_BROWSER_FILTER_NONE);
    }
  gtk_tree_model_filter_refilter (
    self->files_tree_model);
}

static void
tree_view_setup (
  PanelFileBrowserWidget * self,
  GtkTreeView *            tree_view,
  GtkTreeModel *           model,
  bool                     allow_multi,
  bool                     dnd)
{
  /* instantiate tree view using model */
  gtk_tree_view_set_model (
    tree_view, GTK_TREE_MODEL (model));

  /* init tree view */
  GtkCellRenderer * renderer;
  GtkTreeViewColumn * column;
  if (GTK_TREE_MODEL (self->files_tree_model) ==
        model ||
      GTK_TREE_MODEL (self->bookmarks_tree_model) ==
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
          "text", 0, NULL);
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
        G_CALLBACK (on_drag_data_get), self);
    }

  GtkTreeSelection * sel =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (tree_view));
  g_signal_connect (
    G_OBJECT (sel), "changed",
    G_CALLBACK (on_selection_changed), self);
}

static void
on_position_change (
  GtkStack * stack,
  GParamSpec * pspec,
  PanelFileBrowserWidget * self)
{
  if (self->first_draw)
    {
      return;
    }

  int divider_pos =
    gtk_paned_get_position (GTK_PANED (self));
  g_settings_set_int (
    S_UI, "browser-divider-position", divider_pos);
  g_message (
    "set browser divider position to %d",
    divider_pos);
  /*g_warning ("pos %d", divider_pos);*/
}

static gboolean
on_draw (
  GtkWidget    *widget,
  cairo_t *cr,
  PanelFileBrowserWidget * self)
{
  if (self->first_draw)
    {
      self->first_draw = false;

      /* set divider position */
      int divider_pos =
        g_settings_get_int (
          S_UI,
          "browser-divider-position");
      /*g_warning ("pos %d", divider_pos);*/
      gtk_paned_set_position (
        GTK_PANED (self), divider_pos);
    }

  return FALSE;
}

static void
on_settings_menu_items_changed (
  GMenuModel *             model,
  int                      position,
  int                      removed,
  int                      added,
  PanelFileBrowserWidget * self)
{
  gtk_tree_model_filter_refilter (
    self->files_tree_model);
}

static void
on_instrument_changed (
  GtkComboBox *            cb,
  PanelFileBrowserWidget * self)
{
  const char * active_id =
    gtk_combo_box_get_active_id (cb);

  g_message ("changed: %s", active_id);

  PluginDescriptor * descr =
    (PluginDescriptor *)
    yaml_deserialize (
      active_id, &plugin_descriptor_schema);

  if (SAMPLE_PROCESSOR->instrument_setting &&
      plugin_descriptor_is_same_plugin (
         SAMPLE_PROCESSOR->instrument_setting->
           descr,
         descr))
    return;

  EngineState state;
  engine_wait_for_pause (
    AUDIO_ENGINE, &state, false);

  /* clear previous instrument setting */
  if (SAMPLE_PROCESSOR->instrument_setting)
    {
      object_free_w_func_and_null (
        plugin_setting_free,
        SAMPLE_PROCESSOR->instrument_setting);
    }

  /* set setting */
  PluginSetting * existing_setting =
    plugin_settings_find (
      S_PLUGIN_SETTINGS, descr);
  if (existing_setting)
    {
      SAMPLE_PROCESSOR->instrument_setting =
        plugin_setting_clone (
          existing_setting, F_VALIDATE);
    }
  else
    {
      SAMPLE_PROCESSOR->instrument_setting =
        plugin_setting_new_default (descr);
    }
  g_return_if_fail (
    SAMPLE_PROCESSOR->instrument_setting);

  /* save setting */
  char * setting_yaml =
    yaml_serialize (
      SAMPLE_PROCESSOR->instrument_setting,
      &plugin_setting_schema);
  g_settings_set_string (
    S_UI_FILE_BROWSER, "instrument", setting_yaml);
  g_free (setting_yaml);

  plugin_descriptor_free (descr);

  engine_resume (AUDIO_ENGINE, &state);
}

static void
setup_instrument_cb (
  PanelFileBrowserWidget * self)
{
  /* populate instruments */
  for (size_t i = 0;
       i < PLUGIN_MANAGER->plugin_descriptors->
         len;
       i++)
    {
      PluginDescriptor * descr =
        g_ptr_array_index (
          PLUGIN_MANAGER->plugin_descriptors, i);
      if (plugin_descriptor_is_instrument (descr))
        {
          char * id =
            yaml_serialize (
              descr, &plugin_descriptor_schema);
          gtk_combo_box_text_append (
            self->instrument_cb, id,
            descr->name);
          g_free (id);
        }
    }

  /* set selected instrument */
  if (SAMPLE_PROCESSOR->instrument_setting)
    {
      char * id =
        yaml_serialize (
          SAMPLE_PROCESSOR->instrument_setting->
            descr,
          &plugin_descriptor_schema);
      gtk_combo_box_set_active_id (
        GTK_COMBO_BOX (self->instrument_cb), id);
    }

  /* add instrument signal handler */
  g_signal_connect (
    G_OBJECT (self->instrument_cb), "changed",
    G_CALLBACK (on_instrument_changed), self);
}

PanelFileBrowserWidget *
panel_file_browser_widget_new ()
{
  PanelFileBrowserWidget * self =
    g_object_new (
      PANEL_FILE_BROWSER_WIDGET_TYPE, NULL);

  g_message (
    "Instantiating panel_file_browser widget...");

  self->first_draw = true;

  gtk_label_set_xalign (self->file_info, 0);

  volume_widget_setup (
    self->volume, SAMPLE_PROCESSOR->fader->amp);

  setup_instrument_cb (self);

  /* populate bookmarks */
  self->bookmarks_tree_model =
    GTK_TREE_MODEL (
      create_model_for_locations (self));
  tree_view_setup (
    self, self->bookmarks_tree_view,
    GTK_TREE_MODEL (self->bookmarks_tree_model),
    false, true);
  g_signal_connect (
    self->bookmarks_tree_view, "row-activated",
    G_CALLBACK (on_bookmark_row_activated), self);

  /* populate files */
  self->files_tree_model =
    GTK_TREE_MODEL_FILTER (
      create_model_for_files (self));
  tree_view_setup (
    self, self->files_tree_view,
    GTK_TREE_MODEL (self->files_tree_model),
    false, true);
  g_signal_connect (
    self->files_tree_view, "row-activated",
    G_CALLBACK (on_file_row_activated), self);

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (on_draw), self);
  g_signal_connect (
    G_OBJECT (self), "notify::position",
    G_CALLBACK (on_position_change), self);

  /* connect right click handlers */
  GtkGestureMultiPress * mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self->bookmarks_tree_view)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_bookmark_right_click), self);

  mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self->files_tree_view)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_file_right_click), self);

  return self;
}

static void
panel_file_browser_widget_class_init (
  PanelFileBrowserWidgetClass * _klass)
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
  BIND_CHILD (browser_bot);
  BIND_CHILD (file_info);
  BIND_CHILD (files_tree_view);
  BIND_CHILD (bookmarks_tree_view);
  BIND_CHILD (toggle_audio);
  BIND_CHILD (toggle_midi);
  BIND_CHILD (toggle_presets);
  BIND_CHILD (volume);
  BIND_CHILD (play_btn);
  BIND_CHILD (stop_btn);
  BIND_CHILD (file_settings_btn);
  BIND_CHILD (instrument_cb);

#undef BIND_CHILD

#define BIND_SIGNAL(sig) \
  gtk_widget_class_bind_template_callback ( \
    klass, sig)

  BIND_SIGNAL (toggles_changed);
  BIND_SIGNAL (on_play_clicked);
  BIND_SIGNAL (on_stop_clicked);

#undef BIND_SIGNAL
}

static void
panel_file_browser_widget_init (
  PanelFileBrowserWidget * self)
{
  g_type_ensure (VOLUME_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->selected_files = g_ptr_array_new ();

  /* set menu */
  GSimpleActionGroup * action_group =
    g_simple_action_group_new ();
  GAction * action =
    g_settings_create_action (
      S_UI_FILE_BROWSER, "autoplay");
  g_action_map_add_action (
    G_ACTION_MAP (action_group), action);
  action =
    g_settings_create_action (
      S_UI_FILE_BROWSER, "show-unsupported-files");
  g_action_map_add_action (
    G_ACTION_MAP (action_group), action);
  action =
    g_settings_create_action (
      S_UI_FILE_BROWSER, "show-hidden-files");
  g_action_map_add_action (
    G_ACTION_MAP (action_group), action);
  gtk_widget_insert_action_group (
    GTK_WIDGET (self->file_settings_btn),
    "settings-btn",
    G_ACTION_GROUP (action_group));

  GMenu * menu = g_menu_new ();
  g_menu_append (
    menu, _("Autoplay"), "settings-btn.autoplay");
  g_menu_append (
    menu, _("Show unsupported files"),
    "settings-btn.show-unsupported-files");
  g_menu_append (
    menu, _("Show hidden files"),
    "settings-btn.show-hidden-files");
  gtk_menu_button_set_menu_model (
    self->file_settings_btn,
    G_MENU_MODEL (menu));

  g_signal_connect (
    menu, "items-changed",
    G_CALLBACK (on_settings_menu_items_changed),
    self);
}
