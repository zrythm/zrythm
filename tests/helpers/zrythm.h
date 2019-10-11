/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Zrythm test helper.
 */

#ifndef __TEST_HELPERS_ZRYTHM_H__
#define __TEST_HELPERS_ZRYTHM_H__

#include "audio/engine_dummy.h"
#include "project.h"
#include "zrythm.h"

/**
 * @addtogroup tests
 *
 * @{
 */

/**
 * To be called by every test's main to initialize
 * Zrythm to default values.
 */
static void
test_helper_zrythm_init ()
{
  ZRYTHM = calloc (1, sizeof (Zrythm));
  ZRYTHM->testing = 1;
  PROJECT = calloc (1, sizeof (Project));
  AUDIO_ENGINE->block_length = 256;

  transport_init (TRANSPORT, 0);
  engine_dummy_setup (AUDIO_ENGINE, 0);
}

/**
 * @}
 */

#endif
