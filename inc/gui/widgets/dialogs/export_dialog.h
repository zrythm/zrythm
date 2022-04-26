// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Export dialog.
 */

#ifndef __GUI_WIDGETS_EXPORT_DIALOG_H__
#define __GUI_WIDGETS_EXPORT_DIALOG_H__

#include <adwaita.h>
#include <gtk/gtk.h>

#define EXPORT_DIALOG_WIDGET_TYPE \
  (export_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ExportDialogWidget,
  export_dialog_widget,
  Z,
  EXPORT_DIALOG_WIDGET,
  GtkDialog)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef enum ExportFilenamePattern
{
  EFP_APPEND_FORMAT,
  EFP_PREPEND_DATE_APPEND_FORMAT,
} ExportFilenamePattern;

/**
 * The export dialog.
 */
typedef struct _ExportDialogWidget
{
  GtkDialog parent_instance;

  AdwViewSwitcherTitle * title;
  AdwViewStack *         stack;

  /* audio */
  AdwEntryRow * audio_title;
  AdwEntryRow * audio_artist;
  AdwEntryRow * audio_genre;
  AdwComboRow * audio_format;
  AdwComboRow * audio_bit_depth;
  GtkSwitch *   audio_dither_switch;
  AdwComboRow * audio_filename_pattern;
  AdwComboRow * audio_mixdown_or_stems;
  GtkDropDown * audio_time_range_drop_down;
  GtkTreeView * audio_tracks_treeview;
  GtkLabel *    audio_output_label;

  /* MIDI */
  AdwEntryRow * midi_title;
  AdwEntryRow * midi_artist;
  AdwEntryRow * midi_genre;
  AdwComboRow * midi_format;
  GtkSwitch *   midi_export_lanes_as_tracks_switch;
  AdwComboRow * midi_filename_pattern;
  AdwComboRow * midi_mixdown_or_stems;
  GtkDropDown * midi_time_range_drop_down;
  GtkTreeView * midi_tracks_treeview;
  GtkLabel *    midi_output_label;

#if 0
  GtkEntry *        export_artist;
  GtkEntry *        export_title;
  GtkEntry *        export_genre;
  GtkComboBox *     filename_pattern;
  GtkToggleButton * time_range_song;
  GtkToggleButton * time_range_loop;
  GtkToggleButton * time_range_custom;
  GtkComboBox *     format;
  GtkComboBox *     bit_depth;
  GtkCheckButton *  lanes_as_tracks;
  GtkToggleButton * dither;

  GtkToggleButton * mixdown_toggle;
  GtkToggleButton * stems_toggle;
#endif
} ExportDialogWidget;

/**
 * Creates an export dialog widget and displays it.
 */
ExportDialogWidget *
export_dialog_widget_new (void);

/**
 * @}
 */

#endif
