// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Preferences widget.
 */

#ifndef __GUI_WIDGETS_PREFERENCES_H__
#define __GUI_WIDGETS_PREFERENCES_H__

#include <adwaita.h>

#define PREFERENCES_WIDGET_TYPE (preferences_widget_get_type ())
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

  /** Remembered for toggling visibility. */
  AdwPreferencesRow * audio_backend_rtaudio_device_row;
  AdwPreferencesRow * audio_backend_buffer_size_row;
  AdwPreferencesRow * audio_backend_sample_rate_row;
} PreferencesWidget;

PreferencesWidget *
preferences_widget_new (void);

/**
 * @}
 */

#endif
