/*
  Copyright 2011 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include <sys/time.h>

static inline double
elapsed_s(const struct timespec* start, const struct timespec* end)
{
	return ( (end->tv_sec - start->tv_sec)
	         + ((end->tv_nsec - start->tv_nsec) * 0.000000001) );
}

static inline struct timespec
bench_start(void)
{
	struct timespec start_t;
	clock_gettime(CLOCK_MONOTONIC, &start_t);
	return start_t;
}

static inline double
bench_end(const struct timespec* start_t)
{
	struct timespec end_t;
	clock_gettime(CLOCK_MONOTONIC, &end_t);
	return elapsed_s(start_t, &end_t);
}

