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
  GtkWidget * widget;
  for (int i = 0; i < num_pages; i++)
    {
      /* set the child visible */
      widget =
        foldable_notebook_widget_get_widget_at_page (
          self, i);
      gtk_widget_set_visible (
        widget, (guint) i == page_num ? 1 : 0);
    }
}

/**
 * Sets the folded space visible or not.
 */
void
foldable_notebook_widget_set_visibility (
  FoldableNotebookWidget * self,
  int                      new_visibility)
{
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
      GtkWidget * widget =
        foldable_notebook_widget_get_widget_at_page (
          self, i);
      gtk_widget_set_visible (
        widget, new_visibility);
    }

  if (new_visibility)
    {
      if (self->paned)
        {
          gtk_paned_set_position (
            self->paned, self->prev_pos);
        }
    }
  else
    {
      if (self->paned)
        {
          /*[> remember position before hiding <]*/
          self->prev_pos =
            gtk_paned_get_position (
              self->paned);

          /*[> hide <]*/
          int position;
          if (self->pos_in_paned == GTK_POS_BOTTOM)
            {
              position =
                gtk_widget_get_allocated_height (
                  GTK_WIDGET (self->paned));
            }
          else if (self->pos_in_paned ==
                     GTK_POS_RIGHT)
            {
              position =
                gtk_widget_get_allocated_width (
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

/**
 * Returns if the content of the foldable notebook
 * is visible.
 */
int
foldable_notebook_widget_is_content_visible (
  FoldableNotebookWidget * self)
{
  GtkWidget * widget =
    foldable_notebook_widget_get_current_widget (
      self);
  return
    gtk_widget_get_visible (widget);
}

/**
 * Get the widget currently visible.
 */
GtkWidget *
foldable_notebook_widget_get_current_widget (
  FoldableNotebookWidget * self)
{
  GtkContainer * current_box =
    GTK_CONTAINER (
      z_gtk_notebook_get_current_page_widget (
        GTK_NOTEBOOK (self)));
  GtkWidget * widget =
    z_gtk_container_get_single_child (
      GTK_CONTAINER (current_box));
  return widget;
}

/**
 * Combines the above.
 */
void
foldable_notebook_widget_toggle_visibility (
  FoldableNotebookWidget * self)
{
  g_return_if_fail (self);
  foldable_notebook_widget_set_visibility (
    self,
    !foldable_notebook_widget_is_content_visible (
       self));
}

GtkWidget *
foldable_notebook_widget_get_widget_at_page (
  FoldableNotebookWidget * self,
  int                      page)
{
  GtkContainer * container =
    GTK_CONTAINER (
      gtk_notebook_get_nth_page (
        GTK_NOTEBOOK (self), page));
  GtkWidget * widget =
    z_gtk_container_get_single_child (
      GTK_CONTAINER (container));
  return widget;
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
  GtkWidget * current_tab =
    z_gtk_notebook_get_current_tab_label_widget (
      GTK_NOTEBOOK (self));
  int hit =
    ui_is_child_hit (
      GTK_WIDGET (self),
      current_tab,
      1, 1, x, y,  16, 3);
  if (hit)
    {

      int new_visibility =
        !foldable_notebook_widget_is_content_visible (
           self);

      foldable_notebook_widget_set_visibility (
        self, new_visibility);
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
  /*DzlDockRevealer *        dock_revealer,*/
  GtkPositionType          pos_in_paned)
{
  self->paned = paned;
  self->pos_in_paned = pos_in_paned;
  /*self->dock_revealer = dock_revealer;*/

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
