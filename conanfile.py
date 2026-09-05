# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.build import check_min_cppstd
from conan.tools.env import VirtualRunEnv
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
        self.requires("mpmcqueue/1.0")
        # lilv + its lv2/serd/sord/sratom/zix chain for native LV2
        # plugin discovery/hosting
        self.requires("lilv/0.28.0")

        if self.settings.os in ["Linux", "FreeBSD"]:
            self.requires("freetype/[>=2.14]")
            self.requires("fontconfig/[>=2.15]")

    def build_requirements(self):
        self.tool_requires("cmake/[>=4.3]")
        self.test_requires("gtest/[~1.16]")
        self.test_requires("benchmark/[>=1.9.5]")
        if self.settings_build.os == "Linux":
            self.tool_requires("ninja/[>=1]")

    def configure(self):
        # Conan 2 does not propagate consumer options to build-context
        # tool_requires, so the options the build-context qt also needs are
        # mirrored in conan/profiles/_qt_common: the 8 enabled qt modules,
        # with_pq/with_odbc and with_glib. Keep in sync. (with_dbus needs no
        # mirror - our qt recipe forwards it to its self-tool_requires.)
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

    @staticmethod
    def _profile_sanitizer_option(profile_san):
        """Maps a compiler.sanitizer profile setting value to the vocabulary
        of the 'sanitizer' option (empty string if unset)."""
        if not profile_san:
            return ""
        if "Address" in profile_san and "UndefinedBehavior" in profile_san:
            return "address_undefined"
        if "Thread" in profile_san and "UndefinedBehavior" in profile_san:
            return "thread_undefined"
        if "Thread" in profile_san:
            return "thread"
        if "Address" in profile_san:
            return "address"
        if "UndefinedBehavior" in profile_san:
            return "undefined"
        if "Realtime" in profile_san:
            return "realtime"
        if "Memory" in profile_san:
            return "memory"
        return ""

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
        profile_san_opt = self._profile_sanitizer_option(
            self.settings.get_safe("compiler.sanitizer") or "")
        if profile_san_opt:
            san_opt = profile_san_opt

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

        # Sanitizer runtime options (with suppressions) for test runs,
        # composed into the generated conanrun.sh. Profiles cannot provide
        # the absolute suppressions paths (they are installed into the Conan
        # home), so the recipe folder is the single source of truth
        san_run_options = {}
        if "address" in sans:
            san_run_options["ASAN_OPTIONS"] = (
                "halt_on_error=1:abort_on_error=1:detect_leaks=1"
                ":strict_string_checks=1:detect_stack_use_after_return=1"
                ":check_initialization_order=1:strict_init_order=1"
                f":suppressions={os.path.join(self.recipe_folder, 'tools', 'asan_suppressions.supp')}")
        if "undefined" in sans:
            san_run_options["UBSAN_OPTIONS"] = (
                "print_stacktrace=1:halt_on_error=1:abort_on_error=1"
                f":suppressions={os.path.join(self.recipe_folder, 'tools', 'ubsan_suppressions.supp')}")
        if "thread" in sans:
            san_run_options["TSAN_OPTIONS"] = (
                "halt_on_error=1:second_deadlock_stack=1"
                ":ignore_noninstrumented_modules=1"
                f":suppressions={os.path.join(self.recipe_folder, 'tools', 'tsan_suppressions.supp')}")
        if "address" in sans or "thread" in sans:
            # The QV4 JIT is incompatible with ASan (shadow memory vs
            # JIT-allocated code) and crashes under TSan: force the
            # interpreter
            san_run_options["QV4_FORCE_INTERPRETER"] = "1"
        if "realtime" in sans:
            san_run_options["RTSAN_OPTIONS"] = (
                "halt_on_error=1"
                f":suppressions={os.path.join(self.recipe_folder, 'tools', 'rtsan_suppressions.supp')}")
        if san_run_options:
            run_env = VirtualRunEnv(self)
            env = run_env.environment()
            for var, value in san_run_options.items():
                env.define(var, value)
            run_env.generate()

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

        # Conan-tier SBOM fragment consumed by tools/generate_sbom.py and
        # tools/generate_attributions.py. Keep this the only call site: the
        # upstream API is experimental.
        from conan.tools.sbom import cyclonedx_1_6

        fragment = cyclonedx_1_6(self, name=f"zrythm/{self.version}")
        fragment_path = os.path.join(
            self.generators_folder, "zrythm-conan-sbom.cdx.json"
        )
        with open(fragment_path, "w") as f:
            json.dump(fragment, f, indent=2)
        self.output.info(f"Conan SBOM fragment written to {fragment_path}")

    def validate(self):
        check_min_cppstd(self, "23")

        san_opt = str(self.options.get_safe("sanitizer", "none"))
        profile_san_opt = self._profile_sanitizer_option(
            self.settings.get_safe("compiler.sanitizer") or "")

        if san_opt != "none" and profile_san_opt and san_opt != profile_san_opt:
            raise ConanInvalidConfiguration(
                f"the 'sanitizer' option ('{san_opt}') conflicts with the "
                f"profile's compiler.sanitizer setting ('{profile_san_opt}'); "
                "select the sanitizer in one place only, or make both agree")

        # ThreadSanitizer requires dependencies built with TSan too:
        # uninstrumented Qt aborts the TSan runtime outright (its
        # pthread_clockjoin_np-based thread join is not intercepted by TSan),
        # and races inside other uninstrumented dependencies go undetected.
        # Only the profile setting drives the dependencies' package IDs, so
        # the option alone must not be used to request a TSan build.
        if "thread" in (profile_san_opt or san_opt).split("_") and not profile_san_opt:
            raise ConanInvalidConfiguration(
                "ThreadSanitizer builds require the compiler.sanitizer=Thread "
                "profile setting so that dependencies are instrumented too "
                "(e.g. -pr:h clang_tsan); the 'sanitizer' option alone only "
                "instruments Zrythm itself")
