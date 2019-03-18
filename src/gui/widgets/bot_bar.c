/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Bottomest widget holding a status bar.
 */

#include "gui/widgets/bot_bar.h"
#include "gui/widgets/main_window.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (BotBarWidget,
               bot_bar_widget,
               GTK_TYPE_BOX)

void
bot_bar_widget_refresh (BotBarWidget * self)
{

}

/**
 * Changes the message shown in the status bar.
 */
void
bot_bar_change_status (const char * message)
{
  gtk_statusbar_remove_all (MW_STATUS_BAR,
                            MW_BOT_BAR->context_id);

  gtk_statusbar_push (MW_STATUS_BAR,
                      MW_BOT_BAR->context_id,
                      message);
}

/**
 * Returns if the bot bar contains the given
 * substring.
 */
int
bot_bar_status_contains (const char * substr)
{
  GtkWidget * box =
    gtk_statusbar_get_message_area (
      MW_STATUS_BAR);

  GList *children, *iter;

  children =
    gtk_container_get_children (
      GTK_CONTAINER (box));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      g_object_ref (GTK_WIDGET (iter->data));
      if (GTK_IS_LABEL (iter->data))
        {
          GtkLabel * lbl = GTK_LABEL (iter->data);
          g_list_free (children);
          return g_str_match_string (
            substr,
            gtk_label_get_text (lbl),
            0);
        }
    }
  g_assert_not_reached ();
  return 0;
}

static void
bot_bar_widget_class_init (BotBarWidgetClass * _klass)
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
    gtk_statusbar_get_context_id (self->status_bar,
                                  "Main context");
}

