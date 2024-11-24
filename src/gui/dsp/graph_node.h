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
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#ifndef __AUDIO_GRAPH_NODE_H__
#define __AUDIO_GRAPH_NODE_H__

#include "gui/dsp/plugin.h"
#include "gui/dsp/track.h"

#include "utils/types.h"

class GraphNode;
class Graph;
class PassthroughProcessor;
class Port;
class Fader;
class Track;
class SampleProcessor;
class HardwareProcessor;
class ModulatorMacroProcessor;
struct EngineProcessTimeInfo;
class ChannelSend;
class GraphThread;

namespace zrythm::gui::dsp::plugins
{
class Plugin;
}

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * @brief A node in the processing graph.
 */
class GraphNode
{
public:
#if 0
  /**
   * @brief Graph node type.
   *
   * Graph nodes can be either ports (audio L input, midi input, etc.) or
   * processors (plugin, fader, etc.). This is explained in the user manual.
   * TODO: explain this here too.
   */
  enum class Type
  {
    /** Port. */
    Port,
    /** zrythm::gui::dsp::plugins::Plugin processor. */
    Plugin,
    /** Track processor. */
    Track,
    /** Fader/pan processor. */
    Fader,
    /** Fader/pan processor for monitor. */
    MonitorFader,
    /** Pre-Fader passthrough processor. */
    Prefader,
    /** Sample processor. */
    SampleProcessor,

    /**
     * Initial processor.
     *
     * The initial processor is a dummy processor in the chain processed
     * before anything else.
     */
    InitialProcessor,

    /** Hardware processor. */
    HardwareProcessor,

    ModulatorMacroProcessor,

    /** Channel send. */
    ChannelSend,
  };
#endif

  using NodeData = std::variant<
    PortPtrVariant,
    zrythm::gui::dsp::plugins::PluginPtrVariant,
    TrackPtrVariant,
    Fader *, // Used for normal fader, prefader and monitor fader
    SampleProcessor *,
    HardwareProcessor *,
    ModulatorMacroProcessor *,
    ChannelSend *,
    std::monostate // For initial processor (dummy processor in the chain
                   // processed before anything else. )
    >;

public:
  GraphNode () = default;

  GraphNode (Graph * graph, NodeData data);

  /**
   * Returns a human friendly name of the node.
   */
  std::string get_name () const;

  // void * get_pointer () const;

  /** For general debugging. */
  std::string print_to_str () const;

  void print () const;

  /**
   * Processes the GraphNode.
   */
  ATTR_HOT void process (EngineProcessTimeInfo time_nfo, GraphThread &thread);

  /**
   * Returns the latency of only the given port, without adding the
   * previous/next latencies.
   *
   * It returns the plugin's latency if plugin, otherwise 0.
   */
  ATTR_HOT nframes_t get_single_playback_latency () const;

  /**
   * Sets the playback latency of the given node recursively.
   *
   * Used only when (re)creating the graph.
   *
   * @param dest_latency The total destination latency so far.
   */
  void set_route_playback_latency (nframes_t dest_latency);

  /**
   * Called by an upstream node when it has completed processing.
   */
  ATTR_HOT void trigger ();

  void connect_to (GraphNode &target);

private:
  ATTR_HOT void on_finish (GraphThread &thread);

  ATTR_HOT void process_internal (EngineProcessTimeInfo time_nfo);

  void add_feeds (GraphNode &dest);
  void add_depends (GraphNode &src);

public:
  int id_ = 0;

  /** Ref back to the graph so we don't have to pass it around. */
  Graph * graph_ = nullptr;

  /**
   * @brief Outgoing nodes.
   *
   * Downstream nodes to activate when this node has completed processing.
   *
   * @note These are not owned.
   */
  std::vector<GraphNode *> childnodes_;

  /** Incoming node count. */
  std::atomic<int> refcount_ = 0;

  /** Initial incoming node count. */
  int init_refcount_ = 0;

  /**
   * @brief Incoming nodes.
   *
   * Used when creating the graph so we can traverse it backwards to set the
   * latencies.
   *
   * @note These are not owned.
   */
  std::vector<GraphNode *> parentnodes_;

  NodeData data_;

  /** For debugging. */
  bool terminal_ = false;
  bool initial_ = false;

  /** The playback latency of the node, in samples. */
  nframes_t playback_latency_ = 0;

  /** The route's playback latency so far. */
  nframes_t route_playback_latency_ = 0;
};

template <> struct std::hash<GraphNode::NodeData>
{
  size_t operator() (const GraphNode::NodeData &data) const
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
        [] (const zrythm::gui::dsp::plugins::PluginPtrVariant &pl) {
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
 * @}
 */

#endif
