# Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
#
# This file is part of Zrythm
#
# Zrythm is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Zrythm is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.

qm_dsp_inc = include_directories ('.')

qm_dsp_lib_c_args = [
  '-DNDEBUG',
  '-Dkiss_fft_scalar=double',
  '-Wall',
  '-Wextra',
  '-O3',
  '-ftree-vectorize',
  '-DUSE_PTHREADS',
  ]
if host_machine.cpu() == 'x86_64'
  qm_dsp_lib_c_args += [
    '-msse',
    '-msse2',
    '-mfpmath=sse',
    ]
endif

qm_dsp_lib = static_library (
  'qm_dsp',
  'dsp/onsets/DetectionFunction.cpp',
  'dsp/onsets/PeakPicking.cpp',
  'dsp/phasevocoder/PhaseVocoder.cpp',
  'dsp/signalconditioning/DFProcess.cpp',
  'dsp/signalconditioning/Filter.cpp',
  'dsp/signalconditioning/FiltFilt.cpp',
  'dsp/signalconditioning/Framer.cpp',
  'dsp/tempotracking/TempoTrack.cpp',
  'dsp/tempotracking/TempoTrackV2.cpp',
  'dsp/transforms/FFT.cpp',
  'ext/kissfft/kiss_fft.c',
  'ext/kissfft/tools/kiss_fftr.c',
  'maths/Correlation.cpp',
  'maths/MathUtilities.cpp',
  c_args: qm_dsp_lib_c_args,
  cpp_args: [
    '-Dkiss_fft_scalar=double',
    ],
  pic: true,
  include_directories: [
    qm_dsp_inc,
    include_directories ('ext/kissfft'),
    include_directories ('ext/kissfft/tools'),
    ],
  )
