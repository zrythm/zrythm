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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Audio clip editor, to be used in place of the
 * piano roll when an audio clip/region is selected.
 */

#include "gui/widgets/arranger.h"
#include "gui/widgets/audio_clip_editor.h"
#include "project.h"
#include "utils/resources.h"

G_DEFINE_TYPE (AudioClipEditorWidget,
               audio_clip_editor_widget,
               GTK_TYPE_GRID)

/**
 * Links scroll windows after all widgets have been
 * initialized.
 */
static void
link_scrolls (
  AudioClipEditorWidget * self)
{
  /* link ruler h scroll to arranger h scroll */
  if (self->ruler_scroll)
    {
      gtk_scrolled_window_set_hadjustment (
        self->ruler_scroll,
        gtk_scrolled_window_get_hadjustment (
          GTK_SCROLLED_WINDOW (
            self->arranger_scroll)));
    }
}

void
audio_clip_editor_widget_setup (
  AudioClipEditorWidget * self,
  AudioClipEditor *       ace)
{
  self->audio_clip_editor = ace;

  if (self->arranger)
    {
      arranger_widget_setup (
        Z_ARRANGER_WIDGET (self->arranger),
        &PROJECT->snap_grid_midi);
      gtk_widget_show_all (
        GTK_WIDGET (self->arranger));
    }

  link_scrolls (self);
}

static void
audio_clip_editor_widget_init (
  AudioClipEditorWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
audio_clip_editor_widget_class_init (
  AudioClipEditorWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "audio_clip_editor.ui");

  gtk_widget_class_set_css_name (
    klass, "audio-clip-editor");

  gtk_widget_class_bind_template_child (
    klass,
    AudioClipEditorWidget,
    track_name);
  gtk_widget_class_bind_template_child (
    klass,
    AudioClipEditorWidget,
    color_bar);
  gtk_widget_class_bind_template_child (
    klass,
    AudioClipEditorWidget,
    ruler_scroll);
  gtk_widget_class_bind_template_child (
    klass,
    AudioClipEditorWidget,
    ruler_viewport);
  gtk_widget_class_bind_template_child (
    klass,
    AudioClipEditorWidget,
    ruler);
  gtk_widget_class_bind_template_child (
    klass,
    AudioClipEditorWidget,
    arranger_scroll);
  gtk_widget_class_bind_template_child (
    klass,
    AudioClipEditorWidget,
    arranger_viewport);
  gtk_widget_class_bind_template_child (
    klass,
    AudioClipEditorWidget,
    arranger);
}
