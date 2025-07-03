// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/session/router.h"
#include "gui/backend/backend/actions/arranger_selections_action.h"
#include "gui/backend/backend/actions/tracklist_selections_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/io/file_import.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/chord_track.h"
#include "structure/tracks/foldable_track.h"
#include "structure/tracks/marker_track.h"
#include "structure/tracks/master_track.h"
#include "structure/tracks/modulator_track.h"
#include "structure/tracks/track.h"
#include "structure/tracks/tracklist.h"
#include "utils/gtest_wrapper.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"
#include "utils/views.h"

namespace zrythm::structure::tracks
{

// Tracklist::Tracklist (QObject * parent) : QAbstractListModel (parent) { }

Tracklist::Tracklist (
  Project                     &project,
  PortRegistry                &port_registry,
  TrackRegistry               &track_registry,
  dsp::PortConnectionsManager &port_connections_manager,
  const dsp::TempoMap         &tempo_map)
    : QAbstractListModel (&project), tempo_map_ (tempo_map),
      track_registry_ (track_registry), port_registry_ (port_registry),
      project_ (&project), port_connections_manager_ (&port_connections_manager)
{
}

Tracklist::Tracklist (
  engine::session::SampleProcessor &sample_processor,
  PortRegistry                     &port_registry,
  TrackRegistry                    &track_registry,
  dsp::PortConnectionsManager      &port_connections_manager,
  const dsp::TempoMap              &tempo_map)
    : tempo_map_ (tempo_map), track_registry_ (track_registry),
      port_registry_ (port_registry), sample_processor_ (&sample_processor),
      port_connections_manager_ (&port_connections_manager)
{
}

void
Tracklist::init_loaded (
  PortRegistry                      &port_registry,
  Project *                          project,
  engine::session::SampleProcessor * sample_processor)
{
  port_registry_ = port_registry;
  project_ = project;
  sample_processor_ = sample_processor;

  z_debug ("initializing loaded Tracklist...");
  for (const auto &track_var : get_track_span ())
    {
      std::visit (
        [&] (auto &track) {
          using T = base_type<decltype (track)>;
          if constexpr (std::is_same_v<T, ChordTrack>)
            {
              chord_track_ = track;
            }
          else if constexpr (std::is_same_v<T, MarkerTrack>)
            {
              marker_track_ = track;
            }
          else if constexpr (std::is_same_v<T, MasterTrack>)
            {
              master_track_ = track;
            }
          else if constexpr (std::is_same_v<T, ModulatorTrack>)
            {
              modulator_track_ = track;
            }
          track->tracklist_ = this;
          track->init_loaded (*plugin_registry_, *port_registry_);
        },
        track_var);
    }
}

// ========================================================================
// QML Interface
// ========================================================================

QHash<int, QByteArray>
Tracklist::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[TrackPtrRole] = "track";
  roles[TrackNameRole] = "trackName";
  return roles;
}

int
Tracklist::rowCount (const QModelIndex &parent) const
{
  return static_cast<int> (tracks_.size ());
}

QVariant
Tracklist::data (const QModelIndex &index, int role) const
{
  if (!index.isValid ())
    return {};

  auto track_id = tracks_.at (index.row ());
  auto track = track_id.get_object ();

  z_trace (
    "getting role {} for track {}", role, TrackSpan::name_projection (track));

  switch (role)
    {
    case TrackPtrRole:
      return QVariant::fromStdVariant (track);
    case TrackNameRole:
      return TrackSpan::name_projection (track).to_qstring ();
    default:
      return {};
    }
}

void
Tracklist::setExclusivelySelectedTrack (QVariant track)
{
  auto track_var = qvariantToStdVariant<TrackPtrVariant> (track);
  get_selection_manager ().select_unique (
    TrackSpan::uuid_projection (track_var));
}

// ========================================================================

void
Tracklist::disconnect_port (const Port::Uuid &port_id)
{
  port_connections_manager_->remove_all_connections (port_id);
}

void
Tracklist::disconnect_plugin (const Plugin::Uuid &plugin_id)
{
  auto plugin_var = plugin_registry_->find_by_id_or_throw (plugin_id);
  std::visit (
    [&] (auto &&pl) {
      z_info ("disconnecting plugin {}...", pl->get_name ());

      pl->deleting_ = true;

      if (pl->visible_)
        pl->close_ui ();

      /* disconnect all ports */
      for (auto port : pl->get_input_port_span ().as_base_type ())
        {
          disconnect_port (port->get_uuid ());
        }
      for (auto port : pl->get_output_port_span ().as_base_type ())
        {
          disconnect_port (port->get_uuid ());
        }
      z_debug (
        "disconnected all ports of {} in ports: {} out ports: {}",
        pl->get_name (), pl->in_ports_.size (), pl->out_ports_.size ());

      pl->close ();

      z_debug ("finished disconnecting plugin {}", pl->get_name ());
    },
    plugin_var);
}

void
Tracklist::disconnect_channel (Channel &channel)
{
  z_debug ("disconnecting channel {}", channel.track_->get_name ());
  {
    std::vector<Plugin *> plugins;
    channel.get_plugins (plugins);
    for (const auto &pl : plugins)
      {
        const auto slot = channel.get_plugin_slot (pl->get_uuid ());
        channel.track_->remove_plugin (slot, false, true);
      }
  }

  /* disconnect from output */
  if (channel.has_output ())
    {
      auto * out_track = channel.get_output_track ();
      assert (out_track);
      out_track->remove_child (channel.track_->get_uuid (), true, false, false);
    }

  /* disconnect fader/prefader */
  disconnect_fader (*channel.prefader_);
  disconnect_fader (*channel.fader_);

  /* disconnect all ports */
  std::vector<Port *> ports;
  channel.append_ports (ports, true);
  for (auto * port : ports)
    {
      disconnect_port (port->get_uuid ());
    }
}

void
Tracklist::disconnect_fader (Fader &fader)
{
  const auto disconnect = [&] (auto &port) {
    disconnect_port (port.get_uuid ());
  };

  if (fader.has_audio_ports ())
    {
      auto stereo_in = fader.get_stereo_in_ports ();
      disconnect (stereo_in.first);
      disconnect (stereo_in.second);
      auto stereo_out = fader.get_stereo_out_ports ();
      disconnect (stereo_out.first);
      disconnect (stereo_out.second);
    }
  else if (fader.has_midi_ports ())
    {
      auto &midi_in = fader.get_midi_in_port ();
      disconnect (midi_in);
      auto &midi_out = fader.get_midi_out_port ();
      disconnect (midi_out);
    }

  disconnect (fader.get_amp_port ());
  disconnect (fader.get_balance_port ());
  disconnect (fader.get_mute_port ());
  disconnect (fader.get_solo_port ());
  disconnect (fader.get_listen_port ());
  disconnect (fader.get_mono_compat_enabled_port ());
  disconnect (fader.get_swap_phase_port ());
}

void
Tracklist::disconnect_track_processor (TrackProcessor &track_processor)
{
  auto track = track_processor.get_track ();
  z_return_if_fail (track);

  const auto disconnect_port = [&] (auto &port) {
    port_connections_manager_->remove_all_connections (port.get_uuid ());
  };

  switch (track->get_input_signal_type ())
    {
    case dsp::PortType::Audio:
      disconnect_port (track_processor.get_mono_port ());
      disconnect_port (track_processor.get_input_gain_port ());
      disconnect_port (track_processor.get_output_gain_port ());
      disconnect_port (track_processor.get_monitor_audio_port ());
      iterate_tuple (disconnect_port, track_processor.get_stereo_in_ports ());
      iterate_tuple (disconnect_port, track_processor.get_stereo_out_ports ());

      break;
    case dsp::PortType::Event:
      disconnect_port (track_processor.get_midi_in_port ());
      disconnect_port (track_processor.get_midi_out_port ());
      if (track->has_piano_roll ())
        disconnect_port (track_processor.get_piano_roll_port ());
      break;
    default:
      break;
    }
}

