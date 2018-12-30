/*
 * gui/widgets/bot_bar.c - Bottom bar
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "gui/widgets/bot_bar.h"

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
    bot_bar_left);
}

static void
bot_bar_widget_init (BotBarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

