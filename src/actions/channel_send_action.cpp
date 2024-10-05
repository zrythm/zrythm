// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/channel_send_action.h"
#include "dsp/channel.h"
#include "dsp/port_identifier.h"
#include "dsp/router.h"
#include "dsp/tracklist.h"
#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "project.h"
#include "utils/rt_thread_id.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

ChannelSendAction::ChannelSendAction (
  Type                           type,
  const ChannelSend             &send,
  const Port *                   port,
  const StereoPorts *            stereo,
  float                          amount,
  const PortConnectionsManager * port_connections_mgr)
    : send_before_ (send.clone_unique ()), amount_ (amount),
      send_action_type_ (type)
{
  ChannelSendAction ();

  if (port)
    midi_id_ = std::make_unique<PortIdentifier> (port->id_);

  if (stereo)
    {
      l_id_ = std::make_unique<PortIdentifier> (stereo->get_l ().id_);
      r_id_ = std::make_unique<PortIdentifier> (stereo->get_r ().id_);
    }

  if (port_connections_mgr)
    {
      port_connections_before_ = port_connections_mgr->clone_unique ();
    }
}

bool
ChannelSendAction::connect_or_disconnect (bool connect, bool do_it)
{
  /* get the actual channel send from the project */
  auto send = send_before_->find_in_project ();

  send->disconnect (false);

  if (do_it)
    {
      if (connect)
        {
          auto track = send->get_track ();
          switch (track->out_signal_type_)
            {
            case PortType::Event:
              {
                auto port = Port::find_from_identifier<MidiPort> (*midi_id_);
                send->connect_midi (*port, false, true);
              }
              break;
            case PortType::Audio:
              {
                auto l = Port::find_from_identifier<AudioPort> (*l_id_);
                auto r = Port::find_from_identifier<AudioPort> (*r_id_);
                send->connect_stereo (
                  nullptr, l, r, send_action_type_ == Type::ConnectSidechain,
                  false, true);
              }
              break;
            default:
              break;
            }
        }
    }
  /* else if not doing */
  else
    {
      /* copy the values - connections will be reverted later when resetting the
       * connections manager */
      send->copy_values_from (*send_before_);
    }

  return true;
}

void
ChannelSendAction::perform_impl ()
{
  /* get the actual channel send from the project */
  ChannelSend * send = send_before_->find_in_project ();

  bool need_restore_and_recalc = false;

  bool successful = false;
  switch (send_action_type_)
    {
    case Type::ConnectMidi:
    case Type::ConnectStereo:
    case Type::ChangePorts:
    case Type::ConnectSidechain:
      successful = connect_or_disconnect (true, true);
      need_restore_and_recalc = true;
      break;
    case Type::Disconnect:
      successful = connect_or_disconnect (false, true);
      need_restore_and_recalc = true;
      break;
    case Type::ChangeAmount:
      successful = true;
      send->set_amount (amount_);
      break;
    default:
      break;
    }

  if (!successful)
    {
      throw ZrythmException ("Channel send operation failed");
    }

  if (need_restore_and_recalc)
    {
      save_or_load_port_connections (true);

      ROUTER->recalc_graph (false);
    }

  EVENTS_PUSH (EventType::ET_CHANNEL_SEND_CHANGED, send);
}

void
ChannelSendAction::undo_impl ()
{
  /* get the actual channel send from the project */
  ChannelSend * send = send_before_->find_in_project ();

  bool need_restore_and_recalc = false;

  bool successful = false;
  switch (send_action_type_)
    {
    case Type::ConnectMidi:
    case Type::ConnectStereo:
    case Type::ConnectSidechain:
      successful = connect_or_disconnect (false, true);
      need_restore_and_recalc = true;
      break;
    case Type::ChangePorts:
    case Type::Disconnect:
      successful = connect_or_disconnect (true, false);
      need_restore_and_recalc = true;
      break;
    case Type::ChangeAmount:
      send->set_amount (send_before_->amount_->control_);
      successful = true;
      break;
    default:
      break;
    }

  if (!successful)
    {
      throw ZrythmException ("Failed to undo channel send action");
    }

  if (need_restore_and_recalc)
    {
      save_or_load_port_connections (false);

      ROUTER->recalc_graph (false);

      TRACKLIST->validate ();
    }

  EVENTS_PUSH (EventType::ET_CHANNEL_SEND_CHANGED, send);
}

std::string
ChannelSendAction::to_string () const
{
  switch (send_action_type_)
    {
    case Type::ConnectSidechain:
      return _ ("Connect sidechain");
    case Type::ConnectStereo:
      return _ ("Connect stereo");
    case Type::ConnectMidi:
      return _ ("Connect MIDI");
    case Type::Disconnect:
      return _ ("Disconnect");
    case Type::ChangeAmount:
      return _ ("Change amount");
    case Type::ChangePorts:
      return _ ("Change ports");
    }
  return _ ("Channel send connection");
}