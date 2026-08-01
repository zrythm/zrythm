# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.build import check_min_cppstd
from conan.tools.files import load
import json
import os


class Zrythm(ConanFile):
    name = "zrythm"
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps"
    package_type = "application"
    options = {
        "sanitizer": [
            "none",
            "address",
            "undefined",
            "address_undefined",
            "thread",
            "thread_undefined",
            "realtime",
            "memory",
        ],
    }
    default_options = {"sanitizer": "none"}

    def set_version(self):
        self.version = load(self, "VERSION.txt").lstrip("v").rstrip()

    def requirements(self):
        self.requires("qt/[>=6.11.1]")
        self.requires("magic_enum/0.9.7")
        self.requires("fmt/[~12]")
        self.requires("spdlog/[~1.17]")
        self.requires("scnlib/4.0.1")
        self.requires("boost/[>=1.89.0]")
        self.requires("nlohmann_json/3.12.1")
        self.requires("json-schema-validator/2.4.0")
        self.requires("libsndfile/1.2.2")
        self.requires("zstd/[~1.5]")
        self.requires("type_safe/[>=0.2.4]")
        self.requires("gsl-lite/1.1.0")
        self.requires("au/[~0.5]")
        self.requires("onetbb/2023.0.0")
        self.requires("tracy/[>=0.13.1 <1]")
        self.requires("xxhash/0.8.3")
        if self.settings.os == "Linux":
            self.requires("freetype/[>=2.14]")
            self.requires("fontconfig/[>=2.15]")

    def build_requirements(self):
        self.tool_requires("cmake/[>=4.3]")
        self.test_requires("gtest/[~1.16]")
        self.test_requires("benchmark/[>=1.9.5]")
        if self.settings_build.os == "Linux":
            self.tool_requires("ninja/[>=1]")

    def configure(self):
        # NOTE: the 8 enabled Qt modules below (qtdeclarative, qtquickcontrols2,
        # qttools, qttranslations, qtcanvaspainter, qtlanguageserver, qtsvg,
        # qtshadertools) and with_pq/with_odbc are MIRRORED in
        # conan/profiles/_qt_common. Conan 2 does not propagate these
        # consumer options to build-context tool_requires, so _qt_common must
        # set them too. Keep the two in sync when changing either.
        self.options["qt"].shared = True
        self.options["qt"].with_glib = False
        self.options["harfbuzz"].with_glib = False
        self.options["qt"].with_harfbuzz = True
        self.options["qt"].with_libpng = True
        self.options["qt"].with_libjpeg = "libjpeg-turbo"
        self.options["qt"].with_doubleconversion = True
        self.options["qt"].with_md4c = True
        self.options["qt"].with_icu = False
        self.options["qt"].with_brotli = False
        self.options["qt"].with_sqlite3 = False
        self.options["qt"].with_pq = False
        self.options["qt"].with_odbc = False
        self.options["qt"].opengl = "desktop"

        # Modules enabled in manual builds
        self.options["qt"].qtdeclarative = True
        self.options["qt"].qtquickcontrols2 = True
        self.options["qt"].qttools = True
        self.options["qt"].qttranslations = True
        self.options["qt"].qtcanvaspainter = True
        self.options["qt"].qtlanguageserver = True
        self.options["qt"].qtsvg = True
        self.options["qt"].qtshadertools = True

        # Modules skipped in manual builds (-skip ...)
        self.options["qt"].qtgrpc = False
        self.options["qt"].qtdoc = False
        self.options["qt"].qtwebengine = False
        self.options["qt"].qtconnectivity = False
        self.options["qt"].qtsensors = False
        self.options["qt"].qtserialbus = False
        self.options["qt"].qtserialport = False
        self.options["qt"].qtlocation = False
        self.options["qt"].qtpositioning = False
        self.options["qt"].qtmqtt = False
        self.options["qt"].qtremoteobjects = False
        self.options["qt"].qtopcua = False
        self.options["qt"].qt5compat = False
        self.options["qt"].qtactiveqt = False
        self.options["qt"].qtcoap = False
        self.options["qt"].qtquick3d = False
        self.options["qt"].qtquick3dphysics = False

        if self.settings.os == "Linux":
            self.options["qt"].with_dbus = True
            self.options["qt"].with_freetype = True
            self.options["qt"].with_fontconfig = True
            self.options["qt"].qtwayland = True
            self.options["qt"].with_egl = True
            self.options["qt"].with_x11 = True
            # Only build the wayland-scanner tool in Conan; the system provides
            # the wayland libraries at build time (via pkg-config) and runtime.
            # Building Conan copies causes them to shadow the system's
            # libwayland-egl.so at runtime, breaking EGL in the Qt wayland plugin.
            self.options["wayland"].enable_libraries = False
        elif self.settings.os == "Macos":
            self.options["qt"].openssl = False
            self.options["qt"].with_dbus = False
            self.options["qt"].with_fontconfig = False
            # Enables the offscreen QPA plugin for headless QML tests
            self.options["qt"].with_freetype = True
            self.options["qt"].qtwayland = False
        elif self.settings.os == "Windows":
            self.options["qt"].openssl = False
            self.options["qt"].with_dbus = False
            self.options["qt"].with_fontconfig = False
            # Enables the offscreen QPA plugin for headless QML tests
            self.options["qt"].with_freetype = True
            self.options["qt"].qtwayland = False

        self.options["boost"].header_only = True

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def layout(self):
        cmake_layout(self, build_folder="conanbuild")

    def generate(self):
        tc = CMakeToolchain(self)

        san_opt = str(self.options.get_safe("sanitizer", "none"))
        profile_san = self.settings.get_safe("compiler.sanitizer") or ""
        if profile_san:
            if "Address" in profile_san and "UndefinedBehavior" in profile_san:
                san_opt = "address_undefined"
            elif "Thread" in profile_san and "UndefinedBehavior" in profile_san:
                san_opt = "thread_undefined"
            elif "Thread" in profile_san:
                san_opt = "thread"
            elif "Address" in profile_san:
                san_opt = "address"
            elif "UndefinedBehavior" in profile_san:
                san_opt = "undefined"
            elif "Realtime" in profile_san:
                san_opt = "realtime"
            elif "Memory" in profile_san:
                san_opt = "memory"

        parts = san_opt.split("_")
        sans = set()
        if "address" in parts:
            sans.add("address")
        if "undefined" in parts:
            sans.add("undefined")
        if "thread" in parts:
            sans.add("thread")
        if "realtime" in parts:
            sans.add("realtime")
        if "memory" in parts:
            sans.add("memory")

        tc.cache_variables["ZRYTHM_ENABLE_SANITIZER_ADDRESS"] = "ON" if "address" in sans else "OFF"
        tc.cache_variables["ZRYTHM_ENABLE_SANITIZER_UNDEFINED_BEHAVIOR"] = "ON" if "undefined" in sans else "OFF"
        tc.cache_variables["ZRYTHM_ENABLE_SANITIZER_THREAD"] = "ON" if "thread" in sans else "OFF"
        tc.cache_variables["ZRYTHM_ENABLE_SANITIZER_MEMORY"] = "ON" if "memory" in sans else "OFF"
        tc.cache_variables["ZRYTHM_ENABLE_SANITIZER_REALTIME"] = "ON" if "realtime" in sans else "OFF"
        tc.generate()

        build_rel = os.path.relpath(self.build_folder, self.source_folder)
        gen_rel = os.path.relpath(self.generators_folder, self.source_folder)

        conan_presets_path = os.path.join(self.generators_folder, "CMakePresets.json")
        conan_preset_name = None
        if os.path.exists(conan_presets_path):
            with open(conan_presets_path) as f:
                for p in json.load(f).get("configurePresets", []):
                    conan_preset_name = p["name"]
                    break

        if conan_preset_name:
            bt = str(self.settings.build_type)
            cache_vars = {"CMAKE_BUILD_TYPE": bt}
            if bt in ("RelWithDebInfo", "Release"):
                cache_vars["ZRYTHM_EXTRA_OPTIMIZATIONS"] = "ON"
            if bt == "Release":
                cache_vars["ZRYTHM_UNITY_BUILD"] = "ON"

            # Consumed by the "Qt" (qt-cpp) VS Code extension to locate the Conan-built Qt
            # for design tools (Qt Creator integration, qmlls language server, etc.).
            qt_pkg = self.dependencies["qt"].package_folder
            qt_vendor = {
                "qt-cpp": {
                    "VSCODE_QT_INSTALLATION": qt_pkg,
                    "VSCODE_QT_QTPATHS_EXE": os.path.join(qt_pkg, "bin", "qtpaths"),
                }
            }

            user_presets = {
                "version": 5,
                "include": [
                    "CMakePresets.json",
                    f"{gen_rel}/CMakePresets.json",
                ],
                "configurePresets": [
                    {
                        "name": "default",
                        "displayName": f"{bt} (Conan)",
                        "inherits": ["_base", conan_preset_name],
                        "binaryDir": f"${{sourceDir}}/{build_rel}",
                        "toolchainFile": f"${{sourceDir}}/{gen_rel}/conan_toolchain.cmake",
                        "cacheVariables": cache_vars,
                        "vendor": qt_vendor,
                    }
                ],
                "buildPresets": [
                    {"name": "default", "configurePreset": "default"}
                ],
            }

            user_presets_path = os.path.join(self.source_folder,
                                             "CMakeUserPresets.json")
            with open(user_presets_path, "w") as f:
                f.write(json.dumps(user_presets, indent=2))
            self.output.info("CMakeUserPresets.json generated with Conan environment")

    def validate(self):
        check_min_cppstd(self, "23")
