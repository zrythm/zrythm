// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2017, 2019 Robin Gareus <robin@gareus.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include "dsp/audio_track.h"
#include "dsp/channel_track.h"
#include "dsp/control_room.h"
#include "dsp/engine.h"
#include "dsp/fader.h"
#include "dsp/graph.h"
#include "dsp/graph_node.h"
#include "dsp/graph_thread.h"
#include "dsp/hardware_processor.h"
#include "dsp/modulator_macro_processor.h"
#include "dsp/modulator_track.h"
#include "dsp/port.h"
#include "dsp/router.h"
#include "dsp/sample_processor.h"
#include "dsp/tempo_track.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/audio.h"
#include "utils/env.h"
#include "utils/flags.h"
#include "utils/mpmc_queue.h"
#include "utils/objects.h"
#include "zrythm.h"

Graph::Graph (Router * router, SampleProcessor * sample_processor)
    : sample_processor_ (sample_processor), router_ (router)
{
  z_return_if_fail (
    (router && !sample_processor) || (!router && sample_processor));
}

bool
Graph::is_valid () const
{
  std::vector<GraphNode *> triggers;
  for (auto trigger : setup_init_trigger_list_)
    {
      triggers.push_back (trigger);
    }

  while (!triggers.empty ())
    {
      auto trigger = triggers.back ();
      triggers.pop_back ();

      for (auto child : trigger->childnodes_)
        {
          trigger->childnodes_.pop_back ();
          child->init_refcount_--;
          if (child->init_refcount_ == 0)
            {
              triggers.push_back (child);
            }
        }
    }

  for (auto &[key, node] : setup_graph_nodes_map_)
    {
      if (!node->childnodes_.empty () || node->init_refcount_ > 0)
        return false;
    }

  return true;
}

void
Graph::clear_setup ()
{
  setup_graph_nodes_map_.clear ();
  setup_init_trigger_list_.clear ();
  setup_terminal_nodes_.clear ();
}

void
Graph::rechain ()
{
  z_return_if_fail (trigger_queue_size_.load () == 0);

  /* --- swap setup nodes with graph nodes --- */

  std::swap (graph_nodes_map_, setup_graph_nodes_map_);

  std::swap (init_trigger_list_, setup_init_trigger_list_);
  std::swap (terminal_nodes_, setup_terminal_nodes_);

  terminal_refcnt_.store (terminal_nodes_.size ());

  trigger_queue_.reserve (graph_nodes_map_.size ());

  clear_setup ();
}

void
Graph::add_plugin (Plugin &pl)
{
  z_return_if_fail (!pl.deleting_);
  if (!pl.in_ports_.empty () || !pl.out_ports_.empty ())
    {
      create_node (GraphNode::Type::Plugin, &pl);
    }
}

void
Graph::connect_plugin (Plugin &pl, bool drop_unnecessary_ports)
{
  z_return_if_fail (!pl.deleting_);
  auto pl_node = find_node_from_plugin (&pl);
  z_return_if_fail (pl_node);
  for (auto &port : pl.in_ports_)
    {
      z_return_if_fail (port->get_plugin (true) != nullptr);
      auto port_node = find_node_from_port (port.get ());
      if (drop_unnecessary_ports && !port_node && port->is_control ())
        {
          continue;
        }
      z_return_if_fail (port_node);
      port_node->connect_to (*pl_node);
    }
  for (auto &port : pl.out_ports_)
    {
      z_return_if_fail (port->get_plugin (true) != nullptr);
      auto port_node = find_node_from_port (port.get ());
      z_warn_if_fail (port_node);
      pl_node->connect_to (*port_node);
    }
}

void
Graph::print () const
{
  z_info ("==printing graph");

  for (const auto &[_, node] : setup_graph_nodes_map_)
    {
      node->print ();
    }

  z_info (
    "num trigger nodes %zu | num terminal nodes %zu",
    setup_init_trigger_list_.size (), setup_terminal_nodes_.size ());
  z_info ("==finish printing graph");
}

Graph::~Graph ()
{
  if (main_thread_ || !threads_.empty ())
    {
      z_info ("terminating graph");
      terminate ();
    }
  else
    {
      z_info ("graph already terminated");
    }
}

