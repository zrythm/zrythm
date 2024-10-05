// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "common/dsp/master_track.h"
#include "common/dsp/track_processor.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

/* Run TrackProcessor::process() on master and check that the output buffer
 * matches the input buffer */
TEST_F (ZrythmFixture, ProcessMaster)
{
  /* stop dummy audio engine processing so we can process manually */
  test_project_stop_dummy_engine ();

  for (nframes_t i = 0; i < AUDIO_ENGINE->block_length_; i++)
    {
      P_MASTER_TRACK->processor_->stereo_in_->get_l ().buf_[i] = (float) (i + 1);
    }

  nframes_t             local_offset = 60;
  EngineProcessTimeInfo time_nfo = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = local_offset,
  };
  P_MASTER_TRACK->processor_->process (time_nfo);
  time_nfo.g_start_frame_ = 0;
  time_nfo.g_start_frame_w_offset_ = local_offset;
  time_nfo.local_offset_ = local_offset;
  time_nfo.nframes_ = AUDIO_ENGINE->block_length_ - local_offset;
  P_MASTER_TRACK->processor_->process (time_nfo);

  for (nframes_t i = 0; i < AUDIO_ENGINE->block_length_; i++)
    {
      ASSERT_NEAR (
        P_MASTER_TRACK->processor_->stereo_out_->get_l ().buf_[i],
        (float) (i + 1), 0.000001f);
    }
}