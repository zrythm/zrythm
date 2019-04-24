
dofile("../../../scripts/make-project.lua")

package = make_juce_lv2_project("Vex")

if (os.getenv("LINUX_EMBED")) then
package.files = {
  matchfiles (
    "../source/Vex-src.cpp",
    "../source/VexFilter.cpp",
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
