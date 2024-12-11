// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---
 */

#ifndef __AUDIO_GRAPH_H__
#define __AUDIO_GRAPH_H__

#include <semaphore>

#include "dsp/graph_node.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/track.h"
#include "utils/mpmc_queue.h"
#include "utils/types.h"

class Graph;
class Port;
class Fader;
class Track;
class SampleProcessor;
class GraphThread;
class Router;
class ModulatorMacroProcessor;
class ChannelSend;
class HardwareProcessor;

/**
 * @addtogroup dsp
 *
 * @{
 */

constexpr int MAX_GRAPH_THREADS = 128;

using NodeData = std::variant<
  PortPtrVariant,
  zrythm::gui::old_dsp::plugins::PluginPtrVariant,
  TrackPtrVariant,
  Fader *, // Used for normal fader, prefader and monitor fader
  SampleProcessor *,
  HardwareProcessor *,
  ModulatorMacroProcessor *,
  ChannelSend *,
  std::monostate // For initial processor (dummy processor in the chain
                 // processed before anything else. )
  >;

template <> struct std::hash<NodeData>
{
  size_t operator() (const NodeData &data) const
  {
    return std::visit (
      overload{
        [] (const PortPtrVariant &p) {
          return std::visit (
            [] (auto * ptr) {
              z_return_val_if_fail (ptr, static_cast<size_t> (0));
              return reinterpret_cast<size_t> (ptr);
            },
            p);
        },
        [] (const zrythm::gui::old_dsp::plugins::PluginPtrVariant &pl) {
          return std::visit (
            [] (auto * ptr) {
              z_return_val_if_fail (ptr, static_cast<size_t> (0));
              return reinterpret_cast<size_t> (ptr);
            },
            pl);
        },
        [] (const TrackPtrVariant &t) {
          return std::visit (
            [] (auto * ptr) {
              z_return_val_if_fail (ptr, static_cast<size_t> (0));
              return reinterpret_cast<size_t> (ptr);
            },
            t);
        },
        [] (const std::monostate &) -> size_t { return 1; },
        [] (auto * ptr) {
          z_return_val_if_fail (ptr, static_cast<size_t> (0));
          return reinterpret_cast<size_t> (ptr);
        } },
      data);
  }
};

/**
 * Graph.
 */
class Graph final
{
public:
  using GraphNode = dsp::GraphNode;

public:
  Graph (Router * router, SampleProcessor * sample_processor = nullptr);
  ~Graph ();

  void print () const;

  GraphNode * find_node_from_port (PortPtrVariant port) const;

  GraphNode * find_node_from_plugin (
    zrythm::gui::old_dsp::plugins::PluginPtrVariant pl) const;

  GraphNode *
  find_node_from_track (TrackPtrVariant track, bool use_setup_nodes) const;

  GraphNode * find_node_from_fader (const Fader * fader) const;

  GraphNode * find_node_from_sample_processor (
    const SampleProcessor * sample_processor) const;

  GraphNode * find_node_from_modulator_macro_processor (
    const ModulatorMacroProcessor * processor) const;

  GraphNode * find_node_from_channel_send (const ChannelSend * send) const;

  GraphNode * find_initial_processor_node () const;

  GraphNode *
  find_hw_processor_node (const HardwareProcessor * processor) const;

  /**
   * Returns the max playback latency of the trigger
   * nodes.
   */
  nframes_t get_max_route_playback_latency (bool use_setup_nodes);

  // GraphThread * get_current_thread () const;

  void update_latencies (bool use_setup_nodes);

  /*
   * Adds the graph nodes and connections, then rechains.
   *
   * @param drop_unnecessary_ports Drops any ports that don't connect anywhere.
   * @param rechain Whether to rechain or not. If we are just validating this
   * should be 0.
   */
  void setup (bool drop_unnecessary_ports, bool rechain);

  /**
   * Adds a new connection for the given src and dest ports and validates the
   * graph.
   *
   * @note This should be called on a new instance of Graph.
   *
   * @return Whether the ports can be connected (if the connection will
   * be valid and won't break the acyclicity of the graph).
   */
  bool can_ports_be_connected (const Port &src, const Port &dest);

