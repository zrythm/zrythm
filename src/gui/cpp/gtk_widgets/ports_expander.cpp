// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/port.h"
#include "dsp/port_identifier.h"
#include "dsp/track.h"
#include "gui/cpp/backend/wrapped_object_with_change_signal.h"
#include "gui/cpp/gtk_widgets/inspector_port.h"
#include "gui/cpp/gtk_widgets/ports_expander.h"
#include "plugins/plugin.h"
#include "project.h"

#include <glib/gi18n.h>

#include "libadwaita_wrapper.h"

G_DEFINE_TYPE (
  PortsExpanderWidget,
  ports_expander_widget,
  TWO_COL_EXPANDER_BOX_WIDGET_TYPE)

void
ports_expander_widget_refresh (PortsExpanderWidget * self)
{
  /* FIXME the port counts used don't take into account invisible (notOnGUI)
   * ports */
  if (self->owner_type == PortIdentifier::OwnerType::Plugin)
    {
      if (!self->plugin)
        {
          gtk_widget_set_visible (GTK_WIDGET (self), false);
          return;
        }

      const auto &descr = self->plugin->setting_.descr_;

      auto get_port_count = [&descr] (PortType type, PortFlow flow) {
        switch (type)
          {
          case PortType::Control:
            return flow == PortFlow::Input ? descr.num_ctrl_ins_ : descr.num_ctrl_outs_;
          case PortType::Audio:
            return flow == PortFlow::Input
                     ? descr.num_audio_ins_
                     : descr.num_audio_outs_;
          case PortType::Event:
            return flow == PortFlow::Input ? descr.num_midi_ins_ : descr.num_midi_outs_;
          case PortType::CV:
            return flow == PortFlow::Input ? descr.num_cv_ins_ : descr.num_cv_outs_;
          default:
            return 0;
          }
      };

      bool visible = get_port_count (self->type, self->flow) > 0;
      gtk_widget_set_visible (GTK_WIDGET (self), visible);
    }
  else if (self->owner_type == PortIdentifier::OwnerType::Track)
    {
    }
}

static void
set_icon_from_port_type (PortsExpanderWidget * self, PortType type)
{
  if (type == PortType::Audio)
    expander_box_widget_set_icon_name (
      Z_EXPANDER_BOX_WIDGET (self), "signal-audio");
  else if (type == PortType::CV)
    expander_box_widget_set_icon_name (
      Z_EXPANDER_BOX_WIDGET (self), "signal-cv");
  else if (type == PortType::Event)
    expander_box_widget_set_icon_name (
      Z_EXPANDER_BOX_WIDGET (self), "audio-midi");
  else if (type == PortType::Control)
    expander_box_widget_set_icon_name (
      Z_EXPANDER_BOX_WIDGET (self), "gnome-icon-library-wrench-wide-symbolic");
}

struct PortGroup
{
  PortGroup () = default;
  PortGroup (std::string name) : group_name (std::move (name)) { }

  std::string         group_name;
  std::vector<Port *> ports;
};

static void
item_factory_setup_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  /*PortsExpanderWidget * self = (PortsExpanderWidget *) user_data;*/
  gtk_list_item_set_child (listitem, GTK_WIDGET (adw_bin_new ()));
}

static void
item_factory_bind_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  WrappedObjectWithChangeSignal * obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gtk_list_item_get_item (listitem));
  auto                  port = wrapped_object_with_change_signal_get_port (obj);
  InspectorPortWidget * ip = inspector_port_widget_new (port);
  AdwBin *              bin = ADW_BIN (gtk_list_item_get_child (listitem));
  adw_bin_set_child (bin, GTK_WIDGET (ip));
}

static void
item_factory_unbind_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  AdwBin * bin = ADW_BIN (gtk_list_item_get_child (listitem));
  adw_bin_set_child (bin, nullptr);
}

static void
item_factory_teardown_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  gtk_list_item_set_child (listitem, nullptr);
}

/**
 * Sets up the PortsExpanderWidget for a Plugin.
 */
