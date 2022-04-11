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
 * Fishbowl window
 */

#ifndef __TESTS_HELPERS_FISHBOWL_WINDOW_H__
#define __TESTS_HELPERS_FISHBOWL_WINDOW_H__

#include <gtk/gtk.h>

#define FISHBOWL_WINDOW_WIDGET_TYPE \
  (fishbowl_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FishbowlWindowWidget,
  fishbowl_window_widget,
  Z,
  FISHBOWL_WINDOW_WIDGET,
  GtkWindow);

/**
 * @addtogroup tests
 *
 * @{
 */

typedef struct _FishbowlWindowWidget
{
  GtkWindow         parent_instance;
  GtkToggleButton * changes_allow;
  GtkToggleButton * changes_prevent;
  GtkFishbowl *     bowl;

  /** Storage for count. */
  guint count;
} FishbowlWindowWidget;

/**
 * @}
 */

G_DEFINE_TYPE (
  FishbowlWindowWidget,
  fishbowl_window_widget,
  GTK_TYPE_WINDOW)

static void
prev_button_clicked_cb (
  GtkButton *            btn,
  FishbowlWindowWidget * self)
{
}

static void
next_button_clicked_cb (
  GtkButton *            btn,
  FishbowlWindowWidget * self)
{
}

static GtkWidget *
create_test_label ()
{
  return gtk_label_new ("Test");
}

static int
close_window (FishbowlWindowWidget * win)
{
  gtk_main_quit ();

  return G_SOURCE_REMOVE;
}

FishbowlWindowWidget *
fishbowl_window_widget_new (
  GtkFishCreationFunc creation_func)
{
  FishbowlWindowWidget * self = g_object_new (
    FISHBOWL_WINDOW_WIDGET_TYPE, NULL);

  gtk_fishbowl_set_creation_func (
    self->bowl, creation_func);
  gtk_fishbowl_set_update_delay (
    self->bowl, G_USEC_PER_SEC / 3);

  return self;
}

guint
fishbowl_window_widget_run (
  GtkFishCreationFunc creation_func,
  int                 secs_to_run)
{
  FishbowlWindowWidget * self =
    fishbowl_window_widget_new (creation_func);

  g_timeout_add (
    secs_to_run * 1000, (GSourceFunc) close_window,
    self);

  gtk_widget_show_all (GTK_WIDGET (self));
  gtk_main ();
  guint count = gtk_fishbowl_get_count (self->bowl);

  gtk_widget_destroy (GTK_WIDGET (self));

  return count;
}

guint
fishbowl_window_widget_run_label (int secs_to_run)
{
  return fishbowl_window_widget_run (
    create_test_label, secs_to_run);
}

guint
fishbowl_window_widget_run_button (int secs_to_run)
{
  return fishbowl_window_widget_run (
    create_test_label, secs_to_run);
}

static void
fishbowl_window_widget_init (
  FishbowlWindowWidget * self)
{
  g_type_ensure (GTK_TYPE_FISHBOWL);

  gtk_widget_init_template (GTK_WIDGET (self));

  g_return_if_fail (self->bowl);
}

static void
fishbowl_window_widget_class_init (
  FishbowlWindowWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_template_from_resource (
    GTK_WIDGET_CLASS (klass),
    "/org/zrythm/Zrythm/app/ui/fishbowl_window.ui");

  gtk_widget_class_bind_template_child (
    klass, FishbowlWindowWidget, changes_allow);
  gtk_widget_class_bind_template_child (
    klass, FishbowlWindowWidget, changes_prevent);
  gtk_widget_class_bind_template_child (
    klass, FishbowlWindowWidget, bowl);
  gtk_widget_class_bind_template_callback (
    klass, next_button_clicked_cb);
  gtk_widget_class_bind_template_callback (
    klass, prev_button_clicked_cb);
}

#endif
