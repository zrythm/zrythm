// clang-format off
// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 *
 * ChordDescriptor selector popover.
 */

#ifndef __GUI_WIDGETS_CHORD_SELECTOR_WINDOW_H__
#define __GUI_WIDGETS_CHORD_SELECTOR_WINDOW_H__

#include <adwaita.h>

#define CHORD_SELECTOR_WINDOW_WIDGET_TYPE \
  (chord_selector_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChordSelectorWindowWidget,
  chord_selector_window_widget,
  Z,
  CHORD_SELECTOR_WINDOW_WIDGET,
  AdwDialog)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct ChordObject ChordObject;
typedef struct ScaleObject ScaleObject;

/**
 * A GtkPopover to create a ChordDescriptor for use in the ChordTrack's
 * ChordObject's.
 */
typedef struct _ChordSelectorWindowWidget
{
  AdwDialog parent_instance;

  GtkNotebook * notebook;

  GtkFlowBox *      diatonic_flowbox;
  GtkFlowBoxChild * diatonic_i;
  GtkFlowBoxChild * diatonic_ii;
  GtkFlowBoxChild * diatonic_iii;
  GtkFlowBoxChild * diatonic_iv;
  GtkFlowBoxChild * diatonic_v;
  GtkFlowBoxChild * diatonic_vi;
  GtkFlowBoxChild * diatonic_vii;
  GtkLabel *        diatonic_i_lbl;
  GtkLabel *        diatonic_ii_lbl;
  GtkLabel *        diatonic_iii_lbl;
  GtkLabel *        diatonic_iv_lbl;
  GtkLabel *        diatonic_v_lbl;
  GtkLabel *        diatonic_vi_lbl;
  GtkLabel *        diatonic_vii_lbl;
  GtkFlowBox *      creator_root_note_flowbox;
  GtkFlowBoxChild * creator_root_note_c;
  GtkFlowBoxChild * creator_root_note_cs;
  GtkFlowBoxChild * creator_root_note_d;
  GtkFlowBoxChild * creator_root_note_ds;
  GtkFlowBoxChild * creator_root_note_e;
  GtkFlowBoxChild * creator_root_note_f;
  GtkFlowBoxChild * creator_root_note_fs;
  GtkFlowBoxChild * creator_root_note_g;
  GtkFlowBoxChild * creator_root_note_gs;
  GtkFlowBoxChild * creator_root_note_a;
  GtkFlowBoxChild * creator_root_note_as;
  GtkFlowBoxChild * creator_root_note_b;

  /** All of the above in an array. */
  GtkFlowBoxChild * creator_root_notes[12];

  GtkFlowBox *      creator_type_flowbox;
  GtkFlowBoxChild * creator_type_maj;
  GtkFlowBoxChild * creator_type_min;
  GtkFlowBoxChild * creator_type_dim;
  GtkFlowBoxChild * creator_type_sus4;
  GtkFlowBoxChild * creator_type_sus2;
  GtkFlowBoxChild * creator_type_aug;

  /** All of the above in an array. */
  GtkFlowBoxChild * creator_types[6];

  GtkFlowBox *      creator_accent_flowbox;
  GtkFlowBoxChild * creator_accent_7;
  GtkFlowBoxChild * creator_accent_j7;
  GtkFlowBoxChild * creator_accent_b9;
  GtkFlowBoxChild * creator_accent_9;
  GtkFlowBoxChild * creator_accent_s9;
  GtkFlowBoxChild * creator_accent_11;
  GtkFlowBoxChild * creator_accent_b5_s11;
  GtkFlowBoxChild * creator_accent_s5_b13;
  GtkFlowBoxChild * creator_accent_6_13;

  /** All of the above in an array. */
  GtkFlowBoxChild * creator_accents[9];

  GtkFlowBox *      creator_bass_note_flowbox;
  GtkFlowBoxChild * creator_bass_note_c;
  GtkFlowBoxChild * creator_bass_note_cs;
  GtkFlowBoxChild * creator_bass_note_d;
  GtkFlowBoxChild * creator_bass_note_ds;
  GtkFlowBoxChild * creator_bass_note_e;
  GtkFlowBoxChild * creator_bass_note_f;
  GtkFlowBoxChild * creator_bass_note_fs;
  GtkFlowBoxChild * creator_bass_note_g;
  GtkFlowBoxChild * creator_bass_note_gs;
  GtkFlowBoxChild * creator_bass_note_a;
  GtkFlowBoxChild * creator_bass_note_as;
  GtkFlowBoxChild * creator_bass_note_b;

  /** All of the above in an array. */
  GtkFlowBoxChild * creator_bass_notes[12];

  GtkCheckButton * creator_visibility_all;
  GtkCheckButton * creator_visibility_in_scale;

  /** ScaleObject at the chord's position. */
  ScaleObject * scale;

  /** Temporary copy of the chord descriptor. */
  ChordDescriptor * descr_clone;

  /** The index of the chord in the chord editor. */
  int chord_idx;

} ChordSelectorWindowWidget;

/**
 * Creates the popover.
 *
 * @param parent The parent widget to show the dialog on.
 */
void
chord_selector_window_widget_present (const int chord_idx, GtkWidget * parent);

/**
 * @}
 */

#endif
