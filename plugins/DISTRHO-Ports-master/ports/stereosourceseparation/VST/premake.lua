
dofile("../../../scripts/make-project.lua")

package = make_juce_vst_project("StereoSourceSeparation")

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/kiss_fft/*.c",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}

package.includepaths = {
  package.includepaths,
  "../source/kiss_fft"
}
