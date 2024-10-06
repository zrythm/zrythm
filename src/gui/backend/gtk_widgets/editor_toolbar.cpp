// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/audio_function.h"
#include "common/dsp/automation_function.h"
#include "common/dsp/midi_function.h"
#include "common/dsp/quantize_options.h"
#include "common/utils/resources.h"
#include "gui/backend/backend/actions/actions.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/gtk_widgets/editor_toolbar.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/playhead_scroll_buttons.h"
#include "gui/backend/gtk_widgets/quantize_box.h"
#include "gui/backend/gtk_widgets/snap_grid.h"
#include "gui/backend/gtk_widgets/zoom_buttons.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (EditorToolbarWidget, editor_toolbar_widget, GTK_TYPE_WIDGET)

static void
on_highlighting_changed (
  GtkDropDown * dropdown,
  GParamSpec *  pspec,
  gpointer      user_data)
{
  PIANO_ROLL->set_highlighting (ENUM_INT_TO_VALUE (
    PianoRoll::Highlighting, gtk_drop_down_get_selected (dropdown)));
}

static void
setup_funcs_btn (AdwSplitButton * split_btn)
{
  adw_split_button_set_icon_name (split_btn, "code-context");
  gtk_actionable_set_detailed_action_name (
    GTK_ACTIONABLE (split_btn), "app.editor-function::current");
  gtk_widget_set_tooltip_text (GTK_WIDGET (split_btn), _ ("Apply function"));
  gtk_widget_add_css_class (GTK_WIDGET (split_btn), "raised");
}

/**
 * Appends eligible plugins.
 */
static void
update_audio_funcs_menu (EditorToolbarWidget * self)
{
  self->audio_functions_menu = G_MENU_MODEL (g_menu_new ());
  for (
    unsigned int i = ENUM_VALUE_TO_INT (AudioFunctionType::Invert);
    i < ENUM_VALUE_TO_INT (AudioFunctionType::CustomPlugin); i++)
    {
      /* not implemented yet */
      if (
        i == ENUM_VALUE_TO_INT (AudioFunctionType::NormalizeRMS)
        || i == ENUM_VALUE_TO_INT (AudioFunctionType::NormalizeLUFS))
        continue;

      auto detailed_action = audio_function_get_detailed_action_for_type (
        ENUM_INT_TO_VALUE (AudioFunctionType, i),
        "editor-toolbar.editor-function");
      GMenuItem * item = g_menu_item_new (
        AudioFunctionType_to_string (
          ENUM_INT_TO_VALUE (AudioFunctionType, i), true)
          .c_str (),
        detailed_action.c_str ());
      const char * icon_name = audio_function_get_icon_name_for_type (
        ENUM_INT_TO_VALUE (AudioFunctionType, i));
      g_menu_item_set_attribute (
        item, G_MENU_ATTRIBUTE_ICON, "s", icon_name, nullptr);
      g_menu_append_item (G_MENU (self->audio_functions_menu), item);
    }

    /* ignore plugins for now */
#if 0
  GMenu * plugins_menu = g_menu_new ();
  for (size_t i = 0;
       i < PLUGIN_MANAGER->plugin_descriptors->len;
       i++)
    {
      PluginDescriptor * descr =
        g_ptr_array_index (
          PLUGIN_MANAGER->plugin_descriptors, i);
      if (descr->protocol != PROT_LV2
          || !plugin_descriptor_is_effect (descr)
          || descr->num_audio_ins != 2
          || descr->num_audio_outs != 2
          || descr->category == ANALYZER)
        continue;

      /* skip if open with carla by default */
      PluginSetting * setting =
        plugin_setting_new_default (descr);
      z_return_if_fail (setting);
      bool skip = false;
      if (setting->open_with_carla_)
        {
          skip = true;
        }
      plugin_setting_free (setting);
      if (skip)
        continue;

      char * detailed_action =
        g_strdup_printf (
          "app.editor-function-lv2::%s", descr->uri);
      GMenuItem * item =
        g_menu_item_new (
          descr->name, detailed_action);
      g_menu_item_set_attribute (
        item, G_MENU_ATTRIBUTE_ICON,
        "s", "logo-lv2", nullptr);
      g_menu_append_item (
        G_MENU (plugins_menu), item);
    }

  g_menu_append_submenu (
    G_MENU (self->audio_functions_menu),
    _("Plugin Effect"), G_MENU_MODEL (plugins_menu));
#endif

  g_menu_freeze (G_MENU (self->audio_functions_menu));

  adw_split_button_set_menu_model (
    self->audio_functions_btn, self->audio_functions_menu);
}

