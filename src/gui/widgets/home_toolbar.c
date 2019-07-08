/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "gui/accel.h"
#include "gui/widgets/export_dialog.h"
#include "gui/widgets/home_toolbar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/quantize_mb.h"
#include "gui/widgets/snap_box.h"
#include "gui/widgets/toolbox.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (HomeToolbarWidget,
               home_toolbar_widget,
               GTK_TYPE_TOOLBAR)

/* TODO rename to refresh buttons and refresh
* everything */
void
home_toolbar_widget_refresh_undo_redo_buttons (
  HomeToolbarWidget * self)
{
  g_warn_if_fail (UNDO_MANAGER);

  gtk_widget_set_sensitive (
    GTK_WIDGET (self->undo),
    !stack_is_empty (UNDO_MANAGER->undo_stack));
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->redo),
    !stack_is_empty (UNDO_MANAGER->redo_stack));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), \
    _(tooltip))
  char * undo = _("Undo");
  char * redo = _("Redo");
  if (stack_is_empty (UNDO_MANAGER->undo_stack))
    {
      SET_TOOLTIP (undo, undo);
    }
  else
    {
      UndoableAction * ua =
        (UndoableAction *)
        stack_peek (UNDO_MANAGER->undo_stack);
      char * undo2 =
        undoable_action_stringize (ua);
      undo =
        g_strdup_printf ("%s %s", undo, undo2);
      SET_TOOLTIP (undo, undo);
      g_free (undo2);
      g_free (undo);
    }
  if (stack_is_empty (UNDO_MANAGER->redo_stack))
    {
      SET_TOOLTIP (redo, redo);
    }
  else
    {
      UndoableAction * ua =
        (UndoableAction *)
        stack_peek (UNDO_MANAGER->redo_stack);
      char * redo2 =
        undoable_action_stringize (ua);
      redo =
        g_strdup_printf ("%s %s", redo, redo2);
      SET_TOOLTIP (redo, redo);
      g_free (redo2);
      g_free (redo);
    }
#undef SET_TOOLTIP
}

void
home_toolbar_widget_setup (HomeToolbarWidget * self)
{
  /* FIXME activate/deactivate actions instead */
  home_toolbar_widget_refresh_undo_redo_buttons (self);

  toolbox_widget_refresh (self->toolbox);
  quantize_mb_widget_refresh (
    self->quantize_mb);

  /* setup top toolbar */
  quantize_mb_widget_setup (
    self->quantize_mb,
    QUANTIZE_TIMELINE);

}

static void
home_toolbar_widget_init (HomeToolbarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), \
    _(tooltip))
  SET_TOOLTIP (undo, "Undo");
  SET_TOOLTIP (redo, "Redo");
  SET_TOOLTIP (cut, "Cut");
  SET_TOOLTIP (copy, "Copy");
  SET_TOOLTIP (paste, "Paste");
  SET_TOOLTIP (duplicate, "Duplicate");
  SET_TOOLTIP (delete, "Delete");
  SET_TOOLTIP (clear_selection, "Clear Selection");
  SET_TOOLTIP (select_all, "Select All");
  SET_TOOLTIP (loop_selection, "Loop Selection");
#undef SET_TOOLTIP
}

static void
home_toolbar_widget_class_init (HomeToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "home_toolbar.ui");

  gtk_widget_class_set_css_name (
    klass, "home-toolbar");

  gtk_widget_class_bind_template_child (
    klass,
    HomeToolbarWidget,
    undo);
  gtk_widget_class_bind_template_child (
    klass,
    HomeToolbarWidget,
    redo);
  gtk_widget_class_bind_template_child (
    klass,
    HomeToolbarWidget,
    cut);
  gtk_widget_class_bind_template_child (
    klass,
    HomeToolbarWidget,
    copy);
  gtk_widget_class_bind_template_child (
    klass,
    HomeToolbarWidget,
    paste);
  gtk_widget_class_bind_template_child (
    klass,
    HomeToolbarWidget,
    duplicate);
  gtk_widget_class_bind_template_child (
    klass,
    HomeToolbarWidget,
    delete);
  gtk_widget_class_bind_template_child (
    klass,
    HomeToolbarWidget,
    clear_selection);
  gtk_widget_class_bind_template_child (
    klass,
    HomeToolbarWidget,
    select_all);
  gtk_widget_class_bind_template_child (
    klass,
    HomeToolbarWidget,
    loop_selection);
  gtk_widget_class_bind_template_child (
    klass,
    HomeToolbarWidget,
    toolbox);
  gtk_widget_class_bind_template_child (
    klass,
    HomeToolbarWidget,
    snap_box);
  gtk_widget_class_bind_template_child (
    klass,
    HomeToolbarWidget,
    quantize_mb);
}
