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

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/audio_track.h"
#include "gui/dsp/channel_track.h"
#include "gui/dsp/control_room.h"
#include "gui/dsp/engine.h"
#include "gui/dsp/fader.h"
#include "gui/dsp/graph.h"
#include "gui/dsp/graph_thread.h"
#include "gui/dsp/hardware_processor.h"
#include "gui/dsp/modulator_macro_processor.h"
#include "gui/dsp/modulator_track.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/port.h"
#include "gui/dsp/router.h"
#include "gui/dsp/sample_processor.h"
#include "gui/dsp/tempo_track.h"
#include "gui/dsp/track.h"
#include "gui/dsp/tracklist.h"
#include "utils/audio.h"
#include "utils/env.h"
#include "utils/flags.h"
#include "utils/mpmc_queue.h"

using namespace zrythm;

Graph::Graph (Router * router, SampleProcessor * sample_processor)
    : router_ (router), sample_processor_ (sample_processor)
{
  z_return_if_fail (
    (router && !sample_processor) || (!router && sample_processor));
}

void
Graph::trigger_node (GraphNode &node)
{
  /* check if we can run */
  if (node.refcount_.fetch_sub (1) == 1)
    {
      /* reset reference count for next cycle */
      node.refcount_.store (node.init_refcount_);

      // FIXME: is the code below correct? seems like it would cause data races
      // since we are increasing the size but the pointer might not be pushed in
      // the queue yet?

      /* all nodes that feed this node have completed, so this node be
       * processed now. */
      trigger_queue_size_.fetch_add (1);
      /*z_info ("triggering node, pushing back");*/
      trigger_queue_.push_back (&node);
    }
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

  for (auto &node : setup_graph_nodes_vector_)
    {
      if (!node->childnodes_.empty () || node->init_refcount_ > 0)
        return false;
    }

  return true;
}

void
Graph::clear_setup ()
{
  setup_graph_nodes_vector_.clear ();
  setup_init_trigger_list_.clear ();
  setup_terminal_nodes_.clear ();
}

void
Graph::rechain ()
{
  z_return_if_fail (trigger_queue_size_.load () == 0);

  /* --- swap setup nodes with graph nodes --- */

  std::swap (graph_nodes_vector_, setup_graph_nodes_vector_);

  std::swap (init_trigger_list_, setup_init_trigger_list_);
  std::swap (terminal_nodes_, setup_terminal_nodes_);

  terminal_refcnt_.store (terminal_nodes_.size ());

  trigger_queue_.reserve (graph_nodes_vector_.size ());

  clear_setup ();
}

void
Graph::add_plugin (zrythm::gui::old_dsp::plugins::Plugin &pl)
{
  z_return_if_fail (!pl.deleting_);
  auto pl_var =
    convert_to_variant<zrythm::gui::old_dsp::plugins::PluginPtrVariant> (&pl);
  std::visit (
    [this] (auto &&plugin) {
      if (!plugin->in_ports_.empty () || !plugin->out_ports_.empty ())
        {
          add_node_for_processable (*plugin);
        }
    },
    pl_var);
}

void
Graph::connect_plugin (
  zrythm::gui::old_dsp::plugins::Plugin &pl,
  bool                                   drop_unnecessary_ports)
{
  z_return_if_fail (!pl.deleting_);
  auto * pl_node = find_node_for_processable (pl);
  z_return_if_fail (pl_node);
  for (auto &port : pl.in_ports_)
    {
      z_return_if_fail (port->get_plugin (true) != nullptr);
      auto * port_node = find_node_for_processable (*port);
      if (
        drop_unnecessary_ports && (port_node == nullptr) && port->is_control ())
        {
          continue;
        }
      z_return_if_fail (port_node);
      port_node->connect_to (*pl_node);
    }
  for (auto &port : pl.out_ports_)
    {
      z_return_if_fail (port->get_plugin (true) != nullptr);
      auto * port_node = find_node_for_processable (*port);
      z_return_if_fail (port_node);
      pl_node->connect_to (*port_node);
    }
}