void
Tracklist::disconnect_track (Track &track)
{
  z_debug ("disconnecting track '{}' ({})...", track.get_name (), track.pos_);

  track.disconnecting_ = true;

  /* if this is a group track and has children, remove them */
  if (!track.is_auditioner () && track.can_be_group_target ())
    {
      auto * group_target = dynamic_cast<GroupTargetTrack *> (&track);
      if (group_target != nullptr)
        {
          group_target->remove_all_children (true, false, false);
        }
    }

  /* disconnect all ports and free buffers */
  std::vector<Port *> ports;
  track.append_ports (ports, true);
  for (auto * port : ports)
    {
      port_connections_manager_->remove_all_connections (port->get_uuid ());
    }

  if (!track.is_auditioner ())
    {
      /* disconnect from folders */
      track.remove_from_folder_parents ();
    }

  if (auto channel_track = dynamic_cast<ChannelTrack *> (&track))
    {
      disconnect_channel (*channel_track->channel_);
    }

  track.disconnecting_ = false;

  z_debug ("done disconnecting");
}

std::string
Tracklist::print_port_connection (const dsp::PortConnection &conn) const
{
  auto src_var = port_registry_->find_by_id_or_throw (conn.src_id_);
  auto dest_var = port_registry_->find_by_id_or_throw (conn.dest_id_);
  return std::visit (
    [&] (auto &&src, auto &&dest) {
      auto is_send =
        src->id_->owner_type_ == dsp::PortIdentifier::OwnerType::ChannelSend;
      const char * send_str = is_send ? " (send)" : "";
      if (port_connections_manager_->contains_connection (conn))
        {
          auto src_track_var = get_track_registry ().find_by_id_or_throw (
            src->id_->track_id_.value ());
          auto dest_track_var = get_track_registry ().find_by_id_or_throw (
            dest->id_->track_id_.value ());
          return std::visit (
            [&] (auto &&src_track, auto &&dest_track) {
              return fmt::format (
                "[{} ({})] {} => [{} ({})] {}{}",
                (src_track != nullptr) ? src_track->get_name () : u8"(none)",
                src->id_->track_id_, src->get_label (),
                dest_track ? dest_track->get_name () : u8"(none)",
                dest->id_->track_id_, dest->get_label (), send_str);
            },
            src_track_var, dest_track_var);
        }

      return fmt::format (
        "[track {}] {} => [track {}] {}{}", src->id_->track_id_,
        src->get_label (), dest->id_->track_id_, dest->get_label (), send_str);
    },
    src_var, dest_var);
}

void
Tracklist::move_plugin_automation (
  const Plugin::Uuid         &plugin_id,
  const Track::Uuid          &prev_track_id,
  const Track::Uuid          &track_id_to_move_to,
  zrythm::plugins::PluginSlot new_slot)
{
  auto pl_var = plugin_registry_->find_by_id_or_throw (plugin_id);
  auto prev_track_var = track_registry_->find_by_id_or_throw (prev_track_id);
  auto track_var = track_registry_->find_by_id_or_throw (track_id_to_move_to);
  std::visit (
    [&] (auto &&plugin, auto &&prev_track, auto &&track) {
      using PrevTrackT = base_type<decltype (prev_track)>;
      using TrackT = base_type<decltype (track)>;
      z_debug (
        "moving plugin '{}' automation from {} to {} -> {}", plugin->get_name (),
        prev_track->get_name (), track->get_name (), new_slot);

      if constexpr (
        std::derived_from<PrevTrackT, AutomatableTrack>
        && std::derived_from<TrackT, AutomatableTrack>)
        {
          auto &prev_atl = prev_track->get_automation_tracklist ();
          auto &atl = track->get_automation_tracklist ();

          for (auto * at : prev_atl.get_automation_tracks ())
            {
              auto port_var = port_registry_->find_by_id (at->port_id_);
              if (!port_var)
                continue;

              z_return_if_fail (
                std::holds_alternative<ControlPort *> (port_var->get ()));
              auto * port = std::get<ControlPort *> (port_var->get ());
              if (
                port->id_->owner_type_ == dsp::PortIdentifier::OwnerType::Plugin)
                {
                  auto port_pl = plugin_registry_->find_by_id (
                    port->id_->plugin_id_.value ());
                  if (!port_pl.has_value ())
                    continue;

                  bool match = std::visit (
                    [&] (auto &&p) { return p == plugin; }, port_pl->get ());
                  if (!match)
                    continue;
                }
              else
                continue;

              /* delete from prev channel */
              auto   num_regions_before = at->get_children_vector ().size ();
              auto * removed_at = prev_atl.remove_at (*at, false, false);

              /* add to new channel */
              auto added_at = atl.add_automation_track (*removed_at);
              z_return_if_fail (
                added_at == atl.get_automation_track_at (added_at->index_)
                && added_at->get_children_vector ().size () == num_regions_before);
            }
        }
    },
    pl_var, prev_track_var, track_var);
}

Channel *
Tracklist::get_channel_for_plugin (const Plugin::Uuid &plugin_id)
{
  auto pl_var = plugin_registry_->find_by_id_or_throw (plugin_id);
  return std::visit (
    [&] (auto &&pl) {
      assert (pl->track_id_.has_value ());
      auto track_var = get_track (*pl->track_id_);
      assert (track_var.has_value ());
      return std::visit (
        [] (auto &&track) -> Channel * {
          using TrackT = base_type<decltype (track)>;
          if constexpr (std::derived_from<TrackT, ChannelTrack>)
            {
              return track->get_channel ();
            }
          throw std::runtime_error ("Plugin not in a channel");
        },
        *track_var);
    },
    pl_var);
}

void
Tracklist::mark_track_for_bounce (
  TrackPtrVariant track_var,
  bool            bounce,
  bool            mark_regions,
  bool            mark_children,
  bool            mark_parents)
{
// TODO
#if 0
  std::visit (
    [&] (auto &&track) {
      using TrackT = base_type<decltype (track)>;
      if constexpr (!std::derived_from<TrackT, ChannelTrack>)
        return;

      z_debug (
        "marking {} for bounce {}, mark regions {}", track->get_name (), bounce,
        mark_regions);

      track->bounce_ = bounce;

      if (mark_regions)
        {
          if constexpr (std::derived_from<TrackT, LanedTrack>)
            {
              for (auto &lane_var : track->lanes_)
                {
                  using LaneT = TrackT::TrackLaneType;
                  auto lane = std::get<LaneT *> (lane_var);
                  for (auto * region : lane->get_children_view ())
                    {
                      region->bounce_ = bounce;
                    }
                }
            }

          if constexpr (std::is_same_v<TrackT, ChordTrack>)
            for (
              auto * region :
              track->arrangement::template ArrangerObjectOwner<
                arrangement::ChordRegion>::get_children_view ())
              {
                region->bounce_ = bounce;
              }
        }

      if constexpr (std::derived_from<TrackT, ChannelTrack>)
        {
          auto * direct_out = track->get_channel ()->get_output_track ();
          if (direct_out && mark_parents)
            {
              std::visit (
                [&] (auto &&direct_out_derived) {
                  mark_track_for_bounce (
                    direct_out_derived, bounce, false, false, true);
                },
                convert_to_variant<TrackPtrVariant> (direct_out));
            }
        }

      if (mark_children)
        {
          if constexpr (std::derived_from<TrackT, GroupTargetTrack>)
            {
              for (auto child_id : track->children_)
                {
                  if (
                    auto child_var =
                      track->get_tracklist ()->get_track (child_id))
                    {
                      std::visit (
                        [&] (auto &&c) {
                          c->bounce_to_master_ = track->bounce_to_master_;
                          mark_track_for_bounce (
                            c, bounce, mark_regions, true, false);
                        },
                        *child_var);
                    }
                }
            }
        }
    },
    track_var);
#endif
}

