/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Bounce to audio dialog.
 */

#ifndef __GUI_WIDGETS_BOUNCE_DIALOG_H__
#define __GUI_WIDGETS_BOUNCE_DIALOG_H__

#include "common/dsp/exporter.h"
#include "common/dsp/position.h"
#include "common/utils/types.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#define BOUNCE_DIALOG_WIDGET_TYPE (bounce_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BounceDialogWidget,
  bounce_dialog_widget,
  Z,
  BOUNCE_DIALOG_WIDGET,
  GtkDialog)

TYPEDEF_STRUCT_UNDERSCORED (BounceStepSelectorWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Type of bounce.
 */
enum class BounceDialogWidgetType
{
  BOUNCE_DIALOG_REGIONS,
  BOUNCE_DIALOG_TRACKS,
};

/**
 * The export dialog.
 */
using BounceDialogWidget = struct _BounceDialogWidget
{
  GtkDialog                  parent_instance;
  GtkButton *                cancel_btn;
  GtkButton *                bounce_btn;
  GtkCheckButton *           bounce_with_parents;
  GtkCheckButton *           disable_after_bounce;
  GtkBox *                   bounce_step_box;
  BounceStepSelectorWidget * bounce_step_selector;
  GtkSpinButton *            tail_spin;

  BounceDialogWidgetType type;

  /** Whether to bounce to file instead of on a new
   * audio track (TODO, or make it a gsetting). */
  int bounce_to_file;

  Position start_pos;

  std::string               bounce_name;
  std::shared_ptr<Exporter> exporter;
};

/**
 * Creates a bounce dialog.
 */
BounceDialogWidget *
bounce_dialog_widget_new (
  BounceDialogWidgetType type,
  const std::string     &bounce_name);

/**
 * @}
 */

#endif
