/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Preferences widget.
 */

#ifndef __GUI_WIDGETS_PREFERENCES_H__
#define __GUI_WIDGETS_PREFERENCES_H__

#include <adwaita.h>

#define PREFERENCES_WIDGET_TYPE \
  (preferences_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PreferencesWidget,
  preferences_widget,
  Z,
  PREFERENCES_WIDGET,
  AdwPreferencesWindow)

typedef struct Preferences             Preferences;
typedef struct _MidiControllerMbWidget MidiControllerMbWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_PREFERENCES ZRYTHM->preferences

typedef struct SubgroupInfo
{
  /** Localized name. */
  const char * name;

  const char * group_name;

  const char * group_icon;

  int               group_idx;
  int               subgroup_idx;
  GSettingsSchema * schema;
  GSettings *       settings;
} SubgroupInfo;

/**
 * Preferences widget.
 */
typedef struct _PreferencesWidget
{
  AdwPreferencesWindow parent_instance;
  GtkNotebook *        group_notebook;

  SubgroupInfo subgroup_infos[12][40];
} PreferencesWidget;

PreferencesWidget *
preferences_widget_new (void);

/**
 * @}
 */

#endif
