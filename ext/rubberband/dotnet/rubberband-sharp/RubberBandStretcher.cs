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
using System.Collections.Generic;

namespace RubberBand
{
	public class RubberBandStretcher : IDisposable
	{
		public enum Options
		{
			None = 0,

			ProcessOffline = 0x00000000,
			ProcessRealTime = 0x00000001,

			StretchElastic = 0x00000000,
			StretchPrecise = 0x00000010,

			TransientsCrisp = 0x00000000,
			TransientsMixed = 0x00000100,
			TransientsSmooth = 0x00000200,

			DetectorCompound = 0x00000000,
			DetectorPercussive = 0x00000400,
			DetectorSoft = 0x00000800,

			PhaseLaminar = 0x00000000,
			PhaseIndependent = 0x00002000,

			ThreadingAuto = 0x00000000,
			ThreadingNever = 0x00010000,
			ThreadingAlways = 0x00020000,

			WindowStandard = 0x00000000,
			WindowShort = 0x00100000,
			WindowLong = 0x00200000,

			SmoothingOff = 0x00000000,
			SmoothingOn = 0x00800000,

			FormantShifted = 0x00000000,
			FormantPreserved = 0x01000000,

			PitchHighSpeed = 0x00000000,
			PitchHighQuality = 0x02000000,
			PitchHighConsistency = 0x04000000,

			ChannelsApart = 0x00000000,
			ChannelsTogether = 0x10000000,
		}

		public const Options DefaultOptions = Options.None;
		public const Options PercussiveOptions = Options.WindowShort | Options.PhaseIndependent;

		IntPtr _rbs;

		public RubberBandStretcher(int sampleRate, int channels, Options options = DefaultOptions, double initialTimeRatio = 1.0, double initialPitchScale = 1.0)
		{
			_rbs = RubberBandNativeMethods.RubberBandStretcher_Create(new IntPtr(sampleRate), new IntPtr(channels), (int)options, initialTimeRatio, initialPitchScale);
		}

		public void Dispose()
		{
			if (_rbs != IntPtr.Zero)
			{
				RubberBandNativeMethods.RubberBandStretcher_Delete(_rbs);
				_rbs = IntPtr.Zero;
			}
		}

		public void Reset()
		{
			RubberBandNativeMethods.RubberBandStretcher_Reset(_rbs);
		}

		public void SetTimeRatio(double ratio)
		{
			RubberBandNativeMethods.RubberBandStretcher_SetTimeRatio(_rbs, ratio);
		}

		public void SetPitchScale(double scale)
		{
			RubberBandNativeMethods.RubberBandStretcher_SetPitchScale(_rbs, scale);
		}

		public double GetTimeRatio()
		{
			return RubberBandNativeMethods.RubberBandStretcher_GetTimeRatio(_rbs);
		}

		public double GetPitchScale()
		{
			return RubberBandNativeMethods.RubberBandStretcher_GetPitchScale(_rbs);
		}

		public long GetLatency()
		{
			return RubberBandNativeMethods.RubberBandStretcher_GetLatency(_rbs).ToInt64();
		}

		public void SetTransientsOption(Options options)
		{
			RubberBandNativeMethods.RubberBandStretcher_SetTransientsOption(_rbs, (int)options);
		}

		public void SetDetectorOption(Options options)
		{
			RubberBandNativeMethods.RubberBandStretcher_SetDetectorOption(_rbs, (int)options);
		}

		public void SetPhaseOption(Options options)
		{
			RubberBandNativeMethods.RubberBandStretcher_SetPhaseOption(_rbs, (int)options);
		}

		public void SetFormantOption(Options options)
		{
			RubberBandNativeMethods.RubberBandStretcher_SetFormantOption(_rbs, (int)options);
		}

		public void SetPitchOption(Options options)
		{
			RubberBandNativeMethods.RubberBandStretcher_SetPitchOption(_rbs, (int)options);
		}

		public void SetExpectedInputDuration(long samples)
		{
			RubberBandNativeMethods.RubberBandStretcher_SetExpectedInputDuration(_rbs, new IntPtr(samples));
		}

		public void SetMaxProcessSize(long samples)
		{
			RubberBandNativeMethods.RubberBandStretcher_SetMaxProcessSize(_rbs, new IntPtr(samples));
		}

		public long GetSamplesRequired()
		{
			return RubberBandNativeMethods.RubberBandStretcher_GetSamplesRequired(_rbs).ToInt64();
		}

		public void SetKeyFrameMap(SortedDictionary<long, long> map)
		{
			var mappingData = new List<IntPtr>();

			foreach (var mapping in map)
			{
				mappingData.Add(new IntPtr(mapping.Key));
				mappingData.Add(new IntPtr(mapping.Value));
			}

			RubberBandNativeMethods.RubberBandStretcher_SetKeyFrameMap(_rbs, mappingData.ToArray(), mappingData.Count / 2);
		}

		float[] _studyBuffer = new float[1024];

		public void Study(float[][] input, bool final)
		{
			for (int i = 1; i < input.Length; i++)
				if (input[i].Length != input[0].Length)
					throw new Exception("Invalid input data: Not all channels have the same number of samples");

			Study(input, input[0].Length, final);
		}