static void
update_midi_funcs_menu (EditorToolbarWidget * self)
{
  adw_split_button_set_menu_model (
    self->midi_functions_btn, self->midi_functions_menu);
}

static void
update_automation_funcs_menu (EditorToolbarWidget * self)
{
  adw_split_button_set_menu_model (
    self->automation_functions_btn, self->automation_functions_menu);
}

/**
 * Refreshes relevant widgets.
 */
void
editor_toolbar_widget_refresh (EditorToolbarWidget * self)
{
  Region * region = CLIP_EDITOR->get_region ();
  if (!region)
    {
      return;
    }

  /* set visibility of each tool item */
  gtk_widget_set_visible (GTK_WIDGET (self->chord_highlight_box), false);
  gtk_widget_set_visible (GTK_WIDGET (self->sep_after_chord_highlight), false);
  gtk_widget_set_visible (GTK_WIDGET (self->ghost_notes_btn), false);

  switch (region->id_.type_)
    {
    case RegionType::Midi:
      {
        gtk_stack_set_visible_child_name (
          self->functions_btn_stack, "midi-page");
        MidiFunctionType type = ENUM_INT_TO_VALUE (
          MidiFunctionType, g_settings_get_int (S_UI, "midi-function"));
        auto type_str = MidiFunctionType_to_string (type, true);
        auto str = format_str (_ ("Apply %s"), type_str);
        auto tooltip_str =
          format_str (_ ("Apply %s with previous settings"), type_str);
        adw_split_button_set_label (self->midi_functions_btn, str.c_str ());
        gtk_widget_set_tooltip_text (
          GTK_WIDGET (self->midi_functions_btn), tooltip_str.c_str ());

        /* set visibility of each tool item */
        gtk_widget_set_visible (GTK_WIDGET (self->chord_highlight_box), true);
        gtk_widget_set_visible (
          GTK_WIDGET (self->sep_after_chord_highlight), true);
        gtk_widget_set_visible (GTK_WIDGET (self->ghost_notes_btn), true);
      }
      break;
    case RegionType::Automation:
      {
        gtk_stack_set_visible_child_name (
          self->functions_btn_stack, "automation-page");
        AutomationFunctionType type = ENUM_INT_TO_VALUE (
          AutomationFunctionType,
          g_settings_get_int (S_UI, "automation-function"));
        auto type_str = AutomationFunctionType_to_string (type, true);
        auto str = format_str (_ ("Apply %s"), type_str);
        auto tooltip_str =
          format_str (_ ("Apply %s with previous settings"), type_str);
        adw_split_button_set_label (
          self->automation_functions_btn, str.c_str ());
        gtk_widget_set_tooltip_text (
          GTK_WIDGET (self->automation_functions_btn), tooltip_str.c_str ());
      }
      break;
    case RegionType::Audio:
      {
        gtk_stack_set_visible_child_name (
          self->functions_btn_stack, "audio-page");
        AudioFunctionType type = ENUM_INT_TO_VALUE (
          AudioFunctionType, g_settings_get_int (S_UI, "audio-function"));
        auto type_str = AudioFunctionType_to_string (type, true);
        auto str = format_str (_ ("Apply %s"), type_str);
        auto tooltip_str =
          format_str (_ ("Apply %s with previous settings"), type_str);
        adw_split_button_set_label (self->audio_functions_btn, str.c_str ());
        gtk_widget_set_tooltip_text (
          GTK_WIDGET (self->audio_functions_btn), tooltip_str.c_str ());
      }
      break;
    default:
      /* TODO */
      gtk_stack_set_visible_child_name (self->functions_btn_stack, "empty-page");
      break;
    }
}

using HighlightColumns = enum HighlightColumns
{
  ICON_NAME_COL,
  LABEL_COL,
  ACTION_COL,
};

