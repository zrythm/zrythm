
dofile("../../../scripts/make-project.lua")

package = make_juce_lv2_project("Dexed")

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/msfa/*.cc",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
