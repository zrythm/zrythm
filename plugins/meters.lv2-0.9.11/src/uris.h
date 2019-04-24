/* ebu-r128 LV2 GUI
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef MTR_URIS_H
#define MTR_URIS_H

#include <stdio.h>
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#define MTR_URI "http://gareus.org/oss/lv2/meters#"

#ifdef HAVE_LV2_1_8
#define x_forge_object lv2_atom_forge_object
#else
#define x_forge_object lv2_atom_forge_blank
#endif

#define HIST_LEN (751)

#define DIST_BIN   (361)   // 2 * (DIST_OFF + DIST_RANGE) + 1
#define DIST_SIZE  (360.f) // DIST_BIN in float [ 0 .. DIST_BIN [
#define DIST_RANGE (150.f)
#define DIST_ZERO  (180.f) // DIST_OFF + DIST_RANGE

// offsets in histS for bitmeter
#define BIM_DHIT 0
#define BIM_NHIT 23
#define BIM_DONE 280
#define BIM_NONE 303
#define BIM_DSET 560
#define BIM_LAST 584

#define MTR__ebulevels        MTR_URI "ebulevels"
#define MTR_ebu_loudnessM     MTR_URI "ebu_loudnessM"
#define MTR_ebu_maxloudnM     MTR_URI "ebu_maxloudnM"
#define MTR_ebu_loudnessS     MTR_URI "ebu_loudnessS"
#define MTR_ebu_maxloudnS     MTR_URI "ebu_maxloudnS"
#define MTR_ebu_integrated    MTR_URI "ebu_integrated"
#define MTR_ebu_range_min     MTR_URI "ebu_range_min"
#define MTR_ebu_range_max     MTR_URI "ebu_range_max"
#define MTR_ebu_integrating   MTR_URI "ebu_integrating"
#define MTR_ebu_integr_time   MTR_URI "ebu_integr_time"

#define MTR_ebu_state         MTR_URI "ebu_state"
#define MTR_sdh_state         MTR_URI "sdh_state"
#define MTR_bim_state         MTR_URI "bim_state"

#define MTR__rdr_histogram    MTR_URI "rdr_histogram"
#define MTR__rdr_histpoint    MTR_URI "rdr_histpoint"
#define MTR__rdr_radarpoint   MTR_URI "rdr_radarpoint"
#define MTR__rdr_pointpos     MTR_URI "rdr_pointpos"
#define MTR__rdr_pos_cur      MTR_URI "rdr_pos_cur"
#define MTR__rdr_pos_max      MTR_URI "rdr_pos_max"

#define MTR__sdh_histogram    MTR_URI "sdh_histogram"
#define MTR__sdh_hist_max     MTR_URI "sdh_hist_max"
#define MTR__sdh_hist_var     MTR_URI "sdh_hist_var"
#define MTR__sdh_hist_avg     MTR_URI "sdh_hist_avg"
#define MTR__sdh_hist_peak    MTR_URI "sdh_hist_peak"
#define MTR__sdh_hist_data    MTR_URI "sdh_hist_data"
#define MTR__sdh_information  MTR_URI "sdh_information"

#define MTR__bim_information  MTR_URI "bim_information"
#define MTR__bim_averaging    MTR_URI "bim_averaging"
#define MTR__bim_stats        MTR_URI "bim_stats"
#define MTR__bim_data         MTR_URI "bim_data"
#define MTR__bim_zero         MTR_URI "bim_zero"
#define MTR__bim_pos          MTR_URI "bim_pos"
#define MTR__bim_min          MTR_URI "bim_min"
#define MTR__bim_max          MTR_URI "bim_max"
#define MTR__bim_nan          MTR_URI "bim_nan"
#define MTR__bim_inf          MTR_URI "bim_inf"
#define MTR__bim_den          MTR_URI "bim_den"

#define MTR__truepeak         MTR_URI "truepeak"
#define MTR__dr14reset        MTR_URI "dr14reset"

#define MTR__cckey    MTR_URI "controlkey"
#define MTR__ccval    MTR_URI "controlval"
#define MTR__control  MTR_URI "control"

#define MTR__meteron  MTR_URI "meteron"
#define MTR__meteroff MTR_URI "meteroff"
#define MTR__metercfg MTR_URI "metercfg"

typedef struct {
	LV2_URID atom_Blank;
	LV2_URID atom_Object;
	LV2_URID atom_Int;
	LV2_URID atom_Long;
	LV2_URID atom_Float;
	LV2_URID atom_Double;
	LV2_URID atom_Bool;
	LV2_URID atom_Vector;
	LV2_URID atom_eventTransfer;

	LV2_URID time_Position;
	LV2_URID time_speed;
	LV2_URID time_frame;

	LV2_URID mtr_control; // from backend to UI
	LV2_URID mtr_cckey;
	LV2_URID mtr_ccval;

	LV2_URID mtr_meters_on;
	LV2_URID mtr_meters_off;
	LV2_URID mtr_meters_cfg; // from UI -> backend

	LV2_URID mtr_ebulevels;
	LV2_URID ebu_loudnessM;
	LV2_URID ebu_maxloudnM;
	LV2_URID ebu_loudnessS;
	LV2_URID ebu_maxloudnS;
	LV2_URID ebu_integrated;
	LV2_URID ebu_range_min;
	LV2_URID ebu_range_max;
	LV2_URID ebu_integrating;
	LV2_URID ebu_integr_time;

	LV2_URID ebu_state;
	LV2_URID sdh_state;
	LV2_URID bim_state;

	LV2_URID rdr_histogram;
	LV2_URID rdr_histpoint;
	LV2_URID rdr_radarpoint;
	LV2_URID rdr_pointpos;
	LV2_URID rdr_pos_cur;
	LV2_URID rdr_pos_max;

	LV2_URID sdh_histogram;
	LV2_URID sdh_hist_max;
	LV2_URID sdh_hist_var;
	LV2_URID sdh_hist_avg;
	LV2_URID sdh_hist_peak;
	LV2_URID sdh_hist_data;
	LV2_URID sdh_information;

	LV2_URID bim_information;
	LV2_URID bim_averaging;
	LV2_URID bim_stats;
	LV2_URID bim_data;
	LV2_URID bim_zero;
	LV2_URID bim_pos;
	LV2_URID bim_min;
	LV2_URID bim_max;
	LV2_URID bim_nan;
	LV2_URID bim_inf;
	LV2_URID bim_den;

	LV2_URID mtr_truepeak;
	LV2_URID mtr_dr14reset;

} EBULV2URIs;


// numeric keys
enum {
	KEY_INVALID = 0,
	CTL_START,
	CTL_PAUSE,
	CTL_RESET,
	CTL_TRANSPORTSYNC,
	CTL_AUTORESET,
	CTL_RADARTIME,
	CTL_UISETTINGS,
	CTL_LV2_RADARTIME,
	CTL_LV2_FTM,
	CTL_LV2_RESETRADAR,
	CTL_LV2_RESYNCDONE,
	CTL_SAMPLERATE,
	CTL_WINDOWED,
	CTL_AVERAGE,
};


static inline void
map_eburlv2_uris(LV2_URID_Map* map, EBULV2URIs* uris)
{
	uris->atom_Blank         = map->map(map->handle, LV2_ATOM__Blank);
	uris->atom_Object        = map->map(map->handle, LV2_ATOM__Object);
	uris->atom_Int           = map->map(map->handle, LV2_ATOM__Int);
	uris->atom_Long          = map->map(map->handle, LV2_ATOM__Long);
	uris->atom_Float         = map->map(map->handle, LV2_ATOM__Float);
	uris->atom_Double        = map->map(map->handle, LV2_ATOM__Double);
	uris->atom_Bool          = map->map(map->handle, LV2_ATOM__Bool);
	uris->atom_Vector        = map->map(map->handle, LV2_ATOM__Vector);

	uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);

	uris->time_Position       = map->map(map->handle, LV2_TIME__Position);
	uris->time_speed          = map->map(map->handle, LV2_TIME__speed);
	uris->time_frame          = map->map(map->handle, LV2_TIME__frame);

	uris->mtr_ebulevels       = map->map(map->handle, MTR__ebulevels);
	uris->ebu_loudnessM       = map->map(map->handle, MTR_ebu_loudnessM);
	uris->ebu_maxloudnM       = map->map(map->handle, MTR_ebu_maxloudnM);
	uris->ebu_loudnessS       = map->map(map->handle, MTR_ebu_loudnessS);
	uris->ebu_maxloudnS       = map->map(map->handle, MTR_ebu_maxloudnS);
	uris->ebu_integrated      = map->map(map->handle, MTR_ebu_integrated);
	uris->ebu_range_min       = map->map(map->handle, MTR_ebu_range_min);
	uris->ebu_range_max       = map->map(map->handle, MTR_ebu_range_max);
	uris->ebu_integrating     = map->map(map->handle, MTR_ebu_integrating);
	uris->ebu_integr_time     = map->map(map->handle, MTR_ebu_integr_time);

	uris->ebu_state           = map->map(map->handle, MTR_ebu_state);
	uris->sdh_state           = map->map(map->handle, MTR_sdh_state);
	uris->bim_state           = map->map(map->handle, MTR_bim_state);

	uris->rdr_histogram       = map->map(map->handle, MTR__rdr_histogram);
	uris->rdr_histpoint       = map->map(map->handle, MTR__rdr_histpoint);
	uris->rdr_radarpoint      = map->map(map->handle, MTR__rdr_radarpoint);
	uris->rdr_pointpos        = map->map(map->handle, MTR__rdr_pointpos);
	uris->rdr_pos_cur         = map->map(map->handle, MTR__rdr_pos_cur);
	uris->rdr_pos_max         = map->map(map->handle, MTR__rdr_pos_max);

	uris->sdh_histogram       = map->map(map->handle, MTR__sdh_histogram);
	uris->sdh_hist_max        = map->map(map->handle, MTR__sdh_hist_max);
	uris->sdh_hist_var        = map->map(map->handle, MTR__sdh_hist_var);
	uris->sdh_hist_avg        = map->map(map->handle, MTR__sdh_hist_avg);
	uris->sdh_hist_peak       = map->map(map->handle, MTR__sdh_hist_peak);
	uris->sdh_hist_data       = map->map(map->handle, MTR__sdh_hist_data);
	uris->sdh_information     = map->map(map->handle, MTR__sdh_information);

	uris->bim_information     = map->map(map->handle, MTR__bim_information);
	uris->bim_averaging       = map->map(map->handle, MTR__bim_averaging);
	uris->bim_stats           = map->map(map->handle, MTR__bim_stats);
	uris->bim_data            = map->map(map->handle, MTR__bim_data);
	uris->bim_zero            = map->map(map->handle, MTR__bim_zero);
	uris->bim_pos             = map->map(map->handle, MTR__bim_pos);
	uris->bim_min             = map->map(map->handle, MTR__bim_min);
	uris->bim_max             = map->map(map->handle, MTR__bim_max);
	uris->bim_nan             = map->map(map->handle, MTR__bim_nan);
	uris->bim_inf             = map->map(map->handle, MTR__bim_inf);
	uris->bim_den             = map->map(map->handle, MTR__bim_den);

	uris->mtr_truepeak        = map->map(map->handle, MTR__truepeak);
	uris->mtr_dr14reset       = map->map(map->handle, MTR__dr14reset);

	uris->mtr_cckey          = map->map(map->handle, MTR__cckey);
	uris->mtr_ccval          = map->map(map->handle, MTR__ccval);
	uris->mtr_control        = map->map(map->handle, MTR__control);

	uris->mtr_meters_on       = map->map(map->handle, MTR__meteron);
	uris->mtr_meters_off      = map->map(map->handle, MTR__meteroff);
	uris->mtr_meters_cfg      = map->map(map->handle, MTR__metercfg);
}


static inline LV2_Atom *
forge_kvcontrolmessage(LV2_Atom_Forge* forge,
		const EBULV2URIs* uris,
		LV2_URID uri,
		const int key, const float value)
{
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_frame_time(forge, 0);
	LV2_Atom* msg = (LV2_Atom*)x_forge_object(forge, &frame, 1, uri);

	lv2_atom_forge_property_head(forge, uris->mtr_cckey, 0);
	lv2_atom_forge_int(forge, key);
	lv2_atom_forge_property_head(forge, uris->mtr_ccval, 0);
	lv2_atom_forge_float(forge, value);
	lv2_atom_forge_pop(forge, &frame);
	return msg;
}

static inline int
get_cc_key_value(
		const EBULV2URIs* uris, const LV2_Atom_Object* obj,
		int *k, float *v)
{
	const LV2_Atom* key = NULL;
	const LV2_Atom* value = NULL;
	if (!k || !v) return -1;
	*k = 0; *v = 0.0;

	if (obj->body.otype != uris->mtr_control && obj->body.otype != uris->mtr_meters_cfg) {
		return -1;
	}
	lv2_atom_object_get(obj, uris->mtr_cckey, &key, uris->mtr_ccval, &value, 0);
	if (!key || !value) {
		fprintf(stderr, "MTRlv2: Malformed ctrl message has no key or value.\n");
		return -1;
	}
	*k = ((LV2_Atom_Int*)key)->body;
	*v = ((LV2_Atom_Float*)value)->body;
	return 0;
}

#endif
