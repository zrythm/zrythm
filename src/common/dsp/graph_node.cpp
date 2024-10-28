// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include <inttypes.h>

#include "common/dsp/engine.h"
#include "common/dsp/fader.h"
#include "common/dsp/graph.h"
#include "common/dsp/graph_node.h"
#include "common/dsp/master_track.h"
#include "common/dsp/midi_event.h"
#include "common/dsp/port.h"
#include "common/dsp/router.h"
#include "common/dsp/sample_processor.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/track.h"
#include "common/dsp/track_processor.h"
#include "common/dsp/tracklist.h"
#include "common/dsp/transport.h"
#include "common/plugins/plugin.h"
#include "common/utils/debug.h"
#include "common/utils/mpmc_queue.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include <fmt/printf.h>

GraphNode::GraphNode (Graph * graph, NodeData data)
    : id_ (graph->setup_graph_nodes_map_.size ()), graph_ (graph), data_(data)
{
}

std::string
GraphNode::get_name () const
{
  return std::visit (
    overload {
      [] (const PortPtrVariant &p) {
        return std::visit (
          [] (auto * port) { return port->get_full_designation (); }, p);
      },
      [] (const zrythm::plugins::PluginPtrVariant &pl) {
        return std::visit (
          [] (auto * plugin) {
            Track * track = plugin->get_track ();
            return fmt::format (
              "{}/{} (Plugin)", track->name_, plugin->get_name ());
          },
          pl);
      },
      [] (const TrackPtrVariant &t) {
        return std::visit ([] (auto * track) { return track->get_name(); }, t);
      },
      [] (Fader * f) {
        if (f->type_ == Fader::Type::Monitor)
          return std::string("Monitor Fader");
        Track * track = f->get_track ();
        return fmt::format ("{} {}", track->get_name(),
        f->passthrough_ ? "Pre-Fader/Passthrough" : "Fader");
      },
      [] (ModulatorMacroProcessor * mmp) {
        Track * track = mmp->cv_in_->get_track (true);
        return fmt::format ("{} Modulator Macro Processor", track->name_);
      },
      [] (ChannelSend * s) {
        auto *track = s->get_track ();
        return fmt::format ("{}/Channel Send {}", track->name_, s->slot_ + 1);
      },
      [] (SampleProcessor *) { return std::string("Sample Processor"); },
      [] (HardwareProcessor *) { return std::string("HW Processor"); },
      [] (std::monostate) { return std::string("Initial Processor"); },},
    data_);
}

#if 0
void *
GraphNode::get_pointer () const
{
  switch (type_)
    {
    case Type::Port:
      return port_;
    case Type::Plugin:
      return pl_;
    case Type::Track:
      return track_;
    case Type::Fader:
    case Type::MonitorFader:
      return fader_;
    case Type::Prefader:
      return prefader_;
    case Type::SampleProcessor:
      return sample_processor_;
    case Type::InitialProcessor:
      return nullptr;
    case Type::HardwareProcessor:
      return hw_processor_;
    case Type::ModulatorMacroProcessor:
      return modulator_macro_processor_;
    case Type::ChannelSend:
      return send_;
    }
  return nullptr; // Unreachable, but silences compiler warning
}
#endif

