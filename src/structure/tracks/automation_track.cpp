// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/automation_track.h"

namespace zrythm::structure::tracks
{
AutomationTrack::AutomationTrack (
  const dsp::TempoMap                 &tempo_map,
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
      param_id_ (std::move (param_id))
{
  parameter ()->set_automation_provider (
    [this] (unsigned_frame_t sample_position) -> std::optional<float> {
      if (get_children_vector ().empty ())
        {
          return std::nullopt;
        }
      return get_normalized_value (
        static_cast<signed_frame_t> (sample_position), false);
    });
}

// ========================================================================
// QML Interface
// ========================================================================

void
AutomationTrack::setAutomationMode (AutomationMode automation_mode)
{
  if (automation_mode == automation_mode_)
    return;

  automation_mode_ = automation_mode;
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

// ========================================================================

structure::arrangement::AutomationPoint *
AutomationTrack::get_automation_point_around (
  const double position_ticks,
  double       delta_ticks,
  bool         search_only_backwards)
{
  const auto &tempo_map = tempo_map_;
  auto        pos_frames = tempo_map.tick_to_samples_rounded (position_ticks);
  AutomationPoint * ap = get_automation_point_before (pos_frames, true);
  if (
    (ap != nullptr) && position_ticks - ap->position ()->ticks () <= delta_ticks)
    {
      return ap;
    }

  if (search_only_backwards)
    {
      return nullptr;
    }

  pos_frames = tempo_map.tick_to_samples_rounded (position_ticks + delta_ticks);
  ap = get_automation_point_before (pos_frames, true);
  if (ap != nullptr)
    {
      double diff = ap->position ()->ticks () - position_ticks;
      if (diff >= 0.0)
        return ap;
    }

  return nullptr;
}

auto
AutomationTrack::get_automation_point_before (
  signed_frame_t timeline_position,
  bool           search_only_backwards) const -> AutomationPoint *
{
  auto * r = get_region_before (timeline_position, search_only_backwards);

  if ((r == nullptr) || r->mute ()->muted ())
    {
      return nullptr;
    }

  const auto region_end_frames = r->bounds ()->get_end_position_samples (true);

  /* if region ends before pos, assume pos is the region's end pos */
  signed_frame_t local_pos = timeline_frames_to_local (
    *r,
    !search_only_backwards && (region_end_frames < timeline_position)
      ? region_end_frames - 1
      : timeline_position,
    true);

  for (auto * ap : std::ranges::reverse_view (r->get_children_view ()))
    {
      if (ap->position ()->samples () <= local_pos)
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
  signed_frame_t timeline_frames,
  bool           search_only_regions_enclosing_position) const
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
  signed_frame_t localp = timeline_frames_to_local (
    *region,
    !search_only_regions_enclosing_position
        && (region_end_position < timeline_frames)
      ? region_end_position - 1
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
  signed_frame_t ap_frames = ap->position ()->samples ();
  signed_frame_t next_ap_frames = next_ap->position ()->samples ();
  double         ratio = 1.0;
  signed_frame_t numerator = localp - ap_frames;
  signed_frame_t denominator = next_ap_frames - ap_frames;
  if (numerator == 0)
    {
      ratio = 0.0;
    }
  else if (denominator == 0) [[unlikely]]
    {
      z_warning ("denominator is 0. this should never happen");
      ratio = 1.0;
    }
  else
    {
      ratio = (double) numerator / (double) denominator;
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
  obj.automation_mode_ = other.automation_mode_;
  obj.record_mode_ = other.record_mode_;
  obj.param_id_ = other.param_id_;
}
}
