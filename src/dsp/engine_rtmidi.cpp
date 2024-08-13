// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * SPDX-FileCopyrightText: © 2018-2020 Alexandros Theodotou <alex@zrythm.org>
 */

#include "zrythm-config.h"

#ifdef HAVE_RTMIDI

#  include "dsp/channel.h"
#  include "dsp/engine.h"
#  include "dsp/engine_rtmidi.h"
#  include "dsp/ext_port.h"
#  include "dsp/port.h"
#  include "dsp/router.h"
#  include "dsp/rtmidi_device.h"
#  include "dsp/transport.h"
#  include "gui/widgets/main_window.h"
#  include "plugins/plugin.h"
#  include "project.h"
#  include "settings/settings.h"
#  include "utils/midi.h"
#  include "utils/ui.h"

#  include <glib/gi18n.h>
#  include <gtk/gtk.h>

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

/**
 * Tests if the backend is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_rtmidi_test (GtkWindow * win)
{
  return 0;
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
