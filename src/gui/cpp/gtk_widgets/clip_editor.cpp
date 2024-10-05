// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/clip_editor.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/gtk_widgets/arranger.h"
#include "gui/cpp/gtk_widgets/automation_arranger.h"
#include "gui/cpp/gtk_widgets/automation_editor_space.h"
#include "gui/cpp/gtk_widgets/bot_dock_edge.h"
#include "gui/cpp/gtk_widgets/center_dock.h"
#include "gui/cpp/gtk_widgets/chord_arranger.h"
#include "gui/cpp/gtk_widgets/chord_editor_space.h"
#include "gui/cpp/gtk_widgets/clip_editor.h"
#include "gui/cpp/gtk_widgets/clip_editor_inner.h"
#include "gui/cpp/gtk_widgets/editor_ruler.h"
#include "gui/cpp/gtk_widgets/editor_toolbar.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/midi_arranger.h"
#include "gui/cpp/gtk_widgets/midi_editor_space.h"
#include "gui/cpp/gtk_widgets/midi_modifier_arranger.h"
#include "gui/cpp/gtk_widgets/ruler.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include "common/utils/flags.h"
#include "common/utils/resources.h"

G_DEFINE_TYPE (ClipEditorWidget, clip_editor_widget, GTK_TYPE_BOX)

void
clip_editor_widget_setup (ClipEditorWidget * self)
{
  clip_editor_inner_widget_setup (self->clip_editor_inner);

  gtk_stack_set_visible_child (
    GTK_STACK (self->stack), GTK_WIDGET (self->no_selection_page));
}

/**
 * Called when setting clip editor region using
 * g_idle_add to give the widgets time to have
 * widths.
 */
static int
refresh_editor_ruler_and_arranger (void * user_data)
{
  if (!ui_is_widget_revealed (EDITOR_RULER))
    {
      return TRUE;
    }

  /* ruler must be refreshed first to get the
   * correct px when calling ui_* functions */
  ruler_widget_refresh ((RulerWidget *) EDITOR_RULER);

  CLIP_EDITOR->region_changed_ = false;

  return FALSE;
}

/**
 * To be called when the region changes.
 */
void
clip_editor_widget_on_region_changed (ClipEditorWidget * self)
{
  Region * r = CLIP_EDITOR->get_region ();

  if (r)
    {
      gtk_stack_set_visible_child (
        GTK_STACK (self->stack), GTK_WIDGET (self->main_box));

      clip_editor_inner_widget_refresh (MW_CLIP_EDITOR_INNER);

      g_idle_add (refresh_editor_ruler_and_arranger, nullptr);

      /* update the toolbar */
      editor_toolbar_widget_refresh (self->editor_toolbar);

      if (r->is_midi ())
        {
          /* FIXME remember ID and remove the source function
           * when this widget is disposed to avoid calling on
           * a free'd object */
          g_idle_add (
            (GSourceFunc) midi_editor_space_widget_scroll_to_middle,
            self->clip_editor_inner->midi_editor_space);
        }
    }
  else
    {
      gtk_stack_set_visible_child (
        GTK_STACK (self->stack), GTK_WIDGET (self->no_selection_page));
    }
}

/**
 * Navigates to the region start point.
 *
 * If already at start point and @p
 * center_contents_if_already_at_start is true, the region's
 * contents will be centered.
 */
void
clip_editor_widget_navigate_to_region_start (
  ClipEditorWidget * self,
  bool               center_contents_if_already_at_start)
{
  Region * r = CLIP_EDITOR->get_region ();
  z_return_if_fail (r);
  int              px = ui_pos_to_px_editor (r->pos_, false);
  ArrangerWidget * arranger = r->get_arranger_for_children ();
  auto             settings = arranger_widget_get_editor_settings (arranger);
  std::visit (
    [&] (auto &&s) {
      if (px == s->scroll_start_x_)
        {
          /* execute the best fit action to center contents */
          auto action = zrythm_app->lookup_action ("best-fit");
          auto var = Glib::Variant<Glib::ustring>::create ("editor");
          action->activate_variant (var);
        }
      else
        {
          s->set_scroll_start_x (px, false);
        }
    },
    settings);
}

ClipEditorWidget *
clip_editor_widget_new (void)
{
  auto * self = static_cast<ClipEditorWidget *> (
    g_object_new (CLIP_EDITOR_WIDGET_TYPE, nullptr));

  return self;
}

static void
clip_editor_widget_init (ClipEditorWidget * self)
{
  g_type_ensure (CLIP_EDITOR_INNER_WIDGET_TYPE);
  g_type_ensure (EDITOR_TOOLBAR_WIDGET_TYPE);
  g_type_ensure (ADW_TYPE_STATUS_PAGE);

  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
clip_editor_widget_class_init (ClipEditorWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "clip_editor.ui");

  gtk_widget_class_set_css_name (klass, "clip-editor");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, ClipEditorWidget, x)

  BIND_CHILD (stack);
  BIND_CHILD (main_box);
  BIND_CHILD (editor_toolbar);
  BIND_CHILD (clip_editor_inner);
  BIND_CHILD (no_selection_page);

#undef BIND_CHILD
}
