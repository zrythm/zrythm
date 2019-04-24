
dofile("../../../scripts/make-project.lua")

package = make_juce_lv2_project("TAL-Dub-3")

package.includepaths = {
  package.includepaths,
  "." --fake
}

if (os.getenv("LINUX_EMBED")) then
package.files = {
  matchfiles (
    "../source/TalCore.cpp",
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
