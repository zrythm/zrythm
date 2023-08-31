// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/undo_manager.h"
#include "actions/undo_stack.h"
#include "actions/undoable_action.h"
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
refresh_undo_or_redo_button (HomeToolbarWidget * self, bool redo)
{
  g_warn_if_fail (UNDO_MANAGER);

  AdwSplitButton * split_btn =
    redo ? self->redo_btn : self->undo_btn;
  UndoStack * stack =
    redo ? UNDO_MANAGER->redo_stack : UNDO_MANAGER->undo_stack;

  gtk_widget_set_sensitive (
    GTK_WIDGET (split_btn), !undo_stack_is_empty (stack));

#define SET_TOOLTIP(tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (split_btn), (tooltip))

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
      int max_actions = MIN (8, stack->stack->top + 1);
      for (int i = 0; i < max_actions;)
        {
          UndoableAction * ua =
            stack->stack->elements
              [g_atomic_int_get (&stack->stack->top) - i];

          char * action_str = undoable_action_to_string (ua);

          char tmp[600];
          sprintf (tmp, "app.%s_n", redo ? "redo" : "undo");
          menuitem =
            z_gtk_create_menu_item (action_str, NULL, tmp);
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
      adw_split_button_set_menu_model (
        split_btn, G_MENU_MODEL (menu));
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
  home_toolbar_widget_refresh_undo_redo_buttons (self);

  toolbox_widget_refresh (self->toolbox);
}

static void
home_toolbar_widget_init (HomeToolbarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), tooltip)
  SET_TOOLTIP (cut, _ ("Cut"));
  SET_TOOLTIP (copy, _ ("Copy"));
  SET_TOOLTIP (paste, _ ("Paste"));
  SET_TOOLTIP (duplicate, _ ("Duplicate"));
  SET_TOOLTIP (delete, _ ("Delete"));
  SET_TOOLTIP (clear_selection, _ ("Clear selection"));
  SET_TOOLTIP (select_all, _ ("Select all"));
  SET_TOOLTIP (loop_selection, _ ("Loop selection"));
  SET_TOOLTIP (nudge_left, _ ("Nudge left"));
  SET_TOOLTIP (nudge_right, _ ("Nudge right"));
#undef SET_TOOLTIP
}

static void
home_toolbar_widget_class_init (
  HomeToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "home_toolbar.ui");

  gtk_widget_class_set_css_name (klass, "home-toolbar");
  gtk_widget_class_set_accessible_role (
    klass, GTK_ACCESSIBLE_ROLE_TOOLBAR);

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, HomeToolbarWidget, x)

  BIND_CHILD (undo_btn);
  BIND_CHILD (redo_btn);
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
