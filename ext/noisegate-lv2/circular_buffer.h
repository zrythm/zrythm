/*
  ==============================================================================

 * VEJA NoiseGate
 * Copyright (C) 2021 Jan Janssen <jan@moddevices.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  ==============================================================================
*/

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>

#define MAX_BUFFER_SIZE 128

typedef struct RINGBUFFER_T {
	uint32_t S;
	float m_buffer[MAX_BUFFER_SIZE];
	uint32_t m_size;
	uint32_t m_front;
	uint32_t m_back;
	float power;
} ringbuffer_t;

void ringbuffer_clear(ringbuffer_t *buffer, uint32_t size);
void ringbuffer_push(ringbuffer_t *buffer);
void ringbuffer_push_sample(ringbuffer_t *buffer, const float x);
void ringbuffer_pop(ringbuffer_t *buffer);
void ringbuffer_back_erase(ringbuffer_t *buffer, const uint32_t n);
void ringbuffer_front_erase(ringbuffer_t *buffer, const uint32_t n);
int ringbuffer_peek_index(ringbuffer_t *buffer);
float ringbuffer_push_and_calculate_power(ringbuffer_t *buffer, const float input);
float ringbuffer_front(ringbuffer_t *buffer);
float ringbuffer_back(ringbuffer_t *buffer);
float ringbuffer_get_val(ringbuffer_t *buffer, uint32_t index);
int ringbuffer_empty(ringbuffer_t *buffer);
int ringbuffer_full(ringbuffer_t *buffer);
float * ringbuffer_get_first_pointer(ringbuffer_t *buffer);

#endif // __RINGBUFFER_H__