GraphNode *
Graph::add_port (Port &port, const bool drop_if_unnecessary)
{
  auto owner = port.id_.owner_type_;

  if (owner == PortIdentifier::OwnerType::Plugin)
    {
      port.plugin_ = port.get_plugin (true);
      z_return_val_if_fail (port.plugin_, nullptr);
    }

  if (port.id_.track_name_hash_ != 0)
    {
      port.track_ = port.get_track (true);
      z_return_val_if_fail (port.track_, nullptr);
    }

  /* reset port sources/dests */
  std::vector<PortConnection *> srcs;
  PORT_CONNECTIONS_MGR->get_sources_or_dests (&srcs, port.id_, true);
  port.srcs_.clear ();
  port.src_connections_.clear ();
  for (auto conn : srcs)
    {
      port.srcs_.push_back (Port::find_from_identifier (conn->src_id_));
      z_return_val_if_fail (port.srcs_.back (), nullptr);
      port.src_connections_.push_back (conn);
    }

  std::vector<PortConnection *> dests;
  PORT_CONNECTIONS_MGR->get_sources_or_dests (&dests, port.id_, false);
  port.dests_.clear ();
  port.dest_connections_.clear ();
  for (auto &conn : dests)
    {
      port.dests_.push_back (Port::find_from_identifier (conn->dest_id_));
      z_return_val_if_fail (port.dests_.back (), nullptr);
      port.dest_connections_.push_back (conn);
    }

  /* skip unnecessary control ports */
  if (
    drop_if_unnecessary && port.is_control ()
    && ENUM_BITSET_TEST (
      PortIdentifier::Flags, port.id_.flags_, PortIdentifier::Flags::Automatable))
    {
      auto &control_port = static_cast<ControlPort &> (port);
      auto  found_at = control_port.at_;
      z_return_val_if_fail (found_at, nullptr);
      if (found_at->regions_.empty () && port.srcs_.empty ())
        {
          return nullptr;
        }
    }

  /* drop ports without sources and dests */
  if (
    drop_if_unnecessary && port.dests_.empty () && port.srcs_.empty ()
    && owner != PortIdentifier::OwnerType::Plugin
    && owner != PortIdentifier::OwnerType::Fader
    && owner != PortIdentifier::OwnerType::TrackProcessor
    && owner != PortIdentifier::OwnerType::Track
    && owner != PortIdentifier::OwnerType::ModulatorMacroProcessor
    && owner != PortIdentifier::OwnerType::Channel
    && owner != PortIdentifier::OwnerType::ChannelSend
    && owner != PortIdentifier::OwnerType::AudioEngine
    && owner != PortIdentifier::OwnerType::HardwareProcessor
    && owner != PortIdentifier::OwnerType::Transport
    && !(ENUM_BITSET_TEST (
      PortIdentifier::Flags, port.id_.flags_,
      PortIdentifier::Flags::ManualPress)))
    {
      return nullptr;
    }
  else
    {
      /* allocate buffers to be used during DSP */
      port.allocate_bufs ();
      return create_node (GraphNode::Type::Port, &port);
    }
}

void
Graph::connect_port (Port &port)
{
  auto node = find_node_from_port (&port);
  for (auto &src : port.srcs_)
    {
      auto node2 = find_node_from_port (src);
      z_warn_if_fail (node);
      z_warn_if_fail (node2);
      node2->connect_to (*node);
    }
  for (auto &dest : port.dests_)
    {
      auto node2 = find_node_from_port (dest);
      z_warn_if_fail (node);
      z_warn_if_fail (node2);
      node->connect_to (*node2);
    }
}

nframes_t
Graph::get_max_route_playback_latency (bool use_setup_nodes)
{
  nframes_t   max = 0;
  const auto &nodes =
    use_setup_nodes ? setup_init_trigger_list_ : init_trigger_list_;
  for (const auto &node : nodes)
    {
      if (node->route_playback_latency_ > max)
        max = node->route_playback_latency_;
    }

  return max;
}

