audio_decoder_srcs = [
  'ad_soundfile.c',
  'ad_ffmpeg.c',
  'ad_plugin.c',
  ]

audio_decoder_inc = [
  ext_inc,
  root_inc,
  ]
audio_decoder = static_library (
  'audio-decoder',
  sources: audio_decoder_srcs,
  dependencies: sndfile_dep,
  include_directories: audio_decoder_inc,
)
