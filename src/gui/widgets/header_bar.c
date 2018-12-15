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

G_DEFINE_TYPE (HeaderBarWidget,
               header_bar_widget,
               GTK_TYPE_HEADER_BAR)

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
  gtk_window_iconify (GTK_WINDOW (MAIN_WINDOW));
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
      project_save (filename);
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

   project_save (PROJECT->dir);
}

void
header_bar_widget_set_title (HeaderBarWidget * self,
                             const char * title)
{
  gtk_header_bar_set_title (GTK_HEADER_BAR (self),
                            "Zrythm");
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (self),
                               title);
}

HeaderBarWidget *
header_bar_widget_new ()
{
  HeaderBarWidget * self = g_object_new (
    HEADER_BAR_WIDGET_TYPE,
    NULL);

  return self;
}

static void
header_bar_widget_init (HeaderBarWidget * self)
{
  g_message ("init header bar widget");
  gtk_widget_init_template (GTK_WIDGET (self));

  /* setup menu items */
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *icon = gtk_image_new_from_icon_name ("document-send",
                                                  GTK_ICON_SIZE_MENU);
  GtkWidget *label = gtk_accel_label_new ("_Export Audio");
  GtkWidget *menu_item = gtk_menu_item_new ();
  GtkAccelGroup *accel_group = gtk_accel_group_new ();

  gtk_container_add (GTK_CONTAINER (box), icon);

  gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);

  gtk_widget_add_accelerator (GTK_WIDGET (self->file_export),
                              "activate", accel_group,
                              GDK_KEY_e, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), menu_item);

  gtk_box_pack_end (GTK_BOX (box), label, TRUE, TRUE, 0);

  GList *children, *iter;

  children = gtk_container_get_children(GTK_CONTAINER(self->file_export));
  for(iter = children; iter != NULL; iter = g_list_next(iter))
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  g_list_free(children);
  gtk_container_add (GTK_CONTAINER (self->file_export), box);

  gtk_widget_show_all (GTK_WIDGET (self->file_export));

  /* set button icons */
  GtkWidget * image = gtk_image_new_from_resource (
          "/org/zrythm/icons/close.svg");
  gtk_button_set_image (self->close, image);
  image = gtk_image_new_from_resource (
          "/org/zrythm/icons/minimize.svg");
  gtk_button_set_image (self->minimize, image);
  image = gtk_image_new_from_resource (
          "/org/zrythm/icons/maximize.svg");
  gtk_button_set_image (self->maximize, image);
}

static void
header_bar_widget_class_init (HeaderBarWidgetClass * klass)
{
  g_message ("init header bar class");
  gtk_widget_class_set_template_from_resource (
    GTK_WIDGET_CLASS (klass),
    "/org/zrythm/ui/header_bar.ui");
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    file);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    file_new);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    file_open);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    file_save);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    file_save_as);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    file_export);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    file_quit);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    edit);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    view);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    help);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    window_buttons);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    minimize);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    maximize);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    HeaderBarWidget,
    close);
  gtk_widget_class_bind_template_callback (
    GTK_WIDGET_CLASS (klass),
    minimize_clicked);
  gtk_widget_class_bind_template_callback (
    GTK_WIDGET_CLASS (klass),
    close_clicked);
  gtk_widget_class_bind_template_callback (
    GTK_WIDGET_CLASS (klass),
    maximize_clicked);
  gtk_widget_class_bind_template_callback (
    GTK_WIDGET_CLASS (klass),
    on_file_open_activate);
  gtk_widget_class_bind_template_callback (
    GTK_WIDGET_CLASS (klass),
    on_file_save_activate);
  gtk_widget_class_bind_template_callback (
    GTK_WIDGET_CLASS (klass),
    on_file_save_as_activate);
  gtk_widget_class_bind_template_callback (
    GTK_WIDGET_CLASS (klass),
    on_file_export_activate);
  g_message ("aa");
}
