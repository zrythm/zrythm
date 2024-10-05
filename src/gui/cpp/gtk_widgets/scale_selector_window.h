// SPDX-FileCopyrightText: © 2019, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * MusicalScale selector popover.
 */

#ifndef __GUI_WIDGETS_SCALE_SELECTOR_WINDOW_H__
#define __GUI_WIDGETS_SCALE_SELECTOR_WINDOW_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include "common/dsp/scale.h"
#include "common/utils/types.h"

#define SCALE_SELECTOR_WINDOW_WIDGET_TYPE \
  (scale_selector_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ScaleSelectorWindowWidget,
  scale_selector_window_widget,
  Z,
  SCALE_SELECTOR_WINDOW_WIDGET,
  GtkDialog)

/**
 * @addtogroup widgets
 *
 * @{
 */

class ScaleObject;
class MusicalScale;

/**
 * A GtkPopover to create a ScaleDescriptor for use
 * in the ScaleTrack's ScaleObject's.
 */
using ScaleSelectorWindowWidget = struct _ScaleSelectorWindowWidget
{
  GtkDialog parent_instance;

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

  GtkFlowBox * creator_type_flowbox;
  GtkFlowBox * creator_type_other_flowbox;

  /** All of the above in an array. */
  GtkFlowBoxChild * creator_types[ENUM_COUNT (MusicalScale::Type)];

  /** The owner ScaleObjectWidget. */
  ScaleObject * scale;

  /** The descriptor of the edited scale, so
   * it can be used to save into the ScaleObject. */
  std::unique_ptr<MusicalScale> * descr;
};

/**
 * Creates the popover.
 */
ScaleSelectorWindowWidget *
scale_selector_window_widget_new (ScaleObject * owner);

/**
 * @}
 */

#endif
