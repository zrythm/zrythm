/*
 * gui/widgets/header_bar.c - Main window widget
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

#include "project.h"
#include "gui/widgets/export_dialog.h"
#include "gui/widgets/header_bar.h"
#include "gui/widgets/main_window.h"
#include "undo/undo_manager.h"
#include "utils/resources.h"

G_DEFINE_TYPE (HeaderBarWidget,
               header_bar_widget,
               GTK_TYPE_HEADER_BAR)

void
header_bar_widget_refresh_undo_redo_buttons (HeaderBarWidget * self)
{
  g_assert (UNDO_MANAGER);

  gtk_widget_set_sensitive (
    GTK_WIDGET (self->edit_undo),
    !stack_is_empty (&UNDO_MANAGER->undo_stack));
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->edit_redo),
    !stack_is_empty (&UNDO_MANAGER->redo_stack));
}

static GtkMenuItem *
create_menu_item (GtkAccelGroup * accel_group,
                  gchar *         label_name,
                  gchar *         icon_name,
                  IconType        resource_icon_type,
                  gchar *         resource,
                  int             is_toggle,
                  guint           accel_key,
                  GdkModifierType accel_mods)
{
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *icon;
  if (icon_name)
    icon =
      gtk_image_new_from_icon_name (icon_name,
                                    GTK_ICON_SIZE_MENU);
  else if (resource)
    icon =
      resources_get_icon (resource_icon_type,
                          resource);
  GtkWidget *label = gtk_accel_label_new (label_name);
  GtkWidget * menu_item;
  if (is_toggle)
    menu_item = gtk_check_menu_item_new ();
  else
    menu_item = gtk_menu_item_new ();

  gtk_container_add (GTK_CONTAINER (box), icon);

  gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);

  if (accel_key)
    {
      gtk_widget_add_accelerator (menu_item,
                                  "activate",
                                  accel_group,
                                  accel_key,
                                  accel_mods,
                                  GTK_ACCEL_VISIBLE);
      gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), menu_item);
    }

  gtk_box_pack_end (GTK_BOX (box), label, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (menu_item), box);

  gtk_widget_show_all (menu_item);

  return GTK_MENU_ITEM (menu_item);
}

/**
 * close button event
 */
static void
close_clicked (HeaderBarWidget * self,
               gpointer user_data)
{
  main_window_widget_quit (MAIN_WINDOW);
}

/**
 * minimize button event
 */
static void
minimize_clicked (HeaderBarWidget * self,
                      gpointer user_data)
{
  main_window_widget_minimize (MAIN_WINDOW);
}

/**
 * maximize button event
 */
static void
maximize_clicked (HeaderBarWidget * self,
                      gpointer user_data)
{
  main_window_widget_toggle_maximize (MAIN_WINDOW);
}

static void
on_file_open_activate (GtkMenuItem * menu_item,
                       gpointer      user_data)
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

static void
on_file_export_activate(GtkMenuItem   * menu_item,
           gpointer      user_data)
{
  ExportDialogWidget * export = export_dialog_widget_new ();
  gtk_dialog_run (GTK_DIALOG (export));
}

