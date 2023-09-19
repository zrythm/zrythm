// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "dsp/track_processor.h"
#  include "project.h"
#endif

SCM_DEFINE (
  s_track_processor_get_stereo_in,
  "track-processor-get-stereo-in",
  1,
  0,
  0,
  (SCM track_processor),
  "Returns the stereo in ports instance of the track processor.")
#define FUNC_NAME s_
{
  TrackProcessor * tp = (TrackProcessor *) scm_to_pointer (track_processor);

  return scm_from_pointer (tp->stereo_in, NULL);
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "audio_track_processor.x"
#endif

  scm_c_export ("track-processor-get-stereo-in", NULL);
}

void
guile_audio_track_processor_define_module (void)
{
  scm_c_define_module ("audio track-processor", init_module, NULL);
}
