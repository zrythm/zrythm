/*
 * SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifndef __GUI_WIDGETS_PREROLL_COUNT_SELECTOR_H__
#define __GUI_WIDGETS_PREROLL_COUNT_SELECTOR_H__

/**
 * @file
 *
 * Bounce step selector.
 */

#include "gtk_wrapper.h"

#define PREROLL_COUNT_SELECTOR_WIDGET_TYPE \
  (preroll_count_selector_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PrerollCountSelectorWidget,
  preroll_count_selector_widget,
  Z,
  PREROLL_COUNT_SELECTOR_WIDGET,
  GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

enum class PrerollCountSelectorType
{
  /** Countin before starting playback. */
  PREROLL_COUNT_SELECTOR_METRONOME_COUNTIN,

  /** Preroll to start at before the punch in
   * position during recording. */
  PREROLL_COUNT_SELECTOR_RECORD_PREROLL,
};

typedef struct _PrerollCountSelectorWidget
{
  GtkBox parent_instance;

  PrerollCountSelectorType type;

  GtkToggleButton * none_toggle;
  gulong            none_toggle_id;
  GtkToggleButton * one_bar_toggle;
  gulong            one_bar_toggle_id;
  GtkToggleButton * two_bars_toggle;
  gulong            two_bars_toggle_id;
  GtkToggleButton * four_bars_toggle;
  gulong            four_bars_toggle_id;

} PrerollCountSelectorWidget;

/**
 * Creates a PrerollCountSelectorWidget.
 */
PrerollCountSelectorWidget *
preroll_count_selector_widget_new (PrerollCountSelectorType type);

/**
 * @}
 */

#endif