  /**
   * Starts as many threads as there are cores.
   *
   * @throw ZrythmException on failure.
   */
  void start ();

  /**
   * Tell all threads to terminate.
   */
  void terminate ();

  /**
   * Called when an upstream (parent) node has completed processing.
   */
  ATTR_HOT void trigger_node (GraphNode &node);

private:
  /**
   * @brief Checks for cycles in the graph.
   *
   * @return Whether valid.
   */
  bool is_valid () const;

  void clear_setup ();

  void rechain ();

  void add_plugin (zrythm::gui::old_dsp::plugins::Plugin &pl);
  void connect_plugin (
    zrythm::gui::old_dsp::plugins::Plugin &pl,
    bool                                   drop_unnecessary_ports);
  GraphNode * add_port (
    PortPtrVariant          port_var,
    PortConnectionsManager &mgr,
    bool                    drop_if_unnecessary);

  /**
   * @brief Connects the port as a node.
   *
   * @param port
   */
  void connect_port (PortPtrVariant port);

  /**
   * Creates a new node, adds it to the graph and returns it.
   */
  GraphNode * create_node (
    GraphNode::NameGetter                  name_getter,
    GraphNode::ProcessFunc                 process_func,
    GraphNode::SinglePlaybackLatencyGetter playback_latency_getter,
    NodeData                               data);

public:
  using GraphNodesMap = std::unordered_map<NodeData, std::unique_ptr<GraphNode>>;
  /**
   * @brief List of all graph nodes.
   *
   * This manages the lifetime of graph nodes automatically.
   *
   * The key is a raw pointer to a Plugin, Port, etc.
   */
  GraphNodesMap graph_nodes_map_;

  /**
   * @brief Chain used to setup in the background.
   *
   * This is applied and cleared by @ref rechain().
   *
   * @see graph_nodes_map_
   */
  GraphNodesMap setup_graph_nodes_map_;

  /* --- caches for current graph --- */
  GraphNode * bpm_node_ = nullptr;
  GraphNode * beats_per_bar_node_ = nullptr;
  GraphNode * beat_unit_node_ = nullptr;

  /** Nodes without incoming edges.
   * These run concurrently at the start of each
   * cycle to kick off processing */
  std::vector<GraphNode *> init_trigger_list_;

  /** Number of graph nodes without an outgoing edge. */
  std::vector<GraphNode *> terminal_nodes_;

  /** Remaining unprocessed terminal nodes in this cycle. */
  std::atomic<int> terminal_refcnt_ = 0;

  /** Synchronization with main process callback. */
  /* FIXME: this should probably be binary semaphore but i left it as a
   * counting one out of caution while refactoring from ZixSem */
  std::counting_semaphore<MAX_GRAPH_THREADS> callback_start_sem_{ 0 };
  std::binary_semaphore                      callback_done_sem_{ 0 };

  /** Wake up graph node process threads. */
  std::counting_semaphore<MAX_GRAPH_THREADS> trigger_sem_{ 0 };

  /** Queue containing nodes that can be processed. */
  MPMCQueue<GraphNode *> trigger_queue_;

  /** Number of entries in trigger queue. */
  std::atomic<int> trigger_queue_size_ = 0;

  /** Number of threads waiting for work. */
  std::atomic<int> idle_thread_cnt_ = 0;

  std::vector<GraphNode *> setup_init_trigger_list_;

  /** Used only when constructing the graph so we can traverse the graph
   * backwards to calculate the playback latencies. */
  std::vector<GraphNode *> setup_terminal_nodes_;

  /** Dummy member to make lookups work. */
  // int initial_processor_ = 0;

  /* ------------------------------------ */

  std::vector<std::unique_ptr<GraphThread>> threads_;
  std::unique_ptr<GraphThread>              main_thread_;

  /**
   * An array of pointers to ports that are exposed to the backend and are
   * outputs.
   *
   * Used to clear their buffers when returning early from the processing
   * cycle.
   */
  std::vector<Port *> external_out_ports_;

  /** Sample processor, if temporary graph for sample processor. */
  SampleProcessor * sample_processor_ = nullptr;

  /** Pointer back to router for convenience. */
  Router * router_ = nullptr;
};

/**
 * @}
 */

#endif