void
Graph::print () const
{
  z_info ("==printing graph");

  for (const auto &node : setup_graph_nodes_vector_)
    {
      node->print_node ();
    }

  z_info (
    "num trigger nodes {} | num terminal nodes {}",
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

Graph::GraphNode *
Graph::add_port (
  PortPtrVariant          port_var,
  PortConnectionsManager &mgr,
  const bool              drop_if_unnecessary)
{
  return std::visit (
    [&] (auto &&port) -> GraphNode * {
      using PortT = base_type<decltype (port)>;
      auto owner = port->id_->owner_type_;

      if (owner == dsp::PortIdentifier::OwnerType::Plugin)
        {
          port->plugin_ = port->get_plugin (true);
          z_return_val_if_fail (port->plugin_, nullptr);
        }

      if (port->id_->track_name_hash_ != 0)
        {
          port->track_ = port->get_track (true);
          z_return_val_if_fail (port->track_, nullptr);
        }

      /* reset port sources/dests */
      PortConnectionsManager::ConnectionsVector srcs;
      mgr.get_sources_or_dests (&srcs, *port->id_, true);
      port->srcs_.clear ();
      port->src_connections_.clear ();
      for (const auto &conn : srcs)
        {
          port->srcs_.push_back (Port::find_from_identifier (*conn->src_id_));
          z_return_val_if_fail (port->srcs_.back (), nullptr);
          port->src_connections_.emplace_back (conn->clone_unique ());
        }

      PortConnectionsManager::ConnectionsVector dests;
      mgr.get_sources_or_dests (&dests, *port->id_, false);
      port->dests_.clear ();
      port->dest_connections_.clear ();
      for (const auto &conn : dests)
        {
          port->dests_.push_back (Port::find_from_identifier (*conn->dest_id_));
          z_return_val_if_fail (port->dests_.back (), nullptr);
          port->dest_connections_.emplace_back (conn->clone_unique ());
        }

      /* skip unnecessary control ports */
      if constexpr (std::is_same_v<PortT, ControlPort>)
        {
          if (
            drop_if_unnecessary
            && ENUM_BITSET_TEST (
              dsp::PortIdentifier::Flags, port->id_->flags_,
              dsp::PortIdentifier::Flags::Automatable))
            {
              auto found_at = port->at_;
              z_return_val_if_fail (found_at, nullptr);
              if (
                found_at->region_list_->regions_.empty ()
                && port->srcs_.empty ())
                {
                  return nullptr;
                }
            }
        }

      /* drop ports without sources and dests */
      if (
        drop_if_unnecessary && port->dests_.empty () && port->srcs_.empty ()
        && owner != dsp::PortIdentifier::OwnerType::Plugin
        && owner != dsp::PortIdentifier::OwnerType::Fader
        && owner != dsp::PortIdentifier::OwnerType::TrackProcessor
        && owner != dsp::PortIdentifier::OwnerType::Track
        && owner != dsp::PortIdentifier::OwnerType::ModulatorMacroProcessor
        && owner != dsp::PortIdentifier::OwnerType::Channel
        && owner != dsp::PortIdentifier::OwnerType::ChannelSend
        && owner != dsp::PortIdentifier::OwnerType::AudioEngine
        && owner != dsp::PortIdentifier::OwnerType::HardwareProcessor
        && owner != dsp::PortIdentifier::OwnerType::Transport
        && !(ENUM_BITSET_TEST (
          dsp::PortIdentifier::Flags, port->id_->flags_,
          dsp::PortIdentifier::Flags::ManualPress)))
        {
          return nullptr;
        }

      /* allocate buffers to be used during DSP */
      port->allocate_bufs ();

      return add_node_for_processable (*port);
    },
    port_var);
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
  auto &nodes =
    use_setup_nodes ? setup_graph_nodes_vector_ : graph_nodes_vector_;
  z_debug ("setting all latencies to 0");
  for (auto &node : nodes)
    {
      node->playback_latency_ = 0;
      node->route_playback_latency_ = 0;
    }
  z_debug ("done setting all latencies to 0");

  z_debug ("iterating over {} nodes...", nodes.size ());
  for (auto &node : nodes)
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
    "Playback: {}\n"
    "Recording: {}\n",
    get_max_route_playback_latency (use_setup_nodes), 0);
}

void
Graph::setup (const bool drop_unnecessary_ports, const bool rechain)
{
  /* ========================
   * first add all the nodes
   * ======================== */

  auto * engine = router_->audio_engine_;
  auto * project = engine->project_;
  auto * sample_processor = engine->sample_processor_.get ();
  auto * tracklist = engine->project_->tracklist_;
  auto * monitor_fader = engine->control_room_->monitor_fader_.get ();
  auto * hw_in_processor = engine->hw_in_processor_.get ();
  auto * transport = project->transport_;

  /* add the sample processor */
  add_node_for_processable (*sample_processor);

  /* add the monitor fader */
  add_node_for_processable (*monitor_fader);

  /* add the initial processor */
  add_node_for_processable (initial_processor_);

  /* add the hardware input processor */
  add_node_for_processable (*hw_in_processor);

  /* add plugins */
  for (const auto &cur_tr : tracklist->tracks_)
    {
      std::visit (
        [&] (auto &&tr) {
          z_return_if_fail (tr);
          using TrackT = base_type<decltype (tr)>;

          /* add the track */
          add_node_for_processable (*tr);

          /* handle modulator track */
          if constexpr (std::is_same_v<ModulatorTrack, TrackT>)
            {
              /* add plugins */
              for (auto &pl : tr->modulators_)
                {
                  if (!pl || pl->deleting_)
                    continue;

                  add_plugin (*pl);
                }

              /* add macro processors */
              for (auto &mp : tr->modulator_macro_processors_)
                {
                  add_node_for_processable (*mp);
                }
            }

          if constexpr (std::derived_from<TrackT, ChannelTrack>)
            {
              auto &channel = tr->channel_;

              /* add the fader */
              auto * fader = channel->fader_;
              add_node_for_processable (*fader);

              /* add the prefader */
              auto * prefader = channel->prefader_;
              add_node_for_processable (*prefader);

              /* add plugins */
              std::vector<zrythm::gui::old_dsp::plugins::Plugin *> plugins;
              channel->get_plugins (plugins);
              for (auto * pl : plugins)
                {
                  if (!pl || pl->deleting_)
                    continue;

                  add_plugin (*pl);
                }

              /* add sends */
              if (
                tr->out_signal_type_ == dsp::PortType::Audio
                || tr->out_signal_type_ == dsp::PortType::Event)
                {
                  for (auto &send : channel->sends_)
                    {
                      /* note that we add sends even if empty so that graph
                       * renders properly */
                      add_node_for_processable (*send);
                    }
                }
            }
        },
        cur_tr);
    }

  external_out_ports_.clear ();

  /* add ports */
  std::vector<Port *> ports;
  project->get_all_ports (ports);
  for (auto * port : ports)
    {
      z_return_if_fail (port);
      if (port->deleting_ || (port->id_->owner_type_ == dsp::PortIdentifier::OwnerType::Plugin && port->get_plugin(true)->deleting_))
        continue;

      if (port->id_->flow_ == dsp::PortFlow::Output && port->is_exposed_to_backend ())
        {
          external_out_ports_.push_back (port);
        }

      add_port (
        convert_to_variant<PortPtrVariant> (port),
        *project->port_connections_manager_, drop_unnecessary_ports);
    }

  /* ========================
   * now connect them
   * ======================== */

  /* connect the sample processor */
  {
    auto * node = find_node_for_processable (*sample_processor);
    z_return_if_fail (node);
    auto * port = &sample_processor->fader_->stereo_out_->get_l ();
    auto * node2 = find_node_for_processable (*port);
    node->connect_to (*node2);
    port = &sample_processor->fader_->stereo_out_->get_r ();
    node2 = find_node_for_processable (*port);
    node->connect_to (*node2);
  }

  /* connect the monitor fader */
  {
    auto * node = find_node_for_processable (*monitor_fader);
    auto * port = &monitor_fader->stereo_in_->get_l ();
    auto * node2 = find_node_for_processable (*port);
    node2->connect_to (*node);
    port = &monitor_fader->stereo_in_->get_r ();
    node2 = find_node_for_processable (*port);
    node2->connect_to (*node);
    port = &monitor_fader->stereo_out_->get_l ();
    node2 = find_node_for_processable (*port);
    node->connect_to (*node2);
    port = &monitor_fader->stereo_out_->get_r ();
    node2 = find_node_for_processable (*port);
    node->connect_to (*node2);
  }

  GraphNode * initial_processor_node =
    find_node_for_processable (initial_processor_);

  /* connect the HW input processor */
  GraphNode * hw_processor_node = find_node_for_processable (*hw_in_processor);
  for (auto &port : hw_in_processor->audio_ports_)
    {
      auto node2 = find_node_for_processable (*port);
      z_warn_if_fail (node2);
      hw_processor_node->connect_to (*node2);
    }
  for (auto &port : hw_in_processor->midi_ports_)
    {
      auto node2 = find_node_for_processable (*port);
      hw_processor_node->connect_to (*node2);
    }

  /* connect MIDI editor manual press */
  {
    auto node2 = find_node_for_processable (*engine->midi_editor_manual_press_);
    node2->connect_to (*initial_processor_node);
  }

  /* connect the transport ports */
  {
    auto node = find_node_for_processable (*transport->roll_);
    node->connect_to (*initial_processor_node);
    node = find_node_for_processable (*transport->stop_);
    node->connect_to (*initial_processor_node);
    node = find_node_for_processable (*transport->backward_);
    node->connect_to (*initial_processor_node);
    node = find_node_for_processable (*transport->forward_);
    node->connect_to (*initial_processor_node);
    node = find_node_for_processable (*transport->loop_toggle_);
    node->connect_to (*initial_processor_node);
    node = find_node_for_processable (*transport->rec_toggle_);
    node->connect_to (*initial_processor_node);
  }

  /* connect tracks */
  for (auto &cur_tr : tracklist->tracks_)
    {
      std::visit (
        [&] (auto &tr) {
          using TrackT = base_type<decltype (tr)>;

          /* connect the track */
          auto * const track_node = find_node_for_processable (*tr);
          z_return_if_fail (track_node);
          if (tr->in_signal_type_ == dsp::PortType::Audio)
            {
              if constexpr (std::is_same_v<TrackT, AudioTrack>)
                {
                  auto node2 =
                    find_node_for_processable (*tr->processor_->mono_);
                  node2->connect_to (*track_node);
                  initial_processor_node->connect_to (*node2);
                  node2 =
                    find_node_for_processable (*tr->processor_->input_gain_);
                  node2->connect_to (*track_node);
                  initial_processor_node->connect_to (*node2);
                }

              if constexpr (std::derived_from<TrackT, ProcessableTrack>)
                {
                  auto track_processor = tr->processor_.get ();
                  auto node2 = find_node_for_processable (
                    track_processor->stereo_in_->get_l ());
                  node2->connect_to (*track_node);
                  initial_processor_node->connect_to (*node2);
                  node2 = find_node_for_processable (
                    track_processor->stereo_in_->get_r ());
                  node2->connect_to (*track_node);
                  initial_processor_node->connect_to (*node2);
                  node2 = find_node_for_processable (
                    track_processor->stereo_out_->get_l ());
                  track_node->connect_to (*node2);
                  node2 = find_node_for_processable (
                    track_processor->stereo_out_->get_r ());
                  track_node->connect_to (*node2);
                }
            }
          else if (tr->in_signal_type_ == dsp::PortType::Event)
            {
              if constexpr (std::derived_from<TrackT, ProcessableTrack>)
                {
                  auto track_processor = tr->processor_.get ();

                  if (tr->has_piano_roll () || tr->is_chord ())
                    {
                      /* connect piano roll */
                      auto node2 = find_node_for_processable (
                        *track_processor->piano_roll_);
                      z_return_if_fail (node2);
                      node2->connect_to (*track_node);
                    }

                  auto node2 =
                    find_node_for_processable (*track_processor->midi_in_);
                  node2->connect_to (*track_node);
                  initial_processor_node->connect_to (*node2);
                  node2 =
                    find_node_for_processable (*track_processor->midi_out_);
                  track_node->connect_to (*node2);
                }
            }

          if constexpr (std::derived_from<TrackT, PianoRollTrack>)
            {
              auto track_processor = tr->processor_.get ();

              for (int j = 0; j < 16; j++)
                {
                  for (int k = 0; k < 128; k++)
                    {
                      auto node2 = find_node_for_processable (
                        *track_processor->midi_cc_[j * 128 + k]);
                      if (node2)
                        {
                          node2->connect_to (*track_node);
                        }
                    }

                  auto node2 = find_node_for_processable (
                    *track_processor->pitch_bend_[j]);
                  if (node2 || !drop_unnecessary_ports)
                    {
                      node2->connect_to (*track_node);
                    }

                  node2 = find_node_for_processable (
                    *track_processor->poly_key_pressure_[j]);
                  if (node2 || !drop_unnecessary_ports)
                    {
                      node2->connect_to (*track_node);
                    }

                  node2 = find_node_for_processable (
                    *track_processor->channel_pressure_[j]);
                  if (node2 || !drop_unnecessary_ports)
                    {
                      node2->connect_to (*track_node);
                    }
                }
            }

          if constexpr (std::is_same_v<TrackT, TempoTrack>)
            {
              bpm_node_ = nullptr;
              beats_per_bar_node_ = nullptr;
              beat_unit_node_ = nullptr;

              auto node2 = find_node_for_processable (*tr->bpm_port_);
              if (node2 || !drop_unnecessary_ports)
                {
                  bpm_node_ = node2;
                  node2->connect_to (*track_node);
                }
              node2 = find_node_for_processable (*tr->beats_per_bar_port_);
              if (node2 || !drop_unnecessary_ports)
                {
                  beats_per_bar_node_ = node2;
                  node2->connect_to (*track_node);
                }
              node2 = find_node_for_processable (*tr->beat_unit_port_);
              if (node2 || !drop_unnecessary_ports)
                {
                  beat_unit_node_ = node2;
                  node2->connect_to (*track_node);
                }
              track_node->connect_to (*initial_processor_node);
            }

          if constexpr (std::is_same_v<TrackT, ModulatorTrack>)
            {
              initial_processor_node->connect_to (*track_node);

              for (auto &pl : tr->modulators_)
                {
                  if (pl && !pl->deleting_)
                    {
                      connect_plugin (*pl, drop_unnecessary_ports);
                      for (auto &pl_port : pl->in_ports_)
                        {
                          z_return_if_fail (
                            pl_port->get_plugin (true) != nullptr);
                          auto port_node = find_node_for_processable (*pl_port);
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
                          track_node->connect_to (*port_node);
                        }
                    }
                }

              /* connect the modulator macro processors */
              for (auto &mmp : tr->modulator_macro_processors_)
                {
                  auto mmp_node = find_node_for_processable (*mmp);
                  z_return_if_fail (mmp_node);

                  {
                    auto * const node2 =
                      find_node_for_processable (*mmp->cv_in_);
                    node2->connect_to (*mmp_node);
                  }
                  {
                    auto * const node2 =
                      find_node_for_processable (*mmp->macro_);
                    if (node2 || !drop_unnecessary_ports)
                      {
                        node2->connect_to (*mmp_node);
                      }
                  }

                  auto * const node2 = find_node_for_processable (*mmp->cv_out_);
                  mmp_node->connect_to (*node2);
                }
            }

          if constexpr (std::derived_from<TrackT, ChannelTrack>)
            {

              auto &ch = tr->channel_;
              auto &prefader = ch->prefader_;
              auto &fader = ch->fader_;

              /* connect the fader */
              auto * const fader_node = find_node_for_processable (*fader);
              z_warn_if_fail (fader_node);
              if (fader->type_ == Fader::Type::AudioChannel)
                {
                  /* connect ins */
                  auto node2 =
                    find_node_for_processable (fader->stereo_in_->get_l ());
                  node2->connect_to (*fader_node);
                  node2 =
                    find_node_for_processable (fader->stereo_in_->get_r ());
                  node2->connect_to (*fader_node);

                  /* connect outs */
                  node2 =
                    find_node_for_processable (fader->stereo_out_->get_l ());
                  fader_node->connect_to (*node2);
                  node2 =
                    find_node_for_processable (fader->stereo_out_->get_r ());
                  fader_node->connect_to (*node2);
                }
              else if (fader->type_ == Fader::Type::MidiChannel)
                {
                  auto node2 = find_node_for_processable (*fader->midi_in_);
                  node2->connect_to (*fader_node);
                  node2 = find_node_for_processable (*fader->midi_out_);
                  fader_node->connect_to (*node2);
                }
              {
                auto * const node2 = find_node_for_processable (*fader->amp_);
                if (node2 || !drop_unnecessary_ports)
                  {
                    node2->connect_to (*fader_node);
                  }
              }
              {
                auto * const node2 =
                  find_node_for_processable (*fader->balance_);
                if (node2 || !drop_unnecessary_ports)
                  {
                    node2->connect_to (*fader_node);
                  }
              }
              {
                auto * const node2 = find_node_for_processable (*fader->mute_);
                if (node2 || !drop_unnecessary_ports)
                  {
                    node2->connect_to (*fader_node);
                  }
              }

              /* connect the prefader */
              auto * const prefader_node = find_node_for_processable (*prefader);
              z_warn_if_fail (prefader_node);
              if (prefader->type_ == Fader::Type::AudioChannel)
                {
                  auto * node2 =
                    find_node_for_processable (prefader->stereo_in_->get_l ());
                  node2->connect_to (*prefader_node);
                  node2 =
                    find_node_for_processable (prefader->stereo_in_->get_r ());
                  node2->connect_to (*prefader_node);
                  node2 =
                    find_node_for_processable (prefader->stereo_out_->get_l ());
                  prefader_node->connect_to (*node2);
                  node2 =
                    find_node_for_processable (prefader->stereo_out_->get_r ());
                  prefader_node->connect_to (*node2);
                }
              else if (prefader->type_ == Fader::Type::MidiChannel)
                {
                  auto * node2 = find_node_for_processable (*prefader->midi_in_);
                  node2->connect_to (*prefader_node);
                  node2 = find_node_for_processable (*prefader->midi_out_);
                  prefader_node->connect_to (*node2);
                }

              std::vector<zrythm::gui::old_dsp::plugins::Plugin *> plugins;
              ch->get_plugins (plugins);
              for (auto * const pl : plugins)
                {
                  if (pl && !pl->deleting_)
                    {
                      connect_plugin (*pl, drop_unnecessary_ports);
                    }
                }

              for (auto &send : ch->sends_)
                {
                  /* note that we do not skip empty sends because then port
                   * connection validation will not detect invalid connections
                   * properly */
                  auto * const send_node = find_node_for_processable (*send);

                  auto * node2 = find_node_for_processable (*send->amount_);
                  if (node2)
                    node2->connect_to (*send_node);
                  node2 = find_node_for_processable (*send->enabled_);
                  if (node2)
                    node2->connect_to (*send_node);

                  if (tr->out_signal_type_ == dsp::PortType::Event)
                    {
                      node2 = find_node_for_processable (*send->midi_in_);
                      node2->connect_to (*send_node);
                      node2 = find_node_for_processable (*send->midi_out_);
                      send_node->connect_to (*node2);
                    }
                  else if (tr->out_signal_type_ == dsp::PortType::Audio)
                    {
                      node2 =
                        find_node_for_processable (send->stereo_in_->get_l ());
                      node2->connect_to (*send_node);
                      node2 =
                        find_node_for_processable (send->stereo_in_->get_r ());
                      node2->connect_to (*send_node);
                      node2 =
                        find_node_for_processable (send->stereo_out_->get_l ());
                      send_node->connect_to (*node2);
                      node2 =
                        find_node_for_processable (send->stereo_out_->get_r ());
                      send_node->connect_to (*node2);
                    }
                }
            }
        },
        cur_tr);
    }

  auto connect_port = [&]<FinalPortSubclass PortT> (PortT &p) {
    auto * node = find_node_for_processable (p);
    for (auto &src : p.srcs_)
      {
        auto node2 = find_node_for_processable (*src);
        z_warn_if_fail (node);
        z_warn_if_fail (node2);
        node2->connect_to (*node);
      }
    for (auto &dest : p.dests_)
      {
        auto node2 = find_node_for_processable (*dest);
        z_warn_if_fail (node);
        z_warn_if_fail (node2);
        node->connect_to (*node2);
      }
  };

  for (auto &port : ports)
    {
      if (port->deleting_) [[unlikely]]
        continue;
      if (port->id_->owner_type_ == dsp::PortIdentifier::OwnerType::Plugin)
        {
          auto port_pl = port->get_plugin (true);
          if (port_pl->deleting_)
            continue;
        }

      std::visit (
        [&] (auto &&p) { connect_port (*p); },
        convert_to_variant<PortPtrVariant> (port));
    }

  /* ========================
   * set initial and terminal nodes
   * ======================== */

  setup_terminal_nodes_.clear ();
  setup_init_trigger_list_.clear ();
  for (const auto &node : setup_graph_nodes_vector_)
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
      sample_processor->tracklist_->set_caches (ALL_CACHE_TYPES);
    }
  else
    {
      project->clip_editor_.set_caches ();
      tracklist->set_caches (ALL_CACHE_TYPES);
    }

  if (rechain)
    this->rechain ();
}

