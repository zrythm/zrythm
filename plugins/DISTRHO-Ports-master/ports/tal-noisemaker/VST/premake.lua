
dofile("../../../scripts/make-project.lua")

package = make_juce_vst_project("TAL-NoiseMaker")

package.includepaths = {
  package.includepaths,
  "../source/Engine",
  ".", --fake
  "./intermediate", --fake
  "./intermediate/Release", --fake
  "./intermediate/Debug" --fake
}

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/Engine/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
