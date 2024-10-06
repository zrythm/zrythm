// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/resources.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/backend/tracklist_selections.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/channel_sends_expander.h"
#include "gui/backend/gtk_widgets/color_area.h"
#include "gui/backend/gtk_widgets/fader_controls_expander.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/inspector_track.h"
#include "gui/backend/gtk_widgets/left_dock_edge.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/plugin_strip_expander.h"
#include "gui/backend/gtk_widgets/ports_expander.h"
#include "gui/backend/gtk_widgets/text_expander.h"
#include "gui/backend/gtk_widgets/track_input_expander.h"
#include "gui/backend/gtk_widgets/track_properties_expander.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (InspectorTrackWidget, inspector_track_widget, GTK_TYPE_WIDGET)

static void
reveal_cb (ExpanderBoxWidget * expander, const bool revealed, void * user_data)
{
  InspectorTrackWidget * self = Z_INSPECTOR_TRACK_WIDGET (user_data);

#define SET_SETTING(mem, key) \
  if (expander == Z_EXPANDER_BOX_WIDGET (self->mem)) \
    { \
      g_settings_set_boolean ( \
        S_UI_INSPECTOR, "track-" key "-expanded", revealed); \
    }

  SET_SETTING (track_info, "properties");
  SET_SETTING (inputs, "inputs");
  SET_SETTING (outputs, "outputs");
  SET_SETTING (sends, "sends");
  SET_SETTING (controls, "controls");
  SET_SETTING (inserts, "inserts");
  SET_SETTING (midi_fx, "midi-fx");
  SET_SETTING (fader, "fader");
  SET_SETTING (comment, "comment");

#undef SET_SETTING
}

static void
setup_color (InspectorTrackWidget * self, Track * track)
{
  if (track)
    {
      color_area_widget_setup_track (self->color, track);
    }
  else
    {
      Color color = { 1, 1, 1, 1 };
      color_area_widget_set_color (self->color, color);
    }
}

/**
 * Shows the inspector page for the given tracklist
 * selection.
 *
 * @param set_notebook_page Whether to set the
 *   current left panel tab to the track page.
 */
void
inspector_track_widget_show_tracks (
  InspectorTrackWidget *      self,
  SimpleTracklistSelections * tls,
  bool                        set_notebook_page)
{
  z_debug (
    "showing {} tracks (set notebook page: {})", tls->track_names_.size (),
    set_notebook_page);

  if (set_notebook_page)
    {
      PanelWidget * panel_widget = PANEL_WIDGET (gtk_widget_get_ancestor (
        GTK_WIDGET (MW_LEFT_DOCK_EDGE->track_inspector), PANEL_TYPE_WIDGET));
      z_return_if_fail (panel_widget);
      panel_widget_raise (panel_widget);
      z_debug ("raised track inspector");
    }

  /* show info for first track */
  if (!tls->track_names_.empty ())
    {
      auto track = tls->get_highest_track ();
      auto track_ptr = track;
      z_debug ("track {}", track->name_);

      /* don't attempt to show tracks during disconnect */
      z_return_if_fail (!track->disconnecting_);

      setup_color (self, track);

      track_properties_expander_widget_refresh (self->track_info, track);

      gtk_widget_set_visible (GTK_WIDGET (self->sends), false);
      gtk_widget_set_visible (GTK_WIDGET (self->outputs), false);
      gtk_widget_set_visible (GTK_WIDGET (self->controls), false);
      gtk_widget_set_visible (GTK_WIDGET (self->inputs), false);
      gtk_widget_set_visible (GTK_WIDGET (self->inserts), false);
      gtk_widget_set_visible (GTK_WIDGET (self->midi_fx), false);
      gtk_widget_set_visible (GTK_WIDGET (self->fader), false);
      gtk_widget_set_visible (GTK_WIDGET (self->comment), true);

      text_expander_widget_setup (
        self->comment, true, bind_member_function (*track, &Track::get_comment),
        bind_member_function (*track, &Track::set_comment_with_action), track);
      expander_box_widget_set_label (
        Z_EXPANDER_BOX_WIDGET (self->comment), _ ("Notes"));

      if (track->has_channel ())
        {
          auto ch_track = dynamic_cast<ChannelTrack *> (track);
          gtk_widget_set_visible (GTK_WIDGET (self->sends), true);
#if 0
          gtk_widget_set_visible (
            GTK_WIDGET (self->outputs), true);
          gtk_widget_set_visible (
            GTK_WIDGET (self->controls), true);
#endif
          gtk_widget_set_visible (GTK_WIDGET (self->fader), true);
          gtk_widget_set_visible (GTK_WIDGET (self->inserts), true);

          if (Track::type_has_inputs (track->type_))
            {
              gtk_widget_set_visible (GTK_WIDGET (self->inputs), true);
            }
          if (track->in_signal_type_ == PortType::Event)
            {
              gtk_widget_set_visible (GTK_WIDGET (self->midi_fx), true);
              plugin_strip_expander_widget_setup (
                self->midi_fx, zrythm::plugins::PluginSlotType::MidiFx,
                PluginStripExpanderPosition::PSE_POSITION_INSPECTOR, ch_track);
            }
          track_input_expander_widget_refresh (self->inputs, ch_track);
          ports_expander_widget_setup_track (
            self->outputs, track_ptr,
            PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_SENDS);
          ports_expander_widget_setup_track (
            self->controls, track_ptr,
            PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_CONTROLS);

          plugin_strip_expander_widget_setup (
            self->inserts, zrythm::plugins::PluginSlotType::Insert,
            PluginStripExpanderPosition::PSE_POSITION_INSPECTOR, ch_track);

          fader_controls_expander_widget_setup (self->fader, ch_track);

          channel_sends_expander_widget_setup (
            self->sends, ChannelSendsExpanderPosition::CSE_POSITION_INSPECTOR,
            track_ptr);
        }
    }
  else /* no tracks selected */
    {
      track_properties_expander_widget_refresh (self->track_info, nullptr);
      ports_expander_widget_setup_track (
        self->outputs, nullptr,
        PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_SENDS);
      ports_expander_widget_setup_track (
        self->controls, nullptr,
        PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_CONTROLS);
      text_expander_widget_setup (
        self->comment, false, nullptr, nullptr, nullptr);

      setup_color (self, nullptr);
    }
}

