/**
   @brief audio-decoder - wrapper around libsndfile and libav*
   @file ad.h
   @author Robin Gareus <robin@gareus.org>

   Copyright (C) 2011-2013 Robin Gareus <robin@gareus.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser Public License as published by
   the Free Software Foundation; either version 2.1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/
#ifndef __AD_H__
#define __AD_H__

#include <unistd.h>
#include <stdint.h>

struct adinfo {
	unsigned int sample_rate;
	unsigned int channels;
	int64_t length; //milliseconds
	int64_t frames; //total number of frames (eg a frame for 16bit stereo is 4 bytes).
	int     bit_rate;
	int     bit_depth;
	char *  meta_data;
};

/* global init function - register codecs */
void ad_init();

/* --- public API --- */

/** open an audio file
 * @param fn file-name
 * @param nfo pointer to a adinfo struct which will hold information about the file.
 * @return NULL on error, a pointer to an opaque soundfile-decoder object on success.
 */
void *  ad_open  (const char *fn, struct adinfo *nfo);

/** close an audio file and release decoder structures
 * @param sf decoder handle
 * @return 0 on succees, -1 if sf was invalid or not open (return value can usually be ignored)
 */
int     ad_close (void *sf);

/** seel to a given position in the file
 * @param sf decoder handle
 * @param pos frame position to seek to in frames (1 frame = number-of-channel samples) from the start of the file.
 * @return the current position in frames (multi-channel samples) from the start of the file. On error this function returns -1.
 */
int64_t ad_seek  (void *sf, int64_t pos);

/** decode audio data chunk to raw interleaved channel floating point data
 *
 * @param sf decoder handle
 * @param out place to store data -- must be large enough to hold  (sizeof(float) * len) bytes.
 * @param len number of samples (!) to read (should be a multiple of nfo->channels).
 * @return the number of read samples.
 */
ssize_t ad_read  (void *sf, float* out, size_t len);

/** re-read the file information and meta-data.
 *
 * this is not neccesary in general \ref ad_open includes an inplicit call
 * but meta-data may change in live-stream in which case en explicit call to
 * ad_into is needed to update the inforation
 *
 * @param fn file-name
 * @param nfo pointer to a adinfo struct which will hold information about the file.
 * @return 0 on succees, -1 if sf was invalid or not open
 */
int     ad_info  (void *sf, struct adinfo *nfo);

/** zero initialize the information struct.  * (does not free nfo->meta_data)
 * @param nfo pointer to a adinfo struct
 */
void    ad_clear_nfo     (struct adinfo *nfo);

/** free possibly allocated meta-data text
 * @param nfo pointer to a adinfo struct
 */
void    ad_free_nfo      (struct adinfo *nfo);


/* --- helper functions --- */

/** read file info
 *  combines ad_open() and ad_close()
 */
int ad_finfo             (const char *, struct adinfo *);

/**
 * wrapper around \ref ad_read, downmixes all channels to mono
 */
ssize_t ad_read_mono_dbl (void *, struct adinfo *, double*, size_t);

/**
 * calls dbg() to print file info to stderr.
 *
 * @param dbglvl
 * @param nfo
 */
void ad_dump_nfo            (int dbglvl, struct adinfo *nfo);

/** set audio-decoder debug level  -- all info is printed to stderr.
 *
 * @param lvl debug-level threshold
 * -1: absolutley silent
 *  0: errors only
 *  1: errors + info
 *  2: + debug
 *  3: + low-level-debug info
 */
void ad_set_debuglevel(int lvl);

#endif
