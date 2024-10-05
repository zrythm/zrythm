// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "common/utils/midi.h"

#include "tests/helpers/zrythm_helper.h"

TEST (Midi, MsbLsbConversions)
{
  /* first byte */
  midi_byte_t lsb;
  /* second byte */
  midi_byte_t msb;
  midi_get_bytes_from_combined (12280, &lsb, &msb);
  ASSERT_EQ (lsb, 120);
  ASSERT_EQ (msb, 95);

  midi_byte_t buf[] = { MIDI_CH1_PITCH_WHEEL_RANGE, 120, 95 };
  ASSERT_EQ (midi_get_14_bit_value (buf), 12280);
}