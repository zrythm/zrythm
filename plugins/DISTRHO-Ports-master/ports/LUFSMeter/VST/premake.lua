
dofile("../../../scripts/make-project.lua")

package = make_juce_vst_project("LUFSMeter")

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/filters/*.cpp",
    "../source/gui/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
