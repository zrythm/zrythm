
dofile("../../../scripts/make-project.lua")

package = make_juce_lv2_project("Luftikus")

if (os.getenv("LINUX_EMBED")) then
package.files = {
  matchfiles (
    "../source/PluginProcessor.cpp",
    "../source/dsp/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
else
package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/dsp/*.cpp",
    "../source/gui2/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
end
