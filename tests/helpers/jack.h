// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifdef HAVE_JACK

#include <jack/jack.h>

static bool
test_jack_port_exists (
  const char * port_name);

static bool
test_jack_port_exists (
  const char * port_name)
{
  /* get all input ports */
  const char ** ports = jack_get_ports (
    AUDIO_ENGINE->client, port_name, NULL, 0);

  bool ret = ports && ports[0] != NULL;

  if (ports)
    {
      int i = 0;
      while (ports[i] != NULL)
        {
          //jack_free (ports[i]);
          i++;
        }
    }

  return ret;
}

#define assert_jack_port_exists(name) \
  g_assert_true (test_jack_port_exists (name))

#endif // HAVE_JACK
