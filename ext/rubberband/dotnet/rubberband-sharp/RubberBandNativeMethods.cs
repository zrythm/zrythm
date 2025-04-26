/*
    rubberband-sharp

    A consumer of the rubberband DLL that wraps the RubberBandStretcher
    object exposed by the DLL in a managed .NET type.

    Copyright 2018-2019 Jonathan Gilbert

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy,
    modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    Except as contained in this notice, the name of Jonathan Gilbert
    shall not be used in advertising or otherwise to promote the sale,
    use or other dealings in this Software without prior written
    authorization.
*/

using System;
using System.Runtime.InteropServices;

namespace RubberBand
{
	internal class RubberBandNativeMethods
	{
		public static IntPtr RubberBandStretcher_Create(IntPtr sampleRate, IntPtr channels, int options = 0, double initialTimeRatio = 1.0, double initialPitchScale = 1.0)
		{
			if (IntPtr.Size == 8)
				return RubberBandNativeMethodsx64.RubberBandStretcher_Create(sampleRate, channels, options, initialTimeRatio, initialPitchScale);
			else
				return RubberBandNativeMethodsWin32.RubberBandStretcher_Create(sampleRate, channels, options, initialTimeRatio, initialPitchScale);
		}

		public static void RubberBandStretcher_Delete(IntPtr rbs)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_Delete(rbs);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_Delete(rbs);
		}

