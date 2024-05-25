# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qm_dsp_inc = include_directories ('.')

qm_dsp_lib_c_args = [
  '-DNDEBUG',
  '-Dqm_kiss_fft_scalar=double',
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
  'ext/kissfft/qm_kiss_fft.c',
  'ext/kissfft/tools/qm_kiss_fftr.c',
  'maths/Correlation.cpp',
  'maths/MathUtilities.cpp',
  c_args: qm_dsp_lib_c_args,
  cpp_args: qm_dsp_lib_c_args,
  pic: true,
  include_directories: [
    qm_dsp_inc,
    include_directories ('ext/kissfft'),
    include_directories ('ext/kissfft/tools'),
    ],
  )
