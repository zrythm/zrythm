// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * SPDX-FileCopyrightText: © 2018-2020 Alexandros Theodotou <alex@zrythm.org>
 */

#include "zrythm-config.h"

#if HAVE_RTMIDI

#  include "common/dsp/channel.h"
#  include "common/dsp/engine.h"
#  include "common/dsp/engine_rtmidi.h"
#  include "common/dsp/ext_port.h"
#  include "common/dsp/port.h"
#  include "common/dsp/router.h"
#  include "common/dsp/rtmidi_device.h"
#  include "common/dsp/transport.h"
#  include "common/plugins/plugin.h"
#  include "common/utils/midi.h"
#  include "common/utils/ui.h"
#  include "gui/cpp/backend/project.h"
#  include "gui/cpp/backend/settings/settings.h"
#  include "gui/widgets/main_window.h"

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
