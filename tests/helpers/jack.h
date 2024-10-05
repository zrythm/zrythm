// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"
#include "zrythm-test-config.h"

#include <string>

#include "common/dsp/engine.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"

#if HAVE_JACK

#  include <jack/jack.h>

static bool
test_jack_port_exists (const std::string &port_name);

static bool
test_jack_port_exists (const std::string &port_name)
{
  /* get all input ports */
  const char ** ports =
    jack_get_ports (AUDIO_ENGINE->client_, port_name.c_str (), nullptr, 0);

  bool ret = ports && ports[0] != NULL;

  if (ports)
    {
      int i = 0;
      while (ports[i] != nullptr)
        {
          // jack_free (ports[i]);
          i++;
        }
    }

  return ret;
}

#  define assert_jack_port_exists(name) \
    ASSERT_TRUE (test_jack_port_exists (name))

#endif // HAVE_JACK
