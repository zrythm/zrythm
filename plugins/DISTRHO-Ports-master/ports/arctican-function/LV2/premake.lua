
dofile("../../../scripts/make-project.lua")

package = make_juce_lv2_project("TheFunction")

if (os.getenv("LINUX_EMBED")) then
package.files = {
  matchfiles (
    "../source/PluginProcessor.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
else
package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/Headers/UI/Knob.cpp",
    "../source/Headers/UI/FancyButton.cpp",
    "../source/Headers/Binary Data/Backgrounds/thefunctionbackground.cpp",
    "../source/Headers/Binary Data/UI/button.cpp",
    "../source/Headers/Binary Data/UI/knobs.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
end
