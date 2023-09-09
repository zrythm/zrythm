// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Dialog for viewing/editing port info.
 */

#ifndef __GUI_WIDGETS_DIALOGS_MIDI_FUNCTION_H__
#define __GUI_WIDGETS_DIALOGS_MIDI_FUNCTION_H__

#include "dsp/midi_function.h"

#include <adwaita.h>

#define MIDI_FUNCTION_DIALOG_WIDGET_TYPE \
  (midi_function_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MidiFunctionDialogWidget,
  midi_function_dialog_widget,
  Z,
  MIDI_FUNCTION_DIALOG_WIDGET,
  AdwWindow)

typedef struct ArrangerObject ArrangerObject;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * The midi_function dialog.
 */
typedef struct _MidiFunctionDialogWidget
{
  AdwWindow parent_instance;

  /** Box for header/content. */
  GtkBox * vbox;

  AdwHeaderBar * header;

  /** Main content. */
  AdwPreferencesPage * page;

  GtkButton * ok_btn;
  GtkButton * cancel_btn;

  MidiFunctionType type;

  GSettings * settings;
} MidiFunctionDialogWidget;

/**
 * Fills in \ref opts with the current options in the dialog
 * (fetched from gsettings).
 */
void
midi_function_dialog_widget_get_opts (
  MidiFunctionDialogWidget * self,
  MidiFunctionOpts *         opts);

/**
 * Creates a MIDI function dialog for the given type and
 * pre-fills the values from GSettings.
 */
MidiFunctionDialogWidget *
midi_function_dialog_widget_new (GtkWindow * parent, MidiFunctionType type);

/**
 * @}
 */

#endif
