/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/gtk_flipper.h"
#include "gui/widgets/main_window.h"
#include "utils/gtk.h"
#include "utils/ui.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  FoldableNotebookWidget,
  foldable_notebook_widget,
  GTK_TYPE_BOX)

static void
on_switch_page (
  GtkNotebook *            notebook,
  GtkWidget *              page,
  guint                    page_num,
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
      if (!widget)
        continue;
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
  bool                     new_visibility)
{
  /* toggle visibility of all box children.
   * do this on the children because toggling
   * the visibility of the box causes gtk to
   * automatically hide the tab too */
  int num_pages = gtk_notebook_get_n_pages (
    GTK_NOTEBOOK (self->notebook));
  for (int i = 0; i < num_pages; i++)
    {
      GtkWidget * widget =
        foldable_notebook_widget_get_widget_at_page (
          self, i);
      g_return_if_fail (GTK_IS_WIDGET (widget));
      gtk_widget_set_visible (
        widget, new_visibility);
    }

  if (new_visibility)
    {
      if (self->paned && self->prev_pos > 0)
        {
          gtk_paned_set_position (
            self->paned, self->prev_pos);
        }
    }
  else
    {
      if (self->paned)
        {
          /* remember position before hiding */
          self->prev_pos =
            gtk_paned_get_position (self->paned);

          /* hide */
          int position;
          if (self->pos_in_paned == GTK_POS_BOTTOM)
            {
              position =
                gtk_widget_get_allocated_height (
                  GTK_WIDGET (self->paned));
            }
          else if (self->pos_in_paned == GTK_POS_RIGHT)
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
  return gtk_widget_get_visible (widget);
}

/**
 * Get the widget currently visible.
 */
GtkWidget *
foldable_notebook_widget_get_current_widget (
  FoldableNotebookWidget * self)
{
  GtkWidget * current_box = GTK_WIDGET (
    z_gtk_notebook_get_current_page_widget (
      GTK_NOTEBOOK (self->notebook)));
  GtkWidget * widget = gtk_widget_get_first_child (
    GTK_WIDGET (current_box));
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
  GtkNotebook * notebook = self->notebook;
  int           num_pages =
    gtk_notebook_get_n_pages (notebook);
  g_return_val_if_fail (page < num_pages, NULL);
  GtkWidget * container =
    GTK_WIDGET (gtk_notebook_get_nth_page (
      GTK_NOTEBOOK (notebook), page));
  g_return_val_if_fail (container, NULL);
  GtkWidget * widget = gtk_widget_get_first_child (
    GTK_WIDGET (container));
  g_return_val_if_fail (widget, NULL);
  return widget;
}

/**
 * Tab release callback.
 */
static void
on_multipress_released (
  GtkGestureClick *        gesture,
  gint                     n_press,
  gdouble                  x,
  gdouble                  y,
  FoldableNotebookWidget * self)
{
  bool hit = ui_is_child_hit (
    GTK_WIDGET (self), self->tab_during_press, 1, 1,
    x, y, 16, 3);
  if (hit)
    {

      int new_visibility =
        !foldable_notebook_widget_is_content_visible (
          self);

      foldable_notebook_widget_set_visibility (
        self, new_visibility);
    }
}

/**
 * Tab press callback.
 */
static void
on_multipress_pressed (
  GtkGestureClick *        gesture,
  gint                     n_press,
  gdouble                  x,
  gdouble                  y,
  FoldableNotebookWidget * self)
{
  self->tab_during_press =
    z_gtk_notebook_get_current_tab_label_widget (
      GTK_NOTEBOOK (self->notebook));
}

/**
 * Gets the internal notebook.
 */
GtkNotebook *
foldable_notebook_widget_get_notebook (
  FoldableNotebookWidget * self)
{
  return self->notebook;
}

void
foldable_notebook_widget_add_page (
  FoldableNotebookWidget * self,
  GtkWidget *              child,
  const char *             tab_icon_name,
  const char *             tab_label,
  const char *             tooltip)
{
  GtkNotebook * notebook =
    foldable_notebook_widget_get_notebook (self);
  GtkBox * box = GTK_BOX (gtk_box_new (
    (self->pos_in_paned == GTK_POS_LEFT
     || self->pos_in_paned == GTK_POS_RIGHT)
      ? GTK_ORIENTATION_VERTICAL
      : GTK_ORIENTATION_HORIZONTAL,
    4));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (box), tooltip);
  GtkWidget * img =
    gtk_image_new_from_icon_name (tab_icon_name);
  GtkWidget * img_flipper = gtk_flipper_new (img);
  GtkWidget * lbl = NULL, *lbl_flipper = NULL;
  if (self->with_text)
    {
      lbl = gtk_label_new (tab_label);
      lbl_flipper = gtk_flipper_new (lbl);
    }

  switch (self->pos_in_paned)
    {
    case GTK_POS_LEFT:
      if (self->with_text)
        {
          gtk_box_append (box, lbl_flipper);
          gtk_box_append (box, img_flipper);
          gtk_flipper_set_rotate (
            GTK_FLIPPER (lbl_flipper), true);
          gtk_flipper_set_flip_vertical (
            GTK_FLIPPER (lbl_flipper), true);
          gtk_flipper_set_flip_horizontal (
            GTK_FLIPPER (lbl_flipper), true);
          gtk_flipper_set_rotate (
            GTK_FLIPPER (img_flipper), true);
          gtk_flipper_set_flip_vertical (
            GTK_FLIPPER (img_flipper), true);
          gtk_flipper_set_flip_horizontal (
            GTK_FLIPPER (img_flipper), true);
        }
      else
        {
          gtk_box_append (box, img_flipper);
        }
      break;
    case GTK_POS_RIGHT:
      if (self->with_text)
        {
          gtk_box_append (box, img_flipper);
          gtk_box_append (box, lbl_flipper);
          gtk_flipper_set_rotate (
            GTK_FLIPPER (lbl_flipper), true);
          gtk_flipper_set_rotate (
            GTK_FLIPPER (img_flipper), true);
        }
      else
        {
          gtk_box_append (box, img_flipper);
        }
      break;
    case GTK_POS_BOTTOM:
    case GTK_POS_TOP:
      gtk_box_append (box, img_flipper);
      if (self->with_text)
        {
          gtk_box_append (box, lbl_flipper);
        }
      break;
    }

  gtk_notebook_append_page (
    notebook, child, GTK_WIDGET (box));
  gtk_notebook_set_tab_detachable (
    notebook, child, true);
  gtk_notebook_set_tab_reorderable (
    notebook, child, true);
}

FoldableNotebookWidget *
foldable_notebook_widget_new (
  GtkPositionType pos_in_paned,
  bool            with_text)
{
  FoldableNotebookWidget * self = g_object_new (
    FOLDABLE_NOTEBOOK_WIDGET_TYPE, NULL);

  self->pos_in_paned = pos_in_paned;
  self->with_text = with_text;

  return self;
}

static void
foldable_notebook_widget_finalize (
  FoldableNotebookWidget * self)
{
  G_OBJECT_CLASS (
    foldable_notebook_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

/**
 * Sets up an existing FoldableNotebookWidget.
 */
void
foldable_notebook_widget_setup (
  FoldableNotebookWidget * self,
  GtkPaned *               paned,
  GtkPositionType          pos_in_paned,
  bool                     with_text)
{
  self->paned = paned;
  self->pos_in_paned = pos_in_paned;
  self->with_text = with_text;

  /* make detachable */
  z_gtk_notebook_make_detachable (
    GTK_NOTEBOOK (self->notebook),
    GTK_WINDOW (MAIN_WINDOW));

  /* add signals */
  self->mp =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->mp),
    GTK_PHASE_CAPTURE);
  g_signal_connect (
    G_OBJECT (self->mp), "pressed",
    G_CALLBACK (on_multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (self->mp), "released",
    G_CALLBACK (on_multipress_released), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->notebook),
    GTK_EVENT_CONTROLLER (self->mp));

  gtk_notebook_set_tab_pos (
    self->notebook, pos_in_paned);

  g_return_if_fail (
    self->switch_page_handler_id == 0);
  self->switch_page_handler_id = g_signal_connect (
    G_OBJECT (self->notebook), "switch-page",
    G_CALLBACK (on_switch_page), self);
}

void
foldable_notebook_widget_set_current_page (
  FoldableNotebookWidget * self,
  int                      page_num,
  bool                     block_signals)
{
  if (block_signals && self->switch_page_handler_id)
    g_signal_handler_block (
      self->notebook, self->switch_page_handler_id);

  gtk_notebook_set_current_page (
    self->notebook, page_num);

  if (block_signals && self->switch_page_handler_id)
    g_signal_handler_unblock (
      self->notebook, self->switch_page_handler_id);
}

int
foldable_notebook_widget_get_current_page (
  FoldableNotebookWidget * self)
{
  return gtk_notebook_get_current_page (
    self->notebook);
}

static void
foldable_notebook_widget_class_init (
  FoldableNotebookWidgetClass * klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc)
    foldable_notebook_widget_finalize;
}

static void
foldable_notebook_widget_init (
  FoldableNotebookWidget * self)
{
  self->notebook =
    GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_box_append (
    GTK_BOX (self), GTK_WIDGET (self->notebook));
}