void
Graph::update_latencies (bool use_setup_nodes)
{
  z_info ("updating graph latencies...");

  /* reset latencies */
  auto &nodes = use_setup_nodes ? setup_graph_nodes_map_ : graph_nodes_map_;
  z_debug ("setting all latencies to 0");
  for (auto &[_, node] : nodes)
    {
      node->playback_latency_ = 0;
      node->route_playback_latency_ = 0;
    }
  z_debug ("done setting all latencies to 0");

  z_debug ("iterating over %zu nodes...", nodes.size ());
  for (auto &[_, node] : nodes)
    {
      node->playback_latency_ = node->get_single_playback_latency ();
      if (node->playback_latency_ > 0)
        {
          node->set_route_playback_latency (node->playback_latency_);
        }
    }
  z_debug ("iterating done...");

  z_info (
    "Total latencies:\n"
    "Playback: %ld\n"
    "Recording: %d\n",
    get_max_route_playback_latency (use_setup_nodes), 0);
}

void
Graph::setup (const bool drop_unnecessary_ports, const bool rechain)
{
  /* ========================
   * first add all the nodes
   * ======================== */

  /* add the sample processor */
  create_node (GraphNode::Type::SampleProcessor, SAMPLE_PROCESSOR.get ());

  /* add the monitor fader */
  create_node (GraphNode::Type::MonitorFader, MONITOR_FADER.get ());

  /* add the initial processor */
  create_node (GraphNode::Type::InitialProcessor, &initial_processor_);

  /* add the hardware input processor */
  create_node (GraphNode::Type::HardwareProcessor, HW_IN_PROCESSOR.get ());

  /* add plugins */
  for (const auto &tr : TRACKLIST->tracks_)
    {
      z_return_if_fail (tr);

      /* add the track */
      create_node (GraphNode::Type::Track, tr.get ());

      /* handle modulator track */
      if (auto modulator_track = dynamic_cast<ModulatorTrack *> (tr.get ()))
        {
          /* add plugins */
          for (auto &pl : modulator_track->modulators_)
            {
              if (!pl || pl->deleting_)
                continue;

              add_plugin (*pl);
              pl->update_latency ();

              /* add macro processors */
              for (auto &mp : modulator_track->modulator_macro_processors_)
                {
                  create_node (
                    GraphNode::Type::ModulatorMacroProcessor, mp.get ());
                }
            }
        }

      if (tr->has_channel ())
        {
          auto  channel_track = dynamic_cast<ChannelTrack *> (tr.get ());
          auto &channel = channel_track->channel_;

          /* add the fader */
          create_node (GraphNode::Type::Fader, channel->fader_.get ());

          /* add the prefader */
          create_node (GraphNode::Type::Prefader, channel->prefader_.get ());

          /* add plugins */
          std::vector<Plugin *> plugins;
          channel->get_plugins (plugins);
          for (auto pl : plugins)
            {
              if (!pl || pl->deleting_)
                continue;

              add_plugin (*pl);
              pl->update_latency ();
            }

          /* add sends */
          if (
            tr->out_signal_type_ == PortType::Audio
            || tr->out_signal_type_ == PortType::Event)
            {
              for (auto &send : channel->sends_)
                {
                  /* note that we add sends even if empty so that graph
                   * renders properly */
                  create_node (GraphNode::Type::ChannelSend, send.get ());
                }
            }
        }
    }

  external_out_ports_.clear ();

  /* add ports */
  std::vector<Port *> ports;
  PROJECT->get_all_ports (ports);
  for (auto port : ports)
    {
      z_return_if_fail (port);
      if (port->deleting_ || (port->id_.owner_type_ == PortIdentifier::OwnerType::Plugin && port->get_plugin(true)->deleting_))
        continue;

      if (port->id_.flow_ == PortFlow::Output && port->is_exposed_to_backend ())
        {
          external_out_ports_.push_back (port);
        }

      add_port (*port, drop_unnecessary_ports);
    }

  /* ========================
   * now connect them
   * ======================== */

  /* connect the sample processor */
  {
    auto node = find_node_from_sample_processor (SAMPLE_PROCESSOR.get ());
    auto port = &SAMPLE_PROCESSOR->fader_->stereo_out_->get_l ();
    auto node2 = find_node_from_port (port);
    node->connect_to (*node2);
    port = &SAMPLE_PROCESSOR->fader_->stereo_out_->get_r ();
    node2 = find_node_from_port (port);
    node->connect_to (*node2);
  }

  /* connect the monitor fader */
  {
    auto node = find_node_from_monitor_fader (MONITOR_FADER.get ());
    auto port = &MONITOR_FADER->stereo_in_->get_l ();
    auto node2 = find_node_from_port (port);
    node2->connect_to (*node);
    port = &MONITOR_FADER->stereo_in_->get_r ();
    node2 = find_node_from_port (port);
    node2->connect_to (*node);
    port = &MONITOR_FADER->stereo_out_->get_l ();
    node2 = find_node_from_port (port);
    node->connect_to (*node2);
    port = &MONITOR_FADER->stereo_out_->get_r ();
    node2 = find_node_from_port (port);
    node->connect_to (*node2);
  }

  GraphNode * initial_processor_node = find_initial_processor_node ();

  /* connect the HW input processor */
  GraphNode * hw_processor_node =
    find_hw_processor_node (HW_IN_PROCESSOR.get ());
  for (auto &port : HW_IN_PROCESSOR->audio_ports_)
    {
      auto node2 = find_node_from_port (port.get ());
      z_warn_if_fail (node2);
      hw_processor_node->connect_to (*node2);
    }
  for (auto &port : HW_IN_PROCESSOR->midi_ports_)
    {
      auto node2 = find_node_from_port (port.get ());
      hw_processor_node->connect_to (*node2);
    }

  /* connect MIDI editor manual press */
  {
    auto node2 =
      find_node_from_port (AUDIO_ENGINE->midi_editor_manual_press_.get ());
    node2->connect_to (*initial_processor_node);
  }

  /* connect the transport ports */
  {
    auto node = find_node_from_port (TRANSPORT->roll_.get ());
    node->connect_to (*initial_processor_node);
    node = find_node_from_port (TRANSPORT->stop_.get ());
    node->connect_to (*initial_processor_node);
    node = find_node_from_port (TRANSPORT->backward_.get ());
    node->connect_to (*initial_processor_node);
    node = find_node_from_port (TRANSPORT->forward_.get ());
    node->connect_to (*initial_processor_node);
    node = find_node_from_port (TRANSPORT->loop_toggle_.get ());
    node->connect_to (*initial_processor_node);
    node = find_node_from_port (TRANSPORT->rec_toggle_.get ());
    node->connect_to (*initial_processor_node);
  }

  /* connect tracks */
  for (auto &tr : TRACKLIST->tracks_)
    {
      /* connect the track */
      auto node = find_node_from_track (tr.get (), true);
      if (tr->in_signal_type_ == PortType::Audio)
        {
          if (tr->is_audio ())
            {
              auto audio_track = dynamic_cast<AudioTrack *> (tr.get ());
              auto node2 =
                find_node_from_port (audio_track->processor_->mono_.get ());
              node2->connect_to (*node);
              initial_processor_node->connect_to (*node2);
              node2 = find_node_from_port (
                audio_track->processor_->input_gain_.get ());
              node2->connect_to (*node);
              initial_processor_node->connect_to (*node2);
            }

          auto processable_track = dynamic_cast<ProcessableTrack *> (tr.get ());
          z_return_if_fail (processable_track);
          auto track_processor = processable_track->processor_.get ();
          auto node2 =
            find_node_from_port (&track_processor->stereo_in_->get_l ());
          node2->connect_to (*node);
          initial_processor_node->connect_to (*node2);
          node2 = find_node_from_port (&track_processor->stereo_in_->get_r ());
          node2->connect_to (*node);
          initial_processor_node->connect_to (*node2);
          node2 = find_node_from_port (&track_processor->stereo_out_->get_l ());
          node->connect_to (*node2);
          node2 = find_node_from_port (&track_processor->stereo_out_->get_r ());
          node->connect_to (*node2);
        }
      else if (tr->in_signal_type_ == PortType::Event)
        {
          auto processable_track = dynamic_cast<ProcessableTrack *> (tr.get ());
          z_return_if_fail (processable_track);
          auto track_processor = processable_track->processor_.get ();

          if (tr->has_piano_roll () || tr->is_chord ())
            {
              /* connect piano roll */
              auto node2 =
                find_node_from_port (track_processor->piano_roll_.get ());
              node2->connect_to (*node);
            }

          auto node2 = find_node_from_port (track_processor->midi_in_.get ());
          node2->connect_to (*node);
          initial_processor_node->connect_to (*node2);
          node2 = find_node_from_port (track_processor->midi_out_.get ());
          node->connect_to (*node2);
        }

      if (tr->has_piano_roll ())
        {
          auto processable_track = dynamic_cast<ProcessableTrack *> (tr.get ());
          z_return_if_fail (processable_track);
          auto track_processor = processable_track->processor_.get ();

          for (int j = 0; j < 16; j++)
            {
              for (int k = 0; k < 128; k++)
                {
                  auto node2 = find_node_from_port (
                    track_processor->midi_cc_[j * 128 + k].get ());
                  if (node2)
                    {
                      node2->connect_to (*node);
                    }
                }

              auto node2 =
                find_node_from_port (track_processor->pitch_bend_[j].get ());
              if (node2 || !drop_unnecessary_ports)
                {
                  node2->connect_to (*node);
                }

              node2 = find_node_from_port (
                track_processor->poly_key_pressure_[j].get ());
              if (node2 || !drop_unnecessary_ports)
                {
                  node2->connect_to (*node);
                }

              node2 = find_node_from_port (
                track_processor->channel_pressure_[j].get ());
              if (node2 || !drop_unnecessary_ports)
                {
                  node2->connect_to (*node);
                }
            }
        }

      if (tr->is_tempo ())
        {
          bpm_node_ = nullptr;
          beats_per_bar_node_ = nullptr;
          beat_unit_node_ = nullptr;

          auto tempo_track = dynamic_cast<TempoTrack *> (tr.get ());

          auto node2 = find_node_from_port (tempo_track->bpm_port_.get ());
          if (node2 || !drop_unnecessary_ports)
            {
              bpm_node_ = node2;
              node2->connect_to (*node);
            }
          node2 = find_node_from_port (tempo_track->beats_per_bar_port_.get ());
          if (node2 || !drop_unnecessary_ports)
            {
              beats_per_bar_node_ = node2;
              node2->connect_to (*node);
            }
          node2 = find_node_from_port (tempo_track->beat_unit_port_.get ());
          if (node2 || !drop_unnecessary_ports)
            {
              beat_unit_node_ = node2;
              node2->connect_to (*node);
            }
          node->connect_to (*initial_processor_node);
        }

      if (tr->is_modulator ())
        {
          initial_processor_node->connect_to (*node);

          auto mod_track = dynamic_cast<ModulatorTrack *> (tr.get ());
          for (auto &pl : mod_track->modulators_)
            {
              if (pl && !pl->deleting_)
                {
                  connect_plugin (*pl, drop_unnecessary_ports);
                  for (auto &pl_port : pl->in_ports_)
                    {
                      z_return_if_fail (pl_port->get_plugin (true) != nullptr);
                      auto port_node = find_node_from_port (pl_port.get ());
                      if (
                        drop_unnecessary_ports && !port_node
                        && pl_port->is_control ())
                        {
                          continue;
                        }
                      if (!port_node)
                        {
                          z_error (
                            "failed to find node for port {}",
                            pl_port->get_label ());
                          return;
                        }
                      node->connect_to (*port_node);
                    }
                }
            }

          /* connect the modulator macro processors */
          for (auto &mmp : mod_track->modulator_macro_processors_)
            {
              auto mmp_node =
                find_node_from_modulator_macro_processor (mmp.get ());

              auto node2 = find_node_from_port (mmp->cv_in_.get ());
              node2->connect_to (*mmp_node);
              node2 = find_node_from_port (mmp->macro_.get ());
              if (node2 || !drop_unnecessary_ports)
                {
                  node2->connect_to (*mmp_node);
                }
              node2 = find_node_from_port (mmp->cv_out_.get ());
              mmp_node->connect_to (*node2);
            }
        }

      if (!tr->has_channel ())
        continue;

      auto  channel_track = dynamic_cast<ChannelTrack *> (tr.get ());
      auto &ch = channel_track->channel_;
      auto &prefader = ch->prefader_;
      auto &fader = ch->fader_;

      /* connect the fader */
      node = find_node_from_fader (fader.get ());
      z_warn_if_fail (node);
      if (fader->type_ == Fader::Type::AudioChannel)
        {
          /* connect ins */
          auto node2 = find_node_from_port (&fader->stereo_in_->get_l ());
          node2->connect_to (*node);
          node2 = find_node_from_port (&fader->stereo_in_->get_r ());
          node2->connect_to (*node);

          /* connect outs */
          node2 = find_node_from_port (&fader->stereo_out_->get_l ());
          node->connect_to (*node2);
          node2 = find_node_from_port (&fader->stereo_out_->get_r ());
          node->connect_to (*node2);
        }
      else if (fader->type_ == Fader::Type::MidiChannel)
        {
          auto node2 = find_node_from_port (fader->midi_in_.get ());
          node2->connect_to (*node);
          node2 = find_node_from_port (fader->midi_out_.get ());
          node->connect_to (*node2);
        }
      auto node2 = find_node_from_port (fader->amp_.get ());
      if (node2 || !drop_unnecessary_ports)
        {
          node2->connect_to (*node);
        }
      node2 = find_node_from_port (fader->balance_.get ());
      if (node2 || !drop_unnecessary_ports)
        {
          node2->connect_to (*node);
        }
      node2 = find_node_from_port (fader->mute_.get ());
      if (node2 || !drop_unnecessary_ports)
        {
          node2->connect_to (*node);
        }

      /* connect the prefader */
      node = find_node_from_prefader (prefader.get ());
      z_warn_if_fail (node);
      if (prefader->type_ == Fader::Type::AudioChannel)
        {
          node2 = find_node_from_port (&prefader->stereo_in_->get_l ());
          node2->connect_to (*node);
          node2 = find_node_from_port (&prefader->stereo_in_->get_r ());
          node2->connect_to (*node);
          node2 = find_node_from_port (&prefader->stereo_out_->get_l ());
          node->connect_to (*node2);
          node2 = find_node_from_port (&prefader->stereo_out_->get_r ());
          node->connect_to (*node2);
        }
      else if (prefader->type_ == Fader::Type::MidiChannel)
        {
          node2 = find_node_from_port (prefader->midi_in_.get ());
          node2->connect_to (*node);
          node2 = find_node_from_port (prefader->midi_out_.get ());
          node->connect_to (*node2);
        }

      std::vector<Plugin *> plugins;
      ch->get_plugins (plugins);
      for (auto pl : plugins)
        {
          if (pl && !pl->deleting_)
            {
              connect_plugin (*pl, drop_unnecessary_ports);
            }
        }

      for (auto &send : ch->sends_)
        {
          /* note that we do not skip empty sends because then port connection
           * validation will not detect invalid connections properly */
          node = find_node_from_channel_send (send.get ());

          node2 = find_node_from_port (send->amount_.get ());
          if (node2)
            node2->connect_to (*node);
          node2 = find_node_from_port (send->enabled_.get ());
          if (node2)
            node2->connect_to (*node);

          if (tr->out_signal_type_ == PortType::Event)
            {
              node2 = find_node_from_port (send->midi_in_.get ());
              node2->connect_to (*node);
              node2 = find_node_from_port (send->midi_out_.get ());
              node->connect_to (*node2);
            }
          else if (tr->out_signal_type_ == PortType::Audio)
            {
              node2 = find_node_from_port (&send->stereo_in_->get_l ());
              node2->connect_to (*node);
              node2 = find_node_from_port (&send->stereo_in_->get_r ());
              node2->connect_to (*node);
              node2 = find_node_from_port (&send->stereo_out_->get_l ());
              node->connect_to (*node2);
              node2 = find_node_from_port (&send->stereo_out_->get_r ());
              node->connect_to (*node2);
            }
        }
    }

  for (auto &port : ports)
    {
      if (port->deleting_) [[unlikely]]
        continue;
      if (port->id_.owner_type_ == PortIdentifier::OwnerType::Plugin)
        {
          auto port_pl = port->get_plugin (true);
          if (port_pl->deleting_)
            continue;
        }

      connect_port (*port);
    }

  /* ========================
   * set initial and terminal nodes
   * ======================== */

  setup_terminal_nodes_.clear ();
  setup_init_trigger_list_.clear ();
  for (const auto &[key, node] : setup_graph_nodes_map_)
    {
      if (node->childnodes_.empty ())
        {
          /* terminal node */
          node->terminal_ = true;
          setup_terminal_nodes_.push_back (node.get ());
        }
      if (node->init_refcount_ == 0)
        {
          /* initial node */
          node->initial_ = true;
          setup_init_trigger_list_.push_back (node.get ());
        }
    }

  /* ========================
   * calculate latencies of each port and each processor
   * ======================== */

  update_latencies (true);

  /* ========================
   * set up caches to tracks, channels, plugins, automation tracks, etc.
   *
   * this is because indices can be changed by the GUI thread while the graph is
   * running TODO or maybe not needed since there is a lock now
   * ======================== */

  if (sample_processor_)
    {
      SAMPLE_PROCESSOR->tracklist_->set_caches (ALL_CACHE_TYPES);
    }
  else
    {
      CLIP_EDITOR->set_caches ();
      TRACKLIST->set_caches (ALL_CACHE_TYPES);
    }

  if (rechain)
    this->rechain ();
}

