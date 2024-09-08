// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Project helper.
 */

#ifndef __TEST_HELPERS_PROJECT_H__
#define __TEST_HELPERS_PROJECT_H__

#include "zrythm-test-config.h"

#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "dsp/engine_dummy.h"
#include "dsp/marker_track.h"
#include "dsp/master_track.h"
#include "dsp/midi_note.h"
#include "dsp/recording_manager.h"
#include "dsp/region.h"
#include "dsp/router.h"
#include "dsp/tempo_track.h"
#include "dsp/tracklist.h"
#include "project.h"
#include "project/project_init_flow_manager.h"
#include "utils/gtest_wrapper.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <glib.h>
#include <glib/gi18n.h>

#include "tests/helpers/zrythm_helper.h"

/**
 * @addtogroup tests
 *
 * @{
 */

/** MidiNote value to use. */
constexpr int MN_VAL = 78;
/** MidiNote velocity to use. */
constexpr int MN_VEL = 23;

/** First AP value. */
constexpr float AP_VAL1 = 0.6f;
/** Second AP value. */
constexpr float AP_VAL2 = 0.9f;

/** Marker name. */
constexpr const char * MARKER_NAME = "Marker name";

constexpr auto MUSICAL_SCALE_TYPE = MusicalScale::Type::Ionian;
constexpr auto MUSICAL_SCALE_ROOT = MusicalNote::A;

constexpr int MOVE_TICKS = 400;

constexpr int TOTAL_TL_SELECTIONS = 6;

constexpr const char * MIDI_REGION_NAME = "Midi region";
constexpr const char * AUDIO_REGION_NAME = "Audio region";
constexpr const char * MIDI_TRACK_NAME = "Midi track";
constexpr const char * AUDIO_TRACK_NAME = "Audio track";

/* initial positions */
constexpr int MIDI_REGION_LANE = 2;
constexpr int AUDIO_REGION_LANE = 3;

/* target positions */
constexpr const char * TARGET_MIDI_TRACK_NAME = "Target midi tr";
constexpr const char * TARGET_AUDIO_TRACK_NAME = "Target audio tr";

/* TODO test moving lanes */
constexpr int TARGET_MIDI_REGION_LANE = 0;
constexpr int TARGET_AUDIO_REGION_LANE = 5;

fs::path
test_project_save ();

COLD void
test_project_reload (const fs::path &prj_file);

void
test_project_save_and_reload ();

/**
 * Stop dummy audio engine processing so we can
 * process manually.
 */
void
test_project_stop_dummy_engine ();

void
test_project_check_vs_original_state (
  Position * p1,
  Position * p2,
  int        check_selections);

fs::path
test_project_save ();

void
test_project_reload (const fs::path &prj_file);

void
test_project_save_and_reload ();

/**
 * Bootstraps the test with test data.
 */
class BootstrapTimelineFixture : public ZrythmFixture
{
public:
  void SetUp () override;

  /**
   * Checks that the objects are back to their original state.
   * @param check_selections Also checks that the selections are back to where
   * they were.
   */
  void check_vs_original_state (bool check_selections);

  void TearDown () override;

  virtual ~BootstrapTimelineFixture () = default;

public:
  Position p1_;
  Position p2_;
};

/**
 * Stop dummy audio engine processing so we can process manually.
 */
void
test_project_stop_dummy_engine ();

/**
 * @}
 */

#endif
