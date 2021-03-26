/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Bot dock.
 */

#ifndef __GUI_WIDGETS_BOT_DOCK_EDGE_H__
#define __GUI_WIDGETS_BOT_DOCK_EDGE_H__

#include <gtk/gtk.h>

#define BOT_DOCK_EDGE_WIDGET_TYPE \
  (bot_dock_edge_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BotDockEdgeWidget,
  bot_dock_edge_widget,
  Z, BOT_DOCK_EDGE_WIDGET,
  GtkBox)

#define MW_BOT_DOCK_EDGE MW_CENTER_DOCK->bot_dock_edge

/**
 * Brings up the Clip Editor in the notebook.
 */
#define SHOW_CLIP_EDITOR \
  gtk_notebook_set_current_page ( \
    GTK_NOTEBOOK ( \
      MW_BOT_DOCK_EDGE->bot_notebook), 0)

typedef struct _MixerWidget MixerWidget;
typedef struct _ClipEditorWidget ClipEditorWidget;
typedef struct _ModulatorViewWidget
  ModulatorViewWidget;
typedef struct _FoldableNotebookWidget
  FoldableNotebookWidget;
typedef struct _EventViewerWidget EventViewerWidget;
typedef struct _ChordPadWidget ChordPadWidget;

/**
 * Bot dock widget.
 */
typedef struct _BotDockEdgeWidget
{
  GtkBox                   parent_instance;
  FoldableNotebookWidget * bot_notebook;

  /** Wrapper. */
  //GtkBox *                 modulator_view_box;
  ModulatorViewWidget *    modulator_view;

  /** Event viewer. */
  EventViewerWidget *      event_viewer;

  /** Wrapper. */
  //GtkBox *                 clip_editor_box;
  ClipEditorWidget *       clip_editor;

  /** Wrapper. */
  //GtkBox *                 mixer_box;
  MixerWidget *            mixer;

  /** Chord pads. */
  ChordPadWidget *         chord_pad;
} BotDockEdgeWidget;

void
bot_dock_edge_widget_setup (
  BotDockEdgeWidget * self);

#endif