bool
Graph::validate_with_connection (const Port * src, const Port * dest)
{
  z_return_val_if_fail (src && dest, false);

  AudioEngine::State state;
  AUDIO_ENGINE->wait_for_pause (state, false, true);

  z_debug ("validating for {} to {}", src->get_label (), dest->get_label ());

  setup (false, false);

  /* connect the src/dest if not NULL */
  /* this code is only for creating graphs to test if the connection between
   * src->dest is valid */
  auto node = find_node_from_port (src);
  z_return_val_if_fail (node, false);
  auto node2 = find_node_from_port (dest);
  z_return_val_if_fail (node2, false);
  node->connect_to (*node2);
  z_return_val_if_fail (!node->terminal_, false);
  z_return_val_if_fail (!node2->initial_, false);

  bool valid = is_valid ();

  z_debug ("valid %d", valid);

  AUDIO_ENGINE->resume (state);

  return valid;
}

void
Graph::start ()
{
  int num_cores = std::min (MAX_GRAPH_THREADS, audio_get_num_cores ());

  /* we reserve 1 core for the OS and other tasks and 1 core for the main thread
   */
  auto num_threads = env_get_int ("ZRYTHM_DSP_THREADS", num_cores - 2);
  if (num_threads < 0)
    {
      throw ZrythmException ("number of threads must be >= 0");
    }

  auto create_thread = [&] (auto &thread_ptr, auto is_main, auto idx) {
    try
      {
        thread_ptr = std::make_unique<GraphThread> (idx, is_main, *this);
      }
    catch (const ZrythmException &e)
      {
        terminate ();
        throw ZrythmException (
          "failed to create thread: " + std::string (e.what ()));
      }
  };

  /* create worker threads */
  for (int i = 0; i < num_threads; ++i)
    {
      create_thread (threads_[i], false, i);
    }

  /* and the main thread */
  create_thread (main_thread_, true, -1);

  /* wait for all threads to go idle */
  while (idle_thread_cnt_.load () != static_cast<int> (threads_.size ()))
    {
      /* wait for all threads to go idle */
      z_info ("waiting for threads to go idle after creation...");
      std::this_thread::sleep_for (std::chrono::milliseconds (1));
    }
}

