// SPDX-FileCopyrightText: © 2019, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/channel.h"
#include "common/dsp/region.h"
#include "common/dsp/track.h"
#include "common/utils/gtk.h"
#include "common/utils/resources.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/gtk_widgets/arranger.h"
#include "gui/backend/gtk_widgets/audio_arranger.h"
#include "gui/backend/gtk_widgets/audio_editor_space.h"
#include "gui/backend/gtk_widgets/bot_dock_edge.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/clip_editor.h"
#include "gui/backend/gtk_widgets/clip_editor_inner.h"
#include "gui/backend/gtk_widgets/color_area.h"
#include "gui/backend/gtk_widgets/editor_ruler.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/ruler.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (AudioEditorSpaceWidget, audio_editor_space_widget, GTK_TYPE_BOX)

/**
 * See CLIP_EDITOR_INNER_WIDGET_ADD_TO_SIZEGROUP.
 */
void
audio_editor_space_widget_update_size_group (
  AudioEditorSpaceWidget * self,
  int                      visible)
{
  clip_editor_inner_widget_add_to_left_of_ruler_sizegroup (
    MW_CLIP_EDITOR_INNER, GTK_WIDGET (self->left_box), visible);
}

void
audio_editor_space_widget_refresh (AudioEditorSpaceWidget * self)
{
  /*link_scrolls (self);*/
}

void
audio_editor_space_widget_setup (AudioEditorSpaceWidget * self)
{
  if (self->arranger)
    {
      arranger_widget_setup (
        Z_ARRANGER_WIDGET (self->arranger), ArrangerWidgetType::Audio,
        SNAP_GRID_EDITOR);
    }

  audio_editor_space_widget_refresh (self);
}

static void
audio_editor_space_widget_init (AudioEditorSpaceWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_set_hexpand (GTK_WIDGET (self->arranger), true);
  gtk_widget_set_vexpand (GTK_WIDGET (self->arranger), true);

  self->arranger->type = ArrangerWidgetType::Audio;
}

static void
audio_editor_space_widget_class_init (AudioEditorSpaceWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "audio_editor_space.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, AudioEditorSpaceWidget, x)

  BIND_CHILD (left_box);
  BIND_CHILD (arranger);
}
