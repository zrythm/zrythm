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
#include "actions/undo_stack.h"
#include "actions/undoable_action.h"
#include "gui/accel.h"
#include "gui/widgets/export_dialog.h"
#include "gui/widgets/home_toolbar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/snap_box.h"
#include "gui/widgets/toolbox.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/stack.h"
#include "zrythm_app.h"

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
    !undo_stack_is_empty (UNDO_MANAGER->undo_stack));
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->redo),
    !undo_stack_is_empty (UNDO_MANAGER->redo_stack));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), \
    _(tooltip))
  char * undo = _("Undo");
  char * redo = _("Redo");
  if (undo_stack_is_empty (UNDO_MANAGER->undo_stack))
    {
      SET_TOOLTIP (undo, undo);
    }
  else
    {
      UndoableAction * ua =
        (UndoableAction *)
        undo_stack_peek (UNDO_MANAGER->undo_stack);
      char * undo2 =
        undoable_action_stringize (ua);
      undo =
        g_strdup_printf ("%s %s", undo, undo2);
      SET_TOOLTIP (undo, undo);
      g_free (undo2);
      g_free (undo);
    }
  if (undo_stack_is_empty (UNDO_MANAGER->redo_stack))
    {
      SET_TOOLTIP (redo, redo);
    }
  else
    {
      UndoableAction * ua =
        (UndoableAction *)
        undo_stack_peek (UNDO_MANAGER->redo_stack);
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
  home_toolbar_widget_refresh_undo_redo_buttons (
    self);

  toolbox_widget_refresh (self->toolbox);
}

static void
home_toolbar_widget_init (HomeToolbarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), \
    tooltip)
  SET_TOOLTIP (
    undo, _("Undo"));
  SET_TOOLTIP (
    redo, _("Redo"));
  SET_TOOLTIP (
    cut, _("Cut"));
  SET_TOOLTIP (
    copy, _("Copy"));
  SET_TOOLTIP (
    paste, _("Paste"));
  SET_TOOLTIP (
    duplicate, _("Duplicate"));
  SET_TOOLTIP (
    delete, _("Delete"));
  SET_TOOLTIP (
    clear_selection, _("Clear Selection"));
  SET_TOOLTIP (
    select_all, _("Select All"));
  SET_TOOLTIP (
    loop_selection, _("Loop Selection"));
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

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    HomeToolbarWidget, \
    x)

  BIND_CHILD (undo);
  BIND_CHILD (redo);
  BIND_CHILD (cut);
  BIND_CHILD (copy);
  BIND_CHILD (paste);
  BIND_CHILD (duplicate);
  BIND_CHILD (delete);
  BIND_CHILD (clear_selection);
  BIND_CHILD (select_all);
  BIND_CHILD (loop_selection);
  BIND_CHILD (toolbox);
  BIND_CHILD (snap_box);

#undef BIND_CHILD
}
