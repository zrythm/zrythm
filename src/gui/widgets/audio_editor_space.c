/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/channel.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  AudioEditorSpaceWidget,
  audio_editor_space_widget,
  GTK_TYPE_BOX)

/**
 * Links scroll windows after all widgets have been
 * initialized.
 */
static void
link_scrolls (AudioEditorSpaceWidget * self)
{
  /* link ruler h scroll to arranger h scroll */
  if (MW_CLIP_EDITOR_INNER->ruler_scroll)
    {
      g_return_if_fail (GTK_IS_WIDGET (
        MW_CLIP_EDITOR_INNER->ruler_scroll));
      gtk_scrolled_window_set_hadjustment (
        MW_CLIP_EDITOR_INNER->ruler_scroll,
        gtk_scrolled_window_get_hadjustment (
          GTK_SCROLLED_WINDOW (
            self->arranger_scroll)));
    }
}

/**
 * See CLIP_EDITOR_INNER_WIDGET_ADD_TO_SIZEGROUP.
 */
void
audio_editor_space_widget_update_size_group (
  AudioEditorSpaceWidget * self,
  int                      visible)
{
  clip_editor_inner_widget_add_to_left_of_ruler_sizegroup (
    MW_CLIP_EDITOR_INNER,
    GTK_WIDGET (self->left_box), visible);
}

void
audio_editor_space_widget_refresh (
  AudioEditorSpaceWidget * self)
{
  link_scrolls (self);
}

void
audio_editor_space_widget_setup (
  AudioEditorSpaceWidget * self)
{
  if (self->arranger)
    {
      arranger_widget_setup (
        Z_ARRANGER_WIDGET (self->arranger),
        ARRANGER_WIDGET_TYPE_AUDIO,
        SNAP_GRID_EDITOR);
    }

  audio_editor_space_widget_refresh (self);
}

static void
audio_editor_space_widget_init (
  AudioEditorSpaceWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->arranger->type = ARRANGER_WIDGET_TYPE_AUDIO;
}

static void
audio_editor_space_widget_class_init (
  AudioEditorSpaceWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "audio_editor_space.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, AudioEditorSpaceWidget, x)

  BIND_CHILD (arranger_scroll);
  BIND_CHILD (left_box);
  BIND_CHILD (arranger_viewport);
  BIND_CHILD (arranger);
}
