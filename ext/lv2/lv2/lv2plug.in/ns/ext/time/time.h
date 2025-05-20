// Copyright 2011-2016 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LV2_TIME_TIME_H
#define LV2_TIME_TIME_H

/**
   @defgroup time Time
   @ingroup lv2

   Properties for describing time.

   Note the time extension is purely data, this header merely defines URIs for
   convenience.

   See <http://lv2plug.in/ns/ext/time> for details.

   @{
*/

// clang-format off

#define LV2_TIME_URI    "http://lv2plug.in/ns/ext/time"  ///< http://lv2plug.in/ns/ext/time
#define LV2_TIME_PREFIX LV2_TIME_URI "#"                 ///< http://lv2plug.in/ns/ext/time#

#define LV2_TIME__Time            LV2_TIME_PREFIX "Time"             ///< http://lv2plug.in/ns/ext/time#Time
#define LV2_TIME__Position        LV2_TIME_PREFIX "Position"         ///< http://lv2plug.in/ns/ext/time#Position
#define LV2_TIME__Rate            LV2_TIME_PREFIX "Rate"             ///< http://lv2plug.in/ns/ext/time#Rate
#define LV2_TIME__position        LV2_TIME_PREFIX "position"         ///< http://lv2plug.in/ns/ext/time#position
#define LV2_TIME__barBeat         LV2_TIME_PREFIX "barBeat"          ///< http://lv2plug.in/ns/ext/time#barBeat
#define LV2_TIME__bar             LV2_TIME_PREFIX "bar"              ///< http://lv2plug.in/ns/ext/time#bar
#define LV2_TIME__beat            LV2_TIME_PREFIX "beat"             ///< http://lv2plug.in/ns/ext/time#beat
#define LV2_TIME__beatUnit        LV2_TIME_PREFIX "beatUnit"         ///< http://lv2plug.in/ns/ext/time#beatUnit
#define LV2_TIME__beatsPerBar     LV2_TIME_PREFIX "beatsPerBar"      ///< http://lv2plug.in/ns/ext/time#beatsPerBar
#define LV2_TIME__beatsPerMinute  LV2_TIME_PREFIX "beatsPerMinute"   ///< http://lv2plug.in/ns/ext/time#beatsPerMinute
#define LV2_TIME__frame           LV2_TIME_PREFIX "frame"            ///< http://lv2plug.in/ns/ext/time#frame
#define LV2_TIME__framesPerSecond LV2_TIME_PREFIX "framesPerSecond"  ///< http://lv2plug.in/ns/ext/time#framesPerSecond
#define LV2_TIME__speed           LV2_TIME_PREFIX "speed"            ///< http://lv2plug.in/ns/ext/time#speed

// clang-format on

/**
   @}
*/

#endif // LV2_TIME_TIME_H