void
Graph::terminate ()
{
  g_message ("terminating graph...");

  /* Flag threads to terminate */
  for (auto &thread : threads_)
    {
      thread->signalThreadShouldExit ();
    }
  main_thread_->signalThreadShouldExit ();

  /* wait for all threads to go idle */
  while (idle_thread_cnt_.load () != static_cast<int> (threads_.size ()))
    {
      z_info ("waiting for threads to go idle...");
      std::this_thread::sleep_for (std::chrono::milliseconds (1));
    }

  /* wake-up sleeping threads */
  int tc = idle_thread_cnt_.load ();
  if (tc != static_cast<int> (threads_.size ()))
    {
      z_warning ("expected {} idle threads, found {}", threads_.size (), tc);
    }
  for (int i = 0; i < tc; ++i)
    {
      trigger_sem_.release ();
    }

  /* and the main thread */
  callback_start_sem_.release ();

  /* join threads */
  threads_.clear ();
  main_thread_.reset ();

  z_info ("graph terminated");
}

GraphNode *
Graph::find_node_from_port (const Port * port) const
{
  auto it = setup_graph_nodes_map_.find (const_cast<Port *> (port));
  if (
    it != setup_graph_nodes_map_.end ()
    && it->second->type_ == GraphNode::Type::Port)
    {
      return it->second.get ();
    }
  return nullptr;
}

