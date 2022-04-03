/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Modulator view in the bottom panel.
 */

#ifndef __GUI_WIDGETS_MODULATOR_VIEW_H__
#define __GUI_WIDGETS_MODULATOR_VIEW_H__

#include <gtk/gtk.h>

#define MODULATOR_VIEW_WIDGET_TYPE \
  (modulator_view_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ModulatorViewWidget,
  modulator_view_widget,
  Z,
  MODULATOR_VIEW_WIDGET,
  GtkBox)

typedef struct _ModulatorWidget ModulatorWidget;
typedef struct _ModulatorMacroWidget
  ModulatorMacroWidget;
typedef struct _ColorAreaWidget    ColorAreaWidget;
typedef struct Track               Track;
typedef struct _RotatedLabelWidget RotatedLabelWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_MODULATOR_VIEW \
  MW_BOT_DOCK_EDGE->modulator_view

/**
 * Brings up the ModulatorViewWidget in the notebook.
 */
#define SHOW_MODULATOR_VIEW \
  gtk_notebook_set_current_page ( \
    MW_MODULATOR_VIEW->bot_notebook, 2)

/**
 * The ModulatorViewWidget contains the
 * ModulatorWidgets for the selected Track.
 */
typedef struct _ModulatorViewWidget
{
  GtkBox                 parent_instance;
  GtkBox *               modulators_box;
  GtkBox *               macros_box;
  ColorAreaWidget *      color;
  RotatedLabelWidget *   track_name;
  Track *                track;
  ModulatorWidget *      modulators[14];
  ModulatorMacroWidget * macros[8];
} ModulatorViewWidget;

void
modulator_view_widget_refresh (
  ModulatorViewWidget * self,
  Track *               track);

ModulatorViewWidget *
modulator_view_widget_new (void);

/**
 * @}
 */

#endif