		public void Study(float[][] input, long samples, bool final)
		{
			for (int i = 0; i < input.Length; i++)
				if (input[i].Length < samples)
					throw new Exception("Invalid input data: Channel " + i + " does not have " + samples + " samples of data");

			long totalSamples = input.Length * samples;

			if (totalSamples > _studyBuffer.Length)
				_studyBuffer = new float[totalSamples];

			for (int i = 0; i < input.Length; i++)
				Array.Copy(input[i], 0, _studyBuffer, i * samples, samples);

			RubberBandNativeMethods.RubberBandStretcher_Study(_rbs, _studyBuffer, new IntPtr(samples), input.Length, final);
		}

		float[] _processBuffer = new float[1024];

		public void Process(float[][] input, bool final)
		{
			for (int i = 1; i < input.Length; i++)
				if (input[i].Length != input[0].Length)
					throw new Exception("Invalid input data: Not all channels have the same number of samples");

			Process(input, input[0].Length, final);
		}

		public void Process(float[][] input, long samples, bool final)
		{
			for (int i = 0; i < input.Length; i++)
				if (input[i].Length < samples)
					throw new Exception("Invalid input data: Channel " + i + " does not have " + samples + " samples of data");

			long totalSamples = input.Length * samples;

			if (totalSamples > _processBuffer.Length)
				_processBuffer = new float[totalSamples];

			for (int i = 0; i < input.Length; i++)
				Array.Copy(input[i], 0, _processBuffer, i * samples, samples);

			RubberBandNativeMethods.RubberBandStretcher_Process(_rbs, _processBuffer, new IntPtr(samples), input.Length, final);
		}

		public int Available()
		{
			return RubberBandNativeMethods.RubberBandStretcher_Available(_rbs);
		}

		float[] _outputBuffer = new float[1024];

		public long Retrieve(float[][] output)
		{
			for (int i = 1; i < output.Length; i++)
				if (output[i].Length != output[0].Length)
					throw new Exception("Invalid output buffer: Not all channels have the same number of samples");

			return Retrieve(output, output[0].Length);
		}

		public long Retrieve(float[][] output, long samples)
		{
			for (int i = 0; i < output.Length; i++)
				if (output[i].Length < samples)
					throw new Exception("Invalid output buffer: Channel " + i + " does not have enough space for " + samples + " samples");

			long totalSamples = output.Length * samples;

			if (totalSamples > _outputBuffer.Length)
				_outputBuffer = new float[totalSamples];

			long actualSamples = RubberBandNativeMethods.RubberBandStretcher_Retrieve(_rbs, _outputBuffer, new IntPtr(samples), output.Length).ToInt64();

			for (int i = 0; i < output.Length; i++)
				Array.Copy(_outputBuffer, i * samples, output[i], 0, actualSamples);

			return actualSamples;
		}

		public float GetFrequencyCutoff(int n)
		{
			return RubberBandNativeMethods.RubberBandStretcher_GetFrequencyCutoff(_rbs, n);
		}

		public void SetFrequencyCutoff(int n, float f)
		{
			RubberBandNativeMethods.RubberBandStretcher_SetFrequencyCutoff(_rbs, n, f);
		}

		public long GetInputIncrement()
		{
			return RubberBandNativeMethods.RubberBandStretcher_GetInputIncrement(_rbs).ToInt64();
		}

		public int[] GetOutputIncrements()
		{
			long numberOfIncrements = RubberBandNativeMethods.RubberBandStretcher_GetOutputIncrements(_rbs, null, IntPtr.Zero).ToInt64();

			int[] buffer = new int[numberOfIncrements];

			RubberBandNativeMethods.RubberBandStretcher_GetOutputIncrements(_rbs, buffer, new IntPtr(buffer.Length));

			return buffer;
		}

		public float[] GetPhaseResetCurve()
		{
			long numberOfCurveElements = RubberBandNativeMethods.RubberBandStretcher_GetPhaseResetCurve(_rbs, null, IntPtr.Zero).ToInt64();

			float[] buffer = new float[numberOfCurveElements];

			RubberBandNativeMethods.RubberBandStretcher_GetPhaseResetCurve(_rbs, buffer, new IntPtr(buffer.Length));

			return buffer;
		}

		public int[] GetExactTimePoints()
		{
			long numberOfTimePoints = RubberBandNativeMethods.RubberBandStretcher_GetExactTimePoints(_rbs, null, IntPtr.Zero).ToInt64();

			int[] buffer = new int[numberOfTimePoints];

			RubberBandNativeMethods.RubberBandStretcher_GetExactTimePoints(_rbs, buffer, new IntPtr(buffer.Length));

			return buffer;
		}

		public long GetChannelCount()
		{
			return RubberBandNativeMethods.RubberBandStretcher_GetChannelCount(_rbs).ToInt64();
		}

		public void CalculateStretch()
		{
			RubberBandNativeMethods.RubberBandStretcher_CalculateStretch(_rbs);
		}

		public void SetDebugLevel(int level)
		{
			RubberBandNativeMethods.RubberBandStretcher_SetDebugLevel(_rbs, level);
		}

		public static void SetDefaultDebugLevel(int level)
		{
			RubberBandNativeMethods.RubberBandStretcher_SetDefaultDebugLevel(level);
		}
	}
}
