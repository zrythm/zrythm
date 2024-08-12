// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>
#include <cstring>

#include "gui/backend/event.h"
#include "utils/objects.h"

#include "gtk_wrapper.h"

ZEvent::~ZEvent ()
{
  g_free_and_null (backtrace_);
}
