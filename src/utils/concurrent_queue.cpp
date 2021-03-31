/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Simplified BSD License:
 *
 * Copyright (c) 2013-2016, Cameron Desrochers. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "zrythm-config.h"

#include "utils/concurrent_queue.h"

#ifdef CONCURRENT_QUEUE_SUBPROJECT
#include <concurrentqueue.h>
#else
#include <concurrentqueue/concurrentqueue.h>
#endif

struct traits : public moodycamel::ConcurrentQueueDefaultTraits
{
	// Use a slightly larger default block size; the default offers
	// a good trade off between speed and memory usage, but a bigger
	// block size will improve throughput (which is mostly what
	// we're after with these benchmarks).
	static const size_t BLOCK_SIZE = 128;
};

typedef moodycamel::ConcurrentQueue<void*, traits> MoodycamelCQType, *MoodycamelCQPtr;

extern "C" {

static size_t
power_of_two_size (
  size_t sz)
{
  int32_t power_of_two;
  for (power_of_two = 1;
       1U << power_of_two < sz; ++power_of_two);
  return 1U << power_of_two;
}

bool
concurrent_queue_create (
  void ** handle,
  size_t  num_elements)
{
  num_elements = power_of_two_size (num_elements);
  MoodycamelCQPtr retval =
    new MoodycamelCQType (num_elements);
  if (retval == nullptr) {
    return false;
  }
  *handle = retval;
  return true;
}

bool
concurrent_queue_destroy (
  void * handle)
{
  delete reinterpret_cast<MoodycamelCQPtr>(handle);
  return true;
}

bool
concurrent_queue_enqueue (
  void * handle,
  void * value)
{
  return reinterpret_cast<MoodycamelCQPtr>(handle)->enqueue(value);
}

bool
concurrent_queue_try_enqueue (
  void * handle,
  void * value)
{
  return reinterpret_cast<MoodycamelCQPtr>(handle)->try_enqueue(value);
}

bool
concurrent_queue_try_dequeue (
  void * handle,
  void ** value)
{
  return reinterpret_cast<MoodycamelCQPtr>(handle)->try_dequeue(*value);
}

} /* extern "C" */