GraphNode *
Graph::find_node_from_plugin (const Plugin * pl) const
{
  auto it = setup_graph_nodes_map_.find (const_cast<Plugin *> (pl));
  if (
    it != setup_graph_nodes_map_.end ()
    && it->second->type_ == GraphNode::Type::Plugin)
    {
      return it->second.get ();
    }
  return nullptr;
}

GraphNode *
Graph::find_node_from_track (const Track * track, bool use_setup_nodes) const
{
  const auto &nodes =
    use_setup_nodes ? setup_graph_nodes_map_ : graph_nodes_map_;
  auto it = nodes.find (const_cast<Track *> (track));
  if (it != nodes.end () && it->second->type_ == GraphNode::Type::Track)
    {
      return it->second.get ();
    }
  return nullptr;
}

GraphNode *
Graph::find_node_from_fader (const Fader * fader) const
{
  auto it = setup_graph_nodes_map_.find (const_cast<Fader *> (fader));
  if (
    it != setup_graph_nodes_map_.end ()
    && it->second->type_ == GraphNode::Type::Fader)
    {
      return it->second.get ();
    }
  return nullptr;
}

GraphNode *
Graph::find_node_from_prefader (const Fader * prefader) const
{
  auto it = setup_graph_nodes_map_.find (const_cast<Fader *> (prefader));
  if (
    it != setup_graph_nodes_map_.end ()
    && it->second->type_ == GraphNode::Type::Prefader)
    {
      return it->second.get ();
    }
  return nullptr;
}

