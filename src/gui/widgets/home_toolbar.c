/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/button_with_menu.h"
#include "gui/widgets/dialogs/export_dialog.h"
#include "gui/widgets/home_toolbar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/toolbox.h"
#include "project.h"
#include "utils/error.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/stack.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  HomeToolbarWidget,
  home_toolbar_widget,
  GTK_TYPE_BOX)

static void
refresh_undo_or_redo_button (
  HomeToolbarWidget * self,
  bool                redo)
{
  g_warn_if_fail (UNDO_MANAGER);

  ButtonWithMenuWidget * btn_w_menu =
    redo ? self->redo : self->undo;
  UndoStack * stack =
    redo ? UNDO_MANAGER->redo_stack
         : UNDO_MANAGER->undo_stack;
  GtkButton * btn =
    redo ? self->redo_btn : self->undo_btn;

  gtk_widget_set_sensitive (
    GTK_WIDGET (btn_w_menu),
    !undo_stack_is_empty (stack));

#define SET_TOOLTIP(tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (btn), (tooltip))

  const char * undo_or_redo_str =
    redo ? _ ("Redo") : _ ("Undo");
  GMenu * menu = NULL;
  if (undo_stack_is_empty (stack))
    {
      SET_TOOLTIP (undo_or_redo_str);
    }
  else
    {
      menu = g_menu_new ();
      GMenuItem * menuitem;

      /* fill 8 actions */
      int max_actions =
        MIN (8, stack->stack->top + 1);
      for (int i = 0; i < max_actions;)
        {
          UndoableAction * ua =
            stack->stack->elements
              [g_atomic_int_get (&stack->stack->top)
               - i];

          char * action_str =
            undoable_action_to_string (ua);

          char tmp[600];
          sprintf (
            tmp, "app.%s_n",
            redo ? "redo" : "undo");
          menuitem = z_gtk_create_menu_item (
            action_str, NULL, tmp);
          g_menu_item_set_action_and_target_value (
            menuitem, tmp, g_variant_new_int32 (i));
          g_menu_append_item (menu, menuitem);

          if (i == 0)
            {
              char tooltip[800];
              sprintf (
                tooltip, "%s %s", undo_or_redo_str,
                action_str);
              if (ua->num_actions > 1)
                {
                  char num_actions_str[100];
                  sprintf (
                    num_actions_str, " (x%d)",
                    ua->num_actions);
                  strcat (tooltip, num_actions_str);
                }
              SET_TOOLTIP (tooltip);
            }
          g_free (action_str);

          g_return_if_fail (ua->num_actions >= 1);
          i += ua->num_actions;
        }
    }
#undef SET_TOOLTIP

  if (menu)
    {
      gtk_menu_button_set_menu_model (
        btn_w_menu->menu_btn, G_MENU_MODEL (menu));
    }
}

/* TODO rename to refresh buttons and refresh
* everything */
void
home_toolbar_widget_refresh_undo_redo_buttons (
  HomeToolbarWidget * self)
{
  g_warn_if_fail (UNDO_MANAGER);

  refresh_undo_or_redo_button (self, false);
  refresh_undo_or_redo_button (self, true);
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
  g_type_ensure (BUTTON_WITH_MENU_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), tooltip)
  SET_TOOLTIP (cut, _ ("Cut"));
  SET_TOOLTIP (copy, _ ("Copy"));
  SET_TOOLTIP (paste, _ ("Paste"));
  SET_TOOLTIP (duplicate, _ ("Duplicate"));
  SET_TOOLTIP (delete, _ ("Delete"));
  SET_TOOLTIP (
    clear_selection, _ ("Clear selection"));
  SET_TOOLTIP (select_all, _ ("Select all"));
  SET_TOOLTIP (
    loop_selection, _ ("Loop selection"));
  SET_TOOLTIP (nudge_left, _ ("Nudge left"));
  SET_TOOLTIP (nudge_right, _ ("Nudge right"));
#undef SET_TOOLTIP

  self->undo_btn = GTK_BUTTON (
    gtk_button_new_from_icon_name ("edit-undo"));
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->undo_btn), "app.undo");
  gtk_widget_set_visible (
    GTK_WIDGET (self->undo_btn), true);
  self->redo_btn = GTK_BUTTON (
    gtk_button_new_from_icon_name ("edit-redo"));
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->redo_btn), "app.redo");
  gtk_widget_set_visible (
    GTK_WIDGET (self->redo_btn), true);

  /* setup button with menu widget */
  button_with_menu_widget_setup (
    self->undo, GTK_BUTTON (self->undo_btn), NULL,
    true, 34, _ ("Undo"), _ ("Undo..."));
  button_with_menu_widget_setup (
    self->redo, GTK_BUTTON (self->redo_btn), NULL,
    true, 34, _ ("Redo"), _ ("Redo..."));
}

static void
home_toolbar_widget_class_init (
  HomeToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "home_toolbar.ui");

  gtk_widget_class_set_css_name (
    klass, "home-toolbar");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, HomeToolbarWidget, x)

  BIND_CHILD (undo);
  BIND_CHILD (redo);
  BIND_CHILD (cut);
  BIND_CHILD (copy);
  BIND_CHILD (paste);
  BIND_CHILD (duplicate);
  BIND_CHILD (delete);
  BIND_CHILD (nudge_left);
  BIND_CHILD (nudge_right);
  BIND_CHILD (clear_selection);
  BIND_CHILD (select_all);
  BIND_CHILD (loop_selection);
  BIND_CHILD (toolbox);

#undef BIND_CHILD
}
