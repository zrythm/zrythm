// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/playhead_scroll_buttons.h"
#include "gui/widgets/quantize_box.h"
#include "gui/widgets/range_action_buttons.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/timeline_toolbar.h"
#include "gui/widgets/zoom_buttons.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (TimelineToolbarWidget, timeline_toolbar_widget, GTK_TYPE_BOX)

void
timeline_toolbar_widget_refresh (TimelineToolbarWidget * self)
{
  /* enable/disable merge button */
  bool sensitive = TL_SELECTIONS->can_be_merged ();
  z_debug ("settings merge button sensitivity %d", sensitive);
  gtk_widget_set_sensitive (GTK_WIDGET (self->merge_btn), sensitive);
}

void
timeline_toolbar_widget_setup (TimelineToolbarWidget * self)
{
  snap_grid_widget_setup (self->snap_grid, SNAP_GRID_TIMELINE.get ());
  quantize_box_widget_setup (
    self->quantize_box, QUANTIZE_OPTIONS_TIMELINE.get ());
}

static void
timeline_toolbar_widget_init (TimelineToolbarWidget * self)
{
  g_type_ensure (QUANTIZE_BOX_WIDGET_TYPE);
  g_type_ensure (RANGE_ACTION_BUTTONS_WIDGET_TYPE);
  g_type_ensure (SNAP_GRID_WIDGET_TYPE);
  g_type_ensure (PLAYHEAD_SCROLL_BUTTONS_WIDGET_TYPE);
  g_type_ensure (ZOOM_BUTTONS_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), \
    gtk_widget_get_tooltip_text (GTK_WIDGET (self->x)))

  SET_TOOLTIP (event_viewer_toggle);
  SET_TOOLTIP (musical_mode_toggle);
  SET_TOOLTIP (merge_btn);

#undef SET_TOOLTIP

#if ZRYTHM_TARGET_VER_MAJ == 1
  gtk_widget_set_visible (GTK_WIDGET (self->musical_mode_toggle), false);
#endif

  zoom_buttons_widget_setup (
    self->zoom_buttons, true, GTK_ORIENTATION_HORIZONTAL);
}

static void
timeline_toolbar_widget_class_init (TimelineToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "timeline_toolbar.ui");

  gtk_widget_class_set_accessible_role (klass, GTK_ACCESSIBLE_ROLE_TOOLBAR);

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, TimelineToolbarWidget, x)

  BIND_CHILD (snap_grid);
  BIND_CHILD (quantize_box);
  BIND_CHILD (event_viewer_toggle);
  BIND_CHILD (musical_mode_toggle);
  BIND_CHILD (range_action_buttons);
  BIND_CHILD (zoom_buttons);
  BIND_CHILD (merge_btn);

#undef BIND_CHILD
}
