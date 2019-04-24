/* -*- c-basic-offset: 4 -*- */

/* dssi.h

   DSSI version 1.0
   Copyright (c) 2004, 2009 Chris Cannam, Steve Harris and Sean Bolton
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA  02110-1301  USA
*/

#ifndef DSSI_INCLUDED
#define DSSI_INCLUDED

#include <ladspa.h>
#include <alsa/seq_event.h>

#define DSSI_VERSION "1.0"
#define DSSI_VERSION_MAJOR 1
#define DSSI_VERSION_MINOR 0

#ifdef __cplusplus
extern "C" {
#endif

/* 
   There is a need for an API that supports hosted MIDI soft synths
   with GUIs in Linux audio applications.  In time the GMPI initiative
   should comprehensively address this need, but the requirement for
   Linux applications to be able to support simple hosted synths is
   here now, and GMPI is not.  This proposal (the "DSSI Soft Synth
   Interface" or DSSI, pronounced "dizzy") aims to provide a simple
   solution in a way that we hope will prove complete and compelling
   enough to support now, yet not so compelling as to supplant GMPI or
   any other comprehensive future proposal.

   For simplicity and familiarity, this API is based as far as
   possible on existing work -- the LADSPA plugin API for control
   values and audio processing, and the ALSA sequencer event types for
   MIDI event communication.  The GUI part of the proposal is quite
   new, but may also be applicable retroactively to LADSPA plugins
   that do not otherwise support this synth interface.
*/

typedef struct _DSSI_Program_Descriptor {

    /** Bank number for this program.  Note that DSSI does not support
        MIDI-style separation of bank LSB and MSB values.  There is no
        restriction on the set of available banks: the numbers do not
        need to be contiguous, there does not need to be a bank 0, etc. */
    unsigned long Bank;

    /** Program number (unique within its bank) for this program.
	There is no restriction on the set of available programs: the
	numbers do not need to be contiguous, there does not need to
	be a program 0, etc. */
    unsigned long Program;

    /** Name of the program. */
    const char * Name;

} DSSI_Program_Descriptor;


typedef struct _DSSI_Descriptor {

    /**
     * DSSI_API_Version
     *
     * This member indicates the DSSI API level used by this plugin.
     * If we're lucky, this will never be needed.  For now all plugins
     * must set it to 1.
     */
    int DSSI_API_Version;

    /**
     * LADSPA_Plugin
     *
     * A DSSI synth plugin consists of a LADSPA plugin plus an
     * additional framework for controlling program settings and
     * transmitting MIDI events.  A plugin must fully implement the
     * LADSPA descriptor fields as well as the required LADSPA
     * functions including instantiate() and (de)activate().  It
     * should also implement run(), with the same behaviour as if
     * run_synth() (below) were called with no synth events.
     *
     * In order to instantiate a synth the host calls the LADSPA
     * instantiate function, passing in this LADSPA_Descriptor
     * pointer.  The returned LADSPA_Handle is used as the argument
     * for the DSSI functions below as well as for the LADSPA ones.
     */
    const LADSPA_Descriptor *LADSPA_Plugin;

    /**
     * configure()
     *
     * This member is a function pointer that sends a piece of
     * configuration data to the plugin.  The key argument specifies
     * some aspect of the synth's configuration that is to be changed,
     * and the value argument specifies a new value for it.  A plugin
     * that does not require this facility at all may set this member
     * to NULL.
     *
     * This call is intended to set some session-scoped aspect of a
     * plugin's behaviour, for example to tell the plugin to load
     * sample data from a particular file.  The plugin should act
     * immediately on the request.  The call should return NULL on
     * success, or an error string that may be shown to the user.  The
     * host will free the returned value after use if it is non-NULL.
     *
     * Calls to configure() are not automated as timed events.
     * Instead, a host should remember the last value associated with
     * each key passed to configure() during a given session for a
     * given plugin instance, and should call configure() with the
     * correct value for each key the next time it instantiates the
     * "same" plugin instance, for example on reloading a project in
     * which the plugin was used before.  Plugins should note that a
     * host may typically instantiate a plugin multiple times with the
     * same configuration values, and should share data between
     * instances where practical.
     *
     * Calling configure() completely invalidates the program and bank
     * information last obtained from the plugin.
     *
     * Reserved and special key prefixes
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * The DSSI: prefix
     * ----------------
     * Configure keys starting with DSSI: are reserved for particular
     * purposes documented in the DSSI specification.  At the moment,
     * there is one such key: DSSI:PROJECT_DIRECTORY.  A host may call
     * configure() passing this key and a directory path value.  This
     * indicates to the plugin and its UI that a directory at that
     * path exists and may be used for project-local data.  Plugins
     * may wish to use the project directory as a fallback location
     * when looking for other file data, or as a base for relative
     * paths in other configuration values.
     *
     * The GLOBAL: prefix
     * ------------------
     * Configure keys starting with GLOBAL: may be used by the plugin
     * and its UI for any purpose, but are treated specially by the
     * host.  When one of these keys is used in a configure OSC call
     * from the plugin UI, the host makes the corresponding configure
     * call (preserving the GLOBAL: prefix) not only to the target
     * plugin but also to all other plugins in the same instance
     * group, as well as their UIs.  Note that if any instance
     * returns non-NULL from configure to indicate error, the host
     * may stop there (and the set of plugins on which configure has
     * been called will thus depend on the host implementation).
     * See also the configure OSC call documentation in RFC.txt.
     */
    char *(*configure)(LADSPA_Handle Instance,
		       const char *Key,
		       const char *Value);

    #define DSSI_RESERVED_CONFIGURE_PREFIX "DSSI:"
    #define DSSI_GLOBAL_CONFIGURE_PREFIX "GLOBAL:"
    #define DSSI_PROJECT_DIRECTORY_KEY \
	DSSI_RESERVED_CONFIGURE_PREFIX "PROJECT_DIRECTORY"

    /**
     * get_program()
     *
     * This member is a function pointer that provides a description
     * of a program (named preset sound) available on this synth.  A
     * plugin that does not support programs at all should set this
     * member to NULL.
     *
     * The Index argument is an index into the plugin's list of
     * programs, not a program number as represented by the Program
     * field of the DSSI_Program_Descriptor.  (This distinction is
     * needed to support synths that use non-contiguous program or
     * bank numbers.)
     *
     * This function returns a DSSI_Program_Descriptor pointer that is
     * guaranteed to be valid only until the next call to get_program,
     * deactivate, or configure, on the same plugin instance.  This
     * function must return NULL if passed an Index argument out of
     * range, so that the host can use it to query the number of
     * programs as well as their properties.
     */
    const DSSI_Program_Descriptor *(*get_program)(LADSPA_Handle Instance,
						  unsigned long Index);
    
    /**
     * select_program()
     *
     * This member is a function pointer that selects a new program
     * for this synth.  The program change should take effect
     * immediately at the start of the next run_synth() call.  (This
     * means that a host providing the capability of changing programs
     * between any two notes on a track must vary the block size so as
     * to place the program change at the right place.  A host that
     * wanted to avoid this would probably just instantiate a plugin
     * for each program.)
     * 
     * A plugin that does not support programs at all should set this
     * member NULL.  Plugins should ignore a select_program() call
     * with an invalid bank or program.
     *
     * A plugin is not required to select any particular default
     * program on activate(): it's the host's duty to set a program
     * explicitly.  The current program is invalidated by any call to
     * configure().
     *
     * A plugin is permitted to re-write the values of its input
     * control ports when select_program is called.  The host should
     * re-read the input control port values and update its own
     * records appropriately.  (This is the only circumstance in
     * which a DSSI plugin is allowed to modify its own input ports.)
     */
    void (*select_program)(LADSPA_Handle Instance,
			   unsigned long Bank,
			   unsigned long Program);

    /**
     * get_midi_controller_for_port()
     *
     * This member is a function pointer that returns the MIDI
     * controller number or NRPN that should be mapped to the given
     * input control port.  If the given port should not have any MIDI
     * controller mapped to it, the function should return DSSI_NONE.
     * The behaviour of this function is undefined if the given port
     * number does not correspond to an input control port.  A plugin
     * that does not want MIDI controllers mapped to ports at all may
     * set this member NULL.
     *
     * Correct values can be got using the macros DSSI_CC(num) and
     * DSSI_NRPN(num) as appropriate, and values can be combined using
     * bitwise OR: e.g. DSSI_CC(23) | DSSI_NRPN(1069) means the port
     * should respond to CC #23 and NRPN #1069.
     *
     * The host is responsible for doing proper scaling from MIDI
     * controller and NRPN value ranges to port ranges according to
     * the plugin's LADSPA port hints.  Hosts should not deliver
     * through run_synth any MIDI controller events that have already
     * been mapped to control port values.
     *
     * A plugin should not attempt to request mappings from
     * controllers 0 or 32 (MIDI Bank Select MSB and LSB).
     */
    int (*get_midi_controller_for_port)(LADSPA_Handle Instance,
					unsigned long Port);

    /**
     * run_synth()
     *
     * This member is a function pointer that runs a synth for a
     * block.  This is identical in function to the LADSPA run()
     * function, except that it also supplies events to the synth.
     *
     * A plugin may provide this function, run_multiple_synths() (see
     * below), both, or neither (if it is not in fact a synth).  A
     * plugin that does not provide this function must set this member
     * to NULL.  Authors of synth plugins are encouraged to provide
     * this function if at all possible.
     *
     * The Events pointer points to a block of EventCount ALSA
     * sequencer events, which is used to communicate MIDI and related
     * events to the synth.  Each event is timestamped relative to the
     * start of the block, (mis)using the ALSA "tick time" field as a
     * frame count. The host is responsible for ensuring that events
     * with differing timestamps are already ordered by time.
     *
     * See also the notes on activation, port connection etc in
     * ladpsa.h, in the context of the LADSPA run() function.
     *
     * Note Events
     * ~~~~~~~~~~~
     * There are two minor requirements aimed at making the plugin
     * writer's life as simple as possible:
     * 
     * 1. A host must never send events of type SND_SEQ_EVENT_NOTE.
     * Notes should always be sent as separate SND_SEQ_EVENT_NOTE_ON
     * and NOTE_OFF events.  A plugin should discard any one-point
     * NOTE events it sees.
     * 
     * 2. A host must not attempt to switch notes off by sending
     * zero-velocity NOTE_ON events.  It should always send true
     * NOTE_OFFs.  It is the host's responsibility to remap events in
     * cases where an external MIDI source has sent it zero-velocity
     * NOTE_ONs.
     *
     * Bank and Program Events
     * ~~~~~~~~~~~~~~~~~~~~~~~
     * Hosts must map MIDI Bank Select MSB and LSB (0 and 32)
     * controllers and MIDI Program Change events onto the banks and
     * programs specified by the plugin, using the DSSI select_program
     * call.  No host should ever deliver a program change or bank
     * select controller to a plugin via run_synth.
     */
    void (*run_synth)(LADSPA_Handle    Instance,
		      unsigned long    SampleCount,
		      snd_seq_event_t *Events,
		      unsigned long    EventCount);

    /**
     * run_synth_adding()
     *
     * This member is a function pointer that runs an instance of a
     * synth for a block, adding its outputs to the values already
     * present at the output ports.  This is provided for symmetry
     * with LADSPA run_adding(), and is equally optional.  A plugin
     * that does not provide it must set this member to NULL.
     */
    void (*run_synth_adding)(LADSPA_Handle    Instance,
			     unsigned long    SampleCount,
			     snd_seq_event_t *Events,
			     unsigned long    EventCount);

    /**
     * run_multiple_synths()
     *
     * This member is a function pointer that runs multiple synth
     * instances for a block.  This is very similar to run_synth(),
     * except that Instances, Events, and EventCounts each point to
     * arrays that hold the LADSPA handles, event buffers, and
     * event counts for each of InstanceCount instances.  That is,
     * Instances points to an array of InstanceCount pointers to
     * DSSI plugin instantiations, Events points to an array of
     * pointers to each instantiation's respective event list, and
     * EventCounts points to an array containing each instantiation's
     * respective event count.
     *
     * A host using this function must guarantee that ALL active
     * instances of the plugin are represented in each call to the
     * function -- that is, a host may not call run_multiple_synths()
     * for some instances of a given plugin and then call run_synth()
     * as well for others.  'All .. instances of the plugin' means
     * every instance sharing the same LADSPA label and shared object
     * (*.so) file (rather than every instance sharing the same *.so).
     * 'Active' means any instance for which activate() has been called
     * but deactivate() has not.
     *
     * A plugin may provide this function, run_synths() (see above),
     * both, or neither (if it not in fact a synth).  A plugin that
     * does not provide this function must set this member to NULL.
     * Plugin authors implementing run_multiple_synths are strongly
     * encouraged to implement run_synth as well if at all possible,
     * to aid simplistic hosts, even where it would be less efficient
     * to use it.
     */
    void (*run_multiple_synths)(unsigned long     InstanceCount,
                                LADSPA_Handle    *Instances,
                                unsigned long     SampleCount,
                                snd_seq_event_t **Events,
                                unsigned long    *EventCounts);

    /**
     * run_multiple_synths_adding()
     *
     * This member is a function pointer that runs multiple synth
     * instances for a block, adding each synth's outputs to the
     * values already present at the output ports.  This is provided
     * for symmetry with both the DSSI run_multiple_synths() and LADSPA
     * run_adding() functions, and is equally optional.  A plugin
     * that does not provide it must set this member to NULL.
     */
    void (*run_multiple_synths_adding)(unsigned long     InstanceCount,
                                       LADSPA_Handle    *Instances,
                                       unsigned long     SampleCount,
                                       snd_seq_event_t **Events,
                                       unsigned long    *EventCounts);
} DSSI_Descriptor;

/**
 * DSSI supports a plugin discovery method similar to that of LADSPA:
 *
 * - DSSI hosts may wish to locate DSSI plugin shared object files by
 *    searching the paths contained in the DSSI_PATH and LADSPA_PATH
 *    environment variables, if they are present.  Both are expected
 *    to be colon-separated lists of directories to be searched (in
 *    order), and DSSI_PATH should be searched first if both variables
 *    are set.
 *
 * - Each shared object file containing DSSI plugins must include a
 *   function dssi_descriptor(), with the following function prototype
 *   and C-style linkage.  Hosts may enumerate the plugin types
 *   available in the shared object file by repeatedly calling
 *   this function with successive Index values (beginning from 0),
 *   until a return value of NULL indicates no more plugin types are
 *   available.  Each non-NULL return is the DSSI_Descriptor
 *   of a distinct plugin type.
 */

const DSSI_Descriptor *dssi_descriptor(unsigned long Index);
  
typedef const DSSI_Descriptor *(*DSSI_Descriptor_Function)(unsigned long Index);

/*
 * Macros to specify particular MIDI controllers in return values from
 * get_midi_controller_for_port()
 */

#define DSSI_CC_BITS			0x20000000
#define DSSI_NRPN_BITS			0x40000000

#define DSSI_NONE			-1
#define DSSI_CONTROLLER_IS_SET(n)	(DSSI_NONE != (n))

#define DSSI_CC(n)			(DSSI_CC_BITS | (n))
#define DSSI_IS_CC(n)			(DSSI_CC_BITS & (n))
#define DSSI_CC_NUMBER(n)		((n) & 0x7f)

#define DSSI_NRPN(n)			(DSSI_NRPN_BITS | ((n) << 7))
#define DSSI_IS_NRPN(n)			(DSSI_NRPN_BITS & (n))
#define DSSI_NRPN_NUMBER(n)		(((n) >> 7) & 0x3fff)

#ifdef __cplusplus
}
#endif

#endif /* DSSI_INCLUDED */
