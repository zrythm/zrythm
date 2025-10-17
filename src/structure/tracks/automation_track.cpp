// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/automation_track.h"

namespace zrythm::structure::tracks
{
AutomationTrack::AutomationTrack (
  const dsp::TempoMapWrapper          &tempo_map,
  dsp::FileAudioSourceRegistry        &file_audio_source_registry,
  ArrangerObjectRegistry              &obj_registry,
  dsp::ProcessorParameterUuidReference param_id,
  QObject *                            parent)
    : QObject (parent),
      arrangement::ArrangerObjectOwner<arrangement::AutomationRegion> (
        obj_registry,
        file_audio_source_registry,
        *this),
      tempo_map_ (tempo_map), object_registry_ (obj_registry),
      param_id_ (std::move (param_id)),
      automation_cache_request_debouncer_ (
        utils::make_qobject_unique<utils::PlaybackCacheScheduler> ())
{
  parameter ()->set_automation_provider ([this] (auto sample_position) {
    return automation_mode_.load () == AutomationMode::Read
             ? automation_data_provider_.get_automation_value_rt (sample_position)
             : std::nullopt;
  });

  // Connect signals for children region changes
  QObject::connect (
    get_model (), &arrangement::ArrangerObjectListModel::contentChanged, this,
    &AutomationTrack::automationObjectsNeedRecache, Qt::QueuedConnection);

  // Connect signals for sending cache requests
  QObject::connect (
    &tempo_map_, &dsp::TempoMapWrapper::tempoEventsChanged,
    automation_cache_request_debouncer_.get (),
    [this] () { automation_cache_request_debouncer_->queueCacheRequest ({}); });
  QObject::connect (
    this, &AutomationTrack::automationObjectsNeedRecache,
    automation_cache_request_debouncer_.get (),
    &utils::PlaybackCacheScheduler::queueCacheRequest);

  // Connect signal for handling cache requests
  QObject::connect (
    automation_cache_request_debouncer_.get (),
    &utils::PlaybackCacheScheduler::cacheRequested, this,
    &AutomationTrack::regeneratePlaybackCaches);
}

// ========================================================================
// QML Interface
// ========================================================================

void
AutomationTrack::setAutomationMode (AutomationMode automation_mode)
{
  if (automation_mode == automation_mode_)
    return;

  automation_mode_.store (automation_mode);
  Q_EMIT automationModeChanged (automation_mode);
}

void
AutomationTrack::setRecordMode (AutomationRecordMode record_mode)
{
  if (record_mode == record_mode_)
    return;

  record_mode_ = record_mode;
  Q_EMIT recordModeChanged (record_mode);
}

void
AutomationTrack::regeneratePlaybackCaches (
  utils::ExpandableTickRange affectedRange)
{
  auto children = get_children_view ();
  automation_data_provider_.generate_automation_events (
    tempo_map_.get_tempo_map (), children, affectedRange);
}

// ========================================================================

auto
AutomationTrack::get_region_before (
  units::sample_t pos_samples,
  bool search_only_regions_enclosing_position) const -> AutomationRegion *
{
  auto process_regions = [=] (const auto &regions) {
    if (search_only_regions_enclosing_position)
      {
        for (const auto &region : std::views::reverse (regions))
          {
            if (get_object_bounds (*region)->is_hit (pos_samples))
              return region;
          }
      }
    else
      {
        AutomationRegion * latest_r{};
        auto               latest_distance =
          units::samples (std::numeric_limits<signed_frame_t>::min ());
        for (const auto &region : std::views::reverse (regions))
          {
            auto distance_from_r_end =
              get_object_bounds (*region)->get_end_position_samples (true)
              - pos_samples;
            if (
              units::samples (region->position ()->samples ()) <= pos_samples
              && distance_from_r_end > latest_distance)
              {
                latest_distance = distance_from_r_end;
                latest_r = region;
              }
          }
        return latest_r;
      }
    return static_cast<AutomationRegion *> (nullptr);
  };

  return process_regions (get_children_view ());
}

structure::arrangement::AutomationPoint *
AutomationTrack::get_automation_point_around (
  const units::precise_tick_t position_ticks,
  units::precise_tick_t       delta_ticks,
  bool                        search_only_backwards)
{
  const auto &tempo_map = tempo_map_;
  auto        pos_frames =
    tempo_map.get_tempo_map ().tick_to_samples_rounded (position_ticks);
  AutomationPoint * ap = get_automation_point_before (pos_frames, true);
  if (
    (ap != nullptr)
    && position_ticks - units::ticks (ap->position ()->ticks ()) <= delta_ticks)
    {
      return ap;
    }

  if (search_only_backwards)
    {
      return nullptr;
    }

  pos_frames = tempo_map.get_tempo_map ().tick_to_samples_rounded (
    position_ticks + delta_ticks);
  ap = get_automation_point_before (pos_frames, true);
  if (ap != nullptr)
    {
      const auto diff =
        units::ticks (ap->position ()->ticks ()) - position_ticks;
      if (diff >= units::ticks (0.0))
        return ap;
    }

  return nullptr;
}

auto
AutomationTrack::get_automation_point_before (
  units::sample_t timeline_position,
  bool            search_only_backwards) const -> AutomationPoint *
{
  auto * r = get_region_before (timeline_position, search_only_backwards);

  if ((r == nullptr) || r->mute ()->muted ())
    {
      return nullptr;
    }

  const auto region_end_frames = r->bounds ()->get_end_position_samples (true);

  /* if region ends before pos, assume pos is the region's end pos */
  auto local_pos = timeline_frames_to_local (
    *r,
    !search_only_backwards && (region_end_frames < timeline_position)
      ? region_end_frames - units::samples (1)
      : timeline_position,
    true);

  for (auto * ap : std::ranges::reverse_view (r->get_children_view ()))
    {
      if (units::samples (ap->position ()->samples ()) <= local_pos)
        {
          return ap;
        }
    }

  return nullptr;
}

// TODO: add a listener of automationModeChanged to whoever needs to know this
#if 0
void
AutomationTrack::set_automation_mode (AutomationMode mode)
{
  z_return_if_fail (ZRYTHM_IS_QT_THREAD);

  auto atl = get_automation_tracklist ();
  z_return_if_fail (atl);

  /* add to atl cache if recording */
  if (mode == AutomationMode::Record)
    {
      auto ats_in_record_mode = atl->get_automation_tracks_in_record_mode ();
      if (
        std::ranges::find (
          ats_in_record_mode, this, &QPointer<AutomationTrack>::get)
        == ats_in_record_mode.end ())
        {
          ats_in_record_mode.emplace_back (this);
        }
    }

  automation_mode_ = mode;
}
#endif

bool
AutomationTrack::should_read_automation () const
{
  if (automation_mode_ == AutomationMode::Off)
    return false;

  /* TODO last argument should be true but doesnt work properly atm */
  if (should_be_recording (false))
    return false;

  return true;
}

bool
AutomationTrack::should_be_recording (bool record_aps) const
{
  if (automation_mode_ != AutomationMode::Record) [[likely]]
    return false;

  if (record_mode_ == AutomationRecordMode::Latch)
    {
      /* in latch mode, we are always recording, even if the value doesn't
       * change (an automation point will be created as soon as latch mode is
       * armed) and then only when changes are made) */
      return true;
    }

    // TODO
#if 0
  if (record_mode_ == AutomationRecordMode::Touch)
    {

      const auto &port = get_port ();
      const auto  diff = port.ms_since_last_change ();
      if (
        diff
        < static_cast<RtTimePoint> (AUTOMATION_RECORDING_TOUCH_REL_MS) * 1000)
        {
          /* still recording */
          return true;
        }

      if (!record_aps)
        return recording_started_;
    }
#endif

  return false;
}

std::optional<float>
AutomationTrack::get_normalized_value (
  units::sample_t timeline_frames,
  bool            search_only_regions_enclosing_position) const
{
  auto ap = get_automation_point_before (
    timeline_frames, search_only_regions_enclosing_position);

  /* no automation points yet, return negative (no change) */
  if (ap == nullptr)
    {
      return std::nullopt;
    }

  auto region =
    get_region_before (timeline_frames, search_only_regions_enclosing_position);
  z_return_val_if_fail (region, 0.f);

  /* if region ends before pos, assume pos is the region's end pos */
  const auto region_end_position =
    region->bounds ()->get_end_position_samples (true);
  auto localp = timeline_frames_to_local (
    *region,
    !search_only_regions_enclosing_position
        && (region_end_position < timeline_frames)
      ? region_end_position - units::samples (1)
      : timeline_frames,
    true);

  auto next_ap = region->get_next_ap (*ap, false);

  /* return value at last ap */
  if (next_ap == nullptr)
    {
      return ap->value ();
    }

  bool  prev_ap_lower = ap->value () <= next_ap->value ();
  float cur_next_diff = std::abs (ap->value () - next_ap->value ());

  /* ratio of how far in we are in the curve */
  auto   ap_frames = units::samples (ap->position ()->samples ());
  auto   next_ap_frames = units::samples (next_ap->position ()->samples ());
  double ratio = 1.0;
  auto   numerator = localp - ap_frames;
  auto   denominator = next_ap_frames - ap_frames;
  if (numerator == units::samples (0))
    {
      ratio = 0.0;
    }
  else if (denominator == units::samples (0)) [[unlikely]]
    {
      z_warning ("denominator is 0. this should never happen");
      ratio = 1.0;
    }
  else
    {
      ratio =
        numerator.in<double> (units::samples)
        / denominator.in<double> (units::samples);
    }
  z_return_val_if_fail (ratio >= 0, 0.f);

  auto result =
    static_cast<float> (region->get_normalized_value_in_curve (*ap, ratio));
  result = result * cur_next_diff;
  if (prev_ap_lower)
    result += ap->value ();
  else
    result += next_ap->value ();

  return result;
}

void
init_from (
  AutomationTrack       &obj,
  const AutomationTrack &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<AutomationTrack::ArrangerObjectOwner &> (obj),
    static_cast<const AutomationTrack::ArrangerObjectOwner &> (other),
    clone_type);
  obj.automation_mode_.store (other.automation_mode_.load ());
  obj.record_mode_ = other.record_mode_;
  obj.param_id_ = other.param_id_;
}

void
to_json (nlohmann::json &j, const AutomationTrack &track)
{
  to_json (j, static_cast<const AutomationTrack::ArrangerObjectOwner &> (track));
  j[AutomationTrack::kParamIdKey] = track.param_id_;
  j[AutomationTrack::kAutomationModeKey] = track.automation_mode_.load ();
  j[AutomationTrack::kRecordModeKey] = track.record_mode_;
}

void
from_json (const nlohmann::json &j, AutomationTrack &track)
{
  from_json (j, static_cast<AutomationTrack::ArrangerObjectOwner &> (track));
  j.at (AutomationTrack::kParamIdKey).get_to (track.param_id_);
  AutomationTrack::AutomationMode automation_mode{};
  j.at (AutomationTrack::kAutomationModeKey).get_to (automation_mode);
  track.automation_mode_.store (automation_mode);
  j.at (AutomationTrack::kRecordModeKey).get_to (track.record_mode_);
}
}
