/*
  LV2 Programs Extension
  Copyright 2012 Filipe Coelho <falktx@falktx.com>

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

/**
   @file lv2_programs.h
   C header for the LV2 programs extension <http://kxstudio.sf.net/ns/lv2ext/programs>.
*/

#ifndef LV2_PROGRAMS_H
#define LV2_PROGRAMS_H

#include "lv2.h"
#include "ui.h"

#define LV2_PROGRAMS_URI    "http://kxstudio.sf.net/ns/lv2ext/programs"
#define LV2_PROGRAMS_PREFIX LV2_PROGRAMS_URI "#"

#define LV2_PROGRAMS__Host        LV2_PROGRAMS_PREFIX "Host"
#define LV2_PROGRAMS__Interface   LV2_PROGRAMS_PREFIX "Interface"
#define LV2_PROGRAMS__UIInterface LV2_PROGRAMS_PREFIX "UIInterface"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* LV2_Programs_Handle;

typedef struct _LV2_Program_Descriptor {

    /** Bank number for this program. Note that this extension does not
        support MIDI-style separation of bank LSB and MSB values. There is
        no restriction on the set of available banks: the numbers do not
        need to be contiguous, there does not need to be a bank 0, etc. */
    uint32_t bank;

    /** Program number (unique within its bank) for this program. There is
        no restriction on the set of available programs: the numbers do not
        need to be contiguous, there does not need to be a program 0, etc. */
    uint32_t program;

    /** Name of the program. */
    const char * name;

} LV2_Program_Descriptor;

/**
   Programs extension, plugin data.

   When the plugin's extension_data is called with argument LV2_PROGRAMS__Interface,
   the plugin MUST return an LV2_Programs_Instance structure, which remains valid
   for the lifetime of the plugin.
*/
typedef struct _LV2_Programs_Interface {
    /**
     * get_program()
     *
     * This member is a function pointer that provides a description
     * of a program (named preset sound) available on this plugin.
     *
     * The index argument is an index into the plugin's list of
     * programs, not a program number as represented by the Program
     * field of the LV2_Program_Descriptor. (This distinction is
     * needed to support plugins that use non-contiguous program or
     * bank numbers.)
     *
     * This function returns a LV2_Program_Descriptor pointer that is
     * guaranteed to be valid only until the next call to get_program
     * or deactivate, on the same plugin instance. This function must
     * return NULL if passed an index argument out of range, so that
     * the host can use it to query the number of programs as well as
     * their properties.
     */
    const LV2_Program_Descriptor *(*get_program)(LV2_Handle handle,
                                                 uint32_t index);

    /**
     * select_program()
     *
     * This member is a function pointer that selects a new program
     * for this plugin. The program change should take effect
     * immediately at the start of the next run() call. (This
     * means that a host providing the capability of changing programs
     * between any two notes on a track must vary the block size so as
     * to place the program change at the right place. A host that
     * wanted to avoid this would probably just instantiate a plugin
     * for each program.)
     *
     * Plugins should ignore a select_program() call with an invalid
     * bank or program.
     *
     * A plugin is not required to select any particular default
     * program on activate(): it's the host's duty to set a program
     * explicitly.
     *
     * A plugin is permitted to re-write the values of its input
     * control ports when select_program is called. The host should
     * re-read the input control port values and update its own
     * records appropriately. (This is the only circumstance in which
     * a LV2 plugin is allowed to modify its own control-input ports.)
     */
    void (*select_program)(LV2_Handle handle,
                           uint32_t bank,
                           uint32_t program);

} LV2_Programs_Interface;

/**
   Programs extension, UI data.

   When the UI's extension_data is called with argument LV2_PROGRAMS__UIInterface,
   the UI MUST return an LV2_Programs_UI_Interface structure, which remains valid
   for the lifetime of the UI.
*/
typedef struct _LV2_Programs_UI_Interface {
    /**
     * select_program()
     *
     * This is exactly the same as select_program in LV2_Programs_Instance,
     * but this struct relates to the UI instead of the plugin.
     *
     * When called, UIs should update their state to match the selected program.
     */
    void (*select_program)(LV2UI_Handle handle,
                           uint32_t bank,
                           uint32_t program);

} LV2_Programs_UI_Interface;

/**
    Feature data for LV2_PROGRAMS__Host.
*/
typedef struct _LV2_Programs_Host {
    /**
     *  Opaque host data.
     */
    LV2_Programs_Handle handle;

    /**
     * program_changed()
     *
     * Tell the host to reload a plugin's program.
     * Parameter handle MUST be the 'handle' member of this struct.
     * Parameter index is program index to change.
     * When index is -1, host should reload all the programs.
     *
     * The plugin MUST NEVER call this function on a RT context or during run().
     *
     * NOTE: This call is to inform the host about a program's bank, program or name change.
     *       It DOES NOT change the current selected program.
     */
    void (*program_changed)(LV2_Programs_Handle handle,
                            int32_t index);

} LV2_Programs_Host;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV2_PROGRAMS_H */
