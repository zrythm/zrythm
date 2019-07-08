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

/**
 * \file
 *
 * Donate dialog.
 */

#include "gui/widgets/donate_dialog.h"
#include "project.h"
#include "utils/io.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (DonateDialogWidget,
               donate_dialog_widget,
               GTK_TYPE_DIALOG)

static void
on_clicked (
  GtkButton * btn,
  DonateDialogWidget * self)
{
  if (btn == self->bitcoin)
    {
      gtk_label_set_text (
        self->link,
        "Address: bc1qjfyu2ruyfwv3r6u4hf2nvdh900djep2dlk746j");
    }
  else if (btn == self->paypal)
    {
      gtk_label_set_markup (
        self->link,
        "<a href=\"https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&amp;hosted_button_id=LZWVK6228PQGE&amp;source=url\">https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&amp;hosted_button_id=LZWVK6228PQGE&amp;source=url</a>");
    }
  else if (btn == self->liberapay)
    {
      gtk_label_set_markup (
        self->link,
        "<a href=\"https://liberapay.com/Zrythm/donate\">https://liberapay.com/Zrythm/donate</a>");
    }
}

/**
 * Creates a new donate dialog.
 */
DonateDialogWidget *
donate_dialog_widget_new ()
{
  DonateDialogWidget * self =
    g_object_new (DONATE_DIALOG_WIDGET_TYPE, NULL);

  return self;
}

static void
donate_dialog_widget_class_init (
  DonateDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "donate_dialog.ui");

  gtk_widget_class_bind_template_child (
    klass,
    DonateDialogWidget,
    bitcoin);
  gtk_widget_class_bind_template_child (
    klass,
    DonateDialogWidget,
    paypal);
  gtk_widget_class_bind_template_child (
    klass,
    DonateDialogWidget,
    liberapay);
  gtk_widget_class_bind_template_child (
    klass,
    DonateDialogWidget,
    link);
}

static void
donate_dialog_widget_init (DonateDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  GdkPixbuf * pixbuf;
  GtkWidget * img;
  GtkStyleContext * context;

#define ADD_IMG(basename, obj) \
  pixbuf = \
    gdk_pixbuf_new_from_resource_at_scale ( \
      RESOURCE_PATH ICON_PATH "ext/" basename, \
      200, 200, TRUE, NULL); \
  img = gtk_image_new_from_pixbuf (pixbuf); \
  gtk_widget_set_visible (img, 1); \
  gtk_container_add ( \
    GTK_CONTAINER (obj), img); \
  context = \
    gtk_widget_get_style_context ( \
      GTK_WIDGET (obj)); \
  gtk_style_context_add_class ( \
    context, "donate-button");

  ADD_IMG ("Bitcoin_logo.svg", self->bitcoin);
  ADD_IMG ("pp-logo-200px.png", self->paypal);
  ADD_IMG ("Liberapay_logo_v2_white-on-yellow.svg", self->liberapay);

#undef ADD_IMG

  g_signal_connect (
    self->bitcoin, "clicked",
    G_CALLBACK (on_clicked), self);
  g_signal_connect (
    self->paypal, "clicked",
    G_CALLBACK (on_clicked), self);
  g_signal_connect (
    self->liberapay, "clicked",
    G_CALLBACK (on_clicked), self);
}
