
dofile("../../../scripts/make-project.lua")

package = make_juce_vst_project("TAL-Reverb")

package.includepaths = {
  package.includepaths,
  "." --fake
}

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
