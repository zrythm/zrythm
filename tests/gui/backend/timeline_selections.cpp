// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <random>

#include "gui/cpp/backend/timeline_selections.h"

#include "helpers/project_helper.h"
#include "helpers/zrythm_helper.h"

#include "common/dsp/track.h"
#include "common/utils/gtest_wrapper.h"

TEST_F (BootstrapTimelineFixture, TimelineSelectionsSortByIndices)
{
  TimelineSelections sel;
  Position           start;
  Position           end (static_cast<double> (400));
  const auto &midi_track = TRACKLIST->get_track_by_type (Track::Type::Midi);
  const auto &audio_track = TRACKLIST->get_track_by_type (Track::Type::Audio);
  ASSERT_NONNULL (midi_track);
  ASSERT_NONNULL (audio_track);

  constexpr size_t   num_frames = 1024;
  std::vector<float> dummy_frames (num_frames);
  const MusicalScale scale (MusicalScale::Type::Acoustic, MusicalNote::A);

  std::random_device              rd;
  std::mt19937                    gen (rd ());
  std::uniform_int_distribution<> dis (0, 99); // Adjust range as needed

  for (int i = 0; i < 10; ++i)
    {
      for (int j = 0; j < 10; ++j)
        {
          auto random_i = dis (gen);
          auto random_j = dis (gen);

          auto mr = std::make_unique<MidiRegion> (
            start, end, midi_track->get_name_hash (), random_i, random_j);
          sel.add_object_owned (std::move (mr));

          sel.add_object_owned (std::make_unique<AudioRegion> (
            -1, std::nullopt, false, dummy_frames.data (), num_frames,
            "test clip name", 1, BitDepth::BIT_DEPTH_16, start,
            audio_track->get_name_hash (), random_i, random_j));
          sel.add_object_owned (std::make_unique<AutomationRegion> (
            start, end, audio_track->get_name_hash (), random_i, random_j));
        }
      auto random_i = dis (gen);
      sel.add_object_owned (
        std::make_unique<ChordRegion> (start, end, random_i));
      auto scale_obj = std::make_unique<ScaleObject> (scale);
      scale_obj->index_in_chord_track_ = random_i;
      sel.add_object_owned (std::move (scale_obj));
      auto m = std::make_unique<Marker> ("test");
      m->marker_track_index_ = random_i;
      sel.add_object_owned (std::move (m));
    }

  // shuffle the objects
  std::shuffle (sel.objects_.begin (), sel.objects_.end (), gen);

  z_debug ("before:\n{}", sel);
  sel.sort_by_indices (false);
  z_debug ("after asc:\n{}", sel);
  sel.sort_by_indices (true);
  z_debug ("after desc:\n{}", sel);
}