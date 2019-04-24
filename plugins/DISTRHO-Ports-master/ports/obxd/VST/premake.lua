
dofile("../../../scripts/make-project.lua")

package = make_juce_vst_project("Obxd")

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/Gui/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
