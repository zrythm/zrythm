// SPDX-FileCopyrightText: © 2020-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/channel_send_action.h"
#include "dsp/channel_send.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/channel_send.h"
#include "gui/widgets/channel_send_selector.h"
#include "gui/widgets/item_factory.h"
#include "plugins/plugin_identifier.h"
#include "project.h"
#include "utils/logger.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ChannelSendSelectorWidget,
  channel_send_selector_widget,
  GTK_TYPE_POPOVER)

static ProcessableTrack *
get_track_from_target (ChannelSendTarget * target)
{
  if (target->type == ChannelSendTargetType::None)
    return nullptr;

  auto ret = dynamic_cast<ProcessableTrack *> (
    TRACKLIST->tracks_[target->track_pos].get ());
  z_return_val_if_fail (ret, nullptr);
  return ret;
}

static std::unique_ptr<StereoPorts>
get_sidechain_from_target (ChannelSendTarget * target)
{
  if (target->type != ChannelSendTargetType::PluginSidechain)
    {
      return nullptr;
    }

  Plugin * pl = Plugin::find (target->pl_id);
  z_return_val_if_fail (pl, nullptr);
  auto l =
    static_cast<AudioPort *> (pl->get_port_in_group (target->port_group, true))
      ->clone_unique ();
  auto r =
    static_cast<AudioPort *> (pl->get_port_in_group (target->port_group, false))
      ->clone_unique ();
  return std::make_unique<StereoPorts> (std::move (l), std::move (r));
}

/**
 * Called when row is double clicked.
 */
static void
on_row_activated (
  GtkListView *               list_view,
  guint                       position,
  ChannelSendSelectorWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self), false);
}

static void
on_selection_changed (
  GtkSelectionModel *         selection_model,
  guint                       position,
  guint                       n_items,
  ChannelSendSelectorWidget * self)
{
  GObject * gobj =
    G_OBJECT (gtk_single_selection_get_selected_item (self->view_model));
  if (!gobj)
    return;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  auto target = std::get<ChannelSendTarget *> (wrapped_obj->obj);

  ChannelSend * send = self->send_widget->send;
  bool          is_empty = send->is_empty ();

  auto                          src_track = send->get_track ();
  ProcessableTrack *            dest_track = nullptr;
  std::optional<PortConnection> conn;
  switch (target->type)
    {
    case ChannelSendTargetType::None:
      if (send->is_enabled ())
        {
          try
            {
              UNDO_MANAGER->perform (
                std::make_unique<ChannelSendDisconnectAction> (
                  *self->send_widget->send, *PORT_CONNECTIONS_MGR));
            }
          catch (const ZrythmException &e)
            {
              e.handle (_ ("Failed to disconnect send"));
            }
        }
      break;
    case ChannelSendTargetType::Track:
      dest_track = get_track_from_target (target);
      switch (src_track->out_signal_type_)
        {
        case PortType::Event:
          if (
            PORT_CONNECTIONS_MGR->get_sources_or_dests (
              nullptr, send->midi_out_->id_, false)
            == 1)
            {
              conn = PORT_CONNECTIONS_MGR->get_source_or_dest (
                send->midi_out_->id_, false);
            }
          if (
            is_empty
            || (conn && conn->dest_id_ != dest_track->processor_->midi_in_->id_))
            {
              try
                {
                  UNDO_MANAGER->perform (
                    std::make_unique<ChannelSendConnectMidiAction> (
                      *send, *dest_track->processor_->midi_in_,
                      *PORT_CONNECTIONS_MGR));
                }
              catch (const ZrythmException &e)
                {
                  e.handle (_ ("Failed to connect send"));
                }
            }
          break;
        case PortType::Audio:
          if (
            PORT_CONNECTIONS_MGR->get_sources_or_dests (
              nullptr, send->stereo_out_->get_l ().id_, false)
            == 1)
            {
              conn = PORT_CONNECTIONS_MGR->get_source_or_dest (
                send->stereo_out_->get_l ().id_, false);
            }
          if (is_empty || (conn && conn->dest_id_ != dest_track->processor_->stereo_in_->get_l().id_))
            {
              try
                {
                  UNDO_MANAGER->perform (
                    std::make_unique<ChannelSendConnectStereoAction> (
                      *send, *dest_track->processor_->stereo_in_,
                      *PORT_CONNECTIONS_MGR));
                }
              catch (const ZrythmException &e)
                {
                  e.handle (_ ("Failed to connect send"));
                }
            }
          break;
        default:
          break;
        }
      break;
    case ChannelSendTargetType::PluginSidechain:
      {
        auto dest_sidechain = get_sidechain_from_target (target);
        if (
          PORT_CONNECTIONS_MGR->get_sources_or_dests (
            nullptr, send->stereo_out_->get_l ().id_, false)
          == 1)
          {
            conn = PORT_CONNECTIONS_MGR->get_source_or_dest (
              send->stereo_out_->get_l ().id_, false);
          }
        if (dest_sidechain && (is_empty || !send->is_sidechain_ || (conn && conn->dest_id_ != dest_sidechain->get_l().id_)))
          {
            try
              {
                UNDO_MANAGER->perform (
                  std::make_unique<ChannelSendConnectSidechainAction> (
                    *send, *dest_sidechain, *PORT_CONNECTIONS_MGR));
              }
            catch (const ZrythmException &e)
              {
                e.handle (_ ("Failed to connect send"));
              }
          }
      }
      break;
    }

  gtk_widget_queue_draw (GTK_WIDGET (self->send_widget));
}

