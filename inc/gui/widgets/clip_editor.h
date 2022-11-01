// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 */

#ifndef __GUI_WIDGETS_CLIP_EDITOR_H__
#define __GUI_WIDGETS_CLIP_EDITOR_H__

#include <adwaita.h>
#include <gtk/gtk.h>

#define CLIP_EDITOR_WIDGET_TYPE \
  (clip_editor_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ClipEditorWidget,
  clip_editor_widget,
  Z,
  CLIP_EDITOR_WIDGET,
  GtkBox)

#define MW_CLIP_EDITOR MW_BOT_DOCK_EDGE->clip_editor

typedef struct _ClipEditorInnerWidget ClipEditorInnerWidget;
typedef struct _EditorSelectionInfoWidget
                                    EditorSelectionInfoWidget;
typedef struct _EditorToolbarWidget EditorToolbarWidget;
typedef struct _AudioClipEditorWidget AudioClipEditorWidget;
typedef struct ClipEditor             ClipEditor;

/**
 * The ClipEditorWidget shows in the Clip Editor /
 * Piano Roll tab of the bottom panel, and is a stack
 * of ClipEditorInnerWidget for the piano roll and
 * AudioClipEditorWidget for audio regions.
 */
typedef struct _ClipEditorWidget
{
  GtkBox parent_instance;

  GtkStack * stack;

  GtkBox * main_box;
  //EditorSelectionInfoWidget * editor_selections;
  EditorToolbarWidget *   editor_toolbar;
  ClipEditorInnerWidget * clip_editor_inner;

  /** Page to show when no clip is selected. */
  AdwStatusPage * no_selection_page;
} ClipEditorWidget;

void
clip_editor_widget_setup (ClipEditorWidget * self);

/**
 * To be called when the region changes.
 */
void
clip_editor_widget_on_region_changed (ClipEditorWidget * self);

ClipEditorWidget *
clip_editor_widget_new (void);

#endif