void
ports_expander_widget_setup_plugin (
  PortsExpanderWidget * self,
  PortFlow              flow,
  PortType              type,
  Plugin *              pl)
{
  self->flow = flow;
  self->type = type;
  self->owner_type = PortIdentifier::OwnerType::Plugin;

  /* if same plugin, do nothing (already set up) */
  if (self->plugin == pl)
    return;

  /* set name and icon */
  char fullstr[200];
  strcpy (fullstr, "");

  if (type == PortType::Control && flow == PortFlow::Input)
    strcpy (fullstr, _ ("Controls"));
  else if (type == PortType::Control && flow == PortFlow::Output)
    strcpy (fullstr, _ ("Control Outs"));
  else if (type == PortType::Audio && flow == PortFlow::Input)
    strcpy (fullstr, _ ("Audio Ins"));
  else if (type == PortType::Audio && flow == PortFlow::Output)
    strcpy (fullstr, _ ("Audio Outs"));
  else if (type == PortType::Event && flow == PortFlow::Input)
    strcpy (fullstr, _ ("MIDI Ins"));
  else if (type == PortType::Event && flow == PortFlow::Output)
    strcpy (fullstr, _ ("MIDI Outs"));
  else if (type == PortType::CV && flow == PortFlow::Input)
    strcpy (fullstr, _ ("CV Ins"));
  else if (type == PortType::CV && flow == PortFlow::Output)
    strcpy (fullstr, _ ("CV Outs"));

  expander_box_widget_set_label (Z_EXPANDER_BOX_WIDGET (self), fullstr);

  set_icon_from_port_type (self, type);

  /* readd ports */
  two_col_expander_box_widget_remove_children (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self));

  /* set scrollbar options */
  two_col_expander_box_widget_set_scroll_policy (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_POLICY_NEVER,
    GTK_POLICY_AUTOMATIC);
  two_col_expander_box_widget_set_min_max_size (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), -1, -1, -1, 120);

  TwoColExpanderBoxWidgetPrivate * prv = two_col_expander_box_widget_get_private (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self));
  gtk_widget_set_vexpand_set (GTK_WIDGET (prv->content), 1);

  std::vector<Port *> ports;
  if (pl)
    {
      const auto &source_ports =
        (flow == PortFlow::Input) ? pl->in_ports_ : pl->out_ports_;
      for (const auto &port : source_ports)
        {
          const auto &pi = port->id_;
          if (
            pi.type_ == type
            && !ENUM_BITSET_TEST (
              PortIdentifier::Flags, pi.flags_, PortIdentifier::Flags::NotOnGui))
            {
              ports.push_back (port.get ());
            }
        }
    }
  std::sort (ports.begin (), ports.end (), PortIdentifier::port_group_cmp);

  z_debug (
    "adding ports for plugin %s, type %s flow %s...",
    pl ? pl->get_name () : "(none)", ENUM_NAME (type), ENUM_NAME (flow));

  /* temporary array */
  std::vector<PortGroup> port_groups;

  /* Add ports in group order */
  std::string last_group;
  // int         num_ports = (int) ports->len;
  for (auto port : ports)
    {
      const std::string group = port->id_.port_group_;

      PortGroup * port_group = nullptr;

      /* Check group and add new heading if necessary */
      if (group != last_group)
        {
          const auto group_name =
            group.empty () ? std::string (_ ("Ungrouped")) : group;

          port_groups.emplace_back (group_name);
          port_group = &port_groups.back ();
        }
      last_group = group;

      if (!port_group)
        {
          if (port_groups.empty ())
            {
              port_groups.emplace_back (group);
            }

          port_group = &port_groups.back ();
        }

      port_group->ports.push_back (port);
    }

  /* foreach port group */
  for (auto &pg : port_groups)
    {
      /* add the header */
      if (!pg.group_name.empty ())
        {
          GtkWidget * group_label = gtk_label_new (pg.group_name.c_str ());
          gtk_widget_set_name (
            GTK_WIDGET (group_label),
            "ports-expander-inspector-port-group-label");
          gtk_label_set_xalign (GTK_LABEL (group_label), 0.f);
          gtk_widget_add_css_class (group_label, "small-and-bold");
          gtk_widget_set_margin_top (group_label, 3);
          gtk_widget_set_margin_bottom (group_label, 3);
          two_col_expander_box_widget_add_single (
            Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (group_label));
        }

      /* add the list view of ports */
      GListStore * store =
        g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
      for (auto &p : pg.ports)
        {
          std::visit (
            [&] (auto &&derived_port) {
              WrappedObjectWithChangeSignal * wrapped_obj =
                wrapped_object_with_change_signal_new (
                  derived_port, WrappedObjectType::WRAPPED_OBJECT_TYPE_PORT);
              g_list_store_append (store, wrapped_obj);
            },
            convert_to_variant<PortPtrVariant> (p));
        }
      GtkNoSelection * no_selection =
        gtk_no_selection_new (G_LIST_MODEL (store));
      GtkListItemFactory * list_item_factory =
        gtk_signal_list_item_factory_new ();
      g_signal_connect (
        G_OBJECT (list_item_factory), "setup",
        G_CALLBACK (item_factory_setup_cb), self);
      g_signal_connect (
        G_OBJECT (list_item_factory), "bind", G_CALLBACK (item_factory_bind_cb),
        self);
      g_signal_connect (
        G_OBJECT (list_item_factory), "unbind",
        G_CALLBACK (item_factory_unbind_cb), self);
      g_signal_connect (
        G_OBJECT (list_item_factory), "teardown",
        G_CALLBACK (item_factory_teardown_cb), self);
      GtkListView * list_view = GTK_LIST_VIEW (gtk_list_view_new (
        GTK_SELECTION_MODEL (no_selection), list_item_factory));
      if (pg.ports.size () > 100)
        {
          GtkScrolledWindow * scroll =
            GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
          gtk_scrolled_window_set_child (scroll, GTK_WIDGET (list_view));
          gtk_scrolled_window_set_placement (scroll, GTK_CORNER_BOTTOM_RIGHT);
          gtk_scrolled_window_set_policy (
            scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
          gtk_scrolled_window_set_min_content_height (scroll, 500);
          two_col_expander_box_widget_add_single (
            (TwoColExpanderBoxWidget *) self, (GtkWidget *) scroll);
        }
      else
        {
          two_col_expander_box_widget_add_single (
            (TwoColExpanderBoxWidget *) self, (GtkWidget *) list_view);
        }
    }

  z_debug ("added ports");

  self->plugin = pl;

  ports_expander_widget_refresh (self);
}

