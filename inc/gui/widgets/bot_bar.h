/*
 * gui/widgets/bot_bar.h - Bottom bar
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __GUI_WIDGETS_BOT_BAR_H__
#define __GUI_WIDGETS_BOT_BAR_H__

#include <gtk/gtk.h>

#define BOT_BAR_WIDGET_TYPE \
  (bot_bar_widget_get_type ())
G_DECLARE_FINAL_TYPE (BotBarWidget,
                      bot_bar_widget,
                      Z,
                      BOT_BAR_WIDGET,
                      GtkBox)

#define BOT_BAR MW->bot_bar


typedef struct _BotBarWidget
{
  GtkBox                    parent_instance;
  GtkToolbar *              bot_bar_left;
} BotBarWidget;

void
bot_bar_widget_refresh (BotBarWidget * self);

#endif
