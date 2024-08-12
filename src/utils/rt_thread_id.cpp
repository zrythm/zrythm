#include "utils/rt_thread_id.h"

// Initialize the atomic counter
std::atomic<unsigned int> RTThreadId::next_id (0);

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

// Define the thread-local instance
thread_local RTThreadId current_thread_id;