GraphNode *
Graph::find_node_from_sample_processor (
  const SampleProcessor * sample_processor) const
{
  auto it = setup_graph_nodes_map_.find (
    const_cast<SampleProcessor *> (sample_processor));
  if (
    it != setup_graph_nodes_map_.end ()
    && it->second->type_ == GraphNode::Type::SampleProcessor)
    {
      return it->second.get ();
    }
  return nullptr;
}

GraphNode *
Graph::find_node_from_monitor_fader (const Fader * fader) const
{
  auto it = setup_graph_nodes_map_.find (const_cast<Fader *> (fader));
  if (
    it != setup_graph_nodes_map_.end ()
    && it->second->type_ == GraphNode::Type::MonitorFader)
    {
      return it->second.get ();
    }
  return nullptr;
}

GraphNode *
Graph::find_node_from_channel_send (const ChannelSend * send) const
{
  auto it = setup_graph_nodes_map_.find (const_cast<ChannelSend *> (send));
  if (
    it != setup_graph_nodes_map_.end ()
    && it->second->type_ == GraphNode::Type::ChannelSend)
    {
      return it->second.get ();
    }
  return nullptr;
}

GraphNode *
Graph::find_initial_processor_node () const
{
  auto it = setup_graph_nodes_map_.find (
    const_cast<void *> (static_cast<const void *> (&initial_processor_)));
  if (
    it != setup_graph_nodes_map_.end ()
    && it->second->type_ == GraphNode::Type::InitialProcessor)
    {
      return it->second.get ();
    }
  return nullptr;
}

