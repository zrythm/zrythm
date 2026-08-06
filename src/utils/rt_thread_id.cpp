// SPDX-FileCopyrightText: © 2024, 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/rt_thread_id.h"

// Initialize the atomic counter. ID 0 is reserved to mean "unset": real
// threads always get IDs >= 1, so a default-initialized RTThreadId slot
// (e.g. GraphThread::rt_thread_id_ before the thread stores its real ID)
// can never compare equal to an actual thread's ID.
std::atomic<unsigned int> RTThreadId::next_id (1);

RTThreadId::RTThreadId ()
    : id (next_id.fetch_add (1, std::memory_order_relaxed))
{
}

unsigned int
RTThreadId::get () const
{
  return id;
}

bool
RTThreadId::operator== (const RTThreadId &other) const
{
  return id == other.id;
}
bool
RTThreadId::operator!= (const RTThreadId &other) const
{
  return id != other.id;
}

thread_local RTThreadId current_thread_id;
