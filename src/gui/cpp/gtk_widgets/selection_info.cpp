/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "gui/cpp/gtk_widgets/selection_info.h"
#include "utils/gtk.h"

G_DEFINE_TYPE_WITH_PRIVATE (
  SelectionInfoWidget,
  selection_info_widget,
  GTK_TYPE_GRID)

SelectionInfoWidgetPrivate *
selection_info_widget_get_private (SelectionInfoWidget * self)
{
  return static_cast<SelectionInfoWidgetPrivate *> (
    selection_info_widget_get_instance_private (self));
}

/**
 * Adds a piece of info to the grid.
 *
 * @param label The label widget to place on top.
 *
 *   Usually this will be GtkLabel.
 * @param widget The Widget to place on the bot that
 *   will reflect the information.
 */
void
selection_info_widget_add_info (
  SelectionInfoWidget * self,
  GtkWidget *           label,
  GtkWidget *           widget)
{
  SELECTION_INFO_WIDGET_GET_PRIVATE (self);

  /* x pos to attach to */
  int l;
  if (sel_inf_prv->num_items == 0)
    l = 0;
  else
    l = (sel_inf_prv->num_items - 1) * 2 + 1;

  /* add separator if not 0 */
  if (l > 0)
    {
      GtkWidget * separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
      gtk_widget_set_visible (separator, 1);
      gtk_grid_attach (GTK_GRID (self), separator, l - 1, 0, 1, 2);
    }

  if (label)
    {
      gtk_grid_attach (GTK_GRID (self), label, l, 0, 1, 1);
    }
  gtk_grid_attach (GTK_GRID (self), widget, l, 1, 1, 1);
  gtk_widget_set_vexpand (widget, 1);
  /*g_object_set (*/
  /*G_OBJECT (widget),*/
  /*"height-request", 32);*/

  sel_inf_prv->num_items++;
}

/**
 * Destroys all children.
 */
void
selection_info_widget_clear (SelectionInfoWidget * self)
{
  z_gtk_widget_destroy_all_children (GTK_WIDGET (self));
}

static void
selection_info_widget_class_init (SelectionInfoWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "selection-info");
}

static void
selection_info_widget_init (SelectionInfoWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self), 1);
  gtk_grid_set_column_spacing (GTK_GRID (self), 4);
}