static void
setup_view (ChannelSendSelectorWidget * self)
{
  /* icon, name, pointer to data */
  GListStore * list_store =
    g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  {
    auto * target = new ChannelSendTarget ();
    target->type = ChannelSendTargetType::None;
    WrappedObjectWithChangeSignal * wobj =
      wrapped_object_with_change_signal_new_with_free_func (
        target, WrappedObjectType::WRAPPED_OBJECT_TYPE_CHANNEL_SEND_TARGET,
        ChannelSendTarget::free_func);
    g_list_store_append (list_store, wobj);
  }

  unsigned int select_idx = 0;
  unsigned int count = 1;

  /* setup tracks */
  ChannelSend * send = self->send_widget->send;
  auto          track = send->get_track ();
  for (size_t i = 0; i < TRACKLIST->tracks_.size (); ++i)
    {
      auto target_track = TRACKLIST->tracks_[i].get ();

      z_trace ("target {}", target_track->name_);

      /* skip tracks with non-matching signal types */
      if (
        target_track == track
        || track->out_signal_type_ != target_track->in_signal_type_
        || !Track::type_is_fx (target_track->type_))
        continue;
      z_debug ("adding {}", target_track->name_);

      /* create target */
      auto * target = new ChannelSendTarget ();
      target->type = ChannelSendTargetType::Track;
      target->track_pos = i;

      /* add it to list */
      WrappedObjectWithChangeSignal * wobj =
        wrapped_object_with_change_signal_new_with_free_func (
          target, WrappedObjectType::WRAPPED_OBJECT_TYPE_CHANNEL_SEND_TARGET,
          ChannelSendTarget::free_func);
      g_list_store_append (list_store, wobj);

      if (
        send->is_enabled () && target_track == send->get_target_track (nullptr))
        {
          select_idx = count;
        }
      count++;
    }

  /* setup plugin sidechain inputs */
  z_debug ("setting up sidechains");
  for (auto target_track : TRACKLIST->tracks_ | type_is<ChannelTrack> ())
    {
      if (target_track == track)
        {
          continue;
        }

      auto ch = target_track->channel_.get ();

      std::vector<Plugin *> pls;
      ch->get_plugins (pls);

      for (auto pl : pls)
        {
          z_debug ("plugin {}", pl->get_name ());

          for (auto &port : pl->in_ports_)
            {
              z_debug ("port {}", port->get_label ());

              if (
                !(ENUM_BITSET_TEST (
                  PortIdentifier::Flags, port->id_.flags_,
                  PortIdentifier::Flags::Sidechain))
                || port->id_.type_ != PortType::Audio
                || port->id_.port_group_.empty ()
                || !(ENUM_BITSET_TEST (
                  PortIdentifier::Flags, port->id_.flags_,
                  PortIdentifier::Flags::StereoL)))
                {
                  continue;
                }

              /* find corresponding port in the same port group (e.g., if this
               * is left, find right and vice versa) */
              Port * other_channel = pl->get_port_in_same_group (*port);
              if (!other_channel)
                {
                  continue;
                }
              z_debug ("other channel {}", other_channel->get_label ());

              Port *l = nullptr, *r = NULL;
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, port->id_.flags_,
                  PortIdentifier::Flags::StereoL)
                && ENUM_BITSET_TEST (
                  PortIdentifier::Flags, other_channel->id_.flags_,
                  PortIdentifier::Flags::StereoR))
                {
                  l = port.get ();
                  r = other_channel;
                }
              if (!l || !r)
                {
                  continue;
                }

              /* create target */
              auto * target = new ChannelSendTarget ();
              target->type = ChannelSendTargetType::PluginSidechain;
              target->track_pos = target_track->pos_;
              target->pl_id = pl->id_;
              target->port_group = g_strdup (l->id_.port_group_.c_str ());

              /* add it to list */
              WrappedObjectWithChangeSignal * wobj =
                wrapped_object_with_change_signal_new_with_free_func (
                  target,
                  WrappedObjectType::WRAPPED_OBJECT_TYPE_CHANNEL_SEND_TARGET,
                  ChannelSendTarget::free_func);
              g_list_store_append (list_store, wobj);

              if (send->is_target_sidechain ())
                {
                  auto sp = send->get_target_sidechain ();
                  if (sp->get_l ().id_ == l->id_ && sp->get_r ().id_ == r->id_)
                    {
                      select_idx = count;
                    }
                }
              count++;
            }
        }
    }

  self->view_model = gtk_single_selection_new (G_LIST_MODEL (list_store));
  gtk_list_view_set_model (self->view, GTK_SELECTION_MODEL (self->view_model));
  self->item_factory =
    std::make_unique<ItemFactory> (ItemFactory::Type::IconAndText, false, "");
  gtk_list_view_set_factory (self->view, self->item_factory->list_item_factory_);

  gtk_selection_model_select_item (
    GTK_SELECTION_MODEL (self->view_model), select_idx, true);

  g_signal_connect (
    G_OBJECT (self->view_model), "selection-changed",
    G_CALLBACK (on_selection_changed), self);
}