std::string
GraphNode::print_to_str () const
{
  std::string name = get_name ();
  std::string str1 = fmt::format (
    "node [({}) {}] refcount: {} | terminal: {} | initial: {} | playback latency: {}",
    id_, name, refcount_.load (), terminal_, initial_, playback_latency_);
  std::string str2;
  for (auto *const dest : childnodes_)
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
  for (auto *child_node : childnodes_)
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

  std::visit (
    overload{
      [&] (const PortPtrVariant &p) {
        std::visit (
          [&] (auto * port) {
            using PortT = base_type<decltype (port)>;
            if constexpr (std::is_same_v<PortT, MidiPort>) {
            if (port == AUDIO_ENGINE->midi_editor_manual_press_.get ())
              {
                port->midi_events_.dequeue (
                  time_nfo.local_offset_, time_nfo.nframes_);
                  return;
              }
            }
            
            if (
              AUDIO_ENGINE->is_port_own (*port) && AUDIO_ENGINE->exporting_)
              {
                /* if exporting and the port is not a project port, skip processing */
              }
            else
              {
                port->process (time_nfo, false);
              }
          },
          p);
      },
      [&] (const zrythm::plugins::PluginPtrVariant &pl) {
        std::visit ([&] (auto * plugin) { plugin->process (time_nfo); }, pl);
      },
      [&] (const TrackPtrVariant &t) {
        std::visit (
          [&] (auto * track) {
            using TrackT = base_type<decltype(track)>;
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
      [] (auto &&) { /* No processing needed */ } },
    data_);
}

void
GraphNode::process (EngineProcessTimeInfo time_nfo, GraphThread &thread)
{
  z_return_if_fail (graph_ && graph_->router_);

  /*z_info ("processing {}", graph_node_get_name (node));*/

  // use immediately invoked lambda to handle return scope
  [&] () {
    /* skip BPM during cycle (already processed in router_start_cycle()) */
    if (auto * port = std::get_if<PortPtrVariant> (&data_))
      {
        auto should_return = std::visit (
          [&] (auto * p) -> bool {
            using PortT = base_type<decltype (p)>;
            if constexpr (std::is_same_v<PortT, ControlPort>)
              {
                if (
                  graph_->router_->callback_in_progress_
                  && p == P_TEMPO_TRACK->bpm_port_.get ()) [[unlikely]]
                  {
                    return true;
                  }
              }
            return false;
          },
          *port);
          if (should_return)
            return;
      }

    /* if we are doing a no-roll */
    if (route_playback_latency_ < AUDIO_ENGINE->remaining_latency_preroll_)
      {
        /* only process terminal nodes to set their buffers to 0 */
        return;
      }

    /* only compensate latency when rolling */
    if (TRANSPORT->play_state_ == Transport::PlayState::Rolling)
      {
        /* if the playhead is before the loop-end point and the
         * latency-compensated position is after the loop-end point it means
         * that the loop was crossed, so compensate for that.
         *
         * if the position is before loop-end and position + frames is after
         * loop end (there is a loop inside the range), that should be handled
         * by the ports/processors instead */
        Position playhead_copy = PLAYHEAD;
        z_warn_if_fail (
          route_playback_latency_ >= AUDIO_ENGINE->remaining_latency_preroll_);
        TRANSPORT->position_add_frames (
          &playhead_copy,
          route_playback_latency_ - AUDIO_ENGINE->remaining_latency_preroll_);
        time_nfo.g_start_frame_ = (unsigned_frame_t) playhead_copy.frames_;
        time_nfo.g_start_frame_w_offset_ =
          (unsigned_frame_t) playhead_copy.frames_ + time_nfo.local_offset_;
      }

    /* split at loop points */
    for (
      nframes_t num_processable_frames = 0;
      (num_processable_frames = std::min (
         TRANSPORT->is_loop_point_met (
           (signed_frame_t) time_nfo.g_start_frame_w_offset_, time_nfo.nframes_),
         time_nfo.nframes_))
      != 0;)
      {
#if 0
      z_info (
        "splitting from %ld "
        "(num processable frames %"
        PRIu32 ")",
        g_start_frames, num_processable_frames);
#endif

        /* temporarily change the nframes to avoid having to declare a separate
         * EngineProcessTimeInfo */
        nframes_t orig_nframes = time_nfo.nframes_;
        time_nfo.nframes_ = num_processable_frames;
        process_internal (time_nfo);

        /* calculate the remaining frames */
        time_nfo.nframes_ = orig_nframes - num_processable_frames;

        /* loop back to loop start */
        unsigned_frame_t frames_to_add =
          (num_processable_frames
           + (unsigned_frame_t) TRANSPORT->loop_start_pos_->getFrames ())
          - (unsigned_frame_t) TRANSPORT->loop_end_pos_->getFrames ();
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
      [&] (const zrythm::plugins::PluginPtrVariant &pl) -> nframes_t {
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