static void
on_file_save_as_activate (GtkMenuItem   * menu_item,
            gpointer      user_data)
{
  GtkWidget *dialog;
  GtkFileChooser *chooser;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
  gint res;

  dialog = gtk_file_chooser_dialog_new ("Save Project",
                                        GTK_WINDOW (MAIN_WINDOW),
                                        action,
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

static void
on_file_save_activate (GtkMenuItem   * menu_item,
         gpointer      user_data)
{
   if (!PROJECT->dir ||
       !PROJECT->title)
     {
       on_file_save_as_activate (menu_item, user_data);
       return;
     }
   g_message ("%s project dir ", PROJECT->dir);

   project_save (PROJECT, PROJECT->dir);
}

static void
new_activate (GtkMenuItem * menu_item,
              gpointer      user_data)
{
  g_message ("new activated");
}

static void
undo_activate (GtkMenuItem * menu_item,
              gpointer      user_data)
{
  g_assert (!stack_is_empty (&UNDO_MANAGER->undo_stack));
  undo_manager_undo (UNDO_MANAGER);
  header_bar_widget_refresh_undo_redo_buttons ((HeaderBarWidget *) user_data);
}

static void
redo_activate (GtkMenuItem * menu_item,
              gpointer      user_data)
{
  g_assert (!stack_is_empty (&UNDO_MANAGER->redo_stack));
  undo_manager_redo (UNDO_MANAGER);
  header_bar_widget_refresh_undo_redo_buttons ((HeaderBarWidget *) user_data);
}

static void
cut_activate (GtkMenuItem * menu_item,
              gpointer      user_data)
{
  g_message ("CUT");
}

static void
copy_activate (GtkMenuItem * menu_item,
               gpointer      user_data)
{
  g_message ("new activated");
}

static void
paste_activate (GtkMenuItem * menu_item,
                gpointer      user_data)
{
  g_message ("new activated");
}

static void
delete_activate (GtkMenuItem * menu_item,
                 gpointer      user_data)
{
  g_message ("new activated");
}

void
header_bar_widget_setup (HeaderBarWidget * self,
                             MainWindowWidget * mw,
                             const char * title)
{
  gtk_header_bar_set_title (GTK_HEADER_BAR (self),
                            "Zrythm");
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (self),
                               title);

  /* setup menu items */
#define APPEND_TO_MENU_SHELL(shell) \
  gtk_menu_shell_append (GTK_MENU_SHELL (shell), \
                         GTK_WIDGET (menu_item))
#define APPEND_TO_FILE_MENU \
  APPEND_TO_MENU_SHELL (self->file_menu)
#define APPEND_TO_EDIT_MENU \
  APPEND_TO_MENU_SHELL (self->edit_menu)
#define APPEND_TO_VIEW_MENU \
  APPEND_TO_MENU_SHELL (self->view_menu)
#define APPEND_TO_HELP_MENU \
  APPEND_TO_MENU_SHELL (self->help_menu)
#define CREATE_SEPARATOR \
  menu_item = \
    GTK_MENU_ITEM (gtk_separator_menu_item_new ()); \
  gtk_widget_show_all (GTK_WIDGET (menu_item))

  GtkMenuItem * menu_item;
  menu_item = create_menu_item (mw->accel_group,
                    "_New",
                    "document-new",
                    0,
                    NULL,
                    0,
                    GDK_KEY_n,
                    GDK_CONTROL_MASK);
  g_signal_connect (G_OBJECT (menu_item),
                    "activate",
                    G_CALLBACK (new_activate),
                    NULL);
  APPEND_TO_FILE_MENU;
  menu_item = create_menu_item (mw->accel_group,
                    "_Open",
                    "document-open",
                    0,
                    NULL,
                    0,
                    GDK_KEY_o,
                    GDK_CONTROL_MASK);
  APPEND_TO_FILE_MENU;
  menu_item = create_menu_item (mw->accel_group,
                    "_Save",
                    "document-save",
                    0,
                    NULL,
                    0,
                    GDK_KEY_s,
                    GDK_CONTROL_MASK);
  APPEND_TO_FILE_MENU;
  menu_item = create_menu_item (mw->accel_group,
                    "Save _As",
                    "document-save-as",
                    0,
                    NULL,
                    0,
                    GDK_KEY_s,
                    GDK_CONTROL_MASK |
                      GDK_SHIFT_MASK);
  APPEND_TO_FILE_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_FILE_MENU;
  menu_item = create_menu_item (mw->accel_group,
                    "_Export As",
                    "document-send",
                    0,
                    NULL,
                    0,
                    GDK_KEY_e,
                    GDK_CONTROL_MASK);
  APPEND_TO_FILE_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_FILE_MENU;
  menu_item = create_menu_item (mw->accel_group,
                    "_Preferences",
                    "document-properties",
                    0,
                    NULL,
                    0,
                    GDK_KEY_p,
                    GDK_CONTROL_MASK);
  APPEND_TO_FILE_MENU;
  menu_item = create_menu_item (mw->accel_group,
                    "_Quit",
                    "window-close",
                    0,
                    NULL,
                    0,
                    GDK_KEY_q,
                    GDK_CONTROL_MASK);
  APPEND_TO_FILE_MENU;
#undef APPEND_TO_FILE_MENU

  menu_item =
    create_menu_item (mw->accel_group,
                      "_Undo",
                      "edit-undo",
                      0,
                      NULL,
                      0,
                      GDK_KEY_z,
                      GDK_CONTROL_MASK);
  g_signal_connect (G_OBJECT (menu_item),
                    "activate",
                    G_CALLBACK (undo_activate),
                    self);
  APPEND_TO_EDIT_MENU;
  self->edit_undo = menu_item;
  menu_item =
    create_menu_item (mw->accel_group,
                      "_Redo",
                      "edit-redo",
                      0,
                      NULL,
                      0,
                      GDK_KEY_z,
                      GDK_CONTROL_MASK |
                        GDK_SHIFT_MASK);
  g_signal_connect (G_OBJECT (menu_item),
                    "activate",
                    G_CALLBACK (redo_activate),
                    self);
  APPEND_TO_EDIT_MENU;
  self->edit_redo = menu_item;
  CREATE_SEPARATOR;
  APPEND_TO_EDIT_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "Cu_t",
                      "edit-cut",
                      0,
                      NULL,
                      0,
                      GDK_KEY_x,
                      GDK_CONTROL_MASK);
  g_signal_connect (G_OBJECT (menu_item),
                    "activate",
                    G_CALLBACK (cut_activate),
                    NULL);
  APPEND_TO_EDIT_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "_Copy",
                      "edit-copy",
                      0,
                      NULL,
                      0,
                      GDK_KEY_c,
                      GDK_CONTROL_MASK);
  g_signal_connect (G_OBJECT (menu_item),
                    "activate",
                    G_CALLBACK (copy_activate),
                    NULL);
  APPEND_TO_EDIT_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "_Paste",
                      "edit-paste",
                      0,
                      NULL,
                      0,
                      GDK_KEY_v,
                      GDK_CONTROL_MASK);
  g_signal_connect (G_OBJECT (menu_item),
                    "activate",
                    G_CALLBACK (paste_activate),
                    NULL);
  APPEND_TO_EDIT_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "_Delete",
                      "edit-delete",
                      0,
                      NULL,
                      0,
                      GDK_KEY_Delete,
                      0);
  g_signal_connect (G_OBJECT (menu_item),
                    "activate",
                    G_CALLBACK (delete_activate),
                    NULL);
  APPEND_TO_EDIT_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_EDIT_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "Cle_ar Selection",
                      "edit-clear",
                      0,
                      NULL,
                      0,
                      GDK_KEY_a,
                      GDK_CONTROL_MASK |
                        GDK_SHIFT_MASK);
  APPEND_TO_EDIT_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "Select A_ll",
                      "edit-select-all",
                      0,
                      NULL,
                      0,
                      GDK_KEY_a,
                      GDK_CONTROL_MASK);
  APPEND_TO_EDIT_MENU;
