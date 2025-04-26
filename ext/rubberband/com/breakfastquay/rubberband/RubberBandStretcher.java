/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2022 Particular Programs Ltd.

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

package com.breakfastquay.rubberband;

import java.util.Map;
import java.util.Set;

public class RubberBandStretcher
{
    public RubberBandStretcher(int sampleRate, int channels,
			       int options,
			       double initialTimeRatio,
			       double initialPitchScale) {
	handle = 0;
	initialise(sampleRate, channels, options,
		   initialTimeRatio, initialPitchScale);
    }

    public native void dispose();

    public native void reset();

    public native void setTimeRatio(double ratio);
    public native void setPitchScale(double scale);

    public native int getChannelCount();
    public native double getTimeRatio();
    public native double getPitchScale();

    public native int getPreferredStartPad();
    public native int getStartDelay();
    public native int getLatency();

    public native void setTransientsOption(int options);
    public native void setDetectorOption(int options);
    public native void setPhaseOption(int options);
    public native void setFormantOption(int options);
    public native void setPitchOption(int options);

    public native void setExpectedInputDuration(long samples);
    public native int getProcessSizeLimit();
    public native void setMaxProcessSize(int samples);

    public native int getSamplesRequired();

    public native void setKeyFrameMap(long[] from, long[] to);
    public void setKeyFrameMap(Map<Long, Long> m) {
        Set<Long> keys = m.keySet();
        int n = keys.size();
        long[] from = new long[n];
        long[] to = new long[n];
        int i = 0;
        for (Long k : keys) {
            from[i] = k.longValue();
            to[i] = m.get(k).longValue();
            ++i;
        }
        setKeyFrameMap(from, to);
    }

    public native void study(float[][] input, int offset, int n, boolean finalBlock);
    public void study(float[][] input, boolean finalBlock) {
	study(input, 0, input[0].length, finalBlock);
    }

    public native void process(float[][] input, int offset, int n, boolean finalBlock);
    public void process(float[][] input, boolean finalBlock) {
	process(input, 0, input[0].length, finalBlock);
    }

    public native int available();

    public native int retrieve(float[][] output, int offset, int n);
    public int retrieve(float[][] output) {
	return retrieve(output, 0, output[0].length);
    }

    private native void initialise(int sampleRate, int channels, int options,
				   double initialTimeRatio,
				   double initialPitchScale);
    private long handle;

    public static final int OptionProcessOffline       = 0x00000000;
    public static final int OptionProcessRealTime      = 0x00000001;

    public static final int OptionStretchElastic       = 0x00000000;
    public static final int OptionStretchPrecise       = 0x00000010;
    
    public static final int OptionTransientsCrisp      = 0x00000000;
    public static final int OptionTransientsMixed      = 0x00000100;
    public static final int OptionTransientsSmooth     = 0x00000200;

    public static final int OptionDetectorCompound     = 0x00000000;
    public static final int OptionDetectorPercussive   = 0x00000400;
    public static final int OptionDetectorSoft         = 0x00000800;

    public static final int OptionPhaseLaminar         = 0x00000000;
    public static final int OptionPhaseIndependent     = 0x00002000;
    
    public static final int OptionThreadingAuto        = 0x00000000;
    public static final int OptionThreadingNever       = 0x00010000;
    public static final int OptionThreadingAlways      = 0x00020000;

    public static final int OptionWindowStandard       = 0x00000000;
    public static final int OptionWindowShort          = 0x00100000;
    public static final int OptionWindowLong           = 0x00200000;

    public static final int OptionSmoothingOff         = 0x00000000;
    public static final int OptionSmoothingOn          = 0x00800000;

    public static final int OptionFormantShifted       = 0x00000000;
    public static final int OptionFormantPreserved     = 0x01000000;

    public static final int OptionPitchHighSpeed       = 0x00000000;
    public static final int OptionPitchHighQuality     = 0x02000000;
    public static final int OptionPitchHighConsistency = 0x04000000;

    public static final int OptionChannelsApart        = 0x00000000;
    public static final int OptionChannelsTogether     = 0x10000000;

    public static final int OptionEngineFaster         = 0x00000000;
    public static final int OptionEngineFiner          = 0x20000000;

    public static final int DefaultOptions             = 0x00000000;
    public static final int PercussiveOptions          = 0x00102000;

    static {
	System.loadLibrary("rubberband-jni");
    }
};

