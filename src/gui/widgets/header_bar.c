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

#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "gui/accel.h"
#include "gui/widgets/export_dialog.h"
#include "gui/widgets/header_bar.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (HeaderBarWidget,
               header_bar_widget,
               GTK_TYPE_HEADER_BAR)

/* TODO rename to refresh buttons and refresh
* everything */
void
header_bar_widget_refresh_undo_redo_buttons (
  HeaderBarWidget * self)
{
  g_warn_if_fail (UNDO_MANAGER);

  gtk_widget_set_sensitive (
    GTK_WIDGET (self->edit_undo),
    !stack_is_empty (UNDO_MANAGER->undo_stack));
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->edit_redo),
    !stack_is_empty (UNDO_MANAGER->redo_stack));

  char * undo = _("_Undo");
  char * redo = _("_Redo");
  if (stack_is_empty (UNDO_MANAGER->undo_stack))
    {
      gtk_label_set_markup_with_mnemonic (
        self->edit_undo_label, undo);
    }
  else
    {
      UndoableAction * ua =
        (UndoableAction *)
        stack_peek (UNDO_MANAGER->undo_stack);
      char * undo2 =
        undoable_action_stringize (ua->type);
      undo =
        g_strdup_printf ("%s %s", undo, undo2);
      gtk_label_set_markup_with_mnemonic (
        self->edit_undo_label, undo);
      g_free (undo2);
      g_free (undo);
    }
  if (stack_is_empty (UNDO_MANAGER->redo_stack))
    {
      gtk_label_set_markup_with_mnemonic (
        self->edit_redo_label, redo);
    }
  else
    {
      UndoableAction * ua =
        (UndoableAction *)
        stack_peek (UNDO_MANAGER->redo_stack);
      char * redo2 =
        undoable_action_stringize (ua->type);
      redo =
        g_strdup_printf ("%s %s", redo, redo2);
      gtk_label_set_markup_with_mnemonic (
        self->edit_redo_label, redo);
      g_free (redo2);
      g_free (redo);
    }
}

static GtkMenuItem *
create_selections_submenu ()
{
  GtkMenuItem * menu_item;
  GtkWidget * menu;
  GtkMenuItem * selections =
    z_gtk_create_menu_item (
      _("Selection"),
      "z-select-rectangular",
      0,
      NULL,
      0,
      NULL);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (selections,
                             menu);

  menu_item =
    z_gtk_create_menu_item (
      _("Loop Selection"),
      "z-media-repeat-album-amarok",
      0,
      NULL,
      0,
      "win.loop-selection");
  gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                         GTK_WIDGET (menu_item));

  return selections;
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
    z_gtk_create_menu_item (
      _("_New"),
      "z-document-new",
      0,
      NULL,
      0,
      "win.new");
  APPEND_TO_FILE_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("_Open"),
      "z-document-open",
      0,
      NULL,
      0,
      "win.open");
  APPEND_TO_FILE_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("_Save"),
      "z-document-save",
      0,
      NULL,
      0,
      "win.save");
  APPEND_TO_FILE_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("Save _As"),
      "z-document-save-as",
      0,
      NULL,
      0,
      "win.save-as");
  APPEND_TO_FILE_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_FILE_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("_Export As"),
      "z-document-send",
      0,
      NULL,
      0,
      "win.export-as");
  APPEND_TO_FILE_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_FILE_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("_Preferences"),
      "z-application-x-m4",
      0,
      NULL,
      0,
      "app.preferences");
  APPEND_TO_FILE_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("_Quit"),
      "z-gtk-quit",
      0,
      NULL,
      0,
      "app.quit");
  APPEND_TO_FILE_MENU;
#undef APPEND_TO_FILE_MENU

  menu_item =
    z_gtk_create_menu_item (
      _("_Undo"),
      "z-edit-undo",
      0,
      NULL,
      0,
      "win.undo");
  APPEND_TO_EDIT_MENU;
  self->edit_undo = menu_item;
  self->edit_undo_label =
    z_gtk_get_label_from_menu_item (
      self->edit_undo);
  g_warn_if_fail (
    GTK_IS_LABEL (self->edit_undo_label));
  menu_item =
    z_gtk_create_menu_item (
      _("_Redo"),
      "z-edit-redo",
      0,
      NULL,
      0,
      "win.redo");
  APPEND_TO_EDIT_MENU;
  self->edit_redo = menu_item;
  self->edit_redo_label =
    z_gtk_get_label_from_menu_item (
      self->edit_redo);
  g_warn_if_fail (
    GTK_IS_LABEL (self->edit_redo_label));
  CREATE_SEPARATOR;
  APPEND_TO_EDIT_MENU;
  menu_item = CREATE_CUT_MENU_ITEM;
  APPEND_TO_EDIT_MENU;
  menu_item = CREATE_COPY_MENU_ITEM;
  APPEND_TO_EDIT_MENU;
  menu_item = CREATE_PASTE_MENU_ITEM;
  APPEND_TO_EDIT_MENU;
  menu_item = CREATE_DELETE_MENU_ITEM;
  APPEND_TO_EDIT_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_EDIT_MENU;
  menu_item = CREATE_CLEAR_SELECTION_MENU_ITEM;
  APPEND_TO_EDIT_MENU;
  menu_item = CREATE_SELECT_ALL_MENU_ITEM;
  APPEND_TO_EDIT_MENU;
  menu_item =
    create_selections_submenu ();
  APPEND_TO_EDIT_MENU;
