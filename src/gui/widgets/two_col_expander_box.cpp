// SPDX-FileCopyrightText: © 2019, 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/expander_box.h"
#include "gui/widgets/two_col_expander_box.h"
#include "utils/flags.h"
#include "utils/gtk.h"

G_DEFINE_TYPE_WITH_PRIVATE (
  TwoColExpanderBoxWidget,
  two_col_expander_box_widget,
  EXPANDER_BOX_WIDGET_TYPE)

/**
 * Gets the private.
 */
TwoColExpanderBoxWidgetPrivate *
two_col_expander_box_widget_get_private (TwoColExpanderBoxWidget * self)
{
  return static_cast<TwoColExpanderBoxWidgetPrivate *> (
    two_col_expander_box_widget_get_instance_private (self));
}

/**
 * Sets the horizontal spacing.
 */
void
two_col_expander_box_widget_set_horizontal_spacing (
  TwoColExpanderBoxWidget * self,
  int                       horizontal_spacing)
{
  /* TODO */
  g_warn_if_reached ();
}

/**
 * Sets whether to show scrollbars or not.
 */
void
two_col_expander_box_widget_set_scroll_policy (
  TwoColExpanderBoxWidget * self,
  GtkPolicyType             hscrollbar_policy,
  GtkPolicyType             vscrollbar_policy)
{
  TwoColExpanderBoxWidgetPrivate * prv =
    two_col_expander_box_widget_get_private (self);

  gtk_scrolled_window_set_policy (
    prv->scroll, hscrollbar_policy, vscrollbar_policy);
}

/**
 * Sets the max size.
 */
void
two_col_expander_box_widget_set_min_max_size (
  TwoColExpanderBoxWidget * self,
  const int                 min_w,
  const int                 min_h,
  const int                 max_w,
  const int                 max_h)
{
  TwoColExpanderBoxWidgetPrivate * prv =
    two_col_expander_box_widget_get_private (self);

  gtk_scrolled_window_set_min_content_width (prv->scroll, min_w);
  gtk_scrolled_window_set_min_content_height (prv->scroll, min_h);
  gtk_scrolled_window_set_max_content_width (prv->scroll, max_w);
  gtk_scrolled_window_set_max_content_height (prv->scroll, max_h);
  gtk_scrolled_window_set_propagate_natural_height (prv->scroll, 1);
}

/**
 * Gets the content box.
 */
GtkBox *
two_col_expander_box_widget_get_content_box (TwoColExpanderBoxWidget * self)
{
  TwoColExpanderBoxWidgetPrivate * prv =
    two_col_expander_box_widget_get_private (self);

  return prv->content;
}

/**
 * Adds the two widgets in a horizontal box with the
 * given spacing.
 */
void
two_col_expander_box_widget_add_pair (
  TwoColExpanderBoxWidget * self,
  GtkWidget *               widget1,
  GtkWidget *               widget2)
{
  TwoColExpanderBoxWidgetPrivate * prv =
    two_col_expander_box_widget_get_private (self);

  GtkWidget * box =
    gtk_box_new (GTK_ORIENTATION_HORIZONTAL, prv->horizontal_spacing);
  gtk_widget_set_visible (box, 1);

  /* pack the widgets */
  gtk_box_append (GTK_BOX (box), widget1);
  gtk_box_append (GTK_BOX (box), widget2);

  /* pack the box to the original box */
  gtk_box_append (GTK_BOX (prv->content), box);
}

/**
 * Adds a single widget taking up the full horizontal
 * space.
 */
void
two_col_expander_box_widget_add_single (
  TwoColExpanderBoxWidget * self,
  GtkWidget *               widget)
{
  TwoColExpanderBoxWidgetPrivate * prv =
    two_col_expander_box_widget_get_private (self);

  /* pack the widget to the original box */
  gtk_box_append (GTK_BOX (prv->content), widget);
}

/**
 * Removes and destroys the children widgets.
 */
void
two_col_expander_box_widget_remove_children (TwoColExpanderBoxWidget * self)
{
  TwoColExpanderBoxWidgetPrivate * prv =
    two_col_expander_box_widget_get_private (self);

  z_gtk_widget_destroy_all_children (GTK_WIDGET (prv->content));
}

static void
two_col_expander_box_widget_class_init (TwoColExpanderBoxWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "two-col-expander-box");
}

static void
two_col_expander_box_widget_init (TwoColExpanderBoxWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self), 1);

  TwoColExpanderBoxWidgetPrivate * prv =
    two_col_expander_box_widget_get_private (self);

  prv->horizontal_spacing = TWO_COL_EXPANDER_BOX_DEFAULT_HORIZONTAL_SPACING;
  prv->vertical_spacing = TWO_COL_EXPANDER_BOX_DEFAULT_VERTICAL_SPACING;

  /* create content box and add it to the original
   * box */
  prv->scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_scrolled_window_set_policy (
    prv->scroll, GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  prv->content =
    GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, prv->vertical_spacing));
  GtkViewport * viewport = GTK_VIEWPORT (gtk_viewport_new (NULL, NULL));
  gtk_viewport_set_scroll_to_focus (viewport, false);
  gtk_viewport_set_child (viewport, GTK_WIDGET (prv->content));
  gtk_scrolled_window_set_child (prv->scroll, GTK_WIDGET (viewport));

  ExpanderBoxWidgetPrivate * prv_exp_box =
    expander_box_widget_get_private (Z_EXPANDER_BOX_WIDGET (self));
  gtk_box_append (GTK_BOX (prv_exp_box->content), GTK_WIDGET (prv->scroll));
  /*gtk_box_set_child_packing (*/
  /*GTK_BOX (prv_exp_box->content),*/
  /*GTK_WIDGET (prv->scroll), F_EXPAND, F_FILL,*/
  /*0, GTK_PACK_START);*/
}
