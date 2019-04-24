/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/* jack-dssi-host.h
 *
 * DSSI Soft Synth Interface
 *
 * This is a host for DSSI plugins.  It listens for MIDI events on an
 * ALSA sequencer port, delivers them to DSSI synths and outputs the
 * result via JACK.
 */

/*
 * Copyright 2004, 2009 Chris Cannam, Steve Harris and Sean Bolton.
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * for any purpose is hereby granted without fee, provided that the
 * above copyright notice and this permission notice are included in
 * all copies or substantial portions of the software.
 */

#ifndef _JACK_DSSI_HOST_H
#define _JACK_DSSI_HOST_H

#include "dssi.h"
#include <lo/lo.h>

#define D3H_MAX_CHANNELS   16  /* MIDI limit */
#define D3H_MAX_INSTANCES  (D3H_MAX_CHANNELS)

/* character used to seperate DSO names from plugin labels on command line */
#define LABEL_SEP ':'

typedef struct _d3h_dll_t d3h_dll_t;

struct _d3h_dll_t {
    d3h_dll_t               *next;
    char                    *name;
    char                    *directory;
    int                      is_DSSI_dll;
    DSSI_Descriptor_Function descfn;      /* if is_DSSI_dll is false, this is a LADSPA_Descriptor_Function */
};

typedef struct _d3h_plugin_t d3h_plugin_t;

struct _d3h_plugin_t {
    d3h_plugin_t          *next;
    int                    number;
    d3h_dll_t             *dll;
    char                  *label;
    int                    is_first_in_dll;
    const DSSI_Descriptor *descriptor;
    int                    ins;
    int                    outs;
    int                    controlIns;
    int                    controlOuts;
    int                    instances;
};

typedef struct _d3h_instance_t d3h_instance_t;

#define MIDI_CONTROLLER_COUNT 128

struct _d3h_instance_t {
    int              number;
    d3h_plugin_t    *plugin;
    int              channel;
    int              inactive;
    char            *friendly_name;
    int              firstControlIn;                       /* the offset to translate instance control in # to global control in # */
    int             *pluginPortControlInNumbers;           /* maps instance LADSPA port # to global control in # */
    long             controllerMap[MIDI_CONTROLLER_COUNT]; /* maps MIDI controller to global control in # */

    int              pluginProgramCount;
    DSSI_Program_Descriptor
                    *pluginPrograms;
    long             currentBank;
    long             currentProgram;
    int              pendingBankLSB;
    int              pendingBankMSB;
    int              pendingProgramChange;

    lo_address       uiTarget;
    lo_address       uiSource;
    int              ui_initial_show_sent;
    int              uiNeedsProgramUpdate;
    char            *ui_osc_control_path;
    char            *ui_osc_configure_path;
    char            *ui_osc_program_path;
    char            *ui_osc_quit_path;
    char            *ui_osc_rate_path;
    char            *ui_osc_show_path;
};

#endif /* _JACK_DSSI_HOST_H */

