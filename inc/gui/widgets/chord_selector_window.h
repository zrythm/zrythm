/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * ChordDescriptor selector popover.
 */

#ifndef __GUI_WIDGETS_CHORD_SELECTOR_WINDOW_H__
#define __GUI_WIDGETS_CHORD_SELECTOR_WINDOW_H__

#include <gtk/gtk.h>

#define CHORD_SELECTOR_WINDOW_WIDGET_TYPE \
  (chord_selector_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChordSelectorWindowWidget,
  chord_selector_window_widget,
  Z, CHORD_SELECTOR_WINDOW_WIDGET,
  GtkWindow)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct ChordObject ChordObject;

/**
 * A GtkPopover to create a ChordDescriptor for use
 * in the ChordTrack's ChordObject's.
 */
typedef struct _ChordSelectorWindowWidget
{
  GtkWindow         parent_instance;

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
  GtkFlowBoxChild * creator_type_maj;
  GtkFlowBoxChild * creator_type_min;
  GtkFlowBoxChild * creator_type_dim;
  GtkFlowBoxChild * creator_type_sus4;
  GtkFlowBoxChild * creator_type_sus2;
  GtkFlowBoxChild * creator_type_aug;
  GtkFlowBoxChild * creator_accent_7;
  GtkFlowBoxChild * creator_accent_j7;
  GtkFlowBoxChild * creator_accent_b9;
  GtkFlowBoxChild * creator_accent_9;
  GtkFlowBoxChild * creator_accent_s9;
  GtkFlowBoxChild * creator_accent_11;
  GtkFlowBoxChild * creator_accent_b5_s11;
  GtkFlowBoxChild * creator_accent_s5_b13;
  GtkFlowBoxChild * creator_accent_6_13;
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
  GtkRadioButton *  creator_visibility_all;
  GtkRadioButton *  creator_visibility_in_scale;

  /** The owner ChordObjectWidget. */
  ChordObjectWidget * chord;

} ChordSelectorWindowWidget;

/**
 * Creates the popover.
 */
ChordSelectorWindowWidget *
chord_selector_window_widget_new (
  ChordObjectWidget * owner);

/**
 * @}
 */

#endif
