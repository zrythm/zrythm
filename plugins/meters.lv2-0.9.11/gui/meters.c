#include <stdio.h>

#if (defined _WIN32 && defined RTK_STATIC_INIT)
#include <pthread.h>
#include <glib.h>
#include <glib-object.h>
#endif

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

extern const LV2UI_Descriptor* lv2ui_kmeter (uint32_t index);
extern const LV2UI_Descriptor* lv2ui_needle (uint32_t index);
extern const LV2UI_Descriptor* lv2ui_phasewheel (uint32_t index);
extern const LV2UI_Descriptor* lv2ui_sdhmeter (uint32_t index);
extern const LV2UI_Descriptor* lv2ui_goniometer (uint32_t index);
extern const LV2UI_Descriptor* lv2ui_dr14meter (uint32_t index);
extern const LV2UI_Descriptor* lv2ui_stereoscope (uint32_t index);
extern const LV2UI_Descriptor* lv2ui_ebur (uint32_t index);
extern const LV2UI_Descriptor* lv2ui_dpm (uint32_t index);
extern const LV2UI_Descriptor* lv2ui_bitmeter (uint32_t index);
extern const LV2UI_Descriptor* lv2ui_surmeter (uint32_t index);

#undef LV2_SYMBOL_EXPORT
#ifdef _WIN32
#    define LV2_SYMBOL_EXPORT __declspec(dllexport)
#else
#    define LV2_SYMBOL_EXPORT  __attribute__ ((visibility ("default")))
#endif
LV2_SYMBOL_EXPORT
const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
#if (defined _WIN32 && defined RTK_STATIC_INIT)
	static int once = 0;
	if (!once) {once = 1; gobject_init_ctor();}
#endif
	switch (index) {
	case 0: return lv2ui_kmeter (index);
	case 1: return lv2ui_needle (index);
	case 2: return lv2ui_phasewheel (index);
	case 3: return lv2ui_sdhmeter (index);
	case 4: return lv2ui_goniometer (index);
	case 5: return lv2ui_dr14meter (index);
	case 6: return lv2ui_stereoscope (index);
	case 7: return lv2ui_ebur (index);
	case 8: return lv2ui_dpm (index);
	case 9: return lv2ui_bitmeter (index);
	case 10: return lv2ui_surmeter (index);
	default:
		return NULL;
	}
}

#if (defined _WIN32 && defined RTK_STATIC_INIT)
static void __attribute__((constructor)) x42_init() {
	pthread_win32_process_attach_np();
}

static void __attribute__((destructor)) x42_fini() {
	pthread_win32_process_detach_np();
}
#endif