void
editor_toolbar_widget_setup (EditorToolbarWidget * self)
{
  /* setup bot toolbar */
  snap_grid_widget_setup (self->snap_grid, SNAP_GRID_EDITOR.get ());
  quantize_box_widget_setup (self->quantize_box, QUANTIZE_OPTIONS_EDITOR.get ());

  /* setup highlighting */
  GtkStringList * slist = gtk_string_list_new (nullptr);
  gtk_string_list_append (slist, _ ("No Highlight"));
  gtk_string_list_append (slist, _ ("Highlight Chord"));
  gtk_string_list_append (slist, _ ("Highlight Scale"));
  gtk_string_list_append (slist, _ ("Highlight Both"));
  GtkDropDown * dropdown = GTK_DROP_DOWN (self->chord_highlighting);
  gtk_drop_down_set_model (dropdown, G_LIST_MODEL (slist));
  gtk_drop_down_set_selected (
    dropdown, ENUM_VALUE_TO_INT (PIANO_ROLL->highlighting_));

  /* setup signals */
  g_signal_connect (
    G_OBJECT (self->chord_highlighting), "notify::selected",
    G_CALLBACK (on_highlighting_changed), self);

  editor_toolbar_widget_refresh (self);
}

static void
dispose_editor_toolbar_widget (EditorToolbarWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->scroll));

  G_OBJECT_CLASS (editor_toolbar_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
editor_toolbar_widget_init (EditorToolbarWidget * self)
{
  g_type_ensure (PLAYHEAD_SCROLL_BUTTONS_WIDGET_TYPE);
  g_type_ensure (SNAP_GRID_WIDGET_TYPE);
  g_type_ensure (ZOOM_BUTTONS_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  setup_funcs_btn (self->midi_functions_btn);
  setup_funcs_btn (self->audio_functions_btn);
  setup_funcs_btn (self->automation_functions_btn);

  /* add action group wrapper */
  GSimpleActionGroup * action_group = g_simple_action_group_new ();
  const GActionEntry   entries[] = {
    { "editor-function", activate_app_action_wrapper, "s" },
  };
  g_action_map_add_action_entries (
    G_ACTION_MAP (action_group), entries, G_N_ELEMENTS (entries), nullptr);
  gtk_widget_insert_action_group (
    GTK_WIDGET (self), "editor-toolbar", G_ACTION_GROUP (action_group));

  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->playhead_scroll->scroll_edges),
    "app.editor-playhead-scroll-edges");
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->playhead_scroll->follow),
    "app.editor-playhead-follow");

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable (GTK_ACTIONABLE (self->x), _ (tooltip))
  /*SET_TOOLTIP (loop_selection, "Loop Selection");*/
#undef SET_TOOLTIP

  update_automation_funcs_menu (self);
  update_midi_funcs_menu (self);
  update_audio_funcs_menu (self);

  /* TODO */
#if 0
  GtkMenuButton * menu_btn =
    button_with_menu_widget_get_menu_button (
      self->functions_btn);
  gtk_menu_button_set_use_popover (menu_btn, false);
#endif

  zoom_buttons_widget_setup (
    self->zoom_buttons, false, GTK_ORIENTATION_HORIZONTAL);
}

static void
editor_toolbar_widget_class_init (EditorToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "editor_toolbar.ui");

  gtk_widget_class_set_css_name (klass, "editor-toolbar");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, EditorToolbarWidget, x)

  BIND_CHILD (scroll);
  BIND_CHILD (chord_highlighting);
  BIND_CHILD (chord_highlight_box);
  BIND_CHILD (sep_after_chord_highlight);
  BIND_CHILD (ghost_notes_btn);
  BIND_CHILD (snap_grid);
  BIND_CHILD (quantize_box);
  BIND_CHILD (event_viewer_toggle);
  BIND_CHILD (automation_functions_menu);
  BIND_CHILD (midi_functions_menu);
  /*BIND_CHILD (audio_functions_menu);*/
  BIND_CHILD (functions_btn_stack);
  BIND_CHILD (audio_functions_btn);
  BIND_CHILD (midi_functions_btn);
  BIND_CHILD (automation_functions_btn);
  BIND_CHILD (playhead_scroll);
  BIND_CHILD (zoom_buttons);

#undef BIND_CHILD

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose_editor_toolbar_widget;
}
