// SPDX-FileCopyrightText: © 2018-2019, 2021-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Piano roll widget.
 */

#ifndef __GUI_WIDGETS_MIDI_EDITOR_SPACE_H__
#define __GUI_WIDGETS_MIDI_EDITOR_SPACE_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#define MIDI_EDITOR_SPACE_WIDGET_TYPE (midi_editor_space_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MidiEditorSpaceWidget,
  midi_editor_space_widget,
  Z,
  MIDI_EDITOR_SPACE_WIDGET,
  GtkWidget);

TYPEDEF_STRUCT_UNDERSCORED (ArrangerWrapperWidget);
TYPEDEF_STRUCT_UNDERSCORED (PianoRollKeysWidget);
TYPEDEF_STRUCT_UNDERSCORED (VelocitySettingsWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_MIDI_EDITOR_SPACE MW_CLIP_EDITOR_INNER->midi_editor_space

/**
 * The piano roll widget is the whole space inside
 * the clip editor tab when a MIDI region is selected.
 */
typedef struct _MidiEditorSpaceWidget
{
  GtkWidget parent_instance;

  GtkPaned * midi_arranger_velocity_paned;

  GtkScrolledWindow * piano_roll_keys_scroll;
  GtkViewport *       piano_roll_keys_viewport;

  GtkBox * midi_notes_box;

  PianoRollKeysWidget * piano_roll_keys;

  /** Piano roll. */
  GtkBox *                midi_arranger_box;
  ArrangerWrapperWidget * arranger_wrapper;
  ArrangerWidget *        modifier_arranger;

  VelocitySettingsWidget * velocity_settings;
  GtkBox *                 midi_vel_chooser_box;
  GtkComboBoxText *        midi_modifier_chooser;

  /** Vertical size goup for the keys and the
   * arranger. */
  GtkSizeGroup * arranger_and_keys_vsize_group;

  /**
   * Whether we scrolled to the middle already (should happen
   * once per run).
   */
  bool scrolled_on_first_refresh;
} MidiEditorSpaceWidget;

void
midi_editor_space_widget_setup (MidiEditorSpaceWidget * self);

/**
 * See CLIP_EDITOR_INNER_WIDGET_ADD_TO_SIZEGROUP.
 */
void
midi_editor_space_widget_update_size_group (
  MidiEditorSpaceWidget * self,
  int                     visible);

/**
 * Updates the scroll adjustment.
 */
void
midi_editor_space_widget_set_piano_keys_scroll_start_y (
  MidiEditorSpaceWidget * self,
  int                     y);

/**
 * To be used as a source func when first showing a MIDI
 * region.
 */
gboolean
midi_editor_space_widget_scroll_to_middle (MidiEditorSpaceWidget * self);

void
midi_editor_space_widget_refresh (MidiEditorSpaceWidget * self);

/**
 * @}
 */

#endif
