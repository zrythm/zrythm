/*
  Copyright 2012 David Robillard <http://drobilla.net>

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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zix/sem.h"
#include "zix/thread.h"

ZixSem   sem;
unsigned n_signals = 1024;

static void*
reader(void* arg)
{
	printf("Reader starting\n");

	for (unsigned i = 0; i < n_signals; ++i) {
		zix_sem_wait(&sem);
	}

	printf("Reader finished\n");
	return NULL;
}

static void*
writer(void* arg)
{
	printf("Writer starting\n");

	for (unsigned i = 0; i < n_signals; ++i) {
		zix_sem_post(&sem);
	}

	printf("Writer finished\n");
	return NULL;
}

int
main(int argc, char** argv)
{
	if (argc > 2) {
		printf("Usage: %s N_SIGNALS\n", argv[0]);
		return 1;
	}

	if (argc > 1) {
		n_signals = atoi(argv[1]);
	}

	printf("Testing %u signals...\n", n_signals);

	if (zix_sem_init(&sem, 0)) {
		fprintf(stderr, "Failed to create semaphore.\n");
		return 1;
	}

	ZixThread reader_thread;
	if (zix_thread_create(&reader_thread, 128, reader, NULL)) {
		fprintf(stderr, "Failed to create reader thread\n");
		return 1;
	}

	ZixThread writer_thread;
	if (zix_thread_create(&writer_thread, 128, writer, NULL)) {
		fprintf(stderr, "Failed to create writer thread\n");
		return 1;
	}

	zix_thread_join(reader_thread, NULL);
	zix_thread_join(writer_thread, NULL);

	zix_sem_destroy(&sem);
	return 0;
}
