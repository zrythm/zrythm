// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>
#include <utility>

#include "dsp/itransport.h"
#include "gui/dsp/audio_port.h"
#include "gui/dsp/carla_native_plugin.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/cv_port.h"
#include "gui/dsp/engine.h"
#include "gui/dsp/fader.h"
#include "gui/dsp/graph.h"
#include "gui/dsp/graph_node.h"
#include "gui/dsp/midi_event.h"
#include "gui/dsp/midi_port.h"
#include "gui/dsp/modulator_macro_processor.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/port.h"
#include "gui/dsp/router.h"
#include "gui/dsp/sample_processor.h"
#include "gui/dsp/track_all.h"
#include "gui/dsp/track_processor.h"
#include "utils/debug.h"
#include "utils/mpmc_queue.h"

GraphNode::GraphNode (
  Graph *          graph,
  NameGetter       name_getter,
  dsp::ITransport &transport,
  NodeData         data)
    : id_ (graph->setup_graph_nodes_map_.size ()), graph_ (graph), data_ (data),
      transport_ (transport), name_getter_ (std::move (name_getter))
{
  if (auto * port = std::get_if<PortPtrVariant> (&data_))
    {
      is_bpm_node_ = std::visit (
        [&] (auto * p) -> bool {
          return ENUM_BITSET_TEST (
            dsp::PortIdentifier::Flags, p->id_->flags_,
            dsp::PortIdentifier::Flags::Bpm);
        },
        *port);
    }
}

std::string
GraphNode::get_name () const
{
  return name_getter_ ();
}

std::string
GraphNode::print_to_str () const
{
  std::string name = get_name ();
  std::string str1 = fmt::format (
    "node [({}) {}] refcount: {} | terminal: {} | initial: {} | playback latency: {}",
    id_, name, refcount_.load (), terminal_, initial_, playback_latency_);
  std::string str2;
  for (auto * const dest : childnodes_)
    {
      name = dest->get_name ();
      str2 = fmt::format ("{} (dest [({}) {}])", str1, dest->id_, name);
      str1 = str2;
    }
  return str1;
}

void
GraphNode::print () const
{
  z_info (print_to_str ());
}

void
GraphNode::on_finish (GraphThread &thread)
{
  bool feeds = false;

  /* notify downstream nodes that depend on this node */
  for (auto * child_node : childnodes_)
    {
#if 0
      /* set the largest playback latency of this
       * route to the child as well */
      self->childnodes[i]->route_playback_latency =
        self->playback_latency;
        /*MAX (*/
          /*self->playback_latency,*/
          /*self->childnodes[i]->*/
            /*route_playback_latency);*/
#endif
      child_node->trigger ();
      feeds = true;
    }

  /* if there are no outgoing edges, this is a
   * terminal node */
  if (!feeds)
    {
      /* notify parent graph */
      thread.on_reached_terminal_node ();
    }
}

void
GraphNode::process_internal (const EngineProcessTimeInfo time_nfo)
{
  z_return_if_fail_cmp (
    time_nfo.g_start_frame_w_offset_, >=, time_nfo.g_start_frame_);

  // z_info ("processing {}", get_name ());

  std::visit (
    overload{
      [&] (PortPtrVariant &p) {
        std::visit (
          [&] (auto * port) {
            using PortT = base_type<decltype (port)>;
            if constexpr (std::is_same_v<PortT, MidiPort>)
              {
                if (
                  ENUM_BITSET_TEST (
                    dsp::PortIdentifier::Flags, port->id_->flags_,
                    dsp::PortIdentifier::Flags::ManualPress))
                  {
                    port->midi_events_.dequeue (
                      time_nfo.local_offset_, time_nfo.nframes_);
                    return;
                  }
              }
            if constexpr (std::is_same_v<PortT, AudioPort>)
              {
                if (
                  port->id_->is_monitor_fader_stereo_in_or_out_port ()
                  && AUDIO_ENGINE->exporting_)
                  {
                    /* if exporting and the port is not a project port, skip
                     * processing */
                    return;
                  }
              }

            port->process (time_nfo, false);
          },
          p);
      },
      [&] (zrythm::gui::old_dsp::plugins::PluginPtrVariant &pl) {
        std::visit ([&] (auto * plugin) { plugin->process (time_nfo); }, pl);
      },
      [&] (TrackPtrVariant &t) {
        std::visit (
          [&] (auto * track) {
            using TrackT = base_type<decltype (track)>;
            if constexpr (std::derived_from<TrackT, ProcessableTrack>)
              {
                track->processor_->process (time_nfo);
              }
          },
          t);
      },
      [&] (Fader * f) { f->process (time_nfo); },
      [&] (ModulatorMacroProcessor * mmp) { mmp->process (time_nfo); },
      [&] (SampleProcessor * sp) {
        sp->process (time_nfo.local_offset_, time_nfo.nframes_);
      },
      [&] (ChannelSend * s) {
        s->process (time_nfo.local_offset_, time_nfo.nframes_);
      },
      [] (auto &&) { /* No processing needed */

      } },
    data_);
}

