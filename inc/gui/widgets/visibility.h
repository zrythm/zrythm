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

#ifndef __GUI_WIDGETS_VISIBILITY_H__
#define __GUI_WIDGETS_VISIBILITY_H__

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define VISIBILITY_WIDGET_TYPE (visibility_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  VisibilityWidget,
  visibility_widget,
  Z,
  VISIBILITY_WIDGET,
  GtkBox)

#define MW_VISIBILITY MW_LEFT_DOCK_EDGE->visibility

typedef struct _TrackVisibilityTreeWidget
  TrackVisibilityTreeWidget;

typedef struct _VisibilityWidget
{
  GtkBox parent_instance;
  /** For MixerSelections. */
  TrackVisibilityTreeWidget * track_visibility;

} VisibilityWidget;

/**
 * Creates the visibility widget.
 *
 * Only once per project.
 */
VisibilityWidget *
visibility_widget_new (void);

/**
 * Refreshes the visibility widget (shows current
 * selections.
 *
 * Uses Project->last_selection to decide which
 * stack to show.
 */
void
visibility_widget_refresh (VisibilityWidget * self);

/**
 * @}
 */

#endif
