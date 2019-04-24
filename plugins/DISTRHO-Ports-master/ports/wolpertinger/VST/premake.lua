
dofile("../../../scripts/make-project.lua")

package = make_juce_vst_project("Wolpertinger")

package.defines = { package.defines, "BUILDDATE=\"`date +%F`\"", "WOLPVERSION=\"0041\"", "WOLPVERSIONSTRING=\"0.4.1\"" }

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