GraphNode *
Graph::find_hw_processor_node (const HardwareProcessor * processor) const
{
  auto it =
    setup_graph_nodes_map_.find (const_cast<HardwareProcessor *> (processor));
  if (
    it != setup_graph_nodes_map_.end ()
    && it->second->type_ == GraphNode::Type::HardwareProcessor)
    {
      return it->second.get ();
    }
  return nullptr;
}

GraphNode *
Graph::find_node_from_modulator_macro_processor (
  const ModulatorMacroProcessor * processor) const
{
  auto it = setup_graph_nodes_map_.find (
    const_cast<ModulatorMacroProcessor *> (processor));
  if (
    it != setup_graph_nodes_map_.end ()
    && it->second->type_ == GraphNode::Type::ModulatorMacroProcessor)
    {
      return it->second.get ();
    }
  return nullptr;
}

/**
 * Creates a new node, adds it to the graph and returns it.
 */
GraphNode *
Graph::create_node (GraphNode::Type type, void * data)
{
  auto [it, inserted] = setup_graph_nodes_map_.emplace (
    data, std::make_unique<GraphNode> (this, type, data));
  return it->second.get ();
}

#if 0
GraphThread *
Graph::get_current_thread () const
{
  for (auto &thread : threads_)
    {
      if (thread->rt_thread_id_ == current_thread_id.get ())
        {
          return thread.get ();
        }
    }
  if (main_thread_->rt_thread_id_ == current_thread_id.get ())
    {
      return main_thread_.get ();
    }
  return nullptr;
}
#endif