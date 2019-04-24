
dofile("../../../scripts/make-project.lua")

package = make_juce_vst_project("Luftikus")

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/dsp/*.cpp",
    "../source/gui2/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