int
Tracklist::get_visible_track_diff (
  Track::TrackUuid src_track,
  Track::TrackUuid dest_track) const
{
  const auto src_track_index = std::distance (
    tracks_.begin (),
    std::ranges::find (tracks_, src_track, &TrackUuidReference::id));
  const auto dest_track_index = std::distance (
    tracks_.begin (),
    std::ranges::find (tracks_, dest_track, &TrackUuidReference::id));

  // Determine range boundaries and direction
  const auto [lower, upper] = std::minmax (src_track_index, dest_track_index);
  const int sign = src_track_index < dest_track_index ? 1 : -1;

  // Count visible tracks in the range (exclusive upper bound)
  auto       span = get_track_span ();
  const auto count = std::ranges::count_if (
    span.begin () + lower, span.begin () + upper, TrackSpan::visible_projection);

  // Apply direction modifier
  return sign * count;
}

void
Tracklist::swap_tracks (const size_t index1, const size_t index2)
{
  z_return_if_fail (std::max (index1, index2) < tracks_.size ());
  AtomicBoolRAII raii{ swapping_tracks_ };

  {
    auto        span = get_track_span ();
    const auto &src_track = Track::from_variant (span.at (index1));
    const auto &dest_track = Track::from_variant (span.at (index2));
    z_debug (
      "swapping tracks {} [{}] and {} [{}]...",
      src_track ? src_track->get_name () : u8"(none)", index1,
      dest_track ? dest_track->get_name () : u8"(none)", index2);
  }

  std::iter_swap (tracks_.begin () + index1, tracks_.begin () + index2);

  {
    auto        span = get_track_span ();
    const auto &src_track = Track::from_variant (span.at (index1));
    const auto &dest_track = Track::from_variant (span.at (index2));

    if (src_track)
      src_track->set_index (index1);
    if (dest_track)
      dest_track->set_index (index2);
  }

  z_debug ("tracks swapped");
}

std::optional<arrangement::ArrangerObjectPtrVariant>
Tracklist::get_region_at_pos (
  signed_frame_t            pos_samples,
  Track *                   track,
  tracks::AutomationTrack * at,
  bool                      include_region_end)
{
  auto is_at_pos = [&] (const auto &region_var) -> bool {
    return std::visit (
      [&] (auto &&region) -> bool {
        using RegionT = base_type<decltype (region)>;
        if constexpr (arrangement::RegionObject<RegionT>)
          {
            const auto region_pos = region->position ()->samples ();
            const auto region_end_pos =
              region->regionMixin ()->bounds ()->get_end_position_samples (true);
            return region_pos <= pos_samples
                   && (include_region_end ? region_end_pos >= pos_samples : region_end_pos > pos_samples);
          }
        else
          {
            throw std::runtime_error ("expected region");
          }
      },
      region_var.get_object ());
  };

  if (track)
    {
      return std::visit (
        [&] (auto &&track_derived) -> std::optional<ArrangerObjectPtrVariant> {
          using TrackT = base_type<decltype (track_derived)>;
          if constexpr (std::derived_from<TrackT, tracks::LanedTrack>)
            {
              for (auto &lane_var : track_derived->lanes_)
                {
                  using TrackLaneT = TrackT::LanedTrackImpl::TrackLaneType;
                  auto lane = std::get<TrackLaneT *> (lane_var);
                  auto ret_var =
                    arrangement::ArrangerObjectSpan{ lane->get_children_vector () }
                      .get_bounded_object_at_position (
                        pos_samples, include_region_end);
                  if (ret_var)
                    return std::get<typename TrackLaneT::RegionT *> (*ret_var);

                  auto region_vars = lane->get_children_vector ();
                  auto it = std::ranges::find_if (region_vars, is_at_pos);
                  if (it != region_vars.end ())
                    {
                      return (*it).get_object ();
                    }
                }
            }
          if constexpr (std::is_same_v<TrackT, tracks::ChordTrack>)
            {
              auto region_vars = track_derived->template ArrangerObjectOwner<
                arrangement::ChordRegion>::get_children_vector ();
              auto it = std::ranges::find_if (region_vars, is_at_pos);
              if (it != region_vars.end ())
                {
                  return (*it).get_object ();
                }
            }
          return std::nullopt;
        },
        convert_to_variant<TrackPtrVariant> (track));
    }
  if (at)
    {
      auto region_vars = at->get_children_vector ();
      auto it = std::ranges::find_if (region_vars, is_at_pos);
      if (it != region_vars.end ())
        {
          return (*it).get_object ();
        }
      return std::nullopt;
    }
  z_return_val_if_reached (std::nullopt);
}

TrackPtrVariant
Tracklist::insert_track (
  const TrackUuidReference       &track_id,
  int                             pos,
  engine::device_io::AudioEngine &engine,
  bool                            publish_events,
  bool                            recalc_graph)
{
  auto track_var = track_id.get_object ();

  beginResetModel ();
  std::visit (
    [&] (auto &&track) {
      using TrackT = base_type<decltype (track)>;
      z_info ("inserting {} at {}", track->get_name (), pos);

      /* throw error if attempted to add a special track (like master) when it
       * already exists */
      if (
        !Track::type_is_deletable (Track::get_type_for_class<TrackT> ())
        && get_track_span ().contains_type<TrackT> ())
        {
          z_error (
            "cannot add track of type {} when it already exists",
            Track::get_type_for_class<TrackT> ());
          return;
        }

      /* set to -1 so other logic knows it is a new track */
      track->set_index (-1);

      /* this needs to be called before appending the track to the tracklist */
      track->set_name (*this, track->get_name (), false);

      /* append the track at the end */
      tracks_.emplace_back (track_id);
      track->set_selection_status_getter ([&] (const TrackUuid &id) {
        return TrackSelectionManager{ selected_tracks_, *track_registry_ }
          .is_selected (id);
      });
      track->tracklist_ = this;

      /* remember important tracks */
      if constexpr (std::is_same_v<TrackT, MasterTrack>)
        master_track_ = track;
      else if constexpr (std::is_same_v<TrackT, ChordTrack>)
        chord_track_ = track;
      else if constexpr (std::is_same_v<TrackT, MarkerTrack>)
        marker_track_ = track;
      else if constexpr (std::is_same_v<TrackT, ModulatorTrack>)
        modulator_track_ = track;

      /* add flags for auditioner track ports */
      if (is_auditioner ())
        {
          std::vector<Port *> ports;
          track->append_ports (ports, true);
          for (auto * port : ports)
            {
              port->id_->flags_ |=
                dsp::PortIdentifier::Flags::SampleProcessorTrack;
            }
        }

      /* if inserting it, swap until it reaches its position */
      if (static_cast<size_t> (pos) != tracks_.size () - 1)
        {
          for (int i = static_cast<int> (tracks_.size ()) - 1; i > pos; --i)
            {
              swap_tracks (i, i - 1);
            }
        }

      track->set_index (pos);

      if (
        /* auditioner doesn't need automation */
        !is_auditioner ())
        {
          /* make the track the only selected track */
          get_selection_manager ().select_unique (track->get_uuid ());
        }

      if constexpr (std::derived_from<TrackT, ChannelTrack>)
        {
          z_return_if_fail (port_connections_manager_);
          track->channel_->connect_channel (*port_connections_manager_, engine);
        }

      /* if audio output route to master */
      if constexpr (!std::is_same_v<TrackT, MasterTrack>)
        {
          if (
            track->get_output_signal_type () == dsp::PortType::Audio
            && master_track_)
            {
              master_track_->add_child (track->get_uuid (), true, false, false);
            }
        }

      track->activate_all_plugins (true);

      if (recalc_graph)
        {
          ROUTER->recalc_graph (false);
        }

      if (publish_events)
        {
          // EVENTS_PUSH (EventType::ET_TRACK_ADDED, added_track);
        }

      z_debug (
        "done - inserted track '{}' ({}) at {}", track->get_name (),
        track->get_uuid (), pos);
    },
    track_var);
  endResetModel ();
  return track_var;
}

