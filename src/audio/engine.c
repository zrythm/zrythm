/*
 * audi/engine.c - The audio engine of zyrthm
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "project.h"
#include "audio/engine.h"

#include <math.h>
#include <stdlib.h>
#include <signal.h>

#include <gtk/gtk.h>

#include <jack/jack.h>

jack_port_t *output_port1, *output_port2;
jack_client_t *client;


/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client follows a simple rule: when the JACK transport is
 * running, copy the input port to the output.  When it stops, exit.
 */

int
process (jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *out1, *out2;
	int i;

	out1 = (jack_default_audio_sample_t*)jack_port_get_buffer (output_port1, nframes);
	out2 = (jack_default_audio_sample_t*)jack_port_get_buffer (output_port2, nframes);

	for( i=0; i<nframes; i++ )
	{
		out1[i] = 0.1f;  /* left */
		out2[i] = 0.2f;  /* right */
	}

	return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown (void *arg)
{
    // TODO
    g_error ("Jack shutting down...");
    exit (1);
}

/**
 * Init audio engine
 */
void
init_audio_engine()
{
    g_message ("Initializing audio engine...");
    project->audio_engine = malloc (sizeof (Audio_Engine));

	const char **ports;
	const char *client_name = "zrythm";
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;
	int i;

	// open a client connection to the JACK server
	client = jack_client_open (client_name, options, &status, server_name);
	if (client == NULL) {
		g_error ("jack_client_open() failed, "
			 "status = 0x%2.0x", status);
		if (status & JackServerFailed) {
			g_error ("Unable to connect to JACK server");
		}
		exit (1);
	}
	if (status & JackServerStarted) {
        // FIXME g_info
		g_message ("JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		g_error ("unique name `%s' assigned\n", client_name);
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	jack_set_process_callback (client, process, NULL);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown (client, jack_shutdown, 0);

	/* create two ports */

	output_port1 = jack_port_register (client, "output1",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput, 0);

	output_port2 = jack_port_register (client, "output2",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput, 0);

	if ((output_port1 == NULL) || (output_port2 == NULL)) {
		g_error ("no more JACK ports available\n");
		exit (1);
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate (client)) {
		g_error ("cannot activate client");
		exit (1);
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsInput);
	if (ports == NULL) {
		g_error ("no physical playback ports\n");
		exit (1);
	}

	if (jack_connect (client, jack_port_name (output_port1), ports[0])) {
		g_error ("cannot connect output ports\n");
	}

	if (jack_connect (client, jack_port_name (output_port2), ports[1])) {
		g_error ("cannot connect output ports\n");
	}

    jack_free (ports);
}

void
close_audio_engine ()
{
    g_message ("closing audio engine...");

    jack_client_close (client);
}
