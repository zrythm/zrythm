// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Clip editor inner widget.
 */

#ifndef __GUI_WIDGETS_CLIP_EDITOR_INNER_H__
#define __GUI_WIDGETS_CLIP_EDITOR_INNER_H__

#include <gtk/gtk.h>

#define CLIP_EDITOR_INNER_WIDGET_TYPE \
  (clip_editor_inner_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ClipEditorInnerWidget,
  clip_editor_inner_widget,
  Z,
  CLIP_EDITOR_INNER_WIDGET,
  GtkWidget)

typedef struct _RulerWidget            RulerWidget;
typedef struct _ColorAreaWidget        ColorAreaWidget;
typedef struct _MidiEditorSpaceWidget  MidiEditorSpaceWidget;
typedef struct _AudioEditorSpaceWidget AudioEditorSpaceWidget;
typedef struct _ChordEditorSpaceWidget ChordEditorSpaceWidget;
typedef struct _AutomationEditorSpaceWidget
                               AutomationEditorSpaceWidget;
typedef struct _ArrangerWidget ArrangerWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_CLIP_EDITOR_INNER MW_CLIP_EDITOR->clip_editor_inner

/**
 * Adds or remove the widget from the
 * "left of ruler" size group.
 */
void
clip_editor_inner_widget_add_to_left_of_ruler_sizegroup (
  ClipEditorInnerWidget * self,
  GtkWidget *             widget,
  bool                    add);

/**
 * The piano roll widget is the whole space inside
 * the clip editor tab when a MIDI region is selected.
 */
typedef struct _ClipEditorInnerWidget
{
  GtkWidget parent_instance;

  ColorAreaWidget *   color_bar;
  GtkBox *            bot_of_arranger_toolbar;
  GtkLabel *          track_name_lbl;
  GtkBox *            left_of_ruler_box;
  GtkScrolledWindow * ruler_scroll;
  GtkViewport *       ruler_viewport;
  RulerWidget *       ruler;
  GtkStack *          editor_stack;
  GtkSizeGroup *      left_of_ruler_size_group;

  /* ==== Piano Roll (Midi Editor) ==== */

  /** Toggle between drum mode and normal mode. */
  GtkToggleButton * toggle_notation;
  GtkToggleButton * toggle_listen_notes;
  GtkToggleButton * show_automation_values;

  MidiEditorSpaceWidget * midi_editor_space;

  /* ==== End Piano Roll (Midi Editor) ==== */

  /* ==== Automation Editor ==== */

  AutomationEditorSpaceWidget * automation_editor_space;

  /* ==== End Automation Editor ==== */

  /* ==== Chord Editor ==== */

  ChordEditorSpaceWidget * chord_editor_space;

  /* ==== End Chord Editor ==== */

  /* ==== Audio Editor ==== */

  AudioEditorSpaceWidget * audio_editor_space;

  /* ==== End Audio Editor ==== */

  /** Size group for keeping the whole ruler and
   * each timeline the same width. */
  GtkSizeGroup * ruler_arranger_hsize_group;

} ClipEditorInnerWidget;

void
clip_editor_inner_widget_setup (ClipEditorInnerWidget * self);

void
clip_editor_inner_widget_refresh (
  ClipEditorInnerWidget * self);

ArrangerWidget *
clip_editor_inner_widget_get_visible_arranger (
  ClipEditorInnerWidget * self);

/**
 * @}
 */

#endif
