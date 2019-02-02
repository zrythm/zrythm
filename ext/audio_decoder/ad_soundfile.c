/**
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
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <math.h>
#include <sndfile.h>

#include "audio_decoder/ad_plugin.h"

/* internal abstraction */

typedef struct {
	SF_INFO sfinfo;
	SNDFILE *sffile;
} sndfile_audio_decoder;

static int parse_bit_depth(int format) {
	/* see http://www.mega-nerd.com/libsndfile/api.html */
	switch (format&0x0f) {
		case SF_FORMAT_PCM_S8: return 8;
		case SF_FORMAT_PCM_16: return 16; /* Signed 16 bit data */
		case SF_FORMAT_PCM_24: return 24; /* Signed 24 bit data */
		case SF_FORMAT_PCM_32: return 32; /* Signed 32 bit data */
		case SF_FORMAT_PCM_U8: return 8;  /* Unsigned 8 bit data (WAV and RAW only) */
		case SF_FORMAT_FLOAT : return 32; /* 32 bit float data */
		case SF_FORMAT_DOUBLE: return 64; /* 64 bit float data */
		default: break;
	}
	return 0;
}

static int ad_info_sndfile(void *sf, struct adinfo *nfo) {
	sndfile_audio_decoder *priv = (sndfile_audio_decoder*) sf;
	if (!priv) return -1;
	if (nfo) {
		nfo->channels    = priv->sfinfo.channels;
		nfo->frames      = priv->sfinfo.frames;
		nfo->sample_rate = priv->sfinfo.samplerate;
		nfo->length      = priv->sfinfo.samplerate ? (priv->sfinfo.frames * 1000) / priv->sfinfo.samplerate : 0;
		nfo->bit_depth   = parse_bit_depth(priv->sfinfo.format);
		nfo->bit_rate    = nfo->bit_depth * nfo->channels * nfo->sample_rate;
		nfo->meta_data   = NULL;
	}
	return 0;
}

static void *ad_open_sndfile(const char *fn, struct adinfo *nfo) {
	sndfile_audio_decoder *priv = (sndfile_audio_decoder*) calloc(1, sizeof(sndfile_audio_decoder));
	priv->sfinfo.format=0;
	if(!(priv->sffile = sf_open(fn, SFM_READ, &priv->sfinfo))){
		dbg(0, "unable to open file '%s'.", fn);
		puts(sf_strerror(NULL));
		int e = sf_error(NULL);
		dbg(0, "error=%i", e);
		free(priv);
		return NULL;
	}
	ad_info_sndfile(priv, nfo);
	return (void*) priv;
}

static int ad_close_sndfile(void *sf) {
	sndfile_audio_decoder *priv = (sndfile_audio_decoder*) sf;
	if (!priv) return -1;
	if(!sf || sf_close(priv->sffile)) { 
		dbg(0, "fatal: bad file close.\n");
		return -1;
	}
	free(priv);
	return 0;
}

static int64_t ad_seek_sndfile(void *sf, int64_t pos) {
	sndfile_audio_decoder *priv = (sndfile_audio_decoder*) sf;
	if (!priv) return -1;
	return sf_seek(priv->sffile, pos, SEEK_SET);
}

static ssize_t ad_read_sndfile(void *sf, float* d, size_t len) {
	sndfile_audio_decoder *priv = (sndfile_audio_decoder*) sf;
	if (!priv) return -1;
	return sf_read_float (priv->sffile, d, len);
}

static int ad_eval_sndfile(const char *f) { 
	char *ext = strrchr(f, '.');
	if (strstr (f, "://")) return 0;
	if (!ext) return 5;
	/* see http://www.mega-nerd.com/libsndfile/ */
	if (!strcasecmp(ext, ".wav")) return 100;
	if (!strcasecmp(ext, ".aiff")) return 100;
	if (!strcasecmp(ext, ".aifc")) return 100;
	if (!strcasecmp(ext, ".snd")) return 100;
	if (!strcasecmp(ext, ".au")) return 100;
	if (!strcasecmp(ext, ".paf")) return 100;
	if (!strcasecmp(ext, ".iff")) return 100;
	if (!strcasecmp(ext, ".svx")) return 100;
	if (!strcasecmp(ext, ".sf")) return 100;
	if (!strcasecmp(ext, ".vcc")) return 100;
	if (!strcasecmp(ext, ".w64")) return 100;
	if (!strcasecmp(ext, ".mat4")) return 100;
	if (!strcasecmp(ext, ".mat5")) return 100;
	if (!strcasecmp(ext, ".pvf5")) return 100;
	if (!strcasecmp(ext, ".xi")) return 100;
	if (!strcasecmp(ext, ".htk")) return 100;
	if (!strcasecmp(ext, ".pvf")) return 100;
	if (!strcasecmp(ext, ".sd2")) return 100;
// libsndfile >= 1.0.18
	if (!strcasecmp(ext, ".flac")) return 80;
	if (!strcasecmp(ext, ".ogg")) return 80;
	return 0;
}

static const ad_plugin ad_sndfile = {
#if 1
  &ad_eval_sndfile,
	&ad_open_sndfile,
	&ad_close_sndfile,
	&ad_info_sndfile,
	&ad_seek_sndfile,
	&ad_read_sndfile
#else
  &ad_eval_null,
	&ad_open_null,
	&ad_close_null,
	&ad_info_null,
	&ad_seek_null,
	&ad_read_null
#endif
};

/* dlopen handler */
const ad_plugin * adp_get_sndfile() {
	return &ad_sndfile;
}
