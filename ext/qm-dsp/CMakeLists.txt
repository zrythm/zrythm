# SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_library(qm_dsp_lib STATIC
  dsp/onsets/DetectionFunction.cpp
  dsp/onsets/PeakPicking.cpp
  dsp/phasevocoder/PhaseVocoder.cpp
  dsp/signalconditioning/DFProcess.cpp
  dsp/signalconditioning/Filter.cpp
  dsp/signalconditioning/FiltFilt.cpp
  dsp/signalconditioning/Framer.cpp
  dsp/tempotracking/TempoTrack.cpp
  dsp/tempotracking/TempoTrackV2.cpp
  dsp/transforms/FFT.cpp
  ext/kissfft/qm_kiss_fft.c
  ext/kissfft/tools/qm_kiss_fftr.c
  maths/Correlation.cpp
  maths/MathUtilities.cpp
)

target_include_directories(qm_dsp_lib 
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/kissfft
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/kissfft/tools
)

target_compile_definitions(qm_dsp_lib PRIVATE
  NDEBUG
  qm_kiss_fft_scalar=double
  USE_PTHREADS
)

target_compile_options(qm_dsp_lib PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
    -O3
    -ftree-vectorize
  >
  $<$<CXX_COMPILER_ID:MSVC>:
    /O2
  >
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
  target_compile_options(qm_dsp_lib PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
      -msse
      -msse2
      -mfpmath=sse
    >
    $<$<CXX_COMPILER_ID:MSVC>:
      /arch:SSE2
    >
  )
endif()

set_target_properties(qm_dsp_lib PROPERTIES UNITY_BUILD OFF)

# set_target_properties(qm_dsp_lib PROPERTIES
#   POSITION_INDEPENDENT_CODE ON
# )
