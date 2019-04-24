
dofile("../../../scripts/make-project.lua")

package = make_juce_vst_project("HiReSam")

package.config["Release"].links = { package.config["Release"].links, "drowaudio" }
package.config["Debug"].links   = { package.config["Debug"].links, "drowaudio_debug" }

package.defines = { package.defines, "USE_DROWAUDIO=1" }

package.includepaths = {
  package.includepaths,
  "../../drowaudio-common"
}

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
