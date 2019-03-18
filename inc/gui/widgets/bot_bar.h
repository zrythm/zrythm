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
 * Bottomest bar.
 */

#ifndef __GUI_WIDGETS_BOT_BAR_H__
#define __GUI_WIDGETS_BOT_BAR_H__

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define BOT_BAR_WIDGET_TYPE \
  (bot_bar_widget_get_type ())
G_DECLARE_FINAL_TYPE (BotBarWidget,
                      bot_bar_widget,
                      Z,
                      BOT_BAR_WIDGET,
                      GtkBox)

#define MW_BOT_BAR MW->bot_bar
#define MW_STATUS_BAR MW_BOT_BAR->status_bar

typedef struct _BotBarWidget
{
  GtkBox                parent_instance;
  GtkStatusbar *        status_bar;

  /** Status bar context id. */
  guint                 context_id;
} BotBarWidget;

void
bot_bar_widget_refresh (BotBarWidget * self);

/**
 * Changes the message shown in the status bar.
 */
void
bot_bar_change_status (const char * message);

/**
 * Returns if the bot bar contains the given
 * substring.
 */
int
bot_bar_status_contains (const char * substr);

/**
 * @}
 */

#endif
