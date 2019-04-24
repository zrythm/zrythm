
dofile("../../../scripts/make-project.lua")

package = make_juce_lv2_project("PitchedDelay")

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/dsp/*.cpp",
    "../source/gui/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
