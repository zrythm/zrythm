// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * SPDX-FileCopyrightText: © 2018-2020 Alexandros Theodotou <alex@zrythm.org>
 */

#include "zrythm-config.h"

#if HAVE_RTMIDI

#  include "gui/backend/backend/project.h"
#  include "gui/backend/backend/settings/settings.h"
#  include "gui/backend/channel.h"
#  include "gui/backend/ui.h"
#  include "gui/dsp/engine.h"
#  include "gui/dsp/engine_rtmidi.h"
#  include "gui/dsp/ext_port.h"
#  include "gui/dsp/plugin.h"
#  include "gui/dsp/port.h"
#  include "gui/dsp/router.h"
#  include "gui/dsp/rtmidi_device.h"
#  include "gui/dsp/transport.h"
#  include "utils/midi.h"

#  include <rtmidi_c.h>

/**
 * Sets up the MIDI engine to use Rtmidi.
 *
 * @param loading Loading a Project or not.
 */
int
engine_rtmidi_setup (AudioEngine * self)
{
  self->midi_buf_size_ = 4096;

  z_info ("Rtmidi set up");

  return 0;
}

/**
 * Gets the number of input ports (devices).
 */
unsigned int
engine_rtmidi_get_num_in_ports (AudioEngine * self)
{
  try
    {
      RtMidiDevice dev (true, 0, nullptr);
      return rtmidi_get_port_count (dev.in_handle_);
    }
  catch (const ZrythmException &e)
    {
      z_warning ("{}", e.what ());
      return 0;
    }
}

void
engine_rtmidi_tear_down (AudioEngine * self)
{
  /* init semaphore */
  // zix_sem_init (&self->port_operation_lock, 1);
}

int
engine_rtmidi_activate (AudioEngine * self, bool activate)
{
  return 0;
}

#endif /* HAVE_RTMIDI */
