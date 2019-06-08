/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/dzl/dzl-dock-revealer.h"
#include "gui/widgets/foldable_notebook.h"
#include "utils/gtk.h"
#include "utils/ui.h"

G_DEFINE_TYPE (
  FoldableNotebookWidget,
  foldable_notebook_widget,
  GTK_TYPE_NOTEBOOK)

static void
on_switch_page (
  GtkNotebook * notebook,
  GtkWidget * page,
  guint       page_num,
  FoldableNotebookWidget * self)
{
  int num_pages =
    gtk_notebook_get_n_pages (notebook);
  GtkBox * box;
  GtkWidget * widget;
  for (int i = 0; i < num_pages; i++)
    {
      box =
        GTK_BOX (
          gtk_notebook_get_nth_page (notebook, i));

      /* set the child visible */
      widget =
        z_gtk_container_get_single_child (
          GTK_CONTAINER (box));
      gtk_widget_set_visible (widget, 1);
    }
}

/**
 * Callback for the foldable notebook.
 */
static void
on_multipress_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  FoldableNotebookWidget * self)
{
  GtkBox * current_box =
    GTK_BOX (
      z_gtk_notebook_get_current_page_widget (
        GTK_NOTEBOOK (self)));
  GtkWidget * current_tab =
    z_gtk_notebook_get_current_tab_label_widget (
      GTK_NOTEBOOK (self));
  GtkPositionType pos_type =
    gtk_notebook_get_tab_pos (
      GTK_NOTEBOOK (self));
  int hit =
    ui_is_child_hit (
      GTK_CONTAINER (self),
      current_tab,
      pos_type == GTK_POS_TOP ||
        pos_type == GTK_POS_BOTTOM,
      pos_type == GTK_POS_LEFT ||
        pos_type == GTK_POS_RIGHT,
      x, y,  3, 3);
  if (hit)
    {
      GtkWidget * widget =
        z_gtk_container_get_single_child (
          GTK_CONTAINER (current_box));
      int new_visibility =
        !gtk_widget_get_visible (widget);

      /* toggle visibility of all box children.
       * do this on the children because toggling
       * the visibility of the box causes gtk to
       * automatically hide the tab too */
      int num_pages =
        gtk_notebook_get_n_pages (
          GTK_NOTEBOOK (self));
      /*int max_width = 0, max_height = 0;*/
      for (int i = 0; i < num_pages; i++)
        {
          current_box =
            GTK_BOX (
              gtk_notebook_get_nth_page (
                GTK_NOTEBOOK (self), i));
          widget =
            z_gtk_container_get_single_child (
              GTK_CONTAINER (current_box));
          gtk_widget_set_visible (
            widget, new_visibility);
        }

      if (!new_visibility)
        {
          if  (self->dock_revealer)
            {
              dzl_dock_revealer_set_position (
                self->dock_revealer, 0);
            }
          else if (self->paned)
            {
              int position;
              if (self->pos_in_paned ==
                    GTK_POS_RIGHT ||
                  self->pos_in_paned ==
                    GTK_POS_BOTTOM)
                {
                  position =
                    gtk_widget_get_allocated_height (
                      GTK_WIDGET (self->paned));
                }
              else
                {
                  position = 0;
                }
              gtk_paned_set_position (
                self->paned, position);
            }
        }
    }
}

FoldableNotebookWidget *
foldable_notebook_widget_new ()
{
  FoldableNotebookWidget * self =
    g_object_new (FOLDABLE_NOTEBOOK_WIDGET_TYPE,
                  NULL);

  return self;
}

/**
 * Sets up an existing FoldableNotebookWidget.
 */
void
foldable_notebook_widget_setup (
  FoldableNotebookWidget * self,
  GtkPaned *               paned,
  DzlDockRevealer *        dock_revealer,
  GtkPositionType          pos_in_paned)
{
  self->paned = paned;
  self->pos_in_paned = pos_in_paned;
  self->dock_revealer = dock_revealer;

  /* add events */
  gtk_widget_add_events (
    GTK_WIDGET (self),
    GDK_ALL_EVENTS_MASK);

  /* add signals */
  self->mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->mp),
    GTK_PHASE_CAPTURE);
  g_signal_connect (
    G_OBJECT (self->mp), "pressed",
    G_CALLBACK (on_multipress_pressed),
    self);
  g_signal_connect (
    G_OBJECT (self), "switch-page",
    G_CALLBACK (on_switch_page),
    self);
}

static void
foldable_notebook_widget_class_init (
  FoldableNotebookWidgetClass * klass)
{
}

static void
foldable_notebook_widget_init (
  FoldableNotebookWidget * self)
{
}
