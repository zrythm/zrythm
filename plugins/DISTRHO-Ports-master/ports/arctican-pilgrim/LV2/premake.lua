
dofile("../../../scripts/make-project.lua")

package = make_juce_lv2_project("ThePilgrim")

if (os.getenv("LINUX_EMBED")) then
package.files = {
  matchfiles (
    "../source/PluginParameter.cpp",
    "../source/PluginProcessor.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
else
package.files = {
  matchfiles (
    "../source/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
end
