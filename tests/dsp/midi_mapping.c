// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "dsp/master_track.h"
#include "dsp/midi_mapping.h"
#include "project.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm.h"

#include "helpers/project.h"
#include "helpers/zrythm.h"

static void
test_midi_mappping (void)
{
  MidiMappings * mappings = midi_mappings_new ();
  g_assert_nonnull (mappings);
  free (mappings);

  ExtPort * ext_port = object_new (ExtPort);
  ext_port->type = EXT_PORT_TYPE_RTAUDIO;
  ext_port->full_name = g_strdup ("ext port1");
  ext_port->short_name = g_strdup ("extport1");

  midi_byte_t buf[3] = { 0xB0, 0x07, 121 };
  midi_mappings_bind_device (
    MIDI_MAPPINGS, buf, ext_port,
    P_MASTER_TRACK->channel->fader->amp, F_NO_PUBLISH_EVENTS);
  g_assert_cmpint (MIDI_MAPPINGS->num_mappings, ==, 1);

  int size = midi_mappings_get_for_port (
    MIDI_MAPPINGS, P_MASTER_TRACK->channel->fader->amp, NULL);
  g_assert_cmpint (size, ==, 1);

  midi_mappings_apply (MIDI_MAPPINGS, buf);

  test_project_save_and_reload ();

  g_assert_true (
    P_MASTER_TRACK->channel->fader->amp
    == MIDI_MAPPINGS->mappings[0]->dest);
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/audio/midi_mapping/"

  g_test_add_func (
    TEST_PREFIX "test midi mapping",
    (GTestFunc) test_midi_mappping);

  return g_test_run ();
}