ChordTrack *
Tracklist::get_chord_track () const
{
  auto span = get_track_span ();
  return std::get<ChordTrack *> (
    *std::ranges::find_if (span, TrackSpan::type_projection<ChordTrack>));
}

AutomationTrack *
Tracklist::get_automation_track_for_port (const Port::Uuid &port_id) const
{
  if (port_to_at_mappings_.contains (port_id))
    {
      auto * at = port_to_at_mappings_.value (port_id);
      if (at->port_id_ == port_id) [[likely]]
        {
          return at;
        }
    }

  for (
    const auto * track :
    get_track_span ().get_elements_derived_from<AutomatableTrack> ())
    {
      auto at =
        track->get_automation_tracklist ().get_automation_track_by_port_id (
          port_id);
      if (at != nullptr)
        {
          port_to_at_mappings_.insertOrAssign (port_id, at);
          return at;
        }
    }
  z_return_val_if_reached (nullptr);
}

struct PluginMoveData
{
  Plugin *                pl = nullptr;
  OptionalTrackPtrVariant track_var;
  plugins::PluginSlot     slot;
};

#if 0
static void
plugin_move_data_free (void * _data, GClosure * closure)
{
  PluginMoveData * self = (PluginMoveData *) _data;
  object_delete_and_null (self);
}
#endif

static void
do_move (PluginMoveData * data)
{
  auto * pl = data->pl;
  auto   prev_slot = *pl->get_slot ();
  auto * prev_track = TrackSpan::derived_type_transformation<AutomatableTrack> (
    *pl->get_track ());
  auto * prev_ch = TRACKLIST->get_channel_for_plugin (pl->get_uuid ());
  z_return_if_fail (prev_ch);

  std::visit (
    [&] (auto &&data_track) {
      using TrackT = base_type<decltype (data_track)>;
      if constexpr (std::derived_from<TrackT, ChannelTrack>)
        {
          /* if existing plugin exists, delete it */
          auto existing_pl = data_track->get_plugin_at_slot (data->slot);
          if (existing_pl)
            {
              data_track->remove_plugin (data->slot, false, true);
            }

          /* move plugin's automation from src to dest */
          TRACKLIST->move_plugin_automation (
            pl->get_uuid (), prev_track->get_uuid (), data_track->get_uuid (),
            data->slot);

          /* remove plugin from its channel */
          PluginUuidReference plugin_ref{
            pl->get_uuid (), data_track->get_plugin_registry ()
          };
          prev_ch->track_->remove_plugin (prev_slot, true, false);

          /* add plugin to its new channel */
          data_track->channel_->add_plugin (
            plugin_ref, data->slot, false, true, false, true, true);

#if 0
          if (data->fire_events)
            {
      // EVENTS_PUSH (EventType::ET_CHANNEL_SLOTS_CHANGED, prev_ch);
      EVENTS_PUSH (
        EventType::ET_CHANNEL_SLOTS_CHANGED,
        data_channel_track->channel_.get ());
      }
#endif
        }
    },
    *data->track_var);
}

#if 0
static void
overwrite_plugin_response_cb (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  PluginMoveData * data = (PluginMoveData *) user_data;
  if (!string_is_equal (response, "overwrite"))
    {
      return;
    }

  do_move (data);
}
#endif

void
Tracklist::move_plugin (
  const Plugin::Uuid &plugin_id,
  const Track::Uuid  &target_track_id,
  plugins::PluginSlot slot,
  bool                confirm_overwrite)
{
  auto pl_var = plugin_registry_->find_by_id_or_throw (plugin_id);
  auto track_var = get_track (target_track_id);
  std::visit (
    [&] (auto &&plugin, auto &&track) {
      using TrackT = base_type<decltype (track)>;
      auto data = std::make_unique<PluginMoveData> ();
      data->pl = plugin;
      data->track_var = track;
      data->slot = slot;

      if constexpr (std::derived_from<TrackT, AutomatableTrack>)
        {
          auto existing_pl = track->get_plugin_at_slot (slot);
          if (existing_pl && confirm_overwrite && ZRYTHM_HAVE_UI)
            {
#if 0
      auto dialog =
        dialogs_get_overwrite_plugin_dialog (GTK_WINDOW (MAIN_WINDOW));
      gtk_window_present (GTK_WINDOW (dialog));
      g_signal_connect_data (
        dialog, "response", G_CALLBACK (overwrite_plugin_response_cb),
        data.release (), plugin_move_data_free, G_CONNECT_DEFAULT);
#endif
              return;
            }

          do_move (data.get ());
        }
    },
    pl_var, *track_var);
}

bool
Tracklist::multiply_track_heights (
  double multiplier,
  bool   visible_only,
  bool   check_only,
  bool   fire_events)
{
  auto span = get_track_span ();
  for (const auto &track_var : span)
    {
      bool ret = std::visit (
        [&] (auto &&track) {
          if (visible_only && !track->should_be_visible ())
            return true;

          if (!track->multiply_heights (multiplier, visible_only, check_only))
            {
              return false;
            }

          if (!check_only && fire_events)
            {
              /* FIXME should be event */
              // track_widget_update_size (tr->widget_);
            }

          return true;
        },
        track_var);
      if (!ret)
        {
          return false;
        }
    }

  return true;
}

int
Tracklist::get_last_pos (const PinOption pin_opt, const bool visible_only) const
{
  auto span =

    std::views::filter (
      get_track_span (),
      [&] (const auto &track_var) {
        const auto &tr = Track::from_variant (track_var);
        const bool  is_pinned = is_track_pinned (tr->get_index ());
        if (pin_opt == PinOption::PinnedOnly && !is_pinned)
          return false;
        if (pin_opt == PinOption::UnpinnedOnly && is_pinned)
          return false;
        if (visible_only && !tr->should_be_visible ())
          return false;

        return true;
      })
    | std::views::reverse | std::views::take (1);

  /* no track with given options found, select the last */
  if (span.empty ())
    return static_cast<int> (tracks_.size ()) - 1;

  return std::visit (
    [&] (const auto &track) { return track->get_index (); }, span.front ());
}

