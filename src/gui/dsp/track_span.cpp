// #include "gui/backend/backend/actions/undo_manager.h"
// #include "gui/backend/backend/project.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/dsp/engine.h"

#include "./track_span.h"

using namespace zrythm;

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
void
TrackSpanImpl<Range>::mark_for_bounce (bool with_parents, bool mark_master)
{
  // FIXME: dependency on audio_engine
  AUDIO_ENGINE->reset_bounce_mode ();

  std::ranges::for_each (*this, [&] (auto &&track_var) {
    std::visit (
      [&] (auto &&tr) {
        if (!with_parents)
          {
            tr->bounce_to_master_ = true;
          }
        tr->mark_for_bounce (true, true, true, with_parents);
      },
      track_var);
  });

  if (mark_master)
    {
      get_master_track ().mark_for_bounce (true, false, false, false);
    }
}

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
void
TrackSpanImpl<Range>::expose_ports_to_backend (AudioEngine &engine)
{
  std::ranges::for_each (*this, [&] (auto &&track_var) {
    std::visit (
      [&] (auto &&tr) {
        using TrackT = base_type<decltype (tr)>;
        if constexpr (std::derived_from<TrackT, ChannelTrack>)
          {
            tr->get_channel ()->expose_ports_to_backend (engine);
          }
      },
      track_var);
  });
}

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
void
TrackSpanImpl<Range>::reconnect_ext_input_ports (AudioEngine &engine)
{
  std::ranges::for_each (*this, [&] (auto &&track_var) {
    std::visit (
      [&] (auto &&tr) {
        using TrackT = base_type<decltype (tr)>;
        if constexpr (std::derived_from<TrackT, ChannelTrack>)
          {
            tr->get_channel ()->reconnect_ext_input_ports (engine);
          }
      },
      track_var);
  });
}

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
bool
TrackSpanImpl<Range>::fix_audio_regions (dsp::FramesPerTick frames_per_tick)
{
  z_debug ("fixing audio region positions...");

  int num_fixed{};
  std::ranges::for_each (*this, [&] (auto &&track_var) {
    if (std::holds_alternative<AudioTrack *> (track_var))
      {
        const auto track = std::get<AudioTrack *> (track_var);
        for (const auto &lane_var : track->lanes_)
          {
            const auto * lane = std::get<AudioLane *> (lane_var);
            for (auto * region : lane->get_children_view ())
              {
                if (region->fix_positions (frames_per_tick))
                  ++num_fixed;
              }
          }
      }
  });

  z_debug ("done fixing {} audio region positions", num_fixed);

  return num_fixed > 0;
}

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
void
TrackSpanImpl<Range>::move_after_copying_or_moving_inside (
  int diff_between_track_below_and_parent)
{
  const auto &lowest_cloned_track = *(std::ranges::max_element (
    *this, [] (const auto &lhs_var, const auto &rhs_var) {
      return std::visit (
        [&] (auto &&lhs, auto &&rhs) {
          return lhs->get_index () < rhs->get_index ();
        },
        lhs_var, rhs_var);
    }));
  const auto  lowest_cloned_track_pos =
    std::visit (position_projection, lowest_cloned_track);

  try
    {
      UNDO_MANAGER->perform (new gui::actions::MoveTracksAction (
        *this, lowest_cloned_track_pos + diff_between_track_below_and_parent));
    }
  catch (const ZrythmException &e)
    {
      e.handle (QObject::tr (
        "Failed to move tracks after copying or moving inside folder"));
      return;
    }

  auto ua_opt = UNDO_MANAGER->get_last_action ();
  z_return_if_fail (ua_opt.has_value ());
  std::visit ([&] (auto &&ua) { ua->num_actions_ = 2; }, ua_opt.value ());
}

// FIXME: just call the code directly where needed?
#if 0
void
TrackSpan::paste_to_pos (int pos)
{
  UNDO_MANAGER->perform (
    new gui::actions::CopyTracksAction (*this, *PORT_CONNECTIONS_MGR, pos));
}
#endif

template class TrackSpanImpl<std::span<const TrackPtrVariant>>;
template class TrackSpanImpl<utils::UuidIdentifiableObjectSpan<TrackRegistry>>;
template class TrackSpanImpl<
  utils::UuidIdentifiableObjectSpan<TrackRegistry, TrackUuidReference>>;
