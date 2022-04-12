// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/expander_box.h"
#include "gui/widgets/gtk_flipper.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE_WITH_PRIVATE (
  ExpanderBoxWidget,
  expander_box_widget,
  GTK_TYPE_BOX)

#define GET_PRIVATE(s) \
  ExpanderBoxWidgetPrivate * prv = \
    expander_box_widget_get_private (s);

static void
on_clicked (
  GtkButton *         button,
  ExpanderBoxWidget * self)
{
  GET_PRIVATE (self);

  bool revealed =
    !gtk_revealer_get_reveal_child (prv->revealer);
  gtk_revealer_set_reveal_child (
    prv->revealer, revealed);
  gtk_widget_set_visible (
    GTK_WIDGET (prv->revealer), revealed);

  if (prv->reveal_cb)
    {
      prv->reveal_cb (
        self, revealed, prv->user_data);
    }
}

/**
 * Gets the private.
 */
ExpanderBoxWidgetPrivate *
expander_box_widget_get_private (
  ExpanderBoxWidget * self)
{
  return expander_box_widget_get_instance_private (
    self);
}

/**
 * Reveals or hides the expander box's contents.
 */
void
expander_box_widget_set_reveal (
  ExpanderBoxWidget * self,
  int                 reveal)
{
  GET_PRIVATE (self);
  gtk_revealer_set_reveal_child (
    prv->revealer, reveal);
  gtk_widget_set_visible (
    GTK_WIDGET (prv->revealer), reveal);
}

/**
 * Sets the label to show.
 */
void
expander_box_widget_set_label (
  ExpanderBoxWidget * self,
  const char *        label)
{
  GET_PRIVATE (self);

  gtk_label_set_text (prv->btn_label, label);
}

void
expander_box_widget_set_orientation (
  ExpanderBoxWidget * self,
  GtkOrientation      orientation)
{
  ExpanderBoxWidgetPrivate * prv =
    expander_box_widget_get_private (self);

  /* set the main orientation */
  prv->orientation = orientation;
  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (self), orientation);

  /* set the orientation of the box inside the
   * expander button */
  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (prv->btn_box),
    orientation == GTK_ORIENTATION_HORIZONTAL
      ? GTK_ORIENTATION_VERTICAL
      : GTK_ORIENTATION_HORIZONTAL);

  /* set the label angle */
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      gtk_flipper_set_flip_horizontal (
        prv->btn_label_flipper, true);
      gtk_flipper_set_flip_vertical (
        prv->btn_label_flipper, true);
      gtk_flipper_set_rotate (
        prv->btn_label_flipper, true);
    }
  else
    {
      gtk_flipper_set_flip_horizontal (
        prv->btn_label_flipper, false);
      gtk_flipper_set_flip_vertical (
        prv->btn_label_flipper, false);
      gtk_flipper_set_rotate (
        prv->btn_label_flipper, false);
    }

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      gtk_widget_set_hexpand (
        GTK_WIDGET (prv->btn_label_flipper), 0);
      gtk_widget_set_vexpand (
        GTK_WIDGET (prv->btn_label_flipper), 1);
    }
  else
    {
      gtk_widget_set_hexpand (
        GTK_WIDGET (prv->btn_label_flipper), 1);
      gtk_widget_set_vexpand (
        GTK_WIDGET (prv->btn_label_flipper), 0);
    }
}

/**
 * Sets the icon name to show.
 */
void
expander_box_widget_set_icon_name (
  ExpanderBoxWidget * self,
  const char *        icon_name)
{
  GET_PRIVATE (self);

  gtk_image_set_from_icon_name (
    prv->btn_img, icon_name);
}

void
expander_box_widget_set_vexpand (
  ExpanderBoxWidget * self,
  bool                expand)
{
  GET_PRIVATE (self);

  gtk_widget_set_vexpand (
    GTK_WIDGET (prv->content), expand);
}

void
expander_box_widget_set_reveal_callback (
  ExpanderBoxWidget *   self,
  ExpanderBoxRevealFunc cb,
  void *                user_data)
{
  GET_PRIVATE (self);

  prv->reveal_cb = cb;
  prv->user_data = user_data;
}

void
expander_box_widget_add_content (
  ExpanderBoxWidget * self,
  GtkWidget *         content)
{
  ExpanderBoxWidgetPrivate * prv =
    expander_box_widget_get_private (self);
  gtk_box_append (prv->content, content);
}

ExpanderBoxWidget *
expander_box_widget_new (
  const char *   label,
  const char *   icon_name,
  GtkOrientation orientation)
{
  ExpanderBoxWidget * self = g_object_new (
    EXPANDER_BOX_WIDGET_TYPE, "visible", 1, NULL);

  expander_box_widget_set_icon_name (
    self, icon_name);
  expander_box_widget_set_orientation (
    self, orientation);
  expander_box_widget_set_label (self, label);

  return self;
}

static void
expander_box_widget_class_init (
  ExpanderBoxWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "expander_box.ui");
  gtk_widget_class_set_css_name (
    klass, "expander-box");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child_private ( \
    klass, ExpanderBoxWidget, x)

  BIND_CHILD (button);
  BIND_CHILD (revealer);
  BIND_CHILD (content);

#undef BIND_CHILD
}

static void
expander_box_widget_init (ExpanderBoxWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  GET_PRIVATE (self);

  prv->btn_label =
    GTK_LABEL (gtk_label_new ("Label"));
  gtk_widget_set_name (
    GTK_WIDGET (prv->btn_label),
    "expander-box-btn-rotated-label");
  prv->btn_label_flipper = GTK_FLIPPER (
    gtk_flipper_new (GTK_WIDGET (prv->btn_label)));
  gtk_widget_set_hexpand (
    GTK_WIDGET (prv->btn_label_flipper), true);
  gtk_widget_set_halign (
    GTK_WIDGET (prv->btn_label_flipper),
    GTK_ALIGN_START);
  prv->btn_img = GTK_IMAGE (
    gtk_image_new_from_icon_name ("plugins"));
  gtk_widget_set_name (
    GTK_WIDGET (prv->btn_img),
    "expander-box-btn-image");
  GtkWidget * box =
    gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_name (
    GTK_WIDGET (box), "expander-box-btn-box");
  gtk_box_append (
    GTK_BOX (box),
    GTK_WIDGET (prv->btn_label_flipper));
  GtkWidget * separator = GTK_WIDGET (
    gtk_separator_new (GTK_ORIENTATION_VERTICAL));
  gtk_widget_add_css_class (separator, "spacer");
  gtk_box_append (GTK_BOX (box), separator);
  gtk_box_append (
    GTK_BOX (box), GTK_WIDGET (prv->btn_img));
  gtk_button_set_child (
    prv->button, GTK_WIDGET (box));
  prv->btn_box = GTK_BOX (box);

  expander_box_widget_set_orientation (
    self, GTK_ORIENTATION_VERTICAL);

  g_signal_connect (
    G_OBJECT (prv->button), "clicked",
    G_CALLBACK (on_clicked), self);
}