void
channel_send_selector_widget_setup (ChannelSendSelectorWidget * self)
{
  setup_view (self);
}

ChannelSendSelectorWidget *
channel_send_selector_widget_new (ChannelSendWidget * send)
{
  auto * self = static_cast<ChannelSendSelectorWidget *> (
    g_object_new (CHANNEL_SEND_SELECTOR_WIDGET_TYPE, nullptr));
  self->send_widget = send;

  return self;
}

static void
channel_send_selector_widget_finalize (ChannelSendSelectorWidget * self)
{
  self->item_factory.~unique_ptr<ItemFactory> ();

  G_OBJECT_CLASS (channel_send_selector_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
channel_send_selector_widget_class_init (ChannelSendSelectorWidgetClass * klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) channel_send_selector_widget_finalize;
}

static void
channel_send_selector_widget_init (ChannelSendSelectorWidget * self)
{
  new (&self->item_factory) std::unique_ptr<ItemFactory> ();

  /* create box */
  self->vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_popover_set_child (GTK_POPOVER (self), GTK_WIDGET (self->vbox));

  /* add scroll */
  GtkWidget * scroll = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (
    GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_propagate_natural_height (
    GTK_SCROLLED_WINDOW (scroll), true);
  gtk_scrolled_window_set_max_content_height (GTK_SCROLLED_WINDOW (scroll), 240);
  gtk_box_append (GTK_BOX (self->vbox), scroll);

  /* add view */
  self->view = GTK_LIST_VIEW (gtk_list_view_new (nullptr, nullptr));
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (scroll), GTK_WIDGET (self->view));
  g_signal_connect (
    G_OBJECT (self->view), "activate", G_CALLBACK (on_row_activated), self);
}
