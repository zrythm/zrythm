// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/channel.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
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
  AutomationEditorSpaceWidget,
  automation_editor_space_widget,
  GTK_TYPE_BOX)

/**
 * Links scroll windows after all widgets have been
 * initialized.
 */
static void
link_scrolls (AutomationEditorSpaceWidget * self)
{
  /* link ruler h scroll to arranger h scroll */
  if (MW_CLIP_EDITOR_INNER->ruler_scroll)
    {
      gtk_scrolled_window_set_hadjustment (
        MW_CLIP_EDITOR_INNER->ruler_scroll,
        gtk_scrolled_window_get_hadjustment (
          self->arranger_scroll));
    }
}

/**
 * See CLIP_EDITOR_INNER_WIDGET_ADD_TO_SIZEGROUP.
 */
void
automation_editor_space_widget_update_size_group (
  AutomationEditorSpaceWidget * self,
  int                           visible)
{
  clip_editor_inner_widget_add_to_left_of_ruler_sizegroup (
    MW_CLIP_EDITOR_INNER,
    GTK_WIDGET (self->left_box), visible);
}

void
automation_editor_space_widget_refresh (
  AutomationEditorSpaceWidget * self)
{
  link_scrolls (self);
  /*automation_editor_legend_widget_refresh (*/
  /*self->legend);*/
}

void
automation_editor_space_widget_setup (
  AutomationEditorSpaceWidget * self)
{
  if (self->arranger)
    {
      arranger_widget_setup (
        Z_ARRANGER_WIDGET (self->arranger),
        ARRANGER_WIDGET_TYPE_AUTOMATION,
        SNAP_GRID_EDITOR);

      /*automation_editor_legend_widget_setup (*/
      /*self->legend);*/
    }

  automation_editor_space_widget_refresh (self);
}

static void
automation_editor_space_widget_init (
  AutomationEditorSpaceWidget * self)
{
  /*g_type_ensure (*/
  /*AUTOMATION_EDITOR_LEGEND_WIDGET_TYPE);*/

  gtk_widget_init_template (GTK_WIDGET (self));

  self->arranger->type =
    ARRANGER_WIDGET_TYPE_AUTOMATION;
}

static void
automation_editor_space_widget_class_init (
  AutomationEditorSpaceWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "automation_editor_space.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, AutomationEditorSpaceWidget, x)

  BIND_CHILD (arranger_scroll);
  BIND_CHILD (arranger_viewport);
  BIND_CHILD (arranger);
  BIND_CHILD (left_box);
  /*BIND_CHILD (legend);*/

#undef BIND_CHILD
}
