// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "dsp/engine.h"
#  include "dsp/position.h"
#  include "dsp/transport.h"
#  include "project.h"
#endif

SCM_DEFINE (
  make_position,
  "position-new",
  5,
  0,
  0,
  (SCM bars, SCM beats, SCM sixteenths, SCM ticks, SCM sub_tick),
  "Return a newly-created position as @var{bars}.\
@var{beats}.@var{sixteenths}.@var{ticks}.\
@var{sub_tick}.")
#define FUNC_NAME s_
{
  /* Allocate the Position.  Because we
     use scm_gc_malloc, this memory block will
     be automatically reclaimed when it becomes
     inaccessible, and its members will be traced
     by the garbage collector.  */
  Position * pos = (Position *) scm_gc_malloc (
    sizeof (Position), "position");

  position_init (pos);
  position_add_bars (pos, scm_to_int (bars) - 1);
  position_add_beats (pos, scm_to_int (beats) - 1);
  position_add_sixteenths (pos, scm_to_int (sixteenths) - 1);
  position_add_ticks (pos, (double) scm_to_int (ticks));
  position_add_ticks (pos, scm_to_double (sub_tick));
  position_update_frames_from_ticks (pos, 0.0);

  /* wrap the Position * in a new foreign object
   * and return that object */
  return scm_from_pointer (pos, NULL);
}
#undef FUNC_NAME

SCM_DEFINE (
  print_position,
  "position-print",
  1,
  0,
  0,
  (SCM pos),
  "Prints the position as `bars.beats.sixteenths.\
ticks.sub_tick`.")
#define FUNC_NAME s_
{
  Position * refpos = scm_to_pointer (pos);

  char buf[120];
  position_to_string (refpos, buf);

  SCM out_port = scm_current_output_port ();
  scm_display (scm_from_utf8_string (buf), out_port);

  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "audio_position.x"
#endif

  scm_c_export ("position-new", "position-print", NULL);
}

void
guile_audio_position_define_module (void)
{
  scm_c_define_module ("audio position", init_module, NULL);
}
