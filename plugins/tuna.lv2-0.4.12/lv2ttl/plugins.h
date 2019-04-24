#include "lv2ttl/tuna1.h"
#include "lv2ttl/tuna2.h"

#define X42_MULTIPLUGIN_NAME "Instrument Tuner"
#define X42_MULTIPLUGIN_URI "http://gareus.org/oss/lv2/tuna"

static const RtkLv2Description _plugins[] = {
	_plugin_tuna_one,
	_plugin_tuna_two,
};
