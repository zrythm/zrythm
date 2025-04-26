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
    Rubber Band Library obtained by agreement with the copyright
    holders, you may redistribute and/or modify it under the terms
    described in that licence.

    If you wish to distribute code using the Rubber Band Library
    under terms other than those of the GNU General Public License,
    you must obtain a valid commercial licence before doing so.
*/

#ifndef RUBBERBAND_STRETCHER_H
#define RUBBERBAND_STRETCHER_H
    
#define RUBBERBAND_VERSION "4.0.0"
#define RUBBERBAND_API_MAJOR_VERSION 3
#define RUBBERBAND_API_MINOR_VERSION 0

#undef RUBBERBAND_DLLEXPORT
#ifdef _MSC_VER
#ifndef RUBBERBAND_STATIC
#define RUBBERBAND_DLLEXPORT __declspec(dllexport)
#else
#define RUBBERBAND_DLLEXPORT
#endif
#else
#define RUBBERBAND_DLLEXPORT
#endif

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cstddef>

namespace RubberBand
{

/**
 * @mainpage RubberBand
 *
 * ### Summary
 * 
 * The primary Rubber Band Library API is contained in the class
 * RubberBand::RubberBandStretcher. This class can perform both pitch
 * shifting and time stretching and supports every feature of the
 * library. A simpler, more limited API that supports only
 * pitch-shifting can be found in RubberBand::RubberBandLiveShifter.
 *
 * RubberBandStretcher supports two processing modes, offline and
 * real-time, and two processing "engines", known as the R2 or Faster
 * engine and the R3 or Finer engine. The choices of processing mode
 * and engine are fixed on construction: see the constructor
 * RubberBandStretcher::RubberBandStretcher. The two engines work
 * identically in API terms, and both of them support both offline and
 * real-time modes as described below.
 *
 * ### Offline mode
 *
 * In offline mode, you must provide the audio block-by-block in
 * two passes. In the first pass, call RubberBandStretcher::study() on
 * each block; in the second pass, call RubberBandStretcher::process()
 * on each block and receive the output via
 * RubberBandStretcher::retrieve().
 *
 * In offline mode, the time and pitch ratios are fixed as soon as the
 * study pass has begun and cannot be changed afterwards. (But see
 * RubberBandStretcher::setKeyFrameMap() for a way to do pre-planned
 * variable time stretching in offline mode.) Offline mode also
 * performs padding and delay compensation so that the stretched
 * result has an exact start and duration.
 *
 * ### Real-time mode
 *
 * In \b real-time mode, there is no study pass, just a single
 * streaming pass in which the audio is passed to
 * RubberBandStretcher::process() and output received via
 * RubberBandStretcher::retrieve().
 *
 * In real-time mode you can change the time and pitch ratios at any
 * time.
 *
 * You may need to perform signal padding and delay compensation in
 * real-time mode; see RubberBandStretcher::getPreferredStartPad() and
 * RubberBandStretcher::getStartDelay() for details.
 *
 * Rubber Band Library is RT-safe when used in real-time mode with
 * "normal" processing parameters. That is, it performs no allocation,
 * locking, or blocking operations after initialisation during normal
 * use, even when the time and pitch ratios change. There are certain
 * exceptions that include error states and extremely rapid changes
 * between extreme ratios, as well as the case in which more frames
 * are passed to RubberBandStretcher::process() than the values
 * returned by RubberBandStretcher::getSamplesRequired() or set using
 * RubberBandStretcher::setMaxProcessSize(), when buffer reallocation
 * may occur. See the latter function's documentation for
 * details. Note that offline mode is never RT-safe.
 *
 * ### Thread safety
 * 
 * Multiple instances of RubberBandStretcher may be created and used
 * in separate threads concurrently.  However, for any single instance
 * of RubberBandStretcher, you may not call
 * RubberBandStretcher::process() more than once concurrently, and you
 * may not change the time or pitch ratio while a
 * RubberBandStretcher::process() call is being executed (if the
 * stretcher was created in "real-time mode"; in "offline mode" you
 * can't change the ratios during use anyway).
 * 
 * So you can run RubberBandStretcher::process() in its own thread if
 * you like, but if you want to change ratios dynamically from a
 * different thread, you will need some form of mutex in your code.
 * Changing the time or pitch ratio is real-time safe except in
 * extreme circumstances, so for most applications that may change
 * these dynamically it probably makes most sense to do so from the
 * same thread as calls RubberBandStretcher::process(), even if that
 * is a real-time thread.
 */
class RUBBERBAND_DLLEXPORT
RubberBandStretcher
{
public:
    /**
     * Processing options for the timestretcher.  The preferred
     * options should normally be set in the constructor, as a bitwise
     * OR of the option flags.  The default value (DefaultOptions) is
     * intended to give good results in most situations.
     *
     * 1. Flags prefixed \c OptionProcess determine how the timestretcher
     * will be invoked.  These options may not be changed after
     * construction.
     * 
     *   \li \c OptionProcessOffline - Run the stretcher in offline
     *   mode.  In this mode the input data needs to be provided
     *   twice, once to study(), which calculates a stretch profile
     *   for the audio, and once to process(), which stretches it.
     *
     *   \li \c OptionProcessRealTime - Run the stretcher in real-time
     *   mode.  In this mode only process() should be called, and the
     *   stretcher adjusts dynamically in response to the input audio.
     * 
     * The Process setting is likely to depend on your architecture:
     * non-real-time operation on seekable files: Offline; real-time
     * or streaming operation: RealTime.
     *
     * 2. Flags prefixed \c OptionEngine select the core Rubber Band
     * processing engine to be used. These options may not be changed
     * after construction.
     *
     *   \li \c OptionEngineFaster - Use the Rubber Band Library R2
     *   (Faster) engine. This is the engine implemented in Rubber
     *   Band Library v1.x and v2.x, and it remains the default in
     *   newer versions. It uses substantially less CPU than the R3
     *   engine and there are still many situations in which it is
     *   likely to be the more appropriate choice.
     *
     *   \li \c OptionEngineFiner - Use the Rubber Band Library R3
     *   (Finer) engine. This engine was introduced in Rubber Band
     *   Library v3.0. It produces higher-quality results than the R2
     *   engine for most material, especially complex mixes, vocals
     *   and other sounds that have soft onsets and smooth pitch
     *   changes, and music with substantial bass content. However, it
     *   uses much more CPU power than the R2 engine.
     *
     *   Important note: Consider calling getEngineVersion() after
     *   construction to make sure the engine you requested is
     *   active. That's not because engine selection can fail, but
     *   because Rubber Band Library ignores any unknown options
     *   supplied on construction - so a program that requests the R3
     *   engine but ends up linked against an older version of the
     *   library (prior to v3.0) will silently use the R2 engine
     *   instead. Calling the v3.0 function getEngineVersion() will
     *   ensure a link failure in this situation instead, and supply a
     *   reassuring run-time check.
     *
     * 3. Flags prefixed \c OptionTransients control the component
     * frequency phase-reset mechanism in the R2 engine, that may be
     * used at transient points to provide clarity and realism to
     * percussion and other significant transient sounds. These
     * options have no effect when using the R3 engine.  These options
     * may be changed after construction when running in real-time
     * mode, but not when running in offline mode.
     * 
     *   \li \c OptionTransientsCrisp - Reset component phases at the
     *   peak of each transient (the start of a significant note or
     *   percussive event).  This, the default setting, usually
     *   results in a clear-sounding output; but it is not always
     *   consistent, and may cause interruptions in stable sounds
     *   present at the same time as transient events.  The
     *   OptionDetector flags (below) can be used to tune this to some
     *   extent.
     *
     *   \li \c OptionTransientsMixed - Reset component phases at the
     *   peak of each transient, outside a frequency range typical of
     *   musical fundamental frequencies.  The results may be more
     *   regular for mixed stable and percussive notes than
     *   \c OptionTransientsCrisp, but with a "phasier" sound.  The
     *   balance may sound very good for certain types of music and
     *   fairly bad for others.
     *
     *   \li \c OptionTransientsSmooth - Do not reset component phases
     *   at any point.  The results will be smoother and more regular
     *   but may be less clear than with either of the other
     *   transients flags.
     *
     * 4. Flags prefixed \c OptionDetector control the type of
     * transient detector used in the R2 engine.  These options have
     * no effect when using the R3 engine.  These options may be
     * changed after construction when running in real-time mode, but
     * not when running in offline mode.
     *
     *   \li \c OptionDetectorCompound - Use a general-purpose
     *   transient detector which is likely to be good for most
     *   situations.  This is the default.
     *
     *   \li \c OptionDetectorPercussive - Detect percussive
     *   transients.  Note that this was the default and only option
     *   in Rubber Band versions prior to 1.5.
     *
     *   \li \c OptionDetectorSoft - Use an onset detector with less
     *   of a bias toward percussive transients.  This may give better
     *   results with certain material (e.g. relatively monophonic
     *   piano music).
     *
     * 5. Flags prefixed \c OptionPhase control the adjustment of
     * component frequency phases in the R2 engine from one analysis
     * window to the next during non-transient segments.  These
     * options have no effect when using the R3 engine. These options
     * may be changed at any time.
     *
     *   \li \c OptionPhaseLaminar - Adjust phases when stretching in
     *   such a way as to try to retain the continuity of phase
     *   relationships between adjacent frequency bins whose phases
     *   are behaving in similar ways.  This, the default setting,
     *   should give good results in most situations.
     *
     *   \li \c OptionPhaseIndependent - Adjust the phase in each
     *   frequency bin independently from its neighbours.  This
     *   usually results in a slightly softer, phasier sound.
     *
     * 6. Flags prefixed \c OptionThreading control the threading
     * model of the stretcher.  These options may not be changed after
     * construction.
     *
     *   \li \c OptionThreadingAuto - Permit the stretcher to
     *   determine its own threading model.  In the R2 engine this
     *   means using one processing thread per audio channel in
     *   offline mode if the stretcher is able to determine that more
     *   than one CPU is available, and one thread only in realtime
     *   mode.  The R3 engine does not currently have a multi-threaded
     *   mode, but if one is introduced in future, this option may use
     *   it. This is the default.
     *
     *   \li \c OptionThreadingNever - Never use more than one thread.
     *  
     *   \li \c OptionThreadingAlways - Use multiple threads in any
     *   situation where \c OptionThreadingAuto would do so, except omit
     *   the check for multiple CPUs and instead assume it to be true.
     *
     * 7. Flags prefixed \c OptionWindow influence the window size for
     * FFT processing. In the R2 engine these affect the resulting
     * sound quality but have relatively little effect on processing
     * speed. With the R3 engine they can dramatically affect
     * processing speed as well as output quality. These options may
     * not be changed after construction.
     *
     *   \li \c OptionWindowStandard - Use the default window size.
     *   The actual size will vary depending on other parameters.
     *   This option is expected to produce better results than the
     *   other window options in most situations. In the R3 engine
     *   this causes the engine's full multi-resolution processing
     *   scheme to be used.
     *
     *   \li \c OptionWindowShort - Use a shorter window. This has
     *   different effects with R2 and R3 engines.
     *
     *   With the R2 engine it may result in crisper sound for audio
     *   that depends strongly on its timing qualities, but is likely
     *   to sound worse in other ways and will have similar
     *   efficiency.
     *
     *   With the R3 engine, it causes the engine to be restricted to
     *   a single window size, resulting in both dramatically faster
     *   processing and lower delay than OptionWindowStandard, but at
     *   the expense of some sound quality. It may still sound better
     *   for non-percussive material than the R2 engine.
     *
     *   With both engines it reduces the start delay somewhat (see
     *   RubberBandStretcher::getStartDelay) which may be useful for
     *   real-time handling.
     *
     *   \li \c OptionWindowLong - Use a longer window. With the R2
     *   engine this is likely to result in a smoother sound at the
     *   expense of clarity and timing. The R3 engine currently
     *   ignores this option, treating it like OptionWindowStandard.
     *
     * 8. Flags prefixed \c OptionSmoothing control the use of
     * window-presum FFT and time-domain smoothing in the R2
     * engine. These options have no effect when using the R3 engine.
     * These options may not be changed after construction.
     *
     *   \li \c OptionSmoothingOff - Do not use time-domain smoothing.
     *   This is the default.
     *
     *   \li \c OptionSmoothingOn - Use time-domain smoothing.  This
     *   will result in a softer sound with some audible artifacts
     *   around sharp transients, but it may be appropriate for longer
     *   stretches of some instruments and can mix well with
     *   OptionWindowShort.
     *
     * 9. Flags prefixed \c OptionFormant control the handling of
     * formant shape (spectral envelope) when pitch-shifting. These
     * options affect both the R2 and R3 engines.  These options may
     * be changed at any time.
     *
     *   \li \c OptionFormantShifted - Apply no special formant
     *   processing.  The spectral envelope will be pitch shifted as
     *   normal.  This is the default.
     *
     *   \li \c OptionFormantPreserved - Preserve the spectral
     *   envelope of the unshifted signal.  This permits shifting the
     *   note frequency without so substantially affecting the
     *   perceived pitch profile of the voice or instrument.
     *
     * 10. Flags prefixed \c OptionPitch control the method used for
     * pitch shifting. These options affect only realtime mode. In
     * offline mode the method is not adjustable. In the R2 engine
     * these options may be changed at any time; in the R3 engine they
     * may be set only on construction.
     *
     *   \li \c OptionPitchHighSpeed - Favour CPU cost over sound
     *   quality. This is the default. Use this when time-stretching
     *   only, or for fixed pitch shifts where CPU usage is of
     *   concern. Do not use this for arbitrarily time-varying pitch
     *   shifts (see OptionPitchHighConsistency below).
     *
     *   \li \c OptionPitchHighQuality - Favour sound quality over CPU
     *   cost. Use this for fixed pitch shifts where sound quality is
     *   of most concern. Do not use this for arbitrarily time-varying
     *   pitch shifts (see OptionPitchHighConsistency below).

     *   \li \c OptionPitchHighConsistency - Use a method that
     *   supports dynamic pitch changes without discontinuities,
     *   including when crossing the 1.0 pitch scale. This may cost
     *   more in CPU than the default, especially when the pitch scale
     *   is exactly 1.0. You should use this option whenever you wish
     *   to support dynamically changing pitch shift during
     *   processing.
     *
     * 11. Flags prefixed \c OptionChannels control the method used
     * for processing two-channel stereo audio. These options may not
     * be changed after construction.
     *
     *   \li \c OptionChannelsApart - Channels are handled for maximum
     *   individual fidelity, at the expense of synchronisation. In
     *   the R3 engine, this means frequency-bin synchronisation is
     *   maintained more closely for lower-frequency content than
     *   higher.  In R2, it means the stereo channels are processed
     *   individually and only synchronised at transients.  In both
     *   engines this gives the highest quality for the individual
     *   channels but a more diffuse stereo image, an unnatural
     *   increase in "width", and generally a loss of mono
     *   compatibility (i.e. mono mixes from stereo can sound phasy).
     *   This option is the default.
     *
     *   \li \c OptionChannelsTogether - Channels are handled for
     *   higher synchronisation at some expense of individual
     *   fidelity. In particular, a stretcher processing two channels
     *   will treat its input as a stereo pair and aim to maximise
     *   clarity at the centre and preserve mono compatibility.  This
     *   gives relatively less stereo space and width than the
     *   default, as well as slightly lower fidelity for individual
     *   channel content, but the results may be more appropriate for
     *   many situations making use of stereo mixes.
     *
     * Finally, flags prefixed \c OptionStretch are obsolete flags
     * provided for backward compatibility only. They are ignored by
     * the stretcher.
     */
    enum Option {

        OptionProcessOffline       = 0x00000000,
        OptionProcessRealTime      = 0x00000001,

        OptionStretchElastic       = 0x00000000, // obsolete
        OptionStretchPrecise       = 0x00000010, // obsolete
    
        OptionTransientsCrisp      = 0x00000000,
        OptionTransientsMixed      = 0x00000100,
        OptionTransientsSmooth     = 0x00000200,

        OptionDetectorCompound     = 0x00000000,
        OptionDetectorPercussive   = 0x00000400,
        OptionDetectorSoft         = 0x00000800,

        OptionPhaseLaminar         = 0x00000000,
        OptionPhaseIndependent     = 0x00002000,
    
        OptionThreadingAuto        = 0x00000000,
        OptionThreadingNever       = 0x00010000,
        OptionThreadingAlways      = 0x00020000,

        OptionWindowStandard       = 0x00000000,
        OptionWindowShort          = 0x00100000,
        OptionWindowLong           = 0x00200000,

        OptionSmoothingOff         = 0x00000000,
        OptionSmoothingOn          = 0x00800000,

        OptionFormantShifted       = 0x00000000,
        OptionFormantPreserved     = 0x01000000,

        OptionPitchHighSpeed       = 0x00000000,
        OptionPitchHighQuality     = 0x02000000,
        OptionPitchHighConsistency = 0x04000000,

        OptionChannelsApart        = 0x00000000,
        OptionChannelsTogether     = 0x10000000,

        OptionEngineFaster         = 0x00000000,
        OptionEngineFiner          = 0x20000000

        // n.b. Options is int, so we must stop before 0x80000000
    };

    /**
     * A bitwise OR of values from the RubberBandStretcher::Option
     * enum.
     */
    typedef int Options;

    enum PresetOption {
        DefaultOptions             = 0x00000000,
        PercussiveOptions          = 0x00102000
    };

    /**
     * Interface for log callbacks that may optionally be provided to
     * the stretcher on construction.
     *
     * If a Logger is provided, the stretcher will call one of these
     * functions instead of sending output to \c cerr when there is
     * something to report. This allows debug output to be diverted to
     * an application's logging facilities, and/or handled in an
     * RT-safe way. See setDebugLevel() for details about how and when
     * RubberBandStretcher reports something in this way.
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
     * Construct a time and pitch stretcher object to run at the given
     * sample rate, with the given number of channels.
     *
     * Both of the stretcher engines provide their best balance of
     * quality with efficiency at sample rates of 44100 or 48000 Hz.
     * Other rates may be used, and the stretcher should produce
     * sensible output with any rate from 8000 to 192000 Hz, but you
     * are advised to use 44100 or 48000 where practical. Do not use
     * rates below 8000 or above 192000 Hz.
     *
     * Initial time and pitch scaling ratios and other processing
     * options may be provided. In particular, the behaviour of the
     * stretcher depends strongly on whether offline or real-time mode
     * is selected on construction (via OptionProcessOffline or
     * OptionProcessRealTime option - offline is the default).
     * 
     * In offline mode, you must provide the audio block-by-block in
     * two passes: in the first pass calling study(), in the second
     * pass calling process() and receiving the output via
     * retrieve(). In real-time mode, there is no study pass, just a
     * single streaming pass in which the audio is passed to process()
     * and output received via retrieve().
     *
     * In real-time mode you can change the time and pitch ratios at
     * any time, but in offline mode they are fixed and cannot be
     * changed after the study pass has begun. (However, see
     * setKeyFrameMap() for a way to do pre-planned variable time
     * stretching in offline mode.)
     *
     * See the option documentation above for more details.
     */
    RubberBandStretcher(size_t sampleRate,
                        size_t channels,
                        Options options = DefaultOptions,
                        double initialTimeRatio = 1.0,
                        double initialPitchScale = 1.0);

    /**
     * Construct a time and pitch stretcher object with a custom debug
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
    RubberBandStretcher(size_t sampleRate,
                        size_t channels,
                        std::shared_ptr<Logger> logger,
                        Options options = DefaultOptions,
                        double initialTimeRatio = 1.0,
                        double initialPitchScale = 1.0);
    
    ~RubberBandStretcher();

    /**
     * Reset the stretcher's internal buffers.  The stretcher should
     * subsequently behave as if it had just been constructed
     * (although retaining the current time and pitch ratio).
     */
    void reset();

    /**
     * Return the active internal engine version, according to the \c
     * OptionEngine flag supplied on construction. This will return 2
     * for the R2 (Faster) engine or 3 for the R3 (Finer) engine.
     *
     * This function was added in Rubber Band Library v3.0.
     */
    int getEngineVersion() const;
    
    /**
     * Set the time ratio for the stretcher.  This is the ratio of
     * stretched to unstretched duration -- not tempo.  For example, a
     * ratio of 2.0 would make the audio twice as long (i.e. halve the
     * tempo); 0.5 would make it half as long (i.e. double the tempo);
     * 1.0 would leave the duration unaffected.
     *
     * If the stretcher was constructed in Offline mode, the time
     * ratio is fixed throughout operation; this function may be
     * called any number of times between construction (or a call to
     * reset()) and the first call to study() or process(), but may
     * not be called after study() or process() has been called.
     *
     * If the stretcher was constructed in RealTime mode, the time
     * ratio may be varied during operation; this function may be
     * called at any time, so long as it is not called concurrently
     * with process().  You should either call this function from the
     * same thread as process(), or provide your own mutex or similar
     * mechanism to ensure that setTimeRatio and process() cannot be
     * run at once (there is no internal mutex for this purpose).
     */
    void setTimeRatio(double ratio);

    /**
     * Set the pitch scaling ratio for the stretcher.  This is the
     * ratio of target frequency to source frequency.  For example, a
     * ratio of 2.0 would shift up by one octave; 0.5 down by one
     * octave; or 1.0 leave the pitch unaffected.
     *
     * To put this in musical terms, a pitch scaling ratio
     * corresponding to a shift of S equal-tempered semitones (where S
     * is positive for an upwards shift and negative for downwards) is
     * pow(2.0, S / 12.0).
     *
     * If the stretcher was constructed in Offline mode, the pitch
     * scaling ratio is fixed throughout operation; this function may
     * be called any number of times between construction (or a call
     * to reset()) and the first call to study() or process(), but may
     * not be called after study() or process() has been called.
     *
     * If the stretcher was constructed in RealTime mode, the pitch
     * scaling ratio may be varied during operation; this function may
     * be called at any time, so long as it is not called concurrently
     * with process().  You should either call this function from the
     * same thread as process(), or provide your own mutex or similar
     * mechanism to ensure that setPitchScale and process() cannot be
     * run at once (there is no internal mutex for this purpose).
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
     * 
     * This function is supported only in the R3 (OptionEngineFiner)
     * engine. It has no effect in R2 (OptionEngineFaster).
     *
     * This function was added in Rubber Band Library v3.0.
     */
    void setFormantScale(double scale);
    
    /**
     * Return the last time ratio value that was set (either on
     * construction or with setTimeRatio()).
     */
    double getTimeRatio() const;

    /**
     * Return the last pitch scaling ratio value that was set (either
     * on construction or with setPitchScale()).
     */
    double getPitchScale() const;

    /**
     * Return the last formant scaling ratio that was set with
     * setFormantScale, or 0.0 if the default automatic scaling is in
     * effect.
     * 
     * This function is supported only in the R3 (OptionEngineFiner)
     * engine. It always returns 0.0 in R2 (OptionEngineFaster).
     *
     * This function was added in Rubber Band Library v3.0.
     */     
    double getFormantScale() const;

    /**
     * In RealTime mode (unlike in Offline mode) the stretcher
     * performs no automatic padding or delay/latency compensation at
     * the start of the signal. This permits applications to have
     * their own custom requirements, but it also means that by
     * default some samples will be lost or attenuated at the start of
     * the output and the correct linear relationship between input
     * and output sample counts may be lost.
     *
     * Most applications using RealTime mode should solve this by
     * calling getPreferredStartPad() and supplying the returned
     * number of (silent) samples at the start of their input, before
     * their first "true" process() call; and then also calling
     * getStartDelay() and trimming the returned number of samples
     * from the start of their stretcher's output.
     *
     * Ensure you have set the time and pitch scale factors to their
     * proper starting values before calling getRequiredStartPad() or
     * getStartDelay().
     *
     * In Offline mode, padding and delay compensation are handled
     * internally and both functions always return zero.
     *
     * This function was added in Rubber Band Library v3.0.
     *
     * @see getStartDelay
     */
    size_t getPreferredStartPad() const;
    
    /**
     * Return the output delay of the stretcher.  This is the number
     * of audio samples that one should discard at the start of the
     * output, after padding the start of the input with
     * getPreferredStartPad(), in order to ensure that the resulting
     * audio has the expected time alignment with the input.
     *
     * Ensure you have set the time and pitch scale factors to their
     * proper starting values before calling getPreferredStartPad() or
     * getStartDelay().
     *
     * In Offline mode, padding and delay compensation are handled
     * internally and both functions always return zero.
     * 
     * This function was added in Rubber Band Library v3.0. Previously
     * it was called getLatency(). It was renamed to avoid confusion
     * with the number of samples needed at input to cause a block of
     * processing to handle (returned by getSamplesRequired()) which
     * is also sometimes referred to as latency.
     *
     * @see getPreferredStartPad
     */
    size_t getStartDelay() const;

    /**
     * Return the start delay of the stretcher. This is a deprecated
     * alias for getStartDelay().
     *
     * @deprecated
     */
    size_t getLatency() const;
    
    /**
     * Return the number of channels this stretcher was constructed
     * with.
     */
    size_t getChannelCount() const;

    /**
     * Change an OptionTransients configuration setting. This may be
     * called at any time in RealTime mode.  It may not be called in
     * Offline mode (for which the transients option is fixed on
     * construction). This has no effect when using the R3 engine.
     */
    void setTransientsOption(Options options);

    /**
     * Change an OptionDetector configuration setting.  This may be
     * called at any time in RealTime mode.  It may not be called in
     * Offline mode (for which the detector option is fixed on
     * construction). This has no effect when using the R3 engine.
     */
    void setDetectorOption(Options options);

    /**
     * Change an OptionPhase configuration setting.  This may be
     * called at any time in any mode. This has no effect when using
     * the R3 engine.
     *
     * Note that if running multi-threaded in Offline mode, the change
     * may not take effect immediately if processing is already under
     * way when this function is called.
     */
    void setPhaseOption(Options options);

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
     * Change an OptionPitch configuration setting.  This may be
     * called at any time in RealTime mode.  It may not be called in
     * Offline mode (for which the pitch option is fixed on
     * construction). This has no effect when using the R3 engine.
     */
    void setPitchOption(Options options);

    /**
     * Tell the stretcher exactly how many input sample frames it will
     * receive.  This is only useful in Offline mode, when it allows
     * the stretcher to ensure that the number of output samples is
     * exactly correct.  In RealTime mode no such guarantee is
     * possible and this value is ignored.
     *
     * Note that the value of "samples" refers to the number of audio
     * sample frames, which may be multi-channel, not the number of
     * individual samples. (For example, one second of stereo audio
     * sampled at 44100Hz yields a value of 44100 sample frames, not
     * 88200.)  This rule applies throughout the Rubber Band API.
     */
    void setExpectedInputDuration(size_t samples);

    /**
     * Tell the stretcher the maximum number of sample frames that you
     * will ever be passing in to a single process() call.  If you
     * don't call this, the stretcher will assume that you are calling
     * getSamplesRequired() at each cycle and are never passing more
     * samples than are suggested by that function.
     *
     * If your application has some external constraint that means you
     * prefer a fixed block size, then your normal mode of operation
     * would be to provide that block size to this function; to loop
     * calling process() with that size of block; after each call to
     * process(), test whether output has been generated by calling
     * available(); and, if so, call retrieve() to obtain it.  See
     * getSamplesRequired() for a more suitable operating mode for
     * applications without such external constraints.
     *
     * This function may not be called after the first call to study()
     * or process().
     *
     * Note that this value is only relevant to process(), not to
     * study() (to which you may pass any number of samples at a time,
     * and from which there is no output).
     *
     * Despite the existence of this call and its use of a size_t
     * argument, there is an internal limit to the maximum process
     * buffer size that can be requested. Call getProcessSizeLimit()
     * to query that limit. The Rubber Band API is essentially
     * block-based and is not designed to process an entire signal
     * within a single process cycle.
     *
     * Note that the value of "samples" refers to the number of audio
     * sample frames, which may be multi-channel, not the number of
     * individual samples. (For example, one second of stereo audio
     * sampled at 44100Hz yields a value of 44100 sample frames, not
     * 88200.)  This rule applies throughout the Rubber Band API.
     */
    void setMaxProcessSize(size_t samples);

    /**
     * Obtain the overall maximum supported process buffer size in
     * sample frames, which is also the maximum acceptable value to
     * pass to setMaxProcessSize(). This value is fixed across
     * instances and configurations. As of Rubber Band v3.3 it is
     * always 524288 (or 2^19), but in principle it may change in
     * future releases.
     *
     * This function was added in Rubber Band Library v3.3.
     */
    size_t getProcessSizeLimit() const;
    
    /**
     * Ask the stretcher how many audio sample frames should be
     * provided as input in order to ensure that some more output
     * becomes available.
     * 
     * If your application has no particular constraint on processing
     * block size and you are able to provide any block size as input
     * for each cycle, then your normal mode of operation would be to
     * loop querying this function; providing that number of samples
     * to process(); and reading the output (repeatedly if necessary)
     * using available() and retrieve().  See setMaxProcessSize() for
     * a more suitable operating mode for applications that do have
     * external block size constraints.
     *
     * Note that this value is only relevant to process(), not to
     * study() (to which you may pass any number of samples at a time,
     * and from which there is no output).
     *
     * Note that the return value refers to the number of audio sample
     * frames, which may be multi-channel, not the number of
     * individual samples. (For example, one second of stereo audio
     * sampled at 44100Hz yields a value of 44100 sample frames, not
     * 88200.)  This rule applies throughout the Rubber Band API.
     *
     * @see getStartDelay
     */
     size_t getSamplesRequired() const;

    /**
     * Provide a set of mappings from "before" to "after" sample
     * numbers so as to enforce a particular stretch profile.  The
     * argument is a map from audio sample frame number in the source
     * material, to the corresponding sample frame number in the
     * stretched output.  The mapping should be for key frames only,
     * with a "reasonable" gap between mapped samples.
     *
     * This function cannot be used in RealTime mode.
     *
     * This function may not be called after the first call to
     * process().  It should be called after the time and pitch ratios
     * have been set; the results of changing the time and pitch
     * ratios after calling this function are undefined.  Calling
     * reset() will clear this mapping.
     *
     * The key frame map only affects points within the material; it
     * does not determine the overall stretch ratio (that is, the
     * ratio between the output material's duration and the source
     * material's duration).  You need to provide this ratio
     * separately to setTimeRatio(), otherwise the results may be
     * truncated or extended in unexpected ways regardless of the
     * extent of the frame numbers found in the key frame map.
     */
    void setKeyFrameMap(const std::map<size_t, size_t> &);
    
    /**
     * Provide a block of "samples" sample frames for the stretcher to
     * study and calculate a stretch profile from.
     *
     * This is only meaningful in Offline mode, and is required if
     * running in that mode.  You should pass the entire input through
     * study() before any process() calls are made, as a sequence of
     * blocks in individual study() calls, or as a single large block.
     *
     * "input" should point to de-interleaved audio data with one
     * float array per channel. Sample values are conventionally
     * expected to be in the range -1.0f to +1.0f.  "samples" supplies
     * the number of audio sample frames available in "input". If
     * "samples" is zero, "input" may be NULL.
     *
     * Note that the value of "samples" refers to the number of audio
     * sample frames, which may be multi-channel, not the number of
     * individual samples. (For example, one second of stereo audio
     * sampled at 44100Hz yields a value of 44100 sample frames, not
     * 88200.)  This rule applies throughout the Rubber Band API.
     * 
     * Set "final" to true if this is the last block of data that will
     * be provided to study() before the first process() call.
     */
    void study(const float *const *input, size_t samples, bool final);

    /**
     * Provide a block of "samples" sample frames for processing.
     * See also getSamplesRequired() and setMaxProcessSize().
     *
     * "input" should point to de-interleaved audio data with one
     * float array per channel. Sample values are conventionally
     * expected to be in the range -1.0f to +1.0f.  "samples" supplies
     * the number of audio sample frames available in "input".
     *
     * Note that the value of "samples" refers to the number of audio
     * sample frames, which may be multi-channel, not the number of
     * individual samples. (For example, one second of stereo audio
     * sampled at 44100Hz yields a value of 44100 sample frames, not
     * 88200.)  This rule applies throughout the Rubber Band API.
     *
     * Set "final" to true if this is the last block of input data.
     */
    void process(const float *const *input, size_t samples, bool final);

    /**
     * Ask the stretcher how many audio sample frames of output data
     * are available for reading (via retrieve()).
     * 
     * This function returns 0 if no frames are available: this
     * usually means more input data needs to be provided, but if the
     * stretcher is running in threaded mode it may just mean that not
     * enough data has yet been processed.  Call getSamplesRequired()
     * to discover whether more input is needed.
     *
     * Note that the return value refers to the number of audio sample
     * frames, which may be multi-channel, not the number of
     * individual samples. (For example, one second of stereo audio
     * sampled at 44100Hz yields a value of 44100 sample frames, not
     * 88200.)  This rule applies throughout the Rubber Band API.
     *
     * This function returns -1 if all data has been fully processed
     * and all output read, and the stretch process is now finished.
     */
    int available() const;

    /**
     * Obtain some processed output data from the stretcher.  Up to
     * "samples" samples will be stored in each of the output arrays
     * (one per channel for de-interleaved audio data) pointed to by
     * "output".  The number of sample frames available to be
     * retrieved can be queried beforehand with a call to available().
     * The return value is the actual number of sample frames
     * retrieved.
     *
     * Note that the value of "samples" and the return value refer to
     * the number of audio sample frames, which may be multi-channel,
     * not the number of individual samples. (For example, one second
     * of stereo audio sampled at 44100Hz yields a value of 44100
     * sample frames, not 88200.)  This rule applies throughout the
     * Rubber Band API.
     */
    size_t retrieve(float *const *output, size_t samples) const;

    /**
     * Return the value of internal frequency cutoff value n.
     *
     * This function is not for general use and is supported only with
     * the R2 engine.
     */
    float getFrequencyCutoff(int n) const;

    /** 
     * Set the value of internal frequency cutoff n to f Hz.
     *
     * This function is not for general use and is supported only with
     * the R2 engine.
     */
    void setFrequencyCutoff(int n, float f);
    
    /**
     * Retrieve the value of the internal input block increment value.
     *
     * This function is provided for diagnostic purposes only and is
     * supported only with the R2 engine.
     */
    size_t getInputIncrement() const;

    /**
     * In offline mode, retrieve the sequence of internal block
     * increments for output, for the entire audio data, provided the
     * stretch profile has been calculated.  In realtime mode,
     * retrieve any output increments that have accumulated since the
     * last call to getOutputIncrements, to a limit of 16.
     *
     * This function is provided for diagnostic purposes only and is
     * supported only with the R2 engine.
     */
    std::vector<int> getOutputIncrements() const;

    /**
     * In offline mode, retrieve the sequence of internal phase reset
     * detection function values, for the entire audio data, provided
     * the stretch profile has been calculated.  In realtime mode,
     * retrieve any phase reset points that have accumulated since the
     * last call to getPhaseResetCurve, to a limit of 16.
     *
     * This function is provided for diagnostic purposes only and is
     * supported only with the R2 engine.
     */
    std::vector<float> getPhaseResetCurve() const;

    /**
     * In offline mode, retrieve the sequence of internal frames for
     * which exact timing has been sought, for the entire audio data,
     * provided the stretch profile has been calculated.  In realtime
     * mode, return an empty sequence.
     *
     * This function is provided for diagnostic purposes only and is
     * supported only with the R2 engine.
     */
    std::vector<int> getExactTimePoints() const;

    /**
     * Force the stretcher to calculate a stretch profile.  Normally
     * this happens automatically for the first process() call in
     * offline mode.
     *
     * This function is provided for diagnostic purposes only and is
     * supported only with the R2 engine.
     */
    void calculateStretch();

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
     * stretch calculations during normal processing.
     * 
     * 3. Report a large amount of information and also (in the R2
     * engine) add audible ticks to the output at phase reset
     * points. This is seldom useful.
     *
     * The default is whatever has been set using
     * setDefaultDebugLevel(), or 0 if that function has not been
     * called.
     *
     * All output goes to \c cerr unless a custom
     * RubberBandStretcher::Logger has been provided on
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
     * constructed stretchers.
     *
     * @see setDebugLevel
     */
    static void setDefaultDebugLevel(int level);

protected:
    class Impl;
    Impl *m_d;

    RubberBandStretcher(const RubberBandStretcher &) =delete;
    RubberBandStretcher &operator=(const RubberBandStretcher &) =delete;
};

}

#endif
