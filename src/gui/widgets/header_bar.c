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
#include "gui/accel.h"
#include "gui/widgets/export_dialog.h"
#include "gui/widgets/header_bar.h"
#include "gui/widgets/main_window.h"
#include "actions/undo_manager.h"
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
create_menu_item (gchar *         label_name,
                  gchar *         icon_name,
                  IconType        resource_icon_type,
                  gchar *         resource,
                  int             is_toggle,
                  const char *    action_name)
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

  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (menu_item),
    action_name);
  accel_set_accel_label_from_action (
    GTK_ACCEL_LABEL (label),
    action_name);

  gtk_box_pack_end (GTK_BOX (box), label, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (menu_item), box);

  gtk_widget_show_all (menu_item);

  return GTK_MENU_ITEM (menu_item);
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
  menu_item =
    create_menu_item (
      "_New",
      "document-new",
      0,
      NULL,
      0,
      "win.new");
  APPEND_TO_FILE_MENU;
  menu_item =
    create_menu_item (
      "_Open",
      "document-open",
      0,
      NULL,
      0,
      "win.open");
  APPEND_TO_FILE_MENU;
  menu_item =
    create_menu_item (
      "_Save",
      "document-save",
      0,
      NULL,
      0,
      "win.save");
  APPEND_TO_FILE_MENU;
  menu_item =
    create_menu_item (
      "Save _As",
      "document-save-as",
      0,
      NULL,
      0,
      "win.save-as");
  APPEND_TO_FILE_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_FILE_MENU;
  menu_item =
    create_menu_item (
      "_Export As",
      "document-send",
      0,
      NULL,
      0,
      "win.export");
  APPEND_TO_FILE_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_FILE_MENU;
  menu_item =
    create_menu_item (
      "_Preferences",
      "document-properties",
      0,
      NULL,
      0,
      "win.properties"); /* project properties */
  APPEND_TO_FILE_MENU;
  menu_item =
    create_menu_item (
      "_Quit",
      "window-close",
      0,
      NULL,
      0,
      "app.quit");
  APPEND_TO_FILE_MENU;
#undef APPEND_TO_FILE_MENU

  menu_item =
    create_menu_item (
      "_Undo",
      "edit-undo",
      0,
      NULL,
      0,
      "win.undo");
  APPEND_TO_EDIT_MENU;
  self->edit_undo = menu_item;
  menu_item =
    create_menu_item (
      "_Redo",
      "edit-redo",
      0,
      NULL,
      0,
      "win.redo");
  APPEND_TO_EDIT_MENU;
  self->edit_redo = menu_item;
  CREATE_SEPARATOR;
  APPEND_TO_EDIT_MENU;
  menu_item =
    create_menu_item (
      "Cu_t",
      "edit-cut",
      0,
      NULL,
      0,
      "win.cut");
  APPEND_TO_EDIT_MENU;
  menu_item =
    create_menu_item (
      "_Copy",
      "edit-copy",
      0,
      NULL,
      0,
      "win.copy");
  APPEND_TO_EDIT_MENU;
  menu_item =
    create_menu_item (
      "_Paste",
      "edit-paste",
      0,
      NULL,
      0,
      "win.paste");
  APPEND_TO_EDIT_MENU;
  menu_item =
    create_menu_item (
      "_Delete",
      "edit-delete",
      0,
      NULL,
      0,
      "win.delete");
  APPEND_TO_EDIT_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_EDIT_MENU;
  menu_item =
    create_menu_item (
      "Cle_ar Selection",
      "edit-clear",
      0,
      NULL,
      0,
      "win.clear-selection");
  APPEND_TO_EDIT_MENU;
  menu_item =
    create_menu_item (
      "Select A_ll",
      "edit-select-all",
      0,
      NULL,
      0,
      "win.select-all");
  APPEND_TO_EDIT_MENU;
#undef APPEND_TO_EDIT_MENU

  menu_item =
    create_menu_item (
      "Left Panel",
      NULL,
      ICON_TYPE_GNOME_BUILDER,
      "builder-view-left-pane-symbolic-light.svg",
      1,
      "win.toggle-left-panel");
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (
      "Right Panel",
      NULL,
      ICON_TYPE_GNOME_BUILDER,
      "builder-view-right-pane-symbolic-light.svg",
      1,
      "win.toggle-right-panel");
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (
      "Bottom Panel",
      NULL,
      ICON_TYPE_GNOME_BUILDER,
      "builder-view-bottom-pane-symbolic-light.svg",
      1,
      "win.toggle-bot-panel");
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (
      "Status Bar",
      "edit-select-all",
      0,
      NULL,
      1,
      "win.toggle-status-bar");
  APPEND_TO_VIEW_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (
      "_Zoom In",
      "zoom-in",
      0,
      NULL,
      0,
      "win.zoom-in");
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (
      "Zoom _Out",
      "zoom-out",
      0,
      NULL,
      0,
      "win.zoom-out");
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (
      "Original Size",
      "zoom-original",
      0,
      NULL,
      0,
      "win.original-size");
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (
      "Best Fit",
      "zoom-fit-best",
      0,
      NULL,
      0,
      "win.best-fit");
  APPEND_TO_VIEW_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_VIEW_MENU;
  menu_item =
    create_menu_item (
      "_Fullscreen",
      "view-fullscreen",
      0,
      NULL,
      0,
      "app.fullscren");
  APPEND_TO_VIEW_MENU;
#undef APPEND_TO_VIEW_MENU

  menu_item =
    create_menu_item (
      "Manual",
      "help-contents",
      0,
      NULL,
      0,
      "app.manual");
  APPEND_TO_HELP_MENU;
  menu_item =
    create_menu_item (
      "Keyboard Shortcuts",
      "",
      0,
      NULL,
      0,
      "app.shortcuts");
  APPEND_TO_HELP_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_HELP_MENU;
  menu_item =
    create_menu_item (
      "License",
      "",
      0,
      NULL,
      0,
      "app.license");
  APPEND_TO_HELP_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_HELP_MENU;
  menu_item =
    create_menu_item (
      "About",
      "help-about",
      0,
      NULL,
      0,
      "app.about");
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
}