#undef APPEND_TO_EDIT_MENU

  menu_item =
    create_menu_item (mw->accel_group,
                      "Left Panel",
                      NULL,
                      ICON_TYPE_GNOME_BUILDER,
                      "builder-view-left-pane-symbolic-light.svg",
                      1,
                      GDK_KEY_KP_4,
                      GDK_CONTROL_MASK |
                        GDK_SHIFT_MASK);
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "Right Panel",
                      NULL,
                      ICON_TYPE_GNOME_BUILDER,
                      "builder-view-right-pane-symbolic-light.svg",
                      1,
                      GDK_KEY_KP_6,
                      GDK_CONTROL_MASK |
                        GDK_SHIFT_MASK);
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "Bottom Panel",
                      NULL,
                      ICON_TYPE_GNOME_BUILDER,
                      "builder-view-bottom-pane-symbolic-light.svg",
                      1,
                      GDK_KEY_KP_2,
                      GDK_CONTROL_MASK |
                        GDK_SHIFT_MASK);
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "Status Bar",
                      "edit-select-all",
                      0,
                      NULL,
                      1,
                      0,
                      0);
  APPEND_TO_VIEW_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "_Zoom In",
                      "zoom-in",
                      0,
                      NULL,
                      0,
                      GDK_KEY_KP_Add,
                      GDK_CONTROL_MASK);
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "Zoom _Out",
                      "zoom-out",
                      0,
                      NULL,
                      0,
                      GDK_KEY_KP_Subtract,
                      GDK_CONTROL_MASK);
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "Original Size",
                      "zoom-original",
                      0,
                      NULL,
                      0,
                      GDK_KEY_KP_Equal,
                      GDK_CONTROL_MASK);
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "Best Fit",
                      "zoom-fit-best",
                      0,
                      NULL,
                      0,
                      GDK_KEY_bracketleft,
                      GDK_CONTROL_MASK);
  APPEND_TO_VIEW_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "_Fullscreen",
                      "view-fullscreen",
                      0,
                      NULL,
                      0,
                      GDK_KEY_F11,
                      0);
  APPEND_TO_VIEW_MENU;