#undef APPEND_TO_EDIT_MENU

  menu_item =
    z_gtk_create_menu_item (
      _("Left Panel"),
      NULL,
      ICON_TYPE_GNOME_BUILDER,
      "builder-view-left-pane-symbolic-light.svg",
      1,
      "win.toggle-left-panel");
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (menu_item),
    1);
  APPEND_TO_VIEW_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("Right Panel"),
      NULL,
      ICON_TYPE_GNOME_BUILDER,
      "builder-view-right-pane-symbolic-light.svg",
      1,
      "win.toggle-right-panel");
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (menu_item),
    1);
  APPEND_TO_VIEW_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("Bottom Panel"),
      NULL,
      ICON_TYPE_GNOME_BUILDER,
      "builder-view-bottom-pane-symbolic-light.svg",
      1,
      "win.toggle-bot-panel");
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (menu_item),
    1);
  APPEND_TO_VIEW_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("Status Bar"),
      "z-kt-show-statusbar",
      0,
      NULL,
      1,
      "win.toggle-status-bar");
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (menu_item),
    1);
  APPEND_TO_VIEW_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_VIEW_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("_Zoom In"),
      "z-zoom-in",
      0,
      NULL,
      0,
      "win.zoom-in");
  APPEND_TO_VIEW_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("Zoom _Out"),
      "z-zoom-out",
      0,
      NULL,
      0,
      "win.zoom-out");
  APPEND_TO_VIEW_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("Original Size"),
      "z-zoom-original",
      0,
      NULL,
      0,
      "win.original-size");
  APPEND_TO_VIEW_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("Best Fit"),
      "z-zoom-fit-best",
      0,
      NULL,
      0,
      "win.best-fit");
  APPEND_TO_VIEW_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_VIEW_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("_Fullscreen"),
      "z-view-fullscreen",
      0,
      NULL,
      0,
      "app.fullscreen");
  APPEND_TO_VIEW_MENU;
#undef APPEND_TO_VIEW_MENU

  menu_item =
    z_gtk_create_menu_item (
      _("Chat"),
      "z-dialog-messages",
      0,
      NULL,
      0,
      "app.chat");
  APPEND_TO_HELP_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("Manual"),
      "z-help-contents",
      0,
      NULL,
      0,
      "app.manual");
  APPEND_TO_HELP_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("Forums"),
      "z-applications-internet",
      0,
      NULL,
      0,
      "app.forums");
  APPEND_TO_HELP_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("Keyboard Shortcuts"),
      "z-configure-shortcuts",
      0,
      NULL,
      0,
      "app.shortcuts");
  APPEND_TO_HELP_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("Report a Bug"),
      "z-tools-report-bug",
      0,
      NULL,
      0,
      "app.bugreport");
  APPEND_TO_HELP_MENU;
  CREATE_SEPARATOR;
  APPEND_TO_HELP_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("Donate"),
      "z-help-donate",
      0,
      NULL,
      0,
      "app.donate");
  APPEND_TO_HELP_MENU;
  menu_item =
    z_gtk_create_menu_item (
      _("About"),
      "z-help-about",
      0,
      NULL,
      0,
      "app.about");
  APPEND_TO_HELP_MENU;
#undef APPEND_TO_HELP_MENU

#undef APPEND_TO_MENU_SHELL
#undef CREATE_SEPARATOR

  /* FIXME activate/deactivate actions instead */
  header_bar_widget_refresh_undo_redo_buttons (self);
}

void
header_bar_widget_set_subtitle (
  HeaderBarWidget * self,
  const char * subtitle)
{
  gtk_header_bar_set_subtitle (
    GTK_HEADER_BAR (self),
    subtitle);
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
}