std::optional<TrackPtrVariant>
Tracklist::get_visible_track_after_delta (Track::TrackUuid track_id, int delta)
  const
{
  auto span =
    get_track_span () | std::views::filter (TrackSpan::visible_projection);
  auto current_it =
    std::ranges::find (span, track_id, TrackSpan::uuid_projection);
  auto found = std::ranges::next (current_it, delta);
  if (found == span.end ())
    return std::nullopt;
  return *(found);
}

std::optional<TrackPtrVariant>
Tracklist::get_first_visible_track (const bool pinned) const
{
  auto span = get_track_span ().get_visible_tracks ();
  auto it = std::ranges::find_if (span, [&] (const auto &track_var) {
    return std::visit (
      [&] (auto &&track) {
        return is_track_pinned (track->get_uuid ()) == pinned;
      },
      track_var);
  });
  return it != span.end () ? std::make_optional (*it) : std::nullopt;
}

std::optional<TrackPtrVariant>
Tracklist::get_prev_visible_track (TrackUuid track_id) const
{
  return get_visible_track_after_delta (track_id, -1);
}

std::optional<TrackPtrVariant>
Tracklist::get_next_visible_track (TrackUuid track_id) const
{
  return get_visible_track_after_delta (track_id, 1);
}

void
Tracklist::remove_track (const TrackUuid &track_id)
{
  auto track_it = std::ranges::find (tracks_, track_id, &TrackUuidReference::id);
  z_return_if_fail (track_it != tracks_.end ());
  const auto track_index = std::distance (tracks_.begin (), track_it);
  auto       span = get_track_span ();
  auto       track_var = span.at (track_index);
  std::visit (
    [&] (auto &&track) {
      z_return_if_fail (track->get_index () == track_index);
      z_debug (
        "removing [{}] {} - num tracks before deletion: {}", track_index,
        track->get_name (), tracks_.size ());

      beginRemoveRows ({}, track_index, track_index);

      std::optional<TrackPtrVariant> prev_visible = std::nullopt;
      std::optional<TrackPtrVariant> next_visible = std::nullopt;
      if (!is_auditioner ())
        {
          prev_visible = get_prev_visible_track (track_id);
          next_visible = get_next_visible_track (track_id);
        }

      /* remove/deselect all objects */
      track->clear_objects ();

      disconnect_track (*track);

      /* move track to the end */
      auto end_pos = std::ssize (tracks_) - 1;
      move_track (track_id, end_pos, false, std::nullopt);

      get_selection_manager ().remove_from_selection (track->get_uuid ());

      track_it = std::ranges::find (tracks_, track_id, &TrackUuidReference::id);
      z_return_if_fail (track_it != tracks_.end ());
      tracks_.erase (track_it);
      track->unset_selection_status_getter ();

      // recreate the span because underlying vector changed
      span = get_track_span ();

      /* if it was the only track selected, select the next one */
      if (get_selection_manager ().empty ())
        {
          auto track_to_select = next_visible ? next_visible : prev_visible;
          if (!track_to_select && !tracks_.empty ())
            {
              track_to_select = span.at (0);
            }
          if (track_to_select)
            {
              get_selection_manager ().append_to_selection (
                TrackSpan::uuid_projection (*track_to_select));
            }
        }

      endRemoveRows ();

      z_debug ("done removing track {}", track->getName ());
    },
    track_var);
}

void
Tracklist::clear_selections_for_object_siblings (
  const ArrangerObject::Uuid &object_id)
{
// TODO
#if 0
  auto obj_var = PROJECT->find_arranger_object_by_id (object_id);
  std::visit (
    [&] (auto &&obj) {
      using ObjT = base_type<decltype (obj)>;
      if constexpr (std::derived_from<ObjT, arrangement::RegionOwnedObject>)
        {
          auto region = obj->get_region ();
          for (auto * child : region->get_children_view ())
            {
              auto selection_mgr =
                arrangement::ArrangerObjectFactory::get_instance ()
                  ->get_selection_manager_for_object (*child);
              selection_mgr.remove_from_selection (child->get_uuid ());
            }
        }
      else
        {
          auto tl_objs = get_timeline_objects_in_range ();
          for (const auto &tl_obj_var : tl_objs)
            {
              std::visit (
                [&] (auto &&tl_obj) {
                  auto selection_mgr =
                    arrangement::ArrangerObjectFactory::get_instance ()
                      ->get_selection_manager_for_object (*tl_obj);
                  selection_mgr.remove_from_selection (tl_obj->get_uuid ());
                },
                tl_obj_var);
            }
        }
    },
    *obj_var);
#endif
}

std::vector<arrangement::ArrangerObjectPtrVariant>
Tracklist::get_timeline_objects_in_range (
  std::optional<std::pair<dsp::Position, dsp::Position>> range) const
{
  if (!range)
    {
      dsp::Position pos;
      pos.set_to_bar (
        dsp::Position::POSITION_MAX_BAR,
        PROJECT->getTransport ()->ticks_per_bar_,
        AUDIO_ENGINE->frames_per_tick_);
      range.emplace (dsp::Position{}, pos);
    }
  std::vector<ArrangerObjectPtrVariant> ret;
// TODO
#if 0
  for (const auto &track : get_track_span ())
    {
      std::vector<ArrangerObjectPtrVariant> objs;
      std::visit (
        [&] (auto &&tr) {
          tr->append_objects (objs);

          for (auto &obj_var : objs)
            {
              std::visit (
                [&] (auto &&o) {
                  using ObjT = base_type<decltype (o)>;
                  if constexpr (arrangement::BoundedObject<ObjT>)
                    {
                      if (arrangement::ArrangerObjectSpan::bounds_projection (o)
                            ->is_hit_by_range (range->first, range->second))
                        {
                          ret.push_back (o);
                        }
                    }
                  else
                    {
                      if (o->is_start_hit_by_range (range->first, range->second))
                        {
                          ret.push_back (o);
                        }
                    }
                },
                obj_var);
            }
        },
        track);
    }
#endif
  return ret;
}

void
Tracklist::move_track (
  const TrackUuid track_id,
  int             pos,
  bool            always_before_pos,
  std::optional<std::reference_wrapper<engine::session::Router>> router)
{
  auto       track_var = get_track (track_id);
  const auto track_index = get_track_index (track_id);

  std::visit (
    [&] (auto &&track) {
      z_return_if_fail (track_index == track->get_index ());
      z_debug (
        "moving track: {} from {} to {}", track->get_name (), track_index, pos);

      if (pos == track_index)
        return;

      beginMoveRows ({}, track_index, track_index, {}, pos);

      bool move_higher = pos < track_index;

      auto prev_visible = get_prev_visible_track (track_id);
      auto next_visible = get_next_visible_track (track_id);

      if (!is_auditioner ())
        {
          /* clear the editor region if it exists and belongs to this track */
          // CLIP_EDITOR->unset_region_if_belongs_to_track (track_id);

          /* deselect all objects */
          track->Track::unselect_all ();

          get_selection_manager ().remove_from_selection (track->get_uuid ());

          /* if it was the only track selected, select the next one */
          if (
            get_selection_manager ().empty () && (prev_visible || next_visible))
            {
              auto track_to_add = next_visible ? *next_visible : *prev_visible;
              get_selection_manager ().append_to_selection (
                TrackSpan::uuid_projection (track_to_add));
            }
        }

      /* the current implementation currently moves some tracks to tracks.size()
       * + 1 temporarily, so we expand the vector here and resize it back at the
       * end */
      bool expanded = false;
      if (pos >= static_cast<int> (tracks_.size ()))
        {
          tracks_.emplace_back (*track_registry_);
          // tracks_.resize (pos + 1);
          expanded = true;
        }

      if (move_higher)
        {
          /* move all other tracks 1 track further */
          for (int i = track_index; i > pos; i--)
            {
              swap_tracks (i, i - 1);
            }
        }
      else
        {
          /* move all other tracks 1 track earlier */
          for (int i = track_index; i < pos; i++)
            {
              swap_tracks (i, i + 1);
            }

          if (always_before_pos && pos > 0)
            {
              /* swap with previous track */
              swap_tracks (pos, pos - 1);
            }
        }

      if (expanded)
        {
          /* resize back */
          tracks_.erase (tracks_.end () - 1);
        }

      /* make the track the only selected track */
      get_selection_manager ().select_unique (track_id);

      if (router)
        {
          router->get ().recalc_graph (false);
        }

      endMoveRows ();

      z_debug ("finished moving track");
    },
    *track_var);
}