#undef APPEND_TO_VIEW_MENU

  menu_item =
    create_menu_item (mw->accel_group,
                      "Manual",
                      "help-contents",
                      0,
                      NULL,
                      0,
                      0,
                      0);
  APPEND_TO_HELP_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "Keyboard Shortcuts",
                      "",
                      0,
                      NULL,
                      0,
                      0,
                      0);
  APPEND_TO_HELP_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "Tutorials",
                      "",
                      0,
                      NULL,
                      0,
                      0,
                      0);
  APPEND_TO_HELP_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_HELP_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "Release Notes",
                      "",
                      0,
                      NULL,
                      0,
                      0,
                      0);
  APPEND_TO_HELP_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "License",
                      "",
                      0,
                      NULL,
                      0,
                      0,
                      0);
  APPEND_TO_HELP_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_HELP_MENU;
  menu_item =
    create_menu_item (mw->accel_group,
                      "About",
                      "help-about",
                      0,
                      NULL,
                      0,
                      0,
                      0);
  APPEND_TO_HELP_MENU;
#undef APPEND_TO_HELP_MENU

#undef APPEND_TO_MENU_SHELL
#undef CREATE_SEPARATOR

  header_bar_widget_refresh_undo_redo_buttons (self);
}

static void
header_bar_widget_init (HeaderBarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  /* make widget able to notify */
  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);
}

static void
header_bar_widget_class_init (HeaderBarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "header_bar.ui");

  gtk_widget_class_set_css_name (klass,
                                 "header-bar");

  gtk_widget_class_bind_template_child (
    klass,
    HeaderBarWidget,
    menu_bar);
  gtk_widget_class_bind_template_child (
    klass,
    HeaderBarWidget,
    file_menu);
  gtk_widget_class_bind_template_child (
    klass,
    HeaderBarWidget,
    edit_menu);
  gtk_widget_class_bind_template_child (
    klass,
    HeaderBarWidget,
    view_menu);
  gtk_widget_class_bind_template_child (
    klass,
    HeaderBarWidget,
    help_menu);
  gtk_widget_class_bind_template_child (
    klass,
    HeaderBarWidget,
    window_buttons);
  gtk_widget_class_bind_template_child (
    klass,
    HeaderBarWidget,
    minimize);
  gtk_widget_class_bind_template_child (
    klass,
    HeaderBarWidget,
    maximize);
  gtk_widget_class_bind_template_child (
    klass,
    HeaderBarWidget,
    close);
  gtk_widget_class_bind_template_callback (
    klass,
    minimize_clicked);
  gtk_widget_class_bind_template_callback (
    klass,
    close_clicked);
  gtk_widget_class_bind_template_callback (
    klass,
    maximize_clicked);
  gtk_widget_class_bind_template_callback (
    klass,
    on_file_open_activate);
  gtk_widget_class_bind_template_callback (
    klass,
    on_file_save_activate);
  gtk_widget_class_bind_template_callback (
    klass,
    on_file_save_as_activate);
  gtk_widget_class_bind_template_callback (
    klass,
    on_file_export_activate);
}
