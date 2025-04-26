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
	internal class RubberBandNativeMethodsWin32
	{
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern IntPtr RubberBandStretcher_Create(IntPtr sampleRate, IntPtr channels, int options = 0, double initialTimeRatio = 1.0, double initialPitchScale = 1.0);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_Delete(IntPtr rbs);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_Reset(IntPtr rbs);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_SetTimeRatio(IntPtr rbs, double ratio);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_SetPitchScale(IntPtr rbs, double scale);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern double RubberBandStretcher_GetTimeRatio(IntPtr rbs);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern double RubberBandStretcher_GetPitchScale(IntPtr rbs);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern IntPtr RubberBandStretcher_GetLatency(IntPtr rbs);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_SetTransientsOption(IntPtr rbs, int options);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_SetDetectorOption(IntPtr rbs, int options);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_SetPhaseOption(IntPtr rbs, int options);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_SetFormantOption(IntPtr rbs, int options);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_SetPitchOption(IntPtr rbs, int options);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_SetExpectedInputDuration(IntPtr rbs, IntPtr samples);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_SetMaxProcessSize(IntPtr rbs, IntPtr samples);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern IntPtr RubberBandStretcher_GetSamplesRequired(IntPtr rbs);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_SetKeyFrameMap(IntPtr rbs, IntPtr[] mappingData, int numberOfMappings);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_Study(IntPtr rbs, float[] input, IntPtr samples, int channels, bool final);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_Process(IntPtr rbs, float[] input, IntPtr samples, int channels, bool final);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern int RubberBandStretcher_Available(IntPtr rbs);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern IntPtr RubberBandStretcher_Retrieve(IntPtr rbs, float[] output, IntPtr samples, int channels);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern float RubberBandStretcher_GetFrequencyCutoff(IntPtr rbs, int n);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_SetFrequencyCutoff(IntPtr rbs, int n, float f);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern IntPtr RubberBandStretcher_GetInputIncrement(IntPtr rbs);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern IntPtr RubberBandStretcher_GetOutputIncrements(IntPtr rbs, int[] buffer, IntPtr bufferSize);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern IntPtr RubberBandStretcher_GetPhaseResetCurve(IntPtr rbs, float[] buffer, IntPtr bufferSize);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern IntPtr RubberBandStretcher_GetExactTimePoints(IntPtr rbs, int[] buffer, IntPtr bufferSize);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern IntPtr RubberBandStretcher_GetChannelCount(IntPtr rbs);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_CalculateStretch(IntPtr rbs);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_SetDebugLevel(IntPtr rbs, int level);
		[DllImport("rubberband-dll-Win32", CallingConvention = CallingConvention.Cdecl)]
		public static extern void RubberBandStretcher_SetDefaultDebugLevel(int level);
	}
}