/**
 * Sets up the inspector track widget for the first
 * time.
 */
void
inspector_track_widget_setup (
  InspectorTrackWidget *      self,
  SimpleTracklistSelections * tls)
{
  z_return_if_fail (tls);
  if (tls->track_names_.empty ())
    {
      z_error ("no tracks selected. this should never happen");
      return;
    }

  auto track = tls->get_highest_track ();
  z_return_if_fail (track);

  track_properties_expander_widget_setup (self->track_info, track);
}

InspectorTrackWidget *
inspector_track_widget_new (void)
{
  InspectorTrackWidget * self = static_cast<InspectorTrackWidget *> (
    g_object_new (INSPECTOR_TRACK_WIDGET_TYPE, nullptr));

  return self;
}

/**
 * Prepare for finalization.
 */
void
inspector_track_widget_tear_down (InspectorTrackWidget * self)
{
  if (self->fader)
    {
      fader_controls_expander_widget_tear_down (self->fader);
    }
}

static void
inspector_track_widget_class_init (InspectorTrackWidgetClass * _klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_layout_manager_type (wklass, GTK_TYPE_BOX_LAYOUT);
  resources_set_class_template (wklass, "inspector_track.ui");
  gtk_widget_class_set_accessible_role (wklass, GTK_ACCESSIBLE_ROLE_GROUP);

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    GTK_WIDGET_CLASS (wklass), InspectorTrackWidget, child);

  BIND_CHILD (track_info);
  BIND_CHILD (sends);
  BIND_CHILD (outputs);
  BIND_CHILD (controls);
  BIND_CHILD (inputs);
  BIND_CHILD (inserts);
  BIND_CHILD (midi_fx);
  BIND_CHILD (fader);
  BIND_CHILD (comment);
  BIND_CHILD (color);

#undef BIND_CHILD
}

static void
inspector_track_widget_init (InspectorTrackWidget * self)
{
  g_type_ensure (TRACK_PROPERTIES_EXPANDER_WIDGET_TYPE);
  g_type_ensure (TRACK_INPUT_EXPANDER_WIDGET_TYPE);
  g_type_ensure (PORTS_EXPANDER_WIDGET_TYPE);
  g_type_ensure (PLUGIN_STRIP_EXPANDER_WIDGET_TYPE);
  g_type_ensure (CHANNEL_SENDS_EXPANDER_WIDGET_TYPE);
  g_type_ensure (FADER_CONTROLS_EXPANDER_WIDGET_TYPE);
  g_type_ensure (TEXT_EXPANDER_WIDGET_TYPE);
  g_type_ensure (COLOR_AREA_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_add_css_class (GTK_WIDGET (self), "inspector");

  GtkBoxLayout * box_layout =
    GTK_BOX_LAYOUT (gtk_widget_get_layout_manager (GTK_WIDGET (self)));
  gtk_box_layout_set_spacing (box_layout, 2);
  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (box_layout), GTK_ORIENTATION_VERTICAL);

  expander_box_widget_set_vexpand (Z_EXPANDER_BOX_WIDGET (self->inserts), false);
  expander_box_widget_set_vexpand (Z_EXPANDER_BOX_WIDGET (self->midi_fx), false);
  expander_box_widget_set_vexpand (Z_EXPANDER_BOX_WIDGET (self->comment), false);

  /* set states */
#define SET_STATE(mem, key) \
  expander_box_widget_set_reveal ( \
    Z_EXPANDER_BOX_WIDGET (self->mem), \
    g_settings_get_boolean (S_UI_INSPECTOR, "track-" key "-expanded")); \
  expander_box_widget_set_reveal_callback ( \
    Z_EXPANDER_BOX_WIDGET (self->mem), reveal_cb, self)

  SET_STATE (track_info, "properties");
  SET_STATE (inputs, "inputs");
  SET_STATE (outputs, "outputs");
  SET_STATE (sends, "sends");
  SET_STATE (controls, "controls");
  SET_STATE (inserts, "inserts");
  SET_STATE (midi_fx, "midi-fx");
  SET_STATE (fader, "fader");
  SET_STATE (comment, "comment");

#undef SET_STATE
}