bool
Graph::can_ports_be_connected (const Port &src, const Port &dest)
{
  AudioEngine::State state{};
  AUDIO_ENGINE->wait_for_pause (state, false, true);

  z_debug ("validating for {} to {}", src.get_label (), dest.get_label ());

  setup (false, false);

  /* connect the src/dest if not NULL */
  /* this code is only for creating graphs to test if the connection between
   * src->dest is valid */
  bool success = std::visit (
    [&] (auto &&srcp, auto &&destp) {
      auto * node = find_node_for_processable (*srcp);
      z_return_val_if_fail (node, false);
      auto * node2 = find_node_for_processable (*destp);
      z_return_val_if_fail (node2, false);
      node->connect_to (*node2);
      z_return_val_if_fail (!node->terminal_, false);
      z_return_val_if_fail (!node2->initial_, false);
      return true;
    },
    convert_to_variant<PortPtrVariant> (&src),
    convert_to_variant<PortPtrVariant> (&dest));
  if (!success)
    {
      z_info ("failed to connect {} to {}", src.get_label (), dest.get_label ());
      return false;
    }

  bool valid = is_valid ();

  z_debug ("valid {}", valid);

  AUDIO_ENGINE->resume (state);

  return valid;
}

void
Graph::start ()
{
  int num_cores = std::min (MAX_GRAPH_THREADS, utils::audio::get_num_cores ());

  /* we reserve 1 core for the OS and other tasks and 1 core for the main thread
   */
  auto num_threads = env_get_int ("ZRYTHM_DSP_THREADS", num_cores - 2);
  if (num_threads < 0)
    {
      throw ZrythmException ("number of threads must be >= 0");
    }

  auto create_thread = [&] (auto is_main, auto idx) {
    try
      {
        return std::make_unique<GraphThread> (idx, is_main, *this);
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
      threads_.emplace_back (create_thread (false, i));
    }

  /* and the main thread */
  main_thread_ = create_thread (true, -1);

  auto start_thread = [&] (auto &thread) {
    try
      {
        thread->startRealtimeThread (
          juce::Thread::RealtimeOptions ().withPriority (9));
      }
    catch (const ZrythmException &e)
      {
        terminate ();
        throw ZrythmException (
          "failed to start thread: " + std::string (e.what ()));
      }
  };

  /* start them */
  for (auto &thread : threads_)
    {
      start_thread (thread);
    }
  start_thread (main_thread_);

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
  z_info ("terminating graph...");

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
  for (auto &thread : threads_)
    {
      thread->waitForThreadToExit (5);
    }
  main_thread_->waitForThreadToExit (5);
  threads_.clear ();
  main_thread_.reset ();

  z_info ("graph terminated");
}

Graph::GraphNode *
Graph::find_node_for_processable (const dsp::IProcessable &processable) const
{
  auto it =
    std::ranges::find_if (setup_graph_nodes_vector_, [&] (const auto &node) {
      return &node->get_processable () == &processable;
    });
  if (it != setup_graph_nodes_vector_.end ())
    {
      return it->get ();
    }
  return nullptr;
}

Graph::GraphNode *
Graph::add_node_for_processable (dsp::IProcessable &node)
{
  setup_graph_nodes_vector_.emplace_back (std::make_unique<GraphNode> (
    setup_graph_nodes_vector_.size (), *TRANSPORT, node));
  return setup_graph_nodes_vector_.back ().get ();
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
