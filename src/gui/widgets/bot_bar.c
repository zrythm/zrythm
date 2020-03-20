/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Bottomest widget holding a status bar.
 */

#include "audio/engine.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/ui.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (BotBarWidget,
               bot_bar_widget,
               GTK_TYPE_BOX)

void
bot_bar_widget_refresh (BotBarWidget * self)
{

}

static void
on_text_pushed (
  GtkStatusbar *statusbar,
  guint         context_id,
  gchar        *text,
  BotBarWidget * self)
{
  gtk_label_set_markup (
    self->label, text);
}

/**
 * Updates the content of the status bar.
 */
void
bot_bar_widget_update_status (
  BotBarWidget * self)
{
  gtk_statusbar_remove_all (
    MW_STATUS_BAR,
    MW_BOT_BAR->context_id);

  /*const char * orange = UI_COLOR_BRIGHT_ORANGE;*/
#define ORANGIZE(x) \
  "<span " \
  "foreground=\"" UI_COLOR_BRIGHT_ORANGE "\">" x "</span>"

  char str[400];
  sprintf (
    str,
    "%s: " ORANGIZE ("%s") " | "
    "%s: " ORANGIZE ("%s") " | "
    "%s: " ORANGIZE ("%d frames") " | "
    "%s: " ORANGIZE ("%d Hz"),
    _("Audio backend"),
    engine_audio_backend_to_string (
      AUDIO_ENGINE->audio_backend),
    _("MIDI backend"),
    engine_midi_backend_to_string (
      AUDIO_ENGINE->midi_backend),
    _("Audio buffer size"),
    AUDIO_ENGINE->block_length,
    _("Sample rate"),
    AUDIO_ENGINE->sample_rate);

#undef ORANGIZE

  gtk_statusbar_push (MW_STATUS_BAR,
                      MW_BOT_BAR->context_id,
                      str);
}

static gboolean
tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  BotBarWidget *  self)
{
  bot_bar_widget_update_status (self);

  return G_SOURCE_CONTINUE;
}

/**
 * Sets up the bot bar.
 */
void
bot_bar_widget_setup (
  BotBarWidget * self)
{
  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) tick_cb,
    self, NULL);
}

static void
bot_bar_widget_class_init (
  BotBarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "bot_bar.ui");

  gtk_widget_class_set_css_name (klass,
                                 "bot-bar");

  gtk_widget_class_bind_template_child (
    klass,
    BotBarWidget,
    status_bar);
}

static void
bot_bar_widget_init (BotBarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->context_id =
    gtk_statusbar_get_context_id (
      self->status_bar,
      "Main context");

  self->label =
    GTK_LABEL (gtk_label_new (""));
  gtk_label_set_markup (self->label, "");
  gtk_widget_set_visible (
    GTK_WIDGET (self->label), 1);
  GtkWidget * box =
    gtk_statusbar_get_message_area (
      self->status_bar);
  z_gtk_container_remove_all_children (
    GTK_CONTAINER (box));
  gtk_container_add (
    GTK_CONTAINER (box),
    GTK_WIDGET (self->label));

  g_signal_connect (
    self->status_bar, "text-pushed",
    G_CALLBACK (on_text_pushed),
    self);
}

