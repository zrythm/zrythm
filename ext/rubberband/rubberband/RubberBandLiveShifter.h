/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2024 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for the
    Rubber Band Live Pitch Shifter obtained by agreement with the
    copyright holders, you may redistribute and/or modify it under the
    terms described in that licence.

    If you wish to distribute code using Rubber Band Live under terms
    other than those of the GNU General Public License, you must
    obtain a valid commercial licence before doing so.
*/

#ifndef RUBBERBAND_LIVE_SHIFTER_H
#define RUBBERBAND_LIVE_SHIFTER_H

#define RUBBERBAND_VERSION "4.0.0"
#define RUBBERBAND_API_MAJOR_VERSION 3
#define RUBBERBAND_API_MINOR_VERSION 0

#undef RUBBERBAND_LIVE_DLLEXPORT
#ifdef _MSC_VER
#define RUBBERBAND_LIVE_DLLEXPORT __declspec(dllexport)
#else
#define RUBBERBAND_LIVE_DLLEXPORT
#endif

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cstddef>

namespace RubberBand
{

/**
 * ### Summary
 * 
 * RubberBand::RubberBandLiveShifter is an interface to the Rubber
 * Band Library designed for applications that need to perform
 * pitch-shifting only, without time-stretching, and to do so in a
 * straightforward block-by-block process with the shortest available
 * processing delay. For the more general interface, see
 * RubberBand::RubberBandStretcher.
 *
 * RubberBandLiveShifter has a much simpler API than
 * RubberBandStretcher. Its process function, called
 * RubberBandLiveShifter::shift(), accepts a fixed number of sample
 * frames on each call and always returns exactly the same number of
 * sample frames. This is in contrast to the
 * process/available/retrieve call sequence that RubberBandStretcher
 * requires as a result of its variable output rate.
 *
 * The number of frames accepted by and returned from
 * RubberBandLiveShifter::shift() are not under the caller's control:
 * they must always be exactly the number given by
 * RubberBandLiveShifter::getBlockSize(). But this number is fixed for
 * the lifetime of the shifter, so it only needs to be queried once
 * after construction and then fixed-size buffers may be used.
 *
 * Using RubberBandLiveShifter also gives a shorter processing delay
 * than a typical buffering setup using RubberBandStretcher, making it
 * a useful choice for some streamed or live situations. However, it
 * is still not a low-latency effect, with a delay of 50ms or more
 * between input and output signals depending on configuration. (The
 * actual value may be queried via
 * RubberBandLiveShifter::getStartDelay().) The shifter is real-time
 * safe in the sense of avoiding allocation, locking, or blocking
 * operations in the processing path.
 *
 * ### Thread safety
 * 
 * Multiple instances of RubberBandLiveShifter may be created and used
 * in separate threads concurrently.  However, for any single instance
 * of RubberBandLiveShifter, you may not call
 * RubberBandLiveShifter::shift() more than once concurrently, and you
 * may not change the pitch scaling ratio using
 * RubberBandLiveShifter::setPitchScale() while a
 * RubberBandLiveShifter::shift() call is being executed. Changing the
 * ratio is real-time safe, so when the pitch ratio is time-varying,
 * it is normal to update the ratio before each shift call.
 */
class RUBBERBAND_LIVE_DLLEXPORT
RubberBandLiveShifter
{
public:
    enum Option {
        OptionWindowShort          = 0x00000000,
        OptionWindowMedium         = 0x00100000,

        OptionFormantShifted       = 0x00000000,
        OptionFormantPreserved     = 0x01000000,

        OptionChannelsApart        = 0x00000000,
        OptionChannelsTogether     = 0x10000000

        // n.b. Options is int, so we must stop before 0x80000000
    };

    /**
     * A bitwise OR of values from the RubberBandLiveShifter::Option
     * enum.
     */
    typedef int Options;

    enum PresetOption {
        DefaultOptions = 0x00000000
    };

    /**
     * Interface for log callbacks that may optionally be provided to
     * the shifter on construction.
     *
     * If a Logger is provided, the shifter will call one of these
     * functions instead of sending output to \c cerr when there is
     * something to report. This allows debug output to be diverted to
     * an application's logging facilities, and/or handled in an
     * RT-safe way. See setDebugLevel() for details about how and when
     * RubberBandLiveShifter reports something in this way.
     *
     * The message text passed to each of these log functions is a
     * C-style string with no particular guaranteed lifespan. If you
     * need to retain it, copy it before returning. Do not free it.
     *
     * @see setDebugLevel
     * @see setDefaultDebugLevel
     */
    struct Logger {
        /// Receive a log message with no numeric values.
        virtual void log(const char *) = 0;

        /// Receive a log message and one accompanying numeric value.
        virtual void log(const char *, double) = 0;

        /// Receive a log message and two accompanying numeric values.
        virtual void log(const char *, double, double) = 0;
        
        virtual ~Logger() { }
    };
    
    /**
     * Construct a pitch shifter object to run at the given sample
     * rate, with the given number of channels.
     */
    RubberBandLiveShifter(size_t sampleRate, size_t channels,
                          Options options);

    /**
     * Construct a pitch shifter object with a custom debug
     * logger. This may be useful for debugging if the default logger
     * output (which simply goes to \c cerr) is not visible in the
     * runtime environment, or if the application has a standard or
     * more realtime-appropriate logging mechanism.
     *
     * See the documentation for the other constructor above for
     * details of the arguments other than the logger.
     *
     * Note that although the supplied logger gets to decide what to
     * do with log messages, the separately-set debug level (see
     * setDebugLevel() and setDefaultDebugLevel()) still determines
     * whether any given debug message is sent to the logger in the
     * first place.
     */
    RubberBandLiveShifter(size_t sampleRate, size_t channels,
                          std::shared_ptr<Logger> logger,
                          Options options);
    
    ~RubberBandLiveShifter();

    /**
     * Reset the shifter's internal buffers.  The shifter should
     * subsequently behave as if it had just been constructed
     * (although retaining the current pitch ratio).
     */
    void reset();

    /**
     * Set the pitch scaling ratio for the shifter.  This is the ratio
     * of target frequency to source frequency.  For example, a ratio
     * of 2.0 would shift up by one octave; 0.5 down by one octave; or
     * 1.0 leave the pitch unaffected.
     *
     * To put this in musical terms, a pitch scaling ratio
     * corresponding to a shift of S equal-tempered semitones (where S
     * is positive for an upwards shift and negative for downwards) is
     * pow(2.0, S / 12.0).
     *
     * This function may be called at any time, so long as it is not
     * called concurrently with shift().  You should either call this
     * function from the same thread as shift(), or provide your own
     * mutex or similar mechanism to ensure that setPitchScale and
     * shift() cannot be run at once (there is no internal mutex for
     * this purpose).
     */
    void setPitchScale(double scale);

    /**
     * Set a pitch scale for the vocal formant envelope separately
     * from the overall pitch scale.  This is a ratio of target
     * frequency to source frequency.  For example, a ratio of 2.0
     * would shift the formant envelope up by one octave; 0.5 down by
     * one octave; or 1.0 leave the formant unaffected.
     *
     * By default this is set to the special value of 0.0, which
     * causes the scale to be calculated automatically. It will be
     * treated as 1.0 / the pitch scale if OptionFormantPreserved is
     * specified, or 1.0 for OptionFormantShifted.
     *
     * Conversely, if this is set to a value other than the default
     * 0.0, formant shifting will happen regardless of the state of
     * the OptionFormantPreserved/OptionFormantShifted option.
     *
     * This function is provided for special effects only. You do not
     * need to call it for ordinary pitch shifting, with or without
     * formant preservation - just specify or omit the
     * OptionFormantPreserved option as appropriate. Use this function
     * only if you want to shift formants by a distance other than
     * that of the overall pitch shift.
     */
    void setFormantScale(double scale);

    /**
     * Return the last pitch scaling ratio value that was set (either
     * on construction or with setPitchScale()).
     */
    double getPitchScale() const;

    /**
     * Return the last formant scaling ratio that was set with
     * setFormantScale, or 0.0 if the default automatic scaling is in
     * effect.
     */     
    double getFormantScale() const;
    
    /**
     * Return the output delay of the shifter.  This is the number of
     * audio samples that one should discard at the start of the
     * output, in order to ensure that the resulting audio has the
     * expected time alignment with the input.
     *
     * Ensure you have set the pitch scale to its proper starting
     * value before calling getStartDelay().
     */
    size_t getStartDelay() const;
    
    /**
     * Return the number of channels this shifter was constructed
     * with.
     */
    size_t getChannelCount() const;

    /**
     * Change an OptionFormant configuration setting.  This may be
     * called at any time in any mode.
     *
     * Note that if running multi-threaded in Offline mode, the change
     * may not take effect immediately if processing is already under
     * way when this function is called.
     */
    void setFormantOption(Options options);

    /**
     * Query the number of sample frames that must be passed to, and
     * will be returned by, each shift() call. This value is fixed for
     * the lifetime of the shifter.
     *
     * Note that the blocksize refers to the number of audio sample
     * frames, which may be multi-channel, not the number of
     * individual samples.
     */
    size_t getBlockSize() const;
    
    /**
     * Pitch-shift a single block of sample frames. The number of
     * sample frames (samples per channel) processed per call is
     * constant.
     *
     * "input" should point to de-interleaved audio data with one
     * float array per channel, with each array containing n samples
     * where n is the value returned by getBlockSize().
     * 
     * "output" should point to a float array per channel, with each
     * array having enough room to store n samples where n is the value
     * returned by getBlockSize().
     *
     * The input and output must be separate arrays; they cannot alias
     * one another or overlap.
     *
     * Sample values are conventionally expected to be in the range
     * -1.0f to +1.0f.
     */
    void shift(const float *const *input, float *const *output);

    /**
     * Set the level of debug output.  The supported values are:
     *
     * 0. Report errors only.
     * 
     * 1. Report some information on construction and ratio
     * change. Nothing is reported during normal processing unless
     * something changes.
     * 
     * 2. Report a significant amount of information about ongoing
     * calculations during normal processing.
     *
     * The default is whatever has been set using
     * setDefaultDebugLevel(), or 0 if that function has not been
     * called.
     *
     * All output goes to \c cerr unless a custom
     * RubberBandLiveShifter::Logger has been provided on
     * construction. Because writing to \c cerr is not RT-safe, only
     * debug level 0 is RT-safe in normal use by default. Debug levels
     * 0 and 1 use only C-string constants as debug messages, so they
     * are RT-safe if your custom logger is RT-safe. Levels 2 and 3
     * are not guaranteed to be RT-safe in any conditions as they may
     * construct messages by allocation.
     *
     * @see Logger
     * @see setDefaultDebugLevel
     */
    void setDebugLevel(int level);

    /**
     * Set the default level of debug output for subsequently
     * constructed shifters.
     *
     * @see setDebugLevel
     */
    static void setDefaultDebugLevel(int level);

protected:
    class Impl;
    Impl *m_d;

    RubberBandLiveShifter(const RubberBandLiveShifter &) =delete;
    RubberBandLiveShifter &operator=(const RubberBandLiveShifter &) =delete;
};

}

#endif
