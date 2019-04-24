/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/* message_buffer.c

   This file is in the public domain.
*/

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

#define BUFFERS      16		/* must be 2^n */
#define BUFFER_SIZE 256

static char buffer[BUFFERS][BUFFER_SIZE];
static const char *mb_prefix;
static unsigned int initialised = 0;
static unsigned int in_buffer = 0;
static unsigned int out_buffer = 0;
static pthread_t writer_thread;

void *mb_thread_func(void *arg);

void add_message(const char *msg)
{
    strncpy(buffer[in_buffer], msg, BUFFER_SIZE - 1);
    in_buffer = (in_buffer + 1) & (BUFFERS - 1);
}

void mb_init(const char *prefix)
{
    if (initialised) {
	return;
    }
    mb_prefix = prefix;

    pthread_create(&writer_thread, NULL, &mb_thread_func, NULL);

    initialised = 1;
}

void *mb_thread_func(void *arg)
{
    while (1) {
	while (out_buffer != in_buffer) {
	    printf("%s%s", mb_prefix, buffer[out_buffer]);
	    out_buffer = (out_buffer + 1) & (BUFFERS - 1);
	}
	usleep(1000);
    }

    return NULL;
}

/* vi:set ts=8 sts=4 sw=4: */
