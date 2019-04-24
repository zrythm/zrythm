
dofile("../../../scripts/make-project.lua")

package = make_juce_lv2_project("Wolpertinger")

package.defines = { package.defines, "BUILDDATE=\"`date +%F`\"", "WOLPVERSION=\"0041\"", "WOLPVERSIONSTRING=\"0.4.1\"" }

if (os.getenv("LINUX_EMBED")) then
package.files = {
  matchfiles (
    "../source/ADSRenv.cpp",
    "../source/synth.cpp",
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
