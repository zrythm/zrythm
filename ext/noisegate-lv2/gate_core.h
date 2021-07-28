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

#ifndef GATE_CORE_H_INCLUDED
#define GATE_CORE_H_INCLUDED

#include <stdint.h> 
#include "circular_buffer.h"

typedef enum {
    IDLE,
    HOLD,
    DECAY
} gate_state_t;

typedef struct GATE_T {
    float _alpha;
    float _rmsValue, _keyValue, _upperThreshold, _lowerThreshold;
    uint32_t _attackTime, _decayTime, _holdTime;
    uint32_t _attackCounter, _decayCounter, _holdCounter;
    uint32_t _currentState;
    uint32_t _tau;
    
    gate_state_t state;

    ringbuffer_t window;
} gate_t;

/// <summary>This method called to initialize the Gate</summary>
void Gate_Init(gate_t *gate);

/// <summary>This method called to apply the Gate to the input sample</summary>
/// <param name="input">Holds input sample</param> 
/// <param name="output">Holds the address where the output should be written to</param> 
float Gate_ApplyGate(gate_t *gate, const float input, const float key);
                
/// <summary>This method called to set the length of the window</summary>
/// <param name="length">Holds the value that determines the window length</param>  
void Gate_UpdateParameters(gate_t *gate, const uint32_t sampleRate, const uint32_t attack, const uint32_t hold,
                      const uint32_t decay, const uint32_t alpha, const float upperThreshold,
                      const float lowerThreshold);

#endif //GATE_CORE_H_INCLUDED
