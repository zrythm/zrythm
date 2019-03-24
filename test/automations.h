/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __TEST_AUTOMATIONS_H__
#define __TEST_AUTOMATIONS_H__

#include <glib.h>

typedef struct Channel Channel;
typedef struct Automatable Automatable;
typedef struct AutomationTrack AutomationTrack;

typedef struct
{
  Channel *         ch;
  Automatable *     a;
  AutomationTrack * at;
} AutomationTrackFixture;

void
at_fixture_set_up (
  AutomationTrackFixture * fixture,
  gconstpointer user_data);

void
at_fixture_tear_down (
  AutomationTrackFixture * fixture,
  gconstpointer user_data);

/**
 * Tests automation point and curve position-related
 * functions
 * (automation_track_get_ap_before_pos, etc.).
 */
void
test_at_get_x_relevant_to_pos (
  AutomationTrackFixture * fixture,
  gconstpointer user_data);

#endif