/**
 * Sets up the PortsExpanderWidget for Track ports.
 *
 * @param type The type of ports to include.
 */
void
ports_expander_widget_setup_track (
  PortsExpanderWidget *      self,
  Track *                    tr,
  PortsExpanderTrackPortType type)
{
  self->owner_type = PortIdentifier::OwnerType::Track;
  self->track = tr;

  /*PortType in_type =*/
  /*self->track->in_signal_type;*/
  PortType out_type = tr ? self->track->out_signal_type_ : PortType::Audio;

  switch (type)
    {
    case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_CONTROLS:
      expander_box_widget_set_label (
        Z_EXPANDER_BOX_WIDGET (self), _ ("Controls"));
      self->flow = PortFlow::Input;
      self->type = PortType::Control;
      break;
    case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_SENDS:
      expander_box_widget_set_label (Z_EXPANDER_BOX_WIDGET (self), _ ("Sends"));
      self->flow = PortFlow::Output;
      self->type = out_type;
      break;
    case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_STEREO_IN:
      expander_box_widget_set_label (
        Z_EXPANDER_BOX_WIDGET (self), _ ("Stereo In"));
      self->flow = PortFlow::Input;
      self->owner_type = PortIdentifier::OwnerType::Track;
      break;
    case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_MIDI_IN:
      expander_box_widget_set_label (
        Z_EXPANDER_BOX_WIDGET (self), _ ("MIDI In"));
      self->owner_type = PortIdentifier::OwnerType::Track;
      self->flow = PortFlow::Input;
      self->type = PortType::Event;
      break;
    case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_MIDI_OUT:
      expander_box_widget_set_label (
        Z_EXPANDER_BOX_WIDGET (self), _ ("MIDI Out"));
      self->owner_type = PortIdentifier::OwnerType::Track;
      self->flow = PortFlow::Output;
      self->type = PortType::Event;
      break;
    }

  set_icon_from_port_type (self, self->type);

  /* readd ports */
  two_col_expander_box_widget_remove_children (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self));

  TwoColExpanderBoxWidgetPrivate * prv = two_col_expander_box_widget_get_private (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self));
  gtk_widget_set_vexpand_set (GTK_WIDGET (prv->content), 1);

#define ADD_SINGLE(x) \
  ip = inspector_port_widget_new (x); \
  two_col_expander_box_widget_add_single ( \
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (ip))

  if (auto ch_track = dynamic_cast<ChannelTrack *> (tr))
    {
      InspectorPortWidget * ip;
      switch (type)
        {
        case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_CONTROLS:
          ADD_SINGLE (ch_track->channel_->fader_->amp_.get ());
          ADD_SINGLE (ch_track->channel_->fader_->balance_.get ());
          ADD_SINGLE (ch_track->channel_->fader_->mute_.get ());
          break;
        case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_SENDS:
          if (out_type == PortType::Audio)
            {
              ADD_SINGLE (&ch_track->channel_->prefader_->stereo_out_->get_l ());
              ADD_SINGLE (&ch_track->channel_->prefader_->stereo_out_->get_r ());
              ADD_SINGLE (&ch_track->channel_->fader_->stereo_out_->get_l ());
              ADD_SINGLE (&ch_track->channel_->fader_->stereo_out_->get_r ());
            }
          else if (out_type == PortType::Event)
            {
              ADD_SINGLE (ch_track->channel_->prefader_->midi_out_.get ());
              ADD_SINGLE (ch_track->channel_->fader_->midi_out_.get ());
            }
          break;
          break;
        case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_STEREO_IN:
          ADD_SINGLE (&ch_track->processor_->stereo_in_->get_l ());
          ADD_SINGLE (&ch_track->processor_->stereo_in_->get_r ());
          break;
        case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_MIDI_IN:
          ADD_SINGLE (ch_track->processor_->midi_in_.get ());
          break;
        case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_MIDI_OUT:
          ADD_SINGLE (ch_track->channel_->midi_out_.get ());
          break;
        }
    }

#undef ADD_SINGLE
}

static void
ports_expander_widget_class_init (PortsExpanderWidgetClass * klass)
{
}

static void
ports_expander_widget_init (PortsExpanderWidget * self)
{
  /*two_col_expander_box_widget_add_single (*/
  /*Z_TWO_COL_EXPANDER_BOX_WIDGET (self),*/
  /*GTK_WIDGET (self->name));*/
}
