// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/clip.h"
#include "structure/tracks/automation_track.h"

#include <magic_enum.hpp>

namespace zrythm::structure::tracks
{
AutomationTrack::AutomationTrack (
  const dsp::TempoMapWrapper          &tempo_map,
  utils::IObjectRegistry              &registry,
  dsp::ProcessorParameterUuidReference param_id,
  dsp::TimebaseProvider *              timebase_provider,
  QObject *                            parent)
    : QObject (parent),
      arrangement::ArrangerObjectOwner<
        arrangement::AutomationClip> (registry, *this),
      tempo_map_ (tempo_map), registry_ (registry),
      param_id_ (std::move (param_id)), timebase_provider_ (timebase_provider),
      automation_data_provider_ (
        utils::make_qobject_unique<arrangement::AutomationTimelineDataProvider> (
          this)),
      automation_cache_request_debouncer_ (
        utils::make_qobject_unique<utils::PlaybackCacheScheduler> (this))
{
  parameter ()->set_automation_provider ([this] (auto sample_position) {
    return automation_mode_.load () == AutomationMode::Read
             ? automation_data_provider_->get_automation_value_rt (sample_position)
             : std::nullopt;
  });

  QObject::connect (
    get_model (), &arrangement::ArrangerObjectListModel::rowsInserted, this,
    [this] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          auto * clip_obj =
            qobject_cast<arrangement::Clip *> (get_model ()->object_at (i));
          if (clip_obj != nullptr)
            {
              if (auto * tp = clip_obj->timebaseProvider ())
                tp->setSource (timebase_provider_);
            }
        }
    });

  // Connect signals for children clip changes
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

  // cache activity tracking
  const auto &tm = tempo_map_.get_tempo_map ();
  playback_cache_activity_tracker_ = utils::make_qobject_unique<
    PlaybackCacheActivityTracker> (
    automation_cache_request_debouncer_.get (),
    *std::as_const (*automation_data_provider_).get_base_cache (),
    [&tm] (units::sample_t sample) {
      return tm.samples_to_tick (sample).asDouble ();
    },
    this);
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

Q_INVOKABLE void
AutomationTrack::swapRecordMode ()
{
  record_mode_ = static_cast<AutomationRecordMode> (
    (std::to_underlying (record_mode_) + 1)
    % magic_enum::enum_count<AutomationRecordMode> ());
  Q_EMIT recordModeChanged (record_mode_);
}

void
AutomationTrack::regeneratePlaybackCaches (
  utils::ExpandableTickRange affectedRange)
{
  auto children = get_children_view ();
  automation_data_provider_->generate_automation_events (
    tempo_map_.get_tempo_map (), children, affectedRange);

  playback_cache_activity_tracker_->onRegenerationComplete (affectedRange);
}

// ========================================================================

auto
AutomationTrack::get_clip_before (
  units::sample_t pos_samples,
  bool search_only_clips_enclosing_position) const -> AutomationClip *
{
  auto process_clips = [=] (const auto &clips) {
    if (search_only_clips_enclosing_position)
      {
        for (const auto &clip : std::views::reverse (clips))
          {
            if (clip->is_hit (pos_samples))
              return clip;
          }
      }
    else
      {
        AutomationClip * latest_r{};
        auto             latest_distance =
          units::samples (std::numeric_limits<int64_t>::min ());
        for (const auto &clip : std::views::reverse (clips))
          {
            auto distance_from_r_end =
              clip->get_end_position_samples (true) - pos_samples;
            if (
              clip->get_tempo_map ().tick_to_samples_rounded (
                clip->position ()->asTick ())
                <= pos_samples
              && distance_from_r_end > latest_distance)
              {
                latest_distance = distance_from_r_end;
                latest_r = clip;
              }
          }
        return latest_r;
      }
    return static_cast<AutomationClip *> (nullptr);
  };

  return process_clips (get_children_view ());
}