bool
Tracklist::track_name_is_unique (
  const utils::Utf8String &name,
  const TrackUuid          track_to_skip) const
{
  auto track_ids_to_check = std::ranges::to<std::vector> (std::views::filter (
    tracks_, [&] (const auto &id) { return id.id () != track_to_skip; }));
  return !TrackSpan{ track_ids_to_check }.contains_track_name (name);
}

// TODO
#if 0
void
Tracklist::import_regions (
  std::vector<std::vector<std::shared_ptr<Region>>> &region_arrays,
  const FileImportInfo *                             import_info,
  TracksReadyCallback                                ready_cb)
{
  z_debug ("Adding regions into the project...");

  AudioEngine::State state{};
  AUDIO_ENGINE->wait_for_pause (state, false, true);
  int executed_actions = 0;
  try
    {
      for (auto regions : region_arrays)
        {
          z_debug ("REGION ARRAY ({} elements)", regions.size ());
          int i = 0;
          while (!regions.empty ())
            {
              int iter = i++;
              z_debug ("REGION {}", iter);
              auto r = regions.back ();
              regions.pop_back ();
              Track::Type track_type = Track::Type::Audio;
              bool        gen_name = true;
              if (r->is_midi ())
                {
                  track_type = Track::Type::Midi;
                  if (!r->name_.empty ())
                    gen_name = false;
                }
              else if (!r->is_audio ())
                {
                  z_warning ("Unknown region type");
                  continue;
                }

              Track * track = nullptr;
              if (import_info->track_name_hash_)
                {
                  track =
                    find_track_by_name_hash (import_info->track_name_hash_);
                }
              else
                {
                  int index = import_info->track_idx_ + iter;
                  Track::create_empty_at_idx_with_action (track_type, index);
                  auto tmp = get_track (index);
                  std::visit ([&track] (auto * t) { track = t; }, tmp);
                  executed_actions++;
                }
              z_return_if_fail (track);
              track->add_region_plain (r, nullptr, 0, gen_name, false);
              r->select (true, false, true);
              UNDO_MANAGER->perform (
                std::make_unique<CreateArrangerSelectionsAction> (
                  *TL_SELECTIONS));
              ++executed_actions;
            }
        }
    }
  catch (const ZrythmException &e)
    {
      z_warning ("Failed to import regions: {}", e.what ());

      /* undo any performed actions */
      while (executed_actions > 0)
        {
          UNDO_MANAGER->undo ();
          --executed_actions;
        }

      /* rethrow the exception */
      throw;
    }

  if (executed_actions > 0)
    {
      auto last_action = UNDO_MANAGER->get_last_action ();
      last_action->num_actions_ = executed_actions;
    }

  AUDIO_ENGINE->resume (state);

  if (ready_cb)
    {
      ready_cb (import_info);
    }
  }
#endif

void
Tracklist::move_region_to_track (
  ArrangerObjectPtrVariant region_var,
  const Track::Uuid       &to_track_id,
  int                      lane_or_at_index,
  int                      index)
{
#if 0
  // TODO
  auto to_track_var = get_track (to_track_id);
  assert (to_track_var);

  std::visit (
    [&] (auto &&to_track, auto &&region) {
      using RegionT = base_type<decltype (region)>;
      if constexpr (std::derived_from<RegionT, Region>)
        {
          auto from_track_var = *get_track (*region->get_track_id ());
          std::visit (
            [&] (auto &&region_track) {
              z_debug (
                "moving region {} to track {}", region->get_name (),
                to_track->get_name ());

              const bool selected = region->getSelected ();
              auto       clip_editor_region = CLIP_EDITOR->get_region ();

          // TODO remove region from owner

// TODO
#  if 0
      int lane_pos = lane_or_at_index >= 0 ? lane_or_at_index : id_.lane_pos_;
      int at_pos = lane_or_at_index >= 0 ? lane_or_at_index : id_.at_idx_;

      if constexpr (is_automation ())
        {
          auto  automatable_track = dynamic_cast<AutomatableTrack *> (track);
          auto &at = automatable_track->automation_tracklist_->ats_[at_pos];

          /* convert the automation points to match the new automatable */
          auto port_var = PROJECT->find_port_by_id (at->port_id_);
          z_return_if_fail (
            port_var.has_value ()
            && std::holds_alternative<ControlPort *> (port_var.value ()));
          auto * port = std::get<ControlPort *> (port_var.value ());
          z_return_if_fail (port);
          for (auto &ap : get_derived ().aps_)
            {
              ap->fvalue_ = port->normalized_val_to_real (ap->normalized_val_);
            }

          /* add the region to its new track */
          try
            {
              if (index >= 0)
                {
                  track->insert_region (
                    shared_this, at, -1, index, false, false);
                }
              else
                {
                  track->add_region (shared_this, at, -1, false, false);
                }
            }
          catch (const ZrythmException &e)
            {
              e.handle ("Failed to add region to track");
              return;
            }

          z_warn_if_fail (id_.at_idx_ == at->index_);
          get_derived ().set_automation_track (*at);
        }
      else
        {
          if constexpr (is_laned ())
            {
              /* create lanes if they don't exist */
              auto laned_track = dynamic_cast<LanedTrackImpl<
                typename LaneOwnedObject<RegionT>::TrackLaneT> *> (track);
              z_return_if_fail (laned_track);
              laned_track->create_missing_lanes (lane_pos);
            }

          /* add the region to its new track */
          try
            {
              if (index >= 0)
                {
                  track->insert_region (
                    shared_this, nullptr, lane_pos, index, false, false);
                }
              else
                {
                  track->add_region (
                    shared_this, nullptr, lane_pos, false, false);
                }
            }
          catch (const ZrythmException &e)
            {
              e.handle ("Failed to add region to track");
              return;
            }

          z_return_if_fail (id_.lane_pos_ == lane_pos);

          if constexpr (is_laned ())
            {
              using TrackLaneT = typename LaneOwnedObject<RegionT>::TrackLaneT;
              auto laned_track =
                dynamic_cast<LanedTrackImpl<TrackLaneT> *> (track);
              auto lane =
                std::get<TrackLaneT *> (laned_track->lanes_.at (lane_pos));
              z_return_if_fail (
                !lane->region_list_->regions_.empty ()
                && std::get<RegionT *> (lane->region_list_->regions_.at (id_.idx_))
                     == dynamic_cast<RegionT *> (this));
              get_derived ().set_lane (*lane);

              laned_track->create_missing_lanes (lane_pos);

              if (region_track)
                {
                  /* remove empty lanes if the region was the last on its track
                   * lane
                   */
                  auto region_laned_track = dynamic_cast<LanedTrackImpl<
                    typename LaneOwnedObject<RegionT>::TrackLaneT> *> (
                    region_track);
                  region_laned_track->remove_empty_last_lanes ();
                }
            }

          if (link_group)
            {
              link_group->add_region (*this);
            }
        }
#  endif

              /* reset the clip editor region because track_remove_region clears
               * it */
              if (
                clip_editor_region.has_value ()
                && std::holds_alternative<RegionT *> (
                  clip_editor_region.value ()))
                {
                  if (
                    std::get<RegionT *> (clip_editor_region.value ())
                    == dynamic_cast<RegionT *> (this))
                    {
                      {
                        CLIP_EDITOR->set_region (region->get_uuid ());
                      }
                    }
                }

              /* reselect if necessary */
              if (selected)
                {
                  structure::arrangement::ArrangerObjectFactory::get_instance ()
                    ->get_selection_manager_for_object (*region)
                    .append_to_selection (region->get_uuid ());
                }

              // z_debug ("after: {}", get_derived ());
            },
            from_track_var);
        }
    },
    *to_track_var, region_var);
#endif
}

