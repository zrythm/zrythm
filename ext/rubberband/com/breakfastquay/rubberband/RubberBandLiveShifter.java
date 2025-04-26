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

public class RubberBandLiveShifter
{
    public RubberBandLiveShifter(int sampleRate, int channels,
                                 int options) {
	handle = 0;
	initialise(sampleRate, channels, options);
    }

    public native void dispose();

    public native void reset();

    public native void setPitchScale(double scale);

    public native int getChannelCount();
    public native double getPitchScale();

    public native int getStartDelay();

    public native void setFormantOption(int options);

    public native int getBlockSize();
    
    public native void shift(float[][] input, int inOffset, float[][] output, int outOffset);
    public void shift(float[][] input, float[][] output) {
        shift(input, 0, output, 0);
    }

    private native void initialise(int sampleRate, int channels, int options);
    private long handle;

    public static final int OptionWindowShort          = 0x00000000;
    public static final int OptionWindowMedium         = 0x00100000;

    public static final int OptionFormantShifted       = 0x00000000;
    public static final int OptionFormantPreserved     = 0x01000000;

    public static final int OptionChannelsApart        = 0x00000000;
    public static final int OptionChannelsTogether     = 0x10000000;

    static {
	System.loadLibrary("rubberband-jni");
    }
};

