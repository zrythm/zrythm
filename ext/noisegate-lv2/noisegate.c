/*
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
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "gate_core.h"

/**********************************************************************************************************************************************************/

typedef enum {
    PLUGIN_INPUT,
    PLUGIN_KEY,
    PLUGIN_OUTPUT,
    PLUGIN_THRESHOLD,
    PLUGIN_ATTACK,
    PLUGIN_HOLD,
    PLUGIN_DECAY
}PortIndex;

/**********************************************************************************************************************************************************/

typedef struct{

    //ports
    float*            input;
    float*              key;
    float*           output;
    float*        threshold;
    float*           attack;
    float*             hold;
    float*            decay;

    uint32_t     sampleRate;

    gate_t noisegate;

} NoiseGate;

/**********************************************************************************************************************************************************/
//                                                                 local functions                                                                        //
/**********************************************************************************************************************************************************/

/**********************************************************************************************************************************************************/
static LV2_Handle
instantiate(const LV2_Descriptor*   descriptor,
double                              samplerate,
const char*                         bundle_path,
const LV2_Feature* const* features)
{
    NoiseGate* self = (NoiseGate*)malloc(sizeof(NoiseGate));

    self->sampleRate = (uint32_t)samplerate;

    Gate_Init(&self->noisegate);

    return (LV2_Handle)self;
}
/**********************************************************************************************************************************************************/
static void connect_port(LV2_Handle instance, uint32_t port, void *data)
{
    NoiseGate* self = (NoiseGate*)instance;

    switch (port)
    {
        case PLUGIN_INPUT:
            self->input = (float*) data;
            break;
        case PLUGIN_KEY:
            self->key = (float*) data;
            break;
        case PLUGIN_OUTPUT:
            self->output = (float*) data;
            break;
        case PLUGIN_THRESHOLD:
            self->threshold = (float*) data;
            break;
        case PLUGIN_ATTACK:
            self->attack = (float*) data;
            break;
        case PLUGIN_HOLD:
            self->hold = (float*) data;
            break;
        case PLUGIN_DECAY:
            self->decay = (float*) data;
            break;
    }
}
/**********************************************************************************************************************************************************/
void activate(LV2_Handle instance)
{
    // TODO: include the activate function code here
}

/**********************************************************************************************************************************************************/
void run(LV2_Handle instance, uint32_t n_samples)
{
    NoiseGate* self = (NoiseGate*)instance;

    //update parameters
    //lower threshold is 20dB lower
    Gate_UpdateParameters(&self->noisegate, (uint32_t)self->sampleRate,
                         (uint32_t)*self->attack, (uint32_t)*self->hold,
                         (uint32_t)*self->decay, 1, *self->threshold, *self->threshold - 20.0f);


    for (uint32_t i = 0; i < n_samples; ++i)
    {
        self->output[i] = Gate_ApplyGate(&self->noisegate, self->input[i], self->key[i]);
    }
}

/**********************************************************************************************************************************************************/
void deactivate(LV2_Handle instance)
{
    // TODO: include the deactivate function code here
}
/**********************************************************************************************************************************************************/
void cleanup(LV2_Handle instance)
{
    free(instance);
}
/**********************************************************************************************************************************************************/
const void* extension_data(const char* uri)
{
    return NULL;
}
/**********************************************************************************************************************************************************/
static const LV2_Descriptor Descriptor = {
    PLUGIN_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data
};
/**********************************************************************************************************************************************************/
LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    if (index == 0) return &Descriptor;
    else return NULL;
}
/**********************************************************************************************************************************************************/
