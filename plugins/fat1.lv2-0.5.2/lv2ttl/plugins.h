#define X42_MULTIPLUGIN_NAME "Autotune"
#define X42_MULTIPLUGIN_URI "http://gareus.org/oss/lv2/fat1"

#include "lv2ttl/fat1_chroma.h"
#include "lv2ttl/fat1_micro.h"

static const RtkLv2Description _plugins[] = {
	_fat1_micro,
	_fat1_chroma
};
