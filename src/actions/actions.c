/*
 * actions/actions.c - Zrythm GActions
 *
 * Copyright (C) 2019 Alexandros Theodotou
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/actions.h"
#include "actions/undo_manager.h"
#include "project.h"
#include "gui/widgets/export_dialog.h"
#include "gui/widgets/header_bar.h"
#include "gui/widgets/main_window.h"

#include <gtk/gtk.h>

void
activate_about (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data)
{
  GtkAboutDialog *dialog;

  dialog =
    g_object_new (GTK_TYPE_ABOUT_DIALOG,
                  "copyright", "Copyright 2019 Alexandros Theodotou",
                  "logo-icon-name", "org.gnome.clocks",
                  "website", "https://www.zrythm.org",
                  "version", "0.1",
                  NULL);

  gtk_window_present (GTK_WINDOW (dialog));
}

void
activate_quit (GSimpleAction *action,
               GVariant      *variant,
               gpointer       user_data)
{
  g_application_quit (G_APPLICATION (user_data));
}

void
activate_shortcuts (GSimpleAction *action,
                    GVariant      *variant,
                    gpointer       user_data)
{
}

void
activate_zoom_in (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_zoom_out (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_best_fit (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_original_size (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_new (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_open (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  /*GtkDialog * dialog = dialogs_get_open_project_dialog (GTK_WINDOW (MAIN_WINDOW));*/

  /*int res = gtk_dialog_run (GTK_DIALOG (dialog));*/
  /*if (res == GTK_RESPONSE_ACCEPT)*/
    /*{*/
      /*char *filename;*/
      /*GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);*/
      /*filename = gtk_file_chooser_get_filename (chooser);*/
      /*project_load (filename);*/
      /*g_free (filename);*/
    /*}*/

  /*gtk_widget_destroy (dialog);*/
}

void
activate_save (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
   if (!PROJECT->dir ||
       !PROJECT->title)
     {
       activate_save_as (action,
                         variant,
                         user_data);
       return;
     }
   g_message ("%s project dir ", PROJECT->dir);

   project_save (PROJECT, PROJECT->dir);
}

void
activate_save_as (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  GtkWidget *dialog;
  GtkFileChooser *chooser;
  GtkFileChooserAction _action = GTK_FILE_CHOOSER_ACTION_SAVE;
  gint res;

  dialog = gtk_file_chooser_dialog_new ("Save Project",
                                        GTK_WINDOW (MAIN_WINDOW),
                                        _action,
                                        "_Cancel",
                                        GTK_RESPONSE_CANCEL,
                                        "_Save",
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);
  chooser = GTK_FILE_CHOOSER (dialog);

  gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);

  if (PROJECT->title)
    gtk_file_chooser_set_current_name (chooser,
                                       PROJECT->title);
  else
    gtk_file_chooser_set_filename (chooser,
                                   "Untitled project");

  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
    {
      char *filename;

      filename = gtk_file_chooser_get_filename (chooser);
      project_save (PROJECT, filename);
      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}

void
activate_iconify (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{

}

void
activate_export_as (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  ExportDialogWidget * export = export_dialog_widget_new ();
  gtk_dialog_run (GTK_DIALOG (export));
}

void
activate_properties (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_undo (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_assert (!stack_is_empty (&UNDO_MANAGER->undo_stack));
  undo_manager_undo (UNDO_MANAGER);
  header_bar_widget_refresh_undo_redo_buttons ((HeaderBarWidget *) user_data);
}

void
activate_redo (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_assert (!stack_is_empty (&UNDO_MANAGER->redo_stack));
  undo_manager_redo (UNDO_MANAGER);
  header_bar_widget_refresh_undo_redo_buttons ((HeaderBarWidget *) user_data);
}

void
activate_cut (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_copy (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_paste (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_delete (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_clear_selection (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_select_all (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_toggle_left_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_toggle_right_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_toggle_bot_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_toggle_status_bar (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_fullscreen (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  if (MAIN_WINDOW->is_maximized)
    {
      gtk_window_unmaximize (GTK_WINDOW (MAIN_WINDOW));
      MAIN_WINDOW->is_maximized = 0;
    }
  else
    {
      gtk_window_maximize (GTK_WINDOW (MAIN_WINDOW));
    }
}

void
activate_manual (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
}

void
activate_license (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
}

void
activate_snap_to_grid (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  ZRYTHM->snap_grid_timeline->snap_to_grid =
    !ZRYTHM->snap_grid_timeline->snap_to_grid;
}

void
activate_snap_keep_offset (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  ZRYTHM->snap_grid_timeline->snap_to_grid_keep_offset =
    !ZRYTHM->snap_grid_timeline->snap_to_grid_keep_offset;
}

void
activate_snap_events (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  ZRYTHM->snap_grid_timeline->snap_to_events =
    !ZRYTHM->snap_grid_timeline->snap_to_events;
}