void
Tracklist::import_files (
  std::optional<std::vector<utils::Utf8String>> uri_list,
  const FileDescriptor *                        orig_file,
  const Track *                                 track,
  const TrackLane *                             lane,
  int                                           index,
  const dsp::Position *                         pos,
  TracksReadyCallback                           ready_cb)
{
  std::vector<FileDescriptor> file_arr;
  if (orig_file)
    {
      file_arr.push_back (*orig_file);
    }
  else
    {
      for (const auto &uri : *uri_list)
        {
          if (!uri.contains_substr (u8"file://"))
            continue;

          auto file = FileDescriptor::new_from_uri (uri);
          file_arr.push_back (*file);
        }
    }

  if (file_arr.empty ())
    {
      throw ZrythmException (QObject::tr ("No file was found"));
    }
  else if (track && file_arr.size () > 1)
    {
      throw ZrythmException (
        QObject::tr ("Can only drop 1 file at a time on existing tracks"));
    }

  for (const auto &file : file_arr)
    {
      if (file.is_supported () && file.is_audio ())
        {
          if (track && !track->is_audio ())
            {
              throw ZrythmException (
                QObject::tr ("Can only drop audio files on audio tracks"));
            }
        }
      else if (file.is_midi ())
        {
          if (track && !track->has_piano_roll ())
            {
              throw ZrythmException (
                QObject::tr (
                  "Can only drop MIDI files on MIDI/instrument tracks"));
            }
        }
      else
        {
          auto descr = FileDescriptor::get_type_description (file.type_);
          throw ZrythmException (
            format_qstr (QObject::tr ("Unsupported file type {}"), descr));
        }
    }

  std::vector<fs::path> filepaths;
  for (const auto &file : file_arr)
    {
      filepaths.push_back (file.abs_path_);
    }

    // TODO
#if 0
  auto nfo = FileImportInfo (
    track ? track->name_hash_ : 0, lane ? lane->pos_ : 0,
    pos ? *pos : Position (),
    track ? track->pos_ : (index >= 0 ? index : TRACKLIST->tracks_.size ()));

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      for (const auto &filepath : filepaths)
        {

          auto     fi = file_import_new (filepath, &nfo);
          GError * err = nullptr;
          auto     regions = file_import_sync (fi, &err);
          if (err != nullptr)
            {
              throw ZrythmException (format_str (
                QObject::tr ("Failed to import file {}: {}"),
                filepath, err->message));
            }
          std::vector<std::vector<std::shared_ptr<Region>>> region_arrays;
          region_arrays.push_back (regions);
          import_regions (region_arrays, &nfo, ready_cb);

        }
    }
  else
    {
      auto filepaths_null_terminated = filepaths.getNullTerminated ();
      FileImportProgressDialog * dialog = file_import_progress_dialog_new (
        (const char **) filepaths_null_terminated, &nfo, ready_cb,
        UI_ACTIVE_WINDOW_OR_NULL);
      file_import_progress_dialog_run (dialog);
      g_strfreev (filepaths_null_terminated);
    }
#endif
}

#if 0
void
Tracklist::handle_move_or_copy (
  Track               &this_track,
  TrackWidgetHighlight location,
  GdkDragAction        action)
{
  z_debug (
    "this track '{}' - location {} - action {}", this_track.name_,
    track_widget_highlight_to_str (location),
    action == GDK_ACTION_COPY ? "copy" : "move");

  int pos = -1;
  if (location == TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_TOP)
    {
      pos = this_track.pos_;
    }
  else
    {
      auto next = get_next_visible_track (this_track);
      if (next)
        pos = next->pos_;
      /* else if last track, move to end */
      else if (this_track.pos_ == static_cast<int> (tracks_.size ()) - 1)
        pos = tracks_.size ();
      /* else if last visible track but not last track */
      else
        pos = this_track.pos_ + 1;
    }

  if (pos == -1)
    return;

  TRACKLIST_SELECTIONS->select_foldable_children ();

  if (action == GDK_ACTION_COPY)
    {
      if (TRACKLIST_SELECTIONS->contains_uncopyable_track ())
        {
          z_warning ("cannot copy - track selection contains uncopyable track");
          return;
        }

      if (location == TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE)
        {
          try
            {
              UNDO_MANAGER->perform (
                std::make_unique<CopyTracksInsideFoldableTrackAction> (
                  *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
                  *PORT_CONNECTIONS_MGR, this_track.pos_));
            }
          catch (const ZrythmException &e)
            {
              e.handle (QObject::tr ("Failed to copy tracks inside"));
              return;
            }
        }
      /* else if not highlighted inside */
      else
        {
          auto                                &tls = *TRACKLIST_SELECTIONS;
          int                                  num_tls = tls.get_num_tracks ();
          std::unique_ptr<TracklistSelections> after_tls;
          int  diff_between_track_below_and_parent = 0;
          bool copied_inside = false;
          if (static_cast<size_t> (pos) < tracks_.size ())
            {
              auto &track_below = *tracks_[pos];
              auto track_below_parent = track_below.get_direct_folder_parent ();
              tls.sort ();
              auto cur_parent = find_track_by_name (tls.track_names_[0]);

              if (track_below_parent)
                {
                  diff_between_track_below_and_parent =
                    track_below.pos_ - track_below_parent->pos_;
                }

              /* first copy inside new parent */
              if (track_below_parent && track_below_parent != cur_parent)
                {
                  try
                    {
                      UNDO_MANAGER->perform (
                        std::make_unique<CopyTracksInsideFoldableTrackAction> (
                          *tls.gen_tracklist_selections (),
                          *PORT_CONNECTIONS_MGR, track_below_parent->pos_));
                    }
                  catch (const ZrythmException &e)
                    {
                      e.handle (QObject::tr ("Failed to copy track inside"));
                      return;
                    }

                  after_tls = std::make_unique<TracklistSelections> ();
                  for (int j = 1; j <= num_tls; j++)
                    {
                      try
                        {
                          after_tls->add_track (
                            clone_unique_with_variant<TrackVariant> (
                              tracks_[track_below_parent->pos_ + j].get ()));
                        }
                      catch (const ZrythmException &e)
                        {
                          e.handle (QObject::tr ("Failed to clone/add track"));
                          return;
                        }
                    }

                  copied_inside = true;
                }
            }

          /* if not copied inside, copy normally */
          if (!copied_inside)
            {
              try
                {
                  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
                    *tls.gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
                    pos));
                }
              catch (const ZrythmException &e)
                {
                  e.handle (QObject::tr ("Failed to copy tracks"));
                  return;
                }
            }
          /* else if copied inside and there is a track difference, also move */
          else if (diff_between_track_below_and_parent != 0)
            {
              move_after_copying_or_moving_inside (
                *after_tls, diff_between_track_below_and_parent);
            }
        }
    }
  else if (action == GDK_ACTION_MOVE)
    {
      if (location == TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE)
        {
          if (TRACKLIST_SELECTIONS->contains_track (this_track))
            {
              if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
                {
                  ui_show_error_message (
                    QObject::tr ("Error"), QObject::tr ("Cannot drag folder into itself"));
                }
              return;
            }
          /* else if selections do not contain the track dragged into */
          else
            {
              try
                {
                  UNDO_MANAGER->perform (
                    std::make_unique<MoveTracksInsideFoldableTrackAction> (
                      *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
                      this_track.pos_));
                }
              catch (const ZrythmException &e)
                {
                  e.handle (QObject::tr ("Failed to move track inside folder"));
                  return;
                }
            }
        }
      /* else if not highlighted inside */
      else
        {
          auto                                &tls = *TRACKLIST_SELECTIONS;
          int                                  num_tls = tls.get_num_tracks ();
          std::unique_ptr<TracklistSelections> after_tls;
          int  diff_between_track_below_and_parent = 0;
          bool moved_inside = false;
          if (static_cast<size_t> (pos) < tracks_.size ())
            {
              auto &track_below = *tracks_[pos];
              auto track_below_parent = track_below.get_direct_folder_parent ();
              tls.sort ();
              auto cur_parent = find_track_by_name (tls.track_names_[0]);

              if (track_below_parent)
                {
                  diff_between_track_below_and_parent =
                    track_below.pos_ - track_below_parent->pos_;
                }

              /* first move inside new parent */
              if (track_below_parent && track_below_parent != cur_parent)
                {
                  try
                    {
                      UNDO_MANAGER->perform (
                        std::make_unique<MoveTracksInsideFoldableTrackAction> (
                          *tls.gen_tracklist_selections (),
                          track_below_parent->pos_));
                    }
                  catch (const ZrythmException &e)
                    {
                      e.handle (QObject::tr ("Failed to move track inside folder"));
                      return;
                    }

                  after_tls = std::make_unique<TracklistSelections> ();
                  for (int j = 1; j <= num_tls; j++)
                    {
                      try
                        {
                          const auto &cur_track =
                            tracks_[track_below_parent->pos_ + j];
                          std::visit (
                            [&] (auto &&cur_track_casted) {
                              after_tls->add_track (
                                cur_track_casted->clone_unique ());
                            },
                            convert_to_variant<TrackPtrVariant> (
                              cur_track.get ()));
                        }
                      catch (const ZrythmException &e)
                        {
                          e.handle (QObject::tr ("Failed to clone track"));
                          return;
                        }
                    }

                  moved_inside = true;
                }
            } /* endif moved to an existing track */

          /* if not moved inside, move normally */
          if (!moved_inside)
            {
              try
                {
                  UNDO_MANAGER->perform (std::make_unique<MoveTracksAction> (
                    *tls.gen_tracklist_selections (), pos));
                }
              catch (const ZrythmException &e)
                {
                  e.handle (QObject::tr ("Failed to move tracks"));
                  return;
                }
            }
          /* else if moved inside and there is a track difference, also move */
          else if (diff_between_track_below_and_parent != 0)
            {
              move_after_copying_or_moving_inside (
                *after_tls, diff_between_track_below_and_parent);
            }
        }
    } /* endif action is MOVE */
}
#endif

