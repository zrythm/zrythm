#define MULTIPLUGIN 1
#define X42_MULTIPLUGIN_NAME "Meter Collection"
#define X42_MULTIPLUGIN_URI "http://gareus.org/oss/lv2/meters"

#include "lv2ttl/ebur128.h"
#include "lv2ttl/k20stereo.h"
#include "lv2ttl/k14stereo.h"
#include "lv2ttl/k12stereo.h"
#include "lv2ttl/bbc2c.h"
#include "lv2ttl/bbcm6.h"
#include "lv2ttl/din2c.h"
#include "lv2ttl/ebu2c.h"
#include "lv2ttl/nor2c.h"
#include "lv2ttl/vu2c.h"
#include "lv2ttl/tp_rms_stereo.h"
#include "lv2ttl/dr14stereo.h"
#include "lv2ttl/cor.h"
#include "lv2ttl/goniometer.h"
#include "lv2ttl/phasewheel.h"
#include "lv2ttl/spectr30.h"
#include "lv2ttl/stereoscope.h"
#include "lv2ttl/sigdisthist.h"
#include "lv2ttl/bitmeter.h"
#include "lv2ttl/surmeter.h"

static const RtkLv2Description _plugins[] = {
	_plugin_ebur,
	_plugin_k20stereo,
	_plugin_k14stereo,
	_plugin_k12stereo,
	/* iec spec'ed meters */
	_plugin_bbc2c,
	_plugin_bbcm6,
	_plugin_din2c,
	_plugin_ebu2c,
	_plugin_nor2c,
	_plugin_vu2c,
	/* misc loudness */
	_plugin_tprms2,
	_plugin_dr14,
	/* phase related */
	_plugin_cor,
	_plugin_goniometer,
	_plugin_phasewheel,
	/* spectral */
	_plugin_spectr30,
	_plugin_stereoscope,
	/* signal statistics */
	_plugin_sigdisthist,
	_plugin_bitmeter,
	_plugin_surmeter,
};
