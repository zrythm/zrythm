
dofile("../../../scripts/make-project.lua")

package = make_juce_vst_project("TAL-Vocoder-2")

package.includepaths = {
  package.includepaths,
  "../source/engine",
  "../source/engine/synth",
  "." --fake
}

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/engine/vocoder/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