structure::arrangement::AutomationPoint *
AutomationTrack::get_automation_point_around (
  const units::precise_tick_t position_ticks,
  units::precise_tick_t       delta_ticks,
  bool                        search_only_backwards)
{
  const auto &tempo_map = tempo_map_;
  auto        pos_frames = tempo_map.get_tempo_map ().tick_to_samples_rounded (
    dsp::TimelineTick{ position_ticks });
  AutomationPoint * ap = get_automation_point_before (pos_frames, true);
  if (
    (ap != nullptr)
    && position_ticks - arrangement::timeline_ticks (*ap).asQuantity ()
         <= delta_ticks)
    {
      return ap;
    }

  if (search_only_backwards)
    {
      return nullptr;
    }

  pos_frames = tempo_map.get_tempo_map ().tick_to_samples_rounded (
    dsp::TimelineTick{ position_ticks + delta_ticks });
  ap = get_automation_point_before (pos_frames, true);
  if (ap != nullptr)
    {
      const auto diff =
        arrangement::timeline_ticks (*ap).asQuantity () - position_ticks;
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
  auto * r = get_clip_before (timeline_position, search_only_backwards);

  if ((r == nullptr) || r->mute ()->muted ())
    {
      return nullptr;
    }

  const auto clip_end_frames = r->get_end_position_samples (true);

  /* if clip ends before pos, assume pos is the clip's end pos */
  auto local_pos = timeline_frames_to_local (
    *r,
    !search_only_backwards && (clip_end_frames < timeline_position)
      ? clip_end_frames - units::samples (1)
      : timeline_position,
    true);

  const auto &ap_tempo_map = r->get_tempo_map ();
  const auto  ap_clip_start =
    ap_tempo_map.tick_to_samples_rounded (r->position ()->asTick ());
  for (auto * ap : std::ranges::reverse_view (r->get_children_view ()))
    {
      if (
        r->contentWarp ()->contentToTimelineSamples (ap->position ()->asTick ())
          - ap_clip_start
        <= local_pos)
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
  bool            search_only_clips_enclosing_position) const
{
  auto ap = get_automation_point_before (
    timeline_frames, search_only_clips_enclosing_position);

  /* no automation points yet, return negative (no change) */
  if (ap == nullptr)
    {
      return std::nullopt;
    }

  auto clip =
    get_clip_before (timeline_frames, search_only_clips_enclosing_position);
  z_return_val_if_fail (clip, 0.f);

  /* if clip ends before pos, assume pos is the clip's end pos */
  const auto clip_end_position = clip->get_end_position_samples (true);
  auto       localp = timeline_frames_to_local (
    *clip,
    !search_only_clips_enclosing_position && (clip_end_position < timeline_frames)
      ? clip_end_position - units::samples (1)
      : timeline_frames,
    true);

  auto next_ap = clip->get_next_ap (*ap, false);

  /* return value at last ap */
  if (next_ap == nullptr)
    {
      return ap->value ();
    }

  bool  prev_ap_lower = ap->value () <= next_ap->value ();
  float cur_next_diff = std::abs (ap->value () - next_ap->value ());

  /* ratio of how far in we are in the curve */
  const auto &val_tempo_map = clip->get_tempo_map ();
  const auto  val_clip_start =
    val_tempo_map.tick_to_samples_rounded (clip->position ()->asTick ());
  auto ap_frames =
    clip->contentWarp ()->contentToTimelineSamples (ap->position ()->asTick ())
    - val_clip_start;
  auto next_ap_frames =
    clip->contentWarp ()->contentToTimelineSamples (
      next_ap->position ()->asTick ())
    - val_clip_start;
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
    static_cast<float> (clip->get_normalized_value_in_curve (*ap, ratio));
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
  j[AutomationTrack::kParameterKey] = track.param_id_;
  j[AutomationTrack::kAutomationModeKey] = track.automation_mode_.load ();
  j[AutomationTrack::kRecordModeKey] = track.record_mode_;
}

void
from_json (const nlohmann::json &j, AutomationTrack &track)
{
  from_json (j, static_cast<AutomationTrack::ArrangerObjectOwner &> (track));
  j.at (AutomationTrack::kParameterKey).get_to (track.param_id_);
  AutomationTrack::AutomationMode automation_mode{};
  j.at (AutomationTrack::kAutomationModeKey).get_to (automation_mode);
  track.automation_mode_.store (automation_mode);
  j.at (AutomationTrack::kRecordModeKey).get_to (track.record_mode_);
}
}
