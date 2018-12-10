/*
 * gui/widgets/mixer.h - Mixer widget
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

#ifndef __GUI_WIDGETS_MIXER_H__
#define __GUI_WIDGETS_MIXER_H__

#include "gui/widgets/main_window.h"

#include <gtk/gtk.h>

#define MIXER_WIDGET_TYPE                  (mixer_widget_get_type ())
G_DECLARE_FINAL_TYPE (MixerWidget,
                      mixer_widget,
                      MIXER,
                      WIDGET,
                      GtkBox)

#define MW_MIXER MW_BOT_DOCK_EDGE->mixer

typedef struct DragDestBoxWidget DragDestBoxWidget;

typedef struct _MixerWidget
{
  GtkBox                   parent_instance;
  DragDestBoxWidget *      ddbox;  ///< box for DNDing plugins
  MixerWidget *            mixer;
  GtkScrolledWindow *      channels_scroll;
  GtkViewport *            channels_viewport;
  GtkBox *                 channels_box;
  GtkButton *              channels_add;
} MixerWidget;

MixerWidget *
mixer_widget_new ();

/**
 * Adds channel to mixer widget.
 */
void
mixer_widget_add_channel (MixerWidget * self, Channel * channel);

/**
 * Removes channel from mixer widget.
 */
void
mixer_widget_remove_channel (Channel * channel);

#endif