void
GraphNode::process (EngineProcessTimeInfo time_nfo, GraphThread &thread)
{
  z_return_if_fail (graph_ && graph_->router_);

  // z_info ("processing {}", get_name ());

  // use immediately invoked lambda to handle return scope
  [&] () {
    /* skip BPM during cycle (already processed in router_start_cycle()) */
    if (is_bpm_node_ && graph_->router_->callback_in_progress_) [[unlikely]]
      {
        // z_debug ("skipping BPM node");
        return;
      }

    /* if we are doing a no-roll */
    if (route_playback_latency_ < AUDIO_ENGINE->remaining_latency_preroll_)
      {
        /* only process terminal nodes to set their buffers to 0 */
        // z_debug ("no-roll, skipping node");
        return;
      }

    /* only compensate latency when rolling */
    if (transport_.get_play_state () == Transport::PlayState::Rolling)
      {
        /* if the playhead is before the loop-end point and the
         * latency-compensated position is after the loop-end point it means
         * that the loop was crossed, so compensate for that.
         *
         * if the position is before loop-end and position + frames is after
         * loop end (there is a loop inside the range), that should be handled
         * by the ports/processors instead */
        dsp::Position playhead_copy = transport_.get_playhead_position ();
        z_return_if_fail (
          route_playback_latency_ >= AUDIO_ENGINE->remaining_latency_preroll_);
        transport_.position_add_frames (
          playhead_copy,
          route_playback_latency_ - AUDIO_ENGINE->remaining_latency_preroll_);
        time_nfo.g_start_frame_ = (unsigned_frame_t) playhead_copy.frames_;
        time_nfo.g_start_frame_w_offset_ =
          (unsigned_frame_t) playhead_copy.frames_ + time_nfo.local_offset_;
      }

    /* split at loop points */
    for (
      nframes_t num_processable_frames = 0;
      (num_processable_frames = std::min (
         transport_.is_loop_point_met (
           (signed_frame_t) time_nfo.g_start_frame_w_offset_, time_nfo.nframes_),
         time_nfo.nframes_))
      != 0;)
      {
        // z_debug (
        //   "splitting from {} (num processable frames {})",
        //   time_nfo.g_start_frame_w_offset_, num_processable_frames);

        /* temporarily change the nframes to avoid having to declare a separate
         * EngineProcessTimeInfo */
        nframes_t orig_nframes = time_nfo.nframes_;
        time_nfo.nframes_ = num_processable_frames;
        process_internal (time_nfo);

        /* calculate the remaining frames */
        time_nfo.nframes_ = orig_nframes - num_processable_frames;

        /* loop back to loop start */
        auto [transport_loop_start_pos, transport_loop_end_pos] =
          transport_.get_loop_range_positions ();
        unsigned_frame_t frames_to_add =
          (num_processable_frames
           + (unsigned_frame_t) transport_loop_start_pos.frames_)
          - (unsigned_frame_t) transport_loop_end_pos.frames_;
        time_nfo.g_start_frame_w_offset_ += frames_to_add;
        time_nfo.g_start_frame_ += frames_to_add;
        time_nfo.local_offset_ += num_processable_frames;
      }

    z_return_if_fail_cmp (
      time_nfo.g_start_frame_w_offset_, >=, time_nfo.g_start_frame_);

    if (time_nfo.nframes_ > 0)
      {
        process_internal (time_nfo);
      }
  }();

  if (graph_->router_->callback_in_progress_)
    {
      on_finish (thread);
    }
}

void
GraphNode::trigger ()
{
  /* check if we can run */
  if (refcount_.fetch_sub (1) == 1)
    {
      /* reset reference count for next cycle */
      refcount_.store (init_refcount_);

      /* all nodes that feed this node have completed, so this node be processed
       * now. */
      graph_->trigger_queue_size_.fetch_add (1);
      /*z_info ("triggering node, pushing back");*/
      graph_->trigger_queue_.push_back (this);
    }
}

void
GraphNode::add_feeds (GraphNode &dest)
{
  /* return if already added */
  for (auto child : childnodes_)
    {
      if (child == &dest)
        {
          return;
        }
    }

  childnodes_.push_back (&dest);

  terminal_ = false;
}

void
GraphNode::add_depends (GraphNode &src)
{
  ++init_refcount_;
  refcount_ = init_refcount_;

  /* add parent nodes */
  parentnodes_.push_back (&src);

  initial_ = false;
}

nframes_t
GraphNode::get_single_playback_latency () const
{
  return std::visit (
    overload{
      [&] (const zrythm::gui::old_dsp::plugins::PluginPtrVariant &pl) -> nframes_t {
        return std::visit (
          [&] (auto * plugin) { /* latency is already set at this point */
                                return plugin->latency_;
          },
          pl);
      },
      [] (auto &&) -> nframes_t { return 0; } },
    data_);
}

void
GraphNode::set_route_playback_latency (nframes_t dest_latency)
{
  /*long max_latency = 0, parent_latency;*/

  /* set route playback latency */
  if (dest_latency > route_playback_latency_)
    {
      route_playback_latency_ = dest_latency;
    }
  else
    {
      /* assumed that parent nodes already have the previous
       * latency set */
      /* TODO double-check if this is OK to do, needs testing */
      return;
    }

  for (auto parent : parentnodes_)
    {
      parent->set_route_playback_latency (route_playback_latency_);
#if 0
      z_info (
        "added %d route playback latency from node %s to "
        "parent %s. Total route latency on parent: %d",
        dest_latency,
        graph_node_get_name (node),
        graph_node_get_name (parent),
        parent->route_playback_latency);
#endif
    }
}

void
GraphNode::connect_to (GraphNode &target)
{
  if (
    std::find (childnodes_.begin (), childnodes_.end (), &target)
    != childnodes_.end ())
    return;

  add_feeds (target);
  target.add_depends (*this);

  z_warn_if_fail (!terminal_ && !target.initial_);
}
