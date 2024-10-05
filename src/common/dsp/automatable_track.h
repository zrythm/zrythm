// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUTOMATABLE_TRACK_H__
#define __AUDIO_AUTOMATABLE_TRACK_H__

#include "common/dsp/track.h"

/**
 * Interface for a track that has automatable parameters.
 */
class AutomatableTrack
    : virtual public Track,
      public ISerializable<AutomatableTrack>
{
public:
  // Rule of 0
  AutomatableTrack ();

  virtual ~AutomatableTrack () = default;

  void init_loaded () override;

  AutomationTracklist &get_automation_tracklist () const
  {
    return *automation_tracklist_;
  }

  /**
   * Set automation visible and fire events.
   */
  void set_automation_visible (bool visible);

  /**
   * Generates automatables for the track.
   *
   * Should be called as soon as the track is created.
   */
  void generate_automation_tracks ();

  void clear_objects () override { automation_tracklist_->clear_objects (); }

  void get_regions_in_range (
    std::vector<Region *> &regions,
    const Position *       p1,
    const Position *       p2) override
  {
    auto &atl = get_automation_tracklist ();
    for (auto &at : atl.ats_)
      {
        for (auto &region : at->regions_)
          {
            add_region_if_in_range (p1, p2, regions, region.get ());
          }
      }
  }

protected:
  void copy_members_from (const AutomatableTrack &other)
  {
    automation_tracklist_ = other.automation_tracklist_->clone_unique ();
    automation_visible_ = other.automation_visible_;
  }

  bool validate_base () const;

  void set_playback_caches () override
  {
    get_automation_tracklist ().set_caches (CacheType::PlaybackSnapshots);
  }

  void update_name_hash (unsigned int new_name_hash) override
  {
    get_automation_tracklist ().update_track_name_hash (*this);
  }

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  std::unique_ptr<AutomationTracklist> automation_tracklist_;

  /** Flag to set automations visible or not. */
  bool automation_visible_ = false;
};

#endif // __AUDIO_AUTOMATABLE_TRACK_H__
