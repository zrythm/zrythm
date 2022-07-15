/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"

#include "audio/engine.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/custom_image.h"
#include "gui/widgets/splash.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  SplashWindowWidget,
  splash_window_widget,
  GTK_TYPE_WINDOW)

static gboolean
splash_tick_cb (
  GtkWidget *          widget,
  GdkFrameClock *      frame_clock,
  SplashWindowWidget * self)
{
  zix_sem_wait (&zrythm_app->progress_status_lock);
  gtk_label_set_text (self->status_label, zrythm_app->status);
  gtk_progress_bar_set_fraction (
    self->progress_bar, ZRYTHM->progress);
  zix_sem_post (&zrythm_app->progress_status_lock);

  return G_SOURCE_CONTINUE;
}

void
splash_window_widget_close (SplashWindowWidget * self)
{
  g_debug ("closing splash window");

  gtk_widget_remove_tick_callback (
    GTK_WIDGET (self), self->tick_cb_id);
  gtk_window_close (GTK_WINDOW (self));

  EVENTS_PUSH (ET_SPLASH_CLOSED, NULL);
}

static void
finalize (SplashWindowWidget * self)
{
  g_debug ("finalizing splash screen");

  if (zrythm_app->init_thread)
    {
      g_thread_join (zrythm_app->init_thread);
      zrythm_app->init_thread = NULL;
    }

  G_OBJECT_CLASS (splash_window_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

SplashWindowWidget *
splash_window_widget_new (ZrythmApp * app)
{
  SplashWindowWidget * self = g_object_new (
    SPLASH_WINDOW_WIDGET_TYPE, "application",
    G_APPLICATION (app), "title", PROGRAM_NAME, NULL);
  g_return_val_if_fail (
    Z_IS_SPLASH_WINDOW_WIDGET (self), NULL);

  gtk_progress_bar_set_fraction (self->progress_bar, 0.0);

  self->tick_cb_id = gtk_widget_add_tick_callback (
    (GtkWidget *) self, (GtkTickCallback) splash_tick_cb,
    self, NULL);

  return self;
}

static void
splash_window_widget_class_init (
  SplashWindowWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "splash.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, SplashWindowWidget, x)

  BIND_CHILD (img);
  BIND_CHILD (status_label);
  BIND_CHILD (version_label);
  BIND_CHILD (progress_bar);

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
splash_window_widget_init (SplashWindowWidget * self)
{
  g_type_ensure (CUSTOM_IMAGE_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  GtkStyleContext * context =
    gtk_widget_get_style_context (GTK_WIDGET (self));
  gtk_style_context_add_class (context, "splash");

  GdkTexture * texture = z_gdk_texture_new_from_icon_name (
    "zrythm-splash-png", 580, -1, 1);
  custom_image_widget_set_texture (self->img, texture);

  char * ver = zrythm_get_version (true);
  char   ver_str[800];
  sprintf (ver_str, "<small>%s</small>", ver);
  g_free (ver);
  gtk_label_set_markup (self->version_label, ver_str);
}