		public static void RubberBandStretcher_Reset(IntPtr rbs)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_Reset(rbs);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_Reset(rbs);
		}

		public static void RubberBandStretcher_SetTimeRatio(IntPtr rbs, double ratio)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_SetTimeRatio(rbs, ratio);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_SetTimeRatio(rbs, ratio);
		}

		public static void RubberBandStretcher_SetPitchScale(IntPtr rbs, double scale)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_SetPitchScale(rbs, scale);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_SetPitchScale(rbs, scale);
		}

		public static double RubberBandStretcher_GetTimeRatio(IntPtr rbs)
		{
			if (IntPtr.Size == 8)
				return RubberBandNativeMethodsx64.RubberBandStretcher_GetTimeRatio(rbs);
			else
				return RubberBandNativeMethodsWin32.RubberBandStretcher_GetTimeRatio(rbs);
		}

		public static double RubberBandStretcher_GetPitchScale(IntPtr rbs)
		{
			if (IntPtr.Size == 8)
				return RubberBandNativeMethodsx64.RubberBandStretcher_GetPitchScale(rbs);
			else
				return RubberBandNativeMethodsWin32.RubberBandStretcher_GetPitchScale(rbs);
		}

		public static IntPtr RubberBandStretcher_GetLatency(IntPtr rbs)
		{
			if (IntPtr.Size == 8)
				return RubberBandNativeMethodsx64.RubberBandStretcher_GetLatency(rbs);
			else
				return RubberBandNativeMethodsWin32.RubberBandStretcher_GetLatency(rbs);
		}

		public static void RubberBandStretcher_SetTransientsOption(IntPtr rbs, int options)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_SetTransientsOption(rbs, options);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_SetTransientsOption(rbs, options);
		}

		public static void RubberBandStretcher_SetDetectorOption(IntPtr rbs, int options)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_SetDetectorOption(rbs, options);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_SetDetectorOption(rbs, options);
		}

		public static void RubberBandStretcher_SetPhaseOption(IntPtr rbs, int options)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_SetPhaseOption(rbs, options);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_SetPhaseOption(rbs, options);
		}

		public static void RubberBandStretcher_SetFormantOption(IntPtr rbs, int options)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_SetFormantOption(rbs, options);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_SetFormantOption(rbs, options);
		}

		public static void RubberBandStretcher_SetPitchOption(IntPtr rbs, int options)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_SetPitchOption(rbs, options);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_SetPitchOption(rbs, options);
		}

		public static void RubberBandStretcher_SetExpectedInputDuration(IntPtr rbs, IntPtr samples)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_SetExpectedInputDuration(rbs, samples);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_SetExpectedInputDuration(rbs, samples);
		}

		public static void RubberBandStretcher_SetMaxProcessSize(IntPtr rbs, IntPtr samples)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_SetMaxProcessSize(rbs, samples);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_SetMaxProcessSize(rbs, samples);
		}

		public static IntPtr RubberBandStretcher_GetSamplesRequired(IntPtr rbs)
		{
			if (IntPtr.Size == 8)
				return RubberBandNativeMethodsx64.RubberBandStretcher_GetSamplesRequired(rbs);
			else
				return RubberBandNativeMethodsWin32.RubberBandStretcher_GetSamplesRequired(rbs);
		}

		public static void RubberBandStretcher_SetKeyFrameMap(IntPtr rbs, IntPtr[] mappingData, int numberOfMappings)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_SetKeyFrameMap(rbs, mappingData, numberOfMappings);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_SetKeyFrameMap(rbs, mappingData, numberOfMappings);
		}

		public static void RubberBandStretcher_Study(IntPtr rbs, float[] input, IntPtr samples, int channels, bool final)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_Study(rbs, input, samples, channels, final);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_Study(rbs, input, samples, channels, final);
		}

		public static void RubberBandStretcher_Process(IntPtr rbs, float[] input, IntPtr samples, int channels, bool final)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_Process(rbs, input, samples, channels, final);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_Process(rbs, input, samples, channels, final);
		}

		public static int RubberBandStretcher_Available(IntPtr rbs)
		{
			if (IntPtr.Size == 8)
				return RubberBandNativeMethodsx64.RubberBandStretcher_Available(rbs);
			else
				return RubberBandNativeMethodsWin32.RubberBandStretcher_Available(rbs);
		}

		public static IntPtr RubberBandStretcher_Retrieve(IntPtr rbs, float[] output, IntPtr samples, int channels)
		{
			if (IntPtr.Size == 8)
				return RubberBandNativeMethodsx64.RubberBandStretcher_Retrieve(rbs, output, samples, channels);
			else
				return RubberBandNativeMethodsWin32.RubberBandStretcher_Retrieve(rbs, output, samples, channels);
		}

		public static float RubberBandStretcher_GetFrequencyCutoff(IntPtr rbs, int n)
		{
			if (IntPtr.Size == 8)
				return RubberBandNativeMethodsx64.RubberBandStretcher_GetFrequencyCutoff(rbs, n);
			else
				return RubberBandNativeMethodsWin32.RubberBandStretcher_GetFrequencyCutoff(rbs, n);
		}

		public static void RubberBandStretcher_SetFrequencyCutoff(IntPtr rbs, int n, float f)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_SetFrequencyCutoff(rbs, n, f);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_SetFrequencyCutoff(rbs, n, f);
		}

		public static IntPtr RubberBandStretcher_GetInputIncrement(IntPtr rbs)
		{
			if (IntPtr.Size == 8)
				return RubberBandNativeMethodsx64.RubberBandStretcher_GetInputIncrement(rbs);
			else
				return RubberBandNativeMethodsWin32.RubberBandStretcher_GetInputIncrement(rbs);
		}

		public static IntPtr RubberBandStretcher_GetOutputIncrements(IntPtr rbs, int[] buffer, IntPtr bufferSize)
		{
			if (IntPtr.Size == 8)
				return RubberBandNativeMethodsx64.RubberBandStretcher_GetOutputIncrements(rbs, buffer, bufferSize);
			else
				return RubberBandNativeMethodsWin32.RubberBandStretcher_GetOutputIncrements(rbs, buffer, bufferSize);
		}

		public static IntPtr RubberBandStretcher_GetPhaseResetCurve(IntPtr rbs, float[] buffer, IntPtr bufferSize)
		{
			if (IntPtr.Size == 8)
				return RubberBandNativeMethodsx64.RubberBandStretcher_GetPhaseResetCurve(rbs, buffer, bufferSize);
			else
				return RubberBandNativeMethodsWin32.RubberBandStretcher_GetPhaseResetCurve(rbs, buffer, bufferSize);
		}

		public static IntPtr RubberBandStretcher_GetExactTimePoints(IntPtr rbs, int[] buffer, IntPtr bufferSize)
		{
			if (IntPtr.Size == 8)
				return RubberBandNativeMethodsx64.RubberBandStretcher_GetExactTimePoints(rbs, buffer, bufferSize);
			else
				return RubberBandNativeMethodsWin32.RubberBandStretcher_GetExactTimePoints(rbs, buffer, bufferSize);
		}

		public static IntPtr RubberBandStretcher_GetChannelCount(IntPtr rbs)
		{
			if (IntPtr.Size == 8)
				return RubberBandNativeMethodsx64.RubberBandStretcher_GetChannelCount(rbs);
			else
				return RubberBandNativeMethodsWin32.RubberBandStretcher_GetChannelCount(rbs);
		}

		public static void RubberBandStretcher_CalculateStretch(IntPtr rbs)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_CalculateStretch(rbs);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_CalculateStretch(rbs);
		}

		public static void RubberBandStretcher_SetDebugLevel(IntPtr rbs, int level)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_SetDebugLevel(rbs, level);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_SetDebugLevel(rbs, level);
		}

		public static void RubberBandStretcher_SetDefaultDebugLevel(int level)
		{
			if (IntPtr.Size == 8)
				RubberBandNativeMethodsx64.RubberBandStretcher_SetDefaultDebugLevel(level);
			else
				RubberBandNativeMethodsWin32.RubberBandStretcher_SetDefaultDebugLevel(level);
		}
	}
}
