#include "ttf.h"
#define MFD_FILTER(FNX) MFD_FLT(0, FNX)
#include "filters/keysplit.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(1, FNX)
#include "filters/quantize.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(2, FNX)
#include "filters/scalecc.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(3, FNX)
#include "filters/eventblocker.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(4, FNX)
#include "filters/midistrum.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(5, FNX)
#include "filters/mididup.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(6, FNX)
#include "filters/passthru.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(7, FNX)
#include "filters/miditranspose.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(8, FNX)
#include "filters/randvelocity.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(9, FNX)
#include "filters/keyrange.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(10, FNX)
#include "filters/ntapdelay.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(11, FNX)
#include "filters/mididelay.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(12, FNX)
#include "filters/mapkeychannel.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(13, FNX)
#include "filters/velocityrange.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(14, FNX)
#include "filters/velocityscale.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(15, FNX)
#include "filters/nodup.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(16, FNX)
#include "filters/channelfilter.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(17, FNX)
#include "filters/monolegato.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(18, FNX)
#include "filters/chord.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(19, FNX)
#include "filters/mapcc.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(20, FNX)
#include "filters/notetocc.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(21, FNX)
#include "filters/singlechannel.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(22, FNX)
#include "filters/channelmap.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(23, FNX)
#include "filters/nosensing.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(24, FNX)
#include "filters/sostenuto.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(25, FNX)
#include "filters/mapkeyscale.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(26, FNX)
#include "filters/notetoggle.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(27, FNX)
#include "filters/cctonote.c"
#undef MFD_FILTER
#define MFD_FILTER(FNX) MFD_FLT(28, FNX)
#include "filters/enforcescale.c"
#undef MFD_FILTER
#define LOOP_DESC(FN) \
FN(0) \
FN(1) \
FN(2) \
FN(3) \
FN(4) \
FN(5) \
FN(6) \
FN(7) \
FN(8) \
FN(9) \
FN(10) \
FN(11) \
FN(12) \
FN(13) \
FN(14) \
FN(15) \
FN(16) \
FN(17) \
FN(18) \
FN(19) \
FN(20) \
FN(21) \
FN(22) \
FN(23) \
FN(24) \
FN(25) \
FN(26) \
FN(27) \
FN(28) \