void
init_from (
  Tracklist             &obj,
  const Tracklist       &other,
  utils::ObjectCloneType clone_type)
{
  obj.pinned_tracks_cutoff_ = other.pinned_tracks_cutoff_;
  obj.track_registry_ = other.track_registry_;

  if (clone_type == utils::ObjectCloneType::Snapshot)
    {
      obj.tracks_ = other.tracks_;
      obj.selected_tracks_ = other.selected_tracks_;
    }
  else if (clone_type == utils::ObjectCloneType::NewIdentity)
    {
      obj.tracks_.clear ();
      obj.tracks_.reserve (other.tracks_.size ());
      auto span = other.get_track_span ();
      for (const auto &track_var : span)
        {
          std::visit (
            [&] (auto &tr) {
              auto id_ref = obj.track_registry_->clone_object (
                *tr, PROJECT->get_file_audio_source_registry (),
                *obj.track_registry_, PROJECT->get_plugin_registry (),
                PROJECT->get_port_registry (),
                PROJECT->get_arranger_object_registry (), true);
              obj.tracks_.push_back (id_ref);
            },
            track_var);
        }
    }
}

void
Tracklist::handle_click (TrackUuid track_id, bool ctrl, bool shift, bool dragged)
{
  const auto track_var_opt = get_track (track_id);
  z_return_if_fail (track_var_opt.has_value ());
  auto span = get_track_span ();
  auto selected_tracks =
    std::views::filter (span, TrackSpan::selected_projection)
    | std::ranges::to<std::vector> ();
  bool is_selected = get_selection_manager ().is_selected (track_id);
  if (is_selected)
    {
      if ((ctrl || shift) && !dragged)
        {
          if (get_selection_manager ().size () > 1)
            {
              get_selection_manager ().remove_from_selection (track_id);
            }
        }
      else
        {
          /* do nothing */
        }
    }
  else /* not selected */
    {
      if (shift)
        {
          if (!selected_tracks.empty ())
            {
              TrackSpan selected_tracks_span{ selected_tracks };
              auto      highest_var = selected_tracks_span.get_first_track ();
              auto      lowest_var = selected_tracks_span.get_last_track ();
              std::visit (
                [&] (auto &&highest, auto &&lowest) {
                  const auto track_index = get_track_index (track_id);
                  const auto highest_index =
                    get_track_index (highest->get_uuid ());
                  const auto lowest_index =
                    get_track_index (lowest->get_uuid ());

                  if (track_index > highest_index)
                    {
                      /* select all tracks in between */
                      auto tracks_to_select = std::span (
                        tracks_.begin () + highest_index,
                        tracks_.begin () + track_index);
                      get_selection_manager ().select_only_these (
                        tracks_to_select
                        | std::views::transform ([&] (const auto &id_ref) {
                            return id_ref.id ();
                          }));
                    }
                  else if (track_index < lowest_index)
                    {
                      /* select all tracks in between */
                      auto tracks_to_select = std::span (
                        tracks_.begin () + track_index,
                        tracks_.begin () + lowest_index);
                      get_selection_manager ().select_only_these (
                        tracks_to_select
                        | std::views::transform ([&] (const auto &id_ref) {
                            return id_ref.id ();
                          }));
                    }
                },
                highest_var, lowest_var);
            }
        }
      else if (ctrl)
        {
          // append to selections
          get_selection_manager ().append_to_selection (track_id);
        }
      else
        {
          // select exclusively
          get_selection_manager ().select_unique (track_id);
        }
    }
}

Tracklist::~Tracklist ()
{
  z_debug ("freeing tracklist...");

  // Disconnect all signals to prevent access during destruction
  QObject::disconnect ();
}

void
from_json (const nlohmann::json &j, Tracklist &t)
{
  j.at (Tracklist::kPinnedTracksCutoffKey).get_to (t.pinned_tracks_cutoff_);
  // TODO
  // j.at (Tracklist::kTracksKey).get_to (t.tracks_);
  j.at (Tracklist::kSelectedTracksKey).get_to (t.selected_tracks_);
}
}